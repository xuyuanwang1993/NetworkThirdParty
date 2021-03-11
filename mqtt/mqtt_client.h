#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H
/**
 * test broker: test.mosquitto.org
 * broker source : http://www.steves-internet-guide.com/mosquitto-broker/
 * 参照该界面即可完成服务器部署
 */
extern "C"{
#include "MQTT_C/mqtt_pal.h"
#include "MQTT_C/mqtt.h"
}
#include <thread>
#include<memory>
#include<string>
#include<functional>
#include<map>
#include<atomic>
#include<mutex>
namespace micagent {
using std::thread;
using std::string;
using std::shared_ptr;
using std::function;
using std::map;
using std::atomic;
using std::mutex;
/**
 * 接收到订阅消息时触发的回调函数
 * topic_name 订阅主题名
 * message 接收到的数据
 * message_size 数据长度
 */
using MQTT_PUBLISH_CALLBACK=function<void(const string &topic_name,shared_ptr<uint8_t>message,size_t message_size)>;
void reconnect_client_callback(struct mqtt_client* client, void **reconnect_state_vptr);
void publish_callback(void** my_client, struct mqtt_response_publish *published);
/**
 * @brief The mqtt_init_param struct 定义mqtt初始化时传入参数
 * DEFAULT_BUF_SIZE 默认数据发送/接收缓冲区大小
 * MIN_BUF_SIZE 最小缓冲区长度
 * MAX_BUF_SIZE 最大缓冲区长度
 * send_buf_size 发送缓冲区长度
 * recv_buf_size 接收缓冲区长度
 * reconnect_flag 是否开启自动重连  为true开启，默认为true
 */
struct mqtt_init_param{
    constexpr static size_t DEFAULT_BUF_SIZE=32768U;
    constexpr static size_t MIN_BUF_SIZE=2000U;
    constexpr static size_t MAX_BUF_SIZE=10000000U;
    size_t send_buf_size;
    size_t recv_buf_size;
    bool reconnect_flag;
    mqtt_init_param(size_t out_size=DEFAULT_BUF_SIZE,size_t in_size=DEFAULT_BUF_SIZE,\
                    bool _reconnect_flag =true):send_buf_size(out_size),recv_buf_size(in_size),
        reconnect_flag(_reconnect_flag){
        if(send_buf_size<MIN_BUF_SIZE)send_buf_size=MIN_BUF_SIZE;
        if(send_buf_size>MAX_BUF_SIZE)send_buf_size=MAX_BUF_SIZE;
        if(recv_buf_size<MIN_BUF_SIZE)recv_buf_size=MIN_BUF_SIZE;
        if(recv_buf_size>MAX_BUF_SIZE)recv_buf_size=MAX_BUF_SIZE;
    }
};
/**
 * @brief The mqtt_connect_param struct 定义与mqtt broker验证所需信息
 * client_id 独一无二的客户端id，若相同的客户端id登入，会迫使已登录的设备下线
 * will_topic 遗嘱主题名
 * will_message 遗嘱信息
 * will_message_size 遗嘱长度
 * user_name 登录用户名
 * password 登录密码
 * connect_flags 连接属性
 * keep_alive_second  连接超时时间  单位s
 * @details
 * 遗嘱的作用是当某一客户端非正常离线时，会向所有订阅改遗嘱主题的订阅者发送遗嘱消息
 * 下面是可用连接属性，可以以|的形式组合使用
 * enum MQTTConnectFlags {
    MQTT_CONNECT_RESERVED = 1u,//保留字段，暂不使用
    MQTT_CONNECT_CLEAN_SESSION = 2u,//是否清除相同ID已登录客户端
    MQTT_CONNECT_WILL_FLAG = 4u,//是否带有遗嘱信息
    MQTT_CONNECT_WILL_QOS_0 = (0u & 0x03) << 3,//最多发送一次遗嘱
    MQTT_CONNECT_WILL_QOS_1 = (1u & 0x03) << 3,//最少发送一次遗嘱
    MQTT_CONNECT_WILL_QOS_2 = (2u & 0x03) << 3,//确保只发送一次遗嘱
    MQTT_CONNECT_WILL_RETAIN = 32u,//是否在broker保存遗嘱消息
    MQTT_CONNECT_PASSWORD = 64u,//是否携带密码字段
    MQTT_CONNECT_USER_NAME = 128u//是否携带用户名字段
};
 */
struct mqtt_connect_param{
    string client_id;
    string will_topic;
    shared_ptr<uint8_t>will_message;
    size_t will_message_size;
    string user_name;
    string password;
    uint8_t connect_flags;
    uint16_t keep_alive_second;
    mqtt_connect_param():client_id(""),will_topic(""),will_message(nullptr)\
      ,will_message_size(0),user_name(""),password(""),connect_flags(0),keep_alive_second(400){

    }
};
/**
 * @brief The subscribe_param struct 订阅主题时的参数
 * topic_name 主题名
 * max_qos_level 最大接收消息等级
 * message_cb 消息处理回调函数
 * @details
 *最大接收消息等级
 * mqtt 有0 ，1 ，2三个等级
 * 0     消息只会被发送一次，接收到0-1 次,不确保被订阅者处理
 * 1     消息会被接收到一次或多次
 * 2     消息只会被接收到一次
 *
 */
struct subscribe_param{
    string topic_name;
    int max_qos_level;
    MQTT_PUBLISH_CALLBACK message_cb;
    subscribe_param():topic_name(""),max_qos_level(0),message_cb(nullptr){

    }
};

class Imqtt_client final{
    constexpr static uint32_t DEFAULT_REFRESH_INTERVAL_US=100000U;//100ms
    constexpr static uint16_t DEFAULT_PORT=1883U;
    friend void reconnect_client_callback(struct mqtt_client* client, void **reconnect_state_vptr);
    friend void publish_callback(void** my_client, struct mqtt_response_publish *published);

public:
    /**
     * @brief Imqtt_client
     * @param domain_name 域名
     * @param port 服务端口 默认1883
     * @example
     * Imqtt_client("www.meaning.com")
     * Imqtt_client("139.159.137.87")
     * Imqtt_client("www.meaning.com",5883)
     * Imqtt_client("139.159.137.87",5883)
     */
    Imqtt_client(const string&domain_name,uint16_t port=DEFAULT_PORT);
    ~Imqtt_client();
    /**
     * @brief get_mqtt_error 获取mqtt error所对应的字符串
     * @param error mqtt error返回值
     * @return
     */
    static string get_mqtt_error(enum MQTTErrors error){
        if(error>=MQTT_OK)return "MQTT_OK";
        return mqtt_error_str(error);
    }
    /**
     * @brief init mqtt客户端初始化函数，在其它非静态接口调用之前调用
     * @param init_param 初始化参数
     * @param connect_param 连接参数
     * @return
     * @warning 非线程安全，只能调用一次
     */
    enum MQTTErrors init(const mqtt_init_param &init_param,const mqtt_connect_param&connect_param);
    /**
     * @brief publish 向对应主题推送消息
     * @param topic 主题名
     * @param message 消息缓冲
     * @param message_size 消息长度
     * @param publish_flags 消息推送flag
     * @return
     * @details
     * enum MQTTPublishFlags {
    MQTT_PUBLISH_DUP = 8u, //不要设置此flag
    MQTT_PUBLISH_QOS_0 = ((0u << 1) & 0x06),//服务器接收消息,向订阅者最多发送一次
    MQTT_PUBLISH_QOS_1 = ((1u << 1) & 0x06),//服务器接收消息,向订阅者最少发送一次,未收到回复会进行重发
    MQTT_PUBLISH_QOS_2 = ((2u << 1) & 0x06),//服务器接收消息,向订阅者发送一次
    MQTT_PUBLISH_QOS_MASK = ((3u << 1) & 0x06),//不要设置此flag
    MQTT_PUBLISH_RETAIN = 0x01//设置此flag后，推送消息会被后续订阅此主题的设备获取,仅对最后一条消息有效
};
     */
    enum MQTTErrors publish(const string&topic,const void *message,size_t message_size,uint8_t publish_flags);
    /**
     * @brief subscribe 订阅某一主题
     * @param param
     * @return
     */
    enum MQTTErrors subscribe(const subscribe_param & param);
    /**
     * @brief unsubscribe 取消订阅某一主题
     * @param param
     * @return
     */
    enum MQTTErrors unsubscribe(const subscribe_param & param);
    /**
     * @brief ping 探测服务器是否存活
     * @return
     * @details
     * 使用其探测发现服务器工作不正常后，可以调用reconnect
     */
    enum MQTTErrors ping();
    /**
     * @brief disconnect 主动断开连接
     * @return
     * @warning 设置了重连flag时，断开连接后会自动重连
     */
    enum MQTTErrors disconnect();
    /**
     * @brief reconnect 主动重连
     * @return
     * @warning 仅在设置了重连flag后会生效
     */
    enum MQTTErrors reconnect();
    /**
     * @brief loop 以一定的时间间隔更新mqtt_client状态
     * @param interval_us 更新间隔 单位us
     * @details
     * 无限循环，可调用exit 中断循环
     */
    void loop(uint32_t interval_us=1000000 );
    /**
     * @brief exit 中断loop循环
     * @warning 使用exit退出后 client不可用
     */
    void exit();
    /**
     * @brief get_state 获取client运行状态
     * @return
     * @details
     * 若运行状态不为MQTT_OK 判断有异常产生，可尝试重连或者重新创建
     */
    enum MQTTErrors get_state()const{
        return m_client->error;
    }
private:
    mutable mutex m_subscribe_mutex;
    map<string,subscribe_param>m_subscribe_params;
    subscribe_param get_subscribe_param(const string &topic)const{
        std::lock_guard<mutex>locker(m_subscribe_mutex);
        auto iter=m_subscribe_params.find(topic);
        if(iter!=end(m_subscribe_params)){
            return iter->second;
        }
        return subscribe_param();
    }
    void change_subscribe_param(const string &topic,const subscribe_param &param){
        std::lock_guard<mutex>locker(m_subscribe_mutex);
        m_subscribe_params[topic]=param;
    }
    void remove_subscribe_param(const string &topic){
        std::lock_guard<mutex>locker(m_subscribe_mutex);
        auto iter=m_subscribe_params.find(topic);
        if(iter!=end(m_subscribe_params)){
            m_subscribe_params.erase(iter);
        }
    }

private:
    string m_domain_name;
    uint16_t m_port;
    shared_ptr<struct mqtt_client>m_client;
    shared_ptr<mqtt_init_param>m_init_param;
    shared_ptr<mqtt_connect_param>m_connect_param;
    shared_ptr<uint8_t>m_send_buf;
    shared_ptr<uint8_t>m_recv_buf;
    atomic<bool> m_exit_flag;
    atomic<bool> m_running_flag;
};
}
#endif // MQTT_CLIENT_H
