#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include<mutex>
#include<condition_variable>
#include<thread>
#include <functional>
#include<memory>
#include<vector>
#include<set>
#include<atomic>
#include<queue>
#include "c_log.h"
namespace micagent {
using namespace std;
//返回为true的任务会被重复执行
using ThreadTask=std::function<bool()>;
class thread_pool{
    /**
 * @brief MIN_THREAD_NUMS 最少工作线程数
 */
    static constexpr int32_t MIN_THREAD_NUMS=2;
    /**
 * @brief MAX_THREAD_NUMS 最大工作线程数
 */
    static constexpr int32_t MAX_THREAD_NUMS=256;
    /**
     * @brief The task_thread struct 工作线程实例
     */
    struct task_thread{
        //工作线程所需执行的任务
        ThreadTask m_task;
        //线程id
        int32_t m_id;
        //线程池handle
        thread_pool * const m_pool;
        //工作线程
        std::shared_ptr<std::thread> m_thread;
        task_thread(int32_t _id,thread_pool * const _m_pool):m_task(nullptr),m_id(_id),m_pool(_m_pool){
            m_thread=make_shared<thread>([this](){
                while(m_pool->m_is_running){
                    //任务存在，执行任务，若返回为false重置任务为空,为true重复执行任务
                    if(m_task){
                        if(m_task())continue;
                        m_task=nullptr;
                    }
                    unique_lock<mutex> locker(m_pool->m_mutex);
                    if(m_pool->m_task_queue.empty()){
                        //任务队列为空，释放资源锁，恢复线程资源，等待通知
                        m_pool-> m_word_threads++;
                        m_pool->m_conn.wait(locker);
                        //工作线程被唤醒，判断是否为空，不为空则占用线程资源，更改此线程任务
                        if(m_pool->m_is_running&&!m_pool->m_task_queue.empty()){
                            m_task=m_pool->m_task_queue.front();
                            m_pool->m_task_queue.pop();
                            m_pool-> m_word_threads--;
                        }
                    }
                    else {
                        //从任务队列中取出一个任务执行
                        m_task=m_pool->m_task_queue.front();
                        m_pool->m_task_queue.pop();
                    }
                }
            });
        }
    };
public:
    /**
     * @brief thread_pool 构造函数
     * @param thread_nums 线程数目
     */
    thread_pool(int32_t thread_nums=10);
    ~thread_pool();
    thread_pool &operator=(const thread_pool &) = delete;
    thread_pool(const thread_pool &) = delete;
    /**
     * @brief add_thread_task 添加线程任务
     * @param task
     * @return
     */
    bool add_thread_task(const ThreadTask &task);
    /**
     * @brief stop 退出所有工作线程
     */
    void stop();
private:
    /**
     * @brief m_task_queue 工作任务缓存队列
     */
    queue<ThreadTask> m_task_queue;
    /**
     * @brief m_word_threads 空闲的工作线程
     */
    uint32_t m_word_threads;
    /**
     * @brief m_threads 工作线程数组
     */
    std::vector<std::shared_ptr<task_thread>>m_threads;
    mutex m_mutex;
    condition_variable m_conn;
    /**
     * @brief m_is_running 线程池运行标识
     */
    atomic<bool> m_is_running;
};
}

#endif // THREAD_POOL_H
