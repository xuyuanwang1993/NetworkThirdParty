#ifndef RTSP_CONNECTION_H
#define RTSP_CONNECTION_H
#include "tcp_connection.h"
#include "rtsp_message.h"
#include "media.h"
#include "timer_queue.h"
#include "rtp_connection.h"
namespace micagent {
using namespace std;
class rtsp_server;
class rtp_connection;
class rtsp_connection:public tcp_connection{
    friend class rtsp_server;
    friend class rtp_connection;
public:
    rtsp_connection(SOCKET fd,rtsp_server *server):tcp_connection(fd),m_rtsp_server(server),m_read_buf(new rtsp_message),\
    m_send_buf(new rtsp_message(true)),m_url(""),m_url_suffix(""),\
    m_is_authorized(false),m_last_alive_time(Timer::getTimeNow())
    {
        if(!server||!m_read_buf||!m_send_buf)throw runtime_error("init rtsp_connection failed!");
        m_rtpConnPtr.reset(new rtp_connection(this));
    }
    static int64_t diff_alive_time(rtsp_connection* conn,int64_t time_base){
        if(!conn)return 0;
        else {
            return time_base-conn->m_last_alive_time;
        }
    }
    virtual ~ rtsp_connection(){
    }
protected:

    bool handle_read();
     bool handle_write();
     bool handle_error(){return handle_close();}
     rtsp_server *m_rtsp_server;
     shared_ptr<rtsp_message> m_read_buf;
     shared_ptr<rtsp_message> m_send_buf;
     string m_url;
     string m_url_suffix;
private:
     bool send_message(const string &buf);
     bool send_message(const char *buf,uint32_t buf_len);
     bool handle_rtsp_request(const string &buf);

     bool handleCmdOption(map<string ,string>&handle_map);
     bool handleCmdDescribe(map<string ,string>&handle_map);
     bool handleCmdSetup(map<string ,string>&handle_map);
     bool handleCmdPlay(map<string ,string>&handle_map);
     bool handleCmdTeardown(map<string ,string>&handle_map);
     bool handleCmdGetParamter(map<string ,string>&handle_map);

     bool check_authorization_info(map<string ,string>&handle_map,const string &cmd_name="DESCRIBE");
     static void discard_rtcp_packet(SOCKET rtcp_fd);
     bool m_is_authorized;
     atomic<int64_t> m_last_alive_time;

     std::shared_ptr<rtp_connection>m_rtpConnPtr;
    std::shared_ptr<Channel>m_rtcpChannels[MAX_MEDIA_CHANNEL];
};
}
#endif // RTSP_CONNECTION_H
