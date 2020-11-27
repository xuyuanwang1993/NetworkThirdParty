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
#include <queue>
#include "rtp_connection.h"
namespace micagent {
using namespace std;
enum media_frame_type{
    FRAME_I,
    FRAME_VPS,
    FRAME_SPS,
    FRAME_PPS,
    FRAME_SEI,
    FRAME_P,
    FRAME_B,
    FRAME_AUDIO,
    FRAME_GOP_BEGIN,
    FRAME_UNKNOWN
};
class media_source
{
public:
    typedef std::function<bool (MediaChannelId channelId, RtpPacket pkt)> SendFrameCallback;

    media_source():m_sendFrameCallback(nullptr),m_last_mill_recv_time(0){
        auto timePoint = chrono::time_point_cast<chrono::milliseconds>(chrono::steady_clock::now());
        m_last_mill_send_time=timePoint.time_since_epoch().count();
    }
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
    //a=fmtp
    virtual std::string getAttributeFmtp(){return string("");}

    virtual bool handleFrame(MediaChannelId channelId, AVFrame frame) = 0;
    virtual bool handleGopCache(MediaChannelId /*channelid*/,shared_ptr<rtp_connection>/*connection*/){return false;}
    virtual uint32_t getTimeStamp(int64_t mill_second=0)=0;

    virtual void setSendFrameCallback(const SendFrameCallback &cb)
    { m_sendFrameCallback = cb; }

    virtual uint32_t getPayloadType() const
    { return m_payload; }

    virtual uint32_t getClockRate() const
    { return m_clockRate; }
    virtual void setFrameRate(uint32_t frameRate){(void)frameRate;}
    int64_t getLastSendTime()const{return m_last_mill_send_time;}

    static uint32_t removeH264or5EmulationBytes(shared_ptr<uint8_t>to_ptr, uint32_t toMaxSize,
                                                const shared_ptr<uint8_t>from_ptr, uint32_t fromSize) {
        unsigned toSize = 0;
        unsigned i = 0;
        auto from=from_ptr.get();
        auto to=to_ptr.get();
        while (i < fromSize && toSize+1 < toMaxSize) {
            if (i+2 < fromSize && from[i] == 0 && from[i+1] == 0 && from[i+2] == 3) {
                to[toSize] = to[toSize+1] = 0;
                toSize += 2;
                i += 3;
            } else {
                to[toSize] = from[i];
                toSize += 1;
                i += 1;
            }
        }
        return toSize;
    }
    static uint32_t find_next_video_nal_pos(const uint8_t *buf,uint32_t buf_len,uint32_t now_pos);
protected:
    MediaType m_mediaType = NONE;
    uint32_t m_payload = 0;
    uint32_t m_clockRate = 0;
    SendFrameCallback m_sendFrameCallback;
    int64_t m_last_mill_send_time;
    int64_t m_last_mill_recv_time;
};
}
#endif // MEDIA_SOURCE_H
