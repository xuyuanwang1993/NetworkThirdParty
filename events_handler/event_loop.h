#pragma once
#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H
#include "task_scheduler.h"
#include "thread_pool.h"
#include "trigger_event.h"
#include "timer_queue.h"
#include "c_log.h"
namespace micagent {
using namespace std;
class EventLoop{
public:
    /**
     * @brief EventLoop 构造函数
     * @param thread_pool_size 线程池大小
     * @param trigger_threads trigger_queue工作线程数
     * @param capacity trigger_queue工作容器容量
     * @param thread_nums IO处理的线程数
     */
    EventLoop(int32_t thread_pool_size=2,uint32_t trigger_threads=2,uint32_t capacity=2000,uint32_t thread_nums=1);
    ~EventLoop();
    /**
     * @brief stop 停止所有事件循环
     */
    void stop();
    /**
     * @brief add_thread_task 向线程池添加任务
     * @param task
     * @return
     */
    bool add_thread_task(const ThreadTask &task);
    /**
     * @brief addTimer 添加定时器任务
     * @param event
     * @param msec 执行间隔ms
     * @return
     */
    TimerId addTimer(const TimerEvent& event, uint32_t msec);
    /**
     * @brief removeTimer 移除定时器任务
     * @param timerId 定时器id
     */
    void removeTimer(TimerId timerId);
    /**
     * @brief blockRemoveTimer 移除定时器任务
     * @param timerId 定时器id
     */
    void blockRemoveTimer(TimerId timerId);
    /**
     * @brief add_trigger_event 添加trigger事件
     * @param event
     * @return
     */
    bool add_trigger_event( const TriggerEvent &event);
    /**
     * @brief updateChannel 更新channel信息
     * @param channel
     */
    void updateChannel(ChannelPtr channel);
    /**
     * @brief updateChannel 修改已存在的channel信息
     * @param channel
     */
    void updateChannel(Channel* channel);
    /**
      * @brief removeChannel 移除channel
      * @param channel
      */
    void removeChannel(ChannelPtr &channel);
    /**
      * @brief removeChannel 移除channel
      * @param fd
      */
    void removeChannel(SOCKET fd);
    /**
      * @brief get_taskscheduler 获取指定index的TaskScheduler
      * @param index
      * @return 若index不在范围内则会返回null
      */
    shared_ptr<TaskScheduler> get_taskscheduler(int index=-1);
private:
    /**
      * @brief move_index index移位
      */
    void move_index(){ m_index=(m_index+1==m_TaskSchedulers.size())?0:m_index+1;}
    mutex m_mutex;
    /**
     * @brief m_index 当前index
     */
    uint32_t m_index;
    /**
     * @brief m_timer_queue 延时队列
     */
    shared_ptr<TimerQueue> m_timer_queue;
    /**
     * @brief m_thread_pool 线程池
     */
    shared_ptr<thread_pool> m_thread_pool;
    /**
     * @brief m_trigger_queue trigger队列
     */
    shared_ptr<trigger_queue> m_trigger_queue;
    /**
     * @brief The eventloop_task struct IO处理实例
     */
    struct eventloop_task:public enable_shared_from_this<eventloop_task>{
        shared_ptr<TaskScheduler> m_task_scheduler;
        shared_ptr<thread> m_thread;
        atomic<bool> m_init;
        eventloop_task(int _id):m_init(false){
#if 1&&(defined(__linux) || defined(__linux__))
            m_task_scheduler.reset(new EpollTaskScheduler(_id));
#elif 0||defined(WIN32) || defined(_WIN32)
            m_task_scheduler.reset(new SelectTaskScheduler(_id));
#endif
        }
        void start(){
            if(!m_init){
                m_thread.reset(new thread([this](){
                    this->m_task_scheduler->start();
                }));
                m_init=true;
            }
        }
        void stop(){
            if(m_task_scheduler)m_task_scheduler->stop();
            if(m_thread&&m_thread->joinable())m_thread->join();
            m_init=false;
        }
        ~eventloop_task(){
            if(m_init)stop();
        }
    };
    /**
     * @brief m_TaskSchedulers IO处理handler数组
     */
    vector<shared_ptr<eventloop_task>> m_TaskSchedulers;
    /**
     * @brief m_fd_map 缓存fd与TaskScheduler的对应信息
     */
    unordered_map<SOCKET,int> m_fd_map;
    atomic_bool m_is_stop;
};
}
#endif // EVENT_LOOP_H
