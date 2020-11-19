#include "network_util.h"
#include <map>
#include<memory>
#define UNUSED(x) (void)x
#if defined(WIN32) || defined(_WIN32)
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#elif defined(__linux) || defined(__linux__)
#include <netinet/tcp.h>
#include<signal.h>
#endif
#if defined(__linux) || defined(__linux__)
#elif defined(WIN32) || defined(_WIN32)
#endif
#include<vector>
#include<cstdio>
namespace micagent {
uint32_t Network_Util::DEFAULT_INTERFACES=INADDR_ANY;
void Network_Util::DefaultInit()
{
    auto &interface=Network_Util::Instance();
    auto netinterface=interface.get_net_interface_info();
    for(uint32_t i=0;i<netinterface.size();i++){
        if(i==0)interface.SetDefaultInterface(netinterface[i].ip);
        interface.SetNetInterface(i,netinterface[i].ip);
    }
}
bool Network_Util::SetNetInterface(uint32_t index,const string&address){
    lock_guard<mutex> locker(m_mutex);
    if(index>=MAX_INTERFACES)return false;
    return inet_pton(AF_INET,address.c_str(),&m_netinterface[index])>0;
}
void Network_Util::close_socket(SOCKET sockfd)
{
#if defined(__linux) || defined(__linux__)
    ::close(sockfd);
#elif defined(WIN32) || defined(_WIN32)
    ::closesocket(sockfd);
#endif
}
SOCKET Network_Util::build_socket(SOCKET_TYPE type)
{
    if(type!=SCTP)return socket(AF_INET,type,0);
    else {
        return socket(AF_INET,SCTP,IPPROTO_SCTP);
    }
}
SOCKET Network_Util::build_unix_socket(SOCKET_TYPE type,const string &path)
{
    SOCKET fd=socket(PF_UNIX,type,0);
    if(!path.empty()&&fd!=INVALID_SOCKET)
    {
        sockaddr_un addr;
        addr.sun_family=AF_UNIX;
        strncpy(addr.sun_path,path.c_str(),108);
        unlink(addr.sun_path);
        auto ret =::bind(fd,reinterpret_cast<sockaddr *>(&addr),sizeof (addr));
        if(ret!=0)
        {
            NETWORK.close_socket(fd);
            fd=INVALID_SOCKET;
        }
    }
    return fd;
}
bool Network_Util::connect(SOCKET sockfd,string ip,uint16_t port,uint32_t time_out_ms)
{
    bool isConnected = true;
    if (time_out_ms > 0)
    {
        make_noblocking(sockfd);
    }
    struct sockaddr_in addr ;
    bzero(&addr,sizeof (addr));
    socklen_t addrlen = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (::connect(sockfd, (struct sockaddr*)&addr, addrlen) == SOCKET_ERROR)
    {
        if (time_out_ms > 0)
        {
            isConnected = false;
            fd_set fdWrite;
            FD_ZERO(&fdWrite);
            FD_SET(sockfd, &fdWrite);
            struct timeval tv = { static_cast<__time_t>(time_out_ms / 1000), static_cast<__suseconds_t>(time_out_ms % 1000 * 1000 )};
            select(sockfd + 1, nullptr, &fdWrite, nullptr, &tv);
            if (FD_ISSET(sockfd, &fdWrite))
            {
                isConnected = true;
            }
            make_blocking(sockfd,time_out_ms);
        }
        else
        {
            isConnected = false;
        }
    }

    return isConnected;
}
bool Network_Util::connect(SOCKET sockfd,const sockaddr_in &addr,uint32_t time_out_ms)
{
    bool isConnected = true;
    if (time_out_ms > 0)
    {
        make_noblocking(sockfd);
    }
    socklen_t addrlen = sizeof(addr);
    if (::connect(sockfd, (struct sockaddr*)&addr, addrlen) == SOCKET_ERROR)
    {
        if (time_out_ms > 0)
        {
            isConnected = false;
            fd_set fdWrite;
            FD_ZERO(&fdWrite);
            FD_SET(sockfd, &fdWrite);
            struct timeval tv = { static_cast<__time_t>(time_out_ms / 1000), static_cast<__suseconds_t>(time_out_ms % 1000 * 1000 )};
            select(sockfd + 1, nullptr, &fdWrite, nullptr, &tv);
            if (FD_ISSET(sockfd, &fdWrite))
            {
                isConnected = true;
            }
            make_blocking(sockfd,time_out_ms);
        }
        else
        {
            isConnected = false;
        }
    }

    return isConnected;
}
bool Network_Util::listen(SOCKET sockfd,int32_t size)
{
    if(::listen(sockfd, size) == SOCKET_ERROR)
    {
        return false;
    }
    return true;
}
SOCKET Network_Util::accept(SOCKET sockfd,struct sockaddr_in*addr){
    socklen_t addrlen = sizeof (struct sockaddr_in);
    SOCKET clientfd=INVALID_SOCKET;
    if(addr){
        clientfd = ::accept(sockfd, (struct sockaddr*)addr, &addrlen);
    }
    else {
        struct sockaddr_in addr_tmp ;
        bzero(&addr_tmp,sizeof(addr_tmp));
        clientfd = ::accept(sockfd, (struct sockaddr*)&addr_tmp, &addrlen);
    }
    return clientfd;
}
bool Network_Util::make_blocking(SOCKET sockfd, uint32_t write_timeout_millseconds)
{
    bool result=false;
#if defined(__WIN32__) || defined(_WIN32)
    unsigned long arg = 0;
    result = ioctlsocket(sockfd, FIONBIO, &arg) == 0;
#elif defined(__linux) || defined(__linux__)
    int curFlags = fcntl(sockfd, F_GETFL, 0);
    result = fcntl(sockfd, F_SETFL, curFlags&(~O_NONBLOCK)) >= 0;
#endif

    if (write_timeout_millseconds > 0) {
#ifdef SO_SNDTIMEO
#if defined(__WIN32__) || defined(_WIN32)
        DWORD msto = (DWORD)write_timeout_millseconds;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&msto, sizeof(msto) );
#else
        struct timeval tv;
        tv.tv_sec = write_timeout_millseconds/1000;
        tv.tv_usec = (write_timeout_millseconds%1000)*1000;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof tv);
#endif
#endif
    }
    return result;
}
bool Network_Util::set_write_timeout(SOCKET sockfd,uint32_t write_timeout_millseconds)
{
    if (write_timeout_millseconds > 0) {
#ifdef SO_SNDTIMEO
#if defined(__WIN32__) || defined(_WIN32)
        DWORD msto = (DWORD)write_timeout_millseconds;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&msto, sizeof(msto) );
#else
        struct timeval tv;
        tv.tv_sec = write_timeout_millseconds/1000;
        tv.tv_usec = (write_timeout_millseconds%1000)*1000;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof tv);
