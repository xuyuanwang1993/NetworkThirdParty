#ifndef BYTE_CACHE_BUF_H
#define BYTE_CACHE_BUF_H
//compile with std=c++11
#include <queue>
#include<mutex>
#include<condition_variable>
#include<memory>
#include <sys/time.h>
#include<atomic>
#define BYTE_CACHE_BUF_DEBUG 1
namespace micagent {
using namespace std;
class Byte_Cache_Buf{
    static constexpr uint32_t DEFAULT_CACHE_SIZE=0;
    static constexpr uint32_t DEFAULT_CAPACITY=5;
    static constexpr uint32_t MAX_CACHE_SIZE=10000;
    static constexpr uint32_t MAX_BUF_SIZE=10*1024*1024;
    static constexpr uint32_t MAX_CAPACITY=MAX_CACHE_SIZE*2;
    struct  packet_info{
    //当前帧时间戳
        struct timeval timestamp;
    //当前帧长度
        uint32_t buf_len;
        packet_info(struct timeval _timestamp,uint32_t packet_len):timestamp(_timestamp),buf_len(packet_len){

        }
    };

public:
    /**
     * @brief Byte_Cache_Buf 构造函数
     * @param index 编号
     * @param buf_size 缓存区总长度
     */
    Byte_Cache_Buf(uint32_t index,uint32_t buf_size);
    /**
     * @brief Byte_Cache_Buf 禁止拷贝构造
     */
    Byte_Cache_Buf(const Byte_Cache_Buf &)=delete ;
    ~Byte_Cache_Buf();
    /**
     * @brief insert_buf 向缓存中插入数据
     * @param access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     * @param buf 原始数据指针
     * @param buf_len 原始数据长度
     * @param timestamp 当前数据时间戳
     * @param adt_head 额外需要添加的数据头指针
     * @param head_len 数据头长度
     * @return 缓冲区无法插入或使用对象不匹配时返回false 其余返回true
     */
    bool insert_buf(const void *access_id,const void *buf,uint32_t buf_len,const struct timeval &timestamp={0,0},const void *adt_head=nullptr,uint32_t head_len=0);
    /**
     * @brief get_packet_block 阻塞获取帧数据
     * @param access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     * @param buf 保存数据的指针
     * @param buf_len 最大读取长度,当此长度小于帧长度时，会将帧截断返回，剩余数据依旧缓存,调用成功后此数据修改为实际读取到的长度
     * @param timestamp 保存当前数据的时间戳
     * @return 无数据可读或使用对象不匹配时返回false，成功返回true
     */
    bool get_packet_block(const void *access_id, void *buf,uint32_t &buf_len,struct timeval *timestamp=nullptr);
    /**
     * @brief get_packet_for 超时获取数据
     * @param access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     * @param buf 保存数据的指针
     * @param buf_len 最大读取长度,当此长度小于帧长度时，会将帧截断返回，剩余数据依旧缓存,调用成功后此数据修改为实际读取到的长度
     * @param micro_wait_time 最大等待时间，为0时立即返回,精度us
     * @param timestamp 保存当前数据的时间戳
     * @return 未获取到数据或使用对象不匹配返回false，成功返回true
     */
    bool get_packet_for(const void *access_id, void *buf,uint32_t &buf_len,uint32_t micro_wait_time=0,struct timeval *timestamp=nullptr);
    /**
     * @brief bind 将buf绑定到一个对象上
     * @param access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     * @return 绑定成功返回true，失败返回false
     */
    bool bind(const void *access_id);
    /**
     * @brief release 解除buf上指定对象的绑定
     * @param  access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     * @return 解除成功返回true，失败返回false
     */
    bool release(const void *access_id);
    /**
     * @brief get_access_id 获取当前buf所属对象
     * @return 所属对象的access_id,未绑定则返回0
     */
    uint64_t get_access_id()const;
    /**
     * @brief reset 重置当前buf状态,未解除绑定
     * @param access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     */
    void reset(const void *access_id);
    /**
     * @brief set_param 设置buf缓存大小和容量大小
     * @param  access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     * @param capacity 容量
     * @param cache_size 缓存大小
     */
    void set_param(const void *access_id=nullptr,uint32_t capacity=DEFAULT_CAPACITY,uint32_t cache_size=DEFAULT_CACHE_SIZE);
    /**
     * @brief paused 停止处理数据，并清空数据缓存
     * @param access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     */
    void paused(const void *access_id);
    /**
     * @brief started 重新开始接受数据
     * @param access_id 使用时绑定的用户id，推荐传入使用对象的指针值
     */
    void started(const void *access_id);
    /**
     * @brief clear 清空缓存列表
     * @param access_id
     */
    void clear(const void *access_id);
    /**
     * @brief get_index 获取buf的索引
     * @return
     */

    uint32_t get_index()const{return m_index;}
    /**
     * @brief get_availiable_size 获取可用长度
     * @return
     */
    uint32_t get_availiable_size()const{
    lock_guard<mutex>locker(m_mutex);
    return m_buf_size-(m_write_offset-m_read_offset);}
    /**
     * @brief get_availiable_packet_nums 获取当前可以获取帧的总数目
     * @return
     */
    uint32_t get_availiable_packet_nums()const{
        lock_guard<mutex>locker(m_mutex);
        return  m_packet_queue.size()>m_cache_size?static_cast<uint32_t>(m_packet_queue.size()-m_cache_size):0;
    }
    /**
     * @brief get_working_status 获取buf是否正在工作
     * @return
     */
    bool get_working_status()const{return m_is_working;}
    /**
     * @brief get_total_recv_bytes 获取接收到的总字节数
     * @return
     */
    uint64_t get_total_recv_bytes()const{
        lock_guard<mutex>locker(m_mutex);
        return m_total_recv_size;
    }
    /**
     * @brief get_total_recv_cnt 获取接收到的帧的总数目
     * @return
     */
    uint64_t get_total_recv_cnt()const{
        lock_guard<mutex>locker(m_mutex);
        return m_total_recv_cnt;
    }
private:
    uint32_t m_index;
    //缓存buf
    shared_ptr<uint8_t> m_cache_buf;
    //缓冲区长度
    uint32_t m_buf_size;
    //绑定对象id
    uint64_t m_access_id;
    //数据锁
    mutable mutex m_mutex;
    condition_variable m_cv;
    //读指针偏移量
    uint32_t m_read_offset;
    //写指针偏移量
    uint32_t m_write_offset;
    //帧索引队列
    queue<packet_info> m_packet_queue;
    //缓存帧数目
    uint32_t m_cache_size;
    //帧容量大小
    uint32_t m_capacity;
    //判断buf是否可用标识
    atomic_bool m_is_working;
    //统计数据接收
    uint64_t m_total_recv_size;
    //统计数据接收数目
    uint64_t m_total_recv_cnt;
};
}
#endif // BYTE_CACHE_BUF_H
