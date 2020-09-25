#include "http_send_buf.h"
using namespace micagent;
http_send_buf::http_send_buf(uint32_t capacity):buffer_handle (capacity,nullptr,true,4096)
{

}

bool http_send_buf::insert_packet(const char *buf,uint32_t buf_len)
{
    if(buf_len>MAX_PACKET_SIZE)throw "buffer_handle insert_packet over flow!";
    lock_guard<mutex>locker(m_mutex);
    if(m_packet_list.size()>m_capacity||buf_len==0)return false;
    if(m_packet_list.empty()||(m_packet_list.rbegin())->finished())m_packet_list.push_back(BufferPacket(buf_len));
    auto iter=m_packet_list.rbegin();
    auto ret=iter->append(buf,buf_len);
    if(ret)iter->set_finished();
    return ret;
}
