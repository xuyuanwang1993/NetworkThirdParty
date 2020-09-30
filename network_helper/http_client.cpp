#include "http_client.h"
#include "http_send_buf.h"
using namespace micagent;
//http://www.test.com:8554
//http://www.test.com
//http://www.test.com/test
//http://www.test.com:8554/test
//http://114.114.114.114/test
//http://114.114.114.114
//http://114.114.114.114:8554/test
http_client::url_info http_client::parse_url_info(const string &url, const string _expect_protocal, uint16_t _default_protocal_port)
{
    MICAGENT_INFO("input url:%s",url.c_str());
    url_info info(_expect_protocal,_default_protocal_port);
    if(url.length()>4096){
        MICAGENT_ERROR("url %s overflow the max url size(%u)!",url.c_str(),HTTP_MAX_URL_SIZE);
        return info;
    }
    //check protocal_head://  part
    uint32_t begin_len=0;
    static const char *const protocal_token="://";
    static const  uint32_t protocal_token_size=strlen(protocal_token);
    auto pos=url.find(protocal_token,0,protocal_token_size);
    if(pos!=string::npos){
        auto protocal_head=url.substr(0,pos);
        begin_len+=protocal_head.length()+protocal_token_size;
        if(_expect_protocal!=protocal_head)
        {
            MICAGENT_ERROR("expect_protocal %s not match with %s",_expect_protocal.c_str(),protocal_head.c_str());
            return info;
        }
    }
    char domain[HTTP_MAX_URL_SIZE]={0};
    char api_info[HTTP_MAX_URL_SIZE]={0};
    if(sscanf(url.c_str()+begin_len,"%[^:]:%hu/%s",domain,&info.port,api_info)==3)
    {
        info.ip=domain;
        info.api=api_info;
    }
    else if (sscanf(url.c_str()+begin_len,"%[^:]:%hu",domain,&info.port)==2) {
        info.ip=domain;
    }
    else if(sscanf(url.c_str()+begin_len,"%[^/]/%s",domain,api_info)==2){
        info.ip=domain;
        info.api=api_info;
    }
    else {
        info.ip=domain;
    }
    info.host=info.ip;
    info.ip=NETWORK.parase_domain(info.ip);
    if(info.port==0)info.port=info.default_protocal_port;
    MICAGENT_DEBUG("expect_protocal:%s default_protocal_port:%hu  ip:%s  port:%hu api:%s",info.expect_protocal.c_str(),info.default_protocal_port,info.ip.c_str(),info.port,info.api.c_str());
    return info;
}
http_client::http_client(shared_ptr<tcp_connection_helper>helper, const string &ip, uint16_t port, const string &api):tcp_client (helper,ip,port),m_recv_callback(nullptr)\
,m_http_connect_callback(nullptr),m_http_send_helper(new http_helper()),m_http_recv_helper(new http_helper())
{
    m_send_cache.reset(new http_send_buf(1000));
    m_http_send_helper->set_api(api);
    m_http_recv_helper->set_packet_finished_callback(m_recv_callback);
}
http_client::~http_client()
{

}
bool  http_client::handle_read()
{
    char buf[2048]={0};
    auto len=::recv(m_tcp_channel->fd(),buf,2048,0);
    if(len==0)return false;
    if(len<0){
        if(errno==EAGAIN||errno==EINTR)return true;
        else {
            return false;
        }
    }
    else {
        if(m_http_recv_helper)m_http_recv_helper->update(buf,len);
    }
    return true;
}
void  http_client::tear_down()
{
    clear_connection_info();
}
bool http_client::send_http_packet(map<string, string> head_map, const void *buf, uint32_t buf_len, const void *param, uint32_t param_len, HTTP_PACKET_TYPE type, const string &payload_type,bool reset)
{
    if(!check_is_connected())return false;
    lock_guard<mutex>locker(m_mutex);
    if(!m_send_cache)return false;
    m_http_send_helper->set_packet_type(type);
    if(type==HTTP_POST_REQUEST||type==HTTP_RESPONSE){
        m_http_send_helper->set_body(buf,buf_len,payload_type);
    }
    else if (type==HTTP_GET_REQUEST) {
        m_http_send_helper->set_get_param(string(reinterpret_cast<const char *>(param),param_len).c_str());
    }
    else {
        return false;
    }
    for(auto i:head_map)
    {
        m_http_send_helper->set_head(i.first,i.second);
    }
    auto packet=m_http_send_helper->build_packet(reset);
    if(!m_send_cache->append(packet.c_str(),packet.length()))return false;
    if(!m_tcp_channel->isWriting()){
        m_tcp_channel->enableWriting();
        auto connection_helper=m_connection_helper.lock();
        if(!connection_helper)return false;
        auto loop=connection_helper->get_loop().lock();
        if(!loop)return false;
        loop->updateChannel(m_tcp_channel);
    }
    return true;
}
void http_client::handle_connect_success()
{
    unique_lock<mutex>locker(m_mutex);
    m_http_recv_helper.reset(new http_helper());
    m_http_recv_helper->set_packet_finished_callback(m_recv_callback);
    if(m_http_connect_callback)
    {
        if(locker.owns_lock())locker.unlock();
        m_http_connect_callback();
    }
}
