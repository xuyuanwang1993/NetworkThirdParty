#pragma once
#ifndef TIMER_QUEUE_H
#define TIMER_QUEUE_H
#include <map>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <cstdint>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include<atomic>
#include<set>
#include<condition_variable>
#include "c_log.h"
#define INVALID_TIMER_ID 0
namespace micagent {
using namespace std;
/**
 * @brief TimerEvent 定时器回调函数原型返回true代表循环执行,false代表执行一次
 */
typedef std::function<bool(void)> TimerEvent;
typedef uint32_t TimerId;
/**
 * @brief The Timer class 定时器的简单实现
 */
class Timer
{
public:
    Timer(const TimerEvent& event, uint32_t msec)
        : eventCallback(event)
        , _interval(msec)
    {
        if (_interval == 0)
            _interval = 1;
    }
    Timer() { }
    /**
     * @brief sleep 当前线程休眠
     * @param msec 时间（ms）
     */
    static void sleep(uint32_t msec)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(msec));
    }
    /**
     * @brief getTimeNow 获取从1970年到当前时间的毫秒数
     * @return
     */
    static int64_t getTimeNow(){
        auto timePoint = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    }
    /**
     * @brief getMicroTimeNow 获取从1970年到当前时间的微秒数
     * @return
     */
    static int64_t getMicroTimeNow(){
        auto timePoint = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(timePoint.time_since_epoch()).count();
    }
    /**
     * @brief setEventCallback 设置定时器的回调函数
     * @param event
     */
    void setEventCallback(const TimerEvent& event)
    {
        eventCallback = event;
    }
    /**
     * @brief start 启动定时器,调用此函数会阻塞当前线程
     * @param microseconds 精度 us
     * @param repeat 是否循环执行
     */
    void start(int64_t microseconds, bool repeat = false)
    {
        _isRepeat = repeat;
        auto timeBegin = std::chrono::high_resolution_clock::now();
        int64_t elapsed = 0;
        do
        {
            std::this_thread::sleep_for(std::chrono::microseconds(microseconds - elapsed));
            timeBegin = std::chrono::high_resolution_clock::now();
            eventCallback();
            elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - timeBegin).count();
            if (elapsed < 0)
                elapsed = 0;
        } while (_isRepeat);
    }
    /**
     * @brief stop 停止定时器
     */
    void stop()
    {
        _isRepeat = false;
    }

private:
    friend class TimerQueue;
    /**
     * @brief setNextTimeout 设置定时器到期时间
     * @param currentTimePoint 当前时间
     */
    void setNextTimeout(int64_t currentTimePoint)
    {
        _nextTimeout = currentTimePoint + _interval;
    }
    /**
     * @brief getNextTimeout 获取定时器到期时间
     * @return
     */
    int64_t getNextTimeout() const
    {
        return _nextTimeout;
    }
    /**
     * @brief _isRepeat 是否重复执行
     */
    bool _isRepeat = false;
    TimerEvent eventCallback = [] { return false; };
    /**
     * @brief _interval 执行间隔
     */
    uint32_t _interval = 0;
    /**
     * @brief _nextTimeout 下次执行就绪时间
     */
    int64_t _nextTimeout = 0;
};
/**
 * @brief The TimerQueue class 多线程延时队列，基于最小堆实现（map）
 */
class TimerQueue
{
    /**
     * @brief DEFAULT_INTERVAL 工作线程默认执行间隔
     */
    static constexpr int64_t DEFAULT_INTERVAL=1000;//1s
public:
    TimerQueue();
    //禁用拷贝构造
    TimerQueue(const TimerQueue &)=delete ;
    //禁用拷贝构造
    TimerQueue &operator =(const TimerQueue &)=delete;
    /**
     * @brief addTimer 添加定时器事件
     * @param event 事件回调函数
     * @param msec 执行间隔ms
     * @return 返回定时器id可通过此id删除定时器
     */
    TimerId addTimer(const TimerEvent& event, uint32_t msec);
    /**
     * @brief removeTimer 移除定时器
     * @param timerId 定时器id
     */
    void removeTimer(TimerId timerId);
    /**
     * @brief blockRemoveTimer 移除定时器
     * @param timerId 定时器id
     */
    void blockRemoveTimer(TimerId timerId);
    /**
     * @brief loop 循环处理定时器事件
     */
    void loop();
    /**
     * @brief stop 停止定时器事件处理
     */
    void stop();
    /**
     * @brief get_run_status 获取当前定时器队列的运行状态
     * @return
     */
    bool get_run_status()const{
    DEBUG_LOCK
    return _is_running;}
    ~TimerQueue(){stop();if(m_thread&&m_thread->joinable())m_thread->join();if(m_thread)m_thread.reset();
MICAGENT_MARK("mark!");
    }
private:
    /**
     * @brief getTimeRemaining 获取第一个需要执行的定时器的剩余时间
     * @return
     */
    int64_t getTimeRemaining();
    /**
     * @brief handleTimerEvent 处理定时器事件
     */
    void handleTimerEvent();
    /**
     * @brief _mutex 数据读写锁
     */
    std::mutex _mutex;
    std::mutex m_block_mutex;
    std::mutex _conn_mutex;
    /**
     * @brief _timers 当前的timers集合
     */
    std::unordered_map<TimerId, std::shared_ptr<Timer>> _timers;
    /**
     * @brief _events 当前的定时器事件集合
     */
    std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> _events;
    /**
     * @brief _timers_cache 当次执行时间间隔内新加入的定时器
     */
    std::unordered_map<TimerId, std::shared_ptr<Timer>> _timers_cache;
    /**
     * @brief _events_cache 当次执行时间间隔内新加入的定时器事件
     */
    std::map<std::pair<int64_t, TimerId>, std::shared_ptr<Timer>> _events_cache;
    /**
     * @brief _remove_set 当次执行时间间隔内需要移除的定时器ID的集合
     */
    std::set<TimerId>_remove_set;
    uint32_t _lastTimerId = INVALID_TIMER_ID;
    atomic<bool> _is_running;
    std::condition_variable _conn;
    shared_ptr<thread> m_thread;
#ifdef DEBUG
    mutable mutex m_debug_mutex;
#endif
};
}
#endif // TIMER_QUEUE_H
