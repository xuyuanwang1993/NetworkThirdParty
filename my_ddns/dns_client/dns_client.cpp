#include "dns_client.h"
#include"MD5.h"
#define INVALID_DOMAIN_TOKEN 0
using namespace micagent;
dns_client::dns_client(string server_ip,uint16_t dns_port):\
    m_timer_id(0),m_server_ip(server_ip),m_server_port(dns_port),m_is_running(false),m_send_fd(INVALID_SOCKET),m_domain_token(INVALID_DOMAIN_TOKEN)
{
    m_send_fd=Network_Util::Instance().build_socket(UDP);
    memset(&m_server_addr,0,sizeof (m_server_addr));
    m_server_addr.sin_family=AF_INET;
    if(m_send_fd==INVALID_SOCKET)throw runtime_error("can't build udp socket!");
    m_server_addr.sin_addr.s_addr=inet_addr(m_server_ip.c_str());
    m_server_addr.sin_port=htons(m_server_port);
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
    auto event_loop=m_loop.lock().get();
    if(m_send_channel){
        event_loop->removeChannel(m_send_channel);
        m_send_channel.reset();
        m_send_fd=INVALID_SOCKET;
    }
    if(m_send_fd!=INVALID_SOCKET){
        NETWORK.close_socket(m_send_fd);
    }
    m_server_ip=server_ip;
    m_server_port=dns_port;
    m_send_fd=Network_Util::Instance().build_socket(UDP);
    memset(&m_server_addr,0,sizeof (m_server_addr));
    m_server_addr.sin_family=AF_INET;
    m_server_addr.sin_addr.s_addr=inet_addr(m_server_ip.c_str());
    m_server_addr.sin_port=htons(m_server_port);
    read_init(event_loop);
}
void dns_client::start_work()
{
    lock_guard<mutex>locker(m_mutex);
    auto event_loop=m_loop.lock();
    if(!m_is_running&&event_loop){
        read_init(event_loop.get());
        m_is_running=true;
        update_domain_info();
        if(m_timer_id==0)m_timer_id=event_loop->addTimer([this](){
            lock_guard<mutex>locker(m_mutex);
            update_domain_info();
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
void dns_client::set_user_account_info(string account_name,string account_password)
{
    lock_guard<mutex>locker(m_mutex);
    m_user_account_name=account_name;
    m_user_account_password=account_password;
}
pair<bool,neb::CJsonObject> dns_client::register_to_server(string domain_name,string account,string password,string other_info,int64_t time_out)
{
    if(domain_name.empty())return   {false,neb::CJsonObject()};
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
    rc4_send(sock,object.ToString(),&m_server_addr);
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
    rc4_send(sock,object.ToString(),&m_server_addr);
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
    if(domain_name.empty())return   {false,neb::CJsonObject()};
    neb::CJsonObject object;
    object.Add("cmd","account_find");
    object.Add(TO_STRING(domain_name),domain_name);
    SOCKET sock=Network_Util::Instance().build_socket(UDP);
    if(sock==INVALID_SOCKET)return  {false,neb::CJsonObject()};
    //    {
    //        lock_guard<mutex>locker(m_mutex);
    //        Network_Util::Instance().connect(sock,m_server_addr);
    //    }
    rc4_send(sock,object.ToString(),&m_server_addr);
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
    if(domain_name.empty())return   {false,neb::CJsonObject()};
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
    //    {
    //        lock_guard<mutex>locker(m_mutex);
    //        Network_Util::Instance().connect(sock,m_server_addr);
    //    }
    rc4_send(sock,object.ToString(),&m_server_addr);
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
    //    {
    //        lock_guard<mutex>locker(m_mutex);
    //        Network_Util::Instance().connect(sock,m_server_addr);
    //    }
    rc4_send(sock,object.ToString(),&m_server_addr);
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
    if(m_send_fd==INVALID_SOCKET||m_domain_name.empty())return;
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
        bool find=false;
        for(auto i:net){
            if(i.is_default){
                object.Add(TO_STRING(internal_ip),i.ip);
                find=true;
                break;
            }
        }
        if(!find)object.Add(TO_STRING(internal_ip),net[0].ip);
    }
    object.Add(TO_STRING(user_account_name),m_user_account_name);
    object.Add(TO_STRING(user_account_password),m_user_account_password);
    object.AddEmptySubArray("port_map");
    for(auto i: m_port_map){
        neb::CJsonObject port_info;
        port_info.Add("name",i.second.name);
        port_info.Add("internal_port",i.second.internal_port);
        port_info.Add("external_port",i.second.external_port);
        object["port_map"].Add(port_info);
    }
    rc4_send(m_send_fd,object.ToString(),&m_server_addr);
}
void dns_client::read_init(EventLoop *loop)
{
    if(!loop)return;
    if(m_send_fd!=INVALID_SOCKET){
        NETWORK.make_noblocking(m_send_fd);
        if(m_send_channel)loop->removeChannel(m_send_channel);
        m_send_channel.reset(new Channel(m_send_fd));
        m_send_channel->enableReading();
        auto channel_weak=weak_ptr<Channel>(m_send_channel);
        m_send_channel->setReadCallback([channel_weak,this](Channel *){
            if(channel_weak.lock()){
                this->handle_read();
            }
            return true;
        });
        loop->updateChannel(m_send_channel);
    }
}
void dns_client::get_token()
{
    if(m_send_fd==INVALID_SOCKET||m_domain_name.empty()||m_account.empty())return;
    neb::CJsonObject object;
    object.Add("cmd","get_token");
    object.Add(TO_STRING(domain_name),m_domain_name);
    object.Add(TO_STRING(account),m_account);
    auto info=NETWORK.get_default_net_interface_info();
    object.Add("mac",info.mac);
    rc4_send(m_send_fd,object.ToString(),&m_server_addr);
}
void dns_client::update_domain_info()
{
    if(m_send_fd==INVALID_SOCKET||m_domain_name.empty())return;
    if(m_domain_token==INVALID_DOMAIN_TOKEN){
        get_token();
        return;
    }
    neb::CJsonObject object;
    object.Add("cmd","update_domain_info");
    object.Add(TO_STRING(domain_name),m_domain_name);
    object.Add(TO_STRING(token),m_domain_token);
    auto net=Network_Util::Instance().get_net_interface_info();
    if(net.empty()){
        object.Add(TO_STRING(internal_ip),"0.0.0.0");
    }
    else {
        bool find=false;
        for(auto i:net){
            if(i.is_default){
                object.Add(TO_STRING(internal_ip),i.ip);
                find=true;
                break;
            }
        }
        if(!find)object.Add(TO_STRING(internal_ip),net[0].ip);
    }
    object.Add(TO_STRING(user_account_name),m_user_account_name);
    object.Add(TO_STRING(user_account_password),m_user_account_password);
    object.AddEmptySubArray("port_map");
    for(auto i: m_port_map){
        neb::CJsonObject port_info;
        port_info.Add("name",i.second.name);
        port_info.Add("internal_port",i.second.internal_port);
        port_info.Add("external_port",i.second.external_port);
        object["port_map"].Add(port_info);
    }
    rc4_send(m_send_fd,object.ToString(),&m_server_addr);
}
void dns_client::handle_read()
{
    shared_ptr<char>recv_buf(new char[1500],std::default_delete<char[]>());
    lock_guard<mutex>locker(m_mutex);
    auto read_len=read_packet(m_send_fd,recv_buf.get(),1500,0);
    if(read_len>0) {
        uint32_t out_size=read_len-8;
        shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
        rc4_interface interface(Timer::getTimeNow);
        auto ret_len=interface.decrypt(recv_buf.get(),output.get(),read_len,out_size,interface.generate_key());
        neb::CJsonObject  response(string(output.get(),ret_len));
        string cmd;
        if(!response.Get("cmd",cmd)){
            MICAGENT_DEBUG("unknown response %s!",response.ToFormattedString().c_str());
        }
        else {
            //MICAGENT_DEBUG("response %s!",response.ToFormattedString().c_str());
            if("get_token_response"==cmd){
                handle_get_token_response(response);
            }
            else if ("update_domain_info_response"==cmd) {
                handle_update_domain_info_response(response);
            }
            else {

            }
        }
    }
}
void dns_client::handle_get_token_response(const neb::CJsonObject &object)
{
    string domain_name;
    string account;
    uint32_t token;
    do{
        if(!object.Get("domain_name",domain_name))break;
        if(!object.Get("account",account))break;
        if(!object.Get("token",token))break;
        if(domain_name!=m_domain_name)break;
        if(account!=m_account)break;
        m_domain_token=token;
    }while(0);

}
void dns_client::handle_update_domain_info_response(const neb::CJsonObject &object)
{
    uint32_t token;
    if(!object.Get("token",token)){
        m_domain_token=INVALID_DOMAIN_TOKEN;
    }
    else {
        m_domain_token=token;
    }
}
void dns_client::rc4_send(SOCKET fd, const string &buf, const sockaddr_in *addr)
{
    //MICAGENT_DEBUG("%s",buf.c_str());
    uint32_t out_size=buf.size()+8;
    shared_ptr<char>output(new char[out_size],std::default_delete<char[]>());
    memset(output.get(),0,out_size);
    rc4_interface interface(Timer::getTimeNow);
    auto ret=interface.encrypt(buf.c_str(),output.get(),buf.size(),out_size);
    if(addr){
        ::sendto(fd,output.get(),ret,0,(const sockaddr*)addr,sizeof (sockaddr_in));
    }
    else {
        send(fd,output.get(),ret,0);
    }
}