#endif
#endif
    }
    return true;
}
bool Network_Util::make_noblocking(SOCKET sockfd)
{
#if defined(__linux) || defined(__linux__)
    int flags = fcntl(sockfd, F_GETFL, 0);
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#else
    unsigned long on = 1;
    return ioctlsocket(sockfd, FIONBIO, &on);
#endif
}
bool Network_Util::set_tcp_keepalive(SOCKET sockfd,bool flag,uint32_t try_seconds,uint32_t max_tries,uint32_t try_interval)
{
    if(flag){
#if defined(__WIN32__) || defined(_WIN32)
        // How do we do this in Windows?  For now, just make this a no-op in Windows:
#else
        int const keepalive_enabled = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive_enabled, sizeof keepalive_enabled) < 0) {
            return false;
        }

#ifdef TCP_KEEPIDLE
        uint32_t  keepalive_time = try_seconds==0?180:try_seconds;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPIDLE, (void*)&keepalive_time, sizeof keepalive_time) < 0) {
            return false;
        }
#endif

#ifdef TCP_KEEPCNT
        uint32_t  keepalive_count = max_tries==0?5:max_tries;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPCNT, (void*)&keepalive_count, sizeof keepalive_count) < 0) {
            return false;
        }
#endif

#ifdef TCP_KEEPINTVL
        uint32_t  keepalive_interval = try_interval==0?60:try_interval;
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_KEEPINTVL, (void*)&keepalive_interval, sizeof keepalive_interval) < 0) {
            return false;
        }
