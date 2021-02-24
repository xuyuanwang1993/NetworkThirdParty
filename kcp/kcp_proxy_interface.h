#ifndef KCP_PROXY_INTERFACE_H
#define KCP_PROXY_INTERFACE_H
#include "kcp_common.h"
namespace micagent {
class kcp_proxy_interface:public enable_shared_from_this<kcp_proxy_interface>{
public:
    kcp_proxy_interface(shared_ptr<EventLoop>loop,uint16_t port=IKCP_WORK_PORT_DEFAULT);
    bool send_raw_data(const ikcp_raw_udp_packet_s &packet);
    void init(const kcp_proxy_data_callback &data_callback);
    ~kcp_proxy_interface();
private:
    void  handle_read();
    void handle_write();
    void thread_task();
protected:
//socket
    mutex m_io_mutex;
    ChannelPtr m_udp_channel;
    weak_ptr<EventLoop>m_w_event_loop;
    kcp_proxy_data_callback m_data_callback;
    shared_ptr<thread>m_work_thread;
    atomic<bool>m_init;
    atomic<bool>m_running;
    //data
    shared_ptr<ikcp_proxy_cache<ikcp_raw_udp_packet_s>>m_recv_cache;
    shared_ptr<ikcp_proxy_cache<ikcp_raw_udp_packet_s>>m_send_cache;
};
}
#endif // KCP_PROXY_INTERFACE_H
