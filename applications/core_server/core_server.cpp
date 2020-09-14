#include "core_server.h"
#include "daemon_instance.h"
using namespace micagent;
core_server::core_server():m_dns_port(DEFAULT_DNS_PORT),m_dns_cache_time_ms(DEFAULT_DNS_CACHE_TIME),m_load_banlance_port(DEFAULT_LOAD_BANLANCE_PORT),m_load_banlance_cache_time_ms(DEFAULT_LOAD_BANLANCE_CACHE_TIME)
{
    m_is_running.exchange(false);
}
void core_server::init(const string &json_config_path)
{
    lock_guard<mutex>locker(m_mutex);
    do{
        m_config_path=json_config_path;
        auto pos=m_config_path.find_last_of('/');
        if(pos!=string::npos){
            m_config_path.substr(pos+1);
        }
        m_config_path=daemon_instance::get_pwd_path()+m_config_path;
        shared_ptr<CJsonObject>object(CJsonObject::CreateInstance(json_config_path));
        uint32_t dns_port;
        if(!object->Get("dns_port",dns_port)){
            MICAGENT_ERROR("no dns_port entry!");
            break;
        }
        m_dns_port=dns_port&0xffff;
        uint32_t load_banlance_port;
        if(!object->Get("load_banlance_port",load_banlance_port)){
            MICAGENT_ERROR("no load_banlance_port entry!");
            break;
        }
        m_load_banlance_port=load_banlance_port&0xffff;
        if(!object->Get("dns_cache_time_ms",m_dns_cache_time_ms)){
            MICAGENT_ERROR("no dns_cache_time_ms entry!");
            break;
        }
        if(!object->Get("load_banlance_cache_time_ms",m_load_banlance_cache_time_ms)){
            MICAGENT_ERROR("no load_banlance_cache_time_ms entry!");
            break;
        }
        MICAGENT_INFO("init success!");
        return ;
    }while(0);
    load_default_config(json_config_path);
}
void core_server::generate_daemon_config(const string &pro_name, const string &daemon_config_path)
{
    MICAGENT_INFO("pro_name:%s == daemon_config_path :%s",pro_name.c_str(),daemon_config_path.c_str());
    CJsonObject config;
    CJsonObject entry_pro;
    entry_pro.Add("pro_name",pro_name);
    entry_pro.Add("cmd_options",m_config_path);
    entry_pro.Add("wait_time",0);
    entry_pro.Add("path","./");
    config.Add(entry_pro);
 //dns_port check
    CJsonObject entry_dns;
    entry_dns.Add("pro_name",PORT_CHECK_SHELL);
    string port_check_options=to_string(m_dns_port)+" "+pro_name+" "+"udp";
    entry_dns.Add("cmd_options",port_check_options);
    entry_dns.Add("wait_time",5);
    entry_dns.Add("path","./");
    config.Add(entry_dns);
 //load_banlance check
    CJsonObject entry_load_banlance;
    entry_load_banlance.Add("pro_name",PORT_CHECK_SHELL);
    string entry_load_banlance_port_check_options=to_string(m_load_banlance_port)+" "+pro_name+" "+"udp";
    entry_load_banlance.Add("cmd_options",entry_load_banlance_port_check_options);
    entry_load_banlance.Add("wait_time",5);
    entry_load_banlance.Add("path","./");
    config.Add(entry_load_banlance);
    daemon_instance::modify_run_config(daemon_config_path,config);
}
void core_server::start()
{

    if(!m_is_running){
        lock_guard<mutex>locker(m_mutex);
        if(!m_event_loop)
        {
            m_event_loop.reset(new EventLoop(0,1));
        }
        if(!m_dns_server)
        {
            m_dns_server.reset(new dns_server(m_dns_port,m_dns_cache_time_ms));
            m_dns_server->config(m_event_loop);
        }
        m_dns_server->start_work();
        if(!m_banlance_server)
        {
            m_banlance_server.reset(new load_balance_server(m_load_banlance_port,m_load_banlance_cache_time_ms));
            m_banlance_server->config(m_event_loop);
        }
        m_banlance_server->start_work();
    }
    if(!m_is_running)
    {
        m_is_running.exchange(true);
        loop();
    }
}
void core_server::stop()
{
    if(m_is_running)
    {
        lock_guard<mutex>locker(m_mutex);
        m_dns_server.reset();
        m_banlance_server.reset();
        if(m_event_loop)
        {
            m_event_loop->stop();
            m_event_loop.reset();
        }
        m_is_running.exchange(false);
        {
            unique_lock<mutex>locker(m_exit_mutex);
            m_exit_conn.notify_all();
        }
    }
}
void core_server::loop()
{
    MICAGENT_INFO("loop start!");
    unique_lock<mutex>locker(m_exit_mutex);
    while(m_is_running)
    {
        m_exit_conn.wait(locker);
    }
    MICAGENT_INFO("loop exit!");
}
core_server::~core_server()
{
    stop();
}
void core_server::load_default_config(const string &json_config_path)
{
    MICAGENT_WARNNING("load default config!");
    CJsonObject object;
    m_dns_port=DEFAULT_DNS_PORT;
    m_load_banlance_port=DEFAULT_LOAD_BANLANCE_PORT;
    m_dns_cache_time_ms=DEFAULT_DNS_CACHE_TIME;
    m_load_banlance_cache_time_ms=DEFAULT_LOAD_BANLANCE_CACHE_TIME;
    object.Add("dns_port",m_dns_port);
    object.Add("dns_cache_time_ms",m_dns_cache_time_ms);
    object.Add("load_banlance_port",m_load_banlance_port);
    object.Add("load_banlance_cache_time_ms",m_load_banlance_cache_time_ms);
    object.SetSavePath(json_config_path);
    object.SaveToFile();
}
