#include "websocket_buffer_cache.h"
using namespace micagent;
web_socket_buffer_cache::web_socket_buffer_cache(uint32_t capacity,bool is_websocket_send):buffer_handle (capacity,nullptr,true,WEBSOCKET_PIECE_SIZE),m_is_websocket_send(is_websocket_send)\
  ,m_ws_buf_cache(new BufferPacket(MAX_PACKET_SIZE)),m_ws_payload_len(INVALID_WS_PAYLOAD_LEN),m_ws_data_miss_len(0)
{

}
bool web_socket_buffer_cache::insert_packet(const char *buf,uint32_t buf_len)
{
    bool ret=true;
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_is_websocket_send){
            m_packet_list.push_back(BufferPacket(buf_len));
            m_packet_list.rbegin()->append(buf,buf_len);
            (m_packet_list.rbegin())->set_finished();
            if(m_packet_list.size()>MAX_WS_FRAME_CACHE){
                MICAGENT_BACKTRACE("web_socket_buffer_cache overflows MAX_WS_FRAME_CACHE %u,clear cache list!",MAX_WS_FRAME_CACHE);
                m_packet_list.clear();
                ret=false;
            }
        }
        else {
            if(m_ws_buf_cache->filled_size()+buf_len>2*m_per_read_size){
                MICAGENT_BACKTRACE("web_socket_buffer_cache recv false websocket frame!clear cache!");
                m_ws_buf_cache->reset();
                reset_ws_status();
                ret=false;
                break;
            }
            m_ws_buf_cache->append(buf,buf_len);
            while(1){
                auto read_len=m_ws_buf_cache->filled_size();
                if(m_ws_payload_len!=INVALID_WS_PAYLOAD_LEN){
                    auto iter=m_packet_list.rbegin();//get a operation buf
                    //handle old ws frame
                    if(read_len>=m_ws_data_miss_len){
                        //finish a ws frame
                        iter->append(m_ws_buf_cache->read_ptr(),m_ws_data_miss_len);//copy data
                        iter->set_finished();//update application status
                        m_ws_buf_cache->retrieve(m_ws_data_miss_len);//update cache status
                        reset_ws_status();//update ws status
                        continue;
                    }
                    else {
                        //not finish a ws frame
                        iter->append(m_ws_buf_cache->read_ptr(),read_len);//copy data
                        m_ws_buf_cache->reset();//update cache status
                        m_ws_data_miss_len-=read_len;//update ws status
                        break;//finished handle
                    }
                }
                else {
                    //parse new ws frame
                    if(read_len<2)break;//below min ws header's length,finished
                    bool use_mask=m_ws_buf_cache->read_ptr()[1]&0x80;
                    uint8_t payload_len=m_ws_buf_cache->read_ptr()[1]&0x7F;
                    uint8_t header_len=2;
                    if(use_mask)header_len+=4;
                    if(payload_len==126)header_len+=2;
                    else if (payload_len==127) header_len+=8;
                    if(read_len<header_len)break;//below ws header's length, finished
                    m_ws_payload_len=payload_len;
                    m_ws_data_miss_len=0;
                    if(payload_len<126)m_ws_data_miss_len=payload_len;
                    else if (payload_len==126) {
                        m_ws_data_miss_len|=static_cast<uint32_t>(static_cast<uint8_t>(m_ws_buf_cache->read_ptr()[2])<<8);
                        m_ws_data_miss_len|=static_cast<uint32_t>(static_cast<uint8_t>(m_ws_buf_cache->read_ptr()[3])<<0);
                    }
                    else{
                        //discard too big frame
                        m_ws_data_miss_len|=static_cast<uint32_t>(static_cast<uint8_t>(m_ws_buf_cache->read_ptr()[6])<<24);
                        m_ws_data_miss_len|=static_cast<uint32_t>(static_cast<uint8_t>(m_ws_buf_cache->read_ptr()[7])<<16);
                        m_ws_data_miss_len|=static_cast<uint32_t>(static_cast<uint8_t>(m_ws_buf_cache->read_ptr()[8])<<8);
                        m_ws_data_miss_len|=static_cast<uint32_t>(static_cast<uint8_t>(m_ws_buf_cache->read_ptr()[9])<<0);
                    }
                    m_packet_list.push_back(BufferPacket(header_len+m_ws_data_miss_len));//init a new ws packet
                    auto iter=m_packet_list.rbegin();
                    iter->append(m_ws_buf_cache->read_ptr(),header_len);//copy header
                    m_ws_buf_cache->retrieve(header_len);//update cache status
                    if(0==m_ws_data_miss_len)
                    {//finish a ws frame
                        iter->set_finished();
                        reset_ws_status();
                    }
                }
            }
        }
    }while(0);
    return ret;
}
void web_socket_buffer_cache::util_test()
{
#if UTIL_TEST
    //just test recv handle
    queue<shared_ptr<WS_Frame>>m_frame_queue;
    for(uint32_t len=0;len<200000;len+=5000){
        string buf("len:");
        buf+=to_string(len)+" "+string(len,'*');
        m_frame_queue.push(make_shared<WS_Frame>(buf.c_str(),buf.length(),WS_Frame_Header::WS_TEXT_FRAME));
    }
    shared_ptr<uint8_t>recv_buf(new uint8_t[10*1024*1024],default_delete<uint8_t[]>());
    uint32_t total_len=0;
    while(!m_frame_queue.empty()){
        auto ret=web_socket_helper::WS_EncodeData(*m_frame_queue.front());
        m_frame_queue.pop();
        memcpy(recv_buf.get()+total_len,ret.first.get(),ret.second);
        total_len+=ret.second;
    }
    uint32_t handle_pos=0;
    web_socket_buffer_cache cache(1000);
    uint32_t size_piece=1;
    for(;handle_pos+size_piece<total_len;handle_pos+=size_piece){
        cache.append(recv_buf.get()+handle_pos,size_piece);
        uint32_t frame_len;
        while((frame_len=cache.get_first_packet_size())!=0){
            shared_ptr<uint8_t>frame_buf(new uint8_t[frame_len+1],default_delete<uint8_t[]>());
            auto len=cache.read_packet(frame_buf.get(),frame_len);
            if(len >0){
                auto ws_decode=web_socket_helper::WS_DecodeData(frame_buf.get(),len,false);
                if(ws_decode.data_len>0)
                MICAGENT_DEBUG("frame_len %u recv_len %u decode_info %d %d %d %d %s",frame_len,len,ws_decode.is_fin,ws_decode.use_mask,ws_decode.type,ws_decode.data_len\
                               ,string(reinterpret_cast<const char *>(ws_decode.data.get()),100).c_str());
            }
        }
    }
    if(handle_pos!=total_len){
        cache.append(recv_buf.get()+handle_pos,total_len-handle_pos);
        uint32_t frame_len;
        while((frame_len=cache.get_first_packet_size())!=0){
            shared_ptr<uint8_t>frame_buf(new uint8_t[frame_len+1],default_delete<uint8_t[]>());
            auto len=cache.read_packet(frame_buf.get(),frame_len);
            auto ws_decode=web_socket_helper::WS_DecodeData(frame_buf.get(),len);
            if(ws_decode.data_len>0)
            MICAGENT_DEBUG("frame_len %u recv_len %u decode_info %d %d %d %d %s",frame_len,len,ws_decode.is_fin,ws_decode.use_mask,ws_decode.type,ws_decode.data_len\
                           ,string(reinterpret_cast<const char *>(ws_decode.data.get()),100).c_str());
        }
    }
#endif
}
