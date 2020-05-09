#pragma once
#ifndef KCP_MANAGE_H
#define KCP_MANAGE_H
#include <functional>
#include <memory>
#include "ikcp.h"
#include <atomic>
#include <unordered_map>
#include"event_loop.h"
#include "c_log.h"
/**
 * @brief IKCP_MTU_DEF KCP MTU分片大小
 */
extern IUINT32 IKCP_MTU_DEF;
/**
 * @brief IKCP_WND_RCV 默认接收窗口大小
 */
extern IUINT32 IKCP_WND_RCV;
namespace micagent{
/**
 * @brief RecvDataCallback收到应用层数据帧所调用的函数
 * @param buf 数据缓冲区首地址
 * @param len 缓冲区长度
 * @param kcp kcp对象，kcp对象中存储了kcp_interface this指针
 * @param user_data 创建kcp_interface 时传入的用户指针
 */
typedef std::function<void(const char *buf ,int len ,struct IKCPCB *kcp,void *user_data)> RecvDataCallback;
enum KCP_TRANSFER_MODE{
    DEFULT_MODE,//默认模式,不启用加速，内部时钟10ms 禁止快速重传，启用流控
    NORMAL_MODE,//常规模式，不启用加速，内部时钟10ms 禁止快速重传，关闭流控
    FAST_MODE,//快速模式,启用加速,内部时钟10ms,快速重传,关闭流控
    CUSTOM_MODE,//自定义模式，时钟不要小于5ms 不要大于帧间隔 接收端不要更改传输模式
};
#define DEFAULT_FRAME_SIZE 20000
class kcp_interface:public enable_shared_from_this<kcp_interface>{
    /**
     * @brief MAX_TIMEOUT_TIME 连接超时时间,默认5s
     */
    static int MAX_TIMEOUT_TIME;//5s
    /**
     * @brief MAX_FRAMESIZE 最大允许的单帧大小
     */
    static int MAX_FRAMESIZE;//1024*1024
    static constexpr uint8_t MAX_TIME_OUT_CNT=3;//
    struct kcp_save
    {
        kcp_save(struct IKCPCB *_kcp) :m_kcp(_kcp){}
        struct IKCPCB * kcp_ptr()const {return m_kcp;}
        ~kcp_save(){
            if(m_kcp)ikcp_release(m_kcp);
        }
        struct IKCPCB * m_kcp;
    };
public:
    /**
     * @brief kcp_interface 构造函数
     * @param fd 调用了connect 的udp socket
     * @param conv_id 会话id
     * @param loop 外部传入的事件循环指针
     * @param recvCB 接收回调函数
     * @param userdata 用户数据
     * @param clearCB 清理回调函数
     * @param max_frame_size 最大数据帧大小
     */
    kcp_interface(SOCKET fd,uint32_t conv_id,EventLoop *loop,RecvDataCallback recvCB,void *userdata=nullptr,uint32_t max_frame_size=DEFAULT_FRAME_SIZE);
    /**
     * @brief change_time_out_time 修改会话超时时间，线程不安全
     * @param time_out_ms 超时时间
     */
    static void change_time_out_time(int time_out_ms){MAX_TIMEOUT_TIME=time_out_ms;}
    /**
     * @brief change_max_framesize 修改最大数据帧大小，线程不安全
     * @param frame_size 数据帧大小
     */
    static void change_max_framesize(int frame_size){MAX_FRAMESIZE=frame_size;}
    /**
     * @brief send_userdata 发送应用层数据
     * @param buf
     * @param len
     * @return success 0
     */
    int send_userdata(const char *buf,int len);
    /**
     * @brief raw_send 发送协议数据，不要在外部调用此函数
     * @param buf
     * @param len
     * @return
     */
    int raw_send(const char *buf,int len);
    /*获取会话id*/
    uint32_t conv_id()const{return m_conv_id;}
    /*加入事件循环*/
    void start_work();
    void exit_work();
    /**
     * @brief SetTransferMode 设置传输模式
     * @param mode 传输模式
     * @param nodelay 是否延时重传 为0代表默认最小时延100ms，不为0则为30ms 用于计算重传时间戳
     * @param interval 内部时钟间隔 最小10ms 最大5000ms
     * @param resend 重传缓冲区长度
     * @param nc 拥塞控制 开关
     */
    void SetTransferMode(KCP_TRANSFER_MODE mode,int nodelay=0, int interval=10, int resend=0, int nc=0);
    ~kcp_interface();
private:
    friend class kcp_manager;
    bool update(int64_t time_now=Timer::getTimeNow()){
        lock_guard<mutex>locker(m_mutex);
        ikcp_update(m_kcp->kcp_ptr(),(IUINT32)time_now);
        if(time_now-m_last_alive_time>MAX_TIMEOUT_TIME){
            m_timeout_times++;
            m_last_alive_time=time_now;
        }
        else {
            m_timeout_times=0;
        }
        return m_timeout_times<MAX_TIME_OUT_CNT;
    }
    /**
     * @brief m_udp_channel UDP通道
     */
    std::shared_ptr<Channel> m_udp_channel;
    /**
     * @brief m_kcp kcp对象
     */
    shared_ptr<kcp_save>m_kcp;
    /**
     * @brief m_conv_id 存储会话id
     */
    uint32_t m_conv_id;
#ifdef DEBUG
    mutable mutex m_debug_mutex;
#endif
    /**
     * @brief m_loop 存储事件循环
     */
    EventLoop *m_loop;
    /**
     * @brief m_recvCB 接收回调函数
     */
    RecvDataCallback m_recvCB;
    void * m_data;//用户数据
    /**
     * @brief m_last_alive_time 上次存活时间
     */
    int64_t m_last_alive_time;
    /**
     * @brief m_timeout_times 超时次数，3次代表断开连接
     */
    uint8_t m_timeout_times;
    /**
     * @brief m_frame_size 当前会话最大帧长度
     */
    uint32_t m_frame_size;
    mutex m_mutex;
};

class kcp_manager{
public:
    /**
     * @brief GetInstance 获取kcp_manager单例的引用
     * @return
     */
    static kcp_manager &GetInstance()
    {//单例模式
        static kcp_manager manager;
        return manager;
    }
    /**
     * @brief Config 配置事件循环
     * @param event_loop 外部事件循环指针
     */
    void Config(EventLoop *event_loop)
    {
        if(!get_init_status()&&event_loop)
        {
            {
                DEBUG_LOCK
                m_init.exchange(true);
            }
            lock_guard<mutex>locker(m_mutex);
            m_event_loop=event_loop;
        }
    }
    /**
     * @brief AddConnection 对调用了connect的udp socket添加kcp会话
     */
    std::shared_ptr<kcp_interface> AddConnection(SOCKET fd,uint32_t conv_id,RecvDataCallback recvCB,void *m_data=nullptr,uint32_t max_frame_size=DEFAULT_FRAME_SIZE);
    /**
     * @brief AddConnection 创建kcp会话
     * @param send_port 本地发送端口
     * @param ip 对端接收ip
     * @param port 对端接收端口
     * @return  kcp_interface指针
     */
    std::shared_ptr<kcp_interface> AddConnection(uint16_t send_port,std::string ip,uint16_t port,uint32_t conv_id,RecvDataCallback recvCB,void *m_data=nullptr,uint32_t max_frame_size=DEFAULT_FRAME_SIZE);
    /**
     * @brief GetConnectionHandle 根据fd获取kcp_interface对象指针
     * @param fd
     * @return
     */
    std::shared_ptr<kcp_interface>GetConnectionHandle(SOCKET fd)
    {
        if(!get_init_status())return nullptr;
        std::lock_guard<std::mutex> locker(m_mutex);
        auto iter=m_kcp_map.find(fd);
        if(iter!=std::end(m_kcp_map))return iter->second;
        return nullptr;
    }
    /**
     * @brief CloseConnection 根据fd关闭kcp会话
     * @param fd
     */
    void CloseConnection(SOCKET fd)
    {//移除连接
        if(!get_init_status())return ;
        std::lock_guard<std::mutex> locker(m_mutex);
        auto iter=m_kcp_map.find(fd);
        if(iter!=std::end(m_kcp_map))
        {
            auto conv_id=iter->second->conv_id();
            auto count=m_conv_map.count(conv_id);
            if(count>0){
                auto connect_iter=m_conv_map.find(conv_id);
                while(count!=0){
                    count--;
                    auto kcp_iter=m_kcp_map.find(connect_iter->second);
                    if(kcp_iter!=m_kcp_map.end()){
                        kcp_iter->second->exit_work();
                        m_kcp_map.erase(kcp_iter);
                    }
                    connect_iter++;
                }
                m_conv_map.erase(conv_id);
            }
        }
    }
    /**
     * @brief CloseConnection 根据会话id关闭kcp会话
     * @param conv_id
     */
    void CloseConnection(uint32_t conv_id)
    {//移除连接
        if(!get_init_status())return ;
        std::lock_guard<std::mutex> locker(m_mutex);
        auto count=m_conv_map.count(conv_id);
        if(count>0){
            auto connect_iter=m_conv_map.find(conv_id);
            while(count!=0){
                count--;
                auto kcp_iter=m_kcp_map.find(connect_iter->second);
                    if(kcp_iter!=m_kcp_map.end()){
                        kcp_iter->second->exit_work();
                        m_kcp_map.erase(kcp_iter);
                    }
                    connect_iter++;
            }
            m_conv_map.erase(conv_id);
        }
    }
    /**
     * @brief StartUpdateLoop 开启Update事件循环
     */
    void StartUpdateLoop();
    void StopUpdateLoop();
    bool get_init_status()const{
        DEBUG_LOCK
        return m_init;
    }
private:
    /**
     * @brief UpdateLoop 循环任务
     * @return
     */
    bool UpdateLoop();
    std::atomic<bool> m_init;
    kcp_manager():m_loop_timer(0){}
    ~kcp_manager(){}
    std::mutex m_mutex;
    EventLoop *m_event_loop;
    /**
     * @brief m_conv_map 会话id:SOCKET 映射表
     */
    multimap<uint32_t,SOCKET>m_conv_map;
    /**
     * @brief m_kcp_map 保存所有的kcp会话
     */
    std::unordered_map<SOCKET,std::shared_ptr<kcp_interface>> m_kcp_map;
#ifdef DEBUG
    mutable mutex m_debug_mutex;
#endif
    TimerId m_loop_timer;
};
};
#endif // KCP_MANAGE_H
