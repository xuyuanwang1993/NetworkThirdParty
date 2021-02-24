#ifndef UNIX_SOCKET_HELPER_H
#define UNIX_SOCKET_HELPER_H
#include <unistd.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<string>
#define SOCKET int
#define INVALID_SOCKET (0xffffffff)
using std::string;
/**
 * @brief The unix_socket_base class define the unix_socket base class
 */
class unix_socket_base{
public:
    static constexpr uint32_t PATH_MAX_SIZE=108;
public:
    /**
     * @brief unix_socket_base constructor for the base class
     * @param path local path for the socket
     */
    unix_socket_base(const string &path="");
    /**
     * @brief ~unix_socket_base it will unlink the local path and close the open socket
     */
    virtual ~unix_socket_base();
    /**
     * @brief fd return the descriptor of the socket
     * @return if fd hasn't been set up,it will return -1(0xffffffff)
     */
    SOCKET fd()const{return m_fd;}
    /**
     * @brief build set up the socket
     * @return it will return -1(0xffffffff) when fail setting up the socket
     */
    virtual SOCKET build()=0;
    /**
     * @brief send send a frame to the peer by default_addr or sepcified by the peer_path input
     * @param buf the buf need to send
     * @param len the buf's len
     * @param peer_path destination
     * @return the real send size and -1 for a send error
     */
    virtual ssize_t send(const void *buf,uint32_t len,const string &peer_path="",uint32_t time_out_ms=200)=0;
    /**
     * @brief recv recv a frame from the socket's cache buf
     * @param buf where the recv bytes is saved
     * @param max_len max recv  len
     * @return the real bytes that is written to the buf
     */
    ssize_t recv(void *buf,uint32_t max_len,uint32_t time_out_ms=200);
protected:
    /**
     * @brief m_fd socket's descriptor
     */
    SOCKET m_fd;
    /**
     * @brief m_path local path
     */
    char m_path[PATH_MAX_SIZE+1];
};
/**
 * @brief The unix_stream_socket class like tcp
 */
class  unix_stream_socket:public unix_socket_base{
public:
    unix_stream_socket(const string &path="");
    SOCKET build()override;
    /**
      * @brief reset when a passive client connection is arriving,you can init a unix_stream_socket with this function
      * @param fd new unix_stream_socket's descriptor
      * @param path it's local path if it's necessary
      */
    void reset(SOCKET fd,const string & path="");
    ssize_t send(const void *buf,uint32_t len,const string &peer_path="",uint32_t time_out_ms=200)override;
    /**
     * @brief listen when the socket works as a server,you man call this function
     * @param size the waiting queue's size
     * @return if an error occured,it will return false
     */
    bool listen(int32_t size=10);
    /**
     * @brief connect when it work as an active client,you can call this function to connect to the server
     * @param server_path specify a server
     * @return if an error occured,it will return false
     */
    bool connect(const string &server_path);
    /**
     * @brief aacept accept a unix_socket connection when it works as a server
     * @return
     */
    SOCKET aacept();
    ~unix_stream_socket()override{

    }
};
/**
 * @brief The unix_stream_socket class just like udp
 */
class  unix_dgram_socket:public unix_socket_base{
    struct private_protocal_header{
         uint16_t packet_seq;//set a same random num for one packet
         uint32_t left_len;
     };
     static constexpr uint32_t DEFAULT_PACKET_HEADER_LEN=6;
     static constexpr uint32_t DEFAULT_MTU_SIZE=10000;
    static constexpr uint32_t DEFAULT_PACKET_SIZE=DEFAULT_MTU_SIZE+DEFAULT_PACKET_HEADER_LEN;
public:
    unix_dgram_socket(const string &path="");
    SOCKET build()override;
    /**
      * @brief get_peer_path
      * @return
      */
    string get_peer_path()const{return m_peer_path.sun_path;}
    /**
     * @brief set_peer_path cache peer path will be used when you call send with no specifying the destination
     * @param peer_path
     */
    void set_peer_path(const string &peer_path){
        strncpy(m_peer_path.sun_path,peer_path.c_str(),PATH_MAX_SIZE);
    }
    /**
     * @brief get_local_path you can put the local_path into the send_packet when you need a ack from a server
     * @return
     */
    string get_local_path()const{return m_path;}
    ssize_t send(const void *buf,uint32_t len,const string &peer_path="",uint32_t time_out_ms=200)override;
     /**
      * @brief send_packet private  protocal
      * @param buf
      * @param len
      * @param time_out_ms
      * @param peer_path
      * @return
      * @note only used by a udp server
      */
     ssize_t send_packet(const void *buf,uint32_t len,uint32_t time_out_ms=200,const string &peer_path="");
     /**
      * @brief recv_packet private  protocal
      * @param buf
      * @param max_len
      * @param time_out_ms
      * @return
      * @note only use by a udp client
      */
     ssize_t recv_packet(void *buf,uint32_t max_len,uint32_t time_out_ms=200);
    ~unix_dgram_socket()override{

    }
private:
    /**
      * @brief m_peer_path cache the peer path
      */
    sockaddr_un m_peer_path;
    uint8_t m_packet_send_cache[DEFAULT_PACKET_SIZE+1]={0};
     private_protocal_header m_recv_packet_header;
     uint32_t m_packet_cache_len;
     uint8_t m_packet_recv_cache[DEFAULT_PACKET_SIZE+1]={0};
};


#endif // UNIX_SOCKET_HELPER_H
