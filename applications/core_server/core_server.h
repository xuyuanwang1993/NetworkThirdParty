#ifndef CORE_SERVER_H
#define CORE_SERVER_H
#include "load_balance_server.h"
#include "dns_server.h"
#include "ConfigUtils.h"
#include "c_log.h"
#include <iostream>
namespace micagent {
using neb::CJsonObject;
/**
 * @brief The core_server class include a dns_server and a load_balance_server
 * init with a json_config file's path
 */
class core_server{
    static constexpr uint16_t DEFAULT_DNS_PORT=10000;
    static constexpr uint16_t DEFAULT_LOAD_BANLANCE_PORT=10001;
    static constexpr int64_t DEFAULT_DNS_CACHE_TIME=5*60*1000;//5min
    static constexpr int64_t DEFAULT_LOAD_BANLANCE_CACHE_TIME=30*1000;//30s
public:
    core_server();
    void init(const string &json_config_path);
    void generate_daemon_config(const string &pro_name,const string &daemon_config_path);
    void start();
    void stop();
    ~core_server();
private:
    void loop();
    void load_default_config(const string &json_config_path);
private:
    mutex m_mutex;
    mutex m_exit_mutex;
    condition_variable m_exit_conn;
    shared_ptr<EventLoop>m_event_loop;
    shared_ptr<load_balance_server>m_banlance_server;
    shared_ptr<dns_server>m_dns_server;
    uint16_t m_dns_port;
    int64_t m_dns_cache_time_ms;
    uint16_t m_load_banlance_port;
    int64_t m_load_banlance_cache_time_ms;
    atomic_bool m_is_running;
    string m_config_path;
};
}
#endif // CORE_SERVER_H
