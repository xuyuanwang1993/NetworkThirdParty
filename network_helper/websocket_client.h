#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H
#include "http_client.h"
#include "websocket_common.h"
#include "websocket_buffer_cache.h"
namespace micagent {
/**
 * @brief The websocket_client class
 * step 1:connect to server and set callback who will send the ws_connection init packet when the conenction is set up
 * step 2:send ws_connection init packet and set the data handler
 * step 3:recv data from remote server
 * step 4:handle data and check key
 * step 5:if key is mathed,change connection status and start application's data transmission
 * step 6:send application's data with 'send_websocket_data' function
 */
class websocket_client:public  http_client{
    /**
     * @brief SERVER_KEY_STRING websocket's key string
     */
    constexpr static const char *const SERVER_KEY_STRING="Sec-WebSocket-Accept";
public:
    websocket_client(shared_ptr<tcp_connection_helper>helper,const string &ip,uint16_t port,const string &api="");
    ~websocket_client()override;
    /**
     * @brief send_websocket_data send the application's data by this websocket connection
     * @param buf
     * @param buf_len
     * @param type data's type
     * @return if connection is not set up,it will return false immediately
     */
    bool send_websocket_data(const void *buf,uint32_t buf_len,WS_Frame_Header::WS_FrameType type);
    /**
     * @brief set_extern_websocket_frame_callback if you want to process the websockt frame outside,you can set this callback to receive all websocket frames
     * @param cb
     */
    void set_extern_websocket_frame_callback(const WS_FRAME_CALLBACK &cb){
        lock_guard<mutex>locker(m_mutex);
        m_extern_ws_recv_frame_callback=cb;
    }
    /**
     * @brief set_ws_connection_success_callback if you are sensitive to the connection's set-up time,you can set this callback to receive the websocket connection's success set-up status
     * @param cb
     */
    void set_ws_connection_success_callback(const CONNECTION_SUCCESS_CALLBACK &cb){
        lock_guard<mutex>locker(m_mutex);
        m_ws_connection_success_callback=cb;
    }
    /**
     * @brief websocket_init_callback make pair all function's call before a websocket connection is set up
     * @param client
     * @param cb
     * @param cb2
     */
    static void websocket_init_callback(weak_ptr<websocket_client>client,const WS_FRAME_CALLBACK &cb=nullptr,const CONNECTION_SUCCESS_CALLBACK &cb2=nullptr);
    /**
     * @brief ws_connection_is_setup get the connection's set-up status
     * @return
     */
    bool ws_connection_is_setup()const{
        lock_guard<mutex>locker(m_mutex);
        return m_connection_status==WS_CONNECTION_CONNECTED;
    }
    /**
     * @brief util_test test the member functions
     */
    static void util_test();
    void clear_ws_send_cache(){
        lock_guard<mutex>locker(m_mutex);
        m_send_cache->clear();
    }
protected:
    /**
     * @brief handle_read websocket_client's handle network data read
     * @return
     */
    bool handle_read()override;
    /**
     * @brief clear_usr_status if the connection is disconnected ,this function will be called
     */
    void clear_usr_status()override{
        m_connection_status=WS_CONNECTION_CONNECTING;
    }
    /**
     * @brief generate_session_key random generate a client session's key
     * @param ptr always use the this ptr
     * @return
     */
    static string generate_session_key(void *ptr=nullptr);
    /**
     * @brief key_is_match check the key that is returned by the websocket server
     * @param server_key the key that is returned by the websocket server
     * @return if it's match, it will return true
     */
    bool key_is_match(const string &server_key);
    /**
     * @brief handle_setup_packet handle the websocket's handshake packet
     * @return
     */
    bool handle_setup_packet();
    /**
     * @brief send_setup_packet send a handshake packet to  the remote websocket server
     */
    void send_setup_packet();
    /**
     * @brief usr_handle_websocket_frame deriverd classes can override this function to have its' own websocket frame handling
     * @param frame
     */
    virtual void usr_handle_websocket_frame(const WS_Frame &frame);
    /**
     * @brief check_is_send_cache_anyway if this function return true,websocket client won't check the connection status,it will add
     * the send message to send queue no matter the connection is connected or not
     * you can override it to customized your send rule
     * @return
     */
    virtual bool check_is_send_cache_anyway()const{
        return false;
    }
protected:
    /**
     * @brief m_extern_ws_recv_frame_callback when a new websocket frame is received,this function will be called
     */
    WS_FRAME_CALLBACK m_extern_ws_recv_frame_callback;
    /**
     * @brief m_connection_status the websocket connection status
     */
    WS_CONNECTION_STATUS m_connection_status;
    /**
     * @brief m_ws_connection_success_callback when the connection  is successfully established,this function will be called
     */
    CONNECTION_SUCCESS_CALLBACK m_ws_connection_success_callback;
    /**
     * @brief m_ws_session_key client's key that is generated when building it
     */
    const string m_ws_session_key;
    /**
     * @brief m_ws_recv_cache cache recv a websocket stream
     */
    shared_ptr<web_socket_buffer_cache>m_ws_recv_cache;
};
}
#endif // WEBSOCKET_CLIENT_H
