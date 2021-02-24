#include "unix_socket_helper.h"
#include <sys/time.h>

using namespace micagent;
/********************************unix_socket_base**********************************/
unix_socket_base::unix_socket_base(const string &path):m_fd(INVALID_SOCKET)
{
    m_path[PATH_MAX_SIZE]='\0';
    strncpy(m_path,path.c_str(),PATH_MAX_SIZE);
}
unix_socket_base::~unix_socket_base(){
    if(m_fd!=INVALID_SOCKET)NETWORK.close_socket(m_fd);
    if(m_path[0]!='\0')unlink(m_path);
}
ssize_t unix_socket_base::recv(void *buf, uint32_t max_len, uint32_t time_out_ms)
{

    if (time_out_ms > 0)
    {
        NETWORK.make_noblocking(m_fd);
        fd_set fdRead;
        FD_ZERO(&fdRead);
        FD_SET(m_fd, &fdRead);
        struct timeval tv = { static_cast<__time_t>(time_out_ms / 1000), static_cast<__suseconds_t>(time_out_ms % 1000 * 1000 )};
        select(m_fd + 1, &fdRead, nullptr, nullptr, &tv);
        ssize_t ret=-1;
        if (FD_ISSET(m_fd, &fdRead))
        {

            ret= ::read(m_fd,buf,max_len);
        }
        NETWORK.make_blocking(m_fd,time_out_ms);
        return ret;
    }
    else {
        return ::read(m_fd,buf,max_len);
    }
}
/********************************unix_stream_socket**********************************/
unix_stream_socket::unix_stream_socket(const string &path):unix_socket_base(path)
{

}
SOCKET unix_stream_socket::build(){
    m_fd=NETWORK.build_unix_socket(TCP,m_path);
    return m_fd;
}
void unix_stream_socket::reset(SOCKET fd,const string & path)
{
    if(m_path[0]!='\0')unlink(m_path);
    if(m_fd!=INVALID_SOCKET)NETWORK.close_socket(m_fd);
    m_fd=fd;
    strncpy(m_path,path.c_str(),PATH_MAX_SIZE);
}
ssize_t unix_stream_socket::send(const void *buf, uint32_t len, const string &/*peer_path*/, uint32_t time_out_ms)
{
    if (time_out_ms > 0)
    {
        NETWORK.make_noblocking(m_fd);
        fd_set fdWrite;
        FD_ZERO(&fdWrite);
        FD_SET(m_fd, &fdWrite);
        struct timeval tv = { static_cast<__time_t>(time_out_ms / 1000), static_cast<__suseconds_t>(time_out_ms % 1000 * 1000 )};
        select(m_fd + 1,  nullptr, &fdWrite,nullptr, &tv);
        ssize_t ret=-1;
        if (FD_ISSET(m_fd, &fdWrite))
        {

            ret= ::write(m_fd,buf,len);
        }
        NETWORK.make_blocking(m_fd,time_out_ms);
        return ret;
    }
    else {
        return ::write(m_fd,buf,len);
    }
}

