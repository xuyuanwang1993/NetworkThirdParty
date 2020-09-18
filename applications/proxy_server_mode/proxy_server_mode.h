#ifndef PROXY_SERVER_MODE_H
#define PROXY_SERVER_MODE_H
#include "c_log.h"
#include "event_loop.h"
#include "load_balance_client.h"
#include "dns_client.h"
#include "rtsp_server.h"
#include "proxy_server.h"
#include "upnpmapper_mode.h"
#include "unix_socket_helper.h"
#define UNIX_SOCKET_PREFIX "/tmp/"
namespace micagent {
using namespace std;
using neb::CJsonObject;
class proxy_server_mode{
//event_loop
    static constexpr int32_t DEFAULT_THREAD_POOL_SIZE=2;
    static constexpr uint32_t DEFAULT_TRIGGER_THREADS=2;
    static constexpr uint32_t DEFAULT_TRIGGER_QUEUE_SIZE=2000;
    static constexpr uint32_t DEFAULT_NETWORK_IO_THREADS=4;
//dns
    static constexpr int64_t DEFAULT_DNS_UPLOAD_INTERVAL_MS=60*1000;//60s
    static constexpr char   DEFAULT_DNS_SERVER_DOMAIN[]="www.meanning.com";
    static constexpr uint16_t DEFAULT_DNS_PORT=10000;
//load_balance
    static constexpr int64_t DEFAULT_BALANCE_UPLOAD_INTERVAL_MS=2*1000;//2s
    static constexpr uint16_t DEFAULT_BALANCE_PORT=10001;
    static constexpr uint32_t DEFAULT_MAX_PAYLOAD_SIZE=25;
    static constexpr double DEFAULT_SERVER_WEIGHT=0.5;
//rtsp_server
    static constexpr uint16_t DEFAULT_RTSP_SERVER_PORT=8554;
//rtsp_proxy
    static constexpr uint16_t DEFAULT_RTSP_PROXY_PORT=8555;
public:
    proxy_server_mode();
    void init(const string &json_config_path,const string&pro_name="proxy_server_mode");
    void generate_daemon_config(const string &pro_name,const string &daemon_config_path);
    void start();
    void stop();
    ~proxy_server_mode();
private:
    void loop();
    void load_default_config(const string &json_config_path);
    void message_handle();
    void handle_get_config(const string&from);
    void handle_save_config(const string&from);
    void handle_update_config(const CJsonObject&object,const string&from);
    void handle_get_url_info(const string&from);
    void handle_reload_mode(const string&from);
    void handle_cmd_unsupported(const string&cmd,const string&from);
private:
//event_loop
    int32_t m_event_thread_pool_size;
    uint32_t m_event_trigger_threads;
    uint32_t m_event_trigger_queue_size;
    uint32_t m_event_network_io_threads;
    shared_ptr<EventLoop> m_event_loop;
//dns
    int64_t m_dns_upload_interval_ms;
    string m_dns_server_domain;
    uint16_t m_dns_server_port;
    string m_dns_domain_name;
    string m_dns_account_name;
    string m_dns_pass_word;
    string m_dns_description;
    shared_ptr<dns_client>m_dns_client;
//load_balance
    int64_t m_balance_upload_interval_ms;
    string m_balance_server_domain;
    uint16_t m_balance_server_port;
    uint32_t m_balance_max_payload_size;
    double m_balance_server_weight;
    shared_ptr<load_balance_client>m_load_balance_client;
//rtsp_server
    uint16_t m_rtsp_server_port;
    string m_rtsp_account_name;
    string m_rtsp_account_password;
    shared_ptr<rtsp_server>m_rtsp_server;
//rtsp_proxy
    uint16_t m_rtsp_proxy_port;
    shared_ptr<proxy_server>m_proxy_server;
//upnp
    bool m_set_external_ip;
    string m_router_ip;
    uint16_t m_rtsp_server_external_port;
    uint16_t m_rtsp_proxy_external_port;
//itself
    bool m_log_open;
    string m_log_path;
private:
    mutex m_mutex;
    mutex m_exit_mutex;
    condition_variable m_exit_conn;
    shared_ptr<unix_dgram_socket>m_message_fd;
    ChannelPtr m_message_channel;
    atomic_bool m_is_running;
    atomic_bool m_reload_flag;
    string m_config_path;
    set<string>m_url_set;
    string m_pro_name;
    shared_ptr<neb::CJsonObject>m_json_config;
};
}
#endif // PROXY_SERVER_MODE_H
