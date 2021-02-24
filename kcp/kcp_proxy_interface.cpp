#include "kcp_proxy_interface.h"
using namespace micagent;
kcp_proxy_interface::kcp_proxy_interface(shared_ptr<EventLoop>loop,uint16_t port):m_w_event_loop(loop),m_data_callback(nullptr)\
  ,m_init(false),m_running(false)
{//init udp socket
    SOCKET fd=NETWORK.build_socket(UDP);
    if(fd==INVALID_SOCKET)throw runtime_error("Fail to set up kcp_proxy_interface socket!");
    NETWORK.set_reuse_addr(fd);
    if(!NETWORK.bind(fd,port)){
        string error_info="bind udp port "+to_string(port)+" error!";
        throw  error_info;
    }
    NETWORK.make_noblocking(fd);
    m_udp_channel.reset(new Channel(fd));
    m_udp_channel->enableReading();
}
bool kcp_proxy_interface::send_raw_data(const ikcp_raw_udp_packet_s &packet)
{
    if(!m_init)return false;
    auto event_loop=m_w_event_loop.lock();
    if(!event_loop)return false;
    if(!m_send_cache->push(packet.copy()))return false;
    {
        lock_guard<mutex>locker(m_io_mutex);
        m_udp_channel->enableWriting();
        event_loop->updateChannel(m_udp_channel);
    }
    return true;
}
void kcp_proxy_interface::init(const kcp_proxy_data_callback &data_callback)
{
    assert(data_callback!=nullptr);
    if(m_init)return;
    auto event_loop=m_w_event_loop.lock();
    if(!event_loop)return;
    m_init.exchange(true);
    m_data_callback=data_callback;
    weak_ptr<kcp_proxy_interface>interface=shared_from_this();
    //set io callbacks
    m_udp_channel->setReadCallback([interface](Channel*){
        auto strong=interface.lock();
        if(strong)strong->handle_read();
        return true;
    });
    m_udp_channel->setWriteCallback([interface](Channel*){
        auto strong=interface.lock();
        if(strong)strong->handle_write();
        return true;
    });
    event_loop->updateChannel(m_udp_channel);
    //init cache
    m_recv_cache.reset(new ikcp_proxy_cache<ikcp_raw_udp_packet_s>([this](const ikcp_raw_udp_packet_s&data){
        m_data_callback(data);
        return true;
    }));
    m_send_cache.reset(new ikcp_proxy_cache<ikcp_raw_udp_packet_s>([this](const ikcp_raw_udp_packet_s &data){
        auto fd=m_udp_channel->fd();

        auto len=::sendto(fd,data.buf.get(),data.buf_len,0,reinterpret_cast<const sockaddr *>(&data.addr),sizeof (data.addr));
        //printf("send %s %d \r\n",data.buf.get(),len);
        return len!=SOCKET_ERROR;
    }));
    //start work thread
    m_work_thread.reset(new thread([this](){
        this->thread_task();
    }));
}
kcp_proxy_interface::~kcp_proxy_interface()
{//release all resources
    if(m_running){
        m_running.exchange(false);
    }
    if(m_work_thread&&m_work_thread->joinable())m_work_thread->join();
    auto loop=m_w_event_loop.lock();
    if(loop&&m_udp_channel)loop->removeChannel(m_udp_channel);
}
void  kcp_proxy_interface::handle_read()
{
    shared_ptr<uint8_t>buf(new uint8_t[2048],default_delete<uint8_t[]>());
    memset(buf.get(),0,2048);
    sockaddr_in addr;
    socklen_t sock_len=sizeof (addr);
    auto len=::recvfrom(m_udp_channel->fd(),buf.get(),2048,0,reinterpret_cast<sockaddr*>(&addr),&sock_len);
    if(len>0)
    {
        m_recv_cache->push(ikcp_raw_udp_packet_s(buf,static_cast<uint32_t>(len),addr));
    }
}
void kcp_proxy_interface::handle_write()
{
    if(m_send_cache->process_send_data())
    {//all data is sent to
        auto event_loop=m_w_event_loop.lock();
        if(!event_loop)return ;
        {
            lock_guard<mutex>locker(m_io_mutex);
            m_udp_channel->disableWriting();
            event_loop->updateChannel(m_udp_channel);
        }
    }
}
void kcp_proxy_interface::thread_task()
{
    m_running.exchange(true);
    while(m_running)
    {
        m_recv_cache->process_data();
    }
}
