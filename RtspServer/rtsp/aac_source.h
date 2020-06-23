#ifndef AAC_SOURCE_H
#define AAC_SOURCE_H
#include "media_source.h"
#include "rtp.h"

namespace micagent
{
using namespace std;
class aac_source : public media_source
{
public:
    static shared_ptr<aac_source> createNew(uint32_t sampleRate=44100, uint32_t channels=2, bool hasADTS=true);
    virtual ~aac_source();

    uint32_t getSampleRate() const
    { return m_sampleRate; }

    uint32_t getChannels() const
    { return m_channels; }

    string get_media_name()const{return "aac_source";}

    string get_proxy_param()const{
    char buf[32]={0};
    snprintf(buf,31,"%u:%u:%d",m_sampleRate,m_channels,m_hasADTS);
    return buf;}
    // SDP媒体描述 m=
    virtual std::string getMediaDescription(uint16_t port=0);

    // SDP属性 a=
    virtual std::string getAttribute();

    bool handleFrame(MediaChannelId channelId, AVFrame frame);

     uint32_t getTimeStamp();
     aac_source(uint32_t sampleRate=44100, uint32_t channels=2, bool hasADTS=true);
private:
    uint32_t m_sampleRate = 44100;   // 采样频率
    uint32_t m_channels = 2;         // 通道数
    bool m_hasADTS = true;
};

}
#endif // AAC_SOURCE_H
