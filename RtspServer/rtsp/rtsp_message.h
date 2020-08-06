#ifndef RTSP_MESSAGE_H
#define RTSP_MESSAGE_H
#include "buffer_handle.h"
namespace micagent {
using namespace std;
class rtsp_message:public buffer_handle{
    static constexpr uint32_t MAX_TCP_MSS_CACHE=10000;
    typedef enum{
        NONE_PACKET,
        RTCP_PACKET,
        RTSP_PACKET
    }PACKET_TYPE;
public:
    rtsp_message(bool is_send=false);
    ~rtsp_message(){}
protected:
    bool insert_packet(const char *buf,uint32_t buf_len);
private:
    bool m_is_send;
    /**
      * @brief m_crlf 帧分隔符示例 \r\n\r\n\r\n
      */
    static  constexpr const  char *m_crlf="\r\n\r\n";
    /**
     * @brief m_crlf_len 帧分隔符示例长度
     */
    constexpr const static uint8_t m_crlf_len=4;
    constexpr const static uint32_t m_max_packet_size=32968;
    shared_ptr<BufferPacket>m_buf_cache;
    PACKET_TYPE m_packet_type;
    uint32_t m_rtcp_len;
};
}
#endif // RTSP_MESSAGE_H
