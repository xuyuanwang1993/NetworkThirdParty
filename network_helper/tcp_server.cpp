#include "tcp_server.h"
using namespace micagent;
tcp_server::~tcp_server(){
    MICAGENT_MARK("tcp_server exit!");
    unregister_handle();
}
tcp_server::tcp_server(uint16_t listen_port,uint32_t netinterface_index):m_registered(false)
{
    SOCKET fd=Network_Util::Instance().build_socket(TCP);
    if(fd==INVALID_SOCKET)throw runtime_error("build tcp socket failed!");
    Network_Util::Instance().set_reuse_addr(fd);
    Network_Util::Instance().set_reuse_port(fd);
    Network_Util::Instance().make_noblocking(fd);
    auto net=Network_Util::Instance().get_net_interface_info();
    if(!net.empty()){
        m_net_info=net[0];
    }
    Network_Util::Instance().bind(fd,listen_port,netinterface_index);
    m_server_ip=m_net_info.ip;
    //check
    m_server_port=Network_Util::Instance().get_local_port(fd);
    if(m_server_port!=listen_port){
        MICAGENT_MARK("port %hu bind error!bind on port %hu!",listen_port,m_server_port);
    }
    else {
        MICAGENT_MARK("create listen server on %s:%hu!",m_server_ip.c_str(),m_server_port);
    }
    m_listen_channel.reset(new Channel(fd));
}
void tcp_server::register_handle(EventLoop *loop)
{
    if(!m_registered){
        Network_Util::Instance().listen(m_listen_channel->fd(),20);
        m_loop=loop;
        init_server();
        weak_ptr<tcp_server>weak_this=shared_from_this();
        m_listen_channel->setReadCallback([weak_this](Channel *chn){
            auto interface=weak_this.lock();
            if(!interface)return false;
            auto fd=Network_Util::Instance().accept(chn->fd());
            if(fd!=INVALID_SOCKET){
                    interface->add_connection(interface->new_connection(fd));
            }
            return true;
        });
        m_listen_channel->enableReading();
        m_loop->updateChannel(m_listen_channel);
        m_registered.exchange(true);
    }
}
void tcp_server::unregister_handle()
{
    if(m_registered){
        m_registered.exchange(false);
        m_loop->removeChannel(m_listen_channel);
    }
}
void tcp_server::add_connection(shared_ptr<tcp_connection> conn)
{
    if(!m_registered||!conn)return;
    conn->set_disconnect_callback([this](shared_ptr<tcp_connection> conn){
        MICAGENT_LOG(LOG_INFO,"remove %u   %s",conn->fd(),Logger::get_local_time().c_str());
        this->remove_connection(conn->fd());
    });
    lock_guard<mutex>locker(m_mutex);
    auto rm_iter=m_connections.find(conn->fd());
    if(rm_iter!=m_connections.end()){
        MICAGENT_LOG(LOG_FATALERROR,"fd %d release error!",conn->fd());
        if(rm_iter->second){
            rm_iter->second->set_fd_reuse(true);
            rm_iter->second->unregister_handle(m_loop);
        }
        m_connections.erase(rm_iter);
    }
    auto iter=m_connections.emplace(conn->fd(),conn);
    MICAGENT_LOG(LOG_INFO,"add %u   %s   status:%d %lu",conn->fd(),Logger::get_local_time().c_str(),iter.second,m_connections.size());
    conn->register_handle(m_loop);
}
shared_ptr<tcp_connection>tcp_server::new_connection(SOCKET fd)
{
    return make_shared<tcp_connection>(fd);
}
void tcp_server::remove_connection(SOCKET fd)
{
    if(!m_registered)return;
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_connections.find(fd);
    if(iter!=end(m_connections)){
        iter->second->unregister_handle(m_loop);
        m_connections.erase(iter);
    }
}
