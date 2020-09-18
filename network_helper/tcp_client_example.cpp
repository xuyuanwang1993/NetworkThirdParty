#include "tcp_client_example.h"
using namespace micagent;
tcp_client_example::tcp_client_example(shared_ptr<tcp_connection_helper>helper,const string &ip,uint16_t port):tcp_client (helper,ip,port),m_timer_id(INVALID_TIMER_ID),m_seq(0)\
,m_init_status(false)
{
    m_send_cache.reset(new buffer_handle(1000,nullptr,true,1000));
}
bool tcp_client_example::handle_read()
{
    char buf[64]={0};
    auto len=::recv(m_tcp_channel->fd(),buf,64,0);
    if(len==0)return false;
    if(len<0){
        if(errno==EAGAIN||errno==EINTR)return true;
        else {
            return false;
        }
    }
    printf("recv len(%d) %s \r\n",len,string(buf,len).c_str());
    return true;
}
void tcp_client_example::tear_down()
{
    clear_connection_info();
}
