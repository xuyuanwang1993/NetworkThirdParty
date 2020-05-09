#include "tcp_connection.h"
using namespace micagent;
tcp_connection::tcp_connection(SOCKET _fd):m_channel(new Channel(_fd)),m_registered(false),m_disconnect_CB(nullptr)
{
    Network_Util::Instance().make_noblocking(_fd);
    Network_Util::Instance().set_ignore_sigpipe(_fd);
    Network_Util::Instance().set_tcp_keepalive(_fd,true);
}
tcp_connection::~tcp_connection(){
    MICAGENT_MARK("tcp_connection %d exit!",m_channel->fd());
}
void tcp_connection::register_handle(EventLoop *loop)
{
    if(!loop)MICAGENT_MARK("loop is nullptr!");
    else {
        if(!m_registered){
            m_mutex.lock();
            auto interface=shared_from_this();
            m_channel->setReadCallback([interface](Channel *chn){
                (void)chn;
                return interface->handle_read();
            });
            m_channel->setWriteCallback([interface](Channel *chn){
            (void)chn;
                return interface->handle_write();
            });
            m_channel->setCloseCallback([interface](Channel *chn){
            (void)chn;
                return interface->handle_close();
            });
            m_channel->setErrorCallback([interface](Channel *chn){
            (void)chn;
                return interface->handle_error();
            });
            m_channel->enableReading();
            m_mutex.unlock();
            loop->updateChannel(m_channel);
            m_registered.exchange(true);
        }
    }
}
void tcp_connection::unregister_handle(EventLoop *loop)
{
    if(!loop)MICAGENT_MARK("loop is nullptr!");
    else {
        if(m_registered){
            loop->removeChannel(m_channel);
            m_registered.exchange(false);
            m_mutex.lock();
            m_channel->setReadCallback(nullptr);
            m_channel->setCloseCallback(nullptr);
            m_channel->setErrorCallback(nullptr);
            m_channel->setWriteCallback(nullptr);
            m_disconnect_CB=nullptr;
            m_mutex.unlock();
        }
    }
}
