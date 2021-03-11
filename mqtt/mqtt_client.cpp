#include "mqtt_client.h"
#include <stdio.h>
#include <sys/types.h>
#if !defined(WIN32)
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <fcntl.h>
using namespace micagent;
/*
    A template for opening a non-blocking POSIX socket.
*/
static int open_nb_socket(const char* addr, uint16_t  port) {
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC; /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Must be TCP */
    int sockfd = -1;
    int rv;
    struct addrinfo *p, *servinfo;

    /* get address information */
    rv = getaddrinfo(addr, std::to_string(port).c_str(), &hints, &servinfo);
    if(rv != 0) {
        fprintf(stderr, "Failed to open socket (getaddrinfo): %s\n", gai_strerror(rv));
        return -1;
    }

    /* open the first possible socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        /* connect to server */
        rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if(rv == -1) {
            close(sockfd);
            sockfd = -1;
            continue;
        }
        break;
    }

    /* free servinfo */
    freeaddrinfo(servinfo);

    /* make non-blocking */
#if !defined(WIN32)
    if (sockfd != -1) fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
#else
    if (sockfd != INVALID_SOCKET) {
        int iMode = 1;
        ioctlsocket(sockfd, FIONBIO, &iMode);
    }
#endif

    /* return the new socket fd */
    return sockfd;
}

Imqtt_client::Imqtt_client(const string&domain_name,uint16_t port):m_domain_name(domain_name),m_port(port),m_client(nullptr),m_init_param(nullptr),m_send_buf(nullptr),m_recv_buf(nullptr),\
    m_exit_flag(false),m_running_flag(true)
{
    m_client.reset(new struct mqtt_client());
}
Imqtt_client:: ~Imqtt_client()
{
    exit();
    if(m_client&&m_client->socketfd>0)::close(m_client->socketfd);
}
void micagent::reconnect_client_callback(struct mqtt_client* client, void **imqtt_client_vptr)
{
    Imqtt_client *imqtt_client = *(( Imqtt_client**) imqtt_client_vptr);
    /* Close the clients socket if this isn't the initial reconnect call */
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        close(client->socketfd);
    }
    /* Perform error handling here. */
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        printf("reconnect_client: called while client was in error state \"%s\"\n",
               Imqtt_client::get_mqtt_error(client->error).c_str()
               );
    }
    /* Open a new socket. */
    int sockfd = open_nb_socket(imqtt_client->m_domain_name.c_str(), imqtt_client->m_port);
    if (sockfd == -1) {
        perror("Failed to open socket: ");
        imqtt_client->exit();
    }

    /* Reinitialize the client. */
    mqtt_reinit(client, sockfd,
                imqtt_client->m_send_buf.get(), imqtt_client->m_init_param->send_buf_size,
                imqtt_client->m_recv_buf.get(), imqtt_client->m_init_param->recv_buf_size
                );

    //    /* Create an anonymous session */
    //    const char* client_id = imqtt_client->m_connect_param->client_id.c_str();
    //    /* Ensure we have a clean session */
    //    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    //    /* Send connection request to the broker. */
    auto p=imqtt_client->m_connect_param;
    mqtt_connect(client, p->client_id.c_str(),p->will_topic.empty()?nullptr: p->will_topic.c_str(), \
                 p->will_message.get(), p->will_message_size, \
                 p->user_name.empty()?nullptr: p->user_name.c_str()\
                                      , p->password.empty()?nullptr:p->password.c_str(), p->connect_flags, p->keep_alive_second);

    /* Subscribe to the topic. */
    auto m=imqtt_client->m_subscribe_params;
    for(auto i:m){
        mqtt_subscribe(client, i.first.c_str(), i.second.max_qos_level);
    }
}
void micagent::publish_callback(void** my_client, struct mqtt_response_publish *published)
{
    auto c=*((Imqtt_client**)my_client);
    string topic_name(static_cast<const char *>(published->topic_name),published->topic_name_size);
    auto param=c->get_subscribe_param(topic_name);
    if(!param.topic_name.empty()&&param.message_cb){
        const size_t message_size=published->application_message_size;
        shared_ptr<uint8_t>message(new uint8_t[message_size+1],std::default_delete<uint8_t[]>());
        memcpy(message.get(),published->application_message,message_size);
        param.message_cb(topic_name,message,message_size);
    }
}
enum MQTTErrors Imqtt_client::init(const mqtt_init_param &init_param,const mqtt_connect_param&connect_param)
{
    m_init_param.reset(new mqtt_init_param(init_param.send_buf_size,init_param.recv_buf_size,init_param.reconnect_flag));
    m_send_buf.reset(new uint8_t[m_init_param->send_buf_size+1],std::default_delete<uint8_t[]>());
    m_recv_buf.reset(new uint8_t[m_init_param->recv_buf_size+1],std::default_delete<uint8_t[]>());
    m_connect_param.reset(new mqtt_connect_param(connect_param));
    if(m_connect_param->will_message_size>0){
        m_connect_param->will_message.reset(new uint8_t[m_connect_param->will_message_size+1],std::default_delete<uint8_t[]>());
        memcpy(m_connect_param->will_message.get(),connect_param.will_message.get(),m_connect_param->will_message_size);
    }
    //存储当前类的指针，将作为publish回调函数第一个参数
    m_client->publish_response_callback_state=this;
    if(m_init_param->reconnect_flag){
        mqtt_init_reconnect(m_client.get(),
                            reconnect_client_callback, this,
                            publish_callback
                            );
        mqtt_sync(m_client.get());
    }
    else {
        //create socket
        int sockfd = open_nb_socket(m_domain_name.c_str(), m_port);

        if (sockfd == -1) {
            perror("Failed to open socket: ");
            return MQTT_ERROR_SOCKET_ERROR;
        }

        mqtt_init(m_client.get(), sockfd, m_send_buf.get(), init_param.send_buf_size, m_recv_buf.get(), init_param.recv_buf_size, publish_callback);
        auto p=m_connect_param;
        mqtt_connect(m_client.get(), p->client_id.c_str(),p->will_topic.empty()?nullptr: p->will_topic.c_str(), \
                     p->will_message.get(), p->will_message_size, \
                     p->user_name.empty()?nullptr: p->user_name.c_str()\
                                          , p->password.empty()?nullptr:p->password.c_str(), p->connect_flags, p->keep_alive_second);
        /* check that we don't have any errors */
        if (m_client->error != MQTT_OK) {
            fprintf(stderr, "error: %s\n", Imqtt_client::get_mqtt_error(m_client->error).c_str());
            return  m_client->error;
        }
    }
    return m_client->error;
}

