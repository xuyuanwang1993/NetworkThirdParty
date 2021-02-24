#ifndef KCP_PROXY_CONNECTION_H
#define KCP_PROXY_CONNECTION_H
#include "kcp_common.h"
namespace micagent {
using namespace std;
class kcp_proxy_server;
class kcp_proxy_connection:public enable_shared_from_this<kcp_proxy_connection>{
    friend class kcp_proxy_server;
public:
    void register_connection_status_callback(const TriggerEvent &cb){
        m_connection_status_cb=cb;
    }
    IKCP_CONNECTION_STATUS get_connection_status()const {return m_connection_info.connection_status;}
    kcp_proxy_connection(const ikcp_config_s & config,const sockaddr &addr);
     ~kcp_proxy_connection();
protected:
    kcp_proxy_connection(uint32_t conv_id);
    void attach_server(shared_ptr<kcp_proxy_server>server);
    void handle_proxy_server_data(const ikcp_proxy_header_s &header,const ikcp_raw_udp_packet_s&data);
    bool send_syn();
    bool send_syn_ack();
    bool send_finished();
    bool send_rst();
    bool send_keep_alive();
    bool send_kcp_payload(const void *payload,uint32_t kcp_payload_len);
    void handle_syn(const ikcp_raw_udp_packet_s&data);
    void handle_syn_ack(const ikcp_raw_udp_packet_s&data);
    void handle_finished(const ikcp_raw_udp_packet_s&data);
    void handle_rst(const ikcp_raw_udp_packet_s&data);
    void handle_keep_alive(const ikcp_raw_udp_packet_s&data);
     void handle_kcp_payload(const ikcp_raw_udp_packet_s&data);
protected:
    ikcp_connection_info m_connection_info;
    weak_ptr<kcp_proxy_server> m_kcp_proxy_server;
    TriggerEvent m_connection_status_cb;
};
}
#endif // KCP_PROXY_CONNECTION_H
