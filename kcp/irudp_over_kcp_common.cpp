#include "irudp_over_kcp_common.h"
using namespace micagent;
irudp_over_kcp_connection_base::irudp_over_kcp_connection_base(uint32_t conv_id, uint32_t src_key, const Irudp_kcp_param_s&param):m_conv_id(conv_id),m_src_key(src_key),m_kcp(nullptr)
{
    m_kcp=ikcp_create(conv_id,this);
    ikcp_wndsize(m_kcp,param.window_size,param.window_size);
    ikcp_nodelay(m_kcp,param.nodelay,param.interval,param.resend,param.nc);
}
irudp_over_kcp_connection_base::~irudp_over_kcp_connection_base()
{
    if(m_kcp)ikcp_release(m_kcp);
}
