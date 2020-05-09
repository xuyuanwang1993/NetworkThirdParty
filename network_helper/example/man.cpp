#include <string>
#include<algorithm>
#include"event_loop.h"
#include "buffer_handle.h"
#include<iostream>
#include<netdb.h>
#include"tcp_server.h"
#include "http_request.h"
#include"http_response.h"
using namespace std;
using namespace micagent;
#define DECLARE_TEST(name) void name##_test()
#define RUN_TEST(name) name##_test()
#define TEST_WAIT {mutex test;\
    unique_lock<mutex>locker(test);\
    condition_variable con;\
    con.wait(locker);}

DECLARE_TEST(buffer_handle);
DECLARE_TEST(tcp_server);
DECLARE_TEST(http_request);
DECLARE_TEST(http_response);
int main()
{
    Network_Util::DefaultInit();
    Logger::Instance().register_handle();
    Logger::Instance().set_log_to_std(true);
    //RUN_TEST(buffer_handle);
    //RUN_TEST(tcp_server);
    //RUN_TEST(http_request);
    RUN_TEST(http_response);
    Logger::Instance().unregister_handle();
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
DECLARE_TEST(tcp_server)
{
    EventLoop loop;
    shared_ptr<tcp_server>server=make_shared<tcp_server>(8554);
    server->register_handle(&loop);
    TEST_WAIT;
}
DECLARE_TEST(http_request)
{
    http_request test1("/test");
    test1.set_head("User-Agent","Micagent");
    test1.set_head("Date",Logger::Instance().get_local_name());
    test1.set_body("hello!","test");
    MICAGENT_MARK("build  1\r\n%s",test1.build_http_packet(true).c_str());
    string test_packet="POST /test_api HTTP/1.1\r\nHost : micagent\r\nContent-Length : 8\r\n\r\nok111111";
    test1.update(test_packet.c_str(),test_packet.length());
    MICAGENT_MARK("build  2\r\n%s",test1.build_http_packet(true).c_str());

    std::thread t([&test1](){
        MICAGENT_MARK("body for thread get:%s\r\n",test1.get_body(5000).c_str());
        MICAGENT_MARK("Content-Type : %s\r\n",test1.get_head_info("Content-Type").c_str());
    });
    MICAGENT_MARK("start input!");
    for(int i=0;i<test_packet.length();i++){
        test1.update(test_packet.c_str()+i,1);
        putc(test_packet[i],stderr);
        Timer::sleep(10);
    }
    MICAGENT_MARK("end input!");
    t.join();
}
DECLARE_TEST(http_response)
{
    http_response test1("/test");
    test1.set_head("User-Agent","Micagent");
    test1.set_head("Date",Logger::Instance().get_local_name());
    test1.set_body("hello!","test");
    MICAGENT_MARK("build  1\r\n%s",test1.build_http_packet(true).c_str());
    string test_packet="HTTP/1.1 200 OK\r\nHost : micagent\r\nContent-Length : 8\r\n\r\nok111111";
    test1.update(test_packet.c_str(),test_packet.length());
    MICAGENT_MARK("build  2\r\n%s",test1.build_http_packet(true).c_str());

    std::thread t([&test1](){
        MICAGENT_MARK("body for thread get:%s\r\n",test1.get_body(5000).c_str());
        MICAGENT_MARK("Content-Type : %s\r\n",test1.get_head_info("Content-Type").c_str());
        auto info=test1.get_status();
        MICAGENT_MARK("status : %s %s\r\n",info.first.c_str(),info.second.c_str());
    });
    MICAGENT_MARK("start input!");
    for(int i=0;i<test_packet.length();i++){
        test1.update(test_packet.c_str()+i,1);
        putc(test_packet[i],stderr);
        Timer::sleep(10);
    }
    MICAGENT_MARK("end input!");
    t.join();
}
