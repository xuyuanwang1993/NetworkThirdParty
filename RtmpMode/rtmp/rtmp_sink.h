#ifndef RTMP_SINK_H
#define RTMP_SINK_H
#include "rtmp_common.h"
namespace micagent {
using namespace micagent;
/**
 * @brief The rtmp_sink class sink for rtmp  data
 */
class rtmp_sink:public enable_shared_from_this<rtmp_sink>{
public:
    rtmp_sink();
    virtual ~rtmp_sink();
    string get_key()const{return  m_special_name;}
protected:
    virtual bool is_playing()const{return  m_is_playing&&!m_is_closed;}
    virtual void start_playing(){if(!m_is_closed){m_is_playing=true;}}
    virtual void pause(){if(!m_is_closed){m_is_playing=false;}}
    virtual void stop(){m_is_closed=true;m_is_playing=false;}
    virtual string get_base_name()const{ return "rtmp_sink";}
    virtual bool handle_rtmp_message(const rtmp_message_packet& packet)=0;
protected:
    string m_special_name;
    bool m_is_playing;
    bool m_is_closed;
};
class rtmp_connection;
class rtmp_connection_sink:public rtmp_sink{
public:
    rtmp_connection_sink(rtmp_connection *conn);
     virtual ~rtmp_connection_sink()override;
};
}
#endif // RTMP_SINK_H
