#include "rtp_connection.h"
#include "rtsp_connection.h"
#include <random>
using namespace micagent;
rtp_connection::rtp_connection(rtsp_connection *_rtsp_connection):m_rtsp_connection(_rtsp_connection),m_transportMode(RTP_OVER_TCP),m_is_closed(false)
{
    random_device rd;
    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        m_rtpfd[chn]=INVALID_SOCKET;
        m_rtcpfd[chn]=INVALID_SOCKET;
        m_localRtpPort[chn]=0;
        m_localRtcpPort[chn]=0;
        memset(&m_mediaChannelInfo[chn], 0, sizeof(m_mediaChannelInfo[chn]));
        m_mediaChannelInfo[chn].rtpHeader.version = RTP_VERSION;
        m_mediaChannelInfo[chn].packetSeq = rd()&0xffff;
        m_mediaChannelInfo[chn].rtpHeader.seq = 0;
        m_mediaChannelInfo[chn].rtpHeader.ts = htonl(rd());
        m_mediaChannelInfo[chn].rtpHeader.ssrc = htonl(rd());
    }
}
rtp_connection::   ~rtp_connection()
{
    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        if(m_rtpfd[chn] !=INVALID_SOCKET)
        {
            NETWORK.close_socket(m_rtpfd[chn]);
        }

        if(m_rtcpfd[chn] != INVALID_SOCKET)
        {
            NETWORK.close_socket(m_rtcpfd[chn]);
        }
    }
}
bool rtp_connection::sendRtpPacket(MediaChannelId id,RtpPacket packet)
{
    lock_guard<mutex>locker(m_mutex);
    if(m_is_closed)return false;
    if(!m_mediaChannelInfo[id].isSetup)return true;
    setRtpHeader(id,packet);
    if(m_transportMode==RTP_OVER_TCP){
        uint8_t* rtpPktPtr = packet.data.get();
        rtpPktPtr[0] = '$';
        rtpPktPtr[1] = (char)m_mediaChannelInfo[id].rtpChannel;
        rtpPktPtr[2] = (char)(((packet.size-4)&0xFF00)>>8);
        rtpPktPtr[3] = (char)((packet.size -4)&0xFF);
        return m_rtsp_connection->send_message((char*)rtpPktPtr, packet.size);
    }
    else {
        int ret = send(m_rtpfd[id], (const char*)packet.data.get()+4, packet.size-4, 0);
        if(ret < 0)
        {
        //close
            if(!m_is_closed)
            {
                m_is_closed = true;
                for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
                {
                    m_mediaChannelInfo[chn].isPlay = false;
                    m_mediaChannelInfo[chn].isRecord = false;
                }
            }
            return false;
        }
        return true;
    }
}
void rtp_connection::setClockRate(MediaChannelId channelId, uint32_t clockRate)
{
    lock_guard<mutex>locker(m_mutex);
    m_mediaChannelInfo[channelId].clockRate = clockRate;
}
void rtp_connection::setPayloadType(MediaChannelId channelId, uint32_t payload)
{
    lock_guard<mutex>locker(m_mutex);
    m_mediaChannelInfo[channelId].rtpHeader.payload = payload;
}
bool rtp_connection::setupRtpOverTcp(MediaChannelId channelId, uint16_t rtpChannel, uint16_t rtcpChannel)
{
    lock_guard<mutex>locker(m_mutex);
    m_mediaChannelInfo[channelId].rtpChannel = rtpChannel;
    m_mediaChannelInfo[channelId].rtcpChannel = rtcpChannel;
    m_rtpfd[channelId] = m_rtsp_connection->fd();
    m_rtcpfd[channelId] = m_rtsp_connection->fd();
    m_mediaChannelInfo[channelId].isSetup = true;
    m_transportMode = RTP_OVER_TCP;
    return true;
}
bool rtp_connection::setupRtpOverUdp(MediaChannelId channelId, uint16_t rtpPort, uint16_t rtcpPort)
{
    lock_guard<mutex>locker(m_mutex);
    m_mediaChannelInfo[channelId].rtpPort = rtpPort;
    m_mediaChannelInfo[channelId].rtcpPort = rtcpPort;

    std::random_device rd;
    for (int n = 0; n <= 10; n++)
    {
        if(n == 10)
            return false;

        m_localRtpPort[channelId] = rd() & 0xfffe;
        m_localRtcpPort[channelId] =m_localRtpPort[channelId] + 1;

        m_rtpfd[channelId] = ::socket(AF_INET, SOCK_DGRAM, 0);
        if(!NETWORK.bind(m_rtpfd[channelId], m_localRtpPort[channelId]))
        {
            NETWORK.close_socket(m_rtpfd[channelId]);
            continue;
        }

        m_rtcpfd[channelId] = NETWORK.build_socket(UDP);
        if(!NETWORK.bind(m_rtcpfd[channelId], m_localRtcpPort[channelId]))
        {
            NETWORK.close_socket(m_rtpfd[channelId]);
            NETWORK.close_socket(m_rtcpfd[channelId]);
            continue;
        }

        break;
    }

    NETWORK.set_send_buf_size(m_rtpfd[channelId], 50*1024);

    m_mediaChannelInfo[channelId].isSetup = true;
    m_transportMode = RTP_OVER_UDP;
    auto peer_ip=NETWORK.get_peer_ip(m_rtsp_connection->fd());
    MICAGENT_LOG(LOG_INFO,"%s %hu %hu",peer_ip.c_str(),m_mediaChannelInfo[channelId].rtpPort,\
                 m_mediaChannelInfo[channelId].rtcpPort);
    NETWORK.connect(m_rtpfd[channelId],peer_ip,m_mediaChannelInfo[channelId].rtpPort);
    NETWORK.connect(m_rtcpfd[channelId],peer_ip,m_mediaChannelInfo[channelId].rtcpPort);
    NETWORK.make_noblocking(m_rtpfd[channelId]);
    NETWORK.make_noblocking(m_rtcpfd[channelId]);
    return true;
}
bool rtp_connection::setupRtpOverMulticast(MediaChannelId channelId, std::string ip, uint16_t port)
{
    lock_guard<mutex>locker(m_mutex);
    std::random_device rd;
    for (int n = 0; n <= 10; n++)
    {
        if (n == 10)
            return false;

        m_localRtpPort[channelId] = rd() & 0xfffe;
        m_rtpfd[channelId] = NETWORK.build_socket(UDP);
        if (!NETWORK.bind(m_rtpfd[channelId],  m_localRtpPort[channelId]))
        {
            NETWORK.close_socket(m_rtpfd[channelId]);
            continue;
        }
        break;
    }
    m_mediaChannelInfo[channelId].rtpPort = port;
    m_mediaChannelInfo[channelId].isSetup = true;
    NETWORK.connect(m_rtpfd[channelId],ip,m_mediaChannelInfo[channelId].rtpPort);
    m_transportMode = RTP_OVER_MULTICAST;
    return true;
}
SOCKET rtp_connection::get_rtcp_fd(MediaChannelId channel_id)const
{
    lock_guard<mutex>locker(m_mutex);
    return m_rtcpfd[channel_id];
}
uint16_t rtp_connection::get_rtp_port(MediaChannelId channel_id)const
{
    lock_guard<mutex>locker(m_mutex);
    return m_localRtpPort[channel_id];
}
uint16_t rtp_connection::get_rtcp_port(MediaChannelId channel_id)const
{
    lock_guard<mutex>locker(m_mutex);
    return m_localRtcpPort[channel_id];
}
void rtp_connection::startPlay()
{
    lock_guard<mutex>locker(m_mutex);
    for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        if (m_mediaChannelInfo[chn].isSetup)
        {
            m_mediaChannelInfo[chn].isPlay = true;
        }
    }
}
void rtp_connection::stopPlay()
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_is_closed)
    {
        m_is_closed = true;
        for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
        {
            m_mediaChannelInfo[chn].isPlay = false;
            m_mediaChannelInfo[chn].isRecord = false;
        }
    }
}
string rtp_connection::get_rtp_Info(const string &url)
{
    lock_guard<mutex>locker(m_mutex);
    if(url.empty())return string();
    char buf[2048] = { 0 };
    snprintf(buf, 1024, "RTP-Info: ");

    int numChannel = 0;

    auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
    auto ts = timePoint.time_since_epoch().count();
    for (int chn = 0; chn<MAX_MEDIA_CHANNEL; chn++)
    {
        uint32_t rtpTime = (uint32_t)((ts+500)/1000000*m_mediaChannelInfo[chn].clockRate);
        if (m_mediaChannelInfo[chn].isSetup)
        {
            if (numChannel != 0)
                snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), ",");

            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "url=%s/track%d;seq=0;rtptime=%u",
                     url.c_str(), chn, rtpTime);
            numChannel++;
        }
    }

    return std::string(buf);
}
void rtp_connection::dump_rtp_head_info(MediaChannelId channelId)
{
    auto &h=m_mediaChannelInfo[channelId].rtpHeader;
    MICAGENT_LOG(LOG_WARNNING,"extension:%d version:%d payload:%d padding:%d marker:%d\
                 ssrc:%u csrc:%u seq:%hu ts:%u",h.extension,h.version,h.payload,h.padding,h.marker,h.ssrc,h.csrc,h.seq,h.ts);
}
