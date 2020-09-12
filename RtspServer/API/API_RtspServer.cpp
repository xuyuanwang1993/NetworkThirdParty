#define _GLIBCXX_USE_C99 1
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
#ifdef VSPROJECT_EXPORTS
#include "pch.h"
#endif // VSPROJECT_EXPORTS
using namespace micagent;
void  Api_rtsp_server::Api_Rtsp_Server_Init_And_Start(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint16_t port)
{
    if(!handle||port<554)
    {
        std::cout<<"illegal parameter!"<<std::endl;
        return ;
    }
    if(handle->is_run)
    {
        std::cout<<"server is running!"<<std::endl;
        return ;
    }
    std::shared_ptr<micagent::EventLoop> eventLoop;
    if(!handle->event_loop)
    {
        eventLoop.reset(new micagent::EventLoop(0,0,2000,1));
        handle->event_loop=eventLoop;
    }
    else
    {
        eventLoop=handle->event_loop;
    }
    std::shared_ptr<micagent::rtsp_server> server;
    if(!handle->server)
    {
        server.reset(new micagent::rtsp_server(port));
        server->register_handle(eventLoop.get());
        handle->server=server;
    }
    else
    {
        server=handle->server;
    }

    handle->is_run=true;
    cout<<"success init Rtsp_server!"<<endl;
}
bool Api_rtsp_server::Api_Rtsp_Server_AddAuthorization(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,std::string username,std::string password)
{
    if(!handle||username.empty()||password.empty()||!handle->server)
    {
        std::cout<<"illegal parameter!"<<std::endl;
        return false;
    }
    return handle->server->addAuthorizationInfo(username,password);
}

uint32_t Api_rtsp_server::Api_Rtsp_Server_Add_Stream(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,std::string stream_name,Media_Info media_info)
 {
    if(!handle||!handle->is_run)return INVALID_MediaSessionId;
     if(stream_name.empty()||(media_info.viedo_type==DEFAULT&&media_info.audio_type==DEFAULT))
     {
         std::cout<<"illegal parameter! in"<<__FUNCTION__<<std::endl;
         return INVALID_MediaSessionId;
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
     std::string rtspUrl = "rtsp://" + handle->server->get_ip()+":"+std::to_string(handle->server->get_port())+ "/" + session->getSuffix();
     std::cout<<"play this stream using url: "<<rtspUrl<<"!"<<std::endl;
     return handle->server->addMediaSession(session);
 }
void Api_rtsp_server::Api_Rtsp_Server_Remove_Stream(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id)
{
    if(!handle||!handle->is_run) return;
    handle->server->removeMediaSession(session_id);
}

bool Api_rtsp_server::Api_Rtsp_Push_Frame(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id,const void *tmp_buf,int buf_size,int channel_id,int64_t micro_time_now)
{
    if(!handle||!handle->is_run) return false;
    micagent::AVFrame videoFrame = {0};
    videoFrame.timestamp=micro_time_now;
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
    return handle->server->updateFrame(session_id, s_id, videoFrame);
}
void Api_rtsp_server::Api_Rtsp_Add_Frame_Control(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id,uint32_t frame_rate)
{
    if(!handle||!handle->is_run) return ;
    handle->server->setFrameRate(session_id,frame_rate);
}
void Api_rtsp_server::Api_Rtsp_Set_NewConnection_Callback(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,const micagent::NEW_CONNECTION_CALLBACK &callback)
{
    if(!handle||!handle->server) return ;
    handle->server->setNewRtspConnection(callback);
}
void Api_rtsp_server::Api_Rtsp_Set_MediaSession_Callback(std::shared_ptr<Api_rtsp_server::Rtsp_Handle> handle,const micagent::MEDIA_SESSION_CALLBACK &new_session_cb,\
                                                         const micagent::MEDIA_SESSION_CALLBACK &delate_session_cb)
{
    if(!handle||!handle->server) return ;
    handle->server->resgisterMediaSessionCallback(new_session_cb,delate_session_cb);
}
