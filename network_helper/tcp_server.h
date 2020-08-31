#ifndef TCP_SERVER_H
#define TCP_SERVER_H
#include "event_loop.h"
#include "tcp_connection.h"
namespace micagent {
using namespace std;
class tcp_server:public enable_shared_from_this<tcp_server>{
public:
    /**
     * @brief tcp_server init a tcp server on listen_port
     * @param listen_port
     * @param netinterface_index if you want to specify a interface you can set this param the begin with 0
     */
    tcp_server(uint16_t listen_port,uint32_t netinterface_index=UINT32_MAX);
    virtual ~tcp_server();
    /**
     * @brief register_handle add the io descriptor to event loop
     * @param loop
     */
    void register_handle(EventLoop *loop);
    /**
     * @brief unregister_handle unregister from the event loop
     */
    void unregister_handle();
    /**
     * @brief get_ip return the bind ip
     * @return
     */
    string get_ip()const{return m_server_ip;}
    /**
     * @brief get_port
     * @return the work port
     */
    uint16_t get_port()const{return m_server_port;}
    /**
     * @brief add_connection add a built tcp_connection to the server
     * @param conn
     */
    void add_connection(shared_ptr<tcp_connection> conn);
    /**
     * @brief new_connection create a new tcp_connection
     * @param fd the built fd
     * @return new tcp_connection
     */
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
    Network_Util::net_interface_info m_net_info;
};
}
#endif // TCP_SERVER_H
