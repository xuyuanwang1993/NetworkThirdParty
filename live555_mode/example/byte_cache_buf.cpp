#include "byte_cache_buf.h"
#include <cstdio>
#include<string.h>
#include<cstdlib>
using namespace micagent;

Byte_Cache_Buf::Byte_Cache_Buf(uint32_t index,uint32_t buf_size):m_index(index),m_buf_size(buf_size),\
    m_access_id(0),m_read_offset(0),m_write_offset(0),m_cache_size(DEFAULT_CACHE_SIZE),m_capacity(DEFAULT_CAPACITY),\
    m_is_working(true)
{
    if(m_buf_size>MAX_BUF_SIZE){
#if BYTE_CACHE_BUF_DEBUG
        printf("buf %d buf_size %u overflow  for max_size %u!\r\n",m_index,m_buf_size,MAX_BUF_SIZE);
#endif
        m_buf_size=MAX_BUF_SIZE;
    }
    m_cache_buf.reset(new uint8_t[m_buf_size],std::default_delete<uint8_t[]>());
}
Byte_Cache_Buf::~Byte_Cache_Buf()
{
    m_is_working.exchange(false);
    m_cv.notify_all();
}
bool Byte_Cache_Buf::insert_buf(const void *access_id,const void *buf,uint32_t buf_len,const struct timeval &timestamp,const void *adt_head,uint32_t head_len)
{
    if(!m_is_working){
#if BYTE_CACHE_BUF_DEBUG
        printf("buf %d is  not working!\r\n",m_index);
#endif
        return false;
    }
    bool ret=false;
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        //检查是否有可用的空间
        while(m_packet_queue.size()>m_capacity){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d packet_queue is full with max capacity %d,discard one packet!\r\n",m_index,m_capacity);
#endif
            //丢弃一帧
            m_read_offset+=m_packet_queue.front().buf_len;
            m_packet_queue.pop();
        }
        auto availiable_size=m_buf_size-(m_write_offset-m_read_offset);
        auto packet_len=buf_len+head_len;
        if(availiable_size<packet_len){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d packet_queue has not availiable space with available space(%u) for the packet_len(%u)!\r\n",m_index,availiable_size,packet_len);
#endif
            break;
        }
        //判断数据写入是否需要移动缓冲区
        if(m_write_offset+packet_len>m_buf_size){
            memmove(m_cache_buf.get(),m_cache_buf.get()+m_read_offset,m_buf_size-availiable_size);
            m_read_offset=0;
            m_write_offset=m_buf_size-availiable_size;
        }
        //写入数据
        if(adt_head&&head_len>0){
            memcpy(m_cache_buf.get()+m_write_offset,adt_head,head_len);
            m_write_offset+=head_len;
        }
        if(buf&&buf_len>0){
            memcpy(m_cache_buf.get()+m_write_offset,buf,buf_len);
            m_write_offset+=buf_len;
        }
        m_packet_queue.push(packet_info(timestamp,packet_len));
        m_cv.notify_one();
        ret =true;
    }while(0);
    return ret;
}

