#ifndef IRUDP_OVER_KCP_SERVER_H
#define IRUDP_OVER_KCP_SERVER_H
#include "irudp_over_kcp_common.h"
namespace micagent {
class irudp_over_kcp_server;
class irudp_over_kcp_server_connection:public irudp_over_kcp_connection_base{
    friend class irudp_over_kcp_server;
protected:
    irudp_over_kcp_server_connection(shared_ptr<irudp_over_kcp_server>server,uint32_t conv_id,uint32_t src_key,const Irudp_kcp_param_s&param);
    ~irudp_over_kcp_server_connection()override{}
     bool handle_payload(const void *buf,uint32_t buf_len,const sockaddr_in &addr)override;
     bool send_packet(IRUDP_PACKET_TYPE_e type,const void *payload,uint32_t payload_len)override;
protected:
     weak_ptr<irudp_over_kcp_server> m_irudp_server;
     int64_t m_last_alive_time;
};
class irudp_over_kcp_server:public enable_shared_from_this<irudp_over_kcp_server>{
public:
    irudp_over_kcp_server(shared_ptr<EventLoop>loop,uint16_t port);
protected:
protected:
//
    weak_ptr<EventLoop>m_event_loop;
    TimerId m_check_timer;
//send cache
    mutex m_send_mutex;
    queue<Irudp_udp_packet_cache_s>m_send_cache;
//
    mutex m_mutex;
    map<uint32_t,shared_ptr<irudp_over_kcp_server_connection>>m_connections;
};
}
#endif // IRUDP_OVER_KCP_SERVER_H
