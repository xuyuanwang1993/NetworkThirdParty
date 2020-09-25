#ifndef HTTP_HELPER_H
#define HTTP_HELPER_H
#include <string>
#include"timer_queue.h"
#include "c_log.h"
#include <map>
namespace micagent {
using namespace std;
enum HTTP_PACKET_TYPE{
    HTTP_POST_REQUEST,//for post request
    HTTP_GET_REQUEST,//for get request
    HTTP_RESPONSE,//for response
    HTTP_UNKNOWN,//default status,you can't call build_packet
};
class  http_helper;
using HTTP_PACKET_RECV_CALLBACK=function<void (pair<shared_ptr<uint8_t>,uint32_t>)>;
class http_helper{
    friend class http_client;
    static constexpr uint32_t HTTP_INVALID_BODY_LEN=0xffffffff;
    static constexpr uint32_t HTTP_MAX_HEADR_LEN=1024*32;//32k
    static constexpr const char *const HTTP_POST_STRING="POST";
    static constexpr const char *const HTTP_GET_STRING="GET";
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
    http_helper();
    /**
     * @brief http_helper build a class from the existing class
     * @param instance
     */
    http_helper(const http_helper &instance);
    /**
     * @brief update update the http cache with a data stream
     * @param buf
     * @param buf_len
     * @return last incomplete packet's missing len
     */
    uint32_t update(const void *buf,uint32_t buf_len);
    /**
     * @brief set_head set the http_header's key:value
     * @param key
     * @param value
     */
    void set_head(const string &key,const string &value);
    /**
     * @brief set_api config the api
     * @param api
     */
    void set_api(const string&api);
    /**
     * @brief set_packet_type  set the work mode
     * @param type
     */
    void set_packet_type(HTTP_PACKET_TYPE type);
    /**
     * @brief set_get_param set the GET request's param
     * @param param
     */
    void set_get_param(const string &param);
    /**
     * @brief set_response_status set the http response's status
     * @param status_code http status
     * @param status_string http status string
     */
    void set_response_status(const string &status_code,const string &status_string);
    /**
     * @brief set_body set the payload when it works as a http response or a post request
     * @param buf
     * @param buf_len
     * @param content_type the standard http content type string
     */
    void set_body(const void *buf,uint32_t buf_len,const string&content_type=DEFAULT_CONTENT_TYPE);
    /**
     * @brief get_head_info get the specific key's value string
     * @param key
     * @return if key is not existed,it will return  an empty string
     */
    string get_head_info(const string &key)const;
    /**
     * @brief get_api get the api of  http connection
     * @return
     */
    string get_api()const;
    /**
     * @brief get_packet_type get the work mode
     * @return
     */
    HTTP_PACKET_TYPE get_packet_type()const;
    /**
     * @brief get_get_param get the get request's param
     * @return
     */
    string get_get_param()const;
    /**
     * @brief get_response_status get http response's status
     * @return
     */
    pair<string,string>get_response_status()const;
    /**
     * @brief get_body get the http payload
     * @param wait_time_ms set it to nozero and block to get the payload
     * @return
     */
    pair<shared_ptr<uint8_t>,uint32_t> get_body(int64_t wait_time_ms=0)const;
    /**
     * @brief build_packet build a http packet with the content tha contains
     * @param reset if it's true,the function will reset all header info
     * @return
     */
    string build_packet(bool reset=true);
    ~http_helper();
    /**
     * @brief get_next_line get a new line start pos
     * @param input
     * @return
     */
    static const char * get_next_line(const char *input);
    /**
     * @brief get_next_line get a new line start pos
     * @param input
     * @return
     */
    static const uint8_t * get_next_line(const uint8_t *input);
    /**
     * @brief util_test test the member function
     */
    static void util_test();
    /**
     * @brief set_packet_finished_callback set the callback which will be call when you update the class status with the "update" function's calling and a finished http packet occurs
     * @param callback
     */
    void set_packet_finished_callback(const HTTP_PACKET_RECV_CALLBACK &callback){
        lock_guard<mutex>locker(m_mutex);
        m_http_packet_recv_callback=callback;
    }
protected:
    /**
     * @brief load_default_set  derived classes can do a default action'config in this function
     */
    virtual void load_default_set();
private:
    /**
     * @brief reset_header_info reset the header part to default status
     */
    void reset_header_info();
    /**
     * @brief parse_first_len parse http packet's first line
     */
    void parse_first_len();
    /**
     * @brief build_http_response build a http response packet
     * @return
     */
    string build_http_response();
    /**
     * @brief build_http_get_request build a http get requset packet
     * @return
     */
    string build_http_get_request();
    /**
     * @brief build_http_post_request build a http post requset packet
     * @return
     */
    string build_http_post_request();
private:
    mutable mutex m_mutex;
    /**
     * @brief m_con sync the body's blocking acquirement
     */
    mutable condition_variable m_con;
    /**
     * @brief m_head_map a map that saves all key:value pair
     */
    map<string,string>m_head_map;
    /**
     * @brief m_packet_type workr mode
     */
    HTTP_PACKET_TYPE m_packet_type;
    /**
     * @brief m_status_code http response status code
     */
    string m_status_code;
    /**
     * @brief m_status_string http response status string
     */
    string m_status_string;
    /**
     * @brief m_http_version http protocal's version
     */
    string m_http_version;
    /**
     * @brief m_request_method "GET" , "PORT",etc.
     */
    string m_request_method;
    /**
     * @brief m_get_param GET request's param
     */
    string m_get_param;
    /**
     * @brief m_api   the remote resource url you want to access
     */
    string m_api;
    /**
     * @brief m_body_len http body's len
     */
    uint32_t m_body_len;
    /**
     * @brief m_body_filled_len when it work as a http packet receiver,this object may be not equal with m_body_len
     */
    uint32_t m_body_filled_len;
    /**
     * @brief m_body save the http body
     */
    shared_ptr<uint8_t>m_body;
    /**
     * @brief m_buf_cache_len m_buf_cache's length
     */
    uint32_t m_buf_cache_len;
    /**
     * @brief m_buf_cache a cache which cache the outstanding data
     */
    shared_ptr<uint8_t>m_buf_cache;
    /**
     * @brief m_http_packet_recv_callback when a http packet is finished,this function will be called
     */
    HTTP_PACKET_RECV_CALLBACK m_http_packet_recv_callback;
};
}
#endif // HTTP_HELPER_H
