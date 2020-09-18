#include <string>
#include<algorithm>
#include"event_loop.h"
#include "buffer_handle.h"
#include<iostream>
#include<netdb.h>
#include"tcp_server.h"
#include "http_request.h"
#include"http_response.h"
#include "unix_socket_helper.h"
#include "tcp_client_example.h"
#include <signal.h>
using namespace std;
using namespace micagent;
#define DECLARE_TEST(name) void name##_test()
#define RUN_TEST(name) name##_test()
#define TEST_WAIT {mutex test;\
    unique_lock<mutex>locker(test);\
    condition_variable con;\
    con.wait(locker);}

DECLARE_TEST(buffer_handle);
DECLARE_TEST(tcp_server);
DECLARE_TEST(http_request);
DECLARE_TEST(http_response);
void unix_socket_test(int argc,char *argv[]);
void tcp_client_test(int argc,char *argv[]);
int main(int argc,char *argv[])
{
    tcp_client_test(argc,argv);
    //unix_socket_test(argc,argv);
    //    SOCKET fd=NETWORK.build_socket(UDP);
    //    sockaddr_in addr;
    //    addr.sin_family=AF_INET;
    //    addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    //    addr.sin_port=htons(51234);
    //    const int buf_size=20000;
    //    char buf[buf_size]={0};
    //    while(1)
    //    {
    //        auto ret=NETWORK.time_out_sendto(fd,buf,buf_size,0,(const struct sockaddr *)&addr,sizeof (addr),1);
    //        printf("ret %d \r\n",ret);
    //        usleep(100);
    //    }
    //    Network_Util::DefaultInit();
    //    Logger::Instance().register_handle();
    //    Logger::Instance().set_log_to_std(true);
    //    //RUN_TEST(buffer_handle);
    //    //RUN_TEST(tcp_server);
    //    //RUN_TEST(http_request);
    //    RUN_TEST(http_response);
    //    Logger::Instance().unregister_handle();
    return 0;
}
//DECLARE_TEST(buffer_handle)
//{
//    buffer_handle buf(128,nullptr,false);
//    EventLoop loop(128);
//    auto sock=Network_Util::Instance().build_socket(TCP);
//    Network_Util::Instance().set_reuse_addr(sock);
//    Network_Util::Instance().bind(sock,10001);
//    Network_Util::Instance().connect(sock,"192.168.2.199",8554);
//    Network_Util::Instance().make_noblocking(sock);
//    ChannelPtr new_channel(new Channel(sock));
//    Logger::Instance().set_log_to_std(true);
//    new_channel->enableReading();
//    new_channel->setReadCallback([&](Channel *chn){
//        auto len=buf.read_fd(chn->fd());
//        if(len>0){
//            chn->enableWriting();
//            loop.updateChannel(chn);
//            return true;
//        }
//        else {
//            return false;
//        }
//    });
//    new_channel->setWriteCallback([&](Channel *chn){
//        auto ret=buf.send_fd(chn->fd());
//        chn->disableWriting();
//        loop.updateChannel(chn);
//        return  ret>=0;
//    });
//    new_channel->setCloseCallback([&](Channel *chn){
//        exit(-1);
//        return false;
//    });
//    loop.updateChannel(new_channel);
//    TEST_WAIT;
//}
//DECLARE_TEST(tcp_server)
//{
//    EventLoop loop;
//    shared_ptr<tcp_server>server=make_shared<tcp_server>(8554);
//    server->register_handle(&loop);
//    TEST_WAIT;
//}
//DECLARE_TEST(http_request)
//{
//    http_request test1("/test");
//    test1.set_head("User-Agent","Micagent");
//    test1.set_head("Date",Logger::Instance().get_local_time());
//    test1.set_body("hello!","test");
//    MICAGENT_MARK("build  1\r\n%s",test1.build_http_packet(true).c_str());
//    string test_packet="POST /test_api HTTP/1.1\r\nHost : micagent\r\nContent-Length : 8\r\n\r\nok111111";
//    test1.update(test_packet.c_str(),test_packet.length());
//    MICAGENT_MARK("build  2\r\n%s",test1.build_http_packet(true).c_str());

//    std::thread t([&test1](){
//        MICAGENT_MARK("body for thread get:%s\r\n",test1.get_body(5000).c_str());
//        MICAGENT_MARK("Content-Type : %s\r\n",test1.get_head_info("Content-Type").c_str());
//    });
//    MICAGENT_MARK("start input!");
//    for(int i=0;i<test_packet.length();i++){
//        test1.update(test_packet.c_str()+i,1);
//        putc(test_packet[i],stderr);
//        Timer::sleep(10);
//    }
//    MICAGENT_MARK("end input!");
//    t.join();
//}
//DECLARE_TEST(http_response)
//{
//    http_response test1("/test");
//    test1.set_head("User-Agent","Micagent");
//    test1.set_head("Date",Logger::Instance().get_local_time());
//    test1.set_body("hello!","test");
//    MICAGENT_MARK("build  1\r\n%s",test1.build_http_packet(true).c_str());
//    string test_packet="HTTP/1.1 200 OK\r\nHost : micagent\r\nContent-Length : 8\r\n\r\nok111111";
//    test1.update(test_packet.c_str(),test_packet.length());
//    MICAGENT_MARK("build  2\r\n%s",test1.build_http_packet(true).c_str());

