#include "test_server.h"
namespace micagent {
static const char cmd_str[][128]={
    "start",
    "add_trigger",
    "add_timer",
    "delete_timer",
    "add_thread",
    "cancel_thread",
    "add_listen_work",
    "delete_listen_work",
    "add_active_connect",
    "delete_active_connect",
    "add_udp_send_server",
    "delete_udp_send_server",
    "add_udp_send_task",
    "delete_udp_send_task",
    "exit",
    "help",
    "end",
};
static const char * get_help_str(){
    static const char help_str[]="1.add_trigger <print string>\tadd a trigger to print the string you provide!\r\n"
                                 "2.add_timer <true/false> <run_interval(ms)> <print string>\t add a timer (true for loop execution) to run with a interval that"
                                 " specified by run_interval!\r\n"
                                 "3.delete_timer <timer_id>\tdelete a timer via the return value that add_timer cmd had given!\r\n"
                                 "4.add_thread <run_times> <print string>\tadd a thread to print the string for run_times times,0 for loop execution!\r\n"
                                 "5.cancel_thread <thread_id>\tcancel a thread via the return value that add_thread cmd had given!\r\n"
                                 "6.add_listen_work<port><send_interval(ms)><print string>\tadd a listen server who will send the print string every send_interval to the connections!\r\n"
                                 "7.delete_listen_work <listen_server_id>\r\n"
                                 "8.add_active_connect <ip><port><send_interval(ms)><print string>\tset up a tcp connection who will send the print string"
                                 "to the ip:port every send_interval\r\n"
                                 "9.delete_active_connect <connection_id>\r\n"
                                 "10.add_udp_send_server<port><send_interval(ms)><print string>\tadd a udp send server who will send the print string every send_interval to the udp tasks!\r\n"
                                 "11.delete_udp_send_server <udp_send_server_id>\r\n"
                                 "12.add_udp_send_task<udp_send_server_id><ip><port>\r\n"
                                 "13.delete_udp_send_task<send_task_id>\r\n"
                                 "14.exit\r\n";
    return  help_str;
}
test_server::test_server(int32_t thread_pool_size,uint32_t trigger_threads,uint32_t capacity,uint32_t thread_nums):\
    m_event_loop(new EventLoop(thread_pool_size,trigger_threads,capacity,thread_nums)),m_session_id(0)
{
    for (int i=START;i<END;i++) {
        m_cmp_type.emplace(cmd_str[i],i);
    }
    SOCKET tcp_sock=Network_Util::Instance().build_socket(TCP);
    Network_Util::Instance().bind(tcp_sock,TEST_PORT);
    Network_Util::Instance().listen(tcp_sock,10);
    m_tcp_handle.reset(new Channel(tcp_sock));
    m_tcp_handle->enableReading();
    m_tcp_handle->setReadCallback([this](Channel *chn){
        auto sock=Network_Util::Instance().accept(chn->fd());
        if(sock>0){
            auto new_channel=make_shared<Channel>(sock);
            new_channel->enableReading();
            new_channel->setReadCallback([this](Channel *chn){
                shared_ptr<char[]>buf(new char[2048]);
                auto len=recv(chn->fd(),buf.get(),2048,0);
                if(len==0)return false;
                if(len>0){
                    this->handle_read(chn->fd(),buf.get(),len,nullptr);
                }
                return true;
            });
            new_channel->setCloseCallback([this](Channel *chn){
                this->m_event_loop->removeChannel(chn->fd());
                return true;
            });
            m_event_loop->updateChannel(new_channel);
        }
        return true;
    });
    m_event_loop->updateChannel(m_tcp_handle);
    SOCKET udp_sock=Network_Util::Instance().build_socket(UDP);
    Network_Util::Instance().bind(udp_sock,TEST_PORT);
    m_udp_handle.reset(new Channel(udp_sock));
    m_udp_handle->enableReading();
    m_udp_handle->setReadCallback([this](Channel *chn){
        shared_ptr<char[]>buf(new char[2048]);
        sockaddr_in addr;
        bzero(&addr,sizeof (addr));
        socklen_t addr_len=sizeof (addr);
        auto len=recvfrom(chn->fd(),buf.get(),2048,0,(struct sockaddr *)&addr,&addr_len);
        if(len>0){
            this->handle_read(chn->fd(),buf.get(),len,&addr);
        }
        return true;
    });
    m_event_loop->updateChannel(m_udp_handle);
}
void test_server::print_cmd_help()
{
    MICAGENT_MARK("%s",get_help_str());
}
test_server::~test_server()
{
    m_event_loop->stop();
    for(auto i:m_thread_state){
        if(i.second->get_status())i.second->change();
    }
}
void test_server::handle_read(SOCKET fd,const char *buf,uint32_t buf_len,const sockaddr_in *addr)
{
    auto cmd_queue=parase_buf(buf,buf_len);
    do{
        if(cmd_queue.empty())break;
        auto first=cmd_queue.front();

        cmd_queue.pop();
        auto iter=m_cmp_type.find(first);
        if(iter==std::end(m_cmp_type))break;
        switch (iter->second) {
        case ADD_TRIGGER:
            handle_add_trigger(fd,cmd_queue,addr);
            break;
        case ADD_TIMER:
            handle_add_timer(fd,cmd_queue,addr);
            break;
        case DELETE_TIMER:
            handle_delete_timer(fd,cmd_queue,addr);
            break;
        case ADD_THREAD:
            handle_add_thread(fd,cmd_queue,addr);
            break;
        case CANCEL_THREAD:
            handle_cancel_thread(fd,cmd_queue,addr);
            break;
        case ADD_LISTEN_WORK:
            handle_add_listen_work(fd,cmd_queue,addr);
            break;
        case DELETE_LISTEN_WORK:
            handle_delete_listen_work(fd,cmd_queue,addr);
            break;
        case ADD_ACTIVE_CONNECT:
            handle_active_connect(fd,cmd_queue,addr);
            break;
        case DELETE_ACTIVE_CONNECT:
            handle_delete_active_connect(fd,cmd_queue,addr);
            break;
        case ADD_UDP_SEND_SERVER:
            handle_add_udp_send_server(fd,cmd_queue,addr);
            break;
        case DELETE_UDP_SEND_SERVER:
            handle_delete_udp_send_server(fd,cmd_queue,addr);
            break;
        case ADD_UDP_SEND_TASK:
            handle_add_udp_send_task(fd,cmd_queue,addr);
            break;
        case DELETE_UDP_SEND_TASK:
            handle_delete_udp_send_task(fd,cmd_queue,addr);
            break;
        case EXIT:
            handle_exit();
            break;
        case HELP:
            handle_help(fd,addr);
            break;
        default:
            break;
        }
        return;
    }while(0);
    //error handle
    string error_string="error cmd:";
    error_string+=buf;
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_add_trigger(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    do{
        if(param.empty())break;
        string print_string=param.front();
        print_string+="_";
        print_string+=to_string(m_session_id++);
        m_event_loop->add_trigger_event([print_string](){
            MICAGENT_MARK("trigger:%s\r\n",print_string.c_str());
        });
        return;
    }while(0);
    response(fd,addr,"handle add trigger error!\r\n");
}
void test_server::handle_add_timer(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle add timer error ";
    do{
        if(param.empty())break;
        string loop_flag=param.front();
        bool loop=true;
        if(loop_flag=="false")loop=false;
        else if (loop_flag!="true"){
            error_string+="false loop flag!";
            break;
        }
        param.pop();
        if(param.empty())break;
        string run_interval=param.front();
        param.pop();
        uint32_t interval=stoul(run_interval);
        if(param.empty())break;
        string print_string=param.front();
        print_string+="_";
        print_string+=to_string(m_session_id++);
        auto timer_id= m_event_loop->addTimer([loop,print_string](){
            MICAGENT_MARK("Timer:%s\r\n",print_string.c_str());
            return  loop;
        },interval);
        if(timer_id>0&&loop){
            string res="Timer_id : ";
            res+=to_string(timer_id);
            res+="\r\n";
            response(fd,addr,res);
        }
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_delete_timer(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle delete timer error ";
    do{
        if(param.empty())break;
        string timer_id=param.front();
        uint32_t timer=stoul(timer_id);
        m_event_loop->removeTimer(timer);
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_add_thread(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle add_thread error ";
    do{
        if(param.empty())break;
        string run_times=param.front();
        uint32_t times=stoul(run_times);
        param.pop();
        if(param.empty())break;
        string print_string=param.front();

        uint32_t thread_id=m_session_id++;
        print_string+="_";
        print_string+=to_string(thread_id);
        shared_ptr<run_status> m_exit(new run_status(true));
        if(m_event_loop->add_thread_task([print_string,times,m_exit,this,thread_id](){
                                         uint32_t tmp=times;
                                         if(tmp==0){
                                         while(m_exit->get_status())
        {
                                         MICAGENT_MARK("thread : %s\r\n",print_string.c_str());
                                         Timer::sleep(20);
    }
    }
                                         else {
                                         while(tmp-->0&&m_exit->get_status())
        {
                                         MICAGENT_MARK("thread : %s\r\n",print_string.c_str());
                                         Timer::sleep(20);
    }
    }
                                         lock_guard<mutex> lock(this->m_mutex);
                                         m_thread_state.erase(thread_id);
                                         return false;
    })){
            lock_guard<mutex>locker(m_mutex);
            m_thread_state.emplace(make_pair(thread_id,m_exit));
            string res="thread_id : ";
            res+=to_string(thread_id);
            res+="\r\n";
            response(fd,addr,res);
        };
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_cancel_thread(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle cancel thread error ";
    do{
        if(param.empty())break;
        string thread_id=param.front();
        uint32_t id=stoul(thread_id);
        lock_guard<mutex>locker(m_mutex);
        auto iter=m_thread_state.find(id);
        if(iter!=end(m_thread_state)){
            if(iter->second->get_status())iter->second->change();
        }
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_add_listen_work(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle add listen work error ";
    uint16_t port=0;
    uint32_t timer_interval=0;
    string print_string;
    auto server_id=m_session_id++;
    do{
        if(param.size()<3)break;
        string s_port=param.front();
        param.pop();
        port=stoul(s_port);
        string s_timer_interval=param.front();
        param.pop();
        timer_interval=stoul(s_timer_interval);
        print_string=param.front();
        print_string+="_";
        print_string+=to_string(m_session_id);
        if(port==0||port>65535)break;
        SOCKET tcp_sock=Network_Util::Instance().build_socket(TCP);
        if(!Network_Util::Instance().bind(tcp_sock,port)){
            Network_Util::Instance().close_socket(tcp_sock);
            break;
        };
        Network_Util::Instance().listen(tcp_sock,10);
        ChannelPtr server_channel=make_shared<Channel>(tcp_sock);
        server_channel->enableReading();
        server_channel->setReadCallback([this,print_string,timer_interval](Channel *chn){
            auto sock=Network_Util::Instance().accept(chn->fd());
            if(sock>0){
                Network_Util::Instance().set_ignore_sigpipe(sock);
                Network_Util::Instance().make_noblocking(sock);
                auto new_channel=make_shared<Channel>(sock);
                new_channel->enableReading();
                new_channel->setReadCallback([this](Channel *chn){
                    shared_ptr<char>buf(new char[2048]);
                    auto len=recv(chn->fd(),buf.get(),2048,0);
                    if(len==0)return false;
                    return true;
                });
                new_channel->setWriteCallback([this,print_string](Channel *chn){
                    auto len=send(chn->fd(),print_string.c_str(),print_string.length(),0);
                    chn->disableWriting();
                    this->m_event_loop->updateChannel(chn);
                    if(len==0)return false;
                    return true;
                });
                auto timer_id=this->m_event_loop->addTimer([new_channel,this](){
                    new_channel->enableWriting();
                    this->m_event_loop->updateChannel(new_channel);
                    return true;
                },timer_interval);
                new_channel->setCloseCallback([this,timer_id](Channel *chn){
                    this->m_event_loop->removeTimer(timer_id);
                    this->m_event_loop->removeChannel(chn->fd());
                    return true;
                });
                m_event_loop->updateChannel(new_channel);
            }
            return true;
        });
        m_event_loop->updateChannel(server_channel);
        {
            lock_guard<mutex>locker(m_mutex);
            m_listen_server_map.emplace(server_id,server_channel);
        }
        string res="tcp_server_id : ";
        res+=to_string(server_id);
        res+="\r\n";
        response(fd,addr,res);
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_delete_listen_work(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle delete listen work error ";
    do{
        if(param.empty())break;
        string server_id=param.front();
        uint32_t id=stoul(server_id);
        lock_guard<mutex>locker(m_mutex);
        auto iter=m_listen_server_map.find(id);
        if(iter!=end(m_listen_server_map)){
			m_event_loop->removeChannel(iter->second);
            m_listen_server_map.erase(iter);
        }
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_active_connect(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string ip;
    uint16_t port=0;
    uint32_t send_interval=0;
    string print_string;
    string error_string="handle active_connect error ";
    auto thread_id=m_session_id++;
    do{
        if(param.size()<4)break;
        ip=param.front();
        param.pop();
        auto s_port=param.front();
        port=stoul(s_port);
        param.pop();
        auto s_send_interval=param.front();
        send_interval=stoul(s_send_interval);
        param.pop();
        print_string=param.front();
        print_string+="_";
        print_string+=to_string(m_session_id);
        if(port==0)break;
        if(send_interval==0)send_interval=10;
        shared_ptr<run_status> m_exit(new run_status(true));
        auto ret=m_event_loop->add_thread_task([ip,port,send_interval,print_string,m_exit,this,thread_id](){
            auto sock=Network_Util::Instance().build_socket(TCP);
            Network_Util::Instance().set_ignore_sigpipe(sock);
            if(Network_Util::Instance().connect(sock,ip,port)){
                while(m_exit->get_status()){
                    if(send(sock,print_string.c_str(),print_string.length(),0)>0){
                        Timer::sleep(send_interval);
                    }
                    else {
                        break;
                    }
                }
            }
            Network_Util::Instance().close_socket(sock);
            lock_guard<mutex>locker(m_mutex);
            m_thread_state.erase(thread_id);
            return false;
        });
        lock_guard<mutex>locker(m_mutex);
        m_thread_state.emplace(make_pair(thread_id,m_exit));
        string res="connection_id : ";
        res+=to_string(thread_id);
        res+="\r\n";
        response(fd,addr,res);
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_delete_active_connect(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle delete active connect error ";
    do{
        if(param.empty())break;
        string thread_id=param.front();
        uint32_t id=stoul(thread_id);
        lock_guard<mutex>locker(m_mutex);
        auto iter=m_thread_state.find(id);
        if(iter!=end(m_thread_state)){
            if(iter->second->get_status())iter->second->change();
        }
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_add_udp_send_server(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    auto udp_server_id=m_session_id++;
    uint16_t port=0;
    uint32_t send_interval=0;
    string print_string;
    string error_string="handle add udp send server error ";
    do{
        if(param.size()<3)break;
        string s_port=param.front();
        param.pop();
        port=stoul(s_port);
        string s_timer_interval=param.front();
        param.pop();
        send_interval=stoul(s_timer_interval);
        print_string=param.front();
        print_string+="_";
        print_string+=to_string(m_session_id);
        if(port==0)break;
        SOCKET udp_sock=Network_Util::Instance().build_socket(UDP);
        Network_Util::Instance().set_reuse_addr(udp_sock);
        if(!Network_Util::Instance().bind(udp_sock,port)){
            Network_Util::Instance().close_socket(udp_sock);
            break;
        };
        shared_ptr<udp_send_server> udp_server=make_shared<udp_send_server>();
        udp_server->udp_send_server=make_shared<Channel>(udp_sock);

        udp_server->udp_send_server->setWriteCallback([udp_server,print_string,this](Channel *chn){
            {
                udp_server->m_mutex.lock();
                for(auto i :udp_server->m_recv_map){
                    sendto(chn->fd(),print_string.c_str(),print_string.length(),0,(const sockaddr *)&i.second,sizeof (sockaddr_in));
                }
                udp_server->m_mutex.unlock();
            }
            chn->disableWriting();
            m_event_loop->updateChannel(chn);
            return true;
        });
        udp_server->timer_id=m_event_loop->addTimer([udp_server,this](){
            udp_server->udp_send_server->enableWriting();
            m_event_loop->updateChannel(udp_server->udp_send_server);
            return true;
        },send_interval);
        m_event_loop->updateChannel(udp_server->udp_send_server);
        {
            lock_guard<mutex>locker(m_mutex);
            m_udp_server_map.emplace(udp_server_id,udp_server);
        }
        string res="udp_server_id : ";
        res+=to_string(udp_server_id);
        res+="\r\n";
        response(fd,addr,res);
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_delete_udp_send_server(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle delete udp send server error ";
    do{
        if(param.empty())break;
        string udp_server_id=param.front();
        uint32_t id=stoul(udp_server_id);
        lock_guard<mutex>locker(m_mutex);
        auto iter=m_udp_server_map.find(id);
        if(iter!=end(m_udp_server_map)){
            m_event_loop->removeTimer(iter->second->timer_id);
            m_event_loop->removeChannel(iter->second->udp_send_server);
            for(auto i:iter->second->m_recv_map){
                m_udp_task_map.erase(i.first);
            }
            m_udp_server_map.erase(iter);
        }
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_add_udp_send_task(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    auto udp_session_id=m_session_id++;
    uint32_t udp_server_id=0;
    string ip;
    uint16_t port=0;
    string error_string="handle add udp send task error ";
    do{
        if(param.size()<3)break;
        auto s_udp_server_id=param.front();
        udp_server_id=stoul(s_udp_server_id);
        param.pop();
        ip=param.front();
        param.pop();
        auto s_port=param.front();
        port=stoul(s_port);
        {
            lock_guard<mutex>lock(m_mutex);
            auto iter=m_udp_server_map.find(udp_server_id);
            if(iter==end(m_udp_server_map))break;
            {
                m_udp_task_map.emplace(udp_session_id,udp_server_id);
                sockaddr_in addr;
                bzero(&addr,sizeof (addr));
                addr.sin_family=AF_INET;
                addr.sin_addr.s_addr=inet_addr(ip.c_str());
                addr.sin_port=htons(port);
                lock_guard<mutex>locker2(iter->second->m_mutex);
                iter->second->m_recv_map.emplace(udp_session_id,std::move(addr));
            }
        }
        string res="udp_session_id : ";
        res+=to_string(udp_session_id);
        res+="\r\n";
        response(fd,addr,res);
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_delete_udp_send_task(SOCKET fd,queue<string>&param,const sockaddr_in *addr)
{
    string error_string="handle delete udp send task error ";
    do{
        if(param.empty())break;
        string udp_session_id=param.front();
        uint32_t id=stoul(udp_session_id);
        lock_guard<mutex>locker(m_mutex);
        auto iter=m_udp_task_map.find(id);
        if(iter!=end(m_udp_task_map)){
            auto iter2=m_udp_server_map.find(iter->second);
            if(iter2!=end(m_udp_server_map)){
                lock_guard<mutex>locker2(iter2->second->m_mutex);
                iter2->second->m_recv_map.erase(id);
            }
            m_udp_task_map.erase(id);
        }
        return;
    }while(0);
    error_string+="\r\n";
    response(fd,addr,error_string);
}
void test_server::handle_exit()
{
    m_conn.notify_all();
}
void test_server::handle_help(SOCKET fd,const sockaddr_in *addr)
{
    response(fd,addr,get_help_str());
}
queue<string>test_server::parase_buf(const char *buf,uint32_t buf_len)
{
    queue<string>ret;
    string tmp;
    for(uint32_t i=0;i<buf_len;i++){
        if(buf[i]!=' ')tmp.push_back(buf[i]);
        else{
            if(!tmp.empty())ret.push(tmp);
            tmp.clear();
        }
    }
    if(!tmp.empty())ret.push(tmp);
    return ret;
}
}

