#include "upnpmapper_mode.h"
using namespace micagent;
using namespace std;
void upnp_helper::config(EventLoop *loop,bool set_net,string lgd_ip)
{
    if(m_is_config)return;
    m_is_config.exchange(true);
    m_loop=loop;
    m_set_net=set_net;
    m_lgd_ip=lgd_ip;
    auto net_info=NETWORK.get_net_interface_info(true);
    if(!net_info.empty())m_last_internal_ip=net_info[0].ip;
}
void upnp_helper::add_port_task(SOCKET_TYPE type,uint16_t internal_port,uint16_t external_port,string description)
{

}
void upnp_helper::delete_port_task(SOCKET_TYPE type,uint16_t external_port,string description)
{

}
