#include "dns_client.h"
#include <iostream>
using namespace micagent;
using namespace std;
int main()
{
    string test_domain="www.meanning_test.com";
    EventLoop loop;
    dns_client client("139.159.137.87",10000);
#if 1
    auto ret=client.register_to_server(test_domain,"admin","micagent","test");
    cout<<ret.first<<"  "<<ret.second.ToFormattedString()<<endl;
    client.config(&loop,test_domain,"admin","micagent");
    client.start_work();
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
    loop.stop();
    return 0;
}
