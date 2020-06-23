#include "load_balance_client.h"
#include"MD5.h"
using namespace micagent;
load_balance_client::load_balance_client():\
m_loop(nullptr),m_timer_id(0),m_is_running(false),m_server_ip("0.0.0.0"),m_server_port(0),m_domain_name("www.test.com")\
,m_weight(0),m_max_load_size(1),m_now_load(0)
{
    m_send_fd=Network_Util::Instance().build_socket(UDP);
    memset(&m_server_addr,0,sizeof (m_server_addr));
    m_server_addr.sin_family=AF_INET;
    if(m_send_fd==INVALID_SOCKET)throw runtime_error("can't build udp socket!");
    m_server_addr.sin_addr.s_addr=inet_addr(m_server_ip.c_str());
    m_server_addr.sin_port=htons(m_server_port);
    Network_Util::Instance().connect(m_send_fd,m_server_addr);
}
void load_balance_client::config_server_info(EventLoop *loop,string server_ip,uint16_t server_port)
{
    stop_work();
    lock_guard<mutex>locker(m_mutex);
    m_loop=loop;
    m_server_ip=server_ip;
    m_server_port=server_port;
    m_server_addr.sin_addr.s_addr=inet_addr(m_server_ip.c_str());
    m_server_addr.sin_port=htons(m_server_port);
    if(m_send_fd!=INVALID_SOCKET)Network_Util::Instance().connect(m_send_fd,m_server_addr);
}
void load_balance_client::config_client_info(string domain_name,uint32_t max_load_size,double weight)
{
    lock_guard<mutex>locker(m_mutex);
    m_domain_name=domain_name;
    m_max_load_size=max_load_size;
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
pair<bool,neb::CJsonObject>  load_balance_client::find(int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","find");
    SOCKET sock=Network_Util::Instance().build_socket(UDP);
    if(sock==INVALID_SOCKET)return  {false,neb::CJsonObject()};
    {
        lock_guard<mutex>locker(m_mutex);
        Network_Util::Instance().connect(sock,m_server_addr);
    }
    rc4_send(sock,object.ToString());
    shared_ptr<char[]>recv_buf(new char[1500]);
    auto read_len=read_packet(sock,recv_buf.get(),1500,time_out);
    Network_Util::Instance().close_socket(sock);
    if(read_len<=0)return {false,neb::CJsonObject()};
    else {
        uint32_t out_size=read_len-8;
        shared_ptr<char[]>output(new char[out_size]);
        rc4_interface interface(Timer::getTimeNow);
        auto ret_len=interface.decrypt(recv_buf.get(),output.get(),read_len,out_size,interface.generate_key());
        return {true,neb::CJsonObject(string(output.get(),ret_len))};
    }
}
pair<bool,neb::CJsonObject>  load_balance_client::specific_find(string domain_name,int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","specific_find");
    object.Add(TO_STRING(domain_name),domain_name);
    SOCKET sock=Network_Util::Instance().build_socket(UDP);
    if(sock==INVALID_SOCKET)return  {false,neb::CJsonObject()};
    {
        lock_guard<mutex>locker(m_mutex);
        Network_Util::Instance().connect(sock,m_server_addr);
    }
    rc4_send(sock,object.ToString());
    shared_ptr<char[]>recv_buf(new char[1500]);
    auto read_len=read_packet(sock,recv_buf.get(),1500,time_out);
    Network_Util::Instance().close_socket(sock);
    if(read_len<=0)return {false,neb::CJsonObject()};
    else {
        uint32_t out_size=read_len-8;
        shared_ptr<char[]>output(new char[out_size]);
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
    shared_ptr<char[]>output(new char[out_size]);
    memset(output.get(),0,out_size);
    rc4_interface interface(Timer::getTimeNow);
    auto ret=interface.encrypt(buf.c_str(),output.get(),buf.size(),out_size);
    send(fd,output.get(),ret,0);
}
void load_balance_client::start_work()
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_is_running&&m_loop){
        m_is_running=true;
        update();
        if(m_timer_id==0)m_timer_id=m_loop->addTimer([this](){
            lock_guard<mutex>locker(m_mutex);
            update();
            return true;
        },UNPDATE_INTERVAL);
    }
}
void load_balance_client::stop_work()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_is_running&&m_loop){
        m_loop->removeTimer(m_timer_id);
        m_timer_id=0;
        m_is_running=false;
    }
}
