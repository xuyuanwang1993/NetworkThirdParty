#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#define _GLIBCXX_USE_C99 1
#include <string>
#include"timer_queue.h"
#include "c_log.h"
#include <map>
namespace micagent {
using namespace std;
class http_request{
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
    http_request(const string &api="/");
    http_request(const http_request&src);
    void update(const char *buf,uint32_t buf_len);
    void set_head(const string &key,const string &value);
    void set_api(const string&api);
    void set_body(const string &body,const string&content_type=DEFAULT_CONTENT_TYPE);
    string get_head_info(const string &key)const;
    string get_body(int64_t wait_time_ms=0)const;
    string get_api()const;
    string build_http_packet(bool reset);
    ~http_request();
    static const char * get_next_line(const char *input);
private:
    mutable mutex m_mutex;
    mutable condition_variable m_con;
    map<string,string>m_head_map;
    string m_api;
    int m_body_len;
    string m_body;
    string m_buf_cache;
    string m_error_string;
    int m_error_status;
};
}
#endif // HTTP_REQUEST_H
