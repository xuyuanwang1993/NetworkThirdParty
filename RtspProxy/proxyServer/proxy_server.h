#ifndef PROXY_SERVER_H
#define PROXY_SERVER_H
#include "rtsp_server.h"
#include "proxy_protocol.h"
#include <unordered_map>
namespace micagent {
using namespace std;
class proxy_connection;
class proxy_server:public tcp_server{
    //TCP连接无数据通讯超时断开时间
    static constexpr int64_t CONNECTION_TIME_OUT=120*1000;//120s
    friend class proxy_connection;
public:
    proxy_server(uint16_t listen_port=8555,uint32_t netinterface_index=UINT32_MAX);
    //传递rtsp_server弱引用，初始化时调用
    void set_rtsp_server(shared_ptr<rtsp_server>server){m_w_rtsp_server=server;}
    ~proxy_server();
    //创建转发连接
    shared_ptr<tcp_connection>new_connection(SOCKET fd);
private:
    //移除超时连接的处理函数
    void remove_invalid_connection();
    //保存定时器id 用于下次移除
    TimerId m_remove_timer_id;
    //初始化tcp_server时额外操作
    void init_server(){
        auto event_loop=m_loop.lock();
        if(!event_loop)return;
        //初始化连接检查定时器任务
        if(m_remove_timer_id!=INVALID_TIMER_ID){
            event_loop->removeTimer(m_remove_timer_id);
        }
        weak_ptr<tcp_server>weak_this(shared_from_this());
        m_remove_timer_id=event_loop->addTimer([weak_this](){
            auto strong=weak_this.lock();
            if(!strong)return false;
            auto server=dynamic_cast<proxy_server*>(strong.get());
            if(server)server->remove_invalid_connection();
            return true;
        },CONNECTION_TIME_OUT);
        //初始化udp接收channel
        if(m_udp_channel){
            m_udp_channel->setReadCallback([weak_this](Channel *chn){

                auto strong_ptr=weak_this.lock();
                auto strong=dynamic_cast<proxy_server *>(strong_ptr.get());
                do{
                    if(!strong)break;
                    shared_ptr<char>buf(new char[1500],std::default_delete<char[]>());
                    auto ret=recvfrom(chn->fd(),buf.get(),1500,0,nullptr,nullptr);
                    if(ret>0)
                    {
                        lock_guard<mutex>locker(strong->m_data_mutex);
                        strong->m_data_queue.push(make_pair(buf,ret));
                        strong->m_data_cv.notify_one();
                    }
                }while(0);
                return true;
            });
            m_udp_channel->enableReading();
            event_loop->updateChannel(m_udp_channel);
        }
        //初始化udp数据处理线程
        if(!m_udp_data_handle_thread)
        {
            m_udp_data_handle_thread.reset(new thread([weak_this](){
                auto strong_ptr=weak_this.lock();
                auto strong=dynamic_cast<proxy_server *>(strong_ptr.get());
                unique_lock<mutex>locker(strong->m_data_mutex);
                while(1)
                {//当外部引用为0时，退出线程
                    if(strong_ptr.use_count()<=1)break;
                    if(strong->m_data_queue.empty())strong->m_data_cv.wait(locker);
                    while(!strong->m_data_queue.empty())
                    {
                        strong->udp_data_input(strong->m_data_queue.front());
                        strong->m_data_queue.pop();
                    }
                }
            }));
        }
    }

    //udp数据处理
    void udp_data_input(const pair<shared_ptr<char>,uint16_t>&buf_pair);
    //添加udp发送索引
    void add_udp_map(uint32_t stream_token,SOCKET fd)
    {
        lock_guard<mutex>locker(m_mutex);
        m_udp_connections_map.emplace(stream_token,fd);
    }
    //移除udp发送索引
    void remove_udp_map(uint32_t stream_token)
    {
        m_udp_connections_map.erase(stream_token);
    }
    //udp数据通道
    ChannelPtr m_udp_channel;
    mutex m_data_mutex;
    condition_variable m_data_cv;
    //数据队列
    queue<pair<shared_ptr<char >,uint16_t>>m_data_queue;
    shared_ptr<thread>m_udp_data_handle_thread;
    //rtsp数据处理接口
    //创建流 删除流 推送数据
    weak_ptr<rtsp_server>m_w_rtsp_server;
    //存放stream_token----fd------tcp_connection关系
    unordered_map<uint32_t,SOCKET>m_udp_connections_map;
};
}
#endif // PROXY_SERVER_H