bool Byte_Cache_Buf::get_packet_block(const void *access_id, void *buf,uint32_t &buf_len,struct timeval *timestamp)
{
    if(!m_is_working){
#if BYTE_CACHE_BUF_DEBUG
        printf("buf %d is  not working!\r\n",m_index);
#endif
        return false;
    }
    do{
        unique_lock<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        while(1)
        {
            m_cv.wait(locker,[&](){return m_packet_queue.size()>m_cache_size||!m_is_working;});
            if(!m_is_working){
                return false;
            }
            if(m_packet_queue.size()>m_cache_size)
            {
                auto &front=m_packet_queue.front();
                if(buf_len<front.buf_len)
                {
                    memcpy(buf,m_cache_buf.get()+m_read_offset,buf_len);
                    m_read_offset+=buf_len;
                    front.buf_len-=buf_len;
                    if(timestamp)*timestamp=front.timestamp;
                }
                else {
                    memcpy(buf,m_cache_buf.get()+m_read_offset,front.buf_len);
                    m_read_offset+=front.buf_len;
                    buf_len=front.buf_len;
                    if(timestamp)*timestamp=front.timestamp;
                    m_packet_queue.pop();
                }
                break;
            }
        }
    }while(0);
    return true;
}
bool Byte_Cache_Buf::get_packet_for(const void *access_id, void *buf,uint32_t &buf_len,uint32_t micro_wait_time,struct timeval *timestamp)
{
    if(!m_is_working){
#if BYTE_CACHE_BUF_DEBUG
        printf("buf %d is  not working!\r\n",m_index);
#endif
        return false;
    }
    bool ret=false;
    do{
        unique_lock<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        if(micro_wait_time>0)m_cv.wait_for(locker,std::chrono::microseconds(micro_wait_time),[&](){
        return m_packet_queue.size()>m_cache_size||!m_is_working;});
        if(!m_is_working){
            break;
        }
        if(m_packet_queue.size()>m_cache_size)
        {
            auto &front=m_packet_queue.front();
            if(buf_len<front.buf_len)
            {
                memcpy(buf,m_cache_buf.get()+m_read_offset,buf_len);
                m_read_offset+=buf_len;
                front.buf_len-=buf_len;
                if(timestamp)*timestamp=front.timestamp;
            }
            else {
                memcpy(buf,m_cache_buf.get()+m_read_offset,front.buf_len);
                m_read_offset+=front.buf_len;
                buf_len=front.buf_len;
                if(timestamp)*timestamp=front.timestamp;
                m_packet_queue.pop();
            }
        }
        else {
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d get_packet_for time out!\r\n",m_index);
#endif
            break;
        }
        ret =true;
    }while(0);
    return ret;
}

bool Byte_Cache_Buf::bind(const void *access_id)
{
    bool ret=false;
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_access_id){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu is set,try call release first!\r\n",m_index,reinterpret_cast<uint64_t>(access_id));
#endif
            break;
        }
        m_access_id=reinterpret_cast<uint64_t>(access_id);
        ret =true;
    }while(0);
    return ret;
}
bool Byte_Cache_Buf::release(const void *access_id)
{
    bool ret=false;
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        m_access_id=0;
        ret =true;
    }while(0);
    return ret;
}
uint64_t Byte_Cache_Buf::get_access_id()const
{
    lock_guard<mutex>locker(m_mutex);
    return m_access_id;
}
void Byte_Cache_Buf::reset(const void *access_id)
{
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        m_read_offset=0;
        m_write_offset=0;
        m_is_working.exchange(false);
        m_capacity=DEFAULT_CAPACITY;
        m_cache_size=DEFAULT_CACHE_SIZE;
        while(!m_packet_queue.empty())m_packet_queue.pop();
        //唤醒其对象所有者
        m_cv.notify_all();
    }while(0);
}

void Byte_Cache_Buf::set_param(const void *access_id,uint32_t capacity,uint32_t cache_size)
{
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        m_capacity=capacity;
        m_cache_size=cache_size;
    }while(0);
}

void Byte_Cache_Buf::paused(const void *access_id)
{
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        m_read_offset=0;
        m_write_offset=0;
        m_is_working.exchange(false);
        while(!m_packet_queue.empty())m_packet_queue.pop();
        m_cv.notify_all();
    }while(0);
}
void Byte_Cache_Buf::started(const void *access_id)
{
    do{
        lock_guard<mutex>locker(m_mutex);
        if(m_access_id&&m_access_id!=reinterpret_cast<uint64_t>(access_id)){
#if BYTE_CACHE_BUF_DEBUG
            printf("buf %d access_id %lu mismatch  with saved %lu!\r\n",m_index,reinterpret_cast<uint64_t>(access_id),m_access_id);
#endif
            break;
        }
        m_is_working.exchange(true);
    }while(0);
}
