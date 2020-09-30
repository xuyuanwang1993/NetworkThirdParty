#include "tcp_client.h"
using namespace micagent;
tcp_client::~tcp_client()
{
    m_is_closed=true;
    clear_connection_info();
    MICAGENT_WARNNING("tcp_client %s %hu release!",m_des_ip.c_str(),m_des_port);
}
tcp_client::tcp_client(shared_ptr<tcp_connection_helper> helper, const string &ip, uint16_t port):m_connection_helper(helper),m_des_ip(ip),m_des_port(port),m_is_connecting(false),m_is_closed(false),m_last_time_out(MIN_WAIT_TIME),m_connection_callback(nullptr)\
  ,m_last_connect_success_time_ms(0),m_connection_wait_time_ms(DEFAULT_CONNECTION_INTERVAL_MS)
{
    MICAGENT_INFO("tcp_client %s %hu build!",ip.c_str(),port);
}
bool tcp_client::open_connection(const CONNECTION_INIT_CALLBACK&init_cb)
{
    if(init_cb)init_cb();
    lock_guard<mutex>locker(m_mutex);
    if(m_is_closed)return false;
    if(!m_is_connecting){
        rebuild_connection();
    }
    return true;
}
void tcp_client::reset_addr_info(const string &ip,uint16_t port)
{
    lock_guard<mutex>locker(m_mutex);
    m_des_ip=ip;
    m_des_port=port;
    m_des_addr.sin_addr.s_addr=inet_addr(m_des_ip.c_str());
    m_des_addr.sin_port=htons(m_des_port);
    rebuild_connection();
}
void tcp_client::close_connection()
{
    lock_guard<mutex>locker(m_mutex);
    m_is_closed=true;
    tear_down();
}
bool tcp_client::send_message(const void *buf,uint32_t buf_len)
{
    if(!check_is_connected())return false;
    lock_guard<mutex>locker(m_mutex);
    if(!m_send_cache)return false;
    if(!m_send_cache->append(reinterpret_cast<const char *>(buf),buf_len))return false;
    if(!m_tcp_channel->isWriting()){
        m_tcp_channel->enableWriting();
        auto connection_helper=m_connection_helper.lock();
        if(!connection_helper)return false;
        auto loop=connection_helper->get_loop().lock();
        if(!loop)return false;
        loop->updateChannel(m_tcp_channel);
    }
    return true;
}
bool tcp_client::handle_write()
{
    if(m_send_cache->send_fd(m_tcp_channel->fd())<0)return false;
    if(m_send_cache->get_first_packet_size()!=0){
        if(!m_tcp_channel->isWriting()){
            m_tcp_channel->enableWriting();
            auto connection_helper=m_connection_helper.lock();
            if(connection_helper)
            {
                auto event_loop=connection_helper->get_loop().lock();
                if(event_loop){
                    event_loop->updateChannel(m_tcp_channel);
                }
            }
        }
    }
    else {
        if(m_tcp_channel->isWriting()){
            m_tcp_channel->disableWriting();
            auto connection_helper=m_connection_helper.lock();
            if(connection_helper)
            {
                auto event_loop=connection_helper->get_loop().lock();
                if(event_loop){
                    event_loop->updateChannel(m_tcp_channel);
                }
            }
        }
    }
    return true;
}
void tcp_client::rebuild_connection()
{
    if(!get_init_status()||m_is_connecting||m_is_closed)return;
    m_is_connecting=true;
    clear_connection_info();
    clear_usr_status();
    auto wait_time=get_connection_wait_time();
    if(0==wait_time)reconnect_task();
    else {
        auto helper=m_connection_helper.lock();
        if(helper)
        {
            auto loop=helper->get_loop().lock();
            if(loop){
                weak_ptr<tcp_client>weak_this(shared_from_this());
                loop->addTimer([weak_this](){
                    auto strong=weak_this.lock();
                    if(strong){
                        lock_guard<mutex>locker(strong->m_mutex);
                        strong->reconnect_task();
                    }
                    return false;
                },wait_time);
            }
        }
    }

}
void tcp_client::reconnect_task()
{
    weak_ptr<tcp_client>weak_this(shared_from_this());
    auto connection_helper=m_connection_helper.lock();
    if(connection_helper){
        connection_helper->OpenConnection(m_des_ip,m_des_port,[weak_this](CONNECTION_STATUS status,SOCKET fd){
            auto strong=weak_this.lock();
            if(strong){
                strong->handle_connect(status,fd);
            }
            else {
                MICAGENT_LOG(LOG_INFO,"puhser released!");
                if(status==CONNECTION_SUCCESS){
                    if(fd!=INVALID_SOCKET)NETWORK.close_socket(fd);
                }
            }
        },m_last_time_out);
        m_last_time_out+=1000;
        m_last_time_out=m_last_time_out>MAX_WAIT_TIME?MAX_WAIT_TIME:m_last_time_out;
    }
}
void tcp_client::clear_connection_info()
{
    if(m_tcp_channel){
        auto connection_helper=m_connection_helper.lock();
        if(connection_helper){

            auto loop=connection_helper->get_loop();
            auto event_loop=loop.lock();
            if(event_loop)event_loop->removeChannel(m_tcp_channel);
        }
        m_tcp_channel.reset();
    }
}
void tcp_client:: handle_connect(CONNECTION_STATUS status,SOCKET fd)
{
    if(status==CONNECTION_SYS_ERROR)throw runtime_error("build proxy task error!");
    else if(status==CONNECTION_SUCCESS){
        //连接成功后的处理
        unique_lock<mutex>locker(m_mutex);
        if(m_is_closed||m_tcp_channel){
            NETWORK.close_socket(fd);
            return ;
        }
        m_last_connect_success_time_ms=static_cast<uint32_t>(Timer::getTimeNow());
        Network_Util::Instance().set_ignore_sigpipe(fd);
        Network_Util::Instance().set_tcp_keepalive(fd,true);
        m_last_time_out=MIN_WAIT_TIME;
        m_is_connecting=false;
        m_tcp_channel.reset(new Channel(fd));
        weak_ptr<tcp_client>weak_this(shared_from_this());
        m_tcp_channel->setReadCallback([weak_this](Channel *){
            auto strong=weak_this.lock();
            if(!strong)return false;
            lock_guard<mutex>locker(strong->m_mutex);
            return strong->handle_read();
        }
        );
        m_tcp_channel->setWriteCallback([weak_this](Channel *){
            auto strong=weak_this.lock();
            if(!strong)return false;
            lock_guard<mutex>locker(strong->m_mutex);
            return strong->handle_write();
        });
        m_tcp_channel->setCloseCallback([weak_this](Channel *chn){
            (void)chn;
            auto strong=weak_this.lock();
            if(!strong)return false;
            lock_guard<mutex>locker(strong->m_mutex);
            strong->rebuild_connection();
            return true;
        });
        m_tcp_channel->setErrorCallback([weak_this](Channel *chn){
            (void)chn;
            auto strong=weak_this.lock();
            if(!strong)return false;
            lock_guard<mutex>locker(strong->m_mutex);
            strong->rebuild_connection();
            return true;
        });
        m_tcp_channel->enableReading();
        auto connection_helper=m_connection_helper.lock();
        if(!connection_helper)return;
        auto loop=connection_helper->get_loop().lock();
        if(loop){
            loop->updateChannel(m_tcp_channel);
            if(m_connection_callback){
                locker.unlock();
                m_connection_callback();
            }
        }
    }
    else {
        lock_guard<mutex>locker(m_mutex);
        weak_ptr<tcp_client>weak_this(shared_from_this());
        auto connection_helper=m_connection_helper.lock();
        if(!connection_helper)return;
        auto loop=connection_helper->get_loop().lock();
        if(!loop)throw runtime_error("event loop was quit!");
        MICAGENT_LOG(LOG_DEBUG,"%d connect %s  %hu  error for(%d) %s!  wait %u ms!",fd,m_des_ip.c_str(),m_des_port,status,strerror(errno),m_last_time_out);
        if(fd!=INVALID_SOCKET)NETWORK.close_socket(fd);
        loop->addTimer([weak_this](){
            auto strong=weak_this.lock();
            if(strong){
                lock_guard<mutex>locker(strong->m_mutex);
                auto wait_next=strong->m_last_time_out+1000;
                strong->m_last_time_out=MIN_WAIT_TIME;
                strong->m_is_connecting=false;
                strong->rebuild_connection();
                strong->m_last_time_out=wait_next>MAX_WAIT_TIME?MAX_WAIT_TIME:wait_next;
            }
            return false;
        },m_last_time_out);
    }
}
