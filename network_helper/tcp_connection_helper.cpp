#include "tcp_connection_helper.h"
using namespace micagent;
tcp_connection_helper *tcp_connection_helper::CreateNew(EventLoop *loop)
{
    return new tcp_connection_helper(loop);
}
void tcp_connection_helper::OpenConnection(string ip,uint16_t port,CONNECTION_CALLBACK callback,uint32_t time_out_ms)
{
    do{
        if(!m_loop)break;
        SOCKET fd=NETWORK.build_socket(TCP);
        if(fd==INVALID_SOCKET)break;
        NETWORK.make_noblocking(fd);
        NETWORK.connect(fd,ip,port,0);
        {
            lock_guard<mutex>locker(m_mutex);
            m_sock_set.insert(fd);
        }
        if(time_out_ms>0)
        {
            m_loop->addTimer([this,fd,callback](){
            //由timer移除的fd都归类为超时
                unique_lock<mutex>locker(this->m_mutex);
                auto iter=m_sock_set.find(fd);
                if(iter!=m_sock_set.end())
                {
                    m_sock_set.erase(iter);
                    m_loop->removeChannel(fd);
                    if(locker.owns_lock())locker.unlock();
                    callback(CONNECTION_TIME_OUT,INVALID_SOCKET);
                }
                return false;
            },time_out_ms);
        }
        ChannelPtr channel(new Channel(fd));
        channel->enableWriting();
        channel->setWriteCallback([this,callback](Channel  *chn){
            unique_lock<mutex>locker(m_mutex);
            auto iter=m_sock_set.find(chn->fd());
            if(iter!=m_sock_set.end())
            {
                m_sock_set.erase(iter);
                CONNECTION_STATUS status=CONNECTION_FAILED;
                if(NETWORK.get_socket_error(chn->fd())==0&&NETWORK.get_peer_port(chn->fd())!=0)
                {
                    status=CONNECTION_SUCCESS;
                    chn->set_cycle(true);
                }
                if(locker.owns_lock())locker.unlock();
                m_loop->removeChannel(chn->fd());
                callback(status,chn->fd());
            }
            return true;
        });
        channel->setErrorCallback([this,callback](Channel  *chn){
            unique_lock<mutex>locker(m_mutex);
            auto iter=m_sock_set.find(chn->fd());
            if(iter!=m_sock_set.end())
                {
                    m_sock_set.erase(iter);
                    if(locker.owns_lock())locker.unlock();
                    m_loop->removeChannel(chn->fd());
                    callback(CONNECTION_FAILED,INVALID_SOCKET);
                }
            return false;
        });
        channel->setCloseCallback([this,callback](Channel  *chn){
            unique_lock<mutex>locker(m_mutex);
            auto iter=m_sock_set.find(chn->fd());
            if(iter!=m_sock_set.end())
                {
                    m_sock_set.erase(iter);
                    if(locker.owns_lock())locker.unlock();
                    m_loop->removeChannel(chn->fd());
                    callback(CONNECTION_FAILED,INVALID_SOCKET);
                }
            return false;
        });
        m_loop->updateChannel(channel);
        return;
    }while(0);
    callback(CONNECTION_SYS_ERROR,INVALID_SOCKET);
}
