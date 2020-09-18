#ifndef DNS_SERVER_H
#define DNS_SERVER_H
#include "event_loop.h"
#include "rc4_interface.h"
#include "CJsonObject.hpp"
namespace micagent {
using namespace std;
class dns_server{
    /**
     * @brief MAX_CHACHE_TIME max time a time out dns cache will be saved
     */
    static constexpr int64_t MAX_CHACHE_TIME=5*60*1000;//5min
    /**
     * @brief SESSION_CHECK_INTERVAL check_interval for session int uint ms
     */
    static constexpr int64_t SESSION_CHECK_INTERVAL=30*1000;//30s
    /*database operation*/
    /**
     * @brief The port_info struct describe a port map info
     */
    struct port_info{
        port_info():name(""),internal_port(0),external_port(0){

        }
        /**
         * @brief name application's name
         */
        string name;
        uint16_t internal_port;
        uint16_t external_port;
    };
    /**
     * @brief The data_session struct
     */
    struct data_session{
        uint32_t data_base_id;
        string domain_name;
        string account;
        string password;//md5
        uint32_t search_count;
        string other_info;
        int64_t last_login_time;
        data_session():data_base_id(0),domain_name(""),account(""),password(""),\
        search_count(0),other_info(""),last_login_time(0){

        }
    };

    struct session_info{
        data_session session;
        string external_ip;
        string internal_ip;
        map<string,port_info>port_map;
        int64_t last_alive_time;
        string user_account_name;
        string user_account_password;
        session_info():last_alive_time(Timer::getTimeNow()),user_account_name(""),user_account_password(""){

        }
    };
    struct data_base{
        data_base(){}
        uint32_t m_data_base_size;
        map<string,data_session>m_session_map;
        mutex m_mutex;
        void update_dns_server_session(data_session &session){
            lock_guard<mutex>locker(m_mutex);
            auto iter=m_session_map.find(session.domain_name);
            if(iter!=m_session_map.end())iter->second=session;
            else {
                session.data_base_id=m_data_base_size++;
                auto ret=m_session_map.emplace(session.domain_name,session);
            }
        }
        bool find_dns_server_session(const string &domain_name,data_session &session){
            lock_guard<mutex>locker(m_mutex);
            auto iter=m_session_map.find(domain_name);
            if(iter!=m_session_map.end()){
                iter->second.search_count++;
                session=iter->second;
            }
            return iter!=m_session_map.end();
        }
        void remove_dns_server_session(const string &domain_name){
            lock_guard<mutex>locker(m_mutex);
            auto iter=m_session_map.find(domain_name);
            if(iter!=m_session_map.end())m_session_map.erase(iter);
        }
    };
    /*database operation end*/
public:
    /**
     * @brief dns_server init a dns server
     * @param port work port
     * @param cache_time session's cache name
     */
    dns_server(uint16_t port,int64_t cache_time=MAX_CHACHE_TIME);
    /**
     * @brief config set the base event loop
     * @param loop
     */
    void config(weak_ptr<EventLoop>loop);
    /**
     * @brief start_work add task into event loop
     */
    void start_work();
    /**
     * @brief stop_work remove task from event loop
     */
    void stop_work();
    ~dns_server();
private:
    void handle_read();
    void handle_register(neb::CJsonObject &object);
    void handle_unregister(neb::CJsonObject &object);
    void handle_account_find(neb::CJsonObject&object);
    void handle_dns_find(neb::CJsonObject&object);
    void handle_update(neb::CJsonObject&object);
    void handle_port_check();
    /**
     * @brief check_sessions judge and remove time out session
     */
    void check_sessions();
    /**
     * @brief response response the buf int rc4
     * @param buf
     */
    void response(const string &buf);
private:
    static data_base m_data_base;
    weak_ptr<EventLoop>m_event_loop;
    int64_t m_max_cache_time;
    TimerId m_update_timer;
    ChannelPtr m_channel;
    map<string,session_info> m_session_cache_map;
    mutex m_mutex;
    sockaddr_in m_last_recv_addr;
    socklen_t m_sock_len;
    bool m_is_running;
};
}
#endif // DNS_SERVER_H
