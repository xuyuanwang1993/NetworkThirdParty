#include "dns_server.h"
using namespace std;
using namespace micagent;
int main()
{
    printf("dns_server start!\r\n");
    EventLoop loop(0,1);
    dns_server server(10000);
    server.config(&loop);
    server.start_work();
    while(getchar()!='8')continue;
    server.stop_work();
    loop.stop();
    return 0;
}
