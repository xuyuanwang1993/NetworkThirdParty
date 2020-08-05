#include "c_log.h"
#include "network_util.h"
#include "proxy_protocol.h"
#include <random>
#include<iostream>
using namespace micagent;
using namespace std;
#define  SERVER_PORT 25000
#define CLIENT_PORT (SERVER_PORT+1)
#define frame_base_len 4
#define frame_max_len 100000
#define send_interval  20   //ms
int main(int argc ,char *argv[])
{// param 1   send /recv
    //param 2  raw_udp  raw_tcp raw_hybrid g_tcp   g_hybrid
    //param 3  recv_ip
    Logger::Instance().set_log_to_std(true);
    Logger::Instance().set_log_file_size(10*1024*1024);
    if(argc>1)cout<<Logger::Instance().set_log_path(string(),(string("test")+argv[1]).c_str())<<endl;
    Logger::Instance().register_handle();
    if(argc<3){
        MICAGENT_LOG(LOG_INFO,"send /recv  raw_udp/raw_tcp/raw_hybrid/g_tcp/g_hybrid [ip]");
        exit(EXIT_FAILURE);
    }
    string user=argv[1];
    string mode=argv[2];
    string ip;
    if(user=="client"){
        if(argc<4){
            MICAGENT_LOG(LOG_INFO,"send /recv  raw_udp/raw_tcp/raw_hybrid/g_tcp/g_hybrid [ip]");
            exit(EXIT_FAILURE);
        }
        else {
            ip=argv[3];
        }
    }
    SOCKET tcp_fd=INVALID_SOCKET;
    SOCKET udp_fd=NETWORK.build_socket(UDP);
    if(user=="client"){
        NETWORK.bind(udp_fd,CLIENT_PORT);
    }
    else {
        NETWORK.bind(udp_fd,SERVER_PORT);
    }
    NETWORK.set_send_buf_size(udp_fd,32*1024);
    struct sockaddr_in addr={0};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr(ip.c_str());
    addr.sin_port=htons(SERVER_PORT);
    auto tcp_callback=[&](const void *buf,uint32_t buf_len){
        if(tcp_fd==INVALID_SOCKET)return true;
        return send(tcp_fd,buf,buf_len,0)==buf_len;
    };
    auto udp_callback=[&](const void *buf,uint32_t buf_len){
        auto ret=sendto(udp_fd,buf,buf_len,0,(const struct sockaddr *)&addr,sizeof (sockaddr_in));
        return ret==buf_len;
    };
    auto frame_callback=[&](shared_ptr<ProxyFrame> frame){
        static uint16_t last_frame_seq=0;
        static uint64_t counts=0;
        if(frame->frame_seq-last_frame_seq!=1)
            MICAGENT_LOG(LOG_INFO,"timestamp %u  %u   seq %hu ",frame->timestamp,frame->data_len,frame->frame_seq-last_frame_seq);
        MICAGENT_LOG(LOG_ERROR,"recv sum counts %llu",counts++);
        last_frame_seq=frame->frame_seq;
    };
    shared_ptr<ProxyInterface> interface;
    if(mode=="raw_udp"){
        interface.reset(new ProxyInterface(0,RAW_UDP,tcp_callback,udp_callback,frame_callback));
    }
    else if (mode=="raw_tcp") {
        interface.reset(new ProxyInterface(0,RAW_TCP,tcp_callback,udp_callback,frame_callback));
        if(user=="client"){

            tcp_fd=NETWORK.build_socket(TCP);
            NETWORK.bind(tcp_fd,CLIENT_PORT);
            NETWORK.connect(tcp_fd,ip,SERVER_PORT);
        }
    }
    else if (mode=="raw_hybrid") {
        interface.reset(new ProxyInterface(0,RAW_HYBRID,tcp_callback,udp_callback,frame_callback));
        if(user=="client"){
            tcp_fd=NETWORK.build_socket(TCP);
            NETWORK.bind(tcp_fd,CLIENT_PORT);
            NETWORK.connect(tcp_fd,ip,SERVER_PORT);
        }
    }
    else if (mode=="g_tcp") {
        interface.reset(new ProxyInterface(0,GRADED_TCP,tcp_callback,udp_callback,frame_callback));
        if(user=="client"){
            tcp_fd=NETWORK.build_socket(TCP);
            NETWORK.bind(tcp_fd,CLIENT_PORT);
            NETWORK.connect(tcp_fd,ip,SERVER_PORT);
        }
    }
    else {
        interface.reset(new ProxyInterface(0,GRADED_HYBRID,tcp_callback,udp_callback,frame_callback));
        if(user=="client"){
            tcp_fd=NETWORK.build_socket(TCP);
            NETWORK.bind(tcp_fd,CLIENT_PORT);
            NETWORK.connect(tcp_fd,ip,SERVER_PORT);
        }
    }
    if(user=="server"){
        thread t([&](){
            SOCKET listen_fd=NETWORK.build_socket(TCP);
            NETWORK.bind(listen_fd,SERVER_PORT);
            NETWORK.listen(listen_fd,1);
            tcp_fd=NETWORK.accept(listen_fd);
            NETWORK.close_socket(listen_fd);
        });
        t.detach();
    }
    else {
        NETWORK.connect(udp_fd,ip,SERVER_PORT);
        //开启客户端发送线程
        thread send_thread([&](){
            int seq=0;
            char frame_t[]={0x68,0x67,0x65,0x61};
            random_device rd;
            uint64_t counts=0;
            while(1){
                uint32_t frame_len=rd()%frame_max_len+frame_base_len;
                shared_ptr<char []>send_buf(new char[frame_len+1]);
                memset(send_buf.get(),'a',frame_len);
                auto test=(seq++)%50;
                if(test==0)send_buf.get()[0]=frame_t[2];
                else if (test==49) {
                    send_buf.get()[0]=frame_t[1];
                }
                else if (test==48) {
                    send_buf.get()[0]=frame_t[0];
                }
                else {
                    send_buf.get()[0]=frame_t[3];
                }
                shared_ptr<ProxyFrame>frame(new ProxyFrame(send_buf.get(),frame_len,PH264,0,0));
                if(interface){
                    if(! interface->send_frame(frame)){
                        MICAGENT_LOG(LOG_INFO,"send error!");
                        break;
                    }
                    MICAGENT_LOG(LOG_INFO,"send %u %u %llu",frame->timestamp,frame->data_len,counts++);
                }
                this_thread::sleep_for(chrono::milliseconds(send_interval));
            }
        });
        send_thread.detach();
    }
    queue<pair<const char *,int >>buf_cache;
    mutex m_cache_mutex;
    condition_variable m_cache_conn;
    thread t2([&](){
        const int buf_size=2*1024*1024;
        shared_ptr<char[]>buf(new char[buf_size]);
        int offset=0;
        char *begin_test=buf.get();
        socklen_t len=sizeof (sockaddr_in);
        while(1)
        {
            if(buf_size-offset<2048)offset=0;
            auto ret=recvfrom(udp_fd,begin_test+offset,2048,0,0,0);
            if(ret>0)
            {
                lock_guard<mutex>locker(m_cache_mutex);
                buf_cache.push(make_pair(begin_test+offset,ret));
                offset+=ret;
                m_cache_conn.notify_all();
            }
        }
    });
    t2.detach();
    thread t2_helper([&](){
        unique_lock<mutex>locker(m_cache_mutex);
        while(1)
        {
            if(buf_cache.empty()){
                m_cache_conn.wait(locker);
            }
            while(!buf_cache.empty())
            {
                if(interface)interface->protocol_input(buf_cache.front().first,buf_cache.front().second);
                buf_cache.pop();
            }
        }
    });
    t2_helper.detach();
    queue<pair<const char *,int >>buf_cache2;
    mutex m_cache_mutex2;
    condition_variable m_cache_conn2;
    thread t3([&](){
        const int buf_size=2*1024*1024;
        shared_ptr<char[]>buf(new char[buf_size]);
        int offset=0;
        char *begin_test=buf.get();
        while(1)
        {
            if(tcp_fd==INVALID_SOCKET){
                sleep(1);
                continue;
            }
            if(buf_size-offset<1424)offset=0;
            auto ret=recv(tcp_fd,begin_test+offset,1424,0);
            if(ret>0)
            {
                lock_guard<mutex>locker(m_cache_mutex2);
                buf_cache2.push(make_pair(begin_test+offset,ret));
                offset+=ret;
                m_cache_conn2.notify_all();
            }
            else if(ret==0){
                MICAGENT_LOG(LOG_FATALERROR,"%s",strerror(errno));
                exit(0);
            }
        }
    });
    t3.detach();
    thread t3_helper([&](){
        unique_lock<mutex>locker(m_cache_mutex2);
        PStreamParse parse;
        while(1)
        {
            if(buf_cache2.empty()){
                m_cache_conn2.wait(locker);
            }
            while(!buf_cache2.empty())
            {
                if(interface)
                {
                    auto packet=parse.insert_buf(buf_cache2.front().first,buf_cache2.front().second);
                    while(!packet.empty())
                    {
                        interface->protocol_input(packet.front().first.get(),packet.front().second);
                        packet.pop();
                    }

                }
                buf_cache2.pop();
            }
        }
    });
    t3_helper.detach();
    while(getchar()!='8')continue;
    Logger::Instance().unregister_handle();
    return 0;
}
