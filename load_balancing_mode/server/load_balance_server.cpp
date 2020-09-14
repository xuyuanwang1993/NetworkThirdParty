#include "load_balance_server.h"
#include <cstdlib>
#include<cmath>
#include<set>
using namespace micagent;
using namespace std;
load_balance_server::load_balance_server(uint16_t port,int64_t cache_time):m_max_cache_time(cache_time),\
    m_update_timer(0),m_sock_len(sizeof (sockaddr_in)),m_is_running(false)
{
    auto sock=Network_Util::Instance().build_socket(UDP);
    if(sock==INVALID_SOCKET)throw runtime_error("can't build udp socket!");
    Network_Util::Instance().set_reuse_addr(sock);
    Network_Util::Instance().bind(sock,port);
    Network_Util::Instance().make_noblocking(sock);
    m_channel.reset(new Channel(sock));
    m_channel->setReadCallback([this](Channel *chn){
        (void )chn;
        this->handle_read();
        return true;
    });
    m_channel->enableReading();
    memset(&m_last_recv_addr,0,sizeof (m_last_recv_addr));
    m_last_recv_addr.sin_family=AF_INET;
}
void load_balance_server::config(weak_ptr<EventLoop> loop)
{
    stop_work();
    lock_guard<mutex>locker(m_mutex);
    m_event_loop=loop;
}
void load_balance_server::start_work()
{
    lock_guard<mutex>locker(m_mutex);
    auto event_loop=m_event_loop.lock();
    if(m_is_running||!event_loop)return;
    if(m_channel)event_loop->updateChannel(m_channel);
    if(m_update_timer==0)m_update_timer=event_loop->addTimer([this](){
        this->check_sessions();
        return true;
    },SESSION_CHECK_INTERVAL);
    m_is_running=true;
}
void load_balance_server::stop_work()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_is_running){
        m_is_running=false;
        auto event_loop=m_event_loop.lock();
        if(event_loop&&m_channel)event_loop->removeChannel(m_channel);
        if(event_loop&&m_update_timer!=0){
            event_loop->blockRemoveTimer(m_update_timer);
            m_update_timer=0;
        }
    }
}
load_balance_server::~load_balance_server()
{
    if(m_is_running)stop_work();
}
/*
 * {
 *     cmd : xxx,
 * }
 *
 */
