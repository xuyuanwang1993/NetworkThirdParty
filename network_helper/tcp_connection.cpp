#include "tcp_connection.h"
using namespace micagent;
tcp_connection::tcp_connection(SOCKET _fd):m_channel(new Channel(_fd)),m_registered(false),m_disconnect_CB(nullptr)
{
    Network_Util::Instance().make_noblocking(_fd);
    Network_Util::Instance().set_ignore_sigpipe(_fd);
    Network_Util::Instance().set_tcp_keepalive(_fd,true);
    MICAGENT_MARK("tcp_connection %d new! %s %hu %p\r\n",m_channel->fd(),NETWORK.get_peer_ip(_fd).c_str(),NETWORK.get_peer_port(_fd),this);
}
tcp_connection::~tcp_connection(){
    MICAGENT_MARK("tcp_connection %d exit %s %hu %p\r\n!",m_channel->fd(),NETWORK.get_peer_ip(fd()).c_str(),NETWORK.get_peer_port(fd()),this);
}
void tcp_connection::register_handle(EventLoop *loop)
{
    if(!loop)MICAGENT_MARK("loop is nullptr!");
    else {
        if(!m_registered){
            weak_ptr<tcp_connection>weak_this(shared_from_this());
            m_channel->setReadCallback([weak_this](Channel *chn){
                (void)chn;
                auto interface=weak_this.lock();
                if(!interface)return false;
                return interface->handle_read();
            });
            m_channel->setWriteCallback([weak_this](Channel *chn){
                (void)chn;
                auto interface=weak_this.lock();
                if(!interface)return false;
                return interface->handle_write();
            });
            m_channel->setCloseCallback([weak_this](Channel *chn){
                (void)chn;
                auto interface=weak_this.lock();
                if(!interface)return false;
                return interface->handle_close();
            });
            m_channel->setErrorCallback([weak_this](Channel *chn){
                (void)chn;
                auto interface=weak_this.lock();
                if(!interface)return false;
                return interface->handle_error();
            });
            m_channel->enableReading();
            m_registered.exchange(true);
            loop->updateChannel(m_channel);
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
        }
    }
}
