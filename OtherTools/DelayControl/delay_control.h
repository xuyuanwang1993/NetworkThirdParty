#ifndef DELAY_CONTROL_H
#define DELAY_CONTROL_H
#include <cstdint>
#include<chrono>
#include<thread>
#include<unistd.h>
namespace micagent {
using namespace std;
class  delay_control_base{
public:
    delay_control_base();
    virtual int64_t block_wait_next_due(void *data,bool wait=true)=0;
    virtual ~delay_control_base();
    void reset_due_interval(int64_t micro_interval){m_micro_due_interval=micro_interval;}
    static inline int64_t get_micro_time_now()
    {
        auto timePoint = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(timePoint.time_since_epoch()).count();
    }
    static inline void thread_pause_micro_time(int64_t micro_time)
    {
        //usleep(micro_time);
        this_thread::sleep_for(chrono::microseconds(micro_time));

    }
protected:
    int64_t m_last_due_time;
    int64_t m_micro_due_interval;
    int64_t m_sleep_time_fix;
};
class h264_delay_control:public delay_control_base{
public:
    h264_delay_control(uint32_t frame_rate=25);
    int64_t block_wait_next_due(void *data,bool wait=true);
    ~h264_delay_control(){

    }
};
class h265_delay_control:public delay_control_base{
public:
    h265_delay_control(uint32_t frame_rate=25);
    int64_t block_wait_next_due(void *data,bool wait=true);
    ~h265_delay_control(){

    }
};
}
#endif // DELAY_CONTROL_H
