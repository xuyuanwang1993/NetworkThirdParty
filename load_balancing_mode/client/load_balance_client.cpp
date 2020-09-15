#include "load_balance_client.h"
#include"MD5.h"
using namespace micagent;
load_balance_client::load_balance_client():m_timer_id(INVALID_TIMER_ID),m_is_running(false),m_server_ip("0.0.0.0"),m_server_port(0),m_account_name("test"),m_domain_name("www.test.com")\
,m_weight(0),m_max_load_size(1),m_now_load(0),m_send_fd(INVALID_SOCKET)
{

}
void load_balance_client::config_server_info(weak_ptr<EventLoop> loop, string server_ip, uint16_t server_port)
{
    stop_work();
    lock_guard<mutex>locker(m_mutex);
    m_loop=loop;
    m_server_ip=server_ip;
    m_server_port=server_port;
    m_send_fd=Network_Util::Instance().build_socket(UDP);
    memset(&m_server_addr,0,sizeof (m_server_addr));
    if(m_send_fd==INVALID_SOCKET)throw runtime_error("can't build udp socket!");
    m_server_addr.sin_family=AF_INET;
    m_server_addr.sin_addr.s_addr=inet_addr(m_server_ip.c_str());
    m_server_addr.sin_port=htons(m_server_port);
    if(m_send_fd!=INVALID_SOCKET){
        Network_Util::Instance().connect(m_send_fd,m_server_addr);
    }
}
void load_balance_client::config_client_info(string account,string domain_name,uint32_t max_load_size,double weight,int64_t upload_interval)
{
    lock_guard<mutex>locker(m_mutex);
    m_account_name=account;
    m_domain_name=domain_name;
    m_max_load_size=max_load_size;
    m_upload_interval=upload_interval;
    if(m_max_load_size==0)m_max_load_size=1;
    m_weight=weight;
}
void load_balance_client::increase_load(uint32_t load_size)
{
    lock_guard<mutex>locker(m_mutex);
    m_now_load+=load_size;
}
void load_balance_client::decrease_load(uint32_t load_size)
{
    lock_guard<mutex>locker(m_mutex);
    if(load_size>m_now_load)m_now_load=0;
    else {
        m_now_load-=load_size;
    }
}
pair<bool,neb::CJsonObject>  load_balance_client::find(string account,set<string>exclude_list,int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","find");
    object.Add(TO_STRING(account),account);
    neb::CJsonObject exclude_list_object;
    for(auto i:exclude_list)
    {
        exclude_list_object.Add(i);
    }
    if(exclude_list_object.IsEmpty())
    {
        object.AddEmptySubArray("exclude_list");
    }
    else {
        object.Add("exclude_list",exclude_list_object);
    }
    SOCKET sock=Network_Util::Instance().build_socket(UDP);
    if(sock==INVALID_SOCKET)return  {false,neb::CJsonObject()};
    {
        lock_guard<mutex>locker(m_mutex);
        Network_Util::Instance().connect(sock,m_server_addr);
    }
    rc4_send(sock,object.ToString());
    shared_ptr<char>recv_buf(new char[1500],std::default_delete<char[]>());
    auto read_len=read_packet(sock,recv_buf.get(),1500,time_out);
    Network_Util::Instance().close_socket(sock);
    if(read_len<=0)return {false,neb::CJsonObject()};
    else {
        uint32_t out_size=read_len-8;
        shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
        rc4_interface interface(Timer::getTimeNow);
        auto ret_len=interface.decrypt(recv_buf.get(),output.get(),read_len,out_size,interface.generate_key());
        return {true,neb::CJsonObject(string(output.get(),ret_len))};
    }
}
pair<bool,neb::CJsonObject>  load_balance_client::specific_find(string account, string domain_name, int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","specific_find");
    object.Add(TO_STRING(account),account);
    object.Add(TO_STRING(domain_name),domain_name);
    SOCKET sock=Network_Util::Instance().build_socket(UDP);
    if(sock==INVALID_SOCKET)return  {false,neb::CJsonObject()};
    {
        lock_guard<mutex>locker(m_mutex);
        Network_Util::Instance().connect(sock,m_server_addr);
    }
    rc4_send(sock,object.ToString());
    shared_ptr<char>recv_buf(new char[1500],std::default_delete<char[]>());
    auto read_len=read_packet(sock,recv_buf.get(),1500,time_out);
    Network_Util::Instance().close_socket(sock);
    if(read_len<=0)return {false,neb::CJsonObject()};
    else {
        uint32_t out_size=read_len-8;
        shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
        rc4_interface interface(Timer::getTimeNow);
        auto ret_len=interface.decrypt(recv_buf.get(),output.get(),read_len,out_size,interface.generate_key());
        return {true,neb::CJsonObject(string(output.get(),ret_len))};
    }
}
void load_balance_client::update()
{
    if(m_send_fd==INVALID_SOCKET)return;
    neb::CJsonObject object;
    object.Add("cmd","update");
    object.Add(TO_STRING(account),m_account_name);
    object.Add(TO_STRING(domain_name),m_domain_name);
    double priority=m_now_load>=m_max_load_size?100.0:static_cast<double>(m_now_load)/m_max_load_size;
    double weight=priority>1.0?1.0:m_weight;
    object.Add(TO_STRING(priority),priority);
    object.Add(TO_STRING(weight),weight);
    rc4_send(m_send_fd,object.ToString());
}
void load_balance_client::rc4_send(SOCKET fd,const string &buf)
{
    uint32_t out_size=buf.size()+8;
    shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
    memset(output.get(),0,out_size);
    rc4_interface interface(Timer::getTimeNow);
    auto ret=interface.encrypt(buf.c_str(),output.get(),buf.size(),out_size);
    send(fd,output.get(),ret,0);
}
void load_balance_client::start_work()
{
    lock_guard<mutex>locker(m_mutex);
    auto event_loop=m_loop.lock();
    if(!m_is_running&&event_loop){
        m_is_running=true;
        update();
        if(m_timer_id==INVALID_TIMER_ID)m_timer_id=event_loop->addTimer([this](){
            lock_guard<mutex>locker(m_mutex);
            update();
            return true;
        },m_upload_interval);
    }
}
void load_balance_client::stop_work()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_send_fd!=INVALID_SOCKET)
    {
        NETWORK.close_socket(m_send_fd);
        m_send_fd=INVALID_SOCKET;
    }
    auto event_loop=m_loop.lock();
    if(m_is_running&&event_loop){
        event_loop->removeTimer(m_timer_id);
        m_timer_id=INVALID_TIMER_ID;
        m_is_running=false;
    }
}
