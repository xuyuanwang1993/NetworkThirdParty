#ifndef RTMP_MESSAGE_H
#define RTMP_MESSAGE_H
#include "buffer_handle.h"
#include "rtmp_common.h"
namespace micagent {
class rtmp_connection;
class rtmp_message{
    friend class rtmp_connection;
public:
    rtmp_message(rtmp_connection *_rtmp_connection,RTMP_HANDSHAKE_STATUS status);
    ~rtmp_message();
private:
    bool handle_read();
    bool handle_write();
    bool is_handshake_complete()const;
    pair<const uint8_t *,uint32_t>get_handshake_packet();
private:
//message handle
    bool handle_rtmp_chunk(shared_ptr<rtmp_message_packet>packet);
    bool handle_data();
private:
    handshake_info m_shake_info;
    rtmp_connection *const m_rtmp_connection;
    shared_ptr<BufferPacket>m_send_cache;
    shared_ptr<BufferPacket>m_recv_cache;
    map<uint32_t,shared_ptr<rtmp_message_packet>>m_recv_packet_cache;
};
}
#endif // RTMP_MESSAGE_H
