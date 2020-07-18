#ifndef MEDIA_SOURCE_H
#define MEDIA_SOURCE_H
#include "media.h"
#include "rtp.h"
#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <map>
#include<cstring>
#include "c_log.h"
namespace micagent {
using namespace std;
class media_source
{
public:
    typedef std::function<bool (MediaChannelId channelId, RtpPacket pkt)> SendFrameCallback;

    media_source():m_sendFrameCallback(nullptr){}
    virtual ~media_source();

    virtual MediaType getMediaType() const
    { return m_mediaType; }

    virtual string get_media_name()const{return "media_source";}

    virtual string get_proxy_param()const{return "none";}

    virtual uint32_t getFrameRate() const{return 0;}
    // SDP媒体描述 m=
    virtual std::string getMediaDescription(uint16_t port=0) = 0;

    // SDP媒体属性 a=
    virtual std::string getAttribute()  = 0;

    virtual bool handleFrame(MediaChannelId channelId, AVFrame frame) = 0;

    virtual uint32_t getTimeStamp(int64_t micro_time_now=0)=0;

    virtual void setSendFrameCallback(const SendFrameCallback &cb)
    { m_sendFrameCallback = cb; }

    virtual uint32_t getPayloadType() const
    { return m_payload; }

    virtual uint32_t getClockRate() const
    { return m_clockRate; }
    virtual void setFrameRate(uint32_t frameRate){(void)frameRate;}
protected:
    MediaType m_mediaType = NONE;
    uint32_t m_payload = 0;
    uint32_t m_clockRate = 0;
    SendFrameCallback m_sendFrameCallback;
};
}
#endif // MEDIA_SOURCE_H
