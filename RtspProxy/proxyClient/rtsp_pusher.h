#ifndef RTSP_PUSHER_H
#define RTSP_PUSHER_H
#include "media_session.h"
#include "tcp_connection_helper.h"
#include "proxy_protocol.h"
#include "CJsonObject.hpp"
#include "proxybufhandle.h"
namespace micagent {
using namespace std;
using neb::CJsonObject;
class rtsp_pusher:public proxy_session_base ,public enable_shared_from_this<rtsp_pusher>{
//连接最大超时时间
    static constexpr uint32_t MAX_WAIT_TIME=30*1000;//30s
//连接最小超时时间
    static constexpr uint32_t MIN_WAIT_TIME=5000;//5s
public:
    static rtsp_pusher *CreateNew(weak_ptr<tcp_connection_helper>helper,PTransMode mode,const string &des_name,const string &des_ip ,uint16_t des_port,const string &user_name="",const string &password="")
    {
        return new rtsp_pusher(helper,mode,des_name,des_ip,des_port,user_name,password);
    }
    //输入转发数据
    void proxy_frame(MediaChannelId id,const AVFrame &frame);
    //开启连接
    void open_connection(const vector<media_source_info>&media_source_info);
    //关闭连接
    void close_connection();
    //修改目标ip 端口信息
    void reset_net_info(string ip,uint16_t des_port);
private:
    rtsp_pusher(weak_ptr<tcp_connection_helper>helper,PTransMode mode,const string &des_name,const string &des_ip ,uint16_t des_port,const string &user_name,const string &pass_word);
    //解析媒体源信息
    void parse_source_info(const vector<media_source_info>&media_source_info);
    //重置连接模块
    void reset_connection();
    //连接状态处理回调函数
    void handle_connect(CONNECTION_STATUS status,SOCKET fd);
    //处理转发帧
    bool handle_proxy_request(const shared_ptr<ProxyFrame>&frame);
    //插入tcp数据至发送缓存
    bool send_message(const char *buf,uint32_t buf_len);
    //发送指令集
    void send_get_authorized_info();
    void send_authorization(string nonce);
    void send_set_up_stream();
    void send_tear_down_stream();

    //处理返回数据
    void handle_get_authorized_info_ack(CJsonObject &object);
    void handle_authorization_ack(CJsonObject &object);
    void handle_set_up_stream_ack(CJsonObject &object);
    void handle_tear_down_stream_ack();
private:
    //初始化参数
    //连接辅助
    weak_ptr<tcp_connection_helper>m_connection_helper;
    //传输模式
    PTransMode m_mode;
    //在服务器上创建的转发流名称
    string m_des_name;
    //目标服务器ip
    string m_des_ip;
    //目标服务器转发端口
    uint16_t m_des_port;
    //转发账户名
    string m_user_name;
    //转发账户密码
    string m_pass_word;
    //转发session信息
    //转发session置位标识,未设置不会进行网络连接
    bool m_media_info_set;
    //保存媒体类型
    vector<PMediaTYpe> m_media_info;
    //保存媒体源详细信息
    CJsonObject m_media_json_info;
    //网络io相关
    //是否正在连接
    bool m_is_connecting;
    //不为空代表连接完成
    ChannelPtr m_tcp_channel;
    //发送udp媒体数据的socket
    SOCKET m_udp_fd;
    //udp媒体数据目标地址
    sockaddr_in m_des_addr;
    //重连控制
    //控制连接的超时时间
    uint32_t m_last_time_out;
    //转发状态相关
    //鉴权标识，未鉴权不能发送setup 和teardown
    bool m_is_authorized;
    //判断流是否创建成功，成功后不能再创建，未创建时不会发送媒体数据帧
    bool m_is_setup;
    //判断流是否已经被关闭
    bool m_is_closed;
    //媒体流token
    uint32_t m_stream_token;
    //指令序列号
    uint32_t m_seq;
    //转发实例
    shared_ptr<ProxyInterface>m_proxy_interface;
    //线程锁
    mutex m_mutex;
    //tcp收发数据缓冲区
    shared_ptr<proxy_message> m_recv_buf;
     shared_ptr<proxy_message> m_send_buf;
};
}
#endif // RTSP_PUSHER_H
