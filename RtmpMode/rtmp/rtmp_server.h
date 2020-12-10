#ifndef RTMP_SERVER_H
#define RTMP_SERVER_H
#include "rtmp_common.h"
namespace micagent {
class rtmp_connection;
class rtmp_session;
class rtmp_server:public enable_shared_from_this<rtmp_server>{
    friend class rtmp_connection;
    friend class rtmp_session;
public:
    /**
     * @brief rtmp_server
     * @param listen_port default port is 1935,if the port is less than 1024,you may run this program as a root user
     * @param net_interface_index you can listen on the specific NetInterface,if the Index is UINT32_MAX ,the program max listen on the address 0.0.0.0
     */
    rtmp_server(shared_ptr<EventLoop>loop,uint16_t listen_port=1935,uint32_t net_interface_index=UINT32_MAX);
    ~rtmp_server();
    /**
     * @brief Update_Frame
     * @param stream_name name of the session which will receive the frame
     * @param type  frame's type
     * @param buf
     * @param buf_len
     * @param timestamp  millsecond
     * @return if a error occurred,it will return false
     */
    bool Update_Frame(const string &stream_name,RTMP_MEDIA_TYPE type,const void *buf,uint32_t buf_len,uint32_t timestamp=0);
    bool Add_Raw_Connection(SOCKET fd);
    void Remove_Connection(SOCKET fd);
    void Set_Authorize_Callback(const string &app_name,const RTMP_AUTHORIZE_CALLBACK&callback );
    bool Connection_Authorization_Check(const string &stream_name,const string & authorization_string);
private:
    //work as a server
    void Init();
    void check_all_resources();
    void check_connections();
    void check_sessions();
private:
    ChannelPtr m_listen_channel;
    TimerId m_check_timer;
    uint32_t m_connection_timeout_time;
    mutex m_mutex;
    //weak reference to the event loop
    weak_ptr<EventLoop>m_w_event_loop;
    //save all sessions
    map<string,shared_ptr<rtmp_session>>m_session_map;
    //save all apps' callbacks
    map<string,RTMP_AUTHORIZE_CALLBACK>m_authorize_callbacks;
    //save all rtmp(tcp) connections
    unordered_map<SOCKET,shared_ptr<rtmp_connection>>m_rtmp_connections;
};
}
#endif // RTMP_SERVER_H