#endif
#endif
        return true;
    }else {
#if defined(__WIN32__) || defined(_WIN32)
        return true;
#else
        int const keepalive_enabled = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive_enabled, sizeof keepalive_enabled) < 0) {
            return false;
        }
        return true;
#endif
    }
}
void Network_Util::set_ignore_sigpipe(SOCKET sockfd)
{
#if defined(__linux) || defined(__linux__)
#ifdef SO_NOSIGPIPE
    int set_option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &set_option, sizeof set_option);
#else
    UNUSED(sockfd);
    signal(SIGPIPE, SIG_IGN);
#endif
#endif
}
void Network_Util::set_recv_buf_size(SOCKET sockfd, socklen_t buf_size)
{
    setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof (buf_size));
}
socklen_t Network_Util::get_recv_buf_size(SOCKET sockfd)
{
    socklen_t curSize;
    socklen_t sizeSize = sizeof curSize;
    if(getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF,(char*)&curSize, &sizeSize) < 0)return 0;
    return curSize;
}
void Network_Util::set_send_buf_size(SOCKET sockfd, socklen_t buf_size)
{
    setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof (buf_size));
}
socklen_t Network_Util::get_send_buf_size(SOCKET sockfd)
{
    socklen_t curSize;
    socklen_t sizeSize = sizeof curSize;
    if(getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF,(char*)&curSize, &sizeSize) < 0)return 0;
    return curSize;
}
void Network_Util::set_reuse_addr(SOCKET sockfd)
{
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof on);
}
void Network_Util::set_reuse_port(SOCKET sockfd)
{
#ifdef SO_REUSEPORT
    int on = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&on, sizeof(on));
#else
    (void)sockfd;
#endif
}
bool Network_Util::bind(SOCKET sockfd,uint16_t port,uint32_t index){
    struct sockaddr_in addr;
    bzero(&addr,sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = get_netinterface(index);
    addr.sin_port = htons(port);
    if(::bind(sockfd, (struct sockaddr*)&addr, sizeof addr) == SOCKET_ERROR)
    {
        return false;
    }
    return true;
}
uint8_t Network_Util::get_ttl(SOCKET sockfd)
{
    uint8_t curTTL=0;
    socklen_t lenTTL=sizeof(curTTL);
    if(getsockopt(sockfd,IPPROTO_IP,IP_TTL,(char   *)&curTTL,&lenTTL)==0)return curTTL;
    return 0;
}
int Network_Util::get_socket_error(SOCKET fd)
{
    int err;
    socklen_t len = sizeof err;
    getsockopt(fd, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
    return err;
}
bool Network_Util::set_ttl(SOCKET sockfd, uint8_t ttl)
{
    return setsockopt(sockfd, IPPROTO_IP, IP_TTL, (char *)&ttl, sizeof (socklen_t))>=0;
}
bool Network_Util::set_multicast_if(SOCKET sockfd,uint32_t index)
{
    struct sockaddr_in addr;
    addr.sin_addr.s_addr=get_netinterface(index);
    return setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_IF,(char *)&addr,sizeof (addr))>=0;
}
bool Network_Util::set_multicast_loop(SOCKET sockfd,bool flag)
{
    return setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_LOOP, (char*)&flag,sizeof (flag))>=0;
}
bool Network_Util::set_multicast_ttl(SOCKET sockfd,uint8_t ttl)
{
    return setsockopt(sockfd,IPPROTO_IP,IP_MULTICAST_TTL, (char*)&ttl,sizeof(ttl))>=0;
}
bool Network_Util::join_asm_multicast(SOCKET sockfd,const string &multicast_ip,uint32_t index)
{
    bool ret=false;
    do{
        struct ip_mreq mreq;
        bzero(&mreq,sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        mreq.imr_interface.s_addr = get_netinterface(index);
        if(!check_is_multicast(mreq.imr_multiaddr.s_addr))break;
        ret=setsockopt(sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP, (char*)&mreq,sizeof(mreq))>=0;
#ifdef IP_MULTICAST_ALL
        int multicastAll = 0;
        setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_ALL, (void*)&multicastAll, sizeof multicastAll);
#endif
    }while(0);
    return ret;
}

