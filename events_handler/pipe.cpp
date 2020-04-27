#include "pipe.h"
namespace micagent {
Pipe::Pipe(){

}
bool Pipe::open()
{
#if defined(WIN32) || defined(_WIN32)
    //鉴于socket是一个双向管道，故此例从写管道写入到自身的读管道
    if(m_fd== INVALID_SOCKET){
        m_fd=Network_Util::Instance().build_socket(UDP);
        Network_Util::Instance().bind(m_fd);
        Network_Util::Instance().connect(m_fd, "127.0.0.1", Network_Util::Instance().get_local_port(m_fd));
        Network_Util::Instance().make_noblocking(m_fd);
    }
    return m_fd!= INVALID_SOCKET;
#elif defined(__linux) || defined(__linux__)
    if (pipe2(m_fd, O_NONBLOCK | O_CLOEXEC) < 0)
    {
        return false;
    }
    return true;
#endif
}
void Pipe::close()
{
#if defined(WIN32) || defined(_WIN32)
    if(m_fd!= INVALID_SOCKET)Network_Util::Instance().close_socket(m_fd);
    m_fd= INVALID_SOCKET;
#elif defined(__linux) || defined(__linux__)
    if(m_fd[0]!= INVALID_SOCKET)Network_Util::Instance().close_socket(m_fd[0]);
    if(m_fd[1]!= INVALID_SOCKET)Network_Util::Instance().close_socket(m_fd[1]);
    m_fd[0]=INVALID_SOCKET;
    m_fd[1]=INVALID_SOCKET;
#endif
}
int64_t Pipe::read(uint8_t *buf,uint32_t max_len)
{
#if defined(WIN32) || defined(_WIN32) 
    if(m_fd== INVALID_SOCKET)return  -1;
    return recv(m_fd, (char*)buf, max_len, 0);
#elif defined(__linux) || defined(__linux__)
    if(m_fd[0]==INVALID_SOCKET) return -1;
    return ::read(m_fd[0], buf, max_len);
#endif 
}
int64_t Pipe::write(const uint8_t *buf, uint32_t buf_len)
{
#if defined(WIN32) || defined(_WIN32) 
    if(m_fd== INVALID_SOCKET)return  -1;
    return ::send(m_fd, (char*)buf, buf_len, 0);
#elif defined(__linux) || defined(__linux__) 
    if(m_fd[1]==INVALID_SOCKET) return -1;
    return ::write(m_fd[1], buf, buf_len);
#endif
}
Pipe::~Pipe()
{
    close();
}
}
