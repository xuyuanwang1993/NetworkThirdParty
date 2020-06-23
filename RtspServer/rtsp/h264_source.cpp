#if defined(WIN32) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "h264_source.h"
#include <cstdio>
#include <chrono>
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
bool h264_source::check_frames(AVFrame &frame)
{
    return true;
    auto buf=frame.buffer.get();
    bool ret=false;
    do{
        if(buf[0]==0x00)break;
        int nalu_type=buf[0]&0x1f;
        if(nalu_type==5){
            //i
            m_last_iframe=frame;
            m_send_counts=m_frameRate*2+2;
        }
        else if (nalu_type==7) {
            if(m_frame_sps.size!=0)m_frame_sps=frame;
        }
        else if (nalu_type==8) {
            if(m_frame_pps.size!=0)m_frame_pps=frame;
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

bool h264_source::handleFrame(MediaChannelId channelId, AVFrame frame)
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
            rtpPkt.timestamp = frame.timestamp;
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
            rtpPkt.timestamp = frame.timestamp;
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

uint32_t h264_source::getTimeStamp()
{
    auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
    return (uint32_t)((timePoint.time_since_epoch().count() + 500) / 1000 * 90 );
}
