#include "load_balance_server.h"
#include <iostream>
using namespace std;
using namespace micagent;
int main()
{
    EventLoop loop;
    load_balance_server server(10000);
    server.config(&loop);
    server.start_work();
    while(getchar()!='8')continue;
    server.stop_work();
    loop.stop();
    return 0;
}
