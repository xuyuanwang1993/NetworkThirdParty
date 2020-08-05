#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H
#include <string>
#include"timer_queue.h"
#include "c_log.h"
#include <map>
namespace micagent {
using namespace std;
class http_response{
    static constexpr const char *const HTTP_VERSION="HTTP/1.1";
    static constexpr const char *const CONTENT_KEY="Content-Type";
    static constexpr const char *const DEFAULT_CONTENT_TYPE="application/json";
    static constexpr const char *const UNKNOWN_CONTENT_TYPE="application/octet-stream";
    static constexpr const char *const CONTENT_LENGTH_KEY="Content-Length";
    static constexpr const char *const  HEAD_END="\r\n\r\n";
    static constexpr const int HEAD_END_LEN=4;
    static constexpr const char * const LINE_END="\r\n";
    static constexpr const int LINE_END_LEN=2;
    static constexpr const int KEY_MAX_LEN=128;
    static constexpr const int VALUE_MAX_LEN=1024;
public:
    http_response(const string &status="200",const string &info="OK");
    http_response(const http_response&src);
    void update(const char *buf,uint32_t buf_len);
    void set_status(const string &status="200",const string &info="OK");
    pair<string,string>get_status()const;
    void set_body(const string &body,const string&content_type=DEFAULT_CONTENT_TYPE);
    void set_head(const string &key,const string &value);
    string get_head_info(const string &key)const;
    string get_body(int64_t wait_time_ms=0)const;
    string build_http_packet(bool reset);
    ~http_response();
    static const char * get_next_line(const char *input);
private:
    mutable mutex m_mutex;
    mutable condition_variable m_con;
    map<string,string>m_head_map;
    string m_status;
    string m_info;
    int m_body_len;
    string m_body;
    string m_buf_cache;
    string m_error_string;
    int m_error_status;
};
}
#endif // HTTP_RESPONSE_H
