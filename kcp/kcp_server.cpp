#include "kcp_server.h"
#include <random>
#include"c_log.h"
namespace micagent {
kcp_server::kcp_server(uint16_t port, shared_ptr<EventLoop> loop):m_event_loop(loop),m_session_id(0),m_loop_running(false),m_buf_handle_running(false)
{
    SOCKET fd=Network_Util::Instance().build_socket(UDP);
    if(fd==INVALID_SOCKET)throw runtime_error("fail to create socket!");
    Network_Util::Instance().set_reuse_addr(fd);
    Network_Util::Instance().bind(fd,port);
    Network_Util::Instance().make_noblocking(fd);
    Network_Util::Instance().set_recv_buf_size(fd,32*1024);
    Network_Util::Instance().set_send_buf_size(fd,32*1024);
    m_udp_channel.reset(new Channel(fd));
}

kcp_server::~kcp_server()
{
    unique_lock<mutex> locker(m_exit_mutex);
    if(get_loop_run_status()){
        {
            DEBUG_LOCK
            m_loop_running=false;
        }
        m_status.wait(locker);
    }
    if(get_buf_handle_run_status()){
        {
            DEBUG_LOCK
                    m_buf_handle_running=false;
        }
        {
            unique_lock<mutex>locker(this->m_bufcache_mutex);
            m_buf_handle_wait.notify_one();
        }
        m_status.wait(locker);
    }
    auto event_loop=m_event_loop.lock();
    if(event_loop&&m_udp_channel)event_loop->removeChannel(m_udp_channel);
#ifdef DEBUG
    printf("release kcp_server!\r\n");
#endif
}
int kcp_server::kcp_send_packet(uint32_t conv_id,const void *buf,int len)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_map.find(conv_id);
    if(iter==end(m_session_map))return -1;
    else {
        return  ikcp_send(iter->second->kcp,static_cast<const char *>(buf),len);
    }
}
void kcp_server::kcp_send_multicast_packet(const void *buf,int len)
{
    lock_guard<mutex>locker(m_mutex);
    for(auto i:m_session_map){
        ikcp_send(i.second->kcp,static_cast<const char *>(buf),len);
    }
}
int kcp_server::send_raw_packet(SOCKET fd , const void *buf, int len, const sockaddr_in &addr)
{
    return ::sendto(fd,buf,len,0,(sockaddr *)&addr,sizeof (addr));
}
uint32_t kcp_server::get_session_id()
{
    lock_guard<mutex>locker(m_mutex);
    auto try_times=MAX_TRY_TIMES;
    while(try_times-->0){
        ++m_session_id;
        if(m_session_id==0)++m_session_id;
        if(m_session_map.find(m_session_id)==end(m_session_map))return m_session_id;
    }
    random_device rd;
    m_session_id=rd();
    return 0;
}
void kcp_server::remove_session(uint32_t conv_id)
{
    unique_lock<mutex>locker(m_mutex);
    auto iter=m_session_map.find(conv_id);
    if(iter!=end(m_session_map)){
        auto iter2=m_addr_set.find(addr_to_uint64(iter->second->recv_addr));
        if(iter2!=m_addr_set.end())m_addr_set.erase(iter2);
        if(locker.owns_lock())locker.unlock();
        if(iter->second->close_callback)iter->second->close_callback(conv_id);
        if(!locker.owns_lock())locker.lock();
        m_session_map.erase(iter);
    }
}
bool kcp_server::add_session(uint32_t conv_id, const sockaddr_in &addr, void *user, KCPSERVER_RECVCALLBACK recv_cb, KCPSERVER_CLOSECALLBACK close_cb)
{
    lock_guard<mutex>locker(m_mutex);
    if(m_session_map.find(conv_id)!=end(m_session_map)){
        MICAGENT_MARK("conv_id %d  is in use!",conv_id);
        return false;
    }
    if(m_addr_set.find(addr_to_uint64(addr))!=end(m_addr_set)){
        MICAGENT_MARK("addr %s  %d  is in use!",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
        return false;
    }
    m_addr_set.insert(addr_to_uint64(addr));
    m_session_map.emplace(make_pair(conv_id,make_shared< kcp_session>(conv_id,addr,recv_cb,close_cb,m_udp_channel->fd(),user)));
    return true;
}
bool kcp_server::start_update_loop(uint32_t interval_ms)
{
    lock_guard<mutex>locker(m_mutex);
    m_udp_channel->enableReading();
    auto interface=shared_from_this();
    m_udp_channel->setReadCallback([interface](Channel *chn){
        (void )chn;
        interface->hand_read();
        return true;
    });
    auto event_loop=m_event_loop.lock();
    if(!event_loop)return false;
    event_loop->updateChannel(m_udp_channel);
    if(interval_ms<10)interval_ms=10;
    else if(interval_ms>5000)interval_ms=5000;
    if(!get_loop_run_status()){
    DEBUG_LOCK
        m_loop_running.exchange(true);
        m_loop_running.exchange(event_loop->add_thread_task([this,interval_ms](){
            while(get_loop_run_status()){
                this->update_loop();
                Timer::sleep(interval_ms);
            }
            unique_lock<mutex> locker(m_exit_mutex);
            m_status.notify_one();
            return false;
        }));
    }
    if(!get_buf_handle_run_status()){
    DEBUG_LOCK
        m_buf_handle_running.exchange(true);
        m_buf_handle_running.exchange(event_loop->add_thread_task([this](){
            unique_lock<mutex>locker(this->m_bufcache_mutex);
            while(get_buf_handle_run_status()){
                if(!m_buf_cache.empty()){
                    this->handle_cache();
                }
                else {
                    m_buf_handle_wait.wait_for(locker,chrono::milliseconds(10));
                }
            }
            if(locker.owns_lock())locker.unlock();
            unique_lock<mutex> locker_exit(m_exit_mutex);
            m_status.notify_one();
            return false;
        }));
    }
    DEBUG_LOCK
    return m_loop_running&&m_buf_handle_running;
}
void kcp_server::hand_read()
{
    shared_ptr<char>buf(new char[IKCP_MTU_DEF+1],std::default_delete<char[]>());
    MAKE_ADDR(addr,"0.0.0.0",0);
    socklen_t sock_len=sizeof (addr);
    auto len=recvfrom(m_udp_channel->fd(),buf.get(),IKCP_MTU_DEF,0,(sockaddr *)&addr,&sock_len);
    if(len>=IKCP_OVERHEAD){
        unique_lock<mutex>locker(m_bufcache_mutex);
        m_buf_cache.push(kcp_buf_cache(buf,len));
        m_buf_handle_wait.notify_one();
    }
}
void kcp_server::update_loop()
{
    unique_lock<mutex>locker(m_mutex);
    auto time_now=Timer::getTimeNow();
    for(auto iter=m_session_map.begin();iter!=end(m_session_map);)
    {
        if(time_now-iter->second->last_alive_time>MAX_CACHE_TIME){
            iter->second->time_out_cnt++;
        }
        if(iter->second->time_out_cnt>MAX_TIME_OUT_CNT){
            MICAGENT_MARK("conv_id %d ip %s port:%hu is time out!",iter->second->conv_id,inet_ntoa(iter->second->recv_addr.sin_addr),ntohs(\
                              iter->second->recv_addr.sin_port));
            if(locker.owns_lock())locker.unlock();
            if(iter->second->close_callback)iter->second->close_callback(iter->first);
            if(!locker.owns_lock())locker.lock();
            auto iter2=m_addr_set.find(addr_to_uint64(iter->second->recv_addr));
            if(iter2!=m_addr_set.end())m_addr_set.erase(iter2);
            m_session_map.erase(iter++);
            continue;
        }
        ikcp_update(iter->second->kcp,static_cast<IUINT32>(time_now));
        iter++;
    }
}
uint64_t kcp_server::addr_to_uint64(const sockaddr_in &addr)
{
    uint64_t ret=0;
    ret|=(static_cast<uint64_t>(addr.sin_addr.s_addr)<<(sizeof (uint16_t)*8));
    ret|=addr.sin_port;
    return ret;
}
void kcp_server::handle_cache()
{
    unique_lock<mutex>locker(m_mutex);
    while(!m_buf_cache.empty()){
        auto cache=m_buf_cache.front();
        m_buf_cache.pop();
        uint32_t conv_id=*reinterpret_cast<uint32_t *>(cache.buf.get());
        auto iter=m_session_map.find(conv_id);
        if(iter==end(m_session_map)){
            MICAGENT_MARK("conv_id %d is not existed!",conv_id);
        }
        else {
            iter->second->last_alive_time=Timer::getTimeNow();
            iter->second->time_out_cnt=0;
            if(ikcp_input(iter->second->kcp,cache.buf.get(),cache.buf_len)==0)
            {
                ikcp_flush(iter->second->kcp);
                auto frame_len=ikcp_peeksize(iter->second->kcp);
                if(frame_len<=0)return;
                std::shared_ptr<char> frame_data(new char[frame_len],std::default_delete<char[]>());
                int recv_len=ikcp_recv(iter->second->kcp,frame_data.get(),frame_len);
                if(recv_len>0&&iter->second->recv_callback)
                {
                    if(locker.owns_lock())locker.unlock();
                    iter->second->recv_callback(frame_data.get(),frame_len,&iter->second,iter->second->user);
                    if(!locker.owns_lock())locker.lock();
                }
            }
        }
    }
}
}
