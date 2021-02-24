#include <unistd.h>
#include <stdio.h>
#include<memory>
#include "unix_socket_helper.h"
#include"CJsonObject.hpp"
using namespace  std;
using namespace  neb;
void Get_Mode_Handle(void);
void Post_Mode_Handle(void);
int main(int argc,char **argv)
//int cgiMain()
{
    printf("Content-type:application/json;charset=utf-8\n");	//response header

    if(!getenv("REQUEST_METHOD"))
    {
        printf("error:getenv false");
        return -1;
    }
    auto request_method = getenv("REQUEST_METHOD");

    if(0==strcmp(request_method,"GET"))
    {
        Get_Mode_Handle();
    }
    else if(0==strcmp(request_method,"POST"))
    {
        Post_Mode_Handle();
    }
    else{
        printf("error:request_method false");
    }

    fflush(stdout);
    return 0;
}



void Get_Mode_Handle()
{
    auto UserInput = getenv("QUERY_STRING");
    auto data_len = strlen(UserInput);
    if(data_len == 0||UserInput == nullptr)
    {
        printf("error:GET no data");
        return;
    }
    else {
        printf("not supported!");
    }
}

void Post_Mode_Handle()
{
    size_t ByteLength;//
    ByteLength = stoul(getenv("CONTENT_LENGTH"));
    shared_ptr<char>word(new char[unix_socket_base::PATH_MAX_SIZE+ByteLength+1],default_delete<char[]>());


    fread(word.get()+unix_socket_base::PATH_MAX_SIZE,1,ByteLength,stdin);
    word.get()[unix_socket_base::PATH_MAX_SIZE+ByteLength]='\0';
    string socke_name="/tmp/cgi_";
    socke_name+=to_string(getpid());
    unix_dgram_socket io_socket(socke_name);
    io_socket.build();
    strncpy(word.get(),io_socket.get_local_path().c_str(),unix_socket_base::PATH_MAX_SIZE);
    io_socket.send(word.get(),unix_socket_base::PATH_MAX_SIZE+ByteLength,"/tmp/proxy_server");
    uint32_t recv_size=30*1024;
    shared_ptr<char>recv_buf(new char[recv_size],default_delete<char[]>());
    memset(recv_buf.get(),0,recv_size);
    auto recv_len=io_socket.recv(recv_buf.get(),recv_size);
    if(recv_len>0){
        printf("CONTENT_LENGTH: %ld\n",recv_len);
        recv_buf.get()[recv_len]='\0';
    }
    printf("\n");
    printf("%s",recv_buf.get());
}