void load_balance_server::handle_read()
{
    char buf[1400]={0};
    auto size=recvfrom(m_channel->fd(),buf,1400,0,(sockaddr *)&m_last_recv_addr,&m_sock_len);
    if(size>8){
        auto buf_size=size-8;
        shared_ptr<char>out_string(new char[buf_size],std::default_delete<char[]>());
        rc4_interface interface(Timer::getTimeNow);
        if(interface.decrypt(buf,out_string.get(),size,buf_size,rc4_interface::generate_key())>0)
        {
            neb::CJsonObject object(out_string.get());
            string cmd;
            if(!object.Get("cmd",cmd))return;
            if (cmd=="find") {
                handle_find(object);
            }
            else if(cmd=="specific_find"){
                handle_specific_find(object);
            }
            else if(cmd=="update")handle_update(object);
            else {

            }
        }
    }
}
void load_balance_server::handle_specific_find(neb::CJsonObject&object)
{
    string account;
    if(!object.Get("account",account))return;
    string domain_name;
    if(!object.Get("domain_name",domain_name))return;
    neb::CJsonObject res;
    res.Add("cmd","specific_find_response");
    res.Add("account",account);
    res.Add("domain_name",domain_name);
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_cache_map.find(account);
    if(iter!=m_session_cache_map.end()){
        auto iter2=iter->second.session_map.find(domain_name);
        if(iter2!=iter->second.session_map.end())
        {
            res.Add("info","success");
        }
        else {
            res.Add("info","domain not online");
        }
    }
    else{
        res.Add("info","domain not online");
    }
    response(res.ToString());
}
void load_balance_server::handle_find(neb::CJsonObject &object)
{
    string account;
    if(!object.Get("account",account))return;
    neb::CJsonObject exclude_list;
    if(!object.Get("exclude_list",exclude_list))return;
    set<string> exclude_domains;
    auto size=exclude_list.GetArraySize();
    for(decltype (size)i=0;i<size;i++)
    {
        string exclude_domain;
        if(!exclude_list.Get(i,exclude_domain))break;
        exclude_domains.insert(exclude_domain);
    }
    neb::CJsonObject res;
    res.Add("cmd","find_response");
    res.Add("account",account);
    lock_guard<mutex>locker(m_mutex);
    map<double,string> priority_cache;
    string match_ip=inet_ntoa(m_last_recv_addr.sin_addr);
    auto iter=m_session_cache_map.find(account);
    if(iter!=m_session_cache_map.end())
    {
        for(auto i:iter->second.session_map){
            auto check_iter=exclude_domains.find(i.second.domain_name);
            if(check_iter==exclude_domains.end())
            {
                auto tmp=calculate_priority(match_ip,i.second.ip)*(1-i.second.weight)+i.second.priority*i.second.weight;
                priority_cache.emplace(tmp,i.first);
            }
        }
    }

    if(priority_cache.empty()){
        res.Add("info","no availiable resource");
    }
    else {
        res.Add("info","success");
        res.Add("domain_name",priority_cache.begin()->second);
    }
    response(res.ToString());
}
void load_balance_server::handle_update(neb::CJsonObject&object)
{
    string account;
    if(!object.Get("account",account))return;
    string domain_name;
    if(!object.Get("domain_name",domain_name))return;
    double priority;
    if(!object.Get("priority",priority))return;
    double weight;
    if(!object.Get("weight",weight))return;
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_cache_map.find(account);
    if(iter==m_session_cache_map.end()){
        auto iter2=m_session_cache_map.emplace(account,account_session());
        if(iter2.second)iter=iter2.first;
    }
    if(iter!=m_session_cache_map.end()){
        auto iter2=iter->second.session_map.find(domain_name);
        if(iter2==iter->second.session_map.end())
        {
            auto tmp=iter->second.session_map.emplace(domain_name,session_info(account,domain_name));
            if(tmp.second)iter2=tmp.first;
        }
        iter2->second.ip=inet_ntoa(m_last_recv_addr.sin_addr);
        iter2->second.weight=weight;
        iter2->second.priority=priority;
        iter2->second.last_alive_time=Timer::getTimeNow();
    }
}
void load_balance_server::check_sessions()
{
    lock_guard<mutex>locker(m_mutex);
    auto time_now=Timer::getTimeNow();
    for(auto iter=m_session_cache_map.begin();iter!=m_session_cache_map.end();)
    {
        for(auto iter2=iter->second.session_map.begin();iter2!=iter->second.session_map.end();)
        {
            if(time_now-iter2->second.last_alive_time>m_max_cache_time)iter->second.session_map.erase(iter2++);
            else {
                iter2++;
            }
        }
        if(iter->second.session_map.empty())
        {
            m_session_cache_map.erase(iter++);
        }
        else {
            iter++;
        }
    }
}
void load_balance_server::response(const string &buf)
{
    uint32_t out_size=buf.size()+8;
    shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
    rc4_interface interface(Timer::getTimeNow);
    auto ret=interface.encrypt(buf.c_str(),output.get(),buf.size(),out_size);
    if(m_channel){
        sendto(m_channel->fd(),output.get(),ret,0,(sockaddr *)&m_last_recv_addr,m_sock_len);
    }
}
double load_balance_server::calculate_priority(const string &ip1,const string&ip2)
{
    do{
        int first,second,third,forth;
        sscanf(ip1.c_str(),"%d.%d.%d.%d",&first,&second,&third,&forth);
        int a,b,c,d;
        sscanf(ip2.c_str(),"%d.%d.%d.%d",&a,&b,&c,&d);
        //check
        int check=first|second|third|forth|a|b|c|d;
        if(check>255||check<0)break;
        return (abs(first-a)*TWO_POW_24+abs(second-b)*TWO_POW_16+abs(third-c)*TWO_POW_8+abs(forth-d))/IP_BASE;
    }while(0);
    return 1.0;
}
