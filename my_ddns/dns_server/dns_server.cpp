#include "dns_server.h"
#include <cstdlib>
using namespace micagent;
using namespace std;
dns_server::data_base dns_server::m_data_base;
dns_server::dns_server(uint16_t port,int64_t cache_time):m_max_cache_time(cache_time),\
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
void dns_server::config(weak_ptr<EventLoop> loop)
{
    stop_work();
    lock_guard<mutex>locker(m_mutex);
    m_event_loop=loop;
}
void dns_server::start_work()
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
void dns_server::stop_work()
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
dns_server::~dns_server()
{
    if(m_is_running)stop_work();
}
/*
 * {
 *     cmd : xxx,
 * }
 *
 */
void dns_server::handle_read()
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
            printf("%s\r\n",object.ToFormattedString().c_str());
            if (cmd=="dns_find") {
                handle_dns_find(object);
            }
            else if(cmd=="update"){
                handle_update(object);
            }
            else if(cmd=="register")handle_register(object);
            else if (cmd=="unregister") {
                handle_unregister(object);
            }
            else if (cmd=="account_find") {
                handle_account_find(object);
            }
            else if(cmd=="port_check"){
                handle_port_check();
            }
            else {

            }
        }
    }
}
void dns_server::handle_register(neb::CJsonObject &object)
{
    data_session data;
    if(!object.Get("domain_name",data.domain_name))return;
    neb::CJsonObject res;
    res.Add("cmd","register_response");
    if(m_data_base.find_dns_server_session(data.domain_name,data)){
        res.Add("info","domain exist");
        response(res.ToString());
    }
    else {
        if(!object.Get("account",data.account))return;
        if(!object.Get("password",data.password))return;
        if(!object.Get("other_info",data.other_info))return;
        data.search_count=0;
        data.last_login_time=Timer::getTimeNow();
        m_data_base.update_dns_server_session(data);
        res.Add("info","success");
        response(res.ToString());
    }
}
void dns_server::handle_unregister(neb::CJsonObject &object)
{
    data_session data;
    if(!object.Get("domain_name",data.domain_name))return;
    neb::CJsonObject res;
    res.Add("cmd","unregister_response");
    if(!m_data_base.find_dns_server_session(data.domain_name,data)){
        res.Add("info","domain not exist");
        response(res.ToString());
    }
    else {
        string announce;
        if(!object.Get("announce",announce))return;
        string password;
        if(!object.Get("password",password))return;
        string key=data.domain_name+announce+data.password;
        char * hash_string=Get_MD5_String(key.c_str(),key.size());
        if(password!=hash_string){
            res.Add("info","false password");
            response(res.ToString());
        }
        else {
            {
                lock_guard<mutex>locker(m_mutex);
                m_session_cache_map.erase(data.domain_name);
            }
            m_data_base.remove_dns_server_session(data.domain_name);
            res.Add("info","success");
            response(res.ToString());
        }
        free(hash_string);
    }
}
void dns_server::handle_account_find(neb::CJsonObject&object)
{
    data_session data;
    if(!object.Get("domain_name",data.domain_name))return;
    neb::CJsonObject res;
    res.Add("cmd","account_find_response");
    if(!m_data_base.find_dns_server_session(data.domain_name,data)){
        res.Add("info","domain not exist");
        response(res.ToString());
    }
    else {
        res.Add("info","success");
        res.Add("account",data.account);
        res.Add("other_info",data.other_info);
        response(res.ToString());
    }
}
void dns_server::handle_dns_find(neb::CJsonObject&object)
{
    string domain_name;
    if(!object.Get("domain_name",domain_name))return;
    string account;
    if(!object.Get("account",account))return;
    string announce;
    if(!object.Get("announce",announce))return;
    string password;
    if(!object.Get("password",password))return;
    neb::CJsonObject res;
    res.Add("cmd","dns_find_response");
    res.Add("account",account);
    res.Add("domain_name",domain_name);
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_cache_map.find(domain_name);
    if(iter!=m_session_cache_map.end()){
        if(iter->second.session.account!=account){
            res.Add("info","account not match");
            response(res.ToString());
        }
        else {
            string key=iter->second.session.domain_name+announce+iter->second.session.password;
            char * hash_string=Get_MD5_String(key.c_str(),key.size());
            if(password!=hash_string){
                res.Add("info","password not match");
                response(res.ToString());
                return;
            }
            res.Add("info","success");
            res.Add("internal_ip",iter->second.internal_ip);
            res.Add("external_ip",iter->second.external_ip);
            res.Add("user_account_name",iter->second.user_account_name);
            res.Add("user_account_password",iter->second.user_account_password);
            res.AddEmptySubArray("port_map");
            for(auto i : iter->second.port_map){
                neb::CJsonObject port_info;
                port_info.Add("name",i.second.name);
                port_info.Add("external_port",i.second.external_port);
                port_info.Add("internal_port",i.second.internal_port);
                res["port_map"].Add(port_info);
            }
            response((res.ToString()));
        }

    }
    else {
        res.Add("info","domain not online");
        response(res.ToString());
    }
}
void dns_server::handle_update(neb::CJsonObject&object)
{
    string domain_name;
    if(!object.Get("domain_name",domain_name))return;
    string account;
    if(!object.Get("account",account))return;
    string announce;
    if(!object.Get("announce",announce))return;
    string password;
    if(!object.Get("password",password))return;
    neb::CJsonObject res;
    res.Add("cmd","update_response");
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_cache_map.find(domain_name);
    if(iter==m_session_cache_map.end()){
        data_session session;
        if(!m_data_base.find_dns_server_session(domain_name,session))return;
        session_info cache_session;
        session.last_login_time=Timer::getTimeNow();
        m_data_base.update_dns_server_session(session);
        cache_session.session=session;
        auto iter2=m_session_cache_map.emplace(domain_name,cache_session);
        //just for safety check
        if(!iter2.second)return;
        iter=iter2.first;
    }
    do{
        if(iter==m_session_cache_map.end())break;
        string key=iter->second.session.domain_name+announce+iter->second.session.password;
        char * hash_string=Get_MD5_String(key.c_str(),key.size());
        if(password!=hash_string){
            free(hash_string);
            break;
        }
        free(hash_string);
        object.Get("internal_ip",iter->second.internal_ip);
        object.Get("user_account_name",iter->second.user_account_name);
        object.Get("user_account_password",iter->second.user_account_password);
        iter->second.last_alive_time=Timer::getTimeNow();
        iter->second.external_ip=inet_ntoa(m_last_recv_addr.sin_addr);
        neb::CJsonObject port_map;
        if(!object.Get("port_map",port_map))break;
        int size=port_map.GetArraySize();
        for(int i=0;i<size;i++){
            port_info info;
            if(!port_map[i].Get("name",info.name))continue;
            uint32_t port;
            if(!port_map[i].Get("external_port",port))continue;
            info.external_port=port;
            if(!port_map[i].Get("internal_port",port))continue;
            info.internal_port=port;
            iter->second.port_map[info.name]=info;
        }
    }while(0);
}
void dns_server::handle_port_check()
{
    neb::CJsonObject res;
    res.Add("cmd","port_check_response");
    res.Add("ip",inet_ntoa(m_last_recv_addr.sin_addr));
    res.Add("port",ntohs(m_last_recv_addr.sin_port));
    response(res.ToString());
}
void dns_server::check_sessions()
{
    lock_guard<mutex>locker(m_mutex);
    auto time_now=Timer::getTimeNow();
    for(auto iter=m_session_cache_map.begin();iter!=m_session_cache_map.end();)
    {
        if(time_now-iter->second.last_alive_time>m_max_cache_time)m_session_cache_map.erase(iter++);
        else {
            iter++;
        }
    }
}
void dns_server::response(const string &buf)
{
    uint32_t out_size=buf.size()+8;
    shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
    rc4_interface interface(Timer::getTimeNow);
    auto ret=interface.encrypt(buf.c_str(),output.get(),buf.size(),out_size);
    if(m_channel){
        sendto(m_channel->fd(),output.get(),ret,0,(sockaddr *)&m_last_recv_addr,m_sock_len);
    }
}
