#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H
#include "event_loop.h"
#include "CJsonObject.hpp"
#include "rc4.h"
#define TO_STRING(x) #x
namespace micagent {
using namespace std;
class load_balance_client{
    static constexpr int64_t TIME_OUT_TIME=5*1000;//5s
    static constexpr int64_t UNPDATE_INTERVAL=2*1000;//2s
public:
    load_balance_client();
    void config_server_info(EventLoop *loop,string server_ip,uint16_t server_port);
    void config_client_info(string domain_name,uint32_t max_load_size,double weight=0.5);
    void increase_load(uint32_t load_size);
    void decrease_load(uint32_t load_size);
    void start_work();
    void stop_work();
    pair<bool,neb::CJsonObject>  specific_find(string domain_name,int64_t time_out=TIME_OUT_TIME);
    pair<bool,neb::CJsonObject>  find(int64_t time_out=TIME_OUT_TIME);
    ~load_balance_client(){
        if(m_is_running)m_loop->removeTimer(m_timer_id);
        if(m_send_fd!=INVALID_SOCKET)Network_Util::Instance().close_socket(m_send_fd);
    }
private:
    void update();
    void rc4_send(SOCKET fd,const string &buf);
    ssize_t read_packet(SOCKET fd,void *buf,int buf_len,int64_t time_out=TIME_OUT_TIME)
    {
        bool can_read=true;
        if(time_out>0){
            Network_Util::Instance().make_noblocking(fd);
             fd_set fdRead;
            FD_ZERO(&fdRead);
            FD_SET(fd, &fdRead);
            struct timeval tv = { time_out / 1000, time_out % 1000 * 1000 };
            select(fd + 1,  &fdRead,nullptr, nullptr, &tv);
            if (!FD_ISSET(fd, &fdRead))
            {
                can_read=false;
            }
            Network_Util::Instance().make_blocking(fd);
        }
        if(can_read)return recv(fd,buf,buf_len,0);
        else return -1;
    }
private:
    mutex m_mutex;
    EventLoop *m_loop;
    TimerId m_timer_id;
    bool m_is_running;
    string m_server_ip;
    uint16_t m_server_port;
    sockaddr_in m_server_addr;
    string m_domain_name;
    double m_weight;
    uint32_t m_max_load_size;
    uint32_t m_now_load;
    SOCKET m_send_fd;
};
}


#endif // DNS_CLIENT_H
