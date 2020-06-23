#include "API_RtspServer.h"
#include <iostream>
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
#ifdef VSPROJECT_EXPORTS
#include "pch.h"
#endif // VSPROJECT_EXPORTS
using namespace micagent;
void  Api_rtsp_server::Api_Rtsp_Server_Init_And_Start(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint16_t port)
{
    auto rtsp_handle=handle.lock();
    if(!rtsp_handle||port<554)
    {
        std::cout<<"illegal parameter!"<<std::endl;
        return ;
    }
    if(rtsp_handle->is_run)
    {
        std::cout<<"server is running!"<<std::endl;
        return ;
    }
    std::shared_ptr<micagent::EventLoop> eventLoop;
    if(!rtsp_handle->event_loop)
    {
        eventLoop.reset(new micagent::EventLoop());
        rtsp_handle->event_loop=eventLoop;
    }
    else
    {
        eventLoop=rtsp_handle->event_loop;
    }
    std::shared_ptr<micagent::rtsp_server> server;
    if(!rtsp_handle->server)
    {
        server.reset(new micagent::rtsp_server(port));
        server->register_handle(rtsp_handle->event_loop.get());
        rtsp_handle->server=server;
    }
    else
    {
        server=rtsp_handle->server;
    }
    rtsp_handle->is_run=true;
    cout<<"success init Rtsp_server!"<<endl;
}
bool Api_rtsp_server::Api_Rtsp_Server_AddAuthorization(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,std::string username,std::string password)
{
    auto rtsp_handle=handle.lock();
    if(!rtsp_handle||username.empty()||password.empty()||!rtsp_handle->server)
    {
        std::cout<<"illegal parameter!"<<std::endl;
        return false;
    }
    return rtsp_handle->server->addAuthorizationInfo(username,password);
}

uint32_t Api_rtsp_server::Api_Rtsp_Server_Add_Stream(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,std::string stream_name,Media_Info media_info)
 {
    auto rtsp_handle=handle.lock();
    if(!rtsp_handle||!rtsp_handle->is_run)return 0;
     if(stream_name.empty()||(media_info.viedo_type==DEFAULT&&media_info.audio_type==DEFAULT))
     {
         std::cout<<"illegal parameter! in"<<__FUNCTION__<<std::endl;
         return 0;
     }
     auto session = micagent::media_session::CreateNew(stream_name.c_str());
     if(media_info.viedo_type==H264)
     {
         session->setMediaSource(micagent::channel_0, micagent::h264_source::createNew(media_info.frameRate));
     }
     else if(media_info.viedo_type==H265)
     {
        session->setMediaSource(micagent::channel_0, micagent::h265_source::createNew(media_info.frameRate));
     }
     else
     {
         std::cout<<"video type not supported!"<<std::endl;
     }
     if(media_info.audio_type==AAC)
     {
         session->setMediaSource(micagent::channel_1, micagent::aac_source::createNew(media_info.sampleRate,media_info.channels,media_info.hasADTS));
     }
     else if(media_info.audio_type==PCMA)
     {
         session->setMediaSource(micagent::channel_1, micagent::g711a_source::createNew());
     }
     else
     {
         std::cout<<"audio type not supported!"<<std::endl;
     }
     std::string rtspUrl = "rtsp://" + rtsp_handle->server->get_ip()+":"+std::to_string(rtsp_handle->server->get_port())+ "/" + session->getSuffix();
     std::cout<<"play this stream using url: "<<rtspUrl<<"!"<<std::endl;
     return rtsp_handle->server->addMediaSession(session);
 }
void Api_rtsp_server::Api_Rtsp_Server_Remove_Stream(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id)
{
    auto rtsp_handle=handle.lock();
    if(!rtsp_handle||!rtsp_handle->is_run) return;
    rtsp_handle->server->removeMediaSession(session_id);
}

bool Api_rtsp_server::Api_Rtsp_Push_Frame(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id,const void *tmp_buf,int buf_size,int channel_id)
{
    auto rtsp_handle=handle.lock();
    if(!rtsp_handle||!rtsp_handle->is_run) return false;
    micagent::AVFrame videoFrame = {0};
    auto buf=static_cast<const char *>(tmp_buf);
    if(buf_size>4&&buf[0]==0x00&&buf[1]==0x00&&buf[2]==0x00&&buf[3]==0x01)
    {
        videoFrame.size = buf_size-4;
        videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
        memcpy(videoFrame.buffer.get(), buf+4, videoFrame.size);
    }
    else if(buf_size>3&&buf[0]==0x00&&buf[1]==0x00&&buf[2]==0x01)
    {
        videoFrame.size = buf_size-3;
        videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
        memcpy(videoFrame.buffer.get(), buf+3, videoFrame.size);
    }
    else {
        videoFrame.size = buf_size;
        videoFrame.buffer.reset(new uint8_t[videoFrame.size]);
        memcpy(videoFrame.buffer.get(), buf, videoFrame.size);
    }
    micagent::MediaChannelId s_id=channel_0;
    if(channel_id!=0)s_id=channel_1;
    return rtsp_handle->server->updateFrame(session_id, s_id, videoFrame);
}
void Api_rtsp_server::Api_Rtsp_Add_Frame_Control(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id,uint32_t frame_rate)
{
    auto rtsp_handle=handle.lock();
    if(!rtsp_handle||!rtsp_handle->is_run) return ;
    rtsp_handle->server->setFrameRate(session_id,frame_rate);
}
