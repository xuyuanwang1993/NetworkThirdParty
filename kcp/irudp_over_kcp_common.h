#ifndef IRUDP_OVER_KCP_COMMON_H
#define IRUDP_OVER_KCP_COMMON_H
#include "ikcp.h"
#include "network_util.h"
#include "event_loop.h"
namespace micagent {
enum IRUDP_PACKET_TYPE_e:uint8_t{
    RUDP_HANDSHAKE_CLIENT,//conv_id=0
    RUDP_HANDSHAKE_SERVER,//conv_id=0
    RUDP_RST,
    RUDP_KEEP_ALIVE,//send by client
    RUDP_PAYLOAD,
};
struct Irudp_header_s{
    IRUDP_PACKET_TYPE_e type;
    uint32_t src_key;//generate by the client
};

struct Irudp_kcp_param_s{
    uint32_t window_size:16;
    uint32_t interval:14;
    uint32_t nodelay:1;
    uint32_t nc:1;
    uint8_t resend;
};
enum IRUDP_WORK_MODE_e:uint8_t{
    IRUDP_CLIENT,
    IRUDP_SERVER,
    IRUDP_CONNECTION,
};
enum IRUDP_INT_DEFINES:uint32_t{
    IRUDP_SERVER_CONNECTION_TIMEOUT_CNTS=3,
    IRUDP_SERVER_CONNECTION_TIME_OUT_TIME_MS=20000,//20s
    IRUDP_SERVER_MAX_CACHE_PACKET_NUM=100000,
};

struct Irudp_udp_packet_cache_s{
    shared_ptr<uint8_t>data;
    const uint32_t data_len;
    const sockaddr_in addr;
    Irudp_udp_packet_cache_s(const void *input_data,uint32_t input_len,const sockaddr_in &input_addr):data_len(input_len),addr(input_addr){
        if(data_len>0)
        {
            data.reset(new uint8_t[data_len+1],default_delete<uint8_t[]>());
            memset(data.get(),0,data_len);
            memcpy(data.get(),input_data,data_len);
        }
    }
};

using IRUDP_CONNECTION_CALLBACK=function<void(uint32_t )>;
class irudp_over_kcp_connection_base;
class irudp_over_kcp_server;
class irudp_over_kcp_client;
using IRUDP_SEND_CALLBACK=function<bool (const void *buf,uint32_t buf_len,shared_ptr<irudp_over_kcp_connection_base>)>;
class irudp_over_kcp_connection_base:public enable_shared_from_this<irudp_over_kcp_connection_base>{
protected:
    irudp_over_kcp_connection_base(uint32_t conv_id,uint32_t src_key,const Irudp_kcp_param_s&param);
    virtual ~irudp_over_kcp_connection_base();
    virtual bool handle_payload(const void *buf,uint32_t buf_len,const sockaddr_in &addr)=0;
    virtual bool send_packet(IRUDP_PACKET_TYPE_e type,const void *payload,uint32_t payload_len)=0;
protected:
        const uint32_t m_conv_id;
        const uint32_t m_src_key;
        IKCPCB *m_kcp;
};
}
#endif // IRUDP_OVER_KCP_COMMON_H
