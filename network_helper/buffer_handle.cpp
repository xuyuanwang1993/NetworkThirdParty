#include "buffer_handle.h"
#include <algorithm>
namespace micagent {
buffer_handle::buffer_handle(uint32_t capacity,ReadBufferCallback callback,bool is_stream,uint32_t per_read_size):\
    m_callback(callback),m_per_read_size(per_read_size),m_capacity(capacity),m_is_stream(is_stream)
{
    if(m_capacity<MIN_BUFFER_CAPACITY)m_capacity=MIN_BUFFER_CAPACITY;
    if(m_capacity>MAX_BUFFER_CAPACITY)m_capacity=MAX_BUFFER_CAPACITY;
}
int buffer_handle::read_fd(SOCKET fd)
{
    shared_ptr<char> buf(new char[m_per_read_size]);
    int len=0;
    if(m_is_stream){
        len=recv(fd,buf.get(),m_per_read_size,0);
    }
    else {
        MAKE_ADDR(addr,"0.0.0.0",0);
        socklen_t add_len=sizeof (addr);
        len=recvfrom(fd,buf.get(),m_per_read_size,0,(sockaddr *)&addr,&add_len);
    }
    if(len>0){
        if(m_callback)m_callback(buf.get(),len);
        else {
            insert_packet(buf.get(),len);
        }
    }
    return len;
}
int buffer_handle::send_fd(SOCKET fd,sockaddr_in *addr,int timeout)
{
    if(timeout > 0)Network_Util::Instance().make_blocking(fd,timeout);
    int ret = 0;
    bool success=true;
    lock_guard<mutex>locker(m_mutex);
    do
    {
        if (m_packet_list.empty()||!begin(m_packet_list)->finished())break;
        success =false;
        auto packet_iter=begin(m_packet_list);
        if(!addr){
            ret=::send(fd,packet_iter->read_ptr(),packet_iter->filled_size(),0);
        }
        else {
            ret=sendto(fd,packet_iter->read_ptr(),packet_iter->filled_size(),0,(sockaddr *)addr,sizeof (sockaddr_in));
        }
        if (ret > 0)
        {
            if(!m_is_stream)m_packet_list.pop_front();
            else {
                packet_iter->retrieve(ret);
                if(packet_iter->filled_size()==0){
                    success=true;
                    if(m_packet_list.size()!=1)m_packet_list.pop_front();
                }
            }
        }
        else if (ret < 0)
        {
#if defined(__linux) || defined(__linux__)
            if (errno == EINTR || errno == EAGAIN)
#elif defined(WIN32) || defined(_WIN32)
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK || error == WSAEINPROGRESS || error == 0)
#endif
                ret = 0;
        }
    } while (success);

    if(timeout > 0)
        Network_Util::Instance().make_noblocking(fd);

    return ret;
}
bool buffer_handle::append(const char *buf ,uint32_t buf_len)
{
    return insert_packet(buf,buf_len);
}
uint32_t buffer_handle::get_packet_nums()const
{
    lock_guard<mutex>locker(m_mutex);
    return m_packet_list.size();
}
uint32_t buffer_handle::get_first_packet_size()const
{
    lock_guard<mutex>locker(m_mutex);
    auto begin_iter=begin(m_packet_list);
    if(begin_iter==end(m_packet_list)||!begin_iter->finished())return 0;
    return begin_iter->filled_size();
}
uint32_t buffer_handle::read_packet(char *save_buf,uint32_t buf_len)
{
    lock_guard<mutex>locker(m_mutex);
    auto read_iter=begin(m_packet_list);
    if(read_iter==end(m_packet_list)||!read_iter->finished())return 0;
    auto read_len=read_iter->read_packet(save_buf,buf_len);
    if(m_packet_list.size()>1||!m_is_stream)m_packet_list.pop_front();
    return read_len;
}
bool buffer_handle::insert_packet(const char *buf,uint32_t buf_len)
{
    if(buf_len>MAX_PACKET_SIZE)throw "buffer_handle insert_packet over flow!";
    lock_guard<mutex>locker(m_mutex);
    if(m_packet_list.size()>m_capacity||buf_len==0)return false;
    if(m_packet_list.empty()||rbegin(m_packet_list)->finished())m_packet_list.push_back(BufferPacket(buf_len));
    if(!m_is_stream){
        auto iter=rbegin(m_packet_list);
        iter->set_finished();
        return iter->append(buf,buf_len);
    }
    else {
        auto iter=rbegin(m_packet_list);
        //TCP粘包分包，\r\n\r\n做分隔符
        auto filled_len=iter->filled_size();
        //处理临界状态
        if(filled_len>0){
            uint32_t left_len=filled_len<4?filled_len:3;
            uint32_t right_len=buf_len>3?3:buf_len;
            shared_ptr<char>first_check(new char[left_len+right_len]);
            memcpy(first_check.get(),iter->read_ptr()+filled_len-left_len,left_len);
            memcpy(first_check.get()+left_len,buf,right_len);
            auto ret=search(first_check.get(),first_check.get()+left_len+right_len,m_crlf,m_crlf+m_crlf_len);
            auto copy_len=ret-first_check.get()+m_crlf_len-left_len;
            if(copy_len<4&&copy_len>0){

                iter->append(buf,copy_len);
                buf+=copy_len;
                buf_len-=copy_len;
                iter->set_finished();
                m_packet_list.push_back(BufferPacket(buf_len));
            }
        }
        //处理中间输入
        while(buf_len>m_crlf_len){
            const char *tmp=search(buf,buf+buf_len,m_crlf,m_crlf+m_crlf_len);
            if(tmp!=buf+buf_len){
                m_packet_list.rbegin()->set_finished();
                m_packet_list.rbegin()->append(buf,tmp-buf+m_crlf_len);
                buf_len=buf_len-(tmp-buf+m_crlf_len);
                buf=tmp+m_crlf_len;
                m_packet_list.push_back(BufferPacket(buf_len));
            }
            else {
                break;
            }
        }
        //处理剩余packet的数据
        if(buf_len>0){
            m_packet_list.rbegin()->append(buf,buf_len);
        }
    }
    return true;
}
}
