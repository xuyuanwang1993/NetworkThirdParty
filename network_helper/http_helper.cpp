#include "http_helper.h"
#include <cstring>
#include<algorithm>
using namespace micagent;
const char *const http_helper::CONTENT_LENGTH_KEY="Content-Length";
http_helper::http_helper():m_packet_type(HTTP_UNKNOWN),m_status_code("200"),m_status_string("OK"),m_http_version(HTTP_VERSION),\
    m_request_method(HTTP_POST_STRING),m_get_param(""),m_api(""),m_body_len(HTTP_INVALID_BODY_LEN),m_body_filled_len(0),m_buf_cache_len(0),\
    m_buf_cache(new uint8_t[HTTP_MAX_HEADR_LEN],default_delete<uint8_t[]>()),m_http_packet_recv_callback(nullptr)
{
    memset(m_buf_cache.get(),0,HTTP_MAX_HEADR_LEN);
}
http_helper::http_helper(const http_helper &instance):m_head_map(instance.m_head_map),m_packet_type(instance.m_packet_type)\
,m_status_code(instance.m_status_code),m_status_string(instance.m_status_string),m_http_version(instance.m_http_version)\
,m_request_method(instance.m_request_method),m_get_param(instance.m_get_param),m_api(instance.m_api)\
,m_body_len(instance.m_body_len),m_body_filled_len(instance.m_body_filled_len),m_buf_cache_len(instance.m_buf_cache_len)\
,m_buf_cache(new uint8_t[HTTP_MAX_HEADR_LEN],default_delete<uint8_t[]>())\
,m_http_packet_recv_callback(instance.m_http_packet_recv_callback)
{
    memset(m_buf_cache.get(),0,HTTP_MAX_HEADR_LEN);
    if(m_buf_cache_len>0){
        memcpy(m_buf_cache.get(),instance.m_buf_cache.get(),m_buf_cache_len);
    }
    if(m_body_len!=HTTP_INVALID_BODY_LEN){
        m_body.reset(new uint8_t[m_body_len+1],default_delete<uint8_t[]>());
        if(m_body_filled_len>0){
            memcpy(m_body.get(),instance.m_body.get(),m_body_filled_len);
        }
    }
}
uint32_t http_helper::update(const void *buf, uint32_t buf_len)
{
    unique_lock<mutex>locker(m_mutex);
    //reset cache
    if(m_buf_cache_len==0&&m_body_len==m_body_filled_len){
        m_body_len=HTTP_INVALID_BODY_LEN;
        m_body.reset();
        reset_header_info();
    }
    if(HTTP_INVALID_BODY_LEN==m_body_len)
    {
        if(buf_len>0&&buf){
            if(buf_len+m_buf_cache_len>HTTP_MAX_HEADR_LEN){
                throw std::overflow_error("http header overflow!");
            }
            else {
                memcpy(m_buf_cache.get()+m_buf_cache_len,buf,buf_len);
                m_buf_cache_len+=buf_len;
            }
        }
        //search "\r\n\r\n"
        auto tmp=search(m_buf_cache.get(),m_buf_cache.get()+m_buf_cache_len,HEAD_END,HEAD_END+HEAD_END_LEN);
        if(tmp==m_buf_cache.get()+m_buf_cache_len)return 0;//not find the header end
        //parse first line   str1 str2 str3\r\n
        parse_first_len();
        //parse key:value
        const uint8_t *tmp2=m_buf_cache.get();
        while((tmp2=get_next_line(tmp2))!=nullptr&&tmp2<tmp){
            char key[KEY_MAX_LEN]={0};
            char value[VALUE_MAX_LEN]={0};
            if(sscanf(reinterpret_cast<const char *>(tmp2),"%[^:]: %[^\r\n]",key,value)!=2){
                if(tmp2[0]!='\r'||tmp2[0]!='\n'){
                    MICAGENT_ERROR("false http input:%s",string(reinterpret_cast<const char *>(m_buf_cache.get()),tmp-m_buf_cache.get()).c_str());
                }
                break;
            }
            m_head_map[key]=value;
        }
        auto iter=m_head_map.find(CONTENT_LENGTH_KEY);
        if(iter==end(m_head_map)){
            m_head_map.emplace(make_pair(CONTENT_LENGTH_KEY,to_string(0)));
            m_body_len=0;
        }
        else {
            m_body_len=stoi(iter->second);
            if(m_packet_type!=HTTP_RESPONSE&&m_packet_type!=HTTP_POST_REQUEST){
                m_body_len=0;
                m_head_map.erase(iter);
            }
        }
        m_body_filled_len=0;
        m_body.reset(new uint8_t[m_body_len+1],default_delete<uint8_t[]>());
        auto header_len=tmp-m_buf_cache.get()+HEAD_END_LEN;
        auto left_len=m_buf_cache_len-header_len;
        if(left_len>m_body_len){
            throw std::overflow_error("http data overflow!");
        }
        m_buf_cache_len=0;
        if(left_len>0){
            memcpy(m_body.get()+m_body_filled_len,m_buf_cache.get()+header_len,left_len);
            m_body_filled_len+=left_len;
        }
    }
    else {
        if(buf_len>0&&buf){
            if(buf_len+m_body_filled_len>m_body_len){
                throw std::overflow_error("http header overflow!");
            }
            else {
                memcpy(m_body.get()+m_body_filled_len,buf,buf_len);
                m_body_filled_len+=buf_len;
            }
        }
    }
    if(m_body_filled_len==m_body_len){
        //packet recv_over
        if(m_http_packet_recv_callback){
            auto data=m_body;
            auto data_len=m_body_len;
            if(locker.owns_lock())locker.unlock();
            m_http_packet_recv_callback(pair<shared_ptr<uint8_t>,uint32_t>(data,data_len));
        }
        if(!locker.owns_lock())locker.lock();
        m_con.notify_all();
        return 0;
    }
    else {
        if(m_body_len!=HTTP_INVALID_BODY_LEN)return m_body_len-m_body_filled_len;
        else {
            return 0;
        }
    }
}
void http_helper::set_head(const string &key,const string &value)
{
    lock_guard<mutex>locker(m_mutex);
    m_head_map[key]=value;
}
void http_helper:: set_api(const string&api)
{
    lock_guard<mutex>locker(m_mutex);
    m_api=api;
}
void http_helper::set_packet_type(HTTP_PACKET_TYPE type)
{
    lock_guard<mutex>locker(m_mutex);
    m_packet_type=type;
    if(type==HTTP_GET_REQUEST){
        m_body.reset();
        m_body_len=0;
        m_body_filled_len=0;
        m_request_method=HTTP_GET_STRING;
    }
    else if (type==HTTP_POST_REQUEST) {
        m_request_method=HTTP_POST_STRING;
    }
    load_default_set();
}
void http_helper::set_get_param(const string &param)
{
    lock_guard<mutex>locker(m_mutex);
    m_get_param=param;
}
void http_helper::set_response_status(const string &status_code,const string &status_string)
{
    lock_guard<mutex>locker(m_mutex);
    m_status_code=status_code;
    m_status_string=status_string;
}
void http_helper::set_body(const void *buf,uint32_t buf_len,const string&content_type)
{
    lock_guard<mutex>locker(m_mutex);
    if(m_packet_type==HTTP_POST_REQUEST||m_packet_type==HTTP_RESPONSE){
        m_body_len=buf_len;
        m_body.reset(new uint8_t[m_body_len+1],default_delete<uint8_t[]>());
        memcpy(m_body.get(),buf,m_body_len);
        m_body_filled_len=m_body_len;
        if(buf_len>0){
            m_head_map[CONTENT_KEY]=content_type;
            m_head_map[CONTENT_LENGTH_KEY]=to_string(buf_len);
        }
        m_con.notify_all();
    }
}
string http_helper::get_head_info(const string &key)const
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_head_map.find(key);
    if(iter!=end(m_head_map))return iter->second;
    else {
        return string("");
    }
}
string http_helper::get_api()const
{
    lock_guard<mutex>locker(m_mutex);
    return m_api;
}
HTTP_PACKET_TYPE http_helper::get_packet_type()const
{
    lock_guard<mutex>locker(m_mutex);
    return m_packet_type;
}
string http_helper::get_get_param()const
{
    lock_guard<mutex>locker(m_mutex);
    return m_get_param;
}
pair<string,string>http_helper::get_response_status()const
{
    lock_guard<mutex>locker(m_mutex);
    return {m_status_code,m_status_string};
}
pair<shared_ptr<uint8_t>,uint32_t> http_helper::get_body(int64_t wait_time_ms)const
{
    unique_lock<mutex>locker(m_mutex);
    if(wait_time_ms!=0)m_con.wait_for(locker,chrono::milliseconds(wait_time_ms),[this](){
        return m_body_filled_len==static_cast<string::size_type>(m_body_len);
    });
    if(m_body_filled_len==static_cast<string::size_type>(m_body_len))return make_pair(m_body,m_body_len);
    else return make_pair(shared_ptr<uint8_t>(),0);
}
string http_helper::build_packet(bool reset)
{
    string ret("");
    unique_lock<mutex>locker(m_mutex);
    switch (m_packet_type) {
    case HTTP_RESPONSE:
        ret=build_http_response();
        break;
    case HTTP_GET_REQUEST:
        ret=build_http_get_request();
        break;
    case HTTP_POST_REQUEST:
        ret=build_http_post_request();
        break;
    case HTTP_UNKNOWN:
        MICAGENT_WARNNING("unknown http packets!");
        break;
    }
    if(reset){
        reset_header_info();
        m_body_len=HTTP_INVALID_BODY_LEN;
        m_body.reset();
    }
    return ret;
}
http_helper::~http_helper()
{

}
const char *http_helper::get_next_line(const char *input)
{
    if(!input)return  nullptr;
    while(*input!='\0'){
        if(*input=='\r'||*input=='\n'){
            //find the end
            input++;
            if(*input=='\n') ++input;
            return input;
        }
        input++;
    }
    return  nullptr;
}
const uint8_t *http_helper::get_next_line(const uint8_t *input)
{
    if(!input)return  nullptr;
    while(*input!='\0'){
        if(*input=='\r'||*input=='\n'){
            //find the end
            input++;
            if(*input=='\n') ++input;
            return input;
        }
        input++;
    }
    return  nullptr;
}
void http_helper::parse_first_len()
{
    char str1[1024]={0};
    char str2[1024]={0};
    char str3[1024]={0};
    if(sscanf(reinterpret_cast<const char *>(m_buf_cache.get()),"%s %s %[^\r\n]",str1,str2,str3)!=3){
        MICAGENT_MARK("unknown http head : %s!",string(reinterpret_cast<const char *>(m_buf_cache.get()),m_buf_cache_len).c_str());
        m_packet_type=HTTP_UNKNOWN;
    }
    else {
        MICAGENT_INFO("%s",string(reinterpret_cast<const char *>(m_buf_cache.get()),m_buf_cache_len).c_str());
        uint32_t api_pos=0;
        if(str2[0]=='/')api_pos=1;
        if(strncmp(str1,HTTP_POST_STRING,strlen(HTTP_POST_STRING))==0){
            m_packet_type=HTTP_POST_REQUEST;
            m_api=str2+api_pos;
            m_http_version=str3;
            m_request_method=HTTP_POST_STRING;
        }
        else if(strncmp(str1,HTTP_GET_STRING,strlen(HTTP_GET_STRING))==0){
            m_api=str2;
            m_packet_type=HTTP_GET_REQUEST;
            m_http_version=str3;
            m_request_method=HTTP_GET_STRING;
            auto pos=m_api.find_first_of('?');
            if(pos!=string::npos&&pos!=m_api.length()-1){
                m_get_param=m_api.substr(pos+1,m_api.length()-(pos+1));
                if(api_pos!=pos)m_api=m_api.substr(api_pos,pos);
                else {
                    m_api="";
                }
            }
        }
        else if(strncmp(str1,HTTP_VERSION,strlen(HTTP_VERSION))==0){
            m_packet_type=HTTP_RESPONSE;
            m_http_version=str1;
            m_status_code=str2;
            m_status_string=str3;
        }
        else {
            m_packet_type=HTTP_UNKNOWN;
        }
    }
}
void http_helper::reset_header_info()
{
    m_packet_type=HTTP_UNKNOWN;
    m_head_map.clear();
    m_http_version=HTTP_VERSION;
    load_default_set();
}
string http_helper::build_http_response()
{
    string http_packet=HTTP_VERSION;
    if(m_status_code.empty()){
        m_status_code="200";
        m_status_string="OK";
    }
    http_packet+=" "+m_status_code+" "+m_status_string+LINE_END;
    for(auto i: m_head_map){
        http_packet+=i.first+": ";
        http_packet+=i.second;
        http_packet+=LINE_END;
    }
    http_packet+=LINE_END;
    if(m_body_len==m_body_filled_len)http_packet+=string(reinterpret_cast<const char *>(m_body.get()),m_body_len);
    return http_packet;
}
string http_helper::build_http_get_request()
{
    string http_packet=m_request_method;
    http_packet+=" /"+m_api;
    if(!m_get_param.empty()){
        http_packet+="?"+m_get_param;
    }
    http_packet+=" "+m_http_version+LINE_END;
    for(auto i: m_head_map){
        http_packet+=i.first+": ";
        http_packet+=i.second;
        http_packet+=LINE_END;
    }
    http_packet+=LINE_END;
    return http_packet;
}
string http_helper::build_http_post_request()
{
    string http_packet=m_request_method;
    http_packet+=" /"+m_api;
    http_packet+=" "+m_http_version+LINE_END;
    for(auto i: m_head_map){
        http_packet+=i.first+": ";
        http_packet+=i.second;
        http_packet+=LINE_END;
    }
    http_packet+=LINE_END;
    if(m_body_len==m_body_filled_len)http_packet+=string(reinterpret_cast<const char *>(m_body.get()),m_body_len);
    return http_packet;
}
void http_helper::util_test()
{
#if UTIL_TEST
    {
        //test http get_request build;
        http_helper helper;
        string body="AAAAAAAAAAA";
        string get_param="config=test&value=100";
        helper.set_packet_type(HTTP_GET_REQUEST);
        helper.set_head("test","test");
        helper.set_head("nihao","nihao");
        helper.set_get_param(get_param);
        helper.set_head("host","192.168.2.199");
        helper.set_body(body.c_str(),body.length());
        auto get_request_packet_not_reset=helper.build_packet(false);
        MICAGENT_WARNNING("get_request_packet_not_reset %s",get_request_packet_not_reset.c_str());
        auto get_request_packet_reset=helper.build_packet();
        MICAGENT_WARNNING("get_request_packet_reset %s",get_request_packet_reset.c_str());
        auto get_request_packet_after_reset=helper.build_packet(false);
        MICAGENT_WARNNING("get_request_packet_after_reset %s",get_request_packet_after_reset.c_str());
        //http get_request parse;
        http_helper helper1;
        thread t([&helper1](){
            auto ret=helper1.get_body(1000);
            if(ret.first){
                MICAGENT_DEBUG("thread1 %s",string(reinterpret_cast<const char *>(ret.first.get()),ret.second).c_str());
                MICAGENT_DEBUG("api %s",helper1.get_api().c_str());
                MICAGENT_DEBUG("param %s",helper1.get_get_param().c_str());
                MICAGENT_DEBUG("host %s",helper1.get_head_info("host").c_str());
            }
            else {
                MICAGENT_DEBUG("thread1 nil");
            }
        });
        auto total_len=get_request_packet_not_reset.length();
        for(int i=0;i<total_len;i++)
        {
            helper1.update(get_request_packet_not_reset.c_str()+i,1);
        }
        thread t2([&helper1](){
            auto ret=helper1.get_body();
            if(ret.first)MICAGENT_DEBUG("thread2 %s",string(reinterpret_cast<const char *>(ret.first.get()),ret.second).c_str());
            else {
                MICAGENT_DEBUG("thread2 nil");
            }
        });
        http_helper helper2;
        thread t3([&helper2](){
            auto ret=helper2.get_body(1000);
            if(ret.first)MICAGENT_DEBUG("thread3 %s",string(reinterpret_cast<const char *>(ret.first.get()),ret.second).c_str());
            else {
                MICAGENT_DEBUG("thread3 nil");
            }
        });
        t.join();
        t2.join();
        t3.join();
    }
    {//test http_post_request
        HTTP_PACKET_RECV_CALLBACK cb=[] (pair<shared_ptr<uint8_t>,uint32_t>body){
           MICAGENT_DEBUG("body_len %u,content:%s",body.second,string(reinterpret_cast<const char *>(body.first.get()),body.second).c_str());
        };
        http_helper helper;
        helper.set_packet_type(HTTP_POST_REQUEST);
        string body="AAAAAAAAAAAAAAAAAAAAAA";
        helper.set_head("test","test");
        helper.set_head("nihao","nihao");
        helper.set_head("host","192.168.2.199");
        helper.set_body(body.c_str(),body.length());
        http_helper helper2(helper);
        string body2="BBBBBBBBBBBBBBBBBBBBBBBBBB";
        helper.set_body(body2.c_str(),body2.length());
        auto packet1=helper.build_packet();
        auto packet2=helper2.build_packet();
        MICAGENT_WARNNING("packet1:%s",packet1.c_str());
        MICAGENT_WARNNING("packet2:%s",packet2.c_str());
        http_helper helper3;
        helper3.set_packet_finished_callback(cb);
        for(int i=0;i<packet1.length();i++)
        {
            helper3.update(packet1.c_str()+i,1);
        }
        for(int i=0;i<packet2.length();i++)
        {
            helper3.update(packet2.c_str()+i,1);
        }
        auto packet3=helper3.build_packet();
        MICAGENT_WARNNING("packet3:%s",packet3.c_str());
    }
    {//test http_response
        HTTP_PACKET_RECV_CALLBACK cb=[] (pair<shared_ptr<uint8_t>,uint32_t>body){
           MICAGENT_DEBUG("body_len %u,content:%s",body.second,string(reinterpret_cast<const char *>(body.first.get()),body.second).c_str());
        };
        http_helper helper;
        helper.set_packet_type(HTTP_RESPONSE);
        string body="AAAAAAAAAAAAAAAAAAAAAA";
        helper.set_head("test","test");
        helper.set_head("nihao","nihao");
        helper.set_head("host","192.168.2.199");
        helper.set_body(body.c_str(),body.length());
        http_helper helper2(helper);
        string body2="BBBBBBBBBBBBBBBBBBBBBBBBBB";
        helper.set_body(body2.c_str(),body2.length());
        auto packet1=helper.build_packet();
        auto packet2=helper2.build_packet();
        MICAGENT_WARNNING("packet1:%s",packet1.c_str());
        MICAGENT_WARNNING("packet2:%s",packet2.c_str());
        http_helper helper3;
        helper3.set_packet_finished_callback(cb);
        for(int i=0;i<packet1.length();i++)
        {
            helper3.update(packet1.c_str()+i,1);
        }
        for(int i=0;i<packet2.length();i++)
        {
            helper3.update(packet2.c_str()+i,1);
        }
        helper3.set_response_status("404","Not Found");
        auto packet3=helper3.build_packet();
        MICAGENT_WARNNING("packet3:%s",packet3.c_str());
        MICAGENT_WARNNING("%s",helper3.get_head_info("Connection").c_str());
    }
#endif
}
void http_helper::load_default_set()
{
    m_head_map["Connection"]="keep-alive";
}
