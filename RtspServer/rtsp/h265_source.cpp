
#if defined(WIN32) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "h265_source.h"
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
bool h265_source::check_frames(AVFrame &frame)
{
    auto buf=frame.buffer.get();
    bool ret=false;
    do{
        if(buf[0]==0x00)break;
        int nalu_type=(buf[0]&0x7E)>>1;
        if(nalu_type>=16&&nalu_type<=21){
            //i
            m_last_iframe=frame;
            m_send_counts=m_frameRate*2+2;
        }
        else if (nalu_type==33) {
            if(m_frame_sps.size!=0)m_frame_sps=frame;
        }
        else if (nalu_type==34) {
            if(m_frame_pps.size!=0)m_frame_pps=frame;
        }
        else if (nalu_type==32) {
            if(m_frame_vps.size!=0)m_frame_vps=frame;
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

bool h265_source::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    if(!check_frames(frame))return true;
    uint8_t *frameBuf  = frame.buffer.get();
    uint32_t frameSize = frame.size;
    if(frame.timestamp == 0)
        frame.timestamp = getTimeStamp();

    if (frameSize <= MAX_RTP_PAYLOAD_SIZE)
    {
        RtpPacket rtpPkt;
        rtpPkt.type = frame.type;
        rtpPkt.timestamp = frame.timestamp;
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
            rtpPkt.timestamp = frame.timestamp;
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
            rtpPkt.timestamp = frame.timestamp;
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

    return true;
}


uint32_t h265_source::getTimeStamp()
{
    auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
    return (uint32_t)((timePoint.time_since_epoch().count() + 500) / 1000 * 90);
}