//    std::thread t([&test1](){
//        MICAGENT_MARK("body for thread get:%s\r\n",test1.get_body(5000).c_str());
//        MICAGENT_MARK("Content-Type : %s\r\n",test1.get_head_info("Content-Type").c_str());
//        auto info=test1.get_status();
//        MICAGENT_MARK("status : %s %s\r\n",info.first.c_str(),info.second.c_str());
//    });
//    MICAGENT_MARK("start input!");
//    for(int i=0;i<test_packet.length();i++){
//        test1.update(test_packet.c_str()+i,1);
//        putc(test_packet[i],stderr);
//        Timer::sleep(10);
//    }
//    MICAGENT_MARK("end input!");
//    t.join();
//}
void unix_socket_test(int argc,char *argv[])
{
    auto helper_func=[](){
        printf("option with <option>\r\n");
        printf("1-------------------------udp_echo_server\r\n");
        printf("2-------------------------udp_client\r\n");
        printf("3-------------------------tcp_server\r\n");
        printf("4-------------------------tcp_client\r\n");
        exit(0);
    };
    if(argc<2)
    {
        helper_func();
    }
    int opt=stoi(argv[1]);
    if(opt<1||opt>4)
    {
        helper_func();
    }
    const char *const server_domain="/tmp/test_server.domain";
    const char *const udp_client_domain_base="/tmp/test_client";
    string udp_client_domain=string(udp_client_domain_base)+to_string(getpid())+".domain";
    int path_len=108;
    if(opt==1)
    {//udp echo server
        printf("udp echo server!\r\n");
        unix_dgram_socket socket(server_domain);
        socket.build();
        char buf[4096]={0};
        while(1)
        {
            auto len=socket.recv(buf,4096);
            if(len<=path_len)continue;
            string message(buf+path_len,len-path_len);
            string from(buf,path_len);
            printf("recv %s from %s\r\n",message.c_str(),from.c_str());
            socket.send(message.c_str(),message.length(),from);
        }
    }
    else if (opt==2) {
        printf("udp echo client!\r\n");
        unix_dgram_socket socket(udp_client_domain);
        socket.build();
        socket.set_peer_path(server_domain);
        char buf[4096]={0};
        strncpy(buf,udp_client_domain.c_str(),path_len);
        while(1)
        {
            printf("input the message you want to send:");
            cin.getline(buf+path_len,4096-path_len);
            socket.send(buf,4096);
            auto len =socket.recv(buf+path_len,4096-path_len);
            printf("recv from peer:%s\r\n",string(buf+path_len,len).c_str());
        }
    }
    else if (opt==3) {
        signal(SIGCHLD,SIG_IGN);
        printf("tcp echo server!\r\n");
        auto handle_func=[](SOCKET fd){
            char buf[4096];
            unix_stream_socket socket;
            socket.reset(fd);
            while(1)
            {
                auto len=socket.recv(buf,4096);
                if(len<=0)break;
                string message(buf,len);
                printf("recv len %d  : %s\r\n",len,message.c_str());
                if(socket.send(buf,len)<=0)break;
            }
            NETWORK.close_socket(fd);
        };
        unix_stream_socket server_socket(server_domain);
        server_socket.build();
        server_socket.listen();
        while(1)
        {
            auto fd=server_socket.aacept();
            auto pid=fork();
            if(pid==0)
            {
                handle_func(fd);
                printf("unix_stream_socket connecttion %d exit!\n",fd);
            }
            else {
                NETWORK.close_socket(fd);
            }
        }
    }
    else if (opt==4) {
        printf("tcp echo client!\r\n");
        unix_stream_socket socket;
        socket.build();
        if(!socket.connect(server_domain))
        {
            perror("connect failed!");
            exit(EXIT_FAILURE);
        }
        char buf[4096]={0};
        while(1)
        {
            printf("input the message you want to send:");
            cin.getline(buf,4096);
            string send_str(buf);
            if(socket.send(send_str.c_str(),send_str.length())<=0)break;
            auto len =socket.recv(buf,4096);
            if(len<=0)break;
            printf("recv from peer:%s\r\n",string(buf,len).c_str());
        }
    }
}
void tcp_client_test(int argc,char *argv[])
{
    Logger::Instance().set_log_to_std(true);
    Logger::Instance().register_handle();
    if(argc==1)
    {
        shared_ptr <EventLoop>loop(new EventLoop());
        usleep(20);
        shared_ptr<tcp_server> server(new tcp_server(20000));
        server->register_handle(loop);
        while(getchar()!='8')continue;
        loop->stop();
    }
    else {
        shared_ptr <EventLoop>loop(new EventLoop());
        usleep(20);
        string ip=argv[1];
        uint16_t port=stoul(argv[2])&0xffff;
        shared_ptr<tcp_connection_helper>helper(tcp_connection_helper::CreateNew(loop));
        shared_ptr<tcp_client_example> client( new tcp_client_example (helper,ip,port));
        client->set_example();
        client->set_connect_callback([](){
            MICAGENT_BACKTRACE("connect success!");
        });
        client->open_connection();
        while(1){
            char buf[1024];
            memset(buf,0,1024);
            cin.getline(buf,1024);
            string message(buf);
            if(message=="stop")break;
            message+="\r\n\r\n";
            client->send_message(message.c_str(),message.length());
        }
        loop->stop();
    }

    Logger::Instance().unregister_handle();
}
