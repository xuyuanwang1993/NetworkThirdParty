#include "event_loop.h"
namespace micagent {
EventLoop::EventLoop(int32_t thread_pool_size,uint32_t trigger_threads,uint32_t capacity,uint32_t thread_nums):\
    m_index(0),m_timer_queue(new TimerQueue()),m_thread_pool(new thread_pool(thread_pool_size)),\
    m_trigger_queue(new trigger_queue(trigger_threads,capacity)),m_is_stop(false)
{
    if(thread_nums==0)thread_nums=thread::hardware_concurrency()+1;
    for(uint32_t i=0;i<thread_nums;i++)
    {
        m_TaskSchedulers.push_back(make_shared<eventloop_task>(i));
        m_TaskSchedulers[i]->start();
    }
}
EventLoop::~EventLoop()
{
    if(!m_is_stop)stop();
}
void EventLoop::stop()
{
    if(m_is_stop)return;
    m_is_stop.exchange(true);
    m_thread_pool->stop();
    m_timer_queue->stop();
    m_trigger_queue->stop();
    for(auto i : m_TaskSchedulers)i->stop();
    m_timer_queue.reset();
    m_trigger_queue.reset();
    m_thread_pool.reset();
    m_TaskSchedulers.clear();
}
bool EventLoop::add_thread_task(const ThreadTask &task)
{
    return m_thread_pool->add_thread_task(task);
}
TimerId EventLoop::addTimer(const TimerEvent& event, uint32_t msec)
{
    if(!m_timer_queue->get_run_status())return 0;
    return m_timer_queue->addTimer(event,msec);
}
void EventLoop::removeTimer(TimerId timerId)
{
    if(m_timer_queue->get_run_status())m_timer_queue->removeTimer(timerId);
}
void EventLoop::blockRemoveTimer(TimerId timerId)
{
    if(m_timer_queue->get_run_status())m_timer_queue->blockRemoveTimer(timerId);
}
bool EventLoop::add_trigger_event(const TriggerEvent & event)
{
    return m_trigger_queue->add_trigger_event(event);
}
void EventLoop::updateChannel(ChannelPtr channel)
{
    lock_guard<mutex> locker(m_mutex);
    SOCKET fd=channel->fd();
    if(fd!=INVALID_SOCKET){
        auto iter=m_fd_map.find(fd);
        if(iter!=end(m_fd_map))m_TaskSchedulers[static_cast<size_t>(iter->second)]->m_task_scheduler->updateChannel(channel);
        else {
            m_fd_map.emplace(fd,m_index);
            m_TaskSchedulers[m_index]->m_task_scheduler->updateChannel(channel);
            move_index();
        }
    }
}
void EventLoop::updateChannel(Channel* channel){
    lock_guard<mutex> locker(m_mutex);
    SOCKET fd=channel->fd();
    if(fd!=INVALID_SOCKET){
        auto iter=m_fd_map.find(fd);
        if(iter!=end(m_fd_map))m_TaskSchedulers[static_cast<size_t>(iter->second)]->m_task_scheduler->updateChannel(channel);
    }
}
void EventLoop::removeChannel(ChannelPtr &channel)
{
    lock_guard<mutex> locker(m_mutex);
    SOCKET fd=channel->fd();
    if(fd!= INVALID_SOCKET){
        auto iter=m_fd_map.find(fd);
        if(iter!=end(m_fd_map)){
            m_TaskSchedulers[static_cast<size_t>(iter->second)]->m_task_scheduler->removeChannel(fd);
            m_fd_map.erase(iter);
        }
    }
}
void EventLoop::removeChannel(SOCKET fd)
{
    lock_guard<mutex> locker(m_mutex);
    if(fd != INVALID_SOCKET){
        auto iter=m_fd_map.find(fd);
        if(iter!=end(m_fd_map)){
            m_TaskSchedulers[static_cast<size_t>(iter->second)]->m_task_scheduler->removeChannel(fd);
            m_fd_map.erase(iter);
        }
    }
}
shared_ptr<TaskScheduler> EventLoop::get_taskscheduler(int index)
{
    if(index>=static_cast<int>(m_TaskSchedulers.size()))return  nullptr;
    else if(index==-1)
    {
        index=static_cast<int>(m_index);
        {
            lock_guard<mutex> locker(m_mutex);
            move_index();
        }
    }
    return m_TaskSchedulers[static_cast<uint32_t>(index)]->m_task_scheduler;
}
}
