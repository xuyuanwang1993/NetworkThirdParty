#include "timer_queue.h"
namespace micagent {
using namespace std;
using namespace std::chrono;
TimerQueue::TimerQueue():_is_running(false)
{
    m_thread.reset(new thread(&TimerQueue::loop,this));
}
TimerId TimerQueue::addTimer(const TimerEvent& event, uint32_t ms)
{
    TimerId ret=0;
    {
        std::lock_guard<std::mutex> locker(_mutex);
        int64_t timeout = Timer::getTimeNow();
        ++_lastTimerId;
        if(_lastTimerId==INVALID_TIMER_ID)++_lastTimerId;
        ret =_lastTimerId;
        auto timer = make_shared<Timer>(event, ms);
        timer->setNextTimeout(timeout);
        //将新加入的定时器先加入到缓存中
        _timers_cache.emplace(ret, timer);
        _events_cache.emplace(std::pair<int64_t, TimerId>(timeout + ms, ret), std::move(timer));
    }
    {
        unique_lock<mutex> locker2(_conn_mutex);
        _conn.notify_one();
    }
    return ret;
}

void TimerQueue::removeTimer(TimerId timerId)
{//将需要移除的定时器加入至缓存中
    std::lock_guard<std::mutex> locker(_mutex);
    _remove_set.insert(timerId);
}
void TimerQueue::blockRemoveTimer(TimerId timerId)
{
    std::lock_guard<std::mutex> locker(m_block_mutex);
    auto iter = _timers.find(timerId);
    if (iter != _timers.end())
    {
        int64_t timeout = iter->second->getNextTimeout();
        _events.erase(std::pair<int64_t, TimerId>(timeout, timerId));
        _timers.erase(timerId);
    }
}
void TimerQueue::loop(){
    {
        DEBUG_LOCK
        if(_is_running)return;
        _is_running.exchange(true);
    }
    while(get_run_status()){
        handleTimerEvent();
        auto time=getTimeRemaining();
        if(time<0)time=DEFAULT_INTERVAL;
        unique_lock<mutex> locker(_conn_mutex);
        _conn.wait_for(locker,chrono::milliseconds(time));
    }
    MICAGENT_MARK("TimerQueue loop exit!");
    _timers.clear();
    _events.clear();
    {
        std::lock_guard<std::mutex> locker(_mutex);
        _timers_cache.clear();
        _events_cache.clear();
    }

}
void TimerQueue::stop(){
    {
        DEBUG_LOCK
                _is_running.exchange(false);
    }
    unique_lock<mutex> locker(_conn_mutex);
    _conn.notify_all();
}
int64_t TimerQueue::getTimeRemaining()
{
    std::lock_guard<std::mutex> locker(m_block_mutex);
    if (_timers.empty())
    {
        return -1;
    }
    int64_t msec = _events.begin()->first.first - Timer::getTimeNow();
    if (msec <= 0)
    {
        msec = 0;
    }

    return msec;
}

void TimerQueue::handleTimerEvent()
{
    std::lock_guard<std::mutex> locker2(m_block_mutex);
    {//同步缓存数据
        std::lock_guard<std::mutex> locker(_mutex);
        /*copy data*/
        for(auto &&i :_timers_cache)_timers.emplace(std::move(i));
        _timers_cache.clear();
        for(auto &&i :_events_cache)_events.emplace(std::move(i));
        _events_cache.clear();
        /*remove_timer*/
        for(auto timerId:_remove_set){
            auto iter = _timers.find(timerId);
            if (iter != _timers.end())
            {
                int64_t timeout = iter->second->getNextTimeout();
                _events.erase(std::pair<int64_t, TimerId>(timeout, timerId));
                _timers.erase(timerId);
            }
        }
        _remove_set.clear();
    }
    int64_t timePoint = Timer::getTimeNow();
    while(!_timers.empty() &&_events.begin()->first.first<=timePoint)
    {
        TimerId timerId = _events.begin()->first.second;
        if(_events.begin()->second->eventCallback&&_events.begin()->second->eventCallback())
        {//返回为true代表需要再次执行,再次加入延时队列
            _events.begin()->second->setNextTimeout(timePoint);
            auto timerPtr = std::move(_events.begin()->second);
            _events.erase(_events.begin());
            _events.emplace(std::pair<int64_t, TimerId>(timerPtr->getNextTimeout(), timerId), timerPtr);
        }
        else{
            _timers.erase(_events.begin()->first.second);
            _events.erase(_events.begin());
        }
    }
}
}
