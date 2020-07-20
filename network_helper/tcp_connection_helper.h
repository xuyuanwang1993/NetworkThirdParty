#ifndef TCP_CONNECTION_HELPER_H
#define TCP_CONNECTION_HELPER_H
#include "event_loop.h"
namespace micagent {
using namespace std;
typedef enum{
    CONNECTION_SUCCESS,
    CONNECTION_FAILED,
    CONNECTION_TIME_OUT,
    CONNECTION_SYS_ERROR,
}CONNECTION_STATUS;
using CONNECTION_CALLBACK=function<void(CONNECTION_STATUS,SOCKET)>;

class tcp_connection_helper{
public:
    static tcp_connection_helper *CreateNew(EventLoop *loop);
    void OpenConnection(string ip,uint16_t port,CONNECTION_CALLBACK callback,uint32_t time_out_ms=30);
    ~tcp_connection_helper(){}
    EventLoop *get_loop()const{return m_loop;}
private:
    tcp_connection_helper(EventLoop *loop=nullptr):m_loop(loop){}
    EventLoop *m_loop;
};
}
#endif // TCP_CONNECTION_HELPER_H
