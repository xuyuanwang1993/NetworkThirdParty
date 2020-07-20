#include <string>
#include<iostream>
#include<map>
#include<unistd.h>
#include"live555_client.h"
#include <openssl/ssl.h>
using namespace std;
extern int test_client(int argc, char** argv);
extern shared_ptr<Api_rtsp_server::Rtsp_Handle> g_handle;
int main(int argc,char *argv[])
{
    g_handle.reset(new Api_rtsp_server::Rtsp_Handle);
    Api_rtsp_server::Api_Rtsp_Server_Init_And_Start(g_handle,58555);
    OutPacketBuffer::maxSize=1000000;
     test_client(argc,argv);
    return 0;
}
