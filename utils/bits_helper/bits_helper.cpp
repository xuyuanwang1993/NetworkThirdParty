#include "bits_helper.h"

using namespace micagent;

bits_context::bits_context(void *bits_buf,uint32_t bits_len):m_src_buf(new uint8_t[sizeof (uint64_t)+bits_len],default_delete<uint8_t[]>()),m_buf_size(bits_len),m_last_offet(0)
{
    memcpy(m_src_buf.get(),bits_buf,bits_len);
    if(bits_len>=0x1FFFFFFF)m_max_offset=INVALID_BITS_OFFSET;
    else {
        m_max_offset=bits_len*8;
    }
}
void bits_context::reset(void *bits_buf,uint32_t bits_len)
{
    //多加8位为了安全写入
    m_src_buf.reset(new uint8_t[sizeof (uint64_t)+bits_len],default_delete<uint8_t[]>());
    memcpy(m_src_buf.get(),bits_buf,bits_len);
    m_last_offet=0;
    if(bits_len>=0x1FFFFFFF)m_max_offset=INVALID_BITS_OFFSET;
    else {
        m_max_offset=bits_len*8;
    }
}
bits_context::~bits_context()
{

}
