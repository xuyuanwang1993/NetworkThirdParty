#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H
#define _GLIBCXX_USE_C99 1
#include <string>
#include"timer_queue.h"
#include "c_log.h"
#include <map>
namespace micagent {
using namespace std;
enum HTTP_REQUEST_TYPE{
    HTTP_POST,
    HTTP_GET,
};

class http_request{
    static  const char *const HTTP_VERSION;
    static  const char *const CONTENT_KEY;
    static  const char *const DEFAULT_CONTENT_TYPE;
    static  const char *const UNKNOWN_CONTENT_TYPE;
    static  const char *const CONTENT_LENGTH_KEY;
    static  const char *const  HEAD_END;
    static constexpr const int HEAD_END_LEN=4;
    static  const char * const LINE_END;
    static constexpr const int LINE_END_LEN=2;
    static constexpr const int KEY_MAX_LEN=128;
    static constexpr const int VALUE_MAX_LEN=1024;
public:
    /**
     * @brief http_request init an empty http request
     * @param api set the http request access path
     */
    http_request(const string &api="");
    /**
     * @brief http_request init a http request that copy header from src
     * @param src
     */
    http_request(const http_request&src);
    /**
     * @brief update when you recv a http request ,you can call this function to input the data streaming
     * @param buf
     * @param buf_len
     * @details this function will notify the receiver who blocks in calling get_body
     */
    void update(const char *buf,uint32_t buf_len);
    /**
     * @brief set_head set the head item
     * @param key
     * @param value
     */
    void set_head(const string &key,const string &value);
    /**
     * @brief set_api set the http access path
     * @param api
     */
    void set_api(const string&api);
    /**
     * @brief set_body modify body and set the content-length
     * @param body
     * @param content_type
     */
    void set_body(const string &body,const string&content_type=DEFAULT_CONTENT_TYPE);
    /**
     * @brief get_head_info get the head item
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
     * @brief get_api get the access path
     * @return
     */
    string get_api()const;
    /**
     * @brief build_http_packet get complete http packet
     * @param type HTTP_POST or HTTP_GET
     * @param reset if it is set true,it will reset all info except the api
     * @return
     */
    string build_http_packet(HTTP_REQUEST_TYPE type,bool reset);
    ~http_request();
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
    string m_api;
    int m_body_len;
    string m_body;
    string m_buf_cache;
    string m_error_string;
    int m_error_status;
};
}
#endif // HTTP_REQUEST_H
