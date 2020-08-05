#include "kcp_manage.h"
#include <string>
#include<iostream>
#include<map>
#include<unistd.h>
#include<kcp_server.h>
using namespace std;
using namespace micagent;
#define BASE_PORT 8092
#define BUF_SIZE 20000
void server_mode(shared_ptr<EventLoop> loop);
void client_mode(shared_ptr<EventLoop> loop,string server_ip,uint16_t local_port,uint32_t nums);
void kcp_mem_test(uint16_t port);
int main(int argc,char *argv[])
{
    Logger::Instance().register_handle();
    Logger::Instance().set_log_to_std(true);
    Logger::Instance().set_clear_flag(true);
    Logger::Instance().set_log_file_size(200*1024);
    Network_Util::Instance().SetDefaultInterface("192.168.2.111");
    opterr=1;
    char chn;
    string server_ip="192.168.2.111";
    uint16_t local_port=0;
    uint32_t nums=0;
    shared_ptr<EventLoop> loop(new EventLoop(32));
    while((chn=getopt(argc,argv,"shi:l:n:"))!=-1){
        switch(chn){
        case 's':
            MICAGENT_MARK("server mode!");
            server_mode(loop);
            break;
        case 'h':
            printf(" -s  \t server_mode\r\n");
            printf(" -i  \t server_ip\r\n");
            printf(" -l  \t local_base_port\r\n");
            printf(" -n  \t nums_connection\r\n");
            break;
        case 'i':
            MICAGENT_MARK("opt server_Ip :%s!",optarg);
            server_ip=optarg;
            break;
        case 'l':
            MICAGENT_MARK("opt local_port :%s!",optarg);
            local_port=stoul(optarg);
            break;
        case 'n':
            MICAGENT_MARK("opt nums :%s!",optarg);
            nums=stoul(optarg);
            break;
        default:
            MICAGENT_MARK("unknown opt :%s!",optarg);
            break;
        }
    }
    if(local_port==0){
        MICAGENT_MARK("no local_port!");
    }
    else {
        MICAGENT_MARK("server_ip %s local_port %d!",server_ip.c_str(),local_port);
        kcp_manager::GetInstance().Config(loop.get());
        kcp_manager::GetInstance().StartUpdateLoop();
        client_mode(loop,server_ip,local_port,nums);

    }
    #ifdef DEBUG
    MICAGENT_MARK("loop exit begin!");
    #endif
    loop.reset();
        #ifdef DEBUG
    MICAGENT_MARK("loop exit end!");
    #endif
    return 0;
}
void server_mode(shared_ptr<EventLoop> loop)
{
    SOCKET server_fd=Network_Util::Instance().build_socket(TCP);
    shared_ptr<kcp_server> test_server (new kcp_server(BASE_PORT,loop.get()));
    if(test_server->start_update_loop(10)){
        MICAGENT_MARK("kcp_server_run!");
    }
    Network_Util::Instance().bind(server_fd,BASE_PORT);
    Network_Util::Instance().listen(server_fd);
    Network_Util::Instance().set_ignore_sigpipe(server_fd);
    ChannelPtr server_chn(new Channel(server_fd));
    server_chn->enableReading();
    server_chn->setReadCallback([loop,test_server](Channel *chn){
        SOCKET new_fd=Network_Util::Instance().accept(chn->fd());
        MICAGENT_LOG(LOG_INFO,"accept %d!",new_fd);
        if(new_fd>0){
            ChannelPtr new_channel(new Channel(new_fd));
            new_channel->enableReading();
            uint32_t conv_id=test_server->get_session_id();
            new_channel->setReadCallback([conv_id,test_server,loop](Channel *chn){
                uint16_t port;
                auto len=recv(chn->fd(),&port,sizeof (port),0);
                if(len==0){
                    return false;
                }
                if(len>0){
                    struct sockaddr_in addr ;
                    bzero(&addr,sizeof (addr));
                    socklen_t addrlen = sizeof(struct sockaddr_in);
                    if (getpeername(chn->fd(), (struct sockaddr *)&addr, &addrlen) == 0)
                    {
                        MICAGENT_LOG(LOG_INFO,"recv from %s %hu for port %hu!",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port),port);
                        addr.sin_port=htons(port);
                        if(test_server->add_session(conv_id,addr))MICAGENT_MARK("add %d session success!",conv_id);
                        else {
                            MICAGENT_MARK("add %d session failed!",conv_id);
                        }
                        send(chn->fd(),&conv_id,sizeof (conv_id),0);
                        Network_Util::Instance().make_noblocking(chn->fd());
                    }
                }
                return true;
            });
            new_channel->setCloseCallback([loop,conv_id,test_server](Channel *chn){
                loop->removeChannel(chn->fd());
                test_server->remove_session(conv_id);
                MICAGENT_LOG(LOG_ERROR,"conv_id %d exit!",conv_id);
                return false;
            });
            loop->updateChannel(new_channel);
        }
        return true;
    });
    loop->updateChannel(server_chn);
    atomic_bool is_exit(false);
    std::thread t([&](){
                             uint32_t index=0;
                             while(!is_exit){
                                 test_server->kcp_send_multicast_packet(&index,sizeof (index));
                                 Timer::sleep(1);
                                 index++;
                             }
                             return false;
                         });
    while(getchar()!='8')continue;
    loop->stop();
    Logger::Instance().unregister_handle();
   is_exit.exchange(true);
   loop->removeChannel(server_chn);
    if(t.joinable()) t.join();
    test_server->clear();
    test_server.reset();
}
void client_mode(shared_ptr<EventLoop> loop,string server_ip,uint16_t local_port,uint32_t nums)
{
    shared_ptr<map<SOCKET,uint32_t>>save(new map<SOCKET,uint32_t>);
    shared_ptr<mutex>m_save_mutex(new mutex);
    while(nums-->0){
        auto func=[](const char *buf ,int len ,struct IKCPCB *kcp,void *user_data){
        uint32_t index=*((uint32_t *)buf);
        //MICAGENT_LOG(LOG_DEBUG,"%u    %u",kcp->conv,index);
    };
        uint16_t port=local_port+nums;
        SOCKET fd=Network_Util::Instance().build_socket(TCP);
        ChannelPtr channel(new Channel(fd));
        channel->enableReading();
        channel->setReadCallback([m_save_mutex,channel,port,fd,save,loop,server_ip,func](Channel *chn){
            int recv_len;
            uint32_t conv_id=0;
            if((recv_len=recv(fd,&conv_id,sizeof (conv_id),0))>0){
                MICAGENT_LOG(LOG_INFO,"conv_id %d  for port %hu!",conv_id,port);
                auto conn=kcp_manager::GetInstance().AddConnection(port,server_ip.c_str(),BASE_PORT,conv_id,func);
                conn->start_work();
                m_save_mutex->lock();
                save->emplace(make_pair(fd,conv_id));
                m_save_mutex->unlock();
                channel->setCloseCallback([loop,conv_id,save,m_save_mutex](Channel *chn){
                    loop->removeChannel(chn->fd());
                    kcp_manager::GetInstance().CloseConnection(conv_id);
                    m_save_mutex->lock();
                    save->erase(chn->fd());
                    m_save_mutex->unlock();
                    return false;
         });
            }
            else if(recv_len==0){
                MICAGENT_MARK("  %d  %s",recv_len,strerror(errno));
                return false;
            }
            return true;
        });
        channel->setCloseCallback([loop](Channel *chn){
                    loop->removeChannel(chn->fd());
                    return false;
         });

        if(Network_Util::Instance().connect(fd,server_ip,BASE_PORT,2000)){
            Network_Util::Instance().set_ignore_sigpipe(fd);
            send(fd,&port,sizeof (port),0);
            Network_Util::Instance().make_noblocking(fd);
            loop->updateChannel(channel);
        }
        else {
            MICAGENT_LOG(LOG_ERROR,"fail to build kcp connect with port %hu!",local_port);
        }
        Timer::sleep(10);
    }
    //while(getchar()!='8')continue;
    Timer::sleep(10000);
    kcp_manager::GetInstance().StopUpdateLoop();
    m_save_mutex->lock();
    for(auto i :*save.get()){
        kcp_manager::GetInstance().CloseConnection(i.second);
        loop->removeChannel(i.first);
    }
    m_save_mutex->unlock();
}
void kcp_mem_test(uint16_t port)
{
    SOCKET send_fd=Network_Util::Instance().build_socket(UDP);
    Network_Util::Instance().bind(send_fd,port);
    Network_Util::Instance().connect(send_fd,"192.168.2.199",port);
    auto kcp=ikcp_create(0,&send_fd);
    ikcp_nodelay(kcp, 1, 10, 2, 1);
    kcp->output=[](const char *buf, int len, struct IKCPCB *kcp, void *_user)->int{
    SOCKET fd=*static_cast<SOCKET*>(_user);
            return ::send(fd,buf,len,0);
        };
    char buf[10000]={0};
    memset(buf,'a',10000);
    uint32_t count=10000;
    while(count-->0){
        ikcp_send(kcp,buf,1000);
        ikcp_flush(kcp);
        Timer::sleep(1);
        printf("%d \r\n",count);
    }
    ikcp_release(kcp);
    Timer::sleep(10000);
}
