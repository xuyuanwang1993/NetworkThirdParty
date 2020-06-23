#include "rtsp_message.h"
#include"algorithm"
using namespace micagent;
rtsp_message::rtsp_message(bool is_send):buffer_handle (1000),m_is_send(is_send),m_buf_cache(new BufferPacket(MAX_PACKET_SIZE)),m_packet_type(NONE_PACKET),\
    m_rtcp_len(0){

}
bool rtsp_message::insert_packet(const char *buf,uint32_t buf_len)
{
    bool ret=true;
    do{
        lock_guard<mutex>locker(m_mutex);
        if(!m_is_send){
            if(m_buf_cache->filled_size()+buf_len>MAX_PACKET_SIZE){
                MICAGENT_LOG(LOG_ERROR,"packet overflow the packet cache!");
                m_buf_cache.reset();
                ret=false;
                break;
            }
            m_buf_cache->append(buf,buf_len);
            while(1){
                auto read_len=m_buf_cache->filled_size();
                if(m_rtcp_len!=0){
                    if(read_len<m_rtcp_len)break;
                    m_buf_cache->retrieve(m_rtcp_len);
                    m_rtcp_len=0;
                }
                //check size
                read_len=m_buf_cache->filled_size();
                if(read_len<4)break;
                auto read_ptr=m_buf_cache->read_ptr();
                //check rtcp
                if(read_ptr[0]=='$'){
                    m_rtcp_len=(read_ptr[2]<<8 | read_ptr[3])+4;
                    continue;
                }
                else {
                    //search "\r\n\r\n"
                    auto ret=search(read_ptr,read_ptr+read_len,m_crlf,m_crlf+m_crlf_len);
                    if(ret==read_ptr+read_len)break;
                    auto rtsp_message_len=ret-read_ptr+m_crlf_len;
                    BufferPacket buffer(rtsp_message_len);
                    buffer.append(m_buf_cache->read_ptr(),rtsp_message_len);
                    buffer.set_finished();
                    m_buf_cache->retrieve(rtsp_message_len);
                    m_packet_list.push_back(move(buffer));
                }
            }
        }
        else {
            m_packet_list.push_back(BufferPacket(buf_len));
            m_packet_list.rbegin()->append(buf,buf_len);
            rbegin(m_packet_list)->set_finished();
            if(m_packet_list.size()>MAX_TCP_MSS_CACHE){
                for(auto iter=m_packet_list.begin();iter!=m_packet_list.end();){
                    if(iter->read_ptr()[0]=='$')m_packet_list.erase(iter++);
                    else {
                        iter++;
                    }
                }
            }
        }
    }while(0);
    return ret;
}
