#ifndef UPNPMAPPER_MODE_H
#define UPNPMAPPER_MODE_H
#include "upnpmapper.h"
#include <map>
namespace micagent {
using IpChangeCallback=function<void(string ip)>;
class upnp_helper{
    static constexpr uint32_t CHECK_INTERVAL=10*1000;//10s
    struct upnp_task{
        SOCKET_TYPE type;
        uint16_t internal_port;
        uint16_t external_port;
        string description;
        upnp_task(SOCKET_TYPE _type,uint16_t _internal_port,uint16_t _external_port,string _description):\
        type(_type),internal_port(_internal_port),external_port(_external_port),description(_description){

        }
    };
public:
    /**
     * @brief Instance single instance
     * @return
     */
    static upnp_helper &Instance(){static upnp_helper helper;return helper;}
    /**
     * @brief config init the upnp_helper
     * @param loop event loop
     * @param set_net whether to add external ip to the default interface
     * @param lgd_ip the external router's ip
     */
    void config(EventLoop *loop,bool set_net,string lgd_ip);
    /**
     * @brief add_port_task add a continued port map task
     * @param type TCP/UDP
     * @param internal_port
     * @param external_port
     * @param description
     */
    void add_port_task(SOCKET_TYPE type,uint16_t internal_port,uint16_t external_port,string description);
    /**
     * @brief delete_port_task delete a continued port map task
     * @param type TCP/UDP
     * @param external_port
     */
    void delete_port_task(SOCKET_TYPE type, uint16_t external_port, string);
    /**
     * @brief set_internal_callback set the callback when the internal ip is changed
     * @param cb
     */
    void set_internal_callback(const IpChangeCallback &cb){
        lock_guard<mutex>locker(m_mutex);
        m_internal_callback=cb;
    }
    /**
     * @brief set_external_callback set the callback when the external ip is changed
     * @param cb
     */
    void set_external_callback(const IpChangeCallback &cb){
        lock_guard<mutex>locker(m_mutex);
        m_external_callback=cb;
    }
private:
    ~upnp_helper(){
        if(m_loop&&m_check_timer_id)m_loop->blockRemoveTimer(m_check_timer_id);
    }
    upnp_helper():m_is_config(false),m_set_net(false),m_loop(nullptr),m_check_timer_id(INVALID_TIMER_ID),m_check_counts(0),m_lgd_ip(""),m_internal_callback(nullptr),m_external_callback(nullptr),\
    m_last_external_ip(""),m_last_internal_ip("")
    {
    }
    void add_external_ip_to_dev(std::string ip);
    void check_internal_ip();
    void check_external_ip();
    void check_port_task();
private:
    atomic_bool m_is_config;
    bool m_set_net;
    EventLoop *m_loop;
    TimerId m_check_timer_id;
    uint32_t m_check_counts;
    mutex m_mutex;
    string m_lgd_ip;
    string m_dev_name;
    //callback
    IpChangeCallback m_internal_callback;
    IpChangeCallback m_external_callback;
    //check
    string m_last_external_ip;
    string m_last_internal_ip;
    map<string,upnp_task> m_upnp_task_map;

};
}
#endif // UPNPMAPPER_MODE_H
