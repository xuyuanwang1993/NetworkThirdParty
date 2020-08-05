#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include "event_loop.h"
#include "tcp_connection.h"
namespace micagent {
using namespace std;
class tcp_server:public enable_shared_from_this<tcp_server>{
public:
    tcp_server(uint16_t listen_port,uint32_t netinterface_index=UINT32_MAX);
    virtual ~tcp_server();
    void register_handle(EventLoop *loop);
    void unregister_handle();
    string get_ip()const{return m_server_ip;}
    uint16_t get_port()const{return m_server_port;}
    void add_connection(shared_ptr<tcp_connection> conn);
    virtual shared_ptr<tcp_connection>new_connection(SOCKET fd);
protected:
    virtual void init_server(){
    }
    void remove_connection(SOCKET fd);
    EventLoop *m_loop;
    atomic<bool> m_registered;
    mutex m_mutex;
    string m_server_ip;
    uint16_t m_server_port;
    ChannelPtr m_listen_channel;
    unordered_map<SOCKET,shared_ptr<tcp_connection>>m_connections;
};
}
#endif // TCP_SERVER_H
