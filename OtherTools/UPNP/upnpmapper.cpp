#include "upnpmapper.h"
#include <iostream>
using  namespace micagent;
using  std::cout;
using  std::endl;

Upnp_Connection::Upnp_Connection(UpnpMapper * server,EventLoop *loop, int sockfd,UPNP_COMMAND mode)
    :m_channel(new Channel(sockfd)),m_upnp_mapper(server),m_event_loop(loop),m_mode(mode)
{
    Network_Util::Instance().make_noblocking(sockfd);
    m_channel->setReadCallback([this](Channel *chn){
        return this->onRead();
    });
    m_channel->setCloseCallback([this](Channel *chn){
        m_event_loop->removeChannel(chn->fd());
        m_upnp_mapper->removeConnection(chn->fd());
#if UPNP_LOG_ON
        cout<<"close Upnp_Connection : "<<chn->fd()<<endl;
#endif
        return true;
    });
    m_channel->enableReading();
}
void Upnp_Connection::start_work()
{
    m_event_loop->updateChannel(m_channel);
}
void Upnp_Connection::stop_work(){
    m_event_loop->removeChannel(m_channel);
}
Upnp_Connection::~Upnp_Connection()
{

}
#define SOAPPREFIX "s"
#define SERVICEPREFIX "u"
string Upnp_Connection::build_xml_packet(string action,std::vector<std::pair<string ,string>>args)
{

    string packet="<?xml version=\"1.0\"?>\r\n";
    packet+="<" SOAPPREFIX ":Envelope\r\n";
    packet+="    xmlns:" SOAPPREFIX "=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n    ";
    packet+=SOAPPREFIX ":encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n  ";
    packet+="<" SOAPPREFIX ":Body>\r\n    ";
    packet+="<" SERVICEPREFIX ":"+action+" xmlns:" SERVICEPREFIX "=\""+m_upnp_mapper->m_control_string+"\">\r\n";
    if(!args.empty())
        for(auto i : args)
        {
            packet+="      <"+i.first+">"+i.second+"</"+i.first+">\r\n";
        }
    packet+="    </" SERVICEPREFIX ":"+action+">\r\n";
    packet+="  </" SOAPPREFIX ":Body>\r\n</" SOAPPREFIX ":Envelope>\r\n";
    packet+="\r\n";
    string request="POST ";
    request=request+m_upnp_mapper->m_control_url+" HTTP/1.1\r\n";
    request=request+"Host: "+m_upnp_mapper->m_lgd_ip+":"+std::to_string(m_upnp_mapper->m_lgd_port)+"\r\n";
    request=request+"SOAPAction: \""+m_upnp_mapper->m_control_string+"#"+action+"\"\r\n";
    request=request+"User-Agent: micagent_upnp_tool\r\n";
    request=request+"Content-Type: text/xml; charset=\"utf-8\"\r\n";
    request=request+"Connection: close\r\n";
    request=request+"Content-Length: "+std::to_string(packet.length())+"\r\n";
    request+="\r\n";
    request+=packet;
    return request;
}
bool Upnp_Connection::onRead()
{
    shared_ptr<char>buf(new char[4096],std::default_delete<char[]>());
    bool ret=true;
    while(1){
        int size = recv(m_channel->fd(),buf.get(),4096,0);
        if(size==0){
            ret=false;
            break;
        }
        else if(size<0) {
            break;
        }
        else {
            m_buf+=string(buf.get(),size);
        }
        memset(buf.get(),0,4096);
    }
#if UPNP_LOG_ON
    cout<<m_buf<<endl;
#endif
    if(check_packet())HandleData();
    return ret;
}
bool Upnp_Connection::check_packet()
{
#if UPNP_LOG_ON
    cout<<"############"<<endl;
    cout<<m_buf<<endl;
    cout<<"****************"<<endl;
#endif
    string key_body="\r\n\r\n";
    string key_length="Content-Length";
    bool ret=false;
    do{
        auto pos=m_buf.find(key_body);
        if(pos==string::npos)break;
        auto pos2=m_buf.find(key_length);
        if(pos2==string::npos)break;
        char len[16]={0};

        if(sscanf(m_buf.c_str()+pos2,"%*[^:]: %[^\r\n]",len)!=1)break;
        if(m_buf.length()-std::stoi(len)-pos-key_body.length()!=0)break;
        ret=true;
    }while(0);
    return ret;
}
void Upnp_Connection::send_get_wanip()
{
    std::vector<std::pair<string ,string>>args;
    std::string request=this->build_xml_packet("GetExternalIPAddress",args);
    send(m_channel->fd(),request.c_str(),request.length(),0);
}
void Upnp_Connection::send_add_port_mapper(SOCKET_TYPE type,string internal_ip,int internal_port,int external_port,string description,int alive_time)
{
    std::vector<std::pair<string ,string>>args;
    args.emplace_back(std::pair<string,string>("NewRemoteHost",string()));
    args.emplace_back(std::pair<string,string>("NewExternalPort",std::to_string(external_port)));
    args.emplace_back(std::pair<string,string>("NewProtocol",type==SOCKET_TYPE::TCP?"TCP":"UDP"));
    args.emplace_back(std::pair<string,string>("NewInternalPort",std::to_string(internal_port)));
    args.emplace_back(std::pair<string,string>("NewInternalClient",internal_ip));
    args.emplace_back(std::pair<string,string>("NewEnabled","1"));
    args.emplace_back(std::pair<string,string>("NewPortMappingDescription",description));
    args.emplace_back(std::pair<string,string>("NewLeaseDuration",to_string(alive_time)));
    std::string request=this->build_xml_packet("AddPortMapping",args);
    send(m_channel->fd(),request.c_str(),request.length(),0);
}
void Upnp_Connection::send_get_specific_port_mapping_entry(SOCKET_TYPE type,int external_port)
{
    std::vector<std::pair<string ,string>>args;
    args.emplace_back(std::pair<string,string>("NewRemoteHost",string()));
    args.emplace_back(std::pair<string,string>("NewExternalPort",std::to_string(external_port)));
    args.emplace_back(std::pair<string,string>("NewProtocol",type==SOCKET_TYPE::TCP?"TCP":"UDP"));
    args.emplace_back(std::pair<string,string>("NewInternalPort",string()));
    args.emplace_back(std::pair<string,string>("NewInternalClient",string()));
    args.emplace_back(std::pair<string,string>("NewEnabled",string()));
    args.emplace_back(std::pair<string,string>("NewPortMappingDescription",string()));
    args.emplace_back(std::pair<string,string>("NewLeaseDuration",string()));
    std::string request=this->build_xml_packet("GetSpecificPortMappingEntry",args);
    send(m_channel->fd(),request.c_str(),request.length(),0);
}
void Upnp_Connection::send_delete_port_mapper(SOCKET_TYPE type,int external_port)
{
    std::vector<std::pair<string ,string>>args;
    args.emplace_back(std::pair<string,string>("NewRemoteHost",string()));
    args.emplace_back(std::pair<string,string>("NewExternalPort",std::to_string(external_port)));
    args.emplace_back(std::pair<string,string>("NewProtocol",type==SOCKET_TYPE::TCP?"TCP":"UDP"));
    std::string request=this->build_xml_packet("DeletePortMapping",args);
    send(m_channel->fd(),request.c_str(),request.length(),0);
}
void Upnp_Connection::send_get_control_url()
{
    string request="GET ";
    request=request+m_upnp_mapper->m_location_src+" HTTP/1.1\r\n";
    request=request+"Host: "+m_upnp_mapper->m_lgd_ip+":"+std::to_string(m_upnp_mapper->m_lgd_port)+"\r\n";
    request=request+"User-Agent: micagent_upnp_tool\r\n";
    request=request+"Accept: application/xml\r\n";
    request=request+"Connection: keep-alive\r\n";
    request+="\r\n";
    send(m_channel->fd(),request.c_str(),request.length(),0);
}
void Upnp_Connection::handle_get_wanip()
{
    if(m_buf.empty())return;
    char status[5]={0};
    char info[128]={0};
    do{
        if(sscanf(m_buf.c_str(),"%*s %s %[^\r\n]",status,info)!=2)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
        if(strcmp(status,"200")!=0)
        {
#if UPNP_LOG_ON
            cout<<"can't found externalIPAddress! errorcode :"<<status<<" info:"<<info<<endl;
#endif
            break;
        }
        string key="NewExternalIPAddress";
        auto pos=m_buf.find(key);
        if(pos==string::npos)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
        char externalIP[32]={0};
        if(sscanf(m_buf.c_str()+pos,"%*[^>]>%[^<]",externalIP)!=1)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
#if UPNP_LOG_ON
        cout<<"NewExternalIPAddress : "<<externalIP<<endl;
#endif
        this->m_upnp_mapper->m_wan_ip=externalIP;
        if(this->m_callback)m_callback(true);
    }while(0);
}
void Upnp_Connection::handle_add_port_mapper()
{
    if(m_buf.empty())return;
    char status[5]={0};
    char info[128]={0};
    bool b_success=false;
    do{
        if(sscanf(m_buf.c_str(),"%*s %s %[^\r\n]",status,info)!=2)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
        if(strcmp(status,"200")!=0)
        {
#if UPNP_LOG_ON
            cout<<"can't addportmapping! errorcode :"<<status<<" info:"<<info<<endl;
#endif
            break;
        }
#if UPNP_LOG_ON
        cout<<"add port mapping success!"<<endl;
#endif
        b_success=true;
    }while(0);
    if(m_callback)m_callback(b_success);
}
void Upnp_Connection::handle_get_specific_port_mapping_entry()
{
    if(m_buf.empty())return;
    char status[5]={0};
    char info[128]={0};
    bool b_success=false;
    do{
        if(sscanf(m_buf.c_str(),"%*s %s %[^\r\n]",status,info)!=2)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
        if(strcmp(status,"200")!=0)
        {
#if UPNP_LOG_ON
            cout<<"can't get port mapping entry! errorcode :"<<status<<" info:"<<info<<endl;
#endif
            break;
        }
        string key="NewInternalClient";
        auto pos=m_buf.find(key);
        if(pos==string::npos)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
        char internalIP[32]={0};
        if(sscanf(m_buf.c_str()+pos,"%*[^>]>%[^<]",internalIP)!=1)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
        auto interface_info=Network_Util::Instance().get_net_interface_info(false);
        bool find=false;
        for(auto i:interface_info){
            if(i.ip==internalIP){
                find=true;
                break;
            }
        }
        if(!find)break;
#if UPNP_LOG_ON
        cout<<"get port mapping entry success!"<<endl;
#endif
        b_success=true;
    }while(0);
    if(m_callback)m_callback(b_success);
}
void Upnp_Connection:: handle_delete_port_mapper()
{
    if(m_buf.empty())return;
    char status[5]={0};
    char info[128]={0};
    bool b_success=false;
    do{
        if(sscanf(m_buf.c_str(),"%*s %s %[^\r\n]",status,info)!=2)
        {
#if UPNP_LOG_ON
            cout<<"false http response! : "<<m_buf<<endl;
#endif
            break;
        }
        char errorinfo[128]="OK";
        if(strcmp(status,"200")!=0)
        {
#if UPNP_LOG_ON
            cout<<"can't deleteportmapping! errorcode :"<<status<<" info:"<<info<<endl;
#endif
            string key="errorDescription";
            auto pos=m_buf.find(key);
            if(pos==string::npos)
            {
#if UPNP_LOG_ON
                cout<<"false http response! : "<<m_buf<<endl;
#endif
                break;
            }
            if(sscanf(m_buf.c_str()+pos,"%*[^>]>%[^<]",errorinfo)!=1)
            {
#if UPNP_LOG_ON
                cout<<"false http response! : "<<m_buf<<endl;
#endif
                break;
            }
        }
#if UPNP_LOG_ON
        cout<<"delete port mapping success!"<<"(error info :"<<errorinfo<<")"<<endl;
#endif
        b_success=true;
    }while(0);
    if(m_callback)m_callback(b_success);
}
void Upnp_Connection::handle_get_control_url()
{
    string key_string1="<service>";
    string key_string2="</service>";
    string key_string3="urn:schemas-upnp-org:service:WANPPPConnection:1";
    string key_string4="urn:schemas-upnp-org:service:WANIPConnection:1";
    string key_string5="<controlURL>";
    int start_pos=0;
    while(start_pos<m_buf.length())
    {
        //find service
        int pos1=m_buf.find(key_string1,start_pos);
        if(pos1==string::npos)break;
        //find service's end;
        int pos2=m_buf.find(key_string2,pos1);
        if(pos2==string::npos)break;
        string tmp=m_buf.substr(pos1,pos2-pos1);
        if(tmp.find(key_string3,0)!=string::npos)
        {
            this->m_upnp_mapper->m_control_string=key_string3;
        }
        else if(tmp.find(key_string4,0)!=string::npos)
        {
            this->m_upnp_mapper->m_control_string=key_string4;
        }
        else {
            //not match
#if UPNP_LOG_ON
            cout<<endl<<tmp<<endl;
            cout<<"not match"<<endl;
#endif
            start_pos=pos2+key_string2.length();
            continue;
        }
        int pos3=tmp.find(key_string5);
        if(pos3==string::npos)break;
        char t_control_url[256]={0};
        if(sscanf(tmp.c_str()+pos3,"%*[^>]>%[^<]",t_control_url)!=1)
        {
#if UPNP_LOG_ON
            cout<<"false xml file : "<<tmp.c_str()+pos3<<endl;
#endif
            break;
        }
        this->m_upnp_mapper->m_control_url=t_control_url;
#if UPNP_LOG_ON
        cout<<"control url :"<<this->m_upnp_mapper->m_control_url<<endl;
#endif
        this->m_upnp_mapper->m_init_ok=true;
        this->reset_mode(UPNP_GETEXTERNALIPADDRESS);
        this->send_get_wanip();
        break;
    }
}
void Upnp_Connection::HandleData()
{
    switch(m_mode)
    {
    case UPNP_GETCONTROLURL:
        handle_get_control_url();
    case UPNP_GETEXTERNALIPADDRESS:
        handle_get_wanip();
        break;
    case UPNP_ADDPORTMAPPING:
        handle_add_port_mapper();
        break;
    case UPNP_GETSPECIFICPORTMAPPINGENTRY:
        handle_get_specific_port_mapping_entry();
        break;
    case UPNP_DELETEPORTMAPPING:
        handle_delete_port_mapper();
        break;
    default:
        break;
    }
}
void UpnpMapper::Init(EventLoop *event_loop,string lgd_ip)
{
    m_control_url=string();
    m_event_loop=event_loop;
    m_lgd_ip=lgd_ip;
    m_lgd_port=0;
    m_location_src=string();
    m_wan_ip=string();
    m_init_ok=false;
    int fd=socket(AF_INET,SOCK_DGRAM,0);
    m_udp_channel.reset(new Channel(fd));
    m_udp_channel->enableReading();
    m_udp_channel->setReadCallback([this](Channel *chn){
        struct sockaddr_in addr = {0};
        socklen_t addr_len=sizeof addr;
        char buf[4096]={0};
        int len=recvfrom(this->m_udp_channel->fd(),buf,4096,0,(struct sockaddr *)&addr,(socklen_t *)&addr_len);
        if(len>0)
        {
            if(m_lgd_ip==inet_ntoa(addr.sin_addr))
            {
                this->m_event_loop->removeChannel(this->m_udp_channel);
#if UPNP_LOG_ON
                cout<<"find upnp device!"<<endl;
                cout<<buf<<endl;
#endif
                string tmp=buf;
                int match_pos=tmp.find("LOCATION",0);
                if(match_pos==string::npos)match_pos=tmp.find("location",0);
                do{
                    if(match_pos==string::npos)
                    {
#if UPNP_LOG_ON
                        cout<<"can't find location url!"<<endl;
#endif
                        break;
                    }
                    char igd_port[32]={0};
                    char location_src[256]={0};
                    if(sscanf(tmp.c_str()+match_pos,"%*[^/]//%*[^:]:%[^/]%[^\r\n]",igd_port,location_src)!=2)break;
                    m_lgd_port=std::stoi(igd_port);
                    m_location_src=location_src;
                    std::thread t(std::bind(&UpnpMapper::get_control_url,this));
                    t.detach();
                    return true;
                }while(0);
#if UPNP_LOG_ON
                cout<<"false upnp device!"<<endl;
#endif
            }
            else {
#if UPNP_LOG_ON
                cout<<"recvfrom  ip:"<<inet_ntoa(addr.sin_addr)<<" port : "<<ntohs(addr.sin_port)<<endl<<buf<<endl;
#endif
            }
        }
        return true;
    });
    event_loop->updateChannel(m_udp_channel);
    send_discover_packet();
    event_loop->addTimer([this](){
        if(this->m_location_src!=string())
        {
            this->m_event_loop->removeChannel(m_udp_channel);
            m_udp_channel.reset();
            return false;
        }
        send_discover_packet();
#if UPNP_LOG_ON
        cout<<"no upnp suppot!"<<endl;
#endif
        return true;
    },MAX_WAIT_TIME*2);
}
void UpnpMapper:: Api_addportMapper(SOCKET_TYPE type,string internal_ip,int internal_port,int external_port,string description,UPNPCALLBACK callback,int alive_time)
{
    if(!m_init_ok||m_control_url==string())return;
    std::thread t(std::bind(&UpnpMapper::add_port_mapper,this,type,internal_ip,internal_port,external_port,description,callback,alive_time));
    t.detach();
}
void UpnpMapper:: Api_GetSpecificPortMappingEntry(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback)
{
    if(!m_init_ok||m_control_url==string())return;
    std::thread t(std::bind(&UpnpMapper::get_specific_port_mapping_entry,this,type,external_port,callback));
    t.detach();
}
void UpnpMapper::Api_deleteportMapper(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback)
{
    if(!m_init_ok||m_control_url==string())return;
    std::thread t(std::bind(&UpnpMapper::delete_port_mapper,this,type,external_port,callback));
    t.detach();
}
void UpnpMapper::Api_GetNewexternalIP(UPNPCALLBACK callback)
{
    if(!m_init_ok||m_control_url==string())return;
    std::thread t(std::bind(&UpnpMapper::get_wanip,this,callback));
    t.detach();
}
std::shared_ptr<Upnp_Connection> UpnpMapper::newConnection(SOCKET sockfd,UPNP_COMMAND mode)
{
    return std::make_shared<Upnp_Connection>(this, m_event_loop, sockfd,mode);
}
void UpnpMapper::addConnection(SOCKET sockfd, std::shared_ptr<Upnp_Connection> Conn)
{
    std::lock_guard<std::mutex> locker(m_map_mutex);
    m_connections.emplace(sockfd, Conn);
}
void UpnpMapper::removeConnection(SOCKET sockfd)
{
    std::lock_guard<std::mutex> locker(m_map_mutex);
    auto iter=m_connections.find(sockfd);
    if(iter!=m_connections.end()){
        iter->second->stop_work();
        m_connections.erase(iter);
    }
}
void UpnpMapper::addTimeoutEvent(SOCKET sockfd)
{
    //m_event_loop->addTimer([this,sockfd](){this->removeConnection(sockfd);return false;},MAX_WAIT_TIME);
}
void UpnpMapper::send_discover_packet()
{
    if(!m_udp_channel||m_init_ok)return;
#if UPNP_LOG_ON
    cout<<"send_discover_packet"<<endl;
#endif
    struct sockaddr_in addr = {0};
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=inet_addr("239.255.255.250");
    addr.sin_port=htons(1900);
    socklen_t  addr_len=sizeof addr ;
    string discover_packet;
    //send ppp discover
    discover_packet="M-SEARCH * HTTP/1.1\r\n";
    discover_packet+="HOST: 239.255.255.250:1900\r\n";
    discover_packet+="ST: urn:schemas-upnp-org:service:WANPPPConnection:1\r\n";
    discover_packet+="MAN: \"ssdp:discover\"\r\n";
    discover_packet+="MX: 9\r\n";
    discover_packet+="\r\n";
    sendto(m_udp_channel->fd(),discover_packet.c_str(),discover_packet.length(),0,(struct sockaddr *)&addr,addr_len);
    //send ip discover
    discover_packet="M-SEARCH * HTTP/1.1\r\n";
    discover_packet+="HOST: 239.255.255.250:1900\r\n";
    discover_packet+="ST: urn:schemas-upnp-org:service:WANIPConnection:1\r\n";
    discover_packet+="MAN: \"ssdp:discover\"\r\n";
    discover_packet+="MX: 9\r\n";
    discover_packet+="\r\n";
    sendto(m_udp_channel->fd(),discover_packet.c_str(),discover_packet.length(),0,(struct sockaddr *)&addr,addr_len);
    //send igd discover
    discover_packet="M-SEARCH * HTTP/1.1\r\n";
    discover_packet+="HOST: 239.255.255.250:1900\r\n";
    discover_packet+="ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n";
    discover_packet+="MAN: \"ssdp:discover\"\r\n";
    discover_packet+="MX: 9\r\n";
    discover_packet+="\r\n";
    sendto(m_udp_channel->fd(),discover_packet.c_str(),discover_packet.length(),0,(struct sockaddr *)&addr,addr_len);
    //send rootdevice discover
    discover_packet="M-SEARCH * HTTP/1.1\r\n";
    discover_packet+="HOST: 239.255.255.250:1900\r\n";
    discover_packet+="ST: upnp:rootdevice\r\n";
    discover_packet+="MAN: \"ssdp:discover\"\r\n";
    discover_packet+="MX: 9\r\n";
    discover_packet+="\r\n";
    sendto(m_udp_channel->fd(),discover_packet.c_str(),discover_packet.length(),0,(struct sockaddr *)&addr,addr_len);
}
void UpnpMapper::get_control_url()
{
    if(m_init_ok)return;
    SOCKET fd=Network_Util::Instance().build_socket(TCP);
    if(fd<=0)
    {
#if UPNP_LOG_ON
        cout<<"failed to create socket!"<<endl;
#endif
        return;
    }
    if(!Network_Util::Instance().connect(fd,m_lgd_ip,m_lgd_port,MAX_WAIT_TIME)){
#if UPNP_LOG_ON
        cout<<"failed to connect to the device!"<<endl;
#endif
        Network_Util::Instance().close_socket(fd);
        return;
    }
    auto conn=newConnection(fd,UPNP_GETCONTROLURL);
    if(conn)
    {
        conn->start_work();
        addConnection(fd,conn);
        conn->send_get_control_url();
    }
}
void UpnpMapper::get_wanip(UPNPCALLBACK callback)
{
    if(!m_init_ok||m_control_url==string())return;
    SOCKET fd=Network_Util::Instance().build_socket(TCP);
    if(fd<=0)
    {
#if UPNP_LOG_ON
        cout<<"failed to create socket!"<<endl;
#endif
        return;
    }
    if(!Network_Util::Instance().connect(fd,m_lgd_ip,m_lgd_port,MAX_WAIT_TIME)){
#if UPNP_LOG_ON
        cout<<"failed to connect to the device!"<<endl;
#endif
        Network_Util::Instance().close_socket(fd);
        return;
    }
    auto conn=newConnection(fd,UPNP_GETEXTERNALIPADDRESS);
    if(conn)
    {
        conn->start_work();
        addConnection(fd,conn);
        if(callback)conn->setUPNPCallback(callback);
        conn->send_get_wanip();
    }
}
void UpnpMapper::add_port_mapper(SOCKET_TYPE type,string internal_ip,int internal_port,int external_port,string description,UPNPCALLBACK callback,int alive_time)
{
    SOCKET fd=Network_Util::Instance().build_socket(TCP);
    if(fd<=0)
    {
#if UPNP_LOG_ON
        cout<<"failed to create socket!"<<endl;
#endif
        return;
    }
    if(!Network_Util::Instance().connect(fd,m_lgd_ip,m_lgd_port,MAX_WAIT_TIME)){
#if UPNP_LOG_ON
        cout<<"failed to connect to the device!"<<endl;
#endif
        Network_Util::Instance().close_socket(fd);
        return;
    }
    auto conn=newConnection(fd,UPNP_ADDPORTMAPPING);
    if(conn)
    {
        conn->start_work();
        addConnection(fd,conn);
        if(callback)conn->setUPNPCallback(callback);
        conn->send_add_port_mapper(type,internal_ip,internal_port,external_port,description,alive_time);
    }
}
void UpnpMapper::get_specific_port_mapping_entry(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback)
{
    SOCKET fd=Network_Util::Instance().build_socket(TCP);
    if(fd<=0)
    {
#if UPNP_LOG_ON
        cout<<"failed to create socket!"<<endl;
#endif
        return;
    }
    if(!Network_Util::Instance().connect(fd,m_lgd_ip,m_lgd_port,MAX_WAIT_TIME)){
#if UPNP_LOG_ON
        cout<<"failed to connect to the device!"<<endl;
#endif
        Network_Util::Instance().close_socket(fd);
        return;
    }
    auto conn=newConnection(fd,UPNP_GETSPECIFICPORTMAPPINGENTRY);
    if(conn)
    {
        conn->start_work();
        addConnection(fd,conn);
        if(callback)conn->setUPNPCallback(callback);
        conn->send_get_specific_port_mapping_entry(type,external_port);
    }
}
void UpnpMapper::delete_port_mapper(SOCKET_TYPE type,int external_port,UPNPCALLBACK callback)
{
    SOCKET fd=Network_Util::Instance().build_socket(TCP);
    if(fd<=0)
    {
#if UPNP_LOG_ON
        cout<<"failed to create socket!"<<endl;
#endif
        return;
    }
    if(!Network_Util::Instance().connect(fd,m_lgd_ip,m_lgd_port,MAX_WAIT_TIME)){
#if UPNP_LOG_ON
        cout<<"failed to connect to the device!"<<endl;
#endif
        Network_Util::Instance().close_socket(fd);
        return;
    }
    auto conn=newConnection(fd,UPNP_DELETEPORTMAPPING);
    if(conn)
    {
        conn->start_work();
        addConnection(fd,conn);
        if(callback)conn->setUPNPCallback(callback);
        conn->send_delete_port_mapper(type,external_port);
    }
}
