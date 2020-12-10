#ifndef RTMP_CONNECTION_H
#define RTMP_CONNECTION_H
#include "rtmp_common.h"
namespace micagent {
using namespace micagent;
class rtmp_server;
class rtmp_message;
class rtmp_connection final:public enable_shared_from_this<rtmp_connection> {
    friend class rtmp_server;
    friend class rtmp_message;
public:
    rtmp_connection(SOCKET fd,shared_ptr<rtmp_server>server);
    ~rtmp_connection();
protected:
    virtual bool handle_read();
    virtual bool handle_write();
    virtual bool handle_close();

protected:

    bool get_alive_time()const{
        lock_guard<mutex>locker(m_mutex);
        return  m_last_alive_time;
    }
    SOCKET fd()const{ return  m_tcp_channel->fd();}
private:
    void disable_write();
protected:
    shared_ptr<Channel> m_tcp_channel;
    mutable mutex m_mutex;
    weak_ptr<rtmp_server>m_rtmp_server;
    int64_t m_last_alive_time;
    shared_ptr<rtmp_message>m_rtmp_message_handle;
};
}
#endif // RTMP_CONNECTION_H
