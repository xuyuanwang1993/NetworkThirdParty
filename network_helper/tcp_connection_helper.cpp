#include "tcp_connection_helper.h"
using namespace micagent;
tcp_connection_helper *tcp_connection_helper::CreateNew(weak_ptr<EventLoop> loop)
{
    return new tcp_connection_helper(loop);
}
void tcp_connection_helper::OpenConnection(string ip,uint16_t port,CONNECTION_CALLBACK callback,uint32_t time_out_ms)
{
    do{
        auto event_loop=m_loop.lock();
        if(!event_loop)break;
        SOCKET fd=NETWORK.build_socket(TCP);
        if(fd==INVALID_SOCKET)break;
        NETWORK.make_noblocking(fd);
        NETWORK.connect(fd,ip,port,0);
        ChannelPtr channel(new Channel(fd));
        TimerId timer_id=INVALID_TIMER_ID;
        timer_id=event_loop->addTimer([this,fd,callback,channel](){
            MICAGENT_LOG(LOG_WARNNING,"connect time out  %d!",fd);
            //由timer移除的fd都归类为超时
            auto event_loop=m_loop.lock();
            if(event_loop)event_loop->removeChannel(fd);
            if(callback)callback(CONNECTION_TIME_OUT,INVALID_SOCKET);
            return false;
        },time_out_ms);
        channel->enableWriting();
        channel->setWriteCallback([this,callback,timer_id](Channel  *chn){
            if(timer_id!=INVALID_TIMER_ID){
                auto event_loop=m_loop.lock();
            if(event_loop)event_loop->blockRemoveTimer(timer_id);
            }
            CONNECTION_STATUS status=CONNECTION_FAILED;
            if(NETWORK.get_socket_error(chn->fd())==0&&NETWORK.get_peer_port(chn->fd())!=0)
            {
                status=CONNECTION_SUCCESS;
                chn->set_cycle(true);
                NETWORK.set_tcp_keepalive(chn->fd(),true);
            }
            auto event_loop=m_loop.lock();
            if(event_loop)event_loop->removeChannel(chn->fd());
            if(callback)callback(status,chn->fd());
            return false;
        });
        channel->enableReading();
        channel->setReadCallback([this,callback,timer_id](Channel  *chn){
            if(timer_id!=INVALID_TIMER_ID){
                auto event_loop=m_loop.lock();
            if(event_loop)event_loop->blockRemoveTimer(timer_id);
            }
            CONNECTION_STATUS status=CONNECTION_FAILED;
            auto event_loop=m_loop.lock();
            if(event_loop)event_loop->removeChannel(chn->fd());
            if(callback)callback(status,chn->fd());
            return false;
        });
        event_loop->updateChannel(channel);
        return;
    }while(0);
    if(callback)callback(CONNECTION_SYS_ERROR,INVALID_SOCKET);
}
