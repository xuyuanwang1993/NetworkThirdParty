#ifndef TIMEOUT_SESSION_TASK_H
#define TIMEOUT_SESSION_TASK_H
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
namespace micagent {
using namespace std;
using TimeoutCallback=function<void()>;
class time_out_session_cache{
    /**
     * @brief The cache_session struct save the timeout session info
     */
    struct  cache_session{
        TimeoutCallback callback;
        int64_t last_timeout;
        uint32_t base_time_out_ms;
        void update(int64_t time_now){
            last_timeout=time_now+base_time_out_ms;
        }
    };

    /**
     * @brief DEFAULT_INTERVAL 工作线程默认执行间隔
     */
    static constexpr int64_t DEFAULT_INTERVAL=1000;//1s
public:
    /**
     * @brief CreateNew the class instance only can be created by this function
     * @return
     */
    static time_out_session_cache *CreateNew(){return new time_out_session_cache();}
    /**
     * @brief remove_task remove timeout task from this session manager according to  source_ptr
     * @param source_ptr
     */
    void remove_task(void *source_ptr);
    /**
     * @brief update_task reset task's next timeout time which is specified by the source_ptr
     * @param source_ptr
     */
    void update_task(void *source_ptr);
    /**
     * @brief update_by_step delay the task's timeout time
     * @param source_ptr specify the task
     * @param time_ms the increment of timeout time
     */
    void update_by_step(void *source_ptr,uint32_t time_ms=1);
    /**
     * @brief add_task add a timeout task
     * @param source_ptr usually ,the source's ptr
     * @param callback the function to be executed when the task is time out
     * @param base_time_out_ms the original timeout time
     * @return it's only be false when the source_ptr has been added
     */
    bool add_task(void *source_ptr,const TimeoutCallback &callback,uint32_t base_time_out_ms=10);
    /**
     * @brief start tart a thread to process the task
     */
    void start();
    /**
     * @brief stop stop the task thread
     */
    void stop();
    /**
     * @brief from_ptr_to_num convert a ptr to uint64_t
     * @param source_ptr
     * @return
     */
    static inline uint64_t from_ptr_to_num(const void *source_ptr){return reinterpret_cast<uint64_t>(source_ptr);}
    //join the task thread
     ~time_out_session_cache();
private:
    time_out_session_cache();
    /**
     * @brief getTimeNow 获取从1970年到当前时间的毫秒数
     * @return
     */
    static int64_t getTimeNow(){
        auto timePoint = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count();
    }
private:
    shared_ptr<thread>m_work_thread;
    mutex m_mutex;
    condition_variable m_cv;
    /**
     * @brief m_task_map save all timeout task
     */
    map<pair<int64_t,uint64_t>,shared_ptr<cache_session>>m_task_map;
    /**
     * @brief m_session_map save all timeout task info
     */
    map<uint64_t,shared_ptr<cache_session>>m_session_map;
    /**
     * @brief m_exit_flag control the thread running status.set it true to exit the thread
     */
    atomic_bool m_exit_flag;
};
}


#endif // TIMEOUT_SESSION_TASK_H
