#ifndef KCP_PROXY_SERVER_H
#define KCP_PROXY_SERVER_H
#include "kcp_common.h"
namespace micagent {
class kcp_proxy_connection;
class kcp_proxy_interface;
class kcp_proxy_server:public enable_shared_from_this<kcp_proxy_server>{
    friend class kcp_proxy_connection;
public:
    kcp_proxy_server();
    void init_proxy_server(uint16_t local_port,sockaddr_in *public_server_addr=nullptr);
    void init_public_server();
    virtual ~kcp_proxy_server();
    bool add_connection(shared_ptr<kcp_proxy_connection>connection);
    bool remove_connection(weak_ptr<kcp_proxy_connection>connection);
    bool send_application_data(weak_ptr<kcp_proxy_connection>connection,const void *data,uint32_t data_len);
protected:
    bool accept_connection(shared_ptr<kcp_proxy_connection>connection);
    void handle_read(const ikcp_raw_udp_packet_s&data);
    void check_all_connections();
    void response_error_info(const ikcp_proxy_header_s &header,const ikcp_raw_udp_packet_s&data,const string error_info);
protected:
    mutex m_mutex;
    TimerId m_check_timer_id;
    shared_ptr<kcp_proxy_interface>m_w_proxy_interface;
    map<uint32_t,shared_ptr<kcp_proxy_connection>>m_proxy_connections;
};
}
#endif // KCP_PROXY_SERVER_H
