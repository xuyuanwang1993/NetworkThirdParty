#define _GLIBCXX_USE_C99 1
#include "tcp_server.h"
#include <iostream>
#include <arpa/inet.h>
#include "MD5.h"
#include "network_util.h"
#include "proxy_protocol.h"
#include "tcp_connection_helper.h"
#include "CJsonObject.hpp"
#include "rtsp_pusher.h"
#include "rtsp_server.h"
#include "aac_source.h"
#include "g711a_source.h"
#include"h264_source.h"
#include "h265_source.h"
#include "file_reader.h"
#include "proxy_server.h"
#include "delay_control.h"
#include "upnpmapper_mode.h"

using namespace std;
using namespace micagent;
using neb::CJsonObject;
// ip port stream_name   file_name mode
#if 0
int main(int argc,char *argv[]){
    Logger::Instance().set_log_to_std(true);
    //Logger::Instance().set_log_path("",to_string(argc));
    Logger::Instance().set_clear_flag(true);
    //Logger::Instance().set_minimum_log_level(LOG_WARNNING);
    shared_ptr<EventLoop> loop(new EventLoop());
    uint16_t server_port=8555;
    uint16_t rtsp_server_port=8554;
    if(argc>=6)
    {//client
        Logger::Instance().register_handle();
        string ip=argv[1];
        string port=argv[2];
        string stream_name=argv[3];
        string  file_name=argv[4];
        string mode=argv[5];
        string file_name2("");
        if(argc==7)file_name2=argv[6];
        shared_ptr<tcp_connection_helper>tcp_helper(tcp_connection_helper::CreateNew(loop));
        shared_ptr<rtsp_pusher> pusher;
        pusher.reset(rtsp_pusher::CreateNew(tcp_helper,(PTransMode)stoi(mode),stream_name,ip,stoul(port),"admin","micagent"));
        shared_ptr<media_session> session(media_session::CreateNew("test"));
        shared_ptr<delay_control_base> delay;
        if(strstr(file_name.c_str(),"h264")!=nullptr){
            session->setMediaSource(channel_0,h264_source::createNew(25));
            delay.reset(new h264_delay_control());
        }
        else {
            session->setMediaSource(channel_0,h265_source::createNew(25));
            delay.reset(new h265_delay_control());
        }
        session->addProxySession(pusher);
        micagent::file_reader_base file(file_name);
        shared_ptr<micagent::file_reader_base> replace;
        if(!file_name2.empty())replace.reset(new micagent::file_reader_base(file_name2));
        std::thread t([&](){
            uint32_t bufSize = 500000;
            uint8_t *frameBuf = new uint8_t[bufSize];
            AVFrame frame;
            uint32_t frame_count=0;
            bool change=false;
            size_t frameSize=0;
            while(1)
            {
                if(!change)frameSize = file.readFrame(frameBuf, bufSize);
                else {
                    frameSize=replace->readFrame(frameBuf,bufSize);
                }
                if(frameSize > 0)
                {
                    frame.size=static_cast<uint32_t>(frameSize)-4;
                    frame.buffer.reset(new uint8_t[frame.size],std::default_delete<uint8_t[]>());
                    memcpy(frame.buffer.get(),frameBuf+4,frameSize-4);
                    frame.timestamp=delay->block_wait_next_due(frameBuf+4)/1000;
                    pusher->proxy_frame(channel_0,frame);
                    frame_count++;
                    if(!change&&frame_count>1000){
                        if(replace){
                            if(strstr(file_name2.c_str(),"h264")!=nullptr){
                                session->setMediaSource(channel_0,h264_source::createNew(25));
                                delay.reset(new h264_delay_control());
                            }
                            else {
                                session->setMediaSource(channel_0,h265_source::createNew(25));
                                delay.reset(new h265_delay_control());
                            }
                            pusher->modify_proxy_params(session->get_media_source_info());
                            change=true;
                        }
                    }
                    //break;
                }
                else
                {
                    break;
                }
            }
            std::cout<<"exit send_thread"<<std::endl;
        }
        );
        t.join();
    }
    else if(argc!=1){
        Logger::Instance().register_handle();
        string lgd_ip="192.168.2.3";
        if(argc==2)lgd_ip=argv[1];
        upnp_helper::Instance().config(loop,true,lgd_ip);
        upnp_helper::Instance().add_port_task(TCP,rtsp_server_port,rtsp_server_port,"rtsp");
        upnp_helper::Instance().add_port_task(TCP,server_port,server_port,"rtsp_proxy");
        upnp_helper::Instance().add_port_task(UDP,server_port,server_port,"rtsp_proxy");
        shared_ptr<rtsp_server>server(new rtsp_server(rtsp_server_port));
        server->addAuthorizationInfo("admin","micagent");
        server->register_handle(loop);
        shared_ptr<proxy_server>pro_server(new proxy_server(server_port));
        pro_server->set_rtsp_server(server);
        pro_server->register_handle(loop);
        while (getchar()!='8') continue;
    }
    else {
        Logger::Instance().register_handle();
        while(1){
            shared_ptr<rtsp_server>server(new rtsp_server(rtsp_server_port));
            server->addAuthorizationInfo("admin","micagent");
            server->register_handle(loop);
            shared_ptr<proxy_server>pro_server(new proxy_server(server_port));
            pro_server->set_rtsp_server(server);
            pro_server->register_handle(loop);
            Timer::sleep(1000000);
        }

    }
    while (getchar()!='8') continue;
    loop->stop();
    Logger::Instance().unregister_handle();
    return 0;
}
#else
int main(int argc,char *argv[])
{
    if(argc<2)
    {
        printf("./pro_name client client_num(int) file_name(open_file_path) trans_mode(0-4) ip port\r\n");
        printf("./pro_name server port\r\n");
        exit(EXIT_FAILURE);
    }
    Logger::Instance().register_handle();
    Logger::Instance().set_log_to_std(true);
    //Logger::Instance().set_log_path("",to_string(argc));
    Logger::Instance().set_clear_flag(true);
    //Logger::Instance().set_minimum_log_level(LOG_WARNNING);
    string mode=argv[1];
    shared_ptr<EventLoop> loop(new EventLoop());
    uint16_t rtsp_server_port=8554;
    uint16_t server_port=8555;
    int client_num=50;
    string file_name;
    PTransMode trans=RAW_TCP;
    string ip="127.0.0.1";
    if(mode=="server")
    {
        if(argc>=3){
            server_port=stoul(argv[2])&0xffff;
            if(server_port<554)server_port=8555;
        }
        while(1){
            shared_ptr<rtsp_server>server(new rtsp_server(rtsp_server_port));
            server->addAuthorizationInfo("admin","micagent");
            server->register_handle(loop);
            shared_ptr<proxy_server>pro_server(new proxy_server(server_port));
            pro_server->set_rtsp_server(server);
            pro_server->register_handle(loop);
            //Timer::sleep(1000000);
            while (getchar()!='8') continue;
            break;
        }

    }
    else {
        if(argc>=3){
            client_num=stoi(argv[2]);
            if(client_num<=0)client_num=50;
        }
        if(argc>=4){
            file_name=argv[3];
        }
        if(argc>=5){
            trans=(PTransMode)stoi(argv[4]);
        }
        if(argc>=6){
            ip=argv[5];
        }
        if(argc>=7){
            server_port=stoul(argv[6])&0xffff;
            if(server_port<554)server_port=8555;
        }
        shared_ptr<tcp_connection_helper>tcp_helper(tcp_connection_helper::CreateNew(loop));
        struct session{
            int index;
            shared_ptr<rtsp_pusher>pusher;
            shared_ptr<thread>thread_run;
        };
        vector<session>sessions;

        auto func=[&](session& new_session){
        string stream_name("test");
            stream_name+=to_string(new_session.index);
            new_session.pusher.reset(rtsp_pusher::CreateNew(tcp_helper,trans,stream_name,ip,server_port,"admin","micagent"));
            shared_ptr<media_session> session(media_session::CreateNew(stream_name));
            shared_ptr<delay_control_base> delay;
            if(strstr(file_name.c_str(),"h264")!=nullptr){
                session->setMediaSource(channel_0,h264_source::createNew(25));
                delay.reset(new h264_delay_control());
            }
            else {
                session->setMediaSource(channel_0,h265_source::createNew(25));
                delay.reset(new h265_delay_control());
            }
            session->addProxySession(new_session.pusher);
            micagent::file_reader_base file(file_name);
            shared_ptr<micagent::file_reader_base> replace;
            uint32_t bufSize = 500000;
            uint8_t *frameBuf = new uint8_t[bufSize];
            AVFrame frame;
            bool change=false;
            size_t frameSize=0;
            while(1)
            {
                if(!change)frameSize = file.readFrame(frameBuf, bufSize);
                else {
                    frameSize=replace->readFrame(frameBuf,bufSize);
                }
                if(frameSize > 0)
                {
                    frame.size=static_cast<uint32_t>(frameSize)-4;
                    frame.buffer.reset(new uint8_t[frame.size],std::default_delete<uint8_t[]>());
                    memcpy(frame.buffer.get(),frameBuf+4,frameSize-4);
                    frame.timestamp=delay->block_wait_next_due(frameBuf+4)/1000;
                    new_session.pusher->proxy_frame(channel_0,frame);
                }
                else
                {
                    break;
                }
            }
            std::cout<<"exit send_thread"<<std::endl;
        };
        for(int i=0;i<client_num;i++)
        {
            session new_session;
            new_session.index=i;
            new_session.thread_run.reset(new thread(std::bind(func,new_session)));
            sessions.push_back(move(new_session));
        }
        while (getchar()!='8') continue;
        for(auto i:sessions)
        {
            if(i.thread_run&&i.thread_run->joinable())i.thread_run->join();
        }
    }
    loop->stop();
    Logger::Instance().unregister_handle();
    return 0;
}
#endif
