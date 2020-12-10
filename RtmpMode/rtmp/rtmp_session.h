#ifndef RTMP_SESSION_H
#define RTMP_SESSION_H
#include "rtmp_common.h"
namespace micagent {
class rtmp_sink;
class rtmp_source;
using namespace micagent;
class rtmp_session:public enable_shared_from_this<rtmp_session>{
public:
    rtmp_session();
    ~rtmp_session();
private:
    mutex m_mutex;
    map<uint32_t,shared_ptr<rtmp_source>>m_rtmp_sources;
    map<string,weak_ptr<rtmp_sink>>m_rtmp_sinks;

};
}
#endif // RTMP_SESSION_H
