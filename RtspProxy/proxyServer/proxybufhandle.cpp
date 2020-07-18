#include "proxybufhandle.h"
using namespace micagent;
proxy_message::proxy_message(bool is_send):buffer_handle (MAX_TCP_MSS_CACHE),m_is_send(is_send)
{
    if(!is_send){
        m_recv_parse.reset(new PStreamParse);
    }
}
proxy_message::~proxy_message()
{

}
bool proxy_message::insert_packet(const char *buf,uint32_t buf_len)
{
    bool ret=true;
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_packet_list.size()>MAX_TCP_MSS_CACHE)m_packet_list.clear();
        if(!m_is_send){
            auto done_packet=m_recv_parse->insert_buf(buf,buf_len);
            while(!done_packet.empty())
            {
                BufferPacket buffer(done_packet.front().second);
                    buffer.append(done_packet.front().first.get(),done_packet.front().second);
                    buffer.set_finished();
                    m_packet_list.push_back(move(buffer));
                    done_packet.pop();
            }
        }
        else {
            m_packet_list.push_back(BufferPacket(buf_len));
            m_packet_list.rbegin()->append(buf,buf_len);
           m_packet_list.rbegin()->set_finished();
        }
    }while(0);
    return ret;
}
