#pragma once
#ifndef TEST_SERVER_H
#define TEST_SERVER_H
#define TEST_PORT 10000
#include "event_loop.h"
#include <map>
namespace micagent {
using namespace std;
/**
 * @brief The test_server class IO_CORE测试类
 */
class test_server{
    typedef enum{
        START=0,
        ADD_TRIGGER,
        ADD_TIMER,
        DELETE_TIMER,
        ADD_THREAD,
        CANCEL_THREAD,
        ADD_LISTEN_WORK,
        DELETE_LISTEN_WORK,
        ADD_ACTIVE_CONNECT,
        DELETE_ACTIVE_CONNECT,
        ADD_UDP_SEND_SERVER,
        DELETE_UDP_SEND_SERVER,
        ADD_UDP_SEND_TASK,
        DELETE_UDP_SEND_TASK,
        EXIT,
        HELP,
        END,
    }CMD_TYPE;
    struct run_status{
        mutex m_mutex;
        bool status;
        run_status(bool first=true):status(first){}
        bool get_status(){
            lock_guard<mutex>locker(m_mutex);
            return status;
        }
        void change(){
            lock_guard<mutex>locker(m_mutex);
            status=!status;
        }
    };

public:
    test_server()=delete;
    /**
     * @brief test_server
     * @param thread_pool_size 线程池大小
     * @param trigger_threads trigger_queue工作线程数
     * @param capacity trigger_queue工作容器容量
     * @param thread_nums IO处理的线程数
     */
    test_server(int32_t thread_pool_size,uint32_t trigger_threads,uint32_t capacity,uint32_t thread_nums);
    ~test_server();
    /**
     * @brief run 阻塞等待退出信号
     */
    void run(){
    while(getchar()!='8')continue;}
    /**
     * @brief print_cmd_help 获取帮助信息
     */
    static void print_cmd_help();
private:
    /**
     * @brief handle_read 处理接收到的数据
     * @param fd 通信fd
     * @param buf 接收缓存
     * @param buf_len 缓存大小
     * @param addr 对端地址信息，若为空代表是tcp连接
     */
    void handle_read(SOCKET fd,const char *buf,uint32_t buf_len,const sockaddr_in *addr);
    void handle_add_trigger(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_add_timer(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_delete_timer(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_add_thread(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_cancel_thread(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_add_listen_work(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_delete_listen_work(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_active_connect(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_delete_active_connect(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_add_udp_send_server(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_delete_udp_send_server(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_add_udp_send_task(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_delete_udp_send_task(SOCKET fd,queue<string>&param,const sockaddr_in *addr);
    void handle_exit();
    void handle_help(SOCKET fd,const sockaddr_in *addr);
    inline void response(SOCKET fd,const sockaddr_in *addr,const string &ack){
        if(ack.empty())return;
        if(addr){
            sendto(fd,ack.c_str(),ack.length(),0,(const sockaddr *)addr,sizeof (sockaddr_in));
        }
        else {
            send(fd,ack.c_str(),ack.length(),0);
        }
    }
    //将输入按字符串拆分
    queue<string>parase_buf(const char *buf,uint32_t buf_len);
private:
    //指令map
    map<string,int>m_cmp_type;
    //事件循环实例
    shared_ptr<EventLoop> m_event_loop;
    //数据读写锁
    mutex m_mutex;
    condition_variable m_conn;
    ChannelPtr m_udp_handle;
    ChannelPtr m_tcp_handle;
    uint32_t m_session_id;
    //线程控制量map
    map<uint32_t,shared_ptr<run_status>>m_thread_state;
    map<uint32_t,ChannelPtr> m_listen_server_map;
    struct udp_send_server{
        ChannelPtr udp_send_server;
        map<uint32_t,sockaddr_in >m_recv_map;
        uint32_t timer_id;
        mutex m_mutex;
    };
    map<uint32_t,shared_ptr<udp_send_server>>m_udp_server_map;
    map<uint32_t,uint32_t>m_udp_task_map;
};
}


#endif // TEST_SERVER_H
