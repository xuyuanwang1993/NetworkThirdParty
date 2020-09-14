#pragma once
#ifndef KCP_SERVER_H
#define KCP_SERVER_H
#include "ikcp.h"
#include <functional>
#include <memory>
#include "ikcp.h"
#include <atomic>
#include<queue>
#include <unordered_map>
#include"event_loop.h"
#include "c_log.h"
#include <unordered_set>
namespace micagent {
using KCPSERVER_RECVCALLBACK=function<void(const char *buf,int len,void *session,void *kcp_server) >;
using KCPSERVER_CLOSECALLBACK=function<void (uint32_t conv_id)>;
class kcp_server:public enable_shared_from_this<kcp_server>{
    static constexpr uint32_t MAX_TRY_TIMES=10000;
    static constexpr int64_t MAX_CACHE_TIME=5*1000;//5s
    static constexpr uint8_t MAX_TIME_OUT_CNT=3;//
public:
    struct kcp_session{
        uint32_t conv_id;
        struct sockaddr_in recv_addr;
        struct IKCPCB * kcp;
        KCPSERVER_RECVCALLBACK recv_callback;
        KCPSERVER_CLOSECALLBACK close_callback;
        int64_t last_alive_time;
        uint8_t time_out_cnt;
        SOCKET fd;
        void *user;
        kcp_session(uint32_t _conv_id,const struct sockaddr_in&addr,KCPSERVER_RECVCALLBACK recv_cb,KCPSERVER_CLOSECALLBACK _close_callback,\
                    SOCKET _fd,void *_user):conv_id(_conv_id),recv_addr(addr),recv_callback(recv_cb),close_callback(_close_callback),\
            last_alive_time(Timer::getTimeNow()),time_out_cnt(0),fd(_fd),user(_user)
        {
            kcp=ikcp_create(conv_id,this);
            ikcp_nodelay(kcp, 1, 10, 2, 1);
            kcp->output=[](const char *buf, int len, struct IKCPCB *kcp, void *_user)->int{
                (void )(kcp);
                kcp_session *session=static_cast<kcp_session *>(_user);
                return kcp_server::send_raw_packet(session->fd,buf,len,session->recv_addr);
            };
        }
        ~kcp_session(){
            MICAGENT_MARK("release kcp %u!",kcp->conv);
            ikcp_release(kcp);
        }
    };
public:
    kcp_server(uint16_t port,shared_ptr<EventLoop>loop);
    kcp_server &operator=(const kcp_server &) = delete;
    kcp_server(const kcp_server &) = delete;
    ~kcp_server();
    int kcp_send_packet(uint32_t conv_id,const void *buf,int len);
    void kcp_send_multicast_packet(const void *buf,int len);
    static int send_raw_packet(SOCKET fd ,const void *buf,int len,const sockaddr_in& addr);
    uint32_t get_session_id();
    void remove_session(uint32_t conv_id);
    bool add_session(uint32_t conv_id,const sockaddr_in &addr,void *user=nullptr,KCPSERVER_RECVCALLBACK recv_cb=nullptr,KCPSERVER_CLOSECALLBACK close_cb=nullptr);
    bool start_update_loop(uint32_t interval_ms=10 );
    bool get_loop_run_status()const{
        DEBUG_LOCK
        return m_loop_running;
    }
    bool get_buf_handle_run_status()const{
        DEBUG_LOCK
        return m_buf_handle_running;
    }
    void clear(){
        lock_guard<mutex>locker(m_mutex);
        auto event_loop=m_event_loop.lock();
        if(event_loop)
        {
             if(m_udp_channel)event_loop->removeChannel(m_udp_channel);
        }
        m_udp_channel.reset();
    }
private:
    void hand_read();
    void update_loop();
    void handle_cache();
    static uint64_t addr_to_uint64(const sockaddr_in &addr);
    mutex m_mutex;
    weak_ptr<EventLoop>m_event_loop;
    uint32_t m_session_id;
    unordered_map<uint32_t,shared_ptr<kcp_session>> m_session_map;
    set<uint64_t> m_addr_set;
    atomic_bool m_loop_running;
    atomic_bool m_buf_handle_running;
    condition_variable m_buf_handle_wait;
    condition_variable m_status;
    mutex m_exit_mutex;
    ChannelPtr m_udp_channel;
    mutex m_bufcache_mutex;
    struct kcp_buf_cache
    {
        shared_ptr<char>buf;
        int buf_len;
        kcp_buf_cache(shared_ptr<char>_buf,int _buf_len):buf(_buf),buf_len(_buf_len) {}
    };
    queue<kcp_buf_cache>m_buf_cache;
#ifdef DEBUG
    mutable mutex m_debug_mutex;
#endif
};
}
#endif // KCP_SERVER_H
