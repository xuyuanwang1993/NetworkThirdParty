#include <string>
#include<iostream>
#include<map>
#include<unistd.h>
#include"live555_client.h"
#include <openssl/ssl.h>
#include "byte_cache_buf.h"
#include <random>
using namespace std;
extern int test_client(int argc, char** argv);
extern shared_ptr<Api_rtsp_server::Rtsp_Handle> g_handle;
int main(int argc,char *argv[])
{
    g_handle.reset(new Api_rtsp_server::Rtsp_Handle);
    Api_rtsp_server::Api_Rtsp_Server_Init_And_Start(g_handle,8554);
    OutPacketBuffer::maxSize=1000000;
     test_client(argc,argv);
//    micagent::Byte_Cache_Buf buf(0,2*1024*1024);
//    buf.bind(&buf);
//    thread get([&buf](){
//        uint8_t recv[1024]={0};
//        struct timeval timestamp;
//        while(1)
//        {
//            uint32_t read_len=1024;
//            auto ret=buf.get_packet_for(&buf,recv,read_len,100,&timestamp);
//            if(!ret&&!buf.get_working_status())break;
//            printf("recv len %u  %ld %ld \r\n",read_len,timestamp.tv_sec,timestamp.tv_usec);
//        }
//        printf("get thread exit!");
//    });
//    buf.set_param(&buf,100);
//    random_device rd;
//    static uint8_t start_code[4]={0x00,0x00,0x00,0x01};
//    uint32_t count=0;
//    while(count++<1000){
//        uint32_t size=(rd()%2048)+1;
//        struct timeval time_now;
//        gettimeofday(&time_now,nullptr);
//        shared_ptr<uint8_t>cache_buf(new uint8_t[size],std::default_delete<uint8_t[]>());
//        buf.insert_buf(&buf,cache_buf.get(),size,time_now,start_code,4);
//        //printf("insert len %u  %ld %ld \r\n",size+4,time_now.tv_sec,time_now.tv_usec);
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//    }
//    buf.reset(&buf);
//    get.join();
//    while(getchar()!='8')continue;
    return 0;
}
