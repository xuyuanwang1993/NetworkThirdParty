#pragma once
#ifndef TRIGGER_EVENT_H
#define TRIGGER_EVENT_H
#include <vector>
#include <memory>
#include <atomic>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<thread>
#include "c_log.h"
namespace micagent {
using namespace  std;

template <typename T>
/**
 * @brief The ring_buf class
 * @details 环形链表模板
 */
class ring_buf
{
    /**
     * @brief MIN_CAPACITY
     * @details 最小容量
     */
    static constexpr uint32_t MIN_CAPACITY=20;
    /**
     * @brief MAX_CAPACITY
     * @details 最大容量
     */
    static constexpr uint32_t MAX_CAPACITY=1000000;
public:
    /**
     * @brief ring_buf
     * @param _capacity 容量
     * @details 构造函数
     */
    ring_buf(uint32_t _capacity=20):m_capacity(_capacity),m_read_pos(0),m_write_pos(0),m_data_counts(0){
        m_capacity=m_capacity<MIN_CAPACITY?MIN_CAPACITY:m_capacity;
        m_capacity=m_capacity>MAX_CAPACITY?MAX_CAPACITY:m_capacity;
        m_buf.resize(m_capacity);
    }
    /**
     * @brief push 插入数据
     * @param entry 数据项左值常引用
     * @return 成功返回true 失败false
     */
    bool push(const T &entry){return push_data(forward<T>(entry));}
    /**
     * @brief push 插入数据
     * @param entry 数据右值引用
     * @return 成功返回true 失败false
     */
    bool push(T &&entry){return push_data(entry);}
    /**
     * @brief pop 取出最早插入的元素
     * @param data_recv 用于接收返回元素
     * @return 成功返回true 链表为空false
     */
    bool pop(T &data_recv){
        if(m_data_counts>0){
            m_data_counts--;
            data_recv=m_buf[m_read_pos];
            ring_move_index(m_read_pos);
            return true;
        }
        return false;
    }
    /**
     * @brief capacity 获取容量
     * @return
     */
    uint32_t capacity()const{return m_capacity;}
    /**
     * @brief full 判断链表是否已满
     * @return
     */
    bool full()const{return m_data_counts==m_capacity;}
    /**
     * @brief size 获取已使用的空间大小
     * @return
     */
    uint32_t size()const {return m_data_counts;}
    /**
     * @brief empty 判断链表是否为空
     * @return
     */
    bool empty()const{return m_data_counts==0;}
private:
    /**
     * @brief ring_move_index 虚拟成环index移位操作
     * @param index
     */
    void ring_move_index(uint32_t &index){index++;index=index==m_capacity?0:index;}
    template<typename F>
    /**
     * @brief push_data 插入数据
     * @param entry 数据右值引用
     * @return 成功返回true链表已满返回false
     */
    bool push_data(F &&entry){
        if(m_data_counts<m_capacity){
            m_buf[m_write_pos]=forward<F>(entry);
            ring_move_index(m_write_pos);
            m_data_counts++;
            return  true;
        }
        return false;
    }
    /**
     * @brief m_capacity 容量
     */
    uint32_t m_capacity;
    /**
     * @brief m_buf 存储元素的数组
     */
    vector<T> m_buf;
    /**
     * @brief m_read_pos 读取数据的索引
     */
    uint32_t m_read_pos;
    /**
     * @brief m_write_pos 写入数据的索引
     */
    uint32_t m_write_pos;
    /**
     * @brief m_data_counts 数组内存储的有效元素数目
     */
    uint32_t m_data_counts;
};
/**
 *TriggerEvent trigger回调函数原型
 */
using TriggerEvent=function<void()>;
/**
 * @brief The trigger_queue class 多线程trigger事件队列
 */
class trigger_queue{
    /**
     * @brief DEFAULT_CAPACITY trigger容器默认容量
     */
    static constexpr uint32_t DEFAULT_CAPACITY=2000;
    /**
     * @brief MAX_THREADS_NUMS trigger事件队列最多允许使用的线程数
     */
    static constexpr uint32_t MAX_THREADS_NUMS=4;
    /**
     * @brief The trigger_params struct trigger容器实例
     */
    struct trigger_params{
        /**
         * @brief m_buf 存储trigger事件
         */
        ring_buf<TriggerEvent> m_buf;
        /**
         * @brief m_mutex m_buf读写锁
         */
        mutex m_mutex;
        condition_variable m_conn;
        /**
         * @brief m_handle 存储上级操作句柄
         */
        trigger_queue *const m_handle;
        /**
         * @brief t 工作线程
         */
        thread t;
        trigger_params(uint32_t capacity,trigger_queue *const handle):m_buf(capacity),m_handle(handle),t([this](){
            unique_lock<mutex>locker(m_mutex);
            //由外部控制工作线程的退出
            while(this->m_handle->get_run_status()){
                //事件容器为空的时候释放读写锁等待唤醒信号
                if(m_buf.empty())m_conn.wait(locker);
                //处理当前容器内的所有事件
                do{
                    TriggerEvent event;
                    if(m_buf.pop(event))event();
                }while(!m_buf.empty());
            }
            MICAGENT_MARK("trigger handle  exit!");
        }){}
    };

public:
    /**
     * @brief trigger_queue 构造函数
     * @param num_threads 工作线程数
     * @param capacity 工作容器的容量
     */
    trigger_queue(uint32_t num_threads=1,uint32_t capacity=DEFAULT_CAPACITY);
    /**
     * @brief add_trigger_event 向事件队列中添加trigger事件
     * @param event
     * @return 成功返回true 失败返回false
     */
    bool add_trigger_event(  TriggerEvent event);
    trigger_queue(const trigger_queue &)=delete ;
    trigger_queue &operator =(const trigger_queue &)=delete;
    ~trigger_queue();
    /**
     * @brief stop 终止工作线程
     */
    void stop();
    bool get_run_status()const{
        DEBUG_LOCK
        return m_is_running;
    }
private:
    /**
     * @brief m_mutex m_trigger_handle/m_next_index 读写锁
     */
    mutex m_mutex;
    /**
     * @brief m_is_running 事件队列工作标识
     */
    atomic<bool> m_is_running;
    /**
     * @brief move_index 移动事件handler的下标
     */
    inline void move_index(){m_next_index++;m_next_index=((m_next_index==m_trigger_handle.size())?0:m_next_index);}
    /**
     * @brief m_trigger_handle 事件handler数组
     */
    vector<shared_ptr<trigger_params>>m_trigger_handle;
    /**
     * @brief m_next_index 当前handler下标
     */
    uint32_t m_next_index;
    mutable mutex m_debug_mutex;
};
}
#endif // TRIGGER_EVENT_H
