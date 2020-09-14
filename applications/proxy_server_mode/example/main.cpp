#include "proxy_server_mode.h"
using namespace micagent;
int main(int argc,char *argv[])
{
    string program=argv[0];
    auto pos=program.find('/');
    if(pos!=string::npos)
    {
        program=program.substr(pos+1);
    }
    micagent::Logger::Instance().set_log_to_std(true);
    micagent::Logger::Instance().register_handle();
    if(argc<2)
    {
        MICAGENT_FATALERROR("option1:run with <config_path> to run this program!");
        MICAGENT_FATALERROR("option2:run with <config_path><daemon_config_path> to modify daemon_task!");
    }
    else if (argc==2) {
        MICAGENT_INFO("run program!");
        proxy_server_mode server;
        server.init(argv[1],program);
        thread test([&](){
            sleep(2);
            server.stop();
        });
        server.start();
        test.join();
    }
    else {
        MICAGENT_INFO("generate_daemon_config!");
        proxy_server_mode server;
        server.init(argv[1],program);
        server.generate_daemon_config(program,argv[2]);
    }
    micagent::Logger::Instance().unregister_handle();
    return 0;
}
