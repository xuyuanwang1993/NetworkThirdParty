#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H
#include "tcp_server.h"
#include "media_session.h"
#include <unordered_map>
#include <media.h>
namespace micagent {
using namespace std;
using RTSP_NEW_CONNECTION_CALLBACK=function<void(string,uint16_t,string)>;
using RTSP_MEDIA_SESSION_CALLBACK=function<void(string url_name)>;
class rtsp_connection;
class rtsp_server:public tcp_server{
    static constexpr uint32_t MIN_FRAME_RATE=5;
    static constexpr int64_t CONNECTION_TIME_OUT=120*1000;//120s
    static constexpr uint32_t MAX_MEDIA_FRAME_SIZE=10*1024*1024;
    friend class rtsp_connection;
public:
    rtsp_server(uint16_t listen_port=554,uint32_t netinterface_index=UINT32_MAX);
    ~rtsp_server(){}
    /**
     * @brief addMediaSession 添加媒体session
     * @param session
     * @return
     */
    MediaSessionId addMediaSession(shared_ptr<media_session>session);
    /**
     * @brief removeMediaSession 移除媒体session
     * @param id
     */
    void removeMediaSession(MediaSessionId id);
    void removeMediaSession(string suffix);
    bool updateFrame(MediaSessionId session_id,MediaChannelId id,const AVFrame &frame);
    void setFrameRate(MediaSessionId session_id,uint32_t frame_rate=25);
    MediaSessionId find_session_id(const string &suffix){
        lock_guard<mutex>locker(m_session_mutex);
        auto iter=m_suffix_map.find(suffix);
        if(iter!=end(m_suffix_map))return iter->second;
        else return INVALID_MediaSessionId;
    }
    bool addAuthorizationInfo(const std::string & username,const std::string & password)
    {
        MICAGENT_LOG(LOG_INFO,"addAuthorizationInfo: %s  %s",username.c_str(),password.c_str());
        std::lock_guard<std::mutex> locker(m_mtxAccountMap);
        if(username.empty()||password.empty())return false;
        auto iter=m_Account_Map.find(username);
        if(iter!=m_Account_Map.end())return password==iter->second;
        m_Account_Map[username]=password;
        return true;
    }
    void removeAuthorizationInfo(std::string & username,std::string & password)
    {
        std::lock_guard<std::mutex> locker(m_mtxAccountMap);
        if(username.empty()||password.empty())return;
        auto iter=m_Account_Map.find(username);
        if(iter!=std::end(m_Account_Map)&&password==iter->second)m_Account_Map.erase(username);
    }
    bool getAuthorizationPassword(std::string &username,std::string &_password)
    {
        std::lock_guard<std::mutex> locker(m_mtxAccountMap);
        _password="";
        if(username.empty())return true;
        auto iter=m_Account_Map.find(username);
        if(iter!=std::end(m_Account_Map))_password=iter->second;
        else return false;
        return true;
    }
    shared_ptr<tcp_connection>new_connection(SOCKET fd);

    bool addProxySession(const string &url_sufix,shared_ptr<proxy_session_base>session);
    void setNewRtspConnection(const RTSP_NEW_CONNECTION_CALLBACK&cb){
        lock_guard<mutex>locker(m_session_mutex);
        m_new_connection_callback=cb;
    }
    void setNoticeClientNumsCallback(const RTSP_NOTICE_CLIENT_NUMS_CALLBACK &cb){
        lock_guard<mutex>locker(m_session_mutex);
        m_notice_client_nums_callback=cb;
    }
    void resgisterMediaSessionCallback(const RTSP_MEDIA_SESSION_CALLBACK &new_session_cb,\
    const RTSP_MEDIA_SESSION_CALLBACK &delete_session_cb){
        lock_guard<mutex>locker(m_session_mutex);
        m_new_media_session_callback=new_session_cb;
        m_delete_media_session_callback=delete_session_cb;
    }
    void changeRtspStreamSource(const string &url_sufix,MediaChannelId channel_id,shared_ptr<media_source>source);
protected:
    void remove_invalid_connection();
    TimerId m_remove_timer_id;
    void init_server(){
    auto event_loop=m_loop.lock();
        if(!event_loop)return;
        if(m_remove_timer_id!=INVALID_TIMER_ID){
            event_loop->removeTimer(m_remove_timer_id);
        }
        weak_ptr<tcp_server> weak_this=shared_from_this();
        m_remove_timer_id=event_loop->addTimer([weak_this](){
            auto strong=weak_this.lock();
            if(!strong)return false;
            auto server=dynamic_cast<rtsp_server*>(strong.get());
            if(server)server->remove_invalid_connection();
            return true;
        },CONNECTION_TIME_OUT);
    }
private:
     shared_ptr<media_session> lookMediaSession(const std::string& suffix);
    shared_ptr<media_session> lookMediaSession(MediaSessionId sessionId);
private:
    std::mutex m_mtxAccountMap;
    std::map<std::string,std::string> m_Account_Map;//存储账号密码映射信息
    std::mutex m_session_mutex;
    std::unordered_map<MediaSessionId,shared_ptr<media_session>>m_session_map;
    std::unordered_map<string,MediaSessionId>m_suffix_map;
    RTSP_NEW_CONNECTION_CALLBACK m_new_connection_callback;
    RTSP_NOTICE_CLIENT_NUMS_CALLBACK m_notice_client_nums_callback;
    RTSP_MEDIA_SESSION_CALLBACK m_new_media_session_callback;
    RTSP_MEDIA_SESSION_CALLBACK m_delete_media_session_callback;
};
}
#endif // RTSP_SERVER_H
