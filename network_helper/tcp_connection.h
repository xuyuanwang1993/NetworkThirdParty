#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H
#include "io_channel.h"
#include "buffer_handle.h"
#include "event_loop.h"
#include "c_log.h"
#include <atomic>
namespace micagent {
using namespace std;
class tcp_connection:public enable_shared_from_this<tcp_connection>
{
    /**
     *disconnectCallBack 连接断开回调函数
     */
    using disconnectCallBack=function<void(shared_ptr<tcp_connection>)>;
public:
    tcp_connection(SOCKET _fd);
    virtual ~tcp_connection();
    /**
     * @brief register_handle 向loop中注册this指针
     * @param loop
     */
    void register_handle(EventLoop *loop);
    void unregister_handle(EventLoop *loop);
    void set_disconnect_callback(const disconnectCallBack&cb ){
        m_disconnect_CB=cb;
    }
    SOCKET fd()const{return m_channel->fd();}
    friend class tcp_server;
    void set_fd_reuse(bool status=false)
    {
        lock_guard<mutex>locker(m_mutex);
        if(m_channel)m_channel->set_cycle(status);
    }
protected:
    virtual bool handle_read(){
        shared_ptr<char>buf(new char[1024],std::default_delete<char[]>());
        auto len=recv(m_channel->fd(),buf.get(),1024,0);
        if(len>0){
            send(m_channel->fd(),buf.get(),len,0);
        }
        return len!=0;
    }
    virtual bool handle_write(){return true;}
    virtual bool handle_close(){if(m_disconnect_CB)m_disconnect_CB(shared_from_this());return true;}
    virtual bool handle_error(){return true;}
    ChannelPtr m_channel;
    atomic_bool m_registered;
    disconnectCallBack m_disconnect_CB;
    mutex m_mutex;
};
}

#endif // TCP_CONNECTION_H
