#include "dns_server.h"
using namespace std;
using namespace micagent;
int main()
{
    printf("dns_server start!\r\n");
    shared_ptr<EventLoop>loop(new EventLoop(0,1));
    dns_server server(10000);
    server.config(loop);
    server.start_work();
    while(getchar()!='8')continue;
    server.stop_work();
    loop->stop();
    return 0;
}
