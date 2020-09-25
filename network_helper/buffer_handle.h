#pragma once
#ifndef BUFFER_HANDLE_H
#define BUFFER_HANDLE_H
#include <list>
#include<mutex>
#include<memory>
#include<functional>
#include "network_util.h"
#include "c_log.h"
namespace micagent {
using namespace std;
/*读取数据回调函数，若设置了回调函数，数据将会移交给用户自行处理，不会存入缓存*/
using ReadBufferCallback=function<void(const char *,uint32_t)>;
/**
 * @brief The buffer_handle class 用于处理网络IO的数据接收与发送
 */
class buffer_handle{
protected:
    /**
     * @brief MAX_BUFFER_CAPACITY 最大buffer块容量
     */
    static constexpr uint32_t MAX_BUFFER_CAPACITY=2048;
    /**
     * @brief MIN_BUFFER_CAPACITY 最小buffer块容量
     */
    static constexpr uint32_t MIN_BUFFER_CAPACITY=2;
    /**
     * @brief MAX_PACKET_SIZE 单帧最大长度
     */
    static constexpr uint32_t MAX_PACKET_SIZE=1024*1024;
    /**
     * @brief PER_READ_SIZE 分片长度
     */
    static constexpr uint32_t PER_READ_SIZE=4096;
public:
    /**
     * @brief buffer_handle 构造函数
     * @param capacity  指定缓冲区最大长度
     * @param callback 读回调函数
     * @param is_stream 是否按流输入
     * @param per_read_size  单个数据包长度
     */
    buffer_handle(uint32_t capacity,ReadBufferCallback callback=nullptr,bool is_stream=true,uint32_t per_read_size=PER_READ_SIZE);
    virtual ~buffer_handle(){}
    /**
     * @brief read_fd 从fd中读取数据
     * @param fd
     * @return 返回实际读取到的长度 失败返回-1
     */
    int read_fd(SOCKET fd);
    /**
     * @brief send_fd 以buffer作为输入用fd发送数据
     * @param fd
     * @param addr 未调用connect的UDP socket需输入addr指定接收地址
     * @param timeout 发送超时时间
     * @return 实际发送长度 -1代表发送失败
     */
    int send_fd(SOCKET fd,sockaddr_in *addr=nullptr,int timeout=0);
    /**
     * @brief append 追加数据到缓存中
     * @param buf
     * @param buf_len
     * @return
     */
    bool append(const void *buf ,uint32_t buf_len);
    /**
     * @brief get_packet_nums 获取可读帧的最大数目
     * @return
     */
    uint32_t get_packet_nums()const;
    /**
     * @brief get_first_packet_size 获取第一帧的长度
     * @return
     */
    uint32_t get_first_packet_size()const;
    /**
     * @brief read_packet 从缓冲区中读取一帧
     * @param save_buf 外部传入保存帧数据的缓冲
     * @param buf_len 缓冲区长度
     * @return 实际读取到的长度
     */
    uint32_t read_packet(void *save_buf,uint32_t buf_len);
protected:
    /**
     * @brief insert_packet 向链表中插入数据
     * @param buf
     * @param buf_len
     * @return
     */
    virtual bool insert_packet(const char *buf,uint32_t buf_len);
protected:
    struct BufferPacket{
    /*数据存放区域*/
       shared_ptr<char> buf;
       /*数据区域长度*/
       uint32_t buf_len;
       /*写入位置*/
       uint32_t write_index;
       /*读取位置*/
       uint32_t read_index;
       /*是否写入完毕*/
       bool is_finished;
       BufferPacket(uint32_t len):buf(new char[len],std::default_delete<char[]>()),buf_len(len),write_index(0),read_index(0),is_finished(false){}
       /*可用长度*/
       uint32_t availiable_size()const{return buf_len-write_index;}
       /*可读长度*/
       uint32_t filled_size()const{return write_index-read_index;}
       /*读指针起始地址*/
        char *read_ptr() {return buf.get()+read_index;}
        /*写指针起始地址*/
        char *write_ptr(){return buf.get()+write_index;}
        /*重置*/
       void reset(){write_index=0;read_index=0;is_finished=false;}
       /*向后移动读指针len长度*/
       void retrieve(uint32_t len){
           if(len+read_index>=write_index)reset();
           else {
               read_index+=len;
           }
       }
       /*读取指定长度*/
       uint32_t  read_packet(char *save_buf,uint32_t max_size){
           auto read_len=filled_size();
           if(max_size<read_len)read_len=max_size;
           if(read_len>0){
               memcpy(save_buf,read_ptr(),read_len);
               retrieve(read_len);
           }
           return read_len;
       }
       /*设置完成状态*/
       void set_finished(){is_finished=true;}
       /*获取完成状态*/
       bool finished()const{return is_finished;}
       /*追加数据*/
       bool append(const char *input_buf,uint32_t input_len){
           if(input_len<=availiable_size()){
               memcpy(write_ptr(),input_buf,input_len);
               write_index+=input_len;
           }
           else if (filled_size()+input_len>MAX_PACKET_SIZE) return false;
           else {
               buf_len=filled_size()+input_len;
               char *new_buf=new char[buf_len];
               memcpy(new_buf,read_ptr(),filled_size());
               memcpy(new_buf+filled_size(),input_buf,input_len);
               read_index=0;
               write_index=buf_len;
               buf.reset(new_buf,std::default_delete<char[]>());
           }
           return true;
       }
    };
    mutable mutex m_mutex;
    /**
     * @brief m_callback 读回调函数
     */
    ReadBufferCallback m_callback;
    /**
     * @brief m_per_read_size 单次读取最大长度
     */
    uint32_t m_per_read_size;
    /**
     * @brief m_capacity 缓存区最大长度
     */
    uint32_t m_capacity;
    /**
     * @brief m_packet_list 帧缓存链表
     */
    list<BufferPacket> m_packet_list;
    /**
     * @brief m_is_stream 是否为按流输入标识
     */
    bool m_is_stream;
private:
    /**
      * @brief m_crlf 帧分隔符示例 \r\n\r\n\r\n
      */
     static  constexpr const  char *m_crlf="\r\n\r\n";
     /**
     * @brief m_crlf_len 帧分隔符示例长度
     */
    constexpr const static uint8_t m_crlf_len=4;
};
}
#endif // BUFFER_HANDLE_H
