#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H
#include "tcp_connection_helper.h"
#include "buffer_handle.h"
namespace micagent {
using CONNECTION_SUCCESS_CALLBACK=function<void()>;
using CONNECTION_INIT_CALLBACK=function<void()>;
/**
 * @brief The tcp_client class a lasting tcp client
 */
class tcp_client:public enable_shared_from_this<tcp_client>{
protected:
    //max connect wait time
    static constexpr uint32_t MAX_WAIT_TIME=30*1000;//30s
    //min connect wait time
    static constexpr uint32_t MIN_WAIT_TIME=5000;//5s
    //default broken connection's reconnection waiting time
    static constexpr uint32_t DEFAULT_CONNECTION_INTERVAL_MS=1000;//1S
public:
    virtual ~tcp_client();
    tcp_client(shared_ptr<tcp_connection_helper>helper,const string &ip,uint16_t port);
    /**
     * @brief open_connection start a connection to the ip and port
     * @param init_cb be called before open a connection
     * @return when it's closed by the user,it will return false.please rebuild the class!
     */
    bool open_connection(const CONNECTION_INIT_CALLBACK&init_cb=nullptr);
    /**
     * @brief reset_addr_info change destination
     * @param ip
     * @param port
     */
    void reset_addr_info(const string &ip,uint16_t port);
    /**
     * @brief close_connection active to close the client before you release it
     */
    void close_connection();
    /**
     * @brief send_message cache send a raw message
     * @param buf
     * @param buf_len
     * @return by default,when conenction is not availiable ,it will return false
     */
    virtual bool send_message(const void *buf,uint32_t buf_len);
    /**
     * @brief check_is_connected
     * @return  if it's connected it will return true
     */
    bool check_is_connected()const{
        lock_guard<mutex>locker(m_mutex);
        return (!m_is_connecting)&&(!m_is_closed)&&m_tcp_channel;
    }
    /**
     * @brief set_connect_callback  this function will be called when the connection is built
     * @param callback
     */
    void set_connect_callback(const CONNECTION_SUCCESS_CALLBACK &callback){
        lock_guard<mutex>locker(m_mutex);
        m_connection_callback=callback;
    }
    void set_connection_wait_time(uint32_t wait_time_ms){
        lock_guard<mutex>locker(m_mutex);
        m_connection_wait_time_ms=wait_time_ms;
    }
protected:
    /**
     * @brief handle_read callback for the network io input
     * @return
     */
    virtual bool handle_read()=0;
    /**
     * @brief handle_write callback for the network io output
     * @return
     */
    virtual bool handle_write();
    /**
     * @brief tear_down active shut down a connection
     */
    virtual void tear_down()=0;
    /**
     * @brief get_init_status an interface for the user to return class init status,if false is return,the connection will not be set up
     * @return
     */
    virtual bool get_init_status()const=0;
    /**
     * @brief clear_usr_status reset all user's info
     */
    virtual void clear_usr_status()=0;
    /**
     * @brief rebuild_connection connect again
     */
    void rebuild_connection();
    /**
     * @brief reconnect_task do reconnect task
     */
    void reconnect_task();
    /**
     * @brief clear_connection_info disconnect from peer
     */
    void clear_connection_info();
    /**
     * @brief handle_connect handle the conenction status
     * @param status
     * @param fd
     */
    void handle_connect(CONNECTION_STATUS status,SOCKET fd);
    /**
     * @brief get_connection_wait_time get the broken conection's reconnection time
     * @return
     */
    uint32_t get_connection_wait_time()const{
        if(m_is_closed)return 0;
        uint32_t time_now=static_cast<uint32_t>(Timer::getTimeNow());
        auto next_connection_time=m_last_connect_success_time_ms+m_connection_wait_time_ms;
        if(time_now>=next_connection_time)return  0;
        else {
            return next_connection_time-time_now;
        }
    }
protected:
    mutable mutex m_mutex;
    /**
     * @brief m_connection_helper a helper for the connection's establish
     */
    weak_ptr<tcp_connection_helper>m_connection_helper;
    /**
     * @brief m_des_ip server's ip
     */
    string m_des_ip;
    /**
     * @brief m_des_port server' port
     */
    uint16_t m_des_port;
    /**
     * @brief m_des_addr sock_addr
     */
    sockaddr_in m_des_addr;
    /**
     * @brief m_tcp_channel channel for communication
     */
    ChannelPtr m_tcp_channel;
    /**
     * @brief m_send_cache cache the send data
     */
    shared_ptr<buffer_handle>m_send_cache;
    /**
     * @brief m_is_connecting if the connection hasn't built up and the client isn't closed,it will set this flag to true
     */
    bool m_is_connecting;
    /**
     * @brief m_is_closed flag for the closed status
     */
    bool m_is_closed;
    /**
     * @brief m_last_time_out the last wait time out.it will be reset when the connection is set up
     */
    uint32_t m_last_time_out;
    /**
     * @brief m_connection_callback new connection callback
     */
    CONNECTION_SUCCESS_CALLBACK m_connection_callback;
    /**
     * @brief m_last_connect_success_time_ms record the last successfully connect time
     */
    uint32_t m_last_connect_success_time_ms;
    /**
     * @brief m_connection_wait_time_ms broken connection's reconnection waiting time
     */
    uint32_t m_connection_wait_time_ms;
};
}
#endif // TCP_CLIENT_H
