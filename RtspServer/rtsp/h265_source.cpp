
#if defined(WIN32) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "h265_source.h"
#include "Base64.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

using namespace micagent;
using namespace std;

h265_source::h265_source(uint32_t frameRate)
    : m_frameRate(frameRate),m_send_counts(0)
{
    m_payload = 96;
    m_mediaType = H265;
    m_clockRate = 90000;
}
media_frame_type h265_source::get_frame_type(uint8_t byte)
{
    media_frame_type ret=FRAME_UNKNOWN;
    int nalu_type=(byte&0x7E)>>1;
    if(nalu_type>=16&&nalu_type<=21)ret=FRAME_I;
    switch (nalu_type) {
    case 1:
    case 2:
        ret=FRAME_P;
        break;
    case 32:
        ret=FRAME_VPS;
        break;
    case 33:
        ret=FRAME_SPS;
        break;
    case 34:
        ret=FRAME_PPS;
        break;
    case 39:
    case 40:
        ret=FRAME_SEI;
        break;
    default:
        break;
    }
    return  ret;
}
bool h265_source::check_frames(media_frame_type type, AVFrame &frame, uint32_t offset,uint32_t frame_len)
{
    bool ret=false;
    do{
        if(type==FRAME_UNKNOWN||frame_len==0)break;
        auto frame_begin_ptr=frame.buffer.get()+offset;
        if(type==FRAME_I){
            //i
            m_last_iframe.reset(new AVFrame(frame_len));
            memcpy(m_last_iframe.get()->buffer.get(),frame_begin_ptr,frame_len);
            m_last_iframe->type=frame.type;
            m_send_counts=m_frameRate*2+2;
        }
        else if (type==FRAME_SPS) {
            m_frame_sps.reset(new AVFrame(frame_len));
            m_frame_sps->type=frame.type;
            memcpy(m_frame_sps.get()->buffer.get(),frame_begin_ptr,frame_len);
        }
        else if (type==FRAME_VPS) {
            m_frame_vps.reset(new AVFrame(frame_len));
            m_frame_vps->type=frame.type;
            memcpy(m_frame_vps.get()->buffer.get(),frame_begin_ptr,frame_len);
            m_send_counts=m_frameRate*2+2;
        }
        else if (type==FRAME_PPS) {
            m_frame_pps.reset(new AVFrame(frame_len));
            m_frame_pps->type=frame.type;
            memcpy(m_frame_pps.get()->buffer.get(),frame_begin_ptr,frame_len);
        }
        else if (type==FRAME_SEI) {
            m_last_sei.reset(new AVFrame(frame_len));
            m_last_sei->type=frame.type;
            memcpy(m_last_sei.get()->buffer.get(),frame_begin_ptr,frame_len);
        }
        else {
            if(m_send_counts!=0)m_send_counts--;
            if(m_send_counts==0){
                break;
            }
        }
        ret=true;
    }while(0);
    return ret;
}
shared_ptr<h265_source> h265_source::createNew(uint32_t frameRate)
{
    return make_shared<h265_source>(frameRate);
}

h265_source::~h265_source()
{

}

string h265_source::getMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP 96", port);

    return string(buf);
}

