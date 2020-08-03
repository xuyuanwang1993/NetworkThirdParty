#ifndef H265_SOURCE_H
#define H265_SOURCE_H
#include "media_source.h"
#include "rtp.h"

namespace micagent
{
using namespace std;
class h265_source : public media_source
{
public:
    static shared_ptr<h265_source> createNew(uint32_t frameRate=30);
    virtual ~h265_source();

    void setFrameRate(uint32_t frameRate)
    { m_frameRate = frameRate; }

    uint32_t getFrameRate() const
    { return m_frameRate; }

    string get_media_name()const{return "h265_source";}

    // SDP媒体描述 m=
    virtual std::string getMediaDescription(uint16_t port=0);

    // SDP属性 a=
    virtual std::string getAttribute();

    bool handleFrame(MediaChannelId channelId, AVFrame frame);
    bool handleGopCache(MediaChannelId channelid,shared_ptr<rtp_connection>connection);
    static media_frame_type get_frame_type(uint8_t byte);
     uint32_t getTimeStamp(int64_t micro_time_now=0);
     h265_source(uint32_t frameRate=30);
private:
     bool check_frames(media_frame_type type,AVFrame &frame);

    shared_ptr<AVFrame> m_frame_vps;
    shared_ptr<AVFrame> m_frame_sps;
    shared_ptr<AVFrame> m_frame_pps;
    shared_ptr<AVFrame> m_last_sei;
    shared_ptr<AVFrame> m_last_iframe;
    uint32_t m_frameRate = 25;
    uint32_t m_send_counts;
};

}
#endif // H265_SOURCE_H
