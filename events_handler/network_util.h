﻿#pragma once
#ifndef SOCKET_UTIL_H
#define SOCKET_UTIL_H
/*
 * 参考live555
 */
 #include <cstdint>
#include <cstring>
#include<string>
#include<vector>
#include<string.h>
#include<mutex>
#if defined(__linux) || defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netpacket/packet.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <net/route.h>
#include <netdb.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>
#define SOCKET int
#define INVALID_SOCKET  (-1)
#define SOCKET_ERROR    (-1)
#elif defined(WIN32) || defined(_WIN32)
#define bzero(a,b) memset(a,0,b)
#define FD_SETSIZE      1024
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#define SHUT_RD 0
#define SHUT_WR 1
#define SHUT_RDWR 2

#else

#endif
namespace micagent {
using namespace  std;
typedef enum{
    TCP=SOCK_STREAM,
    UDP=SOCK_DGRAM
}SOCKET_TYPE;
#define MAKE_ADDR(name,ip,port) sockaddr_in name; \
bzero(&name,sizeof(name));\
name.sin_family = AF_INET;\
name.sin_port = htons(port);\
name.sin_addr.s_addr = inet_addr(ip);

#define NETWORK Network_Util::Instance()
class Network_Util{
    static constexpr  uint32_t MAX_INTERFACES=20;
    static  uint32_t DEFAULT_INTERFACES;
public:
    /*获取实例*/
    static Network_Util&Instance(){static Network_Util instance;return instance; }
    /*配置设备工作默认ip*/
    static void SetDefaultInterface(const string&address){DEFAULT_INTERFACES=inet_addr(address.c_str());}
    /*默认初始化*/
    static void DefaultInit();
    /*配置网卡ip*/
    bool SetNetInterface(uint32_t index,const string&address);
    /*关闭socket*/
    void close_socket(SOCKET sockfd);
    /*创建socket*/
    SOCKET build_socket(SOCKET_TYPE type);
    /*连接socket*/
    bool connect(SOCKET sockfd,string ip,uint16_t port,uint32_t time_out_ms=0);
    /*连接socket*/
    bool connect(SOCKET sockfd,const sockaddr_in &addr,uint32_t time_out_ms=0);
    /*监听*/
    bool listen(SOCKET sockfd,int32_t size=10);
    /*accept建立连接*/
    SOCKET accept(SOCKET sockfd,struct sockaddr_in*addr=nullptr);
    /*设置阻塞模式*/
    bool make_blocking(SOCKET sockfd,uint32_t write_timeout_millseconds=0);
    /*设置非阻塞模式*/
    bool make_noblocking(SOCKET sockfd);
    /*设置写超时*/
    bool set_write_timeout(SOCKET sockfd,uint32_t write_timeout_millseconds=0);
    /*设置TCP的keepalive*/
    bool set_tcp_keepalive(SOCKET sockfd,bool flag,uint32_t try_seconds=30,uint32_t max_tries=4,uint32_t try_interval=5);
    /*忽略sigpipe*/
    void set_ignore_sigpipe(SOCKET sockfd);
    /*设置接收buf*/
    void set_recv_buf_size(SOCKET sockfd, socklen_t buf_size);
    /*获取接收buf大小*/
	socklen_t get_recv_buf_size(SOCKET sockfd);
    /*设置发送buf*/
    void set_send_buf_size(SOCKET sockfd, socklen_t buf_size);
    /*获取发送buf大小*/
	socklen_t get_send_buf_size(SOCKET sockfd);
    /*设置地址复用*/
    void set_reuse_addr(SOCKET sockfd);
    /*设置端口复用*/
    void set_reuse_port(SOCKET sockfd);
    /*绑定端口*/
    bool bind(SOCKET sockfd,uint16_t port=0,uint32_t index=MAX_INTERFACES);
    /*获取TTL值*/
    uint8_t get_ttl(SOCKET sockfd);
    /*获取socket错误信息*/
    int get_socket_error(SOCKET fd);
    /*设置TTL值*/
    bool set_ttl(SOCKET sockfd,uint8_t ttl=64);
    /*设置组播发送网络接口地址*/
    bool set_multicast_if(SOCKET sockfd,uint32_t index=MAX_INTERFACES);
    /*设置回环许可*/
    bool set_multicast_loop(SOCKET sockfd,bool flag=true);
    /*设置组播TTL*/
    bool set_multicast_ttl(SOCKET sockfd,uint8_t ttl=1);
    /*ASM模式多播加入*/
    bool join_asm_multicast(SOCKET sockfd,const string &multicast_ip,uint32_t index=MAX_INTERFACES);
    /*ASM模式多播退出*/
    bool leave_asm_multicast(SOCKET sockfd,const string &multicast_ip,uint32_t index=MAX_INTERFACES);
    /*SSM模式多播加入*/
    bool join_ssm_multicast(SOCKET sockfd,const string &multicast_ip,const vector<string>&included_ip,uint32_t index=MAX_INTERFACES);
    /*SSM模式多播退出*/
    bool leave_ssm_multicast(SOCKET sockfd,const string &multicast_ip,const vector<string>&included_ip,uint32_t index=MAX_INTERFACES);
    /*SFM模式多播添加源,需先加入组播*/
    bool join_sfm_multicast(SOCKET sockfd,const string &multicast_ip,const string &excluded_ip,uint32_t index=MAX_INTERFACES);
    /*SFM模式多播移除源,需先加入组播*/
    bool leave_sfm_multicast(SOCKET sockfd,const string &multicast_ip,const string &excluded_ip,uint32_t index=MAX_INTERFACES);
    /*获取本地IP列表*/
    struct net_interface_info{
        string dev_name;
        string ip;
        string mac;
        net_interface_info(const string &_dev_name,const string & _ip,const string &_mac):dev_name(_dev_name),ip(_ip),mac(_mac){

        }
    };
    /*获取网卡配置信息*/
    const vector<Network_Util::net_interface_info> & get_net_interface_info(bool update=true);
    /*获取本地绑定的端口*/
    uint16_t get_local_port(SOCKET sockfd);
    /*获取本地绑定的IP*/
    string get_local_ip(SOCKET sockfd);
    /*获取对端的端口*/
    uint16_t get_peer_port(SOCKET sockfd);
    /*获取对端的IP*/
    string get_peer_ip(SOCKET sockfd);
    /*获取对端address*/
    bool get_peer_addr(SOCKET sockfd,struct sockaddr_in *addr);
    /*域名解析*/
    string parase_domain(const string &domain_info);
private:
    Network_Util();
    ~Network_Util();
    uint32_t get_netinterface(uint32_t index)const;
    /*inet_addr(const char *)*/
    static bool  check_is_multicast(uint32_t net_ip);
    uint32_t m_netinterface[MAX_INTERFACES];
    vector<net_interface_info> m_net_interface_info_cache;
    mutable mutex m_mutex;
};
}
#endif // SOCKET_UTIL_H
