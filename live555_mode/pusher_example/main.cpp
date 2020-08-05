#include <string>
#include<iostream>
#include<map>
#include<unistd.h>
#include"live555_client.h"
#include <openssl/ssl.h>
using namespace std;
extern int test_client(int argc, char** argv);
int main(int argc,char *argv[])
{
    OutPacketBuffer::maxSize=1000000;
     test_client(argc,argv);
    return 0;
}
