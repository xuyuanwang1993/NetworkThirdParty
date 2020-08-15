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
     * @brief DEFAULT_INTERVAL 工作线程默认执行间隔
     */
    static constexpr int64_t DEFAULT_INTERVAL=1000;//1s
public:
    static time_out_session_cache *CreateNew(){return new time_out_session_cache();}
    void remove_task(void *source_ptr);
    void update_task(void *source_ptr,uint32_t step_ms=1);
    bool add_task(void *source_ptr,const TimeoutCallback &callback,uint32_t base_time_out_ms=10);
    void start();
    void stop();
    static inline uint64_t from_ptr_to_num(const void *source_ptr){return reinterpret_cast<uint64_t>(source_ptr);}
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
    map<pair<int64_t,uint64_t>,TimeoutCallback>m_task_map;
    map<uint64_t,pair<int64_t,TimeoutCallback>>m_session_map;
    atomic_bool m_exit_flag;
};
}


#endif // TIMEOUT_SESSION_TASK_H
