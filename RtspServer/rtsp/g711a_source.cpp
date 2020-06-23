#if defined(WIN32) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "g711a_source.h"
#include <cstdio>
#include <chrono>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

using namespace micagent;
using namespace std;

g711a_source::g711a_source()
{
    m_payload = 8;
    m_mediaType = PCMA;
    m_clockRate = 8000;
}

shared_ptr<g711a_source> g711a_source::createNew()
{
    return make_shared<g711a_source>();
}

g711a_source::~g711a_source()
{

}

string g711a_source::getMediaDescription(uint16_t port)
{
    char buf[100] = {0};
    sprintf(buf, "m=audio %hu RTP/AVP 8", port);

    return string(buf);
}

string g711a_source::getAttribute()
{
    return string("a=rtpmap:8 PCMA/8000/1");
}

bool g711a_source::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    if (frame.size > MAX_RTP_PAYLOAD_SIZE)
    {
        return false;
    }

    uint8_t *frameBuf  = frame.buffer.get();
    uint32_t frameSize = frame.size;

    RtpPacket rtpPkt;
    rtpPkt.type = frame.type;
    rtpPkt.timestamp = frame.timestamp;
    rtpPkt.size = frameSize + 4 + RTP_HEADER_SIZE;
    rtpPkt.last = 1;

    memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE, frameBuf, frameSize);

    if(m_sendFrameCallback)
        return m_sendFrameCallback(channelId, rtpPkt);

    return true;
}

uint32_t g711a_source::getTimeStamp()
{
    auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
    return (uint32_t)((timePoint.time_since_epoch().count()+500)/1000*8);
}

