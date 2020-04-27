#include "kcp_manage.h"
#include <mutex>
#include <string>
#include <stdint.h>
#include <utility>
#include <chrono>
#include <future>
using namespace micagent;
int kcp_interface::MAX_TIMEOUT_TIME=5*1000;
int kcp_interface::MAX_FRAMESIZE=IKCP_WND_RCV*IKCP_MTU_DEF;
static  int kcp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    (void )(kcp);
    kcp_interface *interface=static_cast<kcp_interface *>(user);
    return interface->raw_send(buf,len);
}

kcp_interface::kcp_interface(int fd,unsigned int conv_id,EventLoop *event_loop,RecvDataCallback recvCB,void *data,ClearCallback clearCB,uint32_t max_frame_size)
    :m_udp_channel(new Channel(fd)),m_conv_id(conv_id),m_have_cleared(false),m_loop(event_loop),m_clear_CB(clearCB),m_recvCB(recvCB),m_data(data),m_frame_size(max_frame_size)
{
    if(!m_loop)throw runtime_error("event_loop is invalid!");
    m_kcp.reset(new kcp_save(ikcp_create(conv_id,this)));
    if(m_frame_size>MAX_FRAMESIZE){
        MICAGENT_LOG(LOG_ERROR,"kcp's max frame cache is %d!but now the input frame's max size is %d !\r\n",MAX_FRAMESIZE,m_frame_size);
        m_frame_size=MAX_FRAMESIZE;
    }
    SetTransferMode(FAST_MODE);
    m_last_alive_time=Timer::getTimeNow();
    m_timeout_times=0;
    m_kcp->kcp_ptr()->output=kcp_output;
}
void kcp_interface::SetTransferMode(KCP_TRANSFER_MODE mode,int nodelay, int interval, int resend, int nc)
{
    lock_guard<mutex>locker(m_mutex);
    switch(mode)
    {
    case DEFULT_MODE:
        ikcp_nodelay(m_kcp->kcp_ptr(), 0, 10, 0, 0);
        break;
    case NORMAL_MODE:
        ikcp_nodelay(m_kcp->kcp_ptr(), 0, 10, 0, 1);
        break;
    case FAST_MODE:
        ikcp_nodelay(m_kcp->kcp_ptr(), 1, 10, 2, 1);
        break;
    case CUSTOM_MODE:
        ikcp_nodelay(m_kcp->kcp_ptr(), nodelay, interval, resend, nc);
        break;
    default:
        break;
    }
}
void kcp_interface::start_work()
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_udp_channel)return;
    m_kcp->kcp_ptr()->user=this;
    m_udp_channel->enableReading();
    //读到的数据输入kcp进行处理
    auto interface=shared_from_this();
    m_udp_channel->setReadCallback([interface](Channel *chn){
        if(interface->get_clear_status())return true;
        shared_ptr<char[]> buf(new char[IKCP_MTU_DEF]);
        auto ret=recv(chn->fd(),buf.get(),IKCP_MTU_DEF,0);
        if(ret==0)return false;
        else if (ret>0) {
            unique_lock<mutex> locker(interface->m_mutex);
            interface->m_last_alive_time=Timer::getTimeNow();
            if(ikcp_input(interface->m_kcp->kcp_ptr(),buf.get(),ret)==0)
            {
                ikcp_flush(interface->m_kcp->kcp_ptr());
                std::shared_ptr<char[]> frame_data(new char[interface->m_frame_size]);
                int recv_len=ikcp_recv(interface->m_kcp->kcp_ptr(),frame_data.get(),interface->m_frame_size);
                if(recv_len>0&&interface->m_recvCB)
                {
                    if(locker.owns_lock())locker.unlock();
                    interface->m_recvCB(frame_data.get(),recv_len,interface->m_kcp->kcp_ptr(),interface->m_data);
                }
            }
        }
        return true;
    });
    m_udp_channel->setCloseCallback([interface](Channel *chn){
        (void)chn;
        interface->clear();
        return true;
    });
    m_loop->updateChannel(m_udp_channel);
}
int kcp_interface::send_userdata(const char *buf,int len)
{
    lock_guard<mutex>locker(m_mutex);
    if(get_clear_status())return -1;
    if(len>MAX_FRAMESIZE)len=MAX_FRAMESIZE;
    return ikcp_send(this->m_kcp->kcp_ptr(),buf,len);
}
int kcp_interface::raw_send(const char *buf,int len)
{
    if(get_clear_status())return -1;
    return ::send(this->m_udp_channel->fd(),buf,len,0);
}
void kcp_interface::clear()
{
    if(!get_clear_status())
    {
        {
            DEBUG_LOCK
            m_have_cleared=true;
        }
        {
            lock_guard<mutex>locker(m_mutex);
            if(m_udp_channel)m_loop->removeChannel(m_udp_channel->fd());
            if(!m_clear_CB)return;
        }
        m_clear_CB();
    }
}

