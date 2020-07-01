#define _GLIBCXX_USE_C99 1
#include "http_response.h"
#include <algorithm>
#include <string>
namespace micagent {
http_response::http_response(const string &status,const string &info):m_status(status),m_info(info),m_body_len(-1),m_body(string())\
,m_buf_cache(string()),m_error_string("success"),m_error_status(0)
{
    m_head_map.emplace(make_pair(http_response::CONTENT_KEY,http_response::DEFAULT_CONTENT_TYPE));
}
http_response::http_response(const http_response&src):m_head_map(src.m_head_map),m_status(src.m_status),m_info(src.m_info),m_body_len(src.m_body_len),m_body(src.m_body)\
,m_buf_cache(string()),m_error_string("success"),m_error_status(0)
{

}
void http_response::update(const char *buf,uint32_t buf_len)
{
    lock_guard<mutex>locker(m_mutex);
    if(buf_len>0&&buf!=nullptr)m_buf_cache+=string(buf,buf_len);
    auto local_buf=m_buf_cache.c_str();
    if(m_body_len==(-1)){
        //not parse head
        auto tmp=search(local_buf,local_buf+m_buf_cache.length(),HEAD_END,HEAD_END+HEAD_END_LEN);
        if(tmp==local_buf+m_buf_cache.length())return;
        char status[10]={0};
        char info_string[256]={0};
        char http_version[32]={0};
        if(sscanf(local_buf,"%s %s %[^\r\n]",http_version,status,info_string)!=3){
            MICAGENT_MARK("unknown http head : %s!",local_buf);
            return;
        }
        MICAGENT_MARK("Head info: %s   \r\n%s %s %s!",local_buf,http_version,status,info_string);
        m_status=status;
        m_info=info_string;
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
        auto iter=m_head_map.find(http_response::CONTENT_LENGTH_KEY);
        if(iter==end(m_head_map)){
            m_head_map.emplace(make_pair(http_response::CONTENT_LENGTH_KEY,to_string(0)));
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
void http_response::set_head(const string &key,const string &value)
{
    lock_guard<mutex>locker(m_mutex);
    m_head_map.emplace(key,value);
}
pair<string,string>http_response::get_status()const
{
    lock_guard<mutex>locker(m_mutex);
    return make_pair(m_status,m_info);
}
void http_response::set_body(const string &body,const string&content_type)
{
    lock_guard<mutex>locker(m_mutex);
    m_body_len=static_cast<int>(body.length());
    m_body=body;
    if(!content_type.empty())m_head_map[CONTENT_KEY]=content_type;
    m_con.notify_all();
}
string http_response::get_head_info(const string &key)const
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_head_map.find(key);
    return iter==end(m_head_map)?string():iter->second;
}
string http_response::get_body(int64_t wait_time_ms)const
{
    unique_lock<mutex>locker(m_mutex);
    if(wait_time_ms!=0)m_con.wait_for(locker,chrono::milliseconds(wait_time_ms),[this](){
        return m_body.length()==static_cast<string::size_type>(m_body_len);
    });
    if(m_body.length()==static_cast<string::size_type>(m_body_len))return m_body;
    else return string();
}
string http_response::build_http_packet(bool reset)
{
    lock_guard<mutex>locker(m_mutex);
    if(m_status.empty()){
        m_status="200";
        m_info="OK";
    }
    if(m_body.length()==static_cast<string::size_type>(m_body_len)){
        m_head_map.emplace(make_pair(http_response::CONTENT_LENGTH_KEY,to_string(m_body_len)));
    }
    string http_packet=HTTP_VERSION;
    http_packet+=" ";
    http_packet+=m_status+" " +m_info;
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
http_response::~http_response()
{

}
const char * http_response::get_next_line(const char *input)
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
