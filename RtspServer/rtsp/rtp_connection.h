#ifndef RTP_CONNECTION_H
#define RTP_CONNECTION_H
#include "media.h"
#include "rtp.h"
#include "network_util.h"
#include <mutex>
namespace micagent {
class rtsp_connection;
class rtp_connection{
    friend class rtsp_connection;
public:
    rtp_connection(rtsp_connection *_rtsp_connection);
    virtual ~rtp_connection();
    bool sendRtpPacket(MediaChannelId id,RtpPacket packet);
    void setClockRate(MediaChannelId channelId, uint32_t clockRate);
    void setPayloadType(MediaChannelId channelId, uint32_t payload);
    void setBuildTimesTamp(MediaChannelId channelId,int64_t micro_timestamp);
    bool setupRtpOverTcp(MediaChannelId channelId, uint16_t rtpChannel, uint16_t rtcpChannel);
    bool setupRtpOverUdp(MediaChannelId channelId, uint16_t rtpPort, uint16_t rtcpPort);
    bool setupRtpOverMulticast(MediaChannelId channelId, std::string ip, uint16_t port);
    SOCKET get_rtcp_fd(MediaChannelId channel_id)const;
    uint16_t get_rtp_port(MediaChannelId channel_id)const;
    uint16_t get_rtcp_port(MediaChannelId channel_id)const;
    uint32_t get_session_id()const{
        return static_cast<uint32_t>((size_t)this);
    }
    void startPlay();
    void stopPlay();
    string get_rtp_Info(const string &url);
    void set_see_idr(){
        lock_guard<mutex>locker(m_mutex);
        m_see_idr=true;
    }
    bool get_see_idr(){
        lock_guard<mutex>locker(m_mutex);
        return m_see_idr;
    }
    void set_got_gop(){
        lock_guard<mutex>locker(m_mutex);
        m_got_gop=true;
    }
    bool get_got_gop(){
        lock_guard<mutex>locker(m_mutex);
        return m_got_gop;
    }
private:
    void dump_rtp_head_info(MediaChannelId channelId);
    inline void setRtpHeader(MediaChannelId channelId, RtpPacket pkt)
    {
        m_mediaChannelInfo[channelId].rtpHeader.marker = pkt.last;
        m_mediaChannelInfo[channelId].rtpHeader.ts = htonl(pkt.timestamp);
        m_mediaChannelInfo[channelId].rtpHeader.seq = htons(m_mediaChannelInfo[channelId].packetSeq++);
        memcpy(pkt.data.get()+4, &m_mediaChannelInfo[channelId].rtpHeader, RTP_HEADER_SIZE);
    }
    rtsp_connection *m_rtsp_connection;
    MediaChannelInfo m_mediaChannelInfo[MAX_MEDIA_CHANNEL];
    TransportMode m_transportMode;
    SOCKET m_rtpfd[MAX_MEDIA_CHANNEL], m_rtcpfd[MAX_MEDIA_CHANNEL];
    uint16_t m_localRtpPort[MAX_MEDIA_CHANNEL];
    uint16_t m_localRtcpPort[MAX_MEDIA_CHANNEL];
    bool m_is_closed;
    mutable mutex m_mutex;
    bool m_see_idr;
    bool m_got_gop;
};
}
#endif // RTP_CONNECTION_H
