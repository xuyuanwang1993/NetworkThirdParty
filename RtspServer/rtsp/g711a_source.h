#ifndef G711A_SOURCE_H
#define G711A_SOURCE_H
#include "media_source.h"
#include "rtp.h"

namespace micagent
{
using namespace std;
class g711a_source : public media_source
{
public:
    static shared_ptr<g711a_source> createNew();
    virtual ~g711a_source();

    uint32_t getSampleRate() const
    { return m_sampleRate; }

    string get_media_name()const{return "g711a_source";}

    uint32_t getChannels() const
    { return m_channels; }

    // SDP媒体描述 m=
    virtual std::string getMediaDescription(uint16_t port=0);

    // SDP属性 a=
    virtual std::string getAttribute();

    bool handleFrame(MediaChannelId channelId, AVFrame frame);

     uint32_t getTimeStamp(int64_t micro_time_now=0);
     g711a_source();
private:


    uint32_t m_sampleRate = 8000;   // 采样频率
    uint32_t m_channels = 1;        // 通道数
};

}
#endif // G711A_SOURCE_H
