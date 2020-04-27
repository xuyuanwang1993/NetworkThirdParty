#include "thread_pool.h"
#include "timer_queue.h"
#include "trigger_event.h"
#include  "network_util.h"
#include "pipe.h"
#include "task_scheduler.h"
#include <stdio.h>
#include<iostream>
#include"event_loop.h"
#include <test_server.h>
using namespace micagent;
void thread_pool_test();
void time_queue_test();
void trigger_event_test();
void network_util_test();
void task_scheduler_test();
void event_loop_test(int argc,char *argv[]);
int main(int argc,char *argv[]){
    (void) argc;
    (void)argv;
    Logger::Instance().set_log_to_std(true);
    //thread_pool_test();
    //time_queue_test();
   //trigger_event_test();
//    network_util_test();
    //task_scheduler_test();
   event_loop_test(argc,argv);
    return 0;
}
void thread_pool_test(){
    thread_pool threads(50);
    threads.add_thread_task([&]()->bool{
                                static int i=0;
                                MICAGENT_MARK("loop print1!");
                                this_thread::sleep_for(chrono::seconds(2));
                                //if(i==2)threads.stop();
                                return i++<3;
                            });
    int count=0;
    for(int i =0;i<10;i++){
        auto func=[i]()->bool{
                MICAGENT_MARK(" print %d   !\r\n",i);
                //this_thread::sleep_for(chrono::microseconds(2));
                return false;
    };
        while(!threads.add_thread_task(func)){
            //this_thread::sleep_for(chrono::milliseconds(1));
            cout<<"no wvailiable thread! "<<count++<<"           "<<i<<endl;
        }
        //this_thread::sleep_for(chrono::microseconds(2));
    }
    int count2=0;
    while(1){
        int c=getchar();
        if(c=='a'){
            threads.add_thread_task([count2](){
                MICAGENT_MARK("keyboard input %d \r\n",count2);
                return false;
            });
            count2++;
        }
        if(c=='8')break;
    }
    //getchar();
}
void time_queue_test(){
    TimerQueue test;
    std::thread t(&TimerQueue::loop,&test);
    //    auto timer_id=test.addTimer([]()->bool{
    //                                    MICAGENT_MARK("loop timer!  111111111\r\n");
    //                                    return true;
    //                                },1000);
    //    this_thread::sleep_for(chrono::seconds(5));
    //    test.removeTimer(timer_id);
    //    timer_id=test.addTimer([]()->bool{
    //                               MICAGENT_MARK("loop timer!  222222\r\n");
    //                               return true;
    //                           },1000);
    //    this_thread::sleep_for(chrono::seconds(5));
    //    test.removeTimer(timer_id);
    //    this_thread::sleep_for(chrono::seconds(5));
    //    MICAGENT_MARK("add \r\n");
    //    timer_id=test.addTimer([]()->bool{
    //                               MICAGENT_MARK("loop timer!  23333322222\r\n");
    //                               return false;
    //                           },1);
    //    this_thread::sleep_for(chrono::milliseconds(2));
    //    MICAGENT_MARK("add over\r\n");
    //    timer_id=test.addTimer([&]()->bool{
    //                               MICAGENT_MARK("loop timer!  23333322222\r\n");
    //                               test.addTimer([](){MICAGENT_MARK("test add timer in loop!\r\n");return false;},10);
    //                               return false;
    //                           },1);
    //    this_thread::sleep_for(chrono::milliseconds(10));
    //    for(int i=0;i<10000000;i++){
    //        test.addTimer([i](){
    //            MICAGENT_MARK("timer run   %d\r\n",i);
    //            return false;
    //        },0);
    //        MICAGENT_MARK("----------------\r\n");
    //       // this_thread::sleep_for(chrono::milliseconds(1));
    //    }

    //test.stop();
    for(uint32_t i=0;i<10;i++){
        int64_t addtime=Timer::getTimeNow();
        test.addTimer([i,addtime](){
            MICAGENT_MARK("%d mistake %ld ms\r\n",i,Timer::getTimeNow()-addtime-(i+1));
            return false;
        },(i+1));
    }
    while(getchar()!='8');
    test.stop();
     t.join();
}
void trigger_event_test()
{
    trigger_queue test(2,100000);
    // this_thread::sleep_for(chrono::seconds(3));
    uint32_t i=0;
    atomic<int> count(0);
    int64_t begin_time=Timer::getTimeNow();
    atomic<int64_t> last_time(Timer::getTimeNow());
    uint32_t counts=10;
    for(;i<counts;i++){
        if(!test.add_trigger_event([&](){
                                   last_time.exchange(Timer::getTimeNow());
                                   count++;
    })){
            MICAGENT_MARK("add_trigger_event failed! %d \r\n",i);
        }
    }
    while(getchar()!='8'){
        cout<<"execute "<<count<<" triggers using "<<last_time-begin_time<<" ms"<<endl;
    }
}
void network_util_test()
{
	Pipe m_pipe;
	m_pipe.open();
	SOCKET fd = m_pipe();
	thread t([fd]() {
		while (1) {
			send(fd, "test", sizeof("test"), 0);
			Timer::sleep(30);
		}
		});
	char buf[128] = { 0 };
    while(1){
		auto ret = recv(fd, buf, 128,0);
        if (ret > 0)MICAGENT_MARK("11111111111 %s\r\n", buf);
		else {
			//perror("recv ");
		}
		Timer::sleep(30);
    }
}
void task_scheduler_test()
{
    SelectTaskScheduler task(1);
    //EpollTaskScheduler task(1);
    auto fd=Network_Util::Instance().build_socket(UDP);
    Network_Util::Instance().make_noblocking(fd);
    Network_Util::Instance().bind(fd,10000);
    Network_Util::Instance().connect(fd,"192.168.2.199",10101);
    ChannelPtr channel(new Channel(fd));
    cout<<fd<<endl;
    channel->setReadCallback([fd](Channel *tmp){
        (void)tmp;
        char buf[1024]={0};
        int64_t len=recv(fd,buf,1024,0);
        if(len>0){
            send(fd,buf,static_cast<size_t>(len),0);
        }
        MICAGENT_MARK("recv %s \r\n",buf);
        return true;
    });
    channel->enableReading();
    task.updateChannel(channel);
    std::thread t(&TaskScheduler::start,&task);
    MICAGENT_MARK("wait---------------\r\n");
    Timer::sleep(10000);
    MICAGENT_MARK("wait----finished-----------\r\n");
    task.stop();
    t.join();
}