string h265_source::getAttribute()
{
    return string("a=rtpmap:96 H265/90000");
}
std::string h265_source::getAttributeFmtp()
{
    string ret("");
    do{
        if(!m_frame_pps||!m_frame_sps||!m_frame_vps)break;
        shared_ptr<uint8_t> vpsWEB(new uint8_t[m_frame_vps->size],std::default_delete<uint8_t[]>());
        auto vpsWEBSize = removeH264or5EmulationBytes(vpsWEB, m_frame_vps->size, m_frame_vps->buffer, m_frame_vps->size);
        if (vpsWEBSize < 6/*'profile_tier_level' offset*/ + 12/*num 'profile_tier_level' bytes*/) break;
        uint8_t const* profileTierLevelHeaderBytes = &vpsWEB.get()[6];
        unsigned profileSpace  = profileTierLevelHeaderBytes[0]>>6; // general_profile_space
        unsigned profileId = profileTierLevelHeaderBytes[0]&0x1F; // general_profile_idc
        unsigned tierFlag = (profileTierLevelHeaderBytes[0]>>5)&0x1; // general_tier_flag
        unsigned levelId = profileTierLevelHeaderBytes[11]; // general_level_idc
        u_int8_t const* interop_constraints = &profileTierLevelHeaderBytes[5];
        char interopConstraintsStr[100];
        sprintf(interopConstraintsStr, "%02X%02X%02X%02X%02X%02X",
                interop_constraints[0], interop_constraints[1], interop_constraints[2],
                interop_constraints[3], interop_constraints[4], interop_constraints[5]);

        auto sprop_vps = micagent::base64Encode(m_frame_vps->buffer.get() ,m_frame_vps->size);
        auto sprop_sps = micagent::base64Encode(m_frame_sps->buffer.get() ,m_frame_sps->size);
        auto sprop_pps = micagent::base64Encode(m_frame_pps->buffer.get() ,m_frame_pps->size);

        char const* fmtpFmt =
                "a=fmtp:%d profile-space=%u"
                ";profile-id=%u"
                ";tier-flag=%u"
                ";level-id=%u"
                ";interop-constraints=%s"
                ";sprop-vps=%s"
                ";sprop-sps=%s"
                ";sprop-pps=%s\r\n";
        unsigned fmtpFmtSize = strlen(fmtpFmt)
                + 3 /* max num chars: rtpPayloadType */ + 20 /* max num chars: profile_space */
                + 20 /* max num chars: profile_id */
                + 20 /* max num chars: tier_flag */
                + 20 /* max num chars: level_id */
                + strlen(interopConstraintsStr)
                + sprop_vps.size()
                +sprop_sps.size()
                + sprop_pps.size();
        shared_ptr<char>fmtp(new char[fmtpFmtSize],std::default_delete<char[]>());
        sprintf(fmtp.get(), fmtpFmt,
                m_payload, profileSpace,
                profileId,
                tierFlag,
                levelId,
                interopConstraintsStr,
                sprop_vps.c_str(),
                sprop_sps.c_str(),
                sprop_pps.c_str());
        ret=fmtp.get();
    }while(0);
    return ret;
}
bool h265_source::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    auto start_pos=find_next_video_nal_pos(frame.buffer.get(),frame.size,0);
    if(start_pos==frame.size)return  false;
    int safty_counts=6;
    while ((safty_counts--)>0) {
        auto type=this->get_frame_type(frame.buffer.get()[start_pos]);
        uint32_t next_pos=frame.size;
        if(type==FRAME_I||type==FRAME_P||type==FRAME_B){

        }else {
            next_pos=find_next_video_nal_pos(frame.buffer.get(),frame.size,start_pos+1);
        }
        frame.type=type;
        uint8_t *frameBuf  = frame.buffer.get()+start_pos;
        uint32_t frameSize = next_pos-start_pos;
        if(next_pos!=frame.size&&frameSize>=4)frameSize-=4;
        if(!check_frames(type,frame,start_pos,frameSize))return true;
        auto timestamp=getTimeStamp(frame.timestamp);
        if (frameSize <= MAX_RTP_PAYLOAD_SIZE)
        {
            RtpPacket rtpPkt;
            rtpPkt.type = frame.type;
            rtpPkt.timestamp = timestamp;
            rtpPkt.size = frameSize + 4 + RTP_HEADER_SIZE;
            rtpPkt.last = 1;

            memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE, frameBuf, frameSize); // 预留 4字节TCP Header, 12字节 RTP Header

            if (m_sendFrameCallback)
            {
                if(!m_sendFrameCallback(channelId, rtpPkt))return false;
            }
        }
        else
        {
            // 参考live555
            char FU[3] = {0};
            char nalUnitType = (frameBuf[0] & 0x7E) >> 1;
            FU[0] = (frameBuf[0] & 0x81) | (49<<1);
            FU[1] = frameBuf[1];
            FU[2] = (0x80 | nalUnitType);

            frameBuf  += 2;
            frameSize -= 2;

            while (frameSize + 3 > MAX_RTP_PAYLOAD_SIZE)
            {
                RtpPacket rtpPkt;
                rtpPkt.type = frame.type;
                rtpPkt.timestamp = timestamp;
                rtpPkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
                rtpPkt.last = 0;

                rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
                rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
                rtpPkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
                memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+3, frameBuf, MAX_RTP_PAYLOAD_SIZE-3);

                if (m_sendFrameCallback)
                {
                    if(!m_sendFrameCallback(channelId, rtpPkt))return false;
                }

                frameBuf  += (MAX_RTP_PAYLOAD_SIZE - 3);
                frameSize -= (MAX_RTP_PAYLOAD_SIZE - 3);

                FU[2] &= ~0x80;
            }

            {
                RtpPacket rtpPkt;
                rtpPkt.type = frame.type;
                rtpPkt.timestamp = timestamp;
                rtpPkt.size = 4 + RTP_HEADER_SIZE + 3 + frameSize;
                rtpPkt.last = 1;

                FU[2] |= 0x40;
                rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
                rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
                rtpPkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
                memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+3, frameBuf, frameSize);

                if (m_sendFrameCallback)
                {
                    if(!m_sendFrameCallback(channelId, rtpPkt))return false;
                }
            }
        }
        if(next_pos>=frame.size)break;
        start_pos=next_pos;
    }
    return true;
}
bool h265_source::handleGopCache(MediaChannelId channelid,shared_ptr<rtp_connection>connection)
{
    if(m_frame_vps)
    {
        connection->set_got_gop();
        queue<shared_ptr<AVFrame>>m_frame_queue;
        if(m_frame_vps)m_frame_queue.push(m_frame_vps);
        if(m_frame_pps)m_frame_queue.push(m_frame_sps);
        if(m_frame_pps)m_frame_queue.push(m_frame_pps);
        if(m_last_sei)m_frame_queue.push(m_last_sei);
        if(m_last_iframe)m_frame_queue.push(m_last_iframe);
        auto timestamp=getTimeStamp(m_last_mill_recv_time);

        while(!m_frame_queue.empty())
        {
            auto front=m_frame_queue.front();
            uint8_t *frameBuf  = front.get()->buffer.get();
            uint32_t frameSize = front.get()->size;
            if (frameSize <= MAX_RTP_PAYLOAD_SIZE)
            {
                RtpPacket rtpPkt;
                rtpPkt.type = front->type;
                rtpPkt.timestamp = timestamp;
                rtpPkt.size = frameSize + 4 + RTP_HEADER_SIZE;
                rtpPkt.last = 1;

                memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE, frameBuf, frameSize); // 预留 4字节TCP Header, 12字节 RTP Header
                if(!connection->sendRtpPacket(channelid,rtpPkt))continue;
            }
            else
            {
                // 参考live555
                char FU[3] = {0};
                char nalUnitType = (frameBuf[0] & 0x7E) >> 1;
                FU[0] = (frameBuf[0] & 0x81) | (49<<1);
                FU[1] = frameBuf[1];
                FU[2] = (0x80 | nalUnitType);

                frameBuf  += 2;
                frameSize -= 2;

                while (frameSize + 3 > MAX_RTP_PAYLOAD_SIZE)
                {
                    RtpPacket rtpPkt;
                    rtpPkt.type = front->type;
                    rtpPkt.timestamp = timestamp;
                    rtpPkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
                    rtpPkt.last = 0;

                    rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
                    rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
                    rtpPkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
                    memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+3, frameBuf, MAX_RTP_PAYLOAD_SIZE-3);
                    if(!connection->sendRtpPacket(channelid,rtpPkt))continue;

                    frameBuf  += (MAX_RTP_PAYLOAD_SIZE - 3);
                    frameSize -= (MAX_RTP_PAYLOAD_SIZE - 3);

                    FU[2] &= ~0x80;
                }

                {
                    RtpPacket rtpPkt;
                    rtpPkt.type = front->type;
                    rtpPkt.timestamp = timestamp;
                    rtpPkt.size = 4 + RTP_HEADER_SIZE + 3 + frameSize;
                    rtpPkt.last = 1;

                    FU[2] |= 0x40;
                    rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU[0];
                    rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU[1];
                    rtpPkt.data.get()[RTP_HEADER_SIZE+6] = FU[2];
                    memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+3, frameBuf, frameSize);

                    if(!connection->sendRtpPacket(channelid,rtpPkt))continue;
                }
            }
            m_frame_queue.pop();
        }
        return true;
    }
    else {
        return false;
    }
}

uint32_t h265_source::getTimeStamp(int64_t mill_second)
{
    if(m_last_mill_recv_time==0||mill_second==0)
    {
        auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
        m_last_mill_send_time=timePoint.time_since_epoch().count();
        m_last_mill_recv_time=mill_second;
        return (uint32_t)((m_last_mill_send_time)  * 90);
    }
    else {
        m_last_mill_send_time+=(mill_second-m_last_mill_recv_time);
        m_last_mill_recv_time=mill_second;
        return (uint32_t)((m_last_mill_send_time ) * 90);
    }
}
