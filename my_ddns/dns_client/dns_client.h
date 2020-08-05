#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H
#include "event_loop.h"
#include "CJsonObject.hpp"
#include "rc4_interface.h"
#define TO_STRING(x) #x
namespace micagent {
using namespace std;
class dns_client{
    static constexpr int64_t TIME_OUT_TIME=5*1000;//5s
    static constexpr int64_t UNPDATE_INTERVAL=60*1000;//60s
    struct port_info{
        port_info(string _name="",uint16_t _internal_port=0,uint16_t _external_port=0):name(_name),internal_port(_internal_port),external_port(_external_port){

        }
        string name;
        uint16_t internal_port;
        uint16_t external_port;
    };
public:
    dns_client(string server_ip,uint16_t dns_port);
    void config(EventLoop *loop,string domain_name,string account,string password);
    void reset_server_info(string server_ip,uint16_t dns_port);
    void start_work();
    void stop_work();
    void set_port_map_item(string name,uint16_t external_port,uint16_t internal_port);
    pair<bool,neb::CJsonObject> register_to_server(string domain_name,string account,string password,string other_info="",int64_t time_out=TIME_OUT_TIME);
    pair<bool,neb::CJsonObject>  unregister_to_server(string domain_name,string password,int64_t time_out=TIME_OUT_TIME);
    pair<bool,neb::CJsonObject>  account_find(string domain_name,int64_t time_out=TIME_OUT_TIME);
    pair<bool,neb::CJsonObject>  dns_find(string domain_name,string account,string password,int64_t time_out=TIME_OUT_TIME);
    pair<bool,neb::CJsonObject> port_check(int64_t time_out=TIME_OUT_TIME);
    ~dns_client(){if(m_is_running)m_loop->removeTimer(m_timer_id);
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
    map<string,port_info>m_port_map;
    string m_server_ip;
    uint16_t m_server_port;
    sockaddr_in m_server_addr;
    bool m_is_running;
    string m_domain_name;
    string m_account;
    string m_password;
    SOCKET m_send_fd;
};
}


#endif // DNS_CLIENT_H
