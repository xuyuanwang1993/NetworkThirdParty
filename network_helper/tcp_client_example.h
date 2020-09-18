#ifndef TCP_CLIENT_EXAMPLE_H
#define TCP_CLIENT_EXAMPLE_H
#include "tcp_client.h"
namespace micagent {
class tcp_client_example:public tcp_client{
public:
    tcp_client_example(shared_ptr<tcp_connection_helper>helper,const string &ip,uint16_t port);
    ~tcp_client_example()override{

        if(m_timer_id!=INVALID_TIMER_ID)
        {
            auto helper=m_connection_helper.lock();
            if(!helper)return;
            auto event_loop=helper->get_loop().lock();
            if(!event_loop)return;
            event_loop->removeTimer(m_timer_id);
        }
    }
    void set_example(){
        lock_guard<mutex>locker(m_mutex);
        if(!m_init_status){
            auto helper=m_connection_helper.lock();
            if(!helper)return;
            auto event_loop=helper->get_loop().lock();
            if(!event_loop)return;
            auto weak_this=weak_from_this();
            m_timer_id=event_loop->addTimer([weak_this](){
                auto *instance=dynamic_cast<tcp_client_example *>(weak_this.lock().get());
                if(!instance)return false;
                string message;
                {
                    lock_guard<mutex>locker(instance->m_mutex);
                    message=to_string(instance->m_seq++)+"\r\n\r\n";
                }
                instance->send_message(message.c_str(),message.length());

                return true;
            },0);
            MICAGENT_WARNNING("timer_id %d ",m_timer_id);
            if(m_timer_id!=INVALID_TIMER_ID)m_init_status.exchange(true);
        }
    }
protected:
    bool handle_read()override;
    void tear_down()override;
    bool get_init_status()const override{
        return m_init_status;
    }
    void clear_usr_status()override{
        m_seq=0;
    }
private:
    TimerId m_timer_id;
    uint32_t m_seq;
    atomic_bool m_init_status;
};
}

#endif // TCP_CLIENT_EXAMPLE_H
