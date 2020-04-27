#include <string>
#include<algorithm>
#include"event_loop.h"
#include "buffer_handle.h"
#include<iostream>
#include<netdb.h>
using namespace std;
using namespace micagent;
#define DECLARE_TEST(name) void name##_test()
#define RUN_TEST(name) name##_test()
#define TEST_WAIT {mutex test;\
    unique_lock<mutex>locker(test);\
    condition_variable con;\
    con.wait(locker);}

DECLARE_TEST(buffer_handle);
int main()
{
    //RUN_TEST(buffer_handle);
    struct protoent *protocol=getprotobyname("arp");
    perror("a");
    if(protocol)
    printf("%d \r\n",protocol->p_proto);
   return 0;
}
DECLARE_TEST(buffer_handle)
{
    buffer_handle buf(128,nullptr,false);
    EventLoop loop(128);
    auto sock=Network_Util::Instance().build_socket(TCP);
    Network_Util::Instance().set_reuse_addr(sock);
    Network_Util::Instance().bind(sock,10001);
    Network_Util::Instance().connect(sock,"192.168.2.199",8554);
    Network_Util::Instance().make_noblocking(sock);
    ChannelPtr new_channel(new Channel(sock));
    Logger::Instance().set_log_to_std(true);
    new_channel->enableReading();
    new_channel->setReadCallback([&](Channel *chn){
        auto len=buf.read_fd(chn->fd());
        if(len>0){
            chn->enableWriting();
            loop.updateChannel(chn);
            return true;
        }
        else {
            return false;
        }
    });
    new_channel->setWriteCallback([&](Channel *chn){
        auto ret=buf.send_fd(chn->fd());
        chn->disableWriting();
        loop.updateChannel(chn);
        return  ret>=0;
    });
    new_channel->setCloseCallback([&](Channel *chn){
        exit(-1);
        return false;
    });
    loop.updateChannel(new_channel);
    TEST_WAIT;
}
