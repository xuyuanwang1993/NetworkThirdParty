#ifndef KCP_COMMON_H
#define KCP_COMMON_H
#include "ikcp.h"
#include "network_util.h"
#include "event_loop.h"
namespace micagent {
enum IKCP_PROXY_DEFINES_E:uint32_t{
//version
    IKCP_PROTOCAL_VERSION=1,
    IKCP_RAW_UDP_CONV_ID=0,
//conv_id define
    IKCP_MIN_APPLICATION_CONV_ID=1,
    IKCP_MAX_APPLICATION_CONV_ID=0xfffffffe,
    IKCP_PROXY_CONTROL_CONV_ID=0xffffffff,
//length
    IKCP_PROXY_HEADER_LEN=12,
    IKCP_CONFIG_LEN=5,
};

enum IKCP_PROXY_CMD_E:uint8_t{
    IKCP_SYN,
    IKCP_SYN_ACK,
    IKCP_CONNECTION_FINISHED,
    IKCP_RST,
    IKCP_KEEPALIVE,
    IKCP_PAYLOAD,
};
struct ikcp_proxy_header_s{
    uint32_t protocal_version:8;
    uint32_t cmd:8;
    uint32_t sn:16;
    uint32_t timestamp;
    uint32_t conv_id;
};
struct ikcp_config_s{
    uint32_t window_size:16;
    uint32_t interval:14;
    uint32_t nodelay:1;
    uint32_t nc:1;
    uint8_t resend;
};
class ikcp_proxy_helper{
public:
    static bool encode_ikcp_proxy_header(void *buf,uint32_t available_len,const ikcp_proxy_header_s&header);
    static bool decode_ikcp_proxy_header(const void*buf,uint32_t available_len,ikcp_proxy_helper &header);
    static bool encode_ikcp_config(void *buf,uint32_t available_len,const ikcp_config_s&config);
    static bool decode_config(const void*buf,uint32_t available_len,ikcp_config_s &config);
};
}
#endif // KCP_COMMON_H
