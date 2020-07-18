#include <sys/select.h>
#include <sys/socket.h>
#include <stdio.h>
#include<random>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include<string>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include<unistd.h>
int main( int argc ,char *argv[])
{
    for(int i=0;i<argc;i++)
    {
        printf("%s\r\n",argv[i]);
    }
    std::random_device rd;
    bool ret=rd()%2;
    int sleep_time=rd()%15+15;
    int fd=socket(AF_INET,SOCK_STREAM,0);
    sockaddr_in addr;
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr("0.0.0.0");
    if(ret)
    {
        addr.sin_port=htons(9000);
    }
    else {
        addr.sin_port=htons(9001);
    }
    ::bind(fd,(sockaddr *)&addr,sizeof (addr));
    listen(fd,1);
    while(1)
    {
        sleep(sleep_time);
 //       abort();
    }
}
