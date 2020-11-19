#include "timeout_session_task.h"
using namespace micagent;
void time_out_session_cache::remove_task(void *source_ptr)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_map.find(from_ptr_to_num(source_ptr));
    if(iter!=std::end(m_session_map)){
        m_task_map.erase(make_pair(iter->second->last_timeout,iter->first));
        m_session_map.erase(iter);
    }
}
void time_out_session_cache::update_task(void *source_ptr)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_map.find(from_ptr_to_num(source_ptr));
    if(iter!=std::end(m_session_map)){
        m_task_map.erase(make_pair(iter->second->last_timeout,iter->first));
        iter->second->update(getTimeNow());
        m_task_map.emplace(make_pair(iter->second->last_timeout,iter->first),iter->second);
    }
}
void time_out_session_cache::update_by_step(void *source_ptr,uint32_t time_ms)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_map.find(from_ptr_to_num(source_ptr));
    if(iter!=std::end(m_session_map)){
        m_task_map.erase(make_pair(iter->second->last_timeout,iter->first));
        iter->second->base_time_out_ms+=time_ms;
        iter->second->last_timeout+=time_ms;
        m_task_map.emplace(make_pair(iter->second->last_timeout,iter->first),iter->second);
    }
}
bool time_out_session_cache::add_task(void *source_ptr, const TimeoutCallback &callback, uint32_t base_time_out_ms)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_session_map.find(from_ptr_to_num(source_ptr));
    bool ret=false;
    if(iter==std::end(m_session_map)){
        auto index=from_ptr_to_num(source_ptr);
        shared_ptr<cache_session>session(new cache_session());
        session->callback=callback;
        session->base_time_out_ms=base_time_out_ms;
        session->update(getTimeNow());
        m_session_map.emplace(index,session);
        m_task_map.emplace(make_pair(session->last_timeout,index),session);
        ret=true;
        m_cv.notify_one();
    }
    return ret;
}
void time_out_session_cache::start()
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_work_thread){
        m_work_thread.reset(new thread([this](){
            unique_lock<mutex>locker(m_mutex);
            MICAGENT_LOG(LOG_INFO,"time_out_session_cache begin!");
            while(!m_exit_flag){
                auto time_now=getTimeNow();
                while(!m_task_map.empty()&&m_task_map.begin()->first.first<=time_now){
                    auto session=m_task_map.begin()->second;
                    m_session_map.erase(m_task_map.begin()->first.second);
                    m_task_map.erase(m_task_map.begin());
                    if(locker.owns_lock())locker.unlock();
                    session->callback();
                    if(!locker.owns_lock())locker.lock();
                }
                auto time_out=DEFAULT_INTERVAL;
                if(!m_task_map.empty()){
                    auto next_time_out_diff=m_task_map.begin()->first.first-getTimeNow();
                    if(next_time_out_diff>0)time_out=next_time_out_diff;
                }
                if(!m_exit_flag)m_cv.wait_for(locker,chrono::milliseconds(time_out));
            }
            m_session_map.clear();
            m_task_map.clear();
            MICAGENT_LOG(LOG_WARNNING,"time_out_session_cache exit!");
        }));
    }
}
void time_out_session_cache:: stop()
{
    lock_guard<mutex>locker(m_mutex);
    m_exit_flag.exchange(true);
    m_cv.notify_all();
}
time_out_session_cache::time_out_session_cache():m_exit_flag(false)
{

}
time_out_session_cache::~time_out_session_cache()
{

    stop();
    if(m_work_thread&&m_work_thread->joinable())m_work_thread->join();
}
