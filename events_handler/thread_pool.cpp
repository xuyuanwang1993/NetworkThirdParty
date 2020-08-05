#include "thread_pool.h"
namespace micagent {
thread_pool::thread_pool(int32_t thread_nums):m_word_threads(0){
    int32_t nums=thread_nums;
    if(nums<MIN_THREAD_NUMS)nums=MIN_THREAD_NUMS;
    if(nums>MAX_THREAD_NUMS)nums=MAX_THREAD_NUMS;
    m_is_running.exchange(true);
    //初始化工作线程
    for(int32_t i=0;i<nums;i++){
        m_threads.push_back(make_shared<task_thread>(i,this));
    }
}
thread_pool::~thread_pool(){
    stop();
    for(auto i : m_threads){
        if(i->m_thread&&i->m_thread->joinable())i->m_thread->join();
        if(i->m_thread)i->m_thread.reset();
    }
    m_threads.clear();
    MICAGENT_MARK("mark");
}
bool thread_pool::add_thread_task(const ThreadTask &task){
    bool ret=false;
    lock_guard<mutex> lock(m_mutex);
    do{
        //当有可用工作线程时，将其插入缓存队列，唤醒一个工作线程
        if(m_word_threads==0)break;
        m_word_threads--;
        m_task_queue.push(move(task));
        m_conn.notify_one();
        ret=true;
    }while(0);
    return ret;
}
void thread_pool::stop()
{
    if(m_is_running){
        m_is_running.exchange(false);
        unique_lock<mutex> locker(m_mutex);
        m_conn.notify_all();
    }
}
}
