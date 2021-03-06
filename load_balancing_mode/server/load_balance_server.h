#ifndef LOAD_BANLANCE_SERVER_H
#define LOAD_BANLANCE_SERVER_H
#include "event_loop.h"
#include "rc4_interface.h"
#include "CJsonObject.hpp"
namespace micagent {
using namespace std;
class load_balance_server{
    static constexpr int64_t MAX_CHACHE_TIME=30*1000;//30s
    static constexpr int64_t SESSION_CHECK_INTERVAL=10*1000;//10s
    static constexpr double TWO_POW_32=static_cast<int64_t>(1)<<32;
    static constexpr double TWO_POW_24=static_cast<int64_t>(1)<<24;
    static constexpr double TWO_POW_16=static_cast<int64_t>(1)<<16;
    static constexpr double TWO_POW_8=static_cast<int64_t>(1)<<8;
    static constexpr double IP_BASE=TWO_POW_32+TWO_POW_24+TWO_POW_16+TWO_POW_8;
    struct session_info{
        string account;
        string domain_name;
        string ip;
        /*实时负载率0.0 -1.0*/
        double priority;
        /*权重0.0-1.0*/
        double weight;
        int64_t last_alive_time;
        session_info(string _account="",string _domain_name=""):account(_account),domain_name(_domain_name)\
          ,ip("0.0.0.0"),priority(0.0),weight(0.0),last_alive_time(Timer::getTimeNow()){

        }
    };
    struct account_session{
        map<string,session_info> session_map;
    };

public:
    load_balance_server(uint16_t port,int64_t cache_time=MAX_CHACHE_TIME);
    void config(weak_ptr<EventLoop>loop);
    void start_work();
    void stop_work();
    ~load_balance_server();
    static double calculate_priority(const string &ip1,const string&ip2);
private:
    void handle_read();
    void handle_specific_find(neb::CJsonObject&object);
    void handle_find(neb::CJsonObject&object);
    void handle_update(neb::CJsonObject&object);
    void check_sessions();
    void response(const string &buf);
private:
    weak_ptr<EventLoop>m_event_loop;
    int64_t m_max_cache_time;
    TimerId m_update_timer;
    ChannelPtr m_channel;
    map<string,account_session> m_session_cache_map;
    mutex m_mutex;
    sockaddr_in m_last_recv_addr;
    socklen_t m_sock_len;
    bool m_is_running;
};
}
#endif // LOAD_BANLANCE_SERVER_H
