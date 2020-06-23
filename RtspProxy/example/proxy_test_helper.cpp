#include "proxy_test_helper.h"
#include <random>
using namespace micagent;
bool proxy_test_helper::tcp_send_callback(ProxyInterface *recv,const void *buf,uint32_t buf_len)
{//tcp发送 不丢包 模拟增加发送延时,网络断开

}
bool proxy_test_helper::udp_send_callback(ProxyInterface *recv,const void *buf,uint32_t buf_len)
{

}
void proxy_test_helper::frame_output_callback(ProxyInterface *recv,shared_ptr<ProxyFrame>frame)
{

}
shared_ptr<ProxyFrame> proxy_test_helper::get_264_frame()
{

}
shared_ptr<ProxyFrame>proxy_test_helper::get_265_frame()
{

}
shared_ptr<ProxyFrame>proxy_test_helper::get_other_frame()
{

}
