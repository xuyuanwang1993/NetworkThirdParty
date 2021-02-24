#ifndef KCP_COMMON_H
#define KCP_COMMON_H
#include "ikcp.h"
#include "network_util.h"
#include "event_loop.h"
#include "ikcp.h"
#include <queue>
namespace micagent {
enum IKCP_PROXY_DEFINES_E:uint32_t{
    //version
    IKCP_PROTOCAL_VERSION=1,
    IKCP_RAW_UDP_CONV_ID=0,
    //conv_id define
    IKCP_MIN_APPLICATION_CONV_ID=1,
    IKCP_MAX_APPLICATION_CONV_ID=0xfffffffe,
    IKCP_PROXY_CONTROL_CONV_ID=0xffffffff,
    //length
    IKCP_PROXY_HEADER_LEN=12,
    IKCP_CONFIG_LEN=5,
    //data defines
    IKCP_MAX_CACHE_BUFFER_LIMITS=10*1024,
    IKCP_MAX_CACHE_BUFFER_NO_LIMIT=0xffffffff,
    //work_port
    IKCP_WORK_PORT_DEFAULT=11111,
    IKCP_WORK_HELP_PORT_DEFAULT=11112,
    //connection define
    IKCP_SEND_KEEP_ALIVE_INTERVAL=30*1000,//30s
    IKCP_CONNECTION_TIME_OUT_TIME=2*60*1000,//2min
    IKCP_CONNECTION_PACKET_RESEND_INTERVAL=2*1000,//2s
    IKCP_CONNECTION_PACKET_RESEND_MAX_TIMES=4,
};

enum IKCP_PROXY_CMD_E:uint8_t{
    IKCP_SYN,
    IKCP_SYN_ACK,
    IKCP_CONNECTION_FINISHED,
    IKCP_RST,
    IKCP_KEEPALIVE,
    IKCP_PAYLOAD,
};
struct ikcp_proxy_header_s{
    uint32_t protocal_version:8;
    uint32_t cmd:8;
    uint32_t sn:16;
    uint32_t timestamp;
    uint32_t conv_id;
};
struct ikcp_config_s{
    uint32_t window_size:16;
    uint32_t interval:14;
    uint32_t nodelay:1;
    uint32_t nc:1;
    uint8_t resend;
};
class ikcp_proxy_helper{
public:
    static bool encode_ikcp_proxy_header(void *buf,uint32_t available_len,const ikcp_proxy_header_s&header);
    static bool decode_ikcp_proxy_header(const void*buf,uint32_t available_len,ikcp_proxy_header_s &header);
    static bool encode_ikcp_config(void *buf,uint32_t available_len,const ikcp_config_s&config);
    static bool decode_ikcp_config(const void*buf,uint32_t available_len,ikcp_config_s &config);
    static ikcp_proxy_helper &Instance(){static ikcp_proxy_helper instance;return instance;}
    TimerQueue &GetTimeQueue()const{
        return *m_kcp_timer_queue;
    }
private:
    ikcp_proxy_helper(){
        m_kcp_timer_queue.reset(new TimerQueue());
    }
    ~ikcp_proxy_helper(){
        if(m_kcp_timer_queue)m_kcp_timer_queue->stop();
    }
    shared_ptr<TimerQueue>m_kcp_timer_queue;
};

template <typename entry>
class ikcp_proxy_cache{
public:
    using kcp_udp_packet_callback=function<bool(const entry&)>;
    bool push( const entry &&data)
    {
        lock_guard<mutex>locker(m_mutex);
        if(m_cache_queue.size()>=m_max_data_cache_buffers||m_cache_queue_helper.size()>=m_max_data_cache_buffers)return false;
        m_cache_queue.push(move(data));
        m_queue_cv.notify_one();
        return true;
    }
    void process_data(){
        unique_lock<mutex>locker(m_mutex);
        if(m_cache_queue.empty())
        {
            m_queue_cv.wait_for(locker,chrono::milliseconds(20));
            if(m_cache_queue.empty())return;
        }
        queue<entry>cache_queue;
        while(!m_cache_queue.empty()){
            cache_queue.push(m_cache_queue.front());
            m_cache_queue.pop();
        }
        locker.unlock();
        //handle data
        while(!cache_queue.empty()){
            m_cb(cache_queue.front());
            cache_queue.pop();
        }
    }
    bool process_send_data(){
        unique_lock<mutex>locker(m_mutex);
        queue<entry>cache_queue;
        while(!m_cache_queue_helper.empty()){
            cache_queue.push(m_cache_queue_helper.front());
            m_cache_queue_helper.pop();
        }
        while(!m_cache_queue.empty()){
            cache_queue.push(m_cache_queue.front());
            m_cache_queue.pop();
        }
        locker.unlock();
        //handle data
        while(!cache_queue.empty()){
            if(!m_cb(cache_queue.front()))break;
            cache_queue.pop();
        }
        locker.lock();
        while(!cache_queue.empty()){
            m_cache_queue_helper.push(cache_queue.front());
            cache_queue.pop();
        }
        return  m_cache_queue.empty();
    }
    ikcp_proxy_cache(const kcp_udp_packet_callback&cb,uint32_t cache_limit=IKCP_MAX_CACHE_BUFFER_NO_LIMIT):m_cb(cb),m_max_data_cache_buffers(cache_limit){
        assert(m_cb!=nullptr);
    }
    ~ikcp_proxy_cache(){
        m_queue_cv.notify_all();
    }
protected:
    condition_variable m_queue_cv;
    mutex m_mutex;
    queue<entry>m_cache_queue;
    queue<entry>m_cache_queue_helper;
    const kcp_udp_packet_callback m_cb;
    const uint32_t m_max_data_cache_buffers;
};
//define data callbacks

struct ikcp_raw_udp_packet_s{
    shared_ptr<uint8_t>buf;
    const uint32_t buf_len;
    const sockaddr_in addr;
    ikcp_raw_udp_packet_s(shared_ptr<uint8_t> _buf,uint32_t _buf_len,const sockaddr_in&_addr):buf(_buf),buf_len(_buf_len),addr(_addr){}
    ikcp_raw_udp_packet_s copy()const{
        shared_ptr<uint8_t>new_buf(new uint8_t[buf_len],default_delete<uint8_t[]>());
        memset(new_buf.get(),0,buf_len);
        memcpy(new_buf.get(),buf.get(),buf_len);
        return ikcp_raw_udp_packet_s(new_buf,buf_len,addr);
    }
};
using kcp_proxy_data_callback=function<void (const ikcp_raw_udp_packet_s&data)>;

enum IKCP_CONNECTION_STATUS:uint8_t{
    IKCP_CONNECTION_DEFAULT,
    IKCP_CONNECTION_SYN_SENT,
    IKCP_CONNECTION_SYN_RECV,
    IKCP_CONNECTION_SYN_ACK_SENT,
    IKCP_CONNECTION_SYN_ACK_RECV,
    IKCP_CONNECTION_FINISHED_SENT,
    IKCP_CONNECTION_FINISHED_RECV,
    IKCP_CONNECTION_CLOSED,
};
struct ikcp_connection_info{
    sockaddr_in peer_addr;
    uint32_t build_timestamp;
    int64_t last_alive_time;
    uint32_t conv_id;
    IKCP_CONNECTION_STATUS connection_status;
    TimerId connect_timer;
    uint32_t packet_resend_times;
    ikcp_config_s config;
    IKCPCB *kcp;
    ikcp_connection_info():build_timestamp(0),last_alive_time(Timer::getTimeNow()),conv_id(IKCP_PROXY_CONTROL_CONV_ID)\
    ,connection_status(IKCP_CONNECTION_DEFAULT),connect_timer(INVALID_TIMER_ID),packet_resend_times(0),kcp(nullptr){

    }
};
}
#endif // KCP_COMMON_H