bool Network_Util::leave_asm_multicast(SOCKET sockfd,const string &multicast_ip,uint32_t index)
{
    bool ret=false;
    do{
        struct ip_mreq mreq;
        bzero(&mreq,sizeof(mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        if(!check_is_multicast(mreq.imr_multiaddr.s_addr))break;
        mreq.imr_interface.s_addr = get_netinterface(index);
        ret=setsockopt(sockfd,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char *)&mreq,sizeof(mreq))>=0;
    }while(0);
    return ret;
}
bool Network_Util::join_ssm_multicast(SOCKET sockfd,const string &multicast_ip,const vector<string>&included_ip,uint32_t index)
{
    bool ret=false;
    do{
        struct ip_mreq_source imr;
        imr.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        if(!check_is_multicast(imr.imr_multiaddr.s_addr))break;
        imr.imr_interface.s_addr = get_netinterface(index);
        for(auto i :included_ip){
            imr.imr_sourceaddr.s_addr = inet_addr(i.c_str());;
            ret=setsockopt(sockfd, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP,\
                           (const char*)&imr, sizeof (struct ip_mreq_source))>=0;
            if(!ret)break;
        }
    }while(0);
    return ret;
}
bool Network_Util::leave_ssm_multicast(SOCKET sockfd,const string &multicast_ip,const vector<string>&included_ip,uint32_t index)
{
    bool ret=false;
    do{
        struct ip_mreq_source imr;
        imr.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        if(!check_is_multicast(imr.imr_multiaddr.s_addr))break;
        imr.imr_interface.s_addr = get_netinterface(index);
        for(auto i :included_ip){
            imr.imr_sourceaddr.s_addr = inet_addr(i.c_str());;
            ret=setsockopt(sockfd, IPPROTO_IP, IP_DROP_SOURCE_MEMBERSHIP,\
                           (const char*)&imr, sizeof (struct ip_mreq_source))>=0;
            if(!ret)break;
        }
    }while(0);
    return ret;
}
bool Network_Util::join_sfm_multicast(SOCKET sockfd,const string &multicast_ip,const string &excluded_ip,uint32_t index)
{
    bool ret=false;
    do{
        struct ip_mreq_source imr;
        imr.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        if(!check_is_multicast(imr.imr_multiaddr.s_addr))break;
        imr.imr_interface.s_addr = get_netinterface(index);
        imr.imr_sourceaddr.s_addr = inet_addr(excluded_ip.c_str());;
        ret=setsockopt(sockfd, IPPROTO_IP, IP_BLOCK_SOURCE,\
                       (const char*)&imr, sizeof (struct ip_mreq_source))>=0;
    }while(0);
    return ret;
}
bool Network_Util::leave_sfm_multicast(SOCKET sockfd,const string &multicast_ip,const string &excluded_ip,uint32_t index)
{
    bool ret=false;
    do{
        struct ip_mreq_source imr;
        imr.imr_multiaddr.s_addr = inet_addr(multicast_ip.c_str());
        if(!check_is_multicast(imr.imr_multiaddr.s_addr))break;
        imr.imr_interface.s_addr = get_netinterface(index);
        imr.imr_sourceaddr.s_addr = inet_addr(excluded_ip.c_str());;
        ret=setsockopt(sockfd, IPPROTO_IP, IP_UNBLOCK_SOURCE,\
                       (const char*)&imr, sizeof (struct ip_mreq_source))>=0;
    }while(0);
    return ret;
}
const vector<Network_Util::net_interface_info> & Network_Util::get_net_interface_info(bool update)
{
    lock_guard<mutex> locker(m_mutex);
    if(update){
        do{
#if defined(__linux) || defined(__linux__)
            //获取所有网卡默认网关信息
            FILE *fp=popen("ip route show","r");
            char gw_buf[1024]={0};
            map<string,string>gw_ip_map;
            while(fgets(gw_buf, sizeof(gw_buf), fp) != nullptr)
            {

                if(strstr(gw_buf,"default")!=nullptr)
                {
                    char ip_str[32]={0};
                    char dev_name[256]={0};
                    if(sscanf(gw_buf,"default via %s dev %s",ip_str,dev_name)==2)
                    {
                        gw_ip_map.emplace(dev_name,ip_str);
                    }
                }
                else {
                    break;
                }
                memset(gw_buf,0,1024);
            }
            pclose(fp);
            SOCKET sockfd = INVALID_SOCKET;
            char buf[512] = { 0 };
            struct ifconf ifconf;
            struct ifreq  *ifreq;
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd == INVALID_SOCKET)
            {
                close(sockfd);
                break;
            }

            ifconf.ifc_len = 512;
            ifconf.ifc_buf = buf;
            if (ioctl(sockfd, SIOCGIFCONF, &ifconf) < 0)
            {
                close(sockfd);
                break;
            }
            ifreq = (struct ifreq*)ifconf.ifc_buf;
            m_net_interface_info_cache.clear();
            for (int i = (ifconf.ifc_len / sizeof(struct ifreq)); i>0; i--)
            {
                if (ifreq->ifr_flags == AF_INET)
                {
                    if (strcmp(ifreq->ifr_name, "lo") != 0)
                    {
                        struct ifreq ifr2;
                        strcpy(ifr2.ifr_name,ifreq->ifr_name);
                        uint8_t mac[6]={0};
                        char mac1[128]={0};
                        if((ioctl(sockfd,SIOCGIFHWADDR,&ifr2) )== 0)
                        {//获取mac ,前6个字节
                            memcpy(mac,ifr2.ifr_hwaddr.sa_data,6);
                            sprintf(mac1,"%02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
                        }
                        string netmask;
                        if((ioctl(sockfd,SIOCGIFNETMASK,&ifr2) )== 0)
                        {//获取netmask ,前6个字节
                            netmask=inet_ntoa(((struct sockaddr_in*)&(ifr2.ifr_netmask))->sin_addr);
                        }
                        string gw_ip="";
                        auto iter=gw_ip_map.find(ifreq->ifr_name);
                        if(iter!=gw_ip_map.end()){
                            gw_ip=iter->second;
                            m_net_interface_info_cache.push_back(net_interface_info(ifreq->ifr_name,inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr),mac1,netmask,gw_ip,true));
                        }
                        else {
                            m_net_interface_info_cache.push_back(net_interface_info(ifreq->ifr_name,inet_ntoa(((struct sockaddr_in*)&(ifreq->ifr_addr))->sin_addr),mac1,netmask,gw_ip));
                        }
                    }
                    ifreq++;
                }
            }
            close(sockfd);
#elif defined(WIN32) || defined(_WIN32)
            PIP_ADAPTER_INFO pIpAdapterInfo = new IP_ADAPTER_INFO();
            unsigned long size = sizeof(IP_ADAPTER_INFO);

            int ret = GetAdaptersInfo(pIpAdapterInfo, &size);
            if (ret == ERROR_BUFFER_OVERFLOW)
            {
                delete pIpAdapterInfo;
                pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[size];
                ret = GetAdaptersInfo(pIpAdapterInfo, &size);
            }

            if (ret != ERROR_SUCCESS)
            {
                delete pIpAdapterInfo;
                break;
            }
            auto p_iter = pIpAdapterInfo;
            while (p_iter)
            {
                IP_ADDR_STRING *pIpAddrString = &(p_iter->IpAddressList);
                while(pIpAddrString)
                {
                    if (strcmp(pIpAddrString->IpAddress.String, "127.0.0.1")!=0
                            && strcmp(pIpAddrString->IpAddress.String, "0.0.0.0")!=0)
                    {
                        char buffer[20];
                        sprintf_s(buffer, "%.2x-%.2x-%.2x-%.2x-%.2x-%.2x", p_iter->Address[0],
                                p_iter->Address[1], p_iter->Address[2], p_iter->Address[3],
                                p_iter->Address[4], p_iter->Address[5]);
                        if(pIpAdapterInfo->Type== MIB_IF_TYPE_ETHERNET)m_net_interface_info_cache.push_back(net_interface_info("MIB_IF_TYPE_ETHERNET", pIpAddrString->IpAddress.String,buffer));
                        else if(pIpAdapterInfo->Type == IF_TYPE_IEEE80211)m_net_interface_info_cache.push_back(net_interface_info("IF_TYPE_IEEE80211", pIpAddrString->IpAddress.String,buffer));
                        else m_net_interface_info_cache.push_back(net_interface_info("UNKNOWN", pIpAddrString->IpAddress.String));
                        break;
                    }
                    pIpAddrString = pIpAddrString->Next;
                };
                p_iter = p_iter->Next;
            }

            delete []pIpAdapterInfo;
#endif
        }while (0);

    }
    return m_net_interface_info_cache;
}
Network_Util::net_interface_info Network_Util::get_default_net_interface_info()
{
    auto info=NETWORK.get_net_interface_info();
    for(auto i:info){
        if(i.is_default){
            return i;
        }
    }
    return Network_Util::net_interface_info();
}
bool Network_Util::modify_net_interface_info(const net_interface_info&net_info)
{
    SOCKET sockfd = INVALID_SOCKET;
    do{
        if(net_info.dev_name.empty())break;
        auto local_net_info=get_net_interface_info(true);
        map<string,Network_Util::net_interface_info>net_info_map;
        Network_Util::net_interface_info old_config;
        bool find=false;
        for(auto i:local_net_info){
            if(i.dev_name==net_info.dev_name){
                old_config=i;
                find=true;
                break;
            }
        }
        if(!find){
            fprintf(stderr,"dev %s  not found!\r\n",net_info.dev_name.c_str());
            break;
        }
        //校验ip信息
        if(!net_info.gateway_ip.empty()){
            auto u32ip = inet_addr(net_info.ip.c_str());
            auto u32netmask = inet_addr(net_info.netmask.c_str());
            //check mask
            bool invalid=false;
            auto den=u32netmask;
            while(den!=0){
                auto num=den%2;
                if(num!=1){
                    invalid=true;
                    break;
                }
                den=den/2;
            }
            if(invalid){
                fprintf(stderr,"netmask %s is invalid!\r\n",net_info.netmask.c_str());
                break;
            }
            auto u32gateway = inet_addr(net_info.gateway_ip.c_str());
            if((u32ip & u32netmask) != (u32netmask & u32gateway)){
                fprintf(stderr,"ip:%s mask:%s gateway:%s not matched!\r\n",net_info.ip.c_str(),net_info.netmask.c_str(),net_info.gateway_ip.c_str());
                break;
            }
        }
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd == INVALID_SOCKET)
        {
            close(sockfd);
            break;
        }
        struct ifreq ifr;
        struct sockaddr_in sin;
        //修改ip
        if(net_info.ip!=old_config.ip)
        {
            strncpy(ifr.ifr_name,   net_info.dev_name.c_str(),   IFNAMSIZ);
            ifr.ifr_name[IFNAMSIZ   -   1]   =   0;
            memset(&sin,   0,   sizeof(sin));
            sin.sin_family   =   AF_INET;
            sin.sin_addr.s_addr   =   inet_addr(net_info.ip.c_str());
            memcpy(&ifr.ifr_addr,   &sin,   sizeof(sin));

            if(ioctl(sockfd, SIOCSIFADDR, &ifr) < 0)
            {
                perror( "Not setup interface! ");
            }
        }
        //修改netmask
        if(old_config.netmask!=net_info.netmask)
        {
            bzero(&ifr,   sizeof(struct   ifreq));
            strncpy(ifr.ifr_name,   net_info.dev_name.c_str(),   IFNAMSIZ);
            ifr.ifr_name[IFNAMSIZ   -   1]   =   0;
            memset(&sin,   0,   sizeof(sin));
            sin.sin_family   =   AF_INET;
            sin.sin_addr.s_addr   =   inet_addr(net_info.netmask.c_str());
            memcpy(&ifr.ifr_addr,   &sin,   sizeof(sin));

            if(ioctl(sockfd, SIOCSIFNETMASK, &ifr ) < 0)
            {
                perror("net mask ioctl error!");
            }
        }
        //修改mac
        if(old_config.mac!=net_info.mac&&!net_info.mac.empty())
        {
            bzero(&ifr,   sizeof(struct   ifreq));
            if(sscanf(net_info.mac.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
                      &ifr.ifr_hwaddr.sa_data[0],
                      &ifr.ifr_hwaddr.sa_data[1],
                      &ifr.ifr_hwaddr.sa_data[2],
                      &ifr.ifr_hwaddr.sa_data[3],
                      &ifr.ifr_hwaddr.sa_data[4],
                      &ifr.ifr_hwaddr.sa_data[5])!=6)break;
            strncpy(ifr.ifr_name,   net_info.dev_name.c_str(),   IFNAMSIZ);
            ifr.ifr_name[IFNAMSIZ   -   1]   =   0;
            memset(&sin,   0,   sizeof(sin));
            ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
            if(0 > ioctl(sockfd, SIOCSIFHWADDR, &ifr))
            {
                perror("mac ioctl error!");
            }
        }
        //更改出口路由
        if(!net_info.gateway_ip.empty())
        {
            struct rtentry rm;
            bzero(&rm,   sizeof(struct rtentry));
            rm.rt_dst.sa_family = AF_INET;
            rm.rt_gateway.sa_family = AF_INET;
            rm.rt_genmask.sa_family = AF_INET;
            memset(&sin,   0,   sizeof(sin));
            sin.sin_family   =   AF_INET;
            shared_ptr<char>name(new char[IFNAMSIZ],default_delete<char[]>());
            strncpy(name.get(),net_info.dev_name.c_str(),IFNAMSIZ);
            rm.rt_dev = name.get();
            rm.rt_flags = RTF_UP | RTF_GATEWAY ;
            //delete old
            if(!old_config.gateway_ip.empty()){
                sin.sin_addr.s_addr   =   inet_addr(old_config.gateway_ip.c_str());
                memcpy(&rm.rt_gateway, &sin,   sizeof(sin));
                if(ioctl(sockfd, SIOCDELRT, &rm ) < 0)
                {
                    perror("gateway delete ioctl error!");
                }
            }
            //add new
            if(!net_info.gateway_ip.empty())
            {
                sin.sin_addr.s_addr   =   inet_addr(net_info.gateway_ip.c_str());
                memcpy(&rm.rt_gateway, &sin,   sizeof(sin));
                if(ioctl(sockfd, SIOCADDRT, &rm ) < 0)
                {
                    perror("gateway add  ioctl   error!");
                }
            }
        }
        //重新启动网卡
        {
            ifr.ifr_flags   |=   IFF_UP   |   IFF_RUNNING;
            if(ioctl(sockfd,   SIOCSIFFLAGS,   &ifr)   <   0)
            {//如果未更改任何配置会进入当前判断内
                perror( "SIOCSIFFLAGS ");
            }
        }
        close(sockfd);
        //更新网卡配置信息
        get_net_interface_info(true);
        return true;
    }while(0);
    if(sockfd!=INVALID_SOCKET)close(sockfd);
    return false;
}
uint16_t Network_Util::get_local_port(SOCKET sockfd)
{
    struct sockaddr_in addr ;
    bzero(&addr,sizeof (addr));
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if (getsockname(sockfd, (struct sockaddr *)&addr, &addrlen) == 0)
    {
        return ntohs(addr.sin_port);
    }
    return 0;
}
string Network_Util::get_local_ip(SOCKET sockfd)
{
    struct sockaddr_in addr ;
    bzero(&addr,sizeof (addr));
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if (getsockname(sockfd, (struct sockaddr *)&addr, &addrlen) == 0)
    {
        return inet_ntoa(addr.sin_addr);
    }
    return "0.0.0.0";
}
uint16_t Network_Util::get_peer_port(SOCKET sockfd)
{
    struct sockaddr_in addr ;
    bzero(&addr,sizeof (addr));
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr *)&addr, &addrlen) == 0)
    {
        return ntohs(addr.sin_port);
    }
    return 0;
}
string Network_Util::get_peer_ip(SOCKET sockfd)
{
    struct sockaddr_in addr ;
    bzero(&addr,sizeof (addr));
    socklen_t addrlen = sizeof(struct sockaddr_in);
    if (getpeername(sockfd, (struct sockaddr *)&addr, &addrlen) == 0)
    {
        return inet_ntoa(addr.sin_addr);
    }
    return "0.0.0.0";
}
bool Network_Util::get_peer_addr(SOCKET sockfd,struct sockaddr_in *addr)
{
    socklen_t addrlen = sizeof(struct sockaddr_in);
    return getpeername(sockfd, (struct sockaddr *)addr, &addrlen) == 0;
}
string Network_Util::parase_domain(const string &domain_info)
{
    struct sockaddr_in addr;
    if(inet_pton(AF_INET,domain_info.c_str(),&addr)!=0)
    {
        return domain_info;
    }
    string ret=string();
    struct addrinfo hints;
    struct addrinfo *result;
    int  s;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = nullptr;
    hints.ai_addr = nullptr;
    hints.ai_next = nullptr;
    s = getaddrinfo(domain_info.c_str(), nullptr, &hints, &result);
    if (s != 0) {
        return ret;
    }
    ret=inet_ntoa((reinterpret_cast<struct sockaddr_in *>(result->ai_addr))->sin_addr);
    freeaddrinfo(result);
    return ret;
}
bool Network_Util::check_is_multicast(uint32_t net_ip)
{
    //转成本机字节序
    uint32_t addressInNetworkOrder = htonl(net_ip);
    return addressInNetworkOrder >  0xE00000FF &&
            addressInNetworkOrder <= 0xEFFFFFFF;
}
Network_Util::Network_Util(){
#if defined(WIN32) || defined(_WIN32)
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData))
    {
        WSACleanup();
    }
#endif
    memset(m_netinterface,0,sizeof (uint32_t)*MAX_INTERFACES);
}
Network_Util::~Network_Util()
{
#if defined(WIN32) || defined(_WIN32)
    WSACleanup();
#endif
}
uint32_t Network_Util::get_netinterface(uint32_t index)const{
    lock_guard<mutex> locker(m_mutex);
    if(index>=MAX_INTERFACES)return DEFAULT_INTERFACES;
    else return m_netinterface[index];
}
bool Network_Util::ip_check_with_mask(string ip1,string ip2,string netmask)
{
    auto val1=inet_addr(ip1.c_str());
    auto val2=inet_addr(ip2.c_str());
    auto val3=inet_addr(netmask.c_str());
    return  (val1&val3)==(val2&val3);
}
}
