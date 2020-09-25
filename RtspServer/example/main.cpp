#include "tcp_server.h"
#include "aac_source.h"
#include "g711a_source.h"
#include"h264_source.h"
#include "h265_source.h"
#include <iostream>
#include <arpa/inet.h>
#include "rtsp_helper.h"
#include "MD5.h"
#include "media.h"
#include "network_util.h"
#include "rtsp_server.h"
#include "file_reader.h"
#include "API_RtspServer.h"
#include "delay_control.h"
using namespace std;
using namespace micagent;
int main(int argc,char *argv[]){
    if(argc < 2)
    {
        printf("Usage: %s test.h264\n", argv[0]);
        return 0;
    }
    uint16_t port=8554;
    if(argc==3)port=stoul(argv[2]);
    Logger::Instance().set_log_to_std(true);
    Logger::Instance().register_handle();
#if 0
    EventLoop loop;
    shared_ptr<rtsp_server>server(new rtsp_server(554));
    server->register_handle(&loop);
    server->addAuthorizationInfo("admin","micagent");
    server->addAuthorizationInfo("admin1","micagent1");
    auto session=media_session::CreateNew("test");
    if(strstr(argv[1],"h264")!=nullptr){
        session->setMediaSource(channel_0,h264_source::createNew());
    }
    else {
        session->setMediaSource(channel_0,h265_source::createNew());
    }
    auto session_id=server->addMediaSession(session);
    micagent::file_reader_base file(argv[1]);
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
                if(!server->updateFrame(session_id,channel_0,frame));
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
    server->removeMediaSession(session_id);
#else
    shared_ptr<Api_rtsp_server::Rtsp_Handle>handle(new Api_rtsp_server::Rtsp_Handle);
    Api_rtsp_server::Api_Rtsp_Server_Init_And_Start(handle,port);
    Api_rtsp_server::Api_Rtsp_Set_NewConnection_Callback(handle,[](string ip,uint16_t port,string url){
        MICAGENT_LOG(LOG_BACKTRACE,"ip:%s port:%hu url:%s!",ip.c_str(),port,url.c_str());
    });
    Api_rtsp_server::Api_Rtsp_Set_MediaSession_Callback(handle,[](string url_name){
        MICAGENT_BACKTRACE("%s is created!",url_name.c_str());
    },[](string url_name){
        MICAGENT_BACKTRACE("%s is removed!",url_name.c_str());
    });
    Api_rtsp_server::Api_Rtsp_Server_AddAuthorization(handle,"admin","micagent");
    Api_rtsp_server::Media_Info media_info;
    media_info.frameRate=25;
    delay_control_base *delay=nullptr;
    if(strstr(argv[1],"h264")!=nullptr){
        media_info.viedo_type=Api_rtsp_server::H264;
        delay=new h264_delay_control(media_info.frameRate);
    }
    else {
        media_info.viedo_type=Api_rtsp_server::H265;
        delay=new h265_delay_control(media_info.frameRate);
    }
    auto session_id=Api_rtsp_server::Api_Rtsp_Server_Add_Stream(handle,"test",media_info);
    micagent::file_reader_base file(argv[1]);
    std::thread t([&](){
        int bufSize = 500000;
        uint8_t *frameBuf = new uint8_t[bufSize];
        int send_count=1;
        int64_t micro_time_now=0;
        while(1)
        {
            int frameSize = file.readFrame(frameBuf, bufSize);
            if(frameSize > 0)
            {
                if(delay)
                {
                    micro_time_now=delay->block_wait_next_due(&frameBuf[4]);
                }
                Api_rtsp_server::Api_Rtsp_Push_Frame(handle,session_id,frameBuf,frameSize,0,micro_time_now/1000);
            }
            else
            {
                break;
            }
            send_count++;
            if(send_count%5000000==0)
            {
                Api_rtsp_server::Api_Rtsp_Server_Remove_Stream(handle,session_id);
                session_id=Api_rtsp_server::Api_Rtsp_Server_Add_Stream(handle,"test",media_info);
            }
        }
        std::cout<<"exit send_thread"<<std::endl;
    } );
    t.join();
#endif
    while (getchar()!='8') continue;
    Logger::Instance().unregister_handle();
    return 0;
}
