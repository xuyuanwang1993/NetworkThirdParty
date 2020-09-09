#include "load_balance_client.h"
#include <iostream>
#include <random>
using namespace micagent;
using namespace std;
string server_ip="192.168.2.111";
uint16_t server_port=10000;
EventLoop loop;
int main(int argc,char *argv[])
{

#if 0
    random_device rd;
    int times=2;//rd()%10+10;
    auto task=[](int index){
    string account="test";
        string domain_name="www.meanning.com"+to_string(index);
        load_balance_client client;
        client.config_server_info(&loop,server_ip,server_port);
        random_device rd;
        uint32_t load_size=50+rd()%50;
        double weight=static_cast<double>(rd())/UINT32_MAX;
        client.config_client_info(account,domain_name,100,0.5);
        client.start_work();
        while(1){
            Timer::sleep(100);
            bool test=(rd()%10)>5;
            if(test)client.increase_load(1);
            else {
                client.decrease_load(1);
            }
        }
        client.stop_work();
    };
    for(int i=0;i<times;i++){
        thread t(task,i);
        t.detach();
    }
while(getchar()!='8')continue;
#else
    load_balance_client client;
    client.config_server_info(nullptr,server_ip,server_port);
    if(argc==3){
        string account=argv[1];
        string domain_name=argv[2];
        auto ret=client.specific_find(account,domain_name);
        cout<<ret.first<<" "<<ret.second.ToFormattedString()<<endl;
    }
    else if(argc==2){
        string account=argv[1];
        auto ret=client.find(account);
        cout<<ret.first<<" "<<ret.second.ToFormattedString()<<endl;
    }
    else {
        _Exit(-1);
    }
    string account=argv[1];
    set<string>exclude_list{"www.meanning.com1"};
    while(1){
        Timer::sleep(1000);
        auto ret=client.find(account,exclude_list);
        cout<<ret.first<<" "<<ret.second.ToFormattedString()<<endl;
    }
#endif
    return 0;
}
