#include "dns_client.h"
#include"MD5.h"
using namespace micagent;
dns_client::dns_client(string server_ip,uint16_t dns_port):\
m_timer_id(0),m_server_ip(server_ip),m_server_port(dns_port),m_is_running(false),m_send_fd(INVALID_SOCKET)
{
     m_send_fd=Network_Util::Instance().build_socket(UDP);
    memset(&m_server_addr,0,sizeof (m_server_addr));
    m_server_addr.sin_family=AF_INET;
    if(m_send_fd==INVALID_SOCKET)throw runtime_error("can't build udp socket!");
    m_server_addr.sin_addr.s_addr=inet_addr(m_server_ip.c_str());
    m_server_addr.sin_port=htons(m_server_port);
    Network_Util::Instance().connect(m_send_fd,m_server_addr);
}
void dns_client::config(weak_ptr<EventLoop> loop, string domain_name, string account, string password, int64_t upload_interval)
{
    stop_work();
    lock_guard<mutex>locker(m_mutex);
    m_loop=loop;
    m_domain_name=domain_name;
    m_account=account;
    m_upload_interval=upload_interval;
    auto tmp=Get_MD5_String(password.c_str(),password.size());
    m_password=tmp;
    free(tmp);
}
void dns_client::reset_server_info(string server_ip,uint16_t dns_port)
{
    lock_guard<mutex>locker(m_mutex);
    if(m_send_fd!=INVALID_SOCKET)NETWORK.close_socket(m_send_fd);
    m_server_ip=server_ip;
    m_server_port=dns_port;
    m_send_fd=Network_Util::Instance().build_socket(UDP);
    memset(&m_server_addr,0,sizeof (m_server_addr));
    m_server_addr.sin_family=AF_INET;
    m_server_addr.sin_addr.s_addr=inet_addr(m_server_ip.c_str());
    m_server_addr.sin_port=htons(m_server_port);
    Network_Util::Instance().connect(m_send_fd,m_server_addr);
    perror("");
}
void dns_client::start_work()
{
    lock_guard<mutex>locker(m_mutex);
    auto event_loop=m_loop.lock();
    if(!m_is_running&&event_loop){
        m_is_running=true;
        update();
        if(m_timer_id==0)m_timer_id=event_loop->addTimer([this](){
            lock_guard<mutex>locker(m_mutex);
            update();
            return true;
        },m_upload_interval);
    }
}
void dns_client::stop_work()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_is_running){
    auto event_loop=m_loop.lock();
        if(event_loop)event_loop->removeTimer(m_timer_id);
        m_timer_id=0;
        m_is_running=false;
    }
}
void dns_client::set_port_map_item(string name,uint16_t external_port,uint16_t internal_port)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_port_map.find(name);
    if(iter!=end(m_port_map)){
        iter->second.internal_port=internal_port;
        iter->second.external_port=external_port;
    }
    else {
        m_port_map.emplace(name,port_info(name,internal_port,external_port));
    }
}
pair<bool,neb::CJsonObject> dns_client::register_to_server(string domain_name,string account,string password,string other_info,int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","register");
    object.Add(TO_STRING(domain_name),domain_name);
    auto tmp=Get_MD5_String(password.c_str(),password.size());
    object.Add(TO_STRING(account),account);
    object.Add(TO_STRING(password),tmp);
    object.Add(TO_STRING(other_info),other_info);
    free(tmp);
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
    if(read_len<=8)return {false,neb::CJsonObject()};
    else {
        uint32_t out_size=read_len-8;
        shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
        rc4_interface interface(Timer::getTimeNow);
        auto ret_len=interface.decrypt(recv_buf.get(),output.get(),read_len,out_size,interface.generate_key());
        return {true,neb::CJsonObject(string(output.get(),ret_len))};
    }
}
pair<bool,neb::CJsonObject>  dns_client::unregister_to_server(string domain_name,string password,int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","unregister");
    object.Add(TO_STRING(domain_name),domain_name);
    auto tmp=Get_MD5_String(password.c_str(),password.size());
    auto announce=to_string(Timer::getTimeNow());
    object.Add(TO_STRING(announce),announce);
    password=domain_name+announce+tmp;
    free(tmp);
    tmp=Get_MD5_String(password.c_str(),password.size());
    object.Add(TO_STRING(password),tmp);
    free(tmp);
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
pair<bool,neb::CJsonObject>  dns_client::account_find(string domain_name,int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","account_find");
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
pair<bool,neb::CJsonObject>  dns_client::dns_find(string domain_name,string account,string password,int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","dns_find");
    object.Add(TO_STRING(domain_name),domain_name);
    auto tmp=Get_MD5_String(password.c_str(),password.size());
    auto announce=to_string(Timer::getTimeNow());
    object.Add(TO_STRING(account),account);
    object.Add(TO_STRING(announce),announce);
    password=domain_name+announce+tmp;
    free(tmp);
    tmp=Get_MD5_String(password.c_str(),password.size());
    object.Add(TO_STRING(password),tmp);
    free(tmp);
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
pair<bool,neb::CJsonObject> dns_client::port_check(int64_t time_out)
{
    neb::CJsonObject object;
    object.Add("cmd","port_check");
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
void dns_client::update()
{
    if(m_send_fd==INVALID_SOCKET)return;
    neb::CJsonObject object;
    object.Add("cmd","update");
    object.Add(TO_STRING(domain_name),m_domain_name);
    auto announce=to_string(Timer::getTimeNow());
    object.Add(TO_STRING(account),m_account);
    object.Add(TO_STRING(announce),announce);
    string password=m_domain_name+announce+m_password;
    auto tmp=Get_MD5_String(password.c_str(),password.size());
    object.Add(TO_STRING(password),tmp);
    free(tmp);
    auto net=Network_Util::Instance().get_net_interface_info();
    if(net.empty()){
        object.Add(TO_STRING(internal_ip),"0.0.0.0");
    }
    else {
        object.Add(TO_STRING(internal_ip),net[0].ip);
    }
    object.AddEmptySubArray("port_map");
    for(auto i: m_port_map){
        neb::CJsonObject port_info;
        port_info.Add("name",i.second.name);
        port_info.Add("internal_port",i.second.internal_port);
        port_info.Add("external_port",i.second.external_port);
        object["port_map"].Add(port_info);
    }
    rc4_send(m_send_fd,object.ToString());
}
void dns_client::rc4_send(SOCKET fd,const string &buf)
{
    uint32_t out_size=buf.size()+8;
    shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
    memset(output.get(),0,out_size);
    rc4_interface interface(Timer::getTimeNow);
    auto ret=interface.encrypt(buf.c_str(),output.get(),buf.size(),out_size);
    send(fd,output.get(),ret,0);
}
