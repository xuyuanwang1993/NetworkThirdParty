#include <string>
#include<iostream>
#include<map>
#include<unistd.h>
#include <kcp_common.h>
#include "kcp_proxy_interface.h"
using namespace std;
using namespace micagent;
#define BASE_PORT 8092
#define BUF_SIZE 20000
void test_func();
int main(int argc,char *argv[])
{

    Logger::Instance().register_handle();
    Logger::Instance().set_log_to_std(true);
    Logger::Instance().set_clear_flag(true);
    Logger::Instance().set_log_file_size(200*1024);
    test_func();
    return 0;
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

    }
    return 0;
}

void test_func()
{
    ikcp_proxy_header_s header;
    header.sn=100;
    header.conv_id=54654;
    header.cmd=10;
    header.protocal_version=2;
    header.timestamp=51312312;
    char buf[12]={0};
    ikcp_proxy_helper::encode_ikcp_proxy_header(buf,12,header);
    ikcp_proxy_header_s header2;
    ikcp_proxy_helper::decode_ikcp_proxy_header(buf,12,header2);
    printf("%d %d %d %d %d\r\n",header2.sn,header2.cmd,header2.conv_id,header2.timestamp,header2.protocal_version);
    ikcp_config_s config;
    config.nc=0;
    config.resend=123;
    config.nodelay=1;
    config.interval=16383;
    config.window_size=62234;
    char bu3f[5]={0};
    ikcp_proxy_helper::encode_ikcp_config(bu3f,5,config);
    ikcp_config_s config2;
    ikcp_proxy_helper::decode_ikcp_config(bu3f,5,config2);
    printf("%d %d %d %u %d\r\n",config2.nc,config2.resend,config2.nodelay,config2.interval,config2.window_size);
    uint64_t send_count=0;
    uint64_t recv_count=0;
    shared_ptr<EventLoop>loop(new EventLoop());
    shared_ptr<kcp_proxy_interface> send_interface(new kcp_proxy_interface(loop,10000));
    send_interface->init([&](const ikcp_raw_udp_packet_s&data){
        printf("send recv %s\r\n",data.buf.get());
    });
    MAKE_ADDR(addr,"127.0.0.1",10000);
    MAKE_ADDR(addr1,"127.0.0.1",10001);
    shared_ptr<kcp_proxy_interface> recv_interface(new kcp_proxy_interface(loop,10001));
    recv_interface->init([&](const ikcp_raw_udp_packet_s&data){
        if(!recv_interface->send_raw_data(data)){
            printf("sink send error!\r\n");
        }
        printf("recv %s\r\n",data.buf.get());
        auto recv_count_tmp=stoul((char*)data.buf.get());
        if(recv_count_tmp-recv_count>2)exit(-1);
        recv_count=recv_count_tmp;
    });


    //   thread send_thread([&](){
    //       while (1) {
    //           shared_ptr<uint8_t>buf(new uint8_t[2048],default_delete<uint8_t[]>());
    //           memset(buf.get(),0,32);
    //           sprintf((char *)buf.get(),"%ld",send_count++);
    //           if(!send_interface->send_raw_data(ikcp_raw_udp_packet_s(buf,2048,addr1)))
    //           {
    //               printf("source send error!\r\n");
    //           }
    //           this_thread::sleep_for(chrono::nanoseconds(1));
    //       }
    //   });
    while (getchar()!='8') {
        continue;
    }
}
