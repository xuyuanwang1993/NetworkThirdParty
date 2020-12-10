#include "rtmp_message.h"
#include "rtmp_connection.h"
#include <random>
using namespace micagent;
rtmp_message::rtmp_message(rtmp_connection *_rtmp_connection, RTMP_HANDSHAKE_STATUS status):m_rtmp_connection(_rtmp_connection)
{
    m_shake_info.handshake_status=status;
}
rtmp_message::~rtmp_message()
{

}
bool rtmp_message::is_handshake_complete()const
{
    return  m_shake_info.handshake_status==RTMP_HANDSHAKE_COMPLETE;
}
pair<const uint8_t *, uint32_t> rtmp_message::get_handshake_packet()
{
    do{
        if (m_shake_info.handshake_status==RTMP_RECV_C0C1||m_shake_info.handshake_status==RTMP_WAIT_S0S1) {
            //first send
            if(m_shake_info.send_buf)break;
            m_shake_info.send_buf.reset(new uint8_t[handshake_info::buf_len+1],default_delete<uint8_t[]>());
            auto buf=m_shake_info.send_buf.get();
            memset(buf,0,handshake_info::buf_len+1);
            uint32_t start_pos=0;
            buf[start_pos]=RTMP_VERSION;
            auto timestamp=static_cast<uint32_t>(Timer::getTimeNow());
            buf[start_pos++]=(timestamp>>24)&0xff;
            buf[start_pos++]=(timestamp>>16)&0xff;
            buf[start_pos++]=(timestamp>>8)&0xff;
            buf[start_pos++]=(timestamp)&0xff;
            start_pos+=4;
            random_device rd;
            for(uint32_t pos=start_pos;pos<1537;pos++)
            {
                buf[pos]=rd()&0xff;
            }
            if(m_shake_info.handshake_status==RTMP_WAIT_S0S1){
                return  make_pair(buf,1537);//C0C1
            }
            start_pos+=1528;
            buf[start_pos++]=(timestamp>>24)&0xff;
            buf[start_pos++]=(timestamp>>16)&0xff;
            buf[start_pos++]=(timestamp>>8)&0xff;
            buf[start_pos++]=(timestamp)&0xff;
            memcpy(buf+start_pos,m_shake_info.recv_buf.get()+1,4);//skip version
            start_pos+=4;
            memcpy(buf+start_pos,m_shake_info.recv_buf.get()+8,1526);//copy C1
            return  make_pair(buf,handshake_info::buf_len);//S0S1S2
        }
        else if (m_shake_info.handshake_status==RTMP_RECV_S0S1S2) {
            //set status to complete
            auto buf=m_shake_info.send_buf.get();
            uint32_t start_pos=1537;
            auto timestamp=static_cast<uint32_t>(Timer::getTimeNow());
            buf[start_pos++]=(timestamp>>24)&0xff;
            buf[start_pos++]=(timestamp>>16)&0xff;
            buf[start_pos++]=(timestamp>>8)&0xff;
            buf[start_pos++]=(timestamp)&0xff;
            memcpy(buf+start_pos,m_shake_info.recv_buf.get()+1,4);
            start_pos+=4;
            memcpy(buf+start_pos,m_shake_info.recv_buf.get()+8,1526);//copy S1
            m_shake_info.handshake_status=RTMP_HANDSHAKE_COMPLETE;
            return make_pair(buf+1537,1536);//C2
        }
    }while(0);
    return  make_pair(nullptr,0);
}
bool rtmp_message::handle_read()
{
    shared_ptr<uint8_t>buf(new uint8_t[4096],default_delete<uint8_t[]>());
    auto read_len=::recv(m_rtmp_connection->fd(),buf.get(),4096,0);
    bool ret=false;
    do{
        if(read_len==0)break;
        if(read_len<0)return  NETWORK.check_socket_io_error_status();
        //handle data
        if(!m_recv_cache->append(buf.get(),static_cast<uint32_t>(read_len)))break;
        ret=handle_data();
    }while(0);
    return  ret;
}
bool rtmp_message::handle_write()
{
    bool ret=true;
    while(m_send_cache->filled_size()>0){
        auto send_size=m_send_cache->filled_size();
        if(send_size>1500)send_size=1500;
        auto send_len=::send(m_rtmp_connection->fd(),m_send_cache->read_ptr(),send_size,0);
        if(send_len<=0){
            ret=NETWORK.check_socket_io_error_status();
            break;
        }
        m_send_cache->retrieve(static_cast<uint32_t>(send_len));
    }
    if(m_send_cache->filled_size()==0){
        m_rtmp_connection->disable_write();
    }
    return ret;
}
bool rtmp_message::handle_data()
{

}
bool rtmp_message::handle_rtmp_chunk(shared_ptr<rtmp_message_packet>packet)
{
}