kcp_interface::~kcp_interface()
{
    MICAGENT_LOG(LOG_INFO,"clear %d",ikcp_getconv(m_kcp->kcp_ptr()));
    if(m_udp_channel)m_loop->removeChannel(m_udp_channel->fd());
}
std::shared_ptr<kcp_interface> kcp_manager::AddConnection(SOCKET fd,uint32_t conv_id,RecvDataCallback recvCB,void *m_data,uint32_t max_frame_size)
{
    if(!get_init_status()||fd==INVALID_SOCKET)return nullptr;
    std::shared_ptr<kcp_interface> interface(new kcp_interface(fd,conv_id,m_event_loop,recvCB,m_data,[fd,this](){
        this->CloseConnection(fd);
    },max_frame_size));
    std::lock_guard<std::mutex> locker(m_mutex);
    m_conv_map.insert(std::make_pair(conv_id,fd));
    m_kcp_map.insert(std::make_pair(fd,interface));
    return interface;
}
std::shared_ptr<kcp_interface> kcp_manager::AddConnection(uint16_t send_port,std::string ip,uint16_t port,uint32_t conv_id,RecvDataCallback recvCB,void *m_data,uint32_t max_frame_size)
{
    if(!get_init_status())return nullptr;
    auto fd=Network_Util::Instance().build_socket(UDP);
    if(fd==INVALID_SOCKET)return  nullptr;
    Network_Util::Instance().set_reuse_addr(fd);
    Network_Util::Instance().bind(fd,send_port);
    Network_Util::Instance().set_send_buf_size(fd,32*1024);
    Network_Util::Instance().set_recv_buf_size(fd,32*1024);
    struct sockaddr_in addr;
    bzero(&addr,sizeof (addr));
    socklen_t addrlen = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    ::connect(fd, (struct sockaddr*)&addr, addrlen);//绑定对端信息
    std::shared_ptr<kcp_interface> interface(new kcp_interface(fd,conv_id,m_event_loop,recvCB,m_data,[fd,this](){
        this->CloseConnection(fd);
    },max_frame_size));
    std::lock_guard<std::mutex> locker(m_mutex);
    m_conv_map.insert(std::make_pair(conv_id,fd));
    m_kcp_map.insert(std::make_pair(fd,interface));
    return interface;
}

void kcp_manager::StartUpdateLoop()
{
    m_event_loop->addTimer(std::bind(&kcp_manager::UpdateLoop,this),10);
}
void kcp_manager::StopUpdateLoop()
{
    DEBUG_LOCK
    m_init.exchange(false);
}
bool kcp_manager::UpdateLoop()
{
    multimap<uint32_t,SOCKET>tmp_map;
    {
        std::lock_guard<std::mutex> locker(m_mutex);
        tmp_map=m_conv_map;
    }
    for(auto i : tmp_map)
    {
        std::unique_lock<std::mutex> locker(m_mutex);
        auto iter=m_kcp_map.find(i.second);
        if(iter!=std::end(m_kcp_map)){
            if(locker.owns_lock())locker.unlock();
            iter->second->update(Timer::getTimeNow());
        }
    }
    return get_init_status();
}
