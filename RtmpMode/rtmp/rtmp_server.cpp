#include "rtmp_server.h"
#include "rtmp_connection.h"
using namespace micagent;
rtmp_server::rtmp_server(shared_ptr<EventLoop> loop, uint16_t listen_port, uint32_t net_interface_index):m_check_timer(INVALID_TIMER_ID)\
,m_connection_timeout_time(DEFAULT_RTMP_CONNECTION_TIMEOUT_TIME),m_w_event_loop(loop)
{
    SOCKET fd=Network_Util::Instance().build_socket(TCP);
    if(fd==INVALID_SOCKET)throw runtime_error("build tcp socket failed!");
    Network_Util::Instance().set_reuse_addr(fd);
    Network_Util::Instance().set_reuse_port(fd);
    Network_Util::Instance().make_noblocking(fd);
    auto net=Network_Util::Instance().get_default_net_interface_info();
    Network_Util::Instance().bind(fd,listen_port,net_interface_index);
    //check
    auto port=Network_Util::Instance().get_local_port(fd);
    if(port!=listen_port){
        MICAGENT_MARK("port %hu bind error!bind on port %hu!",listen_port,port);
    }
    else {
        MICAGENT_MARK("create listen server on %s:%hu!",net.ip.c_str(),port);
    }
    m_listen_channel.reset(new Channel(fd));
    Init();
}
rtmp_server::~rtmp_server()
{
    auto event_loop=m_w_event_loop.lock();
    if(!event_loop)return;
    if(m_listen_channel)event_loop->removeChannel(m_listen_channel);
    if(m_check_timer!=INVALID_TIMER_ID)event_loop->removeTimer(m_check_timer);
}
bool rtmp_server::Update_Frame(const string &stream_name,RTMP_MEDIA_TYPE type,const void *buf,uint32_t buf_len,uint32_t timestamp)
{

}
bool rtmp_server::Add_Raw_Connection(SOCKET fd)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_rtmp_connections.find(fd);
    if(iter!=end(m_rtmp_connections)){
        m_rtmp_connections.erase(iter);
        return false;
    }
    else {
        m_rtmp_connections.emplace(make_pair(fd,make_shared<rtmp_connection>(fd,m_w_event_loop)));
        return true;
    }
}
void rtmp_server::Remove_Connection(SOCKET fd)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_rtmp_connections.find(fd);
    if(iter!=end(m_rtmp_connections)){
        m_rtmp_connections.erase(iter);
    }
}
void rtmp_server::Set_Authorize_Callback(const string &app_name,const RTMP_AUTHORIZE_CALLBACK&callback )
{
    lock_guard<mutex>locker(m_mutex);
    m_authorize_callbacks[app_name]=callback;
}
bool rtmp_server::Connection_Authorization_Check(const string &app_name,const string&authorization_string)
{
    unique_lock<mutex>locker(m_mutex);
    auto iter=m_authorize_callbacks.find(app_name);
    //can't find a authorize callbacks
    if(iter==m_authorize_callbacks.end())return false;
    else {
        //callback is nullptr and it always return true
        if(!iter->second)return true;
        auto callback=iter->second;
        locker.unlock();
        return callback(app_name,authorization_string);
    }
}
void rtmp_server::Init()
{
    auto event_loop=m_w_event_loop.lock();
    if(!event_loop)return;
    Network_Util::Instance().listen(m_listen_channel->fd(),20);
    weak_ptr<rtmp_server>weak_this=shared_from_this();
    m_listen_channel->setReadCallback([weak_this](Channel *chn){
        auto interface=weak_this.lock();
        if(!interface)return false;
        auto fd=Network_Util::Instance().accept(chn->fd());
        if(fd!=INVALID_SOCKET){
            if(!interface->Add_Raw_Connection(fd)){
                NETWORK.close_socket(fd);
            }
        }
        return true;
    });
    m_listen_channel->enableReading();
    event_loop->updateChannel(m_listen_channel);
    m_check_timer=event_loop->addTimer([weak_this](){
        auto interface=weak_this.lock();
        if(!interface)return false;
        interface->check_all_resources();
        return true;
    },RTMP_SERVER_RESOURCE_CHECK_INTERVAL);
}
void rtmp_server::check_all_resources()
{
    check_connections();
    check_sessions();
}
void rtmp_server::check_connections()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_connection_timeout_time==0)return;
    auto time_now=Timer::getTimeNow();
    for(auto iter=m_rtmp_connections.begin();iter!=m_rtmp_connections.end();)
    {
        auto time=iter->second->get_alive_time();
        if(time<time_now){
            if(time_now-time>m_connection_timeout_time){
                m_rtmp_connections.erase(iter++);
                continue;
            }
        }
        iter++;
    }
}
void rtmp_server::check_sessions()
{

}
