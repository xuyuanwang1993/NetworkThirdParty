#if defined(WIN32) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "h264_source.h"
#include <cstdio>
#include <chrono>
#include "Base64.h"
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

using namespace micagent;
using namespace std;

h264_source::h264_source(uint32_t frameRate)
    : m_frameRate(frameRate),m_send_counts(0)
{
    m_payload = 96; // rtp负载类型
    m_mediaType = H264;
    m_clockRate = 90000;
}
media_frame_type h264_source::get_frame_type(uint8_t byte)
{
    media_frame_type ret=FRAME_UNKNOWN;
    int nalu_type=byte&0x1f;
    switch (nalu_type) {
    case 1:
        ret=FRAME_P;
        break;
    case 5:
        ret=FRAME_I;
        break;
    case 6:
        ret=FRAME_SEI;
        break;
    case 7:
        ret=FRAME_SPS;
        break;
    case 8:
        ret=FRAME_PPS;
        break;
    default:
        break;
    }
    return ret;
}
bool h264_source::check_frames(media_frame_type type, AVFrame frame)
{
    bool ret=false;
    do{
        if(type==FRAME_UNKNOWN)break;
        if(type==FRAME_I){
            //i
            m_last_iframe.reset(new AVFrame(frame.size));
            m_last_iframe->type=frame.type;
            memcpy(m_last_iframe.get()->buffer.get(),frame.buffer.get(),frame.size);
            m_send_counts=m_frameRate*2+2;
        }
        else if (type==FRAME_SPS) {
            m_frame_sps.reset(new AVFrame(frame.size));
            m_frame_sps->type=frame.type;
            memcpy(m_frame_sps.get()->buffer.get(),frame.buffer.get(),frame.size);
        }
        else if (type==FRAME_PPS) {
            m_frame_pps.reset(new AVFrame(frame.size));
            m_frame_pps->type=frame.type;
            memcpy(m_frame_pps.get()->buffer.get(),frame.buffer.get(),frame.size);
        }
        else if (type==FRAME_SEI) {
            m_last_sei.reset(new AVFrame(frame.size));
            m_last_sei->type=frame.type;
            memcpy(m_last_sei.get()->buffer.get(),frame.buffer.get(),frame.size);
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
shared_ptr<h264_source> h264_source::createNew(uint32_t frameRate)
{
    return make_shared<h264_source>(frameRate);
}

h264_source::~h264_source()
{

}

string h264_source::getMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP 96", port); // \r\nb=AS:2000

    return string(buf);
}

string h264_source::getAttribute()
{
    return string("a=rtpmap:96 H264/90000");
}
std::string h264_source::getAttributeFmtp()
{
    string ret("");
    do{
        if(!m_frame_sps||!m_frame_pps)break;
        // Set up the "a=fmtp:" SDP line for this stream:
        shared_ptr<uint8_t>spsWEB(new uint8_t[m_frame_sps->size],std::default_delete<uint8_t[]>());
        auto spsWEBSize = removeH264or5EmulationBytes(spsWEB, m_frame_sps->size, m_frame_sps->buffer, m_frame_sps->size);
        if (spsWEBSize < 4) break;
        uint32_t profileLevelId = (spsWEB.get()[1]<<16) | (spsWEB.get()[2]<<8) | spsWEB.get()[3];
        auto sps_base64 = micagent::base64Encode(m_frame_sps->buffer.get() ,m_frame_sps->size);
        auto pps_base64 = micagent::base64Encode(m_frame_pps->buffer.get() ,m_frame_pps->size);
        char const* fmtpFmt =
                "a=fmtp:%d packetization-mode=1"
                ";profile-level-id=%06X"
                ";sprop-parameter-sets=%s,%s\r\n";
        unsigned fmtpFmtSize = strlen(fmtpFmt)
                + 3 /* max char len */
                + 6 /* 3 bytes in hex */
                + sps_base64.size() + pps_base64.size();
        shared_ptr<char>fmtp(new char[fmtpFmtSize],std::default_delete<char[]>());
        sprintf(fmtp.get(), fmtpFmt,
                m_payload,
                profileLevelId,
                sps_base64.c_str(), pps_base64.c_str());
        ret=fmtp.get();
    }while(0);
    return  ret;
}
bool h264_source::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    auto type=this->get_frame_type(frame.buffer.get()[0]);
    if(!check_frames(type,frame))return true;
    frame.type=type;;
    uint8_t *frameBuf  = frame.buffer.get();
    uint32_t frameSize = frame.size;
    auto timestamp=getTimeStamp(frame.timestamp);
    if (frameSize <= MAX_RTP_PAYLOAD_SIZE)
    {
        RtpPacket rtpPkt;
        rtpPkt.type = frame.type;
        rtpPkt.timestamp = timestamp;
        rtpPkt.size = frameSize + 4 + RTP_HEADER_SIZE;
        rtpPkt.last = 1;
        memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE, frameBuf, frameSize); // 预留12字节 rtp header

        if (m_sendFrameCallback)
        {
            if(!m_sendFrameCallback(channelId, rtpPkt))return false;
        }
    }
    else
    {
        char FU_A[2] = {0};

        // 分包参考live555
        FU_A[0] = (frameBuf[0] & 0xE0) | 28;
        FU_A[1] = 0x80 | (frameBuf[0] & 0x1f);

        frameBuf  += 1;
        frameSize -= 1;

        while (frameSize + 2 > MAX_RTP_PAYLOAD_SIZE)
        {
            RtpPacket rtpPkt;
            rtpPkt.type = frame.type;
            rtpPkt.timestamp = timestamp;
            rtpPkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
            rtpPkt.last = 0;

            rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
            rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
            memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+2, frameBuf, MAX_RTP_PAYLOAD_SIZE-2);

            if (m_sendFrameCallback)
            {
                if(!m_sendFrameCallback(channelId, rtpPkt))return false;
            }

            frameBuf  += MAX_RTP_PAYLOAD_SIZE - 2;
            frameSize -= MAX_RTP_PAYLOAD_SIZE - 2;

            FU_A[1] &= ~0x80;
        }

        {
            RtpPacket rtpPkt;
            rtpPkt.type = frame.type;
            rtpPkt.timestamp = timestamp;
            rtpPkt.size = 4 + RTP_HEADER_SIZE + 2 + frameSize;
            rtpPkt.last = 1;

            FU_A[1] |= 0x40;
            rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
            rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
            memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+2, frameBuf, frameSize);

            if (m_sendFrameCallback)
            {
                if(!m_sendFrameCallback(channelId, rtpPkt))return false;
            }
        }
    }
    return true;
}
bool h264_source::handleGopCache(MediaChannelId channelid,shared_ptr<rtp_connection>connection)
{
    if(m_frame_sps&&m_frame_pps&&m_last_iframe)
    {
        connection->set_got_gop();
        queue<shared_ptr<AVFrame>>m_frame_queue;
        m_frame_queue.push(m_frame_sps);
        m_frame_queue.push(m_frame_pps);
        if(m_last_sei)m_frame_queue.push(m_last_sei);
        m_frame_queue.push(m_last_iframe);
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
                char FU_A[2] = {0};

                // 分包参考live555
                FU_A[0] = (frameBuf[0] & 0xE0) | 28;
                FU_A[1] = 0x80 | (frameBuf[0] & 0x1f);

                frameBuf  += 1;
                frameSize -= 1;

                while (frameSize + 2 > MAX_RTP_PAYLOAD_SIZE)
                {
                    RtpPacket rtpPkt;
                    rtpPkt.type = front->type;
                    rtpPkt.timestamp = timestamp;
                    rtpPkt.size = 4 + RTP_HEADER_SIZE + MAX_RTP_PAYLOAD_SIZE;
                    rtpPkt.last = 0;

                    rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
                    rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
                    memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+2, frameBuf, MAX_RTP_PAYLOAD_SIZE-2);
                    if(!connection->sendRtpPacket(channelid,rtpPkt))continue;

                    frameBuf  += MAX_RTP_PAYLOAD_SIZE - 2;
                    frameSize -= MAX_RTP_PAYLOAD_SIZE - 2;

                    FU_A[1] &= ~0x80;
                }

                {
                    RtpPacket rtpPkt;
                    rtpPkt.type = front->type;
                    rtpPkt.timestamp = timestamp;
                    rtpPkt.size = 4 + RTP_HEADER_SIZE + 2 + frameSize;
                    rtpPkt.last = 1;

                    FU_A[1] |= 0x40;
                    rtpPkt.data.get()[RTP_HEADER_SIZE+4] = FU_A[0];
                    rtpPkt.data.get()[RTP_HEADER_SIZE+5] = FU_A[1];
                    memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+2, frameBuf, frameSize);

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
uint32_t h264_source::getTimeStamp(int64_t mill_second)
{
    if(m_last_mill_recv_time==0||mill_second==0)
    {
        auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
        m_last_micro_send_time=timePoint.time_since_epoch().count();
        m_last_mill_recv_time=mill_second;
        return (uint32_t)((m_last_micro_send_time) * 90);
    }
    else {
        m_last_micro_send_time+=(mill_second-m_last_mill_recv_time);
        m_last_mill_recv_time=mill_second;
        return (uint32_t)((m_last_micro_send_time ) * 90);
    }
}
