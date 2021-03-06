#include "rtsp_server.h"
#include"rtsp_connection.h"
#include "network_util.h"
using namespace micagent;
rtsp_server::rtsp_server(uint16_t listen_port,uint32_t netinterface_index):tcp_server (listen_port,netinterface_index),m_remove_timer_id(INVALID_TIMER_ID),\
m_new_connection_callback(nullptr),m_notice_client_nums_callback(nullptr),m_new_media_session_callback(nullptr),m_delete_media_session_callback(nullptr)
{

}
MediaSessionId rtsp_server::addMediaSession(shared_ptr<media_session>session)
{
    if(m_notice_client_nums_callback){
        auto loop=m_loop;
        auto cb=m_notice_client_nums_callback;
        session->setNoticeClientNumsCallback([loop,cb](uint32_t stream_token,uint32_t client_num){
            auto strong_loop=loop.lock();
            if(strong_loop)
            {
                strong_loop->add_trigger_event([cb,stream_token,client_num](){
                    cb(stream_token,client_num);
                });
            }
        });
    }
    unique_lock<mutex>locker(m_session_mutex);
    do{
        if(!session)break;
        auto ret=session->getSessionid();
        auto suffix=session->getSuffix();
        if(m_suffix_map.find(suffix)!=end(m_suffix_map)||m_session_map.find(ret)!=end(m_session_map))break;
        MICAGENT_LOG(LOG_WARNNING," dev %s mac %s example:play rtsp stream using url  rtsp://%s:%hu/%s ",m_net_info.dev_name.c_str(),m_net_info.mac.c_str(),\
        m_net_info.ip.c_str(),get_port(),suffix.c_str());
        m_session_map.emplace(ret,session);
        m_suffix_map.emplace(suffix,ret);
        string rtsp_url="rtsp://";
        rtsp_url+=m_net_info.ip+":"+to_string(get_port())+"/"+suffix;
        if(m_new_media_session_callback)
        {
            if(locker.owns_lock())locker.owns_lock();
            m_new_media_session_callback(rtsp_url);
        }
        return ret;
    }while(0);
    return INVALID_MediaSessionId;
}
void rtsp_server::removeMediaSession(MediaSessionId id)
{
    unique_lock<mutex>locker(m_session_mutex);
    auto iter=m_session_map.find(id);
    if(iter!=end(m_session_map)){
        auto session=iter->second;
        MICAGENT_LOG(LOG_ERROR,"remove stream %s!",session->getSuffix().c_str());
        if(session)m_suffix_map.erase(session->getSuffix());
        m_session_map.erase(iter);
        string rtsp_url="rtsp://";
        rtsp_url+=m_net_info.ip+":"+to_string(get_port())+"/"+session->getSuffix();
        if(m_delete_media_session_callback)
        {
            if(locker.owns_lock())locker.owns_lock();
            m_delete_media_session_callback(rtsp_url);
        }
    }
}
void rtsp_server::removeMediaSession(string suffix)
{
    unique_lock<mutex>locker(m_session_mutex);
    auto iter=m_suffix_map.find(suffix);
    if(iter!=end(m_suffix_map)){
        MICAGENT_LOG(LOG_ERROR,"remove stream %s!",suffix.c_str());
        m_session_map.erase(iter->second);
        m_suffix_map.erase(iter);
        string rtsp_url="rtsp://";
        rtsp_url+=m_net_info.ip+":"+to_string(get_port())+"/"+suffix;
        if(m_delete_media_session_callback)
        {
            if(locker.owns_lock())locker.owns_lock();
            m_delete_media_session_callback(rtsp_url);
        }
    }
}
bool rtsp_server::updateFrame(MediaSessionId session_id,MediaChannelId id,const AVFrame &frame)
{
//safety_check
    if(frame.size>MAX_MEDIA_FRAME_SIZE)return false;
    shared_ptr<media_session>session_ptr(nullptr);
    {
        lock_guard<mutex>locker(m_session_mutex);
        auto iter=m_session_map.find(session_id);
        if(iter==end(m_session_map))return false;
        session_ptr=iter->second;
        if(!session_ptr)return false;
    }
    return session_ptr->updateFrame(id,frame);
}
void rtsp_server::setFrameRate(MediaSessionId session_id,uint32_t frame_rate)
{
    if(frame_rate<MIN_FRAME_RATE)frame_rate=MIN_FRAME_RATE;
    lock_guard<mutex>locker(m_session_mutex);
    auto iter=m_session_map.find(session_id);
    if(iter!=m_session_map.end()){
        if(iter->second)iter->second->setFrameRate(frame_rate);
    }
}
shared_ptr<tcp_connection>rtsp_server::new_connection(SOCKET fd)
{
    return make_shared<rtsp_connection>(fd,this);
}
bool rtsp_server::addProxySession(const string &url_sufix,shared_ptr<proxy_session_base>session)
{
    auto session_ptr=lookMediaSession(url_sufix);
    if(!session_ptr||!session)return false;
    else {
        lock_guard<mutex>locker(m_session_mutex);
        session_ptr->addProxySession(session);
        return true;
    }
}
void rtsp_server::changeRtspStreamSource(const string &url_sufix, MediaChannelId channel_id, shared_ptr<media_source>source)
{
    MICAGENT_LOG(LOG_ERROR,"change stream %s!",url_sufix.c_str());
    auto session_ptr=lookMediaSession(url_sufix);
    if(!session_ptr||!source)return ;
    else {
        lock_guard<mutex>locker(m_session_mutex);
        session_ptr->setMediaSource(channel_id,source);
    }
}
void rtsp_server::remove_invalid_connection()
{
    lock_guard<mutex>locker(m_mutex);
    auto event_loop=m_loop.lock();
    auto time_now=Timer::getTimeNow();
    for(auto iter=m_connections.begin();iter!=m_connections.end();){
        auto time_diff=rtsp_connection::diff_alive_time(dynamic_cast<rtsp_connection *>(iter->second.get()),time_now);
        if(time_diff>CONNECTION_TIME_OUT){
            MICAGENT_LOG(LOG_FATALERROR,"%d   is timeout for %ld",iter->first,time_diff);
            if(event_loop)iter->second->unregister_handle(event_loop.get());
            m_connections.erase(iter++);
        }
        else {
            iter++;
        }
    }
}
shared_ptr<media_session> rtsp_server::lookMediaSession(const std::string& suffix)
{
    lock_guard<mutex>locker(m_session_mutex);
    auto iter=m_suffix_map.find(suffix);
    do{
        if(iter==end(m_suffix_map))break;
        auto session_iter=m_session_map.find(iter->second);
        if(session_iter==end(m_session_map))break;
        return session_iter->second;
    }while(0);
    return nullptr;
}
shared_ptr<media_session> rtsp_server::lookMediaSession(MediaSessionId sessionId)
{
    lock_guard<mutex>locker(m_session_mutex);
    auto session_iter=m_session_map.find(sessionId);
    if(session_iter==end(m_session_map))return nullptr;
    return session_iter->second;
}
