#ifndef PROXY_TEST_HELPER_H
#define PROXY_TEST_HELPER_H
#include "proxy_protocol.h"
#include"c_log.h"
namespace micagent {
using namespace std;
class proxy_test_helper{
public:
    static bool tcp_send_callback(ProxyInterface *recv,const void *,uint32_t buf_len);
    static bool udp_send_callback(ProxyInterface *recv,const void *,uint32_t buf_len);
    static void frame_output_callback(ProxyInterface *recv,shared_ptr<ProxyFrame>frame);
    static shared_ptr<ProxyFrame> get_264_frame();
    static shared_ptr<ProxyFrame>get_265_frame();
    static shared_ptr<ProxyFrame>get_other_frame();
    static inline void dump_frame_info(shared_ptr<ProxyFrame> frame){
        MICAGENT_LOG(LOG_INFO,"frame info,%d %d %d %u  timestamp:%u",frame->media_channel,frame->stream_token,frame->media_type,frame->data_len,frame->timestamp);
    }
};
}
#endif // PROXY_TEST_HELPER_H
