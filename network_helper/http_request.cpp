#include "http_request.h"
#include "algorithm"
namespace micagent {
http_request::http_request(const string &api):m_api(api),m_body_len(-1),m_body(string())\
,m_buf_cache(string()),m_error_string("success"),m_error_status(0)
{
    m_head_map.emplace(make_pair(http_request::CONTENT_KEY,http_request::DEFAULT_CONTENT_TYPE));
}
http_request::http_request(const http_request&src):m_head_map(src.m_head_map),m_api(src.m_api),m_body_len(src.m_body_len),m_body(src.m_body)\
,m_buf_cache(string()),m_error_string("success"),m_error_status(0)
{

}
void http_request::update(const char *buf,uint32_t buf_len)
{
    lock_guard<mutex>locker(m_mutex);
    if(buf_len>0&&buf!=nullptr)m_buf_cache+=string(buf,buf_len);
    auto local_buf=m_buf_cache.c_str();
    if(m_body_len==(-1)){
        //not parse head
        auto tmp=search(local_buf,local_buf+m_buf_cache.length(),HEAD_END,HEAD_END+HEAD_END_LEN);
        if(tmp==local_buf+m_buf_cache.length())return;
        char command[10]={0};
        char api[256]={0};
        char http_version[32]={0};
        if(sscanf(local_buf,"%s %s %[^\r\n]",command,api,http_version)!=3){
            MICAGENT_MARK("unknown http head : %s!",local_buf);
            return;
        }
        MICAGENT_MARK("Head info: %s   \r\n%s %s %s!",local_buf,command,api,http_version);
        m_api=api;
        const char *tmp2=local_buf;
        while((tmp2=get_next_line(tmp2))!=nullptr&&tmp2<tmp){
            char key[KEY_MAX_LEN]={0};
            char value[VALUE_MAX_LEN]={0};
            if(sscanf(tmp2,"%s : %[^\r\n]",key,value)!=2){
                if(tmp2[0]!='\r'||tmp2[0]!='\n'){
                    m_error_status=1;
                m_error_string="false http head input!";
                }
                break;
            }
            m_head_map.emplace(key,value);
        }
        auto iter=m_head_map.find(http_request::CONTENT_LENGTH_KEY);
        if(iter==end(m_head_map)){
            m_head_map.emplace(make_pair(http_request::CONTENT_LENGTH_KEY,to_string(0)));
            m_body_len=0;
        }
        else {
            m_body_len=stoi(iter->second);
        }
        m_buf_cache=tmp+HEAD_END_LEN;
    }
    if(m_body_len!=(-1)&&m_body_len>static_cast<int>(m_body.length())){
        if(m_buf_cache.length()>=static_cast<string::size_type>(m_body_len)){
            m_body=m_buf_cache.substr(0,static_cast<string::size_type>(m_body_len));
            int left_len=m_buf_cache.length()-static_cast<string::size_type>(m_body_len);
            if(left_len>0){
                m_buf_cache=m_buf_cache.substr(static_cast<string::size_type>(m_body_len),static_cast<string::size_type>(left_len));
            }
            else {
                m_buf_cache=string();
            }
        }
    }
    if(m_body_len==static_cast<int>(m_body.length())||m_error_status!=0){
        m_con.notify_all();
    }
}
void http_request::set_head(const string &key,const string &value)
{
    lock_guard<mutex>locker(m_mutex);
    m_head_map.emplace(key,value);
}
void http_request::set_api(const string&api)
{
    lock_guard<mutex>locker(m_mutex);
    m_api=api;
}
void http_request::set_body(const string &body,const string&content_type)
{
    lock_guard<mutex>locker(m_mutex);
    m_body_len=static_cast<int>(body.length());
    m_body=body;
    if(!content_type.empty())m_head_map[CONTENT_KEY]=content_type;
    m_con.notify_all();
}
string http_request::get_head_info(const string &key)const
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_head_map.find(key);
    return iter==end(m_head_map)?string():iter->second;
}
string http_request::get_body(int64_t wait_time_ms)const
{
    unique_lock<mutex>locker(m_mutex);
    if(wait_time_ms!=0)m_con.wait_for(locker,chrono::milliseconds(wait_time_ms),[this](){
        return m_body.length()==static_cast<string::size_type>(m_body_len);
    });
    if(m_body.length()==static_cast<string::size_type>(m_body_len))return m_body;
    else return string();
}
string http_request::get_api()const
{
    lock_guard<mutex>locker(m_mutex);
    return  m_api;
}
string http_request::build_http_packet(bool reset)
{
    lock_guard<mutex>locker(m_mutex);
    if(m_api.empty())m_api="/";
    if(m_body.length()==static_cast<string::size_type>(m_body_len)){
        m_head_map.emplace(make_pair(http_request::CONTENT_LENGTH_KEY,to_string(m_body_len)));
    }
    string http_packet="POST ";
    http_packet+=m_api+" " +HTTP_VERSION;
    http_packet+=LINE_END;
    for(auto i: m_head_map){
        http_packet+=i.first+" : ";
        http_packet+=i.second;
        http_packet+=LINE_END;
    }
    http_packet+=LINE_END;
    http_packet+=m_body;
    if(reset){
        m_head_map.clear();
        m_head_map.emplace(make_pair(CONTENT_KEY,DEFAULT_CONTENT_TYPE));
        m_body_len=-1;
        m_body=string();
        m_error_status=0;
        m_error_string=string();
    }
    return http_packet;
}
http_request::~http_request()
{

}
const char * http_request::get_next_line(const char *input)
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
}
