#ifndef WEBSOCKET_BUFFER_CACHE_H
#define WEBSOCKET_BUFFER_CACHE_H
#include "buffer_handle.h"
#include "websocket_common.h"
namespace micagent {
/**
 * @brief The web_socket_buffer_cache class a buf cache which handle the websocket in the form of stream input
 */
class web_socket_buffer_cache:public buffer_handle{
    static constexpr uint8_t INVALID_WS_PAYLOAD_LEN=0xff;
    static constexpr uint32_t MAX_WS_FRAME_CACHE=500;
    static constexpr uint32_t WEBSOCKET_PIECE_SIZE=1400;
public:
    web_socket_buffer_cache(uint32_t capacity,bool is_websocket_send=false);
    ~web_socket_buffer_cache()override{}
    static void util_test();
protected:
    bool insert_packet(const char *buf,uint32_t buf_len)override;
private:
    void reset_ws_status(){
        m_ws_payload_len=INVALID_WS_PAYLOAD_LEN;
        m_ws_data_miss_len=0;
    }
private:
    /**
     * @brief m_is_websocket_send judge  whether  the buf is a send buf
     */
    bool m_is_websocket_send;
    /**
     * @brief m_ws_buf_cache cache the outstanding data
     */
    shared_ptr<BufferPacket>m_ws_buf_cache;
    /**
     * @brief m_ws_payload_len when it's value is INVALID_WS_PAYLOAD_LEN,this buf will try to parse the cache data to find a payload_len
     */
    uint8_t m_ws_payload_len;
    /**
     * @brief m_ws_data_miss_len last websocket's frame missing len
     */
    uint32_t m_ws_data_miss_len;
};
}
#endif // WEBSOCKET_BUFFER_CACHE_H
