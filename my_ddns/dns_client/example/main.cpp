#include "dns_client.h"
#include <iostream>
using namespace micagent;
using namespace std;
int main()
{
    string test_domain="www.test_207.com";
    shared_ptr<EventLoop> loop(new EventLoop());
    dns_client client("192.168.2.115",10000);
    Logger::Instance().register_handle();
    Logger::Instance().set_log_to_std(true);
#if 1
    client.config(loop,test_domain,"admin","micagent");
    client.start_work();
    client.set_port_map_item("proxy_port",58554,58554);
    client.set_port_map_item("proxy_control",8555,8555);
    while(getchar()!='8')continue;
#else
    while(1){
        auto ret=client.dns_find(test_domain,"admin","micagent");
        cout<<ret.first<<endl<<ret.second.ToFormattedString()<<endl;
        ret=client.port_check();
        cout<<ret.first<<endl<<ret.second.ToFormattedString()<<endl;
        Timer::sleep(5000);
    }
#endif
    client.stop_work();
    loop->stop();
    return 0;
}
