#include "delay_control.h"
using namespace micagent;
delay_control_base::delay_control_base():m_last_due_time(get_micro_time_now()),m_micro_due_interval(1),m_sleep_time_fix(500)
{

}
delay_control_base::~delay_control_base()
{

}
h264_delay_control::h264_delay_control(uint32_t frame_rate):delay_control_base ()
{
    if(frame_rate>0)
    {
        m_micro_due_interval=1000*1000/frame_rate;
    }
}
int64_t h264_delay_control::block_wait_next_due(void *data,bool wait)
{
    char byte=*(static_cast<char *>(data));
/* 0 not used
 *  1 not idr  no partition
 *  2 A slice
 *  3 B slice
 *  4 C slice
 *  5 IDR
 *  6 sei
 *  7 sps
 *  8 pps
 *  >9 not data
 */
    int nalu_type=byte&0x1f;//low 5 bit
    auto time_now=get_micro_time_now();
    if(nalu_type!=0&&nalu_type<6)
    {
        if(wait){
            auto time_need_wait=m_micro_due_interval-(time_now-m_last_due_time)-m_sleep_time_fix;
            if(time_need_wait>0){
                thread_pause_micro_time(time_need_wait);
            }
        }
        m_last_due_time+=m_micro_due_interval;
    }
    if(time_now-m_last_due_time>1000000)m_last_due_time=time_now;
    return m_last_due_time;
}
h265_delay_control::h265_delay_control(uint32_t frame_rate):delay_control_base ()
{
    if(frame_rate>0)
    {
        m_micro_due_interval=1000*1000/frame_rate;
    }
}

int64_t h265_delay_control::block_wait_next_due(void *data,bool wait)
{
    char byte=*(static_cast<char *>(data));
/*
 *  0-9 not idr   data frame
 * 16-21 idr frame
 * 32 vps
 * 33 sps
 * 34 pps
 * 39 prefix sei
 * 40 suffix sei
 */
    auto time_now=get_micro_time_now();
    int nalu_type=(byte&0x7E)>>1;//2-7    6bit
    if((nalu_type<=9)||(nalu_type>=16&&nalu_type<=21))
    {
        if(wait){
            auto time_need_wait=m_micro_due_interval-(time_now-m_last_due_time)-m_sleep_time_fix;
            if(time_need_wait>0){
                thread_pause_micro_time(time_need_wait);
            }
        }
        m_last_due_time+=m_micro_due_interval;
    }
    if(time_now-m_last_due_time>1000000)m_last_due_time=time_now;
    return m_last_due_time;
}
