#ifndef DNS_CLIENT_H
#define DNS_CLIENT_H
#include "event_loop.h"
#include "CJsonObject.hpp"
#include "rc4_interface.h"
#define TO_STRING(x) #x
namespace micagent {
using namespace std;
class dns_client{
    /**
     * @brief TIME_OUT_TIME time out time when try to recv a message
     */
    static constexpr int64_t TIME_OUT_TIME=5*1000;//5s
    /**
     * @brief UNPDATE_INTERVAL the interval that upload itself's info
     */
    static constexpr int64_t UNPDATE_INTERVAL=60*1000;//60s
    /**
     * @brief The port_info struct  port nap info
     */
    struct port_info{
        port_info(string _name="",uint16_t _internal_port=0,uint16_t _external_port=0):name(_name),internal_port(_internal_port),external_port(_external_port){

        }
        /**
         * @brief name application's name
         */
        string name;
        uint16_t internal_port;
        uint16_t external_port;
    };
public:
    /**
     * @brief dns_client init a dns_client
     * @param server_ip the dns_server's ip
     * @param dns_port the dns_server's port
     */
    dns_client(string server_ip,uint16_t dns_port);
    /**
     * @brief config  call this when it works as a dns info uploader
     * @param loop envent loop
     * @param domain_name it's own domain name
     * @param account access account
     * @param password access password
     */
    void config(weak_ptr<EventLoop>loop,string domain_name,string account,string password,int64_t upload_interval=UNPDATE_INTERVAL);
    /**
     * @brief reset_server_info
     * @param server_ip new dns_server ip
     * @param dns_port new dns server port
     */
    void reset_server_info(string server_ip,uint16_t dns_port);
    /**
     * @brief start_work call this when it works as a dns info uploader
     */
    void start_work();
    /**
     * @brief stop_work call this when it works as a dns info uploader
     */
    void stop_work();
    /**
     * @brief set_port_map_item add a port map item,call this when it works as a dns info uploader
     * @param name appilcation's name
     * @param external_port external port on the router
     * @param internal_port internal port on the equipment
     */
    void set_port_map_item(string name,uint16_t external_port,uint16_t internal_port);
     /**
      * @brief set_user_account_info 配置账户及密码
      * @param account_name
      * @param account_password
      */
     void set_user_account_info(string account_name,string account_password);
    /**
     * @brief register_to_server call this when it works as a dns info uploader
     * @param domain_name it's own domain_name
     * @param account access accout
     * @param password access password
     * @param other_info other_info can be used to specify a domain_name
     * @param time_out usually,it's set TIME_OUT_TIME
     * @return
     */
    pair<bool,neb::CJsonObject> register_to_server(string domain_name,string account,string password,string other_info="",int64_t time_out=TIME_OUT_TIME);
    /**
     * @brief unregister_to_server ununregister from dns server
     * @param domain_name it's own domain_name
     * @param password access password
     * @param time_out
     * @return
     * @details call this when it works as a dns info uploader
     */
    pair<bool,neb::CJsonObject>  unregister_to_server(string domain_name,string password,int64_t time_out=TIME_OUT_TIME);
    /**
     * @brief account_find get the domain_name's account's ascription
     * @param domain_name that you want to access
     * @param time_out
     * @return
     */
    pair<bool,neb::CJsonObject>  account_find(string domain_name,int64_t time_out=TIME_OUT_TIME);
    /**
     * @brief dns_find get a domain's info
     * @param domain_name the domain_name you want access
     * @param account access account
     * @param password access password
     * @param time_out
     * @return
     */
    pair<bool,neb::CJsonObject>  dns_find(string domain_name,string account,string password,int64_t time_out=TIME_OUT_TIME);
    /**
     * @brief port_check you can  use this to get your own external ip
     * @param time_out
     * @return
     */
    pair<bool,neb::CJsonObject> port_check(int64_t time_out=TIME_OUT_TIME);
    ~dns_client(){
    auto event_loop=m_loop.lock();
    if(m_is_running&&event_loop)event_loop->removeTimer(m_timer_id);
        if(m_send_fd!=INVALID_SOCKET)Network_Util::Instance().close_socket(m_send_fd);
    }
private:
    void update();
    void rc4_send(SOCKET fd,const string &buf,const sockaddr_in *addr=nullptr);
    /**
     * @brief read_packet timeout to read a udp packet
     * @param fd io descriptor
     * @param buf recv buf
     * @param buf_len max recv len
     * @param time_out  max wait time to read
     * @return actual read size
     */
    ssize_t read_packet(SOCKET fd,void *buf,int buf_len,int64_t time_out=TIME_OUT_TIME)
    {
        bool can_read=true;
        if(time_out>0){
            Network_Util::Instance().make_noblocking(fd);
             fd_set fdRead;
            FD_ZERO(&fdRead);
            FD_SET(fd, &fdRead);
           struct timeval tv = { static_cast<__time_t>(time_out / 1000), static_cast<__suseconds_t>(time_out % 1000 * 1000 )};
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
    weak_ptr<EventLoop>m_loop;
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
    int64_t m_upload_interval;
    string m_user_account_name;
    string m_user_account_password;
};
}


#endif // DNS_CLIENT_H
