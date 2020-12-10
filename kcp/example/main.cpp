#include <string>
#include<iostream>
#include<map>
#include<unistd.h>
#include <kcp_common.h>
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
        client_mode(loop,server_ip,local_port,nums);

    }
    return 0;
}

