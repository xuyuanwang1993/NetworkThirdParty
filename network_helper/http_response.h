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
    /**
     * @brief http_response init am empty http response
     * @param status the response's status code
     * @param info the reason description
     */
    http_response(const string &status="200",const string &info="OK");
    /**
     * @brief http_response build a http response with a duplicated header from src
     * @param src
     */
    http_response(const http_response&src);
    /**
     * @brief update when you recv a http response ,you can call this function to input the data streaming
     * @param buf
     * @param buf_len
     * @details this function will notify the receiver who blocks in calling get_body
     */
    void update(const char *buf,uint32_t buf_len);
    /**
     * @brief set_status set the response's status
     * @param status
     * @param info
     */
    void set_status(const string &status="200",const string &info="OK");
    /**
     * @brief get_status get the response's status item
     * @return
     */
    pair<string,string>get_status()const;
    /**
     * @brief set_body set_body modify body and set the content-length
     * @param body
     * @param content_type
     */
    void set_body(const string &body,const string&content_type=DEFAULT_CONTENT_TYPE);
    /**
     * @brief set_head update the value that is specified by 'key'
     * @param key
     * @param value
     */
    void set_head(const string &key,const string &value);
    /**
     * @brief get_head_info get header info
     * @param key
     * @return
     */
    string get_head_info(const string &key)const;
    /**
     * @brief get_body timeout to get all body when it's ready
     * @param wait_time_ms max wait time
     * @return
     */
    string get_body(int64_t wait_time_ms=0)const;
    /**
     * @brief build_http_packet get complete http packet
     * @param reset if it is set true,it will reset all info except the api
     * @return
     */
    string build_http_packet(bool reset);
    ~http_response();
    /**
     * @brief get_next_line search a http head line
     * @param input
     * @return
     */
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
