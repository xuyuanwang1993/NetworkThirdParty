#include "dns_client.h"
#include <iostream>
using namespace micagent;
using namespace std;
int main()
{
    auto net_info=NETWORK.get_net_interface_info();
    for(auto i:net_info)
    {
        i.dump_info();
    }
    return 0;
    string test_domain="www.test_205.com";
    shared_ptr<EventLoop> loop(new EventLoop());
    dns_client client("139.159.137.87",10000);
#if 0
    auto ret=client.register_to_server(test_domain,"admin","micagent","test");
    cout<<ret.first<<"  "<<ret.second.ToFormattedString()<<endl;
    client.config(loop,test_domain,"admin","micagent");
    client.start_work();
    client.reset_server_info("139.159.137.87",10000);
    ret=client.register_to_server(test_domain,"admin","micagent","test");
    cout<<ret.first<<"  "<<ret.second.ToFormattedString()<<endl;
    client.set_port_map_item("proxy_port",58554,58554);
    client.set_port_map_item("proxy_control",8555,8555);
    while(1)
    {
        sleep(10);
    }
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
