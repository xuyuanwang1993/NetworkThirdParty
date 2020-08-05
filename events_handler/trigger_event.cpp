#include "trigger_event.h"
namespace micagent {
trigger_queue::trigger_queue(uint32_t num_threads,uint32_t capacity):m_is_running(true),m_next_index(0){
    if(num_threads<1)num_threads=1;
    if(num_threads>MAX_THREADS_NUMS)num_threads=MAX_THREADS_NUMS;
    //初始化工作线程
    for(uint32_t i=0;i<num_threads;i++){
        m_trigger_handle.emplace_back(make_shared<trigger_params>(capacity,this));
    }
}
bool trigger_queue::add_trigger_event( TriggerEvent event){
    lock_guard<mutex> locker(m_mutex);
    bool ret;
    {
        //向指定的事件handler中添加trigger任务，同时将其唤醒
        unique_lock<mutex> lock(m_trigger_handle[m_next_index]->m_mutex);
        ret=m_trigger_handle[m_next_index]->m_buf.push(std::move(event));
        m_trigger_handle[m_next_index]->m_conn.notify_one();
        MICAGENT_LOG(LOG_ERROR,"test!");
    }
    //更新handler下标
    move_index();
    return ret;
}
trigger_queue::~trigger_queue(){
    stop();
    for(auto i:m_trigger_handle){
        if(i->t.joinable())i->t.join();
    }
    MICAGENT_MARK("mark!");
}
void trigger_queue::stop()
{
    lock_guard<mutex> locker(m_mutex);
    if(get_run_status()){
        {
            DEBUG_LOCK
                    m_is_running.exchange(false);
        }

        for(uint32_t i=0;i<m_trigger_handle.size();i++){
            unique_lock<mutex>locker(m_trigger_handle[i]->m_mutex);
            m_trigger_handle[i]->m_conn.notify_one();
        }
    }
}
}

