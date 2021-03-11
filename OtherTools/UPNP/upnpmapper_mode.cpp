#include "upnpmapper_mode.h"
#include "c_log.h"
using namespace micagent;
using namespace std;
void upnp_helper::config(shared_ptr<EventLoop> loop, bool set_net, string lgd_ip)
{
    if(m_is_config||!loop)return;
    m_is_config.exchange(true);
    m_loop=loop;
    m_set_net=set_net;
    //m_lgd_ip=lgd_ip;
    auto net_info=NETWORK.get_default_net_interface_info();
    m_last_internal_ip=net_info.ip;
    m_dev_name=net_info.dev_name;
    m_lgd_ip=net_info.gateway_ip;
    UpnpMapper::Instance().Init(loop,m_lgd_ip);

    //start timer check task
    m_check_timer_id=loop->addTimer([this](){
        lock_guard<mutex>locker(m_mutex);
        if(m_check_counts%2==0)
        {
            check_internal_ip();
            check_external_ip();
        }
        if(m_check_counts%5==0)check_port_task();
        m_check_counts++;
        return true;
    },CHECK_INTERVAL);
}
void upnp_helper::add_port_task(SOCKET_TYPE type,uint16_t internal_port,uint16_t external_port,string description)
{
    if(!m_is_config)return;
    lock_guard<mutex>locker(m_mutex);
    string key=type==UDP?"udp":"tcp";
    key+=to_string(external_port);
    auto iter=m_upnp_task_map.find(key);
    if(iter==m_upnp_task_map.end())
    {
        UpnpMapper::Instance().Api_deleteportMapper(type,external_port);
        m_upnp_task_map.emplace(key,upnp_task(type,internal_port,external_port,description));
    }

}
void upnp_helper::delete_port_task(SOCKET_TYPE type,uint16_t external_port,string /*description*/)
{
    if(!m_is_config)return;
    lock_guard<mutex>locker(m_mutex);
    string key=type==UDP?"udp":"tcp";
    key+=to_string(external_port);
    auto iter=m_upnp_task_map.find(key);
    if(iter!=m_upnp_task_map.end())
    {
        m_upnp_task_map.erase(iter);
        UpnpMapper::Instance().Api_deleteportMapper(type,external_port);
    }
}
void upnp_helper::add_external_ip_to_dev(string ip)
{
    if(!m_set_net||ip.empty())return;
    if(ip!=m_last_external_ip)
    {
        MICAGENT_LOG(LOG_WARNNING,"external_ip %s is  changed to %s!\r\n",m_last_external_ip.c_str(),ip.c_str());
        if(!m_last_external_ip.empty())
        {
            std::string delete_command=" ip addr delete ";
            delete_command=delete_command+m_last_external_ip+"/32 dev "+m_dev_name;
            system(delete_command.c_str());
        }
        m_last_external_ip=ip;
        std::string add_command=" ip addr add ";
        add_command=add_command+m_last_external_ip+"/32 dev "+m_dev_name;
        system(add_command.c_str());
        if(m_external_callback)m_external_callback(m_last_external_ip);
    }
    else {
        //MICAGENT_LOG(LOG_INFO,"external_ip %s is not changed!\r\n",m_last_external_ip.c_str());
    }
}
void upnp_helper::check_internal_ip()
{
    auto net_info=NETWORK.get_net_interface_info(true);
    if(!net_info.empty()){
        for(auto i:net_info){
            if(i.is_default){
                string internal_ip=i.ip;
                string dev_name=i.dev_name;
                if(internal_ip!=m_last_internal_ip||m_dev_name!=dev_name)
                {
                    MICAGENT_LOG(LOG_WARNNING,"internal_ip %s is  changed to %s!\r\n",m_last_internal_ip.c_str(),internal_ip.c_str());
                    m_dev_name=dev_name;
                    m_last_internal_ip=internal_ip;
                    if(m_internal_callback)m_internal_callback(internal_ip);
                }
                else {
                    // MICAGENT_LOG(LOG_INFO,"internal_ip %s is not changed!\r\n",m_last_internal_ip.c_str());
                }
                break;
            }
        }

    }
}
void upnp_helper::check_external_ip()
{
    string external_ip=UpnpMapper::Instance().APi_getexternalIP();
    //add_external_ip_to_dev(external_ip);
    UpnpMapper::Instance().Api_GetNewexternalIP();
}
void upnp_helper::check_port_task()
{
    for(auto i: m_upnp_task_map)
    {
        auto task =i.second;
        UpnpMapper::Instance().Api_GetSpecificPortMappingEntry(task.type,task.external_port,[task,this](bool flag){
            if(!flag)
            {
                lock_guard<mutex>locker(m_mutex);
                string key=task.type==UDP?"udp":"tcp";
                key+=to_string(task.external_port);
                auto iter=m_upnp_task_map.find(key);
                if(iter!=m_upnp_task_map.end())
                {
                    if(iter->second.error_cnts>5)
                    {
                        iter->second.error_cnts=0;
                        UpnpMapper::Instance().Api_addportMapper(task.type,m_last_internal_ip,task.internal_port,task.external_port,task.description);
                    }
                    else {
                        iter->second.error_cnts++;
                        UpnpMapper::Instance().Api_deleteportMapper(task.type,task.external_port);
                    }
                }
            }
            else {
                if(task.type==UDP)MICAGENT_LOG(LOG_INFO,"UDP %hu port mapping is working!\r\n",task.external_port);
                else {
                    MICAGENT_LOG(LOG_INFO,"TCP %hu port mapping is working!\r\n",task.external_port);
                }
            }
        });
    }
}
