#include "dns_client.h"
#include <iostream>
using namespace micagent;
using namespace std;
int main()
{
    string test_domain="www.meanning.com";
    EventLoop loop;
    dns_client client("192.168.2.111",10000);
    #if 1
    auto ret=client.register_to_server("www.meanning.com","admin","micagent","test");
    cout<<ret.first<<"  "<<ret.second.ToFormattedString()<<endl;
    client.config(&loop,"www.meanning.com","admin","micagent");
    client.start_work();
    client.set_port_map_item("rtsp",58554,554);
    client.set_port_map_item("rtmp",58555,555);
#else
    while(1){
        auto ret=client.dns_find(test_domain,"admin","micagent");
        cout<<ret.first<<endl<<ret.second.ToFormattedString()<<endl;
        Timer::sleep(5000);
    }
#endif
    while(getchar()!='8')continue;
    client.stop_work();
    loop.stop();
    return 0;
}