enum MQTTErrors Imqtt_client::publish(const string&topic,const void *message,size_t message_size,uint8_t publish_flags)
{
    return mqtt_publish(m_client.get(), topic.c_str(), message, message_size, publish_flags);
}

enum MQTTErrors Imqtt_client::subscribe(const subscribe_param & param)
{
    auto state=mqtt_subscribe(m_client.get(), param.topic_name.c_str(), param.max_qos_level);
    if(state==MQTT_OK){
        change_subscribe_param(param.topic_name,param);
    }
    return state;
}

enum MQTTErrors Imqtt_client::unsubscribe(const subscribe_param & param)
{
    auto state=mqtt_unsubscribe(m_client.get(), param.topic_name.c_str());
    if(state==MQTT_OK){
        remove_subscribe_param(param.topic_name);
    }
    return state;
}

enum MQTTErrors Imqtt_client::ping()
{
    return  mqtt_ping(m_client.get());
}

enum MQTTErrors Imqtt_client::disconnect()
{
    auto error=mqtt_disconnect(m_client.get());
    if(error==MQTT_OK){
        mqtt_sync(m_client.get());
        if(m_client->socketfd>0){
            ::close(m_client->socketfd);
            m_client->socketfd=-1;
        }
    }
    return error;
}

enum MQTTErrors Imqtt_client::reconnect()
{
    auto error=mqtt_reconnect(m_client.get());
    if(error==MQTT_OK)mqtt_sync(m_client.get());
    return error;
}

void Imqtt_client::loop(uint32_t interval_us )
{
    interval_us=interval_us<100?100:interval_us;
    if(!m_exit_flag){
        while(m_running_flag){
            mqtt_sync(m_client.get());
            usleep(interval_us);
        }
        if(m_client->socketfd>0){
            ::close(m_client->socketfd);
            m_client->socketfd=-1;
        }
        m_exit_flag.exchange(true);
    }
}
void Imqtt_client::exit()
{
    m_running_flag.exchange(false);
}
