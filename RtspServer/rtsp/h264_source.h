#ifndef H264_SOURCE_H
#define H264_SOURCE_H
#include "media_source.h"
#include "rtp.h"
namespace micagent
{

class h264_source : public media_source
{
public:
    static shared_ptr<h264_source> createNew(uint32_t frameRate=30);
    ~h264_source();

    void setFrameRate(uint32_t frameRate)
    { m_frameRate = frameRate; }

    uint32_t getFrameRate() const
    { return m_frameRate; }

    string get_media_name()const{return "h264_source";}

    // SDP媒体描述 m=
    virtual std::string getMediaDescription(uint16_t port);

    // SDP媒体属性 a=
    virtual std::string getAttribute();

    bool handleFrame(MediaChannelId channelId, AVFrame frame);

     uint32_t getTimeStamp(int64_t micro_time_now=0);
     h264_source(uint32_t frameRate=30);
private:
     bool check_frames(AVFrame &frame);

    AVFrame m_frame_sps;
    AVFrame m_frame_pps;
    AVFrame m_last_iframe;
    uint32_t m_frameRate = 25;
    uint32_t m_send_counts;
};

}
#endif // H264_SOURCE_H
