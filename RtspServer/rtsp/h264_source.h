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
    bool handleGopCache(MediaChannelId channelid,shared_ptr<rtp_connection>connection);
     uint32_t getTimeStamp(int64_t micro_time_now=0);

     static media_frame_type get_frame_type(uint8_t byte);
     h264_source(uint32_t frameRate=30);
private:
     bool check_frames(media_frame_type type, AVFrame frame);

    shared_ptr<AVFrame> m_frame_sps;
    shared_ptr<AVFrame> m_frame_pps;
    shared_ptr<AVFrame> m_last_sei;
    shared_ptr<AVFrame> m_last_iframe;
    uint32_t m_frameRate = 25;
    uint32_t m_send_counts;
};

}
#endif // H264_SOURCE_H
