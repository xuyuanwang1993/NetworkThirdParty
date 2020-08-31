#ifndef UPNPMAPPER_H
#define UPNPMAPPER_H
#include <string>
#include <functional>
#include<atomic>
#include <map>
#include <vector>
#include <mutex>
#include"event_loop.h"
#define UPNP_LOG_ON  0
namespace micagent {
using namespace std;
using UPNPCALLBACK=std::function<void(bool)>;
class UpnpMapper;
typedef enum{
    UPNP_GETCONTROLURL,
    UPNP_GETEXTERNALIPADDRESS,
    UPNP_ADDPORTMAPPING,
    UPNP_GETSPECIFICPORTMAPPINGENTRY,
    UPNP_DELETEPORTMAPPING,
}UPNP_COMMAND;
class Upnp_Connection{
    friend class UpnpMapper;
public:
    Upnp_Connection()=delete;
    Upnp_Connection(UpnpMapper * server,EventLoop *loop, int sockfd,UPNP_COMMAND mode);
    ~Upnp_Connection();
    void start_work();
    void stop_work();
    void setUPNPCallback(const UPNPCALLBACK& cb)
    {m_callback=cb;}
    void reset_mode(UPNP_COMMAND mode){m_buf.clear();m_mode=mode;};
private:
    ChannelPtr m_channel;
    UpnpMapper * m_upnp_mapper=nullptr;
    EventLoop *m_event_loop = nullptr;
    UPNPCALLBACK m_callback=nullptr;
    UPNP_COMMAND m_mode;
    string m_buf;
private:
    string build_xml_packet(string action,std::vector<std::pair<string ,string>>args);
    bool onRead();//数据处理
    void HandleData();
    bool check_packet();
    void send_get_wanip();
    void send_add_port_mapper(SOCKET_TYPE type,string internal_ip,int internal_port,int external_port,string description,int alive_time=0);
    void send_get_specific_port_mapping_entry(SOCKET_TYPE type,int external_port);
    void send_delete_port_mapper(SOCKET_TYPE type,int external_port);
    void send_get_control_url();
    void handle_get_wanip();
    void handle_add_port_mapper();
    void handle_get_specific_port_mapping_entry();
    void handle_delete_port_mapper();
    void handle_get_control_url();
};
class UpnpMapper{
    friend class Upnp_Connection;
    const int MAX_WAIT_TIME=5000;//5s
public:
    /**
     * @brief Instance single instance
     * @return
     */
    static UpnpMapper &Instance(){static UpnpMapper instance;return instance; }
    /**
     * @brief Init init the UpnpMapper
     * @param event_loop
     * @param lgd_ip the external router'ip
     */
    void Init(EventLoop *event_loop,string lgd_ip);
    /**
     * @brief Api_addportMapper add a port mapper
     * @param type TCP/UDP
     * @param internal_ip
     * @param internal_port
     * @param external_port
     * @param description
     * @param callback if add success the callback will catch a  true param input
     * @param alive_time the time it will be kept and set to 0 for forever
     */
    void Api_addportMapper(SOCKET_TYPE type,string internal_ip,int internal_port,int external_port,string description,UPNPCALLBACK callback=nullptr,int alive_time=0);
    /**
     * @brief Api_GetSpecificPortMappingEntry
     * @param type TCP/UDP
     * @param external_port
     * @param callback if port map existed,the callback will catch a  true param input
     */
    void Api_GetSpecificPortMappingEntry(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback=nullptr);
    /**
     * @brief Api_deleteportMapper delete a port map on external_port
     * @param type TCP/UDP
     * @param external_port
     * @param callback  if port map is deleted,the callback will catch a  true param input
     */
    void Api_deleteportMapper(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback=nullptr);
    /**
     * @brief Api_GetNewexternalIP get the new external ip
     * @param callback
     */
    void Api_GetNewexternalIP(UPNPCALLBACK callback=nullptr);
    /**
     * @brief APi_getexternalIP get the external ip cache
     * @return
     */
    string APi_getexternalIP(){return m_wan_ip;}
    bool Api_discoverOk(){return m_init_ok;}
protected:
    std::shared_ptr<Upnp_Connection> newConnection(SOCKET sockfd,UPNP_COMMAND mode);
    void addConnection(SOCKET sockfd, std::shared_ptr<Upnp_Connection> Conn);
    void removeConnection(SOCKET sockfd);
    void addTimeoutEvent(SOCKET sockfd);
private:
    void send_discover_packet();
    void get_control_url();
    void get_wanip(UPNPCALLBACK callback=nullptr);
    void add_port_mapper(SOCKET_TYPE type,string internal_ip,int internal_port,int external_port,string description,UPNPCALLBACK callback=nullptr,int alive_time=0);
    void get_specific_port_mapping_entry(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback=nullptr);
    void delete_port_mapper(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback=nullptr);
    std::shared_ptr<Channel>m_udp_channel;
    EventLoop *m_event_loop;
    string m_lgd_ip;
    int m_lgd_port;
    string m_location_src;
    string m_wan_ip;
    string m_control_string;
    string m_control_url;
    atomic<bool> m_init_ok;
    std::mutex m_map_mutex;
    std::map<SOCKET, std::shared_ptr<Upnp_Connection>> m_connections;
    UpnpMapper(){
        Network_Util::Instance().get_net_interface_info(true);
    }
    ~UpnpMapper(){
        if(m_udp_channel&&m_event_loop)m_event_loop->removeChannel(m_udp_channel);
    }
};
}
#endif // UPNPMAPPER_H
