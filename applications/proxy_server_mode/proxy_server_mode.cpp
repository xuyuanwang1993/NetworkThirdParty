#include "proxy_server_mode.h"
#include "daemon_instance.h"
#define MICAGENT_CROSS 0
using namespace micagent;
proxy_server_mode::proxy_server_mode():m_log_open(false),m_log_path(""),m_is_running(false),m_reload_flag(false)
{

}
void proxy_server_mode::init(const string &json_config_path,const string&pro_name)
{
    lock_guard<mutex>locker(m_mutex);
    do{
        m_pro_name=pro_name;
        m_config_path=json_config_path;
        char read_path[4096]={0};
        realpath(json_config_path.c_str(),read_path);
        m_config_path=read_path;
        m_json_config.reset(CJsonObject::CreateInstance(json_config_path));
        if(!parse_config())break;
        MICAGENT_INFO("init success!");
        return ;
    }while(0);
    load_default_config(json_config_path);
}
void proxy_server_mode::generate_daemon_config(const string &pro_name,const string &daemon_config_path)
{
    MICAGENT_INFO("pro_name:%s == daemon_config_path :%s",pro_name.c_str(),daemon_config_path.c_str());
    CJsonObject config;
    CJsonObject entry_pro;
    entry_pro.Add("pro_name",pro_name);
    entry_pro.Add("cmd_options",m_config_path);
    entry_pro.Add("wait_time",0);
    entry_pro.Add("path","./");
    config.Add(entry_pro);
    //rtsp_port check
    CJsonObject entry_rtsp;
    entry_rtsp.Add("pro_name",PORT_CHECK_SHELL);
    string port_check_options=to_string(m_rtsp_server_port)+" "+pro_name+" "+"tcp";
    entry_rtsp.Add("cmd_options",port_check_options);
    entry_rtsp.Add("wait_time",5);
    entry_rtsp.Add("path","./");
    config.Add(entry_rtsp);
    //rtsp_proxy check
    CJsonObject entry_rtsp_proxy;
    entry_rtsp_proxy.Add("pro_name",PORT_CHECK_SHELL);
    string entry_rtsp_proxy_port_check_options=to_string(m_rtsp_proxy_port)+" "+pro_name+" "+"tcp";
    entry_rtsp_proxy.Add("cmd_options",entry_rtsp_proxy_port_check_options);
    entry_rtsp_proxy.Add("wait_time",5);
    entry_rtsp_proxy.Add("path","./");
    config.Add(entry_rtsp_proxy);
    //rtsp_proxy_udp
    CJsonObject entry_rtsp_proxy_udp;
    entry_rtsp_proxy_udp.Add("pro_name",PORT_CHECK_SHELL);
    string entry_rtsp_proxy_udp_port_check_options=to_string(m_rtsp_proxy_port)+" "+pro_name+" "+"udp";
    entry_rtsp_proxy_udp.Add("cmd_options",entry_rtsp_proxy_udp_port_check_options);
    entry_rtsp_proxy_udp.Add("wait_time",5);
    entry_rtsp_proxy_udp.Add("path","./");
    config.Add(entry_rtsp_proxy_udp);
    daemon_instance::modify_run_config(daemon_config_path,config);
}
void proxy_server_mode::start()
{
    if(!m_is_running){
        lock_guard<mutex>locker(m_mutex);
        //log
        if(!m_log_open){
            Logger::Instance().unregister_handle();
        }
        else {
            Logger::Instance().register_handle();
            Logger::Instance().set_log_to_std(true);
            Logger::Instance().set_clear_flag(true);
            if(!m_log_path.empty())
            {
                Logger::Instance().set_log_path(m_log_path,m_pro_name);
            }
        }
        if(!m_event_loop)
        {
            m_event_loop.reset(new EventLoop(m_event_thread_pool_size,m_event_trigger_threads,m_event_trigger_queue_size,m_event_network_io_threads));
        }
        //init unix_socket
        if(!m_message_fd)
        {
            string path_prefix=UNIX_SOCKET_PREFIX;
            m_message_fd.reset(new unix_dgram_socket(path_prefix+m_pro_name));
            m_message_fd->build();
            MICAGENT_BACKTRACE("unix_socket path %s  fd %d",(path_prefix+m_pro_name).c_str(),m_message_fd->fd());
            m_message_channel.reset(new Channel(m_message_fd->fd()));
            m_message_channel->setReadCallback([this](Channel *){
                message_handle();
                return true;
            });
            m_message_channel->enableReading();
            m_event_loop->updateChannel(m_message_channel);
        }
        if(!m_dns_client)
        {
            string dns_server_ip=NETWORK.parase_domain(m_dns_server_domain);
            m_dns_client.reset(new dns_client(dns_server_ip,m_dns_server_port));
            m_dns_client->config(m_event_loop,m_dns_domain_name,m_dns_account_name,m_dns_pass_word,m_dns_upload_interval_ms);
            m_dns_client->set_port_map_item("rtsp",m_rtsp_server_external_port, m_rtsp_server_port);
            m_dns_client->set_port_map_item("rtsp_proxy",m_rtsp_proxy_external_port,m_rtsp_proxy_port);
            m_dns_client->set_user_account_info(m_rtsp_account_name,m_rtsp_account_password);
        }
        //m_dns_client->register_to_server(m_dns_domain_name,m_dns_account_name,m_dns_pass_word,m_dns_description);
        m_dns_client->start_work();
        if(!m_load_balance_client)
        {
            string balance_server_ip=NETWORK.parase_domain(m_balance_server_domain);
            m_load_balance_client.reset(new load_balance_client());
            m_load_balance_client->config_server_info(m_event_loop,balance_server_ip,m_balance_server_port);
            m_load_balance_client->config_client_info(m_dns_account_name,m_dns_domain_name,m_balance_max_payload_size,m_balance_server_weight,m_balance_upload_interval_ms);
        }
        m_load_balance_client->start_work();
        if(!m_rtsp_server)
        {
            m_rtsp_server.reset(new rtsp_server(m_rtsp_server_port));
            m_rtsp_server->resgisterMediaSessionCallback([this](string new_url){
                lock_guard<mutex>locker(m_mutex);
                auto iter=m_url_set.find(new_url);
                if(iter==end(m_url_set)){
                    m_load_balance_client->increase_load(1);
                    m_url_set.insert(new_url);
                }
            },[this](string new_url){
                lock_guard<mutex>locker(m_mutex);
                auto iter=m_url_set.find(new_url);
                if(iter!=end(m_url_set)){
                    m_load_balance_client->decrease_load(1);
                    m_url_set.erase(iter);
                }
            });
            m_rtsp_server->addAuthorizationInfo(m_rtsp_account_name,m_rtsp_account_password);
        }
        m_rtsp_server->register_handle(m_event_loop);
        if(!m_proxy_server)
        {
            m_proxy_server.reset(new proxy_server(m_rtsp_proxy_port));
            m_proxy_server->set_rtsp_server(m_rtsp_server);
            weak_ptr<proxy_server>weak_proxy_server(m_proxy_server);
            m_rtsp_server->setNoticeClientNumsCallback([weak_proxy_server](uint32_t token,uint32_t client_nums){
                auto server=weak_proxy_server.lock();
                if(!server)return ;
                server->handle_stream_state(token,client_nums);
            });
        }
        m_proxy_server->register_handle(m_event_loop);
        upnp_helper::Instance().config(m_event_loop,m_set_external_ip,m_router_ip);
        upnp_helper::Instance().add_port_task(TCP,m_rtsp_server_port,m_rtsp_server_external_port,"micagent");
        upnp_helper::Instance().add_port_task(UDP,m_rtsp_proxy_port,m_rtsp_proxy_external_port,"micagent");
        upnp_helper::Instance().add_port_task(TCP,m_rtsp_proxy_port,m_rtsp_proxy_external_port,"micagent");
    }
    if(!m_is_running)
    {
        m_is_running.exchange(true);
        loop();
    }
    if(m_reload_flag)
    {
        MICAGENT_INFO("mode reload!");
        m_reload_flag.exchange(false);
        parse_config();
        start();
    }
}
void proxy_server_mode::stop()
{
    if(m_is_running)
    {
        lock_guard<mutex>locker(m_mutex);
        upnp_helper::Instance().delete_port_task(TCP,m_rtsp_server_external_port,"micagent");
        upnp_helper::Instance().delete_port_task(UDP,m_rtsp_proxy_external_port,"micagent");
        upnp_helper::Instance().delete_port_task(TCP,m_rtsp_proxy_external_port,"micagent");
        if(m_proxy_server)
        {
            m_proxy_server->unregister_handle();
            m_proxy_server.reset();
        }
        if(m_rtsp_server)
        {
            m_rtsp_server->unregister_handle();
            m_rtsp_server.reset();
        }
        if(m_load_balance_client)
        {
            m_load_balance_client->stop_work();
            m_load_balance_client.reset();
        }
        if(m_dns_client)
        {
            m_dns_client->stop_work();
            m_dns_client.reset();
        }
        if(m_message_fd)
        {
            if(m_message_channel){
                m_event_loop->removeChannel(m_message_channel);
                m_message_channel.reset();
            }
            m_message_fd.reset();
        }
        m_is_running.exchange(false);
        {
            lock_guard<mutex>locker2(m_exit_mutex);
            m_exit_conn.notify_all();
        }
        MICAGENT_INFO("stop over!");
    }

}
proxy_server_mode::~proxy_server_mode()
{
    stop();
    if(m_event_loop)m_event_loop->stop();
}
void proxy_server_mode::loop()
{
    MICAGENT_INFO("loop start!");
    unique_lock<mutex>locker(m_exit_mutex);
    while(m_is_running)
    {
        m_exit_conn.wait(locker);
    }
    MICAGENT_INFO("loop exit!");
}
bool proxy_server_mode::parse_config()
{
    do{
        //event_loop
        if(!m_json_config->Get("event_thread_pool_size",m_event_thread_pool_size)){
            MICAGENT_ERROR("no event_thread_pool_size entry!");
            break;
        }
        if(!m_json_config->Get("event_trigger_threads",m_event_trigger_threads)){
            MICAGENT_ERROR("no event_trigger_threads entry!");
            break;
        }
        if(!m_json_config->Get("event_trigger_queue_size",m_event_trigger_queue_size)){
            MICAGENT_ERROR("no event_trigger_queue_size entry!");
            break;
        }
        if(!m_json_config->Get("event_network_io_threads",m_event_network_io_threads)){
            MICAGENT_ERROR("no event_network_io_threads entry!");
            break;
        }
        //dns client
        if(!m_json_config->Get("dns_upload_interval_ms",m_dns_upload_interval_ms)){
            MICAGENT_ERROR("no dns_upload_interval_ms entry!");
            break;
        }
        if(!m_json_config->Get("dns_server_domain",m_dns_server_domain)){
            MICAGENT_ERROR("no dns_server_domain entry!");
            break;
        }
        uint32_t dns_server_port;
        if(!m_json_config->Get("dns_server_port",dns_server_port)){
            MICAGENT_ERROR("no dns_server_port entry!");
            break;
        }
        m_dns_server_port=dns_server_port&0xffff;
        if(!m_json_config->Get("dns_domain_name",m_dns_domain_name)){
            MICAGENT_ERROR("no dns_domain_name entry!");
            break;
        }
        if(!m_json_config->Get("dns_account_name",m_dns_account_name)){
            MICAGENT_ERROR("no dns_account_name entry!");
            break;
        }
        if(!m_json_config->Get("dns_pass_word",m_dns_pass_word)){
            MICAGENT_ERROR("no dns_pass_word entry!");
            break;
        }
        if(!m_json_config->Get("dns_description",m_dns_description)){
            MICAGENT_ERROR("no dns_description entry!");
            break;
        }
        //balance
        if(!m_json_config->Get("balance_upload_interval_ms",m_balance_upload_interval_ms)){
            MICAGENT_ERROR("no balance_upload_interval_ms entry!");
            break;
        }
        if(!m_json_config->Get("balance_server_domain",m_balance_server_domain)){
            MICAGENT_ERROR("no balance_server_domain entry!");
            break;
        }
        uint32_t balance_server_port;
        if(!m_json_config->Get("balance_server_port",balance_server_port)){
            MICAGENT_ERROR("no balance_server_port entry!");
            break;
        }
        m_balance_server_port=balance_server_port&0xffff;
        if(!m_json_config->Get("balance_max_payload_size",m_balance_max_payload_size)){
            MICAGENT_ERROR("no balance_max_payload_size entry!");
            break;
        }
        if(!m_json_config->Get("balance_server_weight",m_balance_server_weight)){
            MICAGENT_ERROR("no balance_server_weight entry!");
            break;
        }
        //rtsp_server
        uint32_t rtsp_server_port;
        if(!m_json_config->Get("rtsp_server_port",rtsp_server_port)){
            MICAGENT_ERROR("no rtsp_server_port entry!");
            break;
        }
        if(!m_json_config->Get("rtsp_server_port",rtsp_server_port)){
            MICAGENT_ERROR("no rtsp_server_port entry!");
            break;
        }
        m_rtsp_server_port=rtsp_server_port&0xffff;
        if(!m_json_config->Get("rtsp_account_name",m_rtsp_account_name)){
            MICAGENT_ERROR("no rtsp_account_name entry!");
            break;
        }
        if(!m_json_config->Get("rtsp_account_password",m_rtsp_account_password)){
            MICAGENT_ERROR("no rtsp_account_password entry!");
            break;
        }
        //rtsp_proxy
        uint32_t rtsp_proxy_port;
        if(!m_json_config->Get("rtsp_proxy_port",rtsp_proxy_port)){
            MICAGENT_ERROR("no rtsp_proxy_port entry!");
            break;
        }
        m_rtsp_proxy_port=rtsp_proxy_port&0xffff;
        //upnp
        if(!m_json_config->Get("set_external_ip",m_set_external_ip)){
            MICAGENT_ERROR("no set_external_ip entry!");
            break;
        }
        if(!m_json_config->Get("router_ip",m_router_ip)){
            MICAGENT_ERROR("no router_ip entry!");
            break;
        }
        uint32_t rtsp_server_external_port;
        if(!m_json_config->Get("rtsp_server_external_port",rtsp_server_external_port)){
            MICAGENT_ERROR("no rtsp_server_external_port entry!");
            break;
        }
        m_rtsp_server_external_port=rtsp_server_external_port&0xffff;
        uint32_t rtsp_proxy_external_port;
        if(!m_json_config->Get("rtsp_proxy_external_port",rtsp_proxy_external_port)){
            MICAGENT_ERROR("no rtsp_proxy_external_port entry!");
            break;
        }
        m_rtsp_proxy_external_port=rtsp_proxy_external_port&0xffff;
        //itself
        if(!m_json_config->Get("log_open",m_log_open)){
            MICAGENT_ERROR("no log_open entry!");
            break;
        }
        if(!m_json_config->Get("log_path",m_log_path)){
            MICAGENT_ERROR("no log_path entry!");
            break;
        }
        return true;
    }while(0);
    return false;
}
void proxy_server_mode::load_default_config(const string &json_config_path)
{
    MICAGENT_WARNNING("load default config!");
    m_json_config.reset(new CJsonObject());
    //event
    m_event_thread_pool_size=DEFAULT_THREAD_POOL_SIZE;
    m_event_trigger_threads=DEFAULT_TRIGGER_THREADS;
    m_event_trigger_queue_size=DEFAULT_TRIGGER_QUEUE_SIZE;
    m_event_network_io_threads=DEFAULT_NETWORK_IO_THREADS;
    m_json_config->Add("event_thread_pool_size",m_event_thread_pool_size);
    m_json_config->Add("event_trigger_threads",m_event_trigger_threads);
    m_json_config->Add("event_trigger_queue_size",m_event_trigger_queue_size);
    m_json_config->Add("event_network_io_threads",m_event_network_io_threads);
    //dns
    m_dns_upload_interval_ms=DEFAULT_DNS_UPLOAD_INTERVAL_MS;
    m_dns_server_domain="www.meanning.com";
    m_dns_server_port=DEFAULT_DNS_PORT;
    m_dns_domain_name="";
    m_dns_account_name="admin";
    m_dns_pass_word="micagent";
    m_dns_description="test_dns";
    m_json_config->Add("dns_upload_interval_ms",m_dns_upload_interval_ms);
    m_json_config->Add("dns_server_domain",m_dns_server_domain);
    m_json_config->Add("dns_server_port",m_dns_server_port);
    m_json_config->Add("dns_domain_name",m_dns_domain_name);
    m_json_config->Add("dns_account_name",m_dns_account_name);
    m_json_config->Add("dns_pass_word",m_dns_pass_word);
    m_json_config->Add("dns_description",m_dns_description);
    //load_balance
    m_balance_upload_interval_ms=DEFAULT_BALANCE_UPLOAD_INTERVAL_MS;
    m_balance_server_domain="www.meanning.com";
    m_balance_server_port=DEFAULT_BALANCE_PORT;
    m_balance_max_payload_size=DEFAULT_MAX_PAYLOAD_SIZE;
    m_balance_server_weight=DEFAULT_SERVER_WEIGHT;
    m_json_config->Add("balance_upload_interval_ms",m_balance_upload_interval_ms);
    m_json_config->Add("balance_server_domain",m_balance_server_domain);
    m_json_config->Add("balance_server_port",m_balance_server_port);
    m_json_config->Add("balance_max_payload_size",m_balance_max_payload_size);
    m_json_config->Add("balance_server_weight",m_balance_server_weight);
    //rtsp_server
    m_rtsp_server_port=DEFAULT_RTSP_SERVER_PORT;
    m_rtsp_account_name="admin";
    m_rtsp_account_password="micagent";
    m_json_config->Add("rtsp_server_port",m_rtsp_server_port);
    m_json_config->Add("rtsp_account_name",m_rtsp_account_name);
    m_json_config->Add("rtsp_account_password",m_rtsp_account_password);
    //rtsp_proxy
    m_rtsp_proxy_port=DEFAULT_RTSP_PROXY_PORT;
    m_json_config->Add("rtsp_proxy_port",m_rtsp_proxy_port);
    //upnp
    m_set_external_ip=false;
    m_router_ip="";
    m_rtsp_server_external_port=m_rtsp_server_port;
    m_rtsp_proxy_external_port=m_rtsp_proxy_port;
    m_json_config->Add("set_external_ip",m_set_external_ip,m_set_external_ip);
    m_json_config->Add("router_ip",m_router_ip);
    m_json_config->Add("rtsp_server_external_port",m_rtsp_server_external_port);
    m_json_config->Add("rtsp_proxy_external_port",m_rtsp_proxy_external_port);
    //itself
    m_log_open=true;
    m_log_path="/home/microcreat/log/";
    m_json_config->Add("log_open",m_log_open,m_log_open);
    m_json_config->Add("log_path",m_log_path);
    m_json_config->SetSavePath(json_config_path);
    m_json_config->SaveToFile();
}
void proxy_server_mode::message_handle()
{
    const uint32_t buf_size=32*1024;
    static const uint32_t path_len=108;
    shared_ptr<char>cache_buf(new char[buf_size],default_delete<char[]>());
    ssize_t size=0;
    {
        lock_guard<mutex>locker(m_mutex);
        size=m_message_fd->recv(cache_buf.get(),buf_size);
    }
    if(size>path_len)
    {
        string from(cache_buf.get(),path_len);
        string message(cache_buf.get()+path_len,size-path_len);
        MICAGENT_WARNNING("message :%s from :%s",message.c_str(),from.c_str());
        do{
            CJsonObject object(message);
            string cmd;
            if(!object.Get("cmd",cmd))break;
            if(cmd=="get_config"){
                handle_get_config(from);
            }
            else if (cmd=="update_config") {
                handle_update_config(object,from);
            }
            else if (cmd=="get_url_info") {
                handle_get_url_info(from);
            }
            else if (cmd=="reload_mode"){
                handle_reload_mode(from);
            }
            else if (cmd=="save_config"){
                handle_save_config(from);
            }
            else  if (cmd=="get_net_config"){
                handle_get_net_config(from);
            }
            else  if (cmd=="update_net_info"){
                handle_update_net_info(object,from);
            }
            else  if (cmd=="restart"){
                handle_restart(from);
            }
            else if (cmd=="device_search") {
                handle_device_search(object,from);
            }
            else {
                handle_cmd_unsupported(cmd,from);
            }
        }while(0);

    }
}
void proxy_server_mode::handle_get_config(const string&from)
{
    CJsonObject response;
    CJsonObject json_response;
    response.Add("cmd","get_config_response");
    lock_guard<mutex>locker(m_mutex);
    json_response.Add("json_config",*m_json_config);
    response.Add("response",json_response);
    auto str_response=response.ToString();
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
}
void proxy_server_mode::handle_save_config(const string&from)
{
    CJsonObject response;
    CJsonObject json_response;
    response.Add("cmd","save_config_response");
    json_response.Add("info","success");
    response.Add("response",json_response);
    auto str_response=response.ToString();
    lock_guard<mutex>locker(m_mutex);
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
    m_json_config->SaveToFile();
}
void proxy_server_mode::handle_update_config(const CJsonObject&object,const string&from)
{
    CJsonObject response;
    CJsonObject json_response;
    response.Add("cmd","update_config_response");
    lock_guard<mutex>locker(m_mutex);
    do{
        CJsonObject config;
        if(!object.Get("config",config)){
            json_response.Add("error","no config entry!");
            break;
        }
        string entry_name;
        bool have_error=false;
        while(config.GetKey(entry_name))
        {
            CJsonObject tmp;
            if(!config.Get(entry_name,tmp)||!m_json_config->Replace(entry_name,tmp))
            {
                string error_info=entry_name;
                error_info+=" entry not find,terminated!";
                json_response.Add("error",error_info);
                have_error=true;
                break;
            }
        }
        if(!have_error)json_response.Add("info","success");
    }while(0);
    m_json_config->SaveToFile();
    response.Add("response",json_response);
    auto str_response=response.ToString();
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
}
void proxy_server_mode::handle_get_url_info(const string&from)
{
    CJsonObject response;
    CJsonObject json_response;
    response.Add("cmd","get_url_info_response");
    CJsonObject url_info;
    for(auto i:m_url_set)
    {
        url_info.Add(i);
    }
    if(url_info.IsEmpty())
    {
        json_response.AddEmptySubArray("url_info");
    }
    else {
        json_response.Add("url_info",url_info);
    }
    response.Add("response",json_response);
    auto str_response=response.ToString();
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
}
void proxy_server_mode::handle_reload_mode(const string&from)
{
    {
        CJsonObject response;
        CJsonObject json_response;
        response.Add("cmd","reload_mode_response");
        json_response.Add("info","success");
        response.Add("response",json_response);
        auto str_response=response.ToString();
        lock_guard<mutex>locker(m_mutex);
        m_message_fd->send(str_response.c_str(),str_response.length(),from);
    }
    m_reload_flag.exchange(true);
    stop();
}
void proxy_server_mode::handle_get_net_config(const string&from)
{
    CJsonObject response;
    CJsonObject json_response;
    response.Add("cmd","get_net_config_response");
    lock_guard<mutex>locker(m_mutex);
    auto net_info_vec=NETWORK.get_net_interface_info();
    CJsonObject net_info_list;
    for(auto i:net_info_vec)
    {
        CJsonObject tmp;
        tmp.Add("dev_name",i.dev_name);
        tmp.Add("ip",i.ip);
        tmp.Add("mac",i.mac);
        tmp.Add("netmask",i.netmask);
        tmp.Add("gateway_ip",i.gateway_ip);
        net_info_list.Add(tmp);
    }
    if(net_info_list.IsEmpty()){
        json_response.AddEmptySubArray("net_info_list");
    }
    else{
        json_response.Add("net_info_list",net_info_list);
    }
    response.Add("response",json_response);
    MICAGENT_DEBUG("send %s\r\n",response.ToFormattedString().c_str());
    auto str_response=response.ToString();
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
}
void proxy_server_mode::handle_update_net_info(const CJsonObject&object,const string&from)
{
    CJsonObject response;
    CJsonObject json_response;
    response.Add("cmd","update_net_info_response");
    lock_guard<mutex>locker(m_mutex);
    bool is_success=false;
    do{
        CJsonObject net_config;
        if(!object.Get("net_config",net_config)){
            json_response.Add("error","no net_config entry!");
            break;
        }
        string dev_name;
        if(!net_config.Get("dev_name",dev_name)){
            break;
        }
        string ip;
        if(!net_config.Get("ip",ip)){
            break;
        }
        string mac;
        if(!net_config.Get("mac",mac)){
            break;
        }
        string netmask;
        if(!net_config.Get("netmask",netmask)){
            break;
        }
        string gateway_ip;
        if(!net_config.Get("gateway_ip",gateway_ip)){
            break;
        }
        Network_Util::net_interface_info new_net_info(dev_name,ip,mac,netmask,gateway_ip);
        is_success=NETWORK.modify_net_interface_info(new_net_info);
        if(is_success){
            local_ip_change(dev_name,ip,netmask,gateway_ip,mac);
        }
    }while(0);
    json_response.Add("net_set_state",is_success,is_success);
    response.Add("response",json_response);
    auto str_response=response.ToString();
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
}
void proxy_server_mode::handle_restart(const string&from)
{
    CJsonObject response;
    response.Add("cmd","restart_response");
    auto str_response=response.ToString();
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
    m_event_loop->addTimer([this](){
        system_reboot();
        return false;
    },20);
}
void proxy_server_mode::handle_device_search(const CJsonObject&object,const string&from)
{
    uint32_t search_port;
    string res;
    if(object.Get("search_port",search_port)){
        res=do_handle_device_search(search_port&0xffff);
    }
    else {
        res=do_handle_device_search();
    }
    m_message_fd->send(res.c_str(),res.length(),from);
}
void proxy_server_mode::handle_cmd_unsupported(const string&cmd,const string&from)
{
    CJsonObject response;
    CJsonObject json_response;
    response.Add("cmd",cmd+"_response");
    json_response.Add("error","not supported!");
    response.Add("response",json_response);
    auto str_response=response.ToString();
    lock_guard<mutex>locker(m_mutex);
    m_message_fd->send(str_response.c_str(),str_response.length(),from);
}
void proxy_server_mode::system_reboot()
{
#if MICAGENT_CROSS
    system("killall -9 system_dog");
#endif
    system("reboot");
}
void proxy_server_mode::local_ip_change(const string&dev,const string&ip,const string&mask,const string&gateway_ip,const string&mac)const
{
#if MICAGENT_CROSS
    if(dev=="eth0")
    {
        string cmd_base="fw_setenv ";
        if(!ip.empty())
        {//set ip
            auto cmd=cmd_base+" ipaddr "+ip;
            system(cmd.c_str());
        }
        if(!mask.empty())
        {
            auto cmd=cmd_base+" netmask "+mask;
            system(cmd.c_str());
        }
        if(!gateway_ip.empty())
        {
            auto cmd=cmd_base+" gatewayip "+gateway_ip;
            system(cmd.c_str());
        }
        if(!mac.empty())
        {
            auto cmd=cmd_base+" ethaddr "+mac;
            system(cmd.c_str());
        }
    }
#endif
}
