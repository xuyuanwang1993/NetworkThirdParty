#ifndef TCP_CONNECTION_HELPER_H
#define TCP_CONNECTION_HELPER_H
#include "event_loop.h"
namespace micagent {
using namespace std;
typedef enum{
    CONNECTION_SUCCESS,//success
    CONNECTION_FAILED,//failed probably caused by the peer is not at work
    CONNECTION_TIME_OUT,//time out can't connect to peer in a set duration
    CONNECTION_SYS_ERROR,//system error that means you device can't work normally
}CONNECTION_STATUS;
//define a connection callback
using CONNECTION_CALLBACK=function<void(CONNECTION_STATUS,SOCKET)>;

class tcp_connection_helper{
public:
    /**
     * @brief CreateNew create a instance
     * @param loop event loop
     * @return
     */
    static tcp_connection_helper *CreateNew(weak_ptr<EventLoop>loop);
    /**
     * @brief OpenConnection create a tcp connection
     * @param ip peer's ip
     * @param port peer'port
     * @param callback
     * @param time_out_ms max connection wait time
     */
    void OpenConnection(string ip,uint16_t port,CONNECTION_CALLBACK callback,uint32_t time_out_ms=30);
    ~tcp_connection_helper(){}
    weak_ptr<EventLoop>get_loop()const{return m_loop;}
private:
    tcp_connection_helper(weak_ptr<EventLoop>loop=weak_ptr<EventLoop>()):m_loop(loop){}
    weak_ptr<EventLoop>m_loop;
};
}
#endif // TCP_CONNECTION_HELPER_H
