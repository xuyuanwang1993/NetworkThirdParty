#pragma once
#ifndef TASK_SCHEDULER_H
#define TASK_SCHEDULER_H
#include "pipe.h"
#include "io_channel.h"
#include "c_log.h"
#include<atomic>
#include<unordered_map>
/*select */
#if defined(__linux) || defined(__linux__)
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#elif defined(WIN32) || defined(_WIN32)
#endif
/*epoll*/
#if defined(__linux) || defined(__linux__)
#include <sys/epoll.h>
#include <errno.h>
#endif
namespace micagent {
/**
 * @brief The TaskScheduler class 基类
 */
class TaskScheduler{
    /**
     * @brief MAX_TIME_OUT 最大允许设置的超时时间
     */
    static constexpr int64_t MAX_TIME_OUT=88888888000;//us
    /**
     * @brief MIN_TIME_OUT 最小允许设置的超时时间
     */
    static constexpr  int64_t MIN_TIME_OUT=2000000;//us
public:
    TaskScheduler(int _id=0);
    virtual ~TaskScheduler(){
        MICAGENT_MARK(" ");
    }
    /**
     * @brief start 事件循环处理，阻塞
     */
    void start();
    /**
     * @brief stop 退出事件循环
     */
    void stop();
    /**
     * @brief get_running_state 获取运行状态
     * @return
     */
    bool get_running_state()const{
    DEBUG_LOCK
    return m_is_running;}
    /**
     * @brief updateChannel 更新channel信息
     * @param channel
     */
    virtual void updateChannel(ChannelPtr channel);
    /**
     * @brief updateChannel 修改已存在的channel的信息
     * @param channel
     */
    virtual void updateChannel(Channel *channel);
    /**
     * @brief removeChannel 移除channel
     * @param channel
     */
    virtual void removeChannel(ChannelPtr &channel);
    /**
     * @brief removeChannel 移除channel
     * @param fd
     */
    virtual void removeChannel(SOCKET fd);
    /**
     * @brief handleEvent 处理IO事件
     * @return
     */
    virtual bool handleEvent();
    /**
     * @brief id 返回调度器id
     * @return
     */
    int id()const{return m_id;}
protected:
    /**
     * @brief handle_channel_events 处理对应fd的IO事件
     * @param fd 触发事件的fd
     * @param events
     */
    bool handle_channel_events(SOCKET fd,int events);
    /**
     * @brief wake_up 调度器唤醒
     */
    void wake_up();
    /**
     * @brief get_time_out 获取当前超时时间
     * @return
     */
    int64_t get_time_out();
    /**
     * @brief m_mutex 资源锁
     */
    mutex m_mutex;
    /**
     * @brief m_channel_map SOCKET:channel信息map
     */
    unordered_map<SOCKET,ChannelPtrW> m_channel_map;
    /**
     * @brief m_wakeup_channel 唤醒通道
     */
    ChannelPtr m_wakeup_channel;
    /**
     * @brief m_last_handle_none 判断最近一次是否没有处理任何事件
     */
    bool m_last_handle_none;
private:
    int m_id;
    shared_ptr<Pipe> m_wake_up_pipe;
#ifdef DEBUG
    mutable mutex m_debug_mutex;
#endif
    atomic<bool> m_is_running;
    /**
     * @brief m_last_time_out 上一次的超时时间
     */
    int64_t m_last_time_out;
};
class SelectTaskScheduler:public TaskScheduler{
public:
    SelectTaskScheduler(int _id=0);
    virtual ~SelectTaskScheduler();
    void updateChannel(ChannelPtr channel);
    void updateChannel(Channel *channel);
    void removeChannel(ChannelPtr &channel);
    void removeChannel(SOCKET fd);
    bool handleEvent();
private:
    /**
     * @brief m_read_sets 读fd集合
     */
    fd_set m_read_sets;
    /**
     * @brief m_write_sets 写fd集合
     */
    fd_set m_write_sets;
    /**
     * @brief m_exception_sets 异常fd集合
     */
    fd_set m_exception_sets;
    /**
     * @brief m_max_sock_num 最大的fd
     */
    SOCKET m_max_sock_num;
};
class EpollTaskScheduler:public TaskScheduler{
public:
    EpollTaskScheduler(int _id=0);
    virtual ~EpollTaskScheduler();
    void updateChannel(ChannelPtr channel);
    void updateChannel(Channel *channel);
    void removeChannel(ChannelPtr &channel);
    void removeChannel(SOCKET fd);
    bool handleEvent();
private:
    /**
      * @brief update 调用epoll接口
      * @param operation 操作项名
      * @param channel
      */
    void update(int operation, ChannelPtr& channel);
    SOCKET m_epoll_fd;
};
}


#endif // TASK_SCHEDULER_H