bool unix_stream_socket::listen(int32_t size)
{
    return ::listen(m_fd,size)==0;
}
bool unix_stream_socket::connect(const string &server_path)
{
    sockaddr_un server_addr;
    server_addr.sun_family=AF_UNIX;
    strncpy(server_addr.sun_path,server_path.c_str(),PATH_MAX_SIZE);
    return ::connect(m_fd,reinterpret_cast<sockaddr *>(&server_addr),sizeof (server_addr))==0;
}
SOCKET unix_stream_socket::aacept()
{
    return ::accept(m_fd,nullptr,nullptr);
}
/********************************unix_dgram_socket**********************************/
unix_dgram_socket::unix_dgram_socket(const string &path):unix_socket_base (path),m_packet_cache_len(0)
{
    memset(&m_peer_path,'\0',sizeof (m_peer_path));
    memset(m_packet_send_cache,0,DEFAULT_PACKET_SIZE);
    memset(m_packet_recv_cache,0,DEFAULT_PACKET_SIZE);
    m_peer_path.sun_family=AF_UNIX;
}
SOCKET unix_dgram_socket::build()
{
    m_fd=NETWORK.build_unix_socket(UDP,m_path);
    return m_fd;
}
ssize_t unix_dgram_socket::send(const void *buf, uint32_t len, const string &peer_path, uint32_t time_out_ms)
{
    if (time_out_ms > 0)
    {
        NETWORK.make_noblocking(m_fd);
        fd_set fdWrite;
        FD_ZERO(&fdWrite);
        FD_SET(m_fd, &fdWrite);
        struct timeval tv = { static_cast<__time_t>(time_out_ms / 1000), static_cast<__suseconds_t>(time_out_ms % 1000 * 1000 )};
        select(m_fd + 1,  nullptr, &fdWrite,nullptr, &tv);
        ssize_t ret=-1;
        if (FD_ISSET(m_fd, &fdWrite))
        {
            if(!peer_path.empty())
            {
                sockaddr_un destination;
                destination.sun_family=AF_UNIX;
                strncpy(destination.sun_path,peer_path.c_str(),PATH_MAX_SIZE);
                ret= :: sendto(m_fd,buf,len,0,reinterpret_cast<sockaddr *>(&destination),sizeof (destination));
            }
            else {
                ret= ::sendto(m_fd,buf,len,0,reinterpret_cast<sockaddr *>(&m_peer_path),sizeof (m_peer_path));
            }
        }
        NETWORK.make_blocking(m_fd,time_out_ms);
        return ret;
    }
    else {
        if(!peer_path.empty())
        {
            sockaddr_un destination;
            destination.sun_family=AF_UNIX;
            strncpy(destination.sun_path,peer_path.c_str(),PATH_MAX_SIZE);
            return  :: sendto(m_fd,buf,len,0,reinterpret_cast<sockaddr *>(&destination),sizeof (destination));
        }
        else {
            return ::sendto(m_fd,buf,len,0,reinterpret_cast<sockaddr *>(&m_peer_path),sizeof (m_peer_path));
        }
    }
}
ssize_t unix_dgram_socket::send_packet(const void *buf,uint32_t len,uint32_t time_out_ms,const string &peer_path)
{
    ssize_t send_len=0;
    do{
        if(0==len)break;
        timeval time_now;
        gettimeofday(&time_now,nullptr);
        uint16_t time_ms=((time_now.tv_usec/1000)&0xff)+((time_now.tv_sec*1000)&0xffff);
        //fill packet_seq
        m_packet_send_cache[0]=(time_ms>>8)&0xff;
        m_packet_send_cache[1]=(time_ms)&0xff;
        while(len>0){
            //fill sn
            uint32_t target_send_len=DEFAULT_MTU_SIZE;
            if(len<DEFAULT_MTU_SIZE)target_send_len=len;
            len-=target_send_len;
            //filled left len
            m_packet_send_cache[2]=(len>>24)&0xff;
            m_packet_send_cache[3]=(len>>16)&0xff;
            m_packet_send_cache[4]=(len>>8)&0xff;
            m_packet_send_cache[5]=(len)&0xff;
            //copy data
            memcpy(m_packet_send_cache+DEFAULT_PACKET_HEADER_LEN,static_cast<const uint8_t *>(buf)+send_len,target_send_len);
            //update
            auto ret=send(m_packet_send_cache,DEFAULT_PACKET_HEADER_LEN+target_send_len,peer_path,time_out_ms);
            memset(m_packet_send_cache+2,0,DEFAULT_PACKET_SIZE-2);
            if(ret<0)break;
            send_len+=target_send_len;
        }
    }while(0);
    return send_len;

}

ssize_t unix_dgram_socket::recv_packet(void *buf,uint32_t max_len,uint32_t time_out_ms)
{
    ssize_t ret=0;
    if(m_packet_cache_len>0){
        //copy cache
        ret=m_packet_cache_len>max_len?max_len:m_packet_cache_len;
        max_len-=ret;
        memcpy(buf,m_packet_recv_cache+DEFAULT_PACKET_HEADER_LEN,static_cast<uint32_t>(ret));
        m_packet_cache_len=0;
        if(m_recv_packet_header.left_len==0)return ret;
    }
    do{
        memset(m_packet_recv_cache,0,DEFAULT_PACKET_SIZE);
        auto recv_len=recv(m_packet_recv_cache,DEFAULT_PACKET_SIZE,time_out_ms);
        if(recv_len<0||recv_len<=DEFAULT_PACKET_HEADER_LEN){
            //error
            break;
        }
        //parse
        uint16_t seq=((m_packet_recv_cache[0]<<8)+m_packet_recv_cache[1])&0xff;
        uint32_t left_len=m_packet_recv_cache[2];
        left_len=(left_len<<8)|m_packet_recv_cache[3];
        left_len=(left_len<<8)|m_packet_recv_cache[4];
        left_len=(left_len<<8)|m_packet_recv_cache[5];
        uint32_t payload_len=static_cast<uint32_t>(recv_len-DEFAULT_PACKET_HEADER_LEN);
        if(m_recv_packet_header.left_len==0)
        {//new packet
            m_recv_packet_header.packet_seq=seq;
        }
        m_recv_packet_header.left_len=left_len;
        if(m_recv_packet_header.packet_seq!=seq)
        {//recv new packet error
            m_packet_cache_len=payload_len;
            m_recv_packet_header.packet_seq=seq;
            break;
        }
        //copy data
        auto copy_len=payload_len>max_len?max_len:payload_len;
        max_len-=copy_len;
        if(copy_len>0)memcpy(static_cast<uint8_t*>(buf)+ret,m_packet_recv_cache+DEFAULT_PACKET_HEADER_LEN,payload_len);
        ret+=copy_len;
    }while(m_recv_packet_header.left_len!=0);
    return ret;
}
