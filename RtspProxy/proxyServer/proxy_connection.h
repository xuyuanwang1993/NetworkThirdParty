#ifndef PROXY_CONNECTION_H
#define PROXY_CONNECTION_H
#include "tcp_connection.h"
#include "proxy_protocol.h"
#include"proxybufhandle.h"
#include "CJsonObject.hpp"
namespace micagent {
using  neb::CJsonObject;
class proxy_server;
class proxy_connection:public tcp_connection{
//nonce字段有效时间
    static constexpr int64_t MAX_NONCE_DIFF=60*1000;//60s
    friend class  proxy_server;
public:
    proxy_connection(SOCKET fd,proxy_server *server);
//比较proxy_connection的上次存活时间与传入时间的差值
    static int64_t diff_alive_time(proxy_connection* conn,int64_t time_base){
        if(!conn)return INT64_MAX;
        else {
            return time_base-conn->m_last_alive_time;
        }
    }
    ~proxy_connection();
protected:
    //连接有可读数据时处理
    bool handle_read();
    //连接可写时的数据处理
     bool handle_write();
     //连接错误处理
     bool handle_error(){return handle_close();}
     //初始化时传入，强引用
     proxy_server *m_proxy_server;
private:
     //tcp 数据发送接口
     bool send_message(const char *buf,uint32_t buf_len);
     //处理协议输出的数据帧
     bool handle_proxy_frame(shared_ptr<ProxyFrame> frame);
     //处理控制帧
     bool handle_proxy_request(const shared_ptr<ProxyFrame>&frame);
     //处理媒体转发帧
     bool handle_proxy_media_data(const shared_ptr<ProxyFrame>&frame);
     //控制命令处理
     void handle_get_authorized_info(CJsonObject &object);
     void handle_authorization(CJsonObject &object);
     void handle_set_up_stream(CJsonObject &object);
     void handle_modify_stream(CJsonObject &object);
     void handle_tear_down_stream(CJsonObject &object);
     //基础回包信息组包
     inline void build_json_response(const string& cmd,uint32_t seq,uint32_t status,const string &info,CJsonObject &object)
     {
         object.Add("cmd",cmd);
         object.Add("seq",seq);
         object.Add("status",status);
         object.Add("info",info);
     }
     //udp媒体数据输入，同时刷新tcp连接的在线时间
     inline bool input_udp_data(const void *buf,uint16_t len)
     {
         lock_guard<mutex>locker(m_mutex);
         m_last_alive_time=Timer::getTimeNow();
         if(m_proxy_interface)return m_proxy_interface->protocol_input(buf,len);
         return  false;
     }
     //tcp接收缓存
     shared_ptr<proxy_message> m_recv_buf;
     //TCP发送缓存
     shared_ptr<proxy_message> m_send_buf;
     //转发对象实例
     shared_ptr<ProxyInterface> m_proxy_interface;
     //连接授权状态
     bool m_is_authorized;
     //连接是否创建完成
     bool m_is_setup;
     //在rtspserver上创建的流id
     uint32_t m_stream_token;
     //上次在线时间
     atomic<int64_t> m_last_alive_time;
     //上次发送时间戳
     uint32_t m_last_send_timestamp;
     //上次帧原始时间戳
     uint32_t m_last_recv_timestamp;
};
}
#endif // PROXY_CONNECTION_H
