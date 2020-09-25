#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H
#include "http_helper.h"
#include "tcp_client.h"
namespace micagent {
using namespace std;
using HTTP_RECV_CALLBACK=function<void(const string &body)>;
class http_client:public tcp_client{
    /**
     * @brief HTTP_MAX_URL_SIZE define the max url len 4096
     */
    constexpr static  uint32_t HTTP_MAX_URL_SIZE=4096;
public:
    struct url_info{
        string expect_protocal;//expected ptotocal string just like 'http' ,'https','ws',etc.
        uint16_t default_protocal_port;//ptotocal's default  port
        string ip;//host ip
        string host;//host name
        uint16_t port;//host port
        string api;//source location
        url_info(const string _expect_protocal="",uint16_t _default_protocal_port=0):expect_protocal(_expect_protocal),default_protocal_port(_default_protocal_port),ip(""),host(""),port(0),api(""){

        }
    };
public:
    /**
     * @brief parse_url_info parase a url string
     * @param url url input
     * @param _expect_protocal expected protocal string
     * @param _default_protocal_port  protocal'default port
     * @return
     */
    static url_info parse_url_info(const string &url,const string _expect_protocal="http",uint16_t _default_protocal_port=80);
    http_client(shared_ptr<tcp_connection_helper>helper,const string &ip,uint16_t port,const string &api="");
    ~http_client()override;
    /**
     * @brief set_http_recv_callback set the callback which is callad when a http response is finished
     * @param callback
     */
    void set_http_recv_callback(const HTTP_PACKET_RECV_CALLBACK&callback){
        lock_guard<mutex>locker(m_mutex);
        m_recv_callback=callback;
    }
    /**
     * @brief set_http_connect_callback set the callback which is callad when this http connection is set up
     * @param callback
     */
    void set_http_connect_callback(const CONNECTION_SUCCESS_CALLBACK &callback){
        lock_guard<mutex>locker(m_mutex);
        m_http_connect_callback=callback;
        weak_ptr<tcp_client>weak_this=shared_from_this();
        m_connection_callback=[weak_this](){
            auto strong=weak_this.lock();
            auto ptr=reinterpret_cast<http_client *>(strong.get());
            if(ptr)ptr->handle_connect_success();
        };
    }
    /**
     * @brief send_http_packet send a http packet with the given param
     * @param head_map head info'map
     * @param buf body
     * @param buf_len body's len
     * @param param get request's param
     * @param param_len param's len
     * @param type
     * @param payload_type
     * @param reset when this flag is set,all head_map's cache will be clear
     * @return
     */
    virtual bool send_http_packet(map<string,string>head_map=map<string,string>(),const void *buf=nullptr,uint32_t buf_len=0,const void *param=nullptr,uint32_t param_len=0,HTTP_PACKET_TYPE type=HTTP_GET_REQUEST,const string &payload_type=http_helper::DEFAULT_CONTENT_TYPE,bool reset=true);
protected:
    /**
     * @brief handle_connect_success you can override this function
     */
    virtual void handle_connect_success();
    /**
     * @brief handle_read network io read handler
     * @return
     */
    bool handle_read()override;
    /**
     * @brief tear_down which be called when you active close the connection
     */
    void tear_down()override;
    /**
     * @brief get_init_status you check you own classes' status
     * @return
     */
    bool get_init_status()const override{
        return true;
    }
    /**
     * @brief clear_usr_status clear you own classes' status
     */
    void clear_usr_status()override{

    }
protected:
    /**
     * @brief m_recv_callback when a new http frame is received,this function will be called
     */
    HTTP_PACKET_RECV_CALLBACK m_recv_callback;
    /**
     * @brief m_http_connect_callback when this function is set up,this function will be called
     */
    CONNECTION_SUCCESS_CALLBACK m_http_connect_callback;
    /**
     * @brief m_http_send_helper helper for build send http packet
     */
    shared_ptr<http_helper>m_http_send_helper;
    /**
     * @brief m_http_recv_helper helper for handle received http packets
     */
    shared_ptr<http_helper>m_http_recv_helper;

};
}
#endif // HTTP_CLIENT_H
