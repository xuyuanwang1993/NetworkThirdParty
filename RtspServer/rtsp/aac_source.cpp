﻿
#if defined(WIN32) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "aac_source.h"
#include <stdlib.h>
#include <cstdio>
#include <chrono>
#include<cstring>
#if defined(__linux) || defined(__linux__)
#include <sys/time.h>
#endif

using namespace micagent;
using namespace std;

aac_source::aac_source(uint32_t sampleRate, uint32_t channels, bool hasADTS)
    : m_sampleRate(sampleRate)
    , m_channels(channels)
    , m_hasADTS(hasADTS)
{
    m_payload = 97;
    m_mediaType = AAC;
    m_clockRate = sampleRate;
}

shared_ptr<aac_source> aac_source::createNew(uint32_t sampleRate, uint32_t channels, bool hasADTS)
{
    return make_shared<aac_source>(sampleRate, channels, hasADTS);
}

aac_source::~aac_source()
{

}

string aac_source::getMediaDescription(uint16_t port)
{
    char buf[100] = { 0 };
    sprintf(buf, "m=audio %hu RTP/AVP 97", port); // \r\nb=AS:64

    return string(buf);
}

static uint32_t AACSampleRate[16] =
{
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0 /*reserved */
};

string aac_source::getAttribute()  // RFC 3640
{
    char buf[500] = { 0 };
    sprintf(buf, "a=rtpmap:97 MPEG4-GENERIC/%u/%u\r\n", m_sampleRate, m_channels);

    uint8_t index = 0;
    for (index = 0; index < 16; index++)
    {
        if (AACSampleRate[index] == m_sampleRate)
            break;
    }
    if (index == 16)
        return ""; // error

    uint8_t profile = 1;
    char configStr[10] = {0};
    sprintf(configStr, "%02x%02x", (uint8_t)((profile+1) << 3)|(index >> 1), (uint8_t)((index << 7)|(m_channels<< 3)));

    sprintf(buf+strlen(buf),
            "a=fmtp:97 profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizelength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
             atoi(configStr));

    return string(buf);
}

#define ADTS_SIZE 7
#define AU_SIZE 4
bool aac_source::handleFrame(MediaChannelId channelId, AVFrame frame)
{
    if (frame.size > (MAX_RTP_PAYLOAD_SIZE-AU_SIZE))
    {
        return false;
    }

    int adtsSize = 0;
    if (m_hasADTS)
    {
        adtsSize = ADTS_SIZE;
    }

    uint8_t *frameBuf = frame.buffer.get() + adtsSize; // 打包RTP去掉ADTS头
    uint32_t frameSize = frame.size - adtsSize;

    char AU[AU_SIZE] = { 0 };
    AU[0] = 0x00;
    AU[1] = 0x10;
    AU[2] = (frameSize & 0x1fe0) >> 5;
    AU[3] = (frameSize & 0x1f) << 3;

    RtpPacket rtpPkt;
    rtpPkt.type = frame.type;
    rtpPkt.timestamp = frame.timestamp;
    rtpPkt.size = frameSize + 4 + RTP_HEADER_SIZE + AU_SIZE;
    rtpPkt.last = 1;

    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 0] = AU[0];
    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 1] = AU[1];
    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 2] = AU[2];
    rtpPkt.data.get()[4 + RTP_HEADER_SIZE + 3] = AU[3];

    memcpy(rtpPkt.data.get()+4+RTP_HEADER_SIZE+AU_SIZE, frameBuf, frameSize);

    if(m_sendFrameCallback)
        return m_sendFrameCallback(channelId, rtpPkt);

    return true;
}

uint32_t aac_source::getTimeStamp()
{
    auto timePoint = chrono::time_point_cast<chrono::microseconds>(chrono::steady_clock::now());
    return (uint32_t)((timePoint.time_since_epoch().count()+500) / 1000 * m_sampleRate / 1000);
}