void server_mode(int32_t thread_pool_size,uint32_t trigger_threads,uint32_t capacity,uint32_t thread_nums)
{
    test_server server(thread_pool_size,trigger_threads,capacity,thread_nums);
    server.run();
}
void client_mode(string ip)
{
    auto sock=Network_Util::Instance().build_socket(UDP);
    Network_Util::Instance().connect(sock,ip,10000);
    Network_Util::Instance().make_noblocking(sock);
    test_server::print_cmd_help();
    bool is_exit=false;
    std::thread t([&](){
        char buf[1024]={0};
        while(!is_exit){
            if(recv(sock,buf,1024,0)>0)MICAGENT_MARK("\r\nresponse:%s\r\n",buf);
        }
    });
    string input;
    char buf[2048];
    while(1){
        memset(buf,0,1024);
        input.clear();
        cout<<"input you command:\t";
        cin.getline(buf,1024);
        input=buf;
        if(input=="88")break;
        else if(input=="h")test_server::print_cmd_help();
        else {
            send(sock,input.c_str(),input.length(),0);
        }
    }
    is_exit=true;
    t.join();
}
void listen_mode(uint16_t port){
    SelectTaskScheduler task(1);
    auto fd=Network_Util::Instance().build_socket(TCP);
    Network_Util::Instance().make_noblocking(fd);
    Network_Util::Instance().bind(fd,port);
    Network_Util::Instance().listen(fd,20);
    ChannelPtr channel(new Channel(fd));
    channel->setReadCallback([&](Channel *chn){
        auto sock=Network_Util::Instance().accept(chn->fd());
        if(sock>0){
            auto new_channel=make_shared<Channel>(sock);
            new_channel->enableReading();
            new_channel->setReadCallback([&](Channel *chn){
                shared_ptr<char>buf(new char[2048]);
                auto len=recv(chn->fd(),buf.get(),2048,0);
                if(len==0)return false;
                if(len>0){
                    MICAGENT_MARK("%s\r\n",buf.get());
                }
                return true;
            });
            new_channel->setCloseCallback([&](Channel *chn){
                task.removeChannel(chn->fd());
                return true;
            });
            task.updateChannel(new_channel);
        }
        return true;
    });
    channel->enableReading();
    task.updateChannel(channel);
    std::thread t(&SelectTaskScheduler::start,&task);
    t.join();
}
void recv_mode(uint16_t port){
    SelectTaskScheduler task(1);
    auto fd=Network_Util::Instance().build_socket(UDP);
    Network_Util::Instance().make_noblocking(fd);
    Network_Util::Instance().bind(fd,port);
    ChannelPtr channel(new Channel(fd));
    channel->setReadCallback([fd](Channel *tmp){
        (void)tmp;
        char buf[1024]={0};
        int64_t len=recv(fd,buf,1024,0);
        if(len>0){
            MICAGENT_MARK("%s\r\n",buf);
        }
        return true;
    });
    channel->enableReading();
    task.updateChannel(channel);
    std::thread t(&SelectTaskScheduler::start,&task);
    t.join();
}
void tcp_recv_mode(string ip,uint16_t port){
    SelectTaskScheduler task(1);
    auto fd=Network_Util::Instance().build_socket(TCP);
    Network_Util::Instance().connect(fd,ip,port);
    Network_Util::Instance().make_noblocking(fd);
    ChannelPtr channel(new Channel(fd));
    channel->setReadCallback([fd](Channel *tmp){
        (void)tmp;
        char buf[1024]={0};
        int64_t len=recv(fd,buf,1024,0);
        if(len>0){
            MICAGENT_MARK("%s\r\n",buf);
        }
        return true;
    });
    channel->enableReading();
    task.updateChannel(channel);
    std::thread t(&SelectTaskScheduler::start,&task);
    t.join();
}
void event_loop_print()
{
    printf("-h|--help \tshow this page!!\r\n");
    printf("-s|--server [-p thread_pool_size(2-256)] [-c capacity(20-1000000)] [-r trigger_threads(1-4)] [-n io_thread_nums(1-10)]\trun as a server!\r\n");
    printf("-c|--client <server_ip>\tserver ip is the server's ip that you wish to control!\r\n");
    printf("-l|--listen <listen_port>\tcreate a listen server who will print all buf the connections recv!\r\n");
    printf("-u|--udp <udp_port>\tcreate a udp recv who will print the recv message!\r\n");
    printf("-t|--tcp <remote_ip><remote_port>\tcreate a tcp recv who will print the recv message!\r\n");
}
void event_loop_test(int argc,char *argv[])
{
    if(argc<2){
        event_loop_print();
        exit(-1);
    }
    argc--;argv++;
    int32_t thread_pool_size=32;//2-256
    uint32_t capacity=2000;//20-1000000
    uint32_t trigger_threads=2;//1-4
    uint32_t thread_nums=2;//1-10
    string ip="0.0.0.0";
    uint16_t port=0;
    argc--;
    string tmp=*argv;
    argv++;
    if(tmp=="-h"||tmp=="--help")event_loop_print();
    else if(tmp=="-s"||tmp=="--server"){
        while(argc>0){
            argc--;
            tmp=*argv;
            argv++;
            if(tmp=="-p"){
                if(argc>0){
                    argc--;
                    tmp=*argv;
                    argv++;
                    thread_pool_size=stoi(tmp);
                    if(thread_pool_size<2||thread_pool_size>256)throw invalid_argument("thread_pool_size");
                }
            }
            else if(tmp=="-c"){
                if(argc>0){
                    argc--;
                    tmp=*argv;
                    argv++;
                    capacity=stoul(tmp);
                    if(capacity<20||capacity>1000000)throw invalid_argument("capacity");
                }
            }
            else if(tmp=="-r"){
                if(argc>0){
                    argc--;
                    tmp=*argv;
                    argv++;
                    trigger_threads=stoul(tmp);
                    if(trigger_threads<1||trigger_threads>4)throw invalid_argument("trigger_threads");
                }
            }
            else if(tmp=="-n"){
                if(argc>0){
                    argc--;
                    tmp=*argv;
                    argv++;
                    thread_nums=stoul(tmp);
                    if(thread_nums<1||thread_nums>10)throw invalid_argument("io_thread_nums");
                }
            }
        }
        MICAGENT_MARK("server_mode\r\n");
        server_mode(thread_pool_size,trigger_threads,capacity,thread_nums);
        MICAGENT_MARK("server_mode exit\r\n");
    }
    else if (tmp=="-c"||tmp=="--client") {
        if(argc>0)ip=*argv;
        sockaddr_in addr;
        if(inet_pton(AF_INET,ip.c_str(),&addr.sin_addr.s_addr)<=0)throw invalid_argument("ip");
        client_mode(ip);
    }
    else if(tmp=="-l"||tmp=="--listen"){
        if(argc>0)port=stoul(*argv);
        listen_mode(port);
    }
    else if(tmp=="-u"||tmp=="--udp"){
        if(argc>0)port=stoul(*argv);
        recv_mode(port);
    }
    else if(tmp=="-t"||tmp=="--tcp"){
        if(argc>0){
            ip=*argv;
            argc--;
            argv++;
        }
        if(argc>0)port=stoul(*argv);
        tcp_recv_mode(ip,port);
    }
    else {
        fprintf(stderr,"run with -h|--help to get help info!\r\n");
    }
}
