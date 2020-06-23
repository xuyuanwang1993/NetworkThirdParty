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
using namespace std;
using namespace micagent;
using neb::CJsonObject;
// ip port stream_name   file_name mode
int main(int argc,char *argv[]){
    Logger::Instance().set_log_to_std(true);
    Logger::Instance().register_handle();
    shared_ptr<EventLoop> loop(new EventLoop());
    if(argc==6)
    {//client
        string ip=argv[1];
        string port=argv[2];
        string stream_name=argv[3];
        string  file_name=argv[4];
        string mode=argv[5];
        auto tcp_helper=tcp_connection_helper::CreateNew(loop.get());
        shared_ptr<rtsp_pusher> pusher;
        pusher.reset(rtsp_pusher::CreateNew(tcp_helper,(PTransMode)stoi(mode),stream_name,ip,stoul(port)));
        shared_ptr<media_session> session(media_session::CreateNew("test"));
        if(strstr(file_name.c_str(),"h264")!=nullptr){
            session->setMediaSource(channel_0,h264_source::createNew(25));
        }
        else {
            session->setMediaSource(channel_0,h265_source::createNew(25));
        }
        session->addProxySession(pusher);
        micagent::file_reader_base file(file_name);
    std::thread t([&](){
        int bufSize = 500000;
        uint8_t *frameBuf = new uint8_t[bufSize];
        AVFrame frame;
        while(1)
        {
            auto timePoint = std::chrono::steady_clock::now();
            auto time_now=std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
            bool bEndOfFrame;
            int frameSize = file.readFrame(frameBuf, bufSize);
            if(frameSize > 0)
            {
                frame.size=frameSize-4;
                frame.buffer.reset(new uint8_t[frame.size]);
                memcpy(frame.buffer.get(),frameBuf+4,frameSize-4);
                pusher->proxy_frame(channel_0,frame);
                    //break;
            }
            else
            {
                break;
            }
            auto timePoint2 = std::chrono::steady_clock::now();
            auto time_now2=std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
            int sleep_time=40-time_now2+time_now;
            if(sleep_time>0)Timer::sleep(sleep_time);
        }
        std::cout<<"exit send_thread"<<std::endl;
    }
        );
    t.join();
    }
    else {
        shared_ptr<rtsp_server>server(new rtsp_server(58554));
        server->register_handle(loop.get());
        shared_ptr<proxy_server>pro_server(new proxy_server());
        pro_server->set_rtsp_server(server);
        pro_server->register_handle(loop.get());
        while (getchar()!='8') continue;
    }
    while (getchar()!='8') continue;
    loop->stop();
    Logger::Instance().unregister_handle();
    return 0;
}
