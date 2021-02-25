#pragma once
#ifndef IO_CHANNEL_H
#define IO_CHANNEL_H
#include "network_util.h"
#include<functional>
#include<memory>
#include "c_log.h"
namespace micagent {
using namespace std;
enum EventType
{
    EVENT_NONE   = 0,
    EVENT_IN     = 1,
    EVENT_PRI    = 2,
    EVENT_OUT    = 4,
    EVENT_ERR    = 8,
    EVENT_HUP    = 16,
    EVENT_RDHUP  = 8192
};
class Channel;
//在读写的回调函数中 如主动调用了错误处理回调函数，应返回false，this指针为当前channel指针
using EventCallBack=function<bool(Channel *)>;
using ChannelPtr=shared_ptr<Channel>;
using ChannelPtrW=weak_ptr<Channel>;
class Channel
{
public:
    Channel() =delete ;
    Channel(SOCKET _fd):m_cycle_flag(false),m_fd(_fd),m_events(0),m_readCallback(nullptr),\
        m_writeCallback(nullptr),m_errorCallback(nullptr),m_closeCallback(nullptr){
        Network_Util::Instance().make_noblocking(m_fd);
       // MICAGENT_BACKTRACE("%s %hu %hu\r\n",NETWORK.get_peer_ip(m_fd).c_str(),NETWORK.get_peer_port(m_fd),NETWORK.get_local_port(m_fd));
    }
    ~Channel(){
        if(!m_cycle_flag&&m_fd!=INVALID_SOCKET){
            //MICAGENT_BACKTRACE("%s %hu %hu\r\n",NETWORK.get_peer_ip(m_fd).c_str(),NETWORK.get_peer_port(m_fd),NETWORK.get_local_port(m_fd));
            Network_Util::Instance().close_socket(m_fd);
        }
    }
    void setReadCallback(const EventCallBack& cb)
    { m_readCallback = cb; }
    void setWriteCallback(const EventCallBack& cb)
    { m_writeCallback = cb; }
    void setCloseCallback(const EventCallBack& cb)
    { m_closeCallback = cb; }
    void setErrorCallback(const EventCallBack& cb)
    { m_errorCallback = cb; }

    void setReadCallback(EventCallBack&& cb)
    {m_readCallback = std::move(cb); }
    void setWriteCallback(EventCallBack&& cb)
    { m_writeCallback = std::move(cb); }
    void setCloseCallback(EventCallBack&& cb)
    { m_closeCallback = std::move(cb); }
    void setErrorCallback(EventCallBack&& cb)
    { m_errorCallback = std::move(cb); }
    /**
     * @brief fd 获取channel对应的fd
     * @return
     */
    SOCKET fd() const { return m_fd; }
    int events() const { return m_events; }
    void setEvents(int events) {
        m_events = events; }

    void enableReading()
    {
        m_events |= EVENT_IN; }

    void enableWriting()
    {
        m_events |= EVENT_OUT; }

    void disableReading()
    {
        m_events &= ~EVENT_IN; }

    void disableWriting()
    {
        m_events &= ~EVENT_OUT; }

    bool isNoneEvent() const {return m_events == EVENT_NONE; }
    bool isWriting() const {return (m_events & EVENT_OUT)!=0; }
    bool isReading() const {return (m_events & EVENT_IN)!=0; }

    void handleEvent(int events)
    {//读写出现异常时会调用关闭回调函数
        do{
            if ((events & (EVENT_PRI | EVENT_IN))&&m_readCallback)
            {
                if(!m_readCallback(this))break;
            }
            if ((events & EVENT_OUT)&&m_writeCallback)
            {
                if(!m_writeCallback(this))break;
            }
            if ((events & EVENT_HUP)&&m_closeCallback)
            {
                m_closeCallback(this);
                return;
            }

            if ((events & (EVENT_ERR))&&m_errorCallback)
            {
                m_errorCallback(this);
            }
            return;
        }while(0);
        if(m_closeCallback)m_closeCallback(this);
    }
    /**
     * @brief set_cycle 改变channel回收状态，若为false在channel析构时会关闭socket
     * @param state
     */
    void set_cycle(bool state){m_cycle_flag=state;}
private:
    bool m_cycle_flag;
    SOCKET m_fd;
    int m_events;
    EventCallBack m_readCallback;
    EventCallBack m_writeCallback;
    EventCallBack m_errorCallback;
    EventCallBack m_closeCallback;
};
}

#endif // IO_CHANNEL_H
