#include "proxy_protocol.h"
using namespace micagent;

static inline char *m_encode8u(char *p, uint8_t c)
{
    *(unsigned char*)p++ = c;
    return p;
}

static inline const char *m_decode8u(const char *p, uint8_t *c)
{
    *c = *(unsigned char*)p++;
    return p;
}

static inline char *m_encode16u(char *p, uint16_t w)
{
#if IWORDS_BIG_ENDIAN
    *(unsigned char*)(p + 0) = (w & 255);
    *(unsigned char*)(p + 1) = (w >> 8);
#else
    *(unsigned short*)(p) = w;
#endif
    p += 2;
    return p;
}

static inline const char *m_decode16u(const char *p, uint16_t *w)
{
#if IWORDS_BIG_ENDIAN
    *w = *(const unsigned char*)(p + 1);
    *w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
    *w = *(const unsigned short*)p;
#endif
    p += 2;
    return p;
}

static inline char *m_encode32u(char *p, uint32_t l)
{
#if IWORDS_BIG_ENDIAN
    *(unsigned char*)(p + 0) = (unsigned char)((l >>  0) & 0xff);
    *(unsigned char*)(p + 1) = (unsigned char)((l >>  8) & 0xff);
    *(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
    *(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
    *(uint32_t*)p = l;
#endif
    p += 4;
    return p;
}

static inline const char *m_decode32u(const void *p, uint32_t *l)
{
#if IWORDS_BIG_ENDIAN
    *l = *(const unsigned char*)(p + 3);
    *l = *(const unsigned char*)(p + 2) + (*l << 8);
    *l = *(const unsigned char*)(p + 1) + (*l << 8);
    *l = *(const unsigned char*)(p + 0) + (*l << 8);
#else
    *l = *(const uint32_t*)p;
#endif
    return (const char*)(p)+4;
}
static inline void decode_proxy_header( ProxyHeader&header,const char *buf){
    buf=m_decode8u(buf,&header.check_byte);
    buf=m_decode8u(buf,&header.media_type);
    buf=m_decode8u(buf,&header.media_channel);
    buf=m_decode8u(buf,&header.frame_type);
    buf=m_decode32u(buf,&header.stream_token);
    buf=m_decode32u(buf,&header.frame_len);
    buf=m_decode16u(buf,&header.frame_seq);
    buf=m_decode16u(buf,&header.fragment_count);
    buf=m_decode16u(buf,&header.fragment_seq);
    buf=m_decode16u(buf,&header.data_len);
    m_decode32u(buf,&header.timestamp);
}
static inline void encode_proxy_header( ProxyHeader&header, char *buf){
    buf=m_encode8u(buf,header.check_byte);
    buf=m_encode8u(buf,header.media_type);
    buf=m_encode8u(buf,header.media_channel);
    buf=m_encode8u(buf,header.frame_type);
    buf=m_encode32u(buf,header.stream_token);
    buf=m_encode32u(buf,header.frame_len);
    buf=m_encode16u(buf,header.frame_seq);
    buf=m_encode16u(buf,header.fragment_count);
    buf=m_encode16u(buf,header.fragment_seq);
    buf=m_encode16u(buf,header.data_len);
    m_encode32u(buf,header.timestamp);
}
ProxyInterface::ProxyInterface(uint32_t token,PTransMode mode,POUTPUT_CALLBACK tcp_callback,POUTPUT_CALLBACK udp_callback,PFRAME_CALLBACK recv_callback)
    :m_stream_token(token),m_mode(mode),m_tcp_callback(tcp_callback),m_udp_callback(udp_callback),m_recv_callback(recv_callback)\
    ,m_last_send_frame_seq(P_INVALID_SEQ),m_last_wait_confirmed_frame_seq(P_INVALID_SEQ),m_last_confirmed_flush_frame_seq(P_INVALID_SEQ)\
    ,m_min_recv_frame_seq(P_INVALID_SEQ){

}
bool ProxyInterface::send_frame(shared_ptr<ProxyFrame> frame)
{
#if FRAGMENT_CACHE_CHECK
    {
        lock_guard<mutex>locker(m_mutex);
        if(m_packet_chache.size()>MAX_FRAGMENT_CACHE_SIZE){
            while (!m_packet_chache.empty()) {
                m_packet_chache.pop();
            }
        }
    }
#endif
    switch (m_mode) {
    case RAW_TCP:
        return raw_tcp_handle_packet(frame);
    case RAW_UDP:
        return raw_udp_handle_packet(frame);
    case RAW_HYBRID:
        return raw_hybrid_handle_packet(frame);
    case GRADED_TCP:
        return graded_tcp_handle_packet(frame);
    case GRADED_HYBRID:
        return graded_hybrid_handle_packet(frame);
    }
}

bool ProxyInterface::protocol_input(const void *buf,uint32_t buf_len)
{
    const char *tmp=static_cast<const char *>(buf);
    //check
    if(tmp[0]!='$'||buf_len<(sizeof (ProxyHeader))){
        return false;
    }
    ProxyHeader header;
    decode_proxy_header(header,tmp);
    {
        lock_guard<mutex>locker(m_mutex);
        if(header.frame_type==FRAME_ACK){
            m_last_confirmed_flush_frame_seq=header.frame_seq;
            return true;
        }
        else {
            if(header.data_len==0)return true;
            auto iter=m_recv_frame_chache.find(header.frame_seq);
            if(iter==m_recv_frame_chache.end()){
                if(circle_compare(m_min_recv_frame_seq,header.frame_seq)){
                    //符合插入条件
                    auto iter2=m_recv_frame_chache.emplace(header.frame_seq,make_shared<ProxyFrameCache>(header));
                    if(iter2.second){
                        iter=iter2.first;
                    }
                    else {
                        //插入失败
                        return false;
                    }
                }
                else {
                    //过期数据
                    return false;
                }
            }
            iter->second->insert_fragment(header,tmp+sizeof (header));
            if(!iter->second->is_finished()){
                //网络质量很差的时候，清空缓存帧，避免内存耗用
                if(m_recv_frame_chache.size()>=MAX_WINDOW_SIZE)m_recv_frame_chache.clear();
                return true;
            }
        }
    }
    switch (m_mode) {
    case RAW_TCP:
        raw_tcp_handle_packet(header.frame_seq);
        break;
    case RAW_UDP:
        raw_udp_handle_packet(header.frame_seq);
        break;
    case RAW_HYBRID:
        raw_hybrid_handle_packet(header.frame_seq);
        break;
    case GRADED_TCP:
        graded_tcp_handle_packet(header.frame_seq);
        break;
    case GRADED_HYBRID:
        graded_hybrid_handle_packet(header.frame_seq);
        break;
    }
    return true;
}
void ProxyInterface::response_frame(uint16_t seq)
{
    if(!m_tcp_callback)return;
    shared_ptr<ProxyFragmentPacket> send_packet(new ProxyFragmentPacket(sizeof (ProxyHeader)));
    m_encode32u(send_packet->buf.get()+POFFSETOF(ProxyHeader,check_byte),'$');
    m_encode32u(send_packet->buf.get()+POFFSETOF(ProxyHeader,stream_token),m_stream_token);
    m_encode8u(send_packet->buf.get()+POFFSETOF(ProxyHeader,frame_type),FRAME_ACK);
    m_encode16u(send_packet->buf.get()+POFFSETOF(ProxyHeader,frame_seq),seq);
    m_encode16u(send_packet->buf.get()+POFFSETOF(ProxyHeader,data_len),0);
    m_tcp_callback(send_packet->buf.get(),send_packet->data_len);
}
bool ProxyInterface::raw_tcp_handle_packet(shared_ptr<ProxyFrame> frame)
{

    unique_lock<mutex>locker(m_mutex);
    if(!m_tcp_callback)return false;

    ProxyHeader header;
    header.check_byte='$';
    header.frame_len=frame->data_len;
    header.frame_seq=m_last_send_frame_seq++;
    header.timestamp=frame->timestamp;
    header.frame_type=get_frame_type(frame->media_type,frame->data_buf.get());
    header.media_type=frame->media_type;
    header.stream_token=m_stream_token;
    header.media_channel=frame->media_channel;
    header.fragment_count=(frame->data_len+P_FRAME_OFFSET)/PROXY_FRAGMENT_SIZE;
    for(uint16_t i=0;i<header.fragment_count;i++)
    {
        header.fragment_seq=i;
        header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
        shared_ptr<char[]>send_buf(new char[P_HEADER_SIZE+header.data_len+1]);
        encode_proxy_header(header,send_buf.get());
        memcpy(send_buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
        if(locker.owns_lock())locker.unlock();
        if(!m_tcp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
        if(!locker.owns_lock())locker.lock();
    }
    return true;
}
bool ProxyInterface::raw_udp_handle_packet(shared_ptr<ProxyFrame> frame)
{
    unique_lock<mutex>locker(m_mutex);
    if(!m_udp_callback)return false;
    ProxyHeader header;
    header.check_byte='$';
    header.frame_len=frame->data_len;
    header.frame_seq=m_last_send_frame_seq++;
    header.timestamp=frame->timestamp;
    header.frame_type=get_frame_type(frame->media_type,frame->data_buf.get());
    header.media_type=frame->media_type;
    header.stream_token=m_stream_token;
    header.media_channel=frame->media_channel;
    header.fragment_count=(frame->data_len+P_FRAME_OFFSET)/PROXY_FRAGMENT_SIZE;
    for(uint16_t i=0;i<header.fragment_count;i++)
    {
        header.fragment_seq=i;
        header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
        shared_ptr<char[]>send_buf(new char[P_HEADER_SIZE+header.data_len+1]);
        encode_proxy_header(header,send_buf.get());
        memcpy(send_buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
        if(locker.owns_lock())locker.unlock();
        if(!m_udp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
        if(!locker.owns_lock())locker.lock();
    }
    return true;
}
bool ProxyInterface::raw_hybrid_handle_packet(shared_ptr<ProxyFrame> frame)
{
    unique_lock<mutex>locker(m_mutex);
    if(!m_udp_callback)return false;
    ProxyHeader header;
    header.check_byte='$';
    header.frame_len=frame->data_len;
    header.frame_seq=m_last_send_frame_seq++;
    header.timestamp=frame->timestamp;
    header.frame_type=get_frame_type(frame->media_type,frame->data_buf.get());
    header.media_type=frame->media_type;
    header.stream_token=m_stream_token;
    header.media_channel=frame->media_channel;
    header.fragment_count=(frame->data_len+P_FRAME_OFFSET)/PROXY_FRAGMENT_SIZE;
    for(uint16_t i=0;i<header.fragment_count;i++)
    {
        header.fragment_seq=i;
        header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
        shared_ptr<char[]>send_buf(new char[P_HEADER_SIZE+header.data_len+1]);
        encode_proxy_header(header,send_buf.get());
        memcpy(send_buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
        if(locker.owns_lock())locker.unlock();
        if(header.frame_type==NORMAL_FRAME){
            if(!m_udp_callback||!m_udp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
        }
        else {
            if(!m_tcp_callback||!m_tcp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
        }
        if(!locker.owns_lock())locker.lock();
    }
    return true;
}
bool ProxyInterface::graded_tcp_handle_packet(shared_ptr<ProxyFrame> frame)
{
    if(!m_tcp_callback)return false;
    unique_lock<mutex>locker(m_mutex);
    ProxyHeader header;
    header.check_byte='$';
    header.frame_len=frame->data_len;
    header.frame_seq=m_last_send_frame_seq++;
    header.timestamp=frame->timestamp;
    header.frame_type=get_frame_type(frame->media_type,frame->data_buf.get());
    header.media_type=frame->media_type;
    header.stream_token=m_stream_token;
    header.media_channel=frame->media_channel;
    header.fragment_count=(frame->data_len+P_FRAME_OFFSET)/PROXY_FRAGMENT_SIZE;
    if(header.frame_type==FLUSH_FRAME){
        //刷新帧，清空缓存
#ifdef DEBUG
        if(!m_packet_chache.empty())printf("graded_tcp send clear cache frames counts %lu \r\n",m_packet_chache.size());
#endif
        while(!m_packet_chache.empty()){
            m_packet_chache.pop();
        }
        m_last_wait_confirmed_frame_seq=header.frame_seq;
        for(uint16_t i=0;i<header.fragment_count;i++)
        {
            header.fragment_seq=i;
            header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
            shared_ptr<char[]>send_buf(new char[P_HEADER_SIZE+header.data_len+1]);
            encode_proxy_header(header,send_buf.get());
            memcpy(send_buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
            if(locker.owns_lock())locker.unlock();
            if(!m_tcp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
            if(!locker.owns_lock())locker.lock();
        }
    }
    else {
        //非刷新帧，检查是否可发送
        if(m_last_wait_confirmed_frame_seq==m_last_confirmed_flush_frame_seq){
            //刷新帧已确认
            while(!m_packet_chache.empty()){
                auto frame_cache=m_packet_chache.front();
                m_packet_chache.pop();
                if(locker.owns_lock())locker.unlock();
                m_tcp_callback(frame_cache.get()->buf.get(),frame_cache->data_len);
                if(!locker.owns_lock())locker.lock();
            }
            for(uint16_t i=0;i<header.fragment_count;i++)
            {
                header.fragment_seq=i;
                header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
                shared_ptr<char[]>send_buf(new char[P_HEADER_SIZE+header.data_len+1]);
                encode_proxy_header(header,send_buf.get());
                memcpy(send_buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
                if(locker.owns_lock())locker.unlock();
                if(!m_tcp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
                if(!locker.owns_lock())locker.lock();
            }
        }
        else {
            //刷新帧待确认，将当前帧加入缓存
#ifdef DEBUG
            //printf("graded_tcp send add cache frames %u \r\n",header.frame_seq);
#endif
            for(uint16_t i=0;i<header.fragment_count;i++)
            {
                header.fragment_seq=i;
                header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
                shared_ptr<ProxyFragmentPacket>packet(new ProxyFragmentPacket(P_HEADER_SIZE+header.data_len));
                encode_proxy_header(header,packet->buf.get());
                memcpy(packet->buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
                m_packet_chache.push(move(packet));
            }
        }
    }
    return true;
}
bool ProxyInterface::graded_hybrid_handle_packet(shared_ptr<ProxyFrame> frame)
{
    if(!m_tcp_callback)return  false;
    unique_lock<mutex>locker(m_mutex);
    ProxyHeader header;
    header.check_byte='$';
    header.frame_len=frame->data_len;
    header.frame_seq=m_last_send_frame_seq++;
    header.timestamp=frame->timestamp;
    header.frame_type=get_frame_type(frame->media_type,frame->data_buf.get());
    header.media_type=frame->media_type;
    header.stream_token=m_stream_token;
    header.media_channel=frame->media_channel;
    header.fragment_count=(frame->data_len+P_FRAME_OFFSET)/PROXY_FRAGMENT_SIZE;
    if(header.frame_type==FLUSH_FRAME){
        //刷新帧，清空缓存
#ifdef DEBUG
        if(!m_packet_chache.empty())printf("graded_hybrid send clear cache frames counts %lu \r\n",m_packet_chache.size());
#endif
        while(!m_packet_chache.empty())
        {
            m_packet_chache.pop();
        }
        m_last_wait_confirmed_frame_seq=header.frame_seq;
        for(uint16_t i=0;i<header.fragment_count;i++)
        {
            header.fragment_seq=i;
            header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
            shared_ptr<char[]>send_buf(new char[P_HEADER_SIZE+header.data_len+1]);
            encode_proxy_header(header,send_buf.get());
            memcpy(send_buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
            if(locker.owns_lock())locker.unlock();
            if(!m_tcp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
            if(!locker.owns_lock())locker.lock();
        }
    }
    else {
        if(!m_udp_callback)return  false;
        //非刷新帧，检查是否可发送
        if(m_last_wait_confirmed_frame_seq==m_last_confirmed_flush_frame_seq){
            //刷新帧已确认
            while(!m_packet_chache.empty()){
                auto frame_cache=m_packet_chache.front();
                m_packet_chache.pop();
                if(locker.owns_lock())locker.unlock();
                if(frame_cache->using_tcp)m_tcp_callback(frame_cache.get()->buf.get(),frame_cache->data_len);
                else {
                    m_udp_callback(frame_cache.get()->buf.get(),frame_cache->data_len);
                }
                if(!locker.owns_lock())locker.lock();
            }
            for(uint16_t i=0;i<header.fragment_count;i++)
            {
                header.fragment_seq=i;
                header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
                shared_ptr<char[]>send_buf(new char[P_HEADER_SIZE+header.data_len+1]);
                encode_proxy_header(header,send_buf.get());
                memcpy(send_buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
                if(locker.owns_lock())locker.unlock();
                if(header.frame_type==NORMAL_FRAME){
                    if(!m_udp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
                }
                else {
                    if(!m_tcp_callback(send_buf.get(),P_HEADER_SIZE+header.data_len))return false;
                }
                if(!locker.owns_lock())locker.lock();
            }
        }
        else {
            //刷新帧待确认，将当前帧加入缓存
#ifdef DEBUG
            //printf("graded_hybrid send add cache frames %u \r\n",header.frame_seq);
#endif
            bool _tcp=header.frame_type!=NORMAL_FRAME;
            for(uint16_t i=0;i<header.fragment_count;i++)
            {
                header.fragment_seq=i;
                header.data_len=(i==header.fragment_count-1)?(frame->data_len-PROXY_FRAGMENT_SIZE*i):PROXY_FRAGMENT_SIZE;
                shared_ptr<ProxyFragmentPacket>packet(new ProxyFragmentPacket(P_HEADER_SIZE+header.data_len,_tcp));
                encode_proxy_header(header,packet->buf.get());
                memcpy(packet->buf.get()+P_HEADER_SIZE,frame->data_buf.get()+header.fragment_seq*PROXY_FRAGMENT_SIZE,header.data_len);
                m_packet_chache.push(move(packet));
            }
        }
    }
    return true;
}

void ProxyInterface::raw_tcp_handle_packet(uint16_t seq)
{
    //每接收一帧都将其直接返回给应用层
    unique_lock<mutex>locker(m_mutex);
    auto iter=m_recv_frame_chache.find(seq);
    if(iter!=m_recv_frame_chache.end()){
        auto &cache= iter->second;
        shared_ptr<ProxyFrame>frame(new ProxyFrame(cache->m_buf_chache.get(),cache->frame_len,cache->media_type,cache->media_channel,m_stream_token,cache->frame_seq));
        m_recv_frame_chache.clear();
        m_min_recv_frame_seq=seq;
        locker.unlock();
        if(m_recv_callback)m_recv_callback(frame);
    }
}
void ProxyInterface::raw_udp_handle_packet(uint16_t seq)
{
    unique_lock<mutex>locker(m_mutex);
    for(auto iter=m_recv_frame_chache.begin();iter!=m_recv_frame_chache.end();)
    {
        if(iter->second->frame_seq>seq)break;
        auto seq_diff=seq-iter->second->frame_seq;
        if(seq_diff>UINT16_MAX/2){
            //比当前接收帧序号靠后的帧
            iter++;
        }
        else{
            if(seq_diff>MAX_ORDER_CACHE_SIZE){
                //移除无效帧
#ifdef DEBUG
                printf("******************raw_udp recv  erase cache frames %u *****%d**********\r\n",iter->second->frame_seq,iter->second->fragment_count-iter->second->m_fragment_seq_set.size());
#endif
                m_recv_frame_chache.erase(iter++);
            }
            else {
                if(!iter->second->is_finished()){
                    //接收未完全的缓存
                    break;
                }
                else {
                    //接收完成直接返回应用层
                    m_min_recv_frame_seq=iter->second->frame_seq;
                    auto &cache= iter->second;
                    shared_ptr<ProxyFrame>frame(new ProxyFrame(cache->m_buf_chache.get(),cache->frame_len,cache->media_type,cache->media_channel,m_stream_token,cache->frame_seq));
                    m_recv_frame_chache.erase(iter++);
                    locker.unlock();
                    if(m_recv_callback)m_recv_callback(frame);
                    locker.lock();
                }
            }
        }
    }
}
void ProxyInterface::raw_hybrid_handle_packet(uint16_t seq)
{
    unique_lock<mutex>locker(m_mutex);
    auto tmp=m_recv_frame_chache.find(seq);
    if(tmp==m_recv_frame_chache.end())return;
    for(auto iter=m_recv_frame_chache.begin();iter!=m_recv_frame_chache.end();)
    {
        if(iter->second->frame_seq>seq)break;
        auto seq_diff=seq-iter->second->frame_seq;
        if(seq_diff>UINT16_MAX/2){
            //比当前接收帧序号靠后的帧
            iter++;
        }
        else{
            if(seq_diff>MAX_ORDER_CACHE_SIZE){
                //移除无效帧
#ifdef DEBUG
                printf("******************raw_hybrid recv  erase cache frames %u *****%d**********\r\n",iter->second->frame_seq,iter->second->fragment_count-iter->second->m_fragment_seq_set.size());
#endif
                m_recv_frame_chache.erase(iter++);
            }
            else {
                if(!iter->second->is_finished()){
                    //接收未完全的缓存
                    break;
                }
                else {
                    //接收完成直接返回应用层
                    m_min_recv_frame_seq=iter->second->frame_seq;
                    auto &cache= iter->second;
                    shared_ptr<ProxyFrame>frame(new ProxyFrame(cache->m_buf_chache.get(),cache->frame_len,cache->media_type,cache->media_channel,m_stream_token,cache->frame_seq));
                    m_recv_frame_chache.erase(iter++);
                    locker.unlock();
                    if(m_recv_callback)m_recv_callback(frame);
                    locker.lock();
                }
            }
        }
    }
}
void ProxyInterface::graded_tcp_handle_packet(uint16_t seq)
{
    unique_lock<mutex>locker(m_mutex);
    auto tmp=m_recv_frame_chache.find(seq);
    if(tmp==m_recv_frame_chache.end())return;
    auto &cache= tmp->second;
    shared_ptr<ProxyFrame>frame(new ProxyFrame(cache->m_buf_chache.get(),cache->frame_len,cache->media_type,cache->media_channel,m_stream_token,cache->frame_seq));
    m_recv_frame_chache.erase(tmp);
    m_min_recv_frame_seq=seq;
    auto type=tmp->second->frame_type;
    locker.unlock();
    if(m_recv_callback)m_recv_callback(frame);
    if(type==FLUSH_FRAME){
        response_frame(seq);
    }

}
void ProxyInterface::graded_hybrid_handle_packet(uint16_t seq)
{
    unique_lock<mutex>locker(m_mutex);
    auto tmp=m_recv_frame_chache.find(seq);
    if(tmp==m_recv_frame_chache.end())return;
    if(tmp->second->frame_type==FLUSH_FRAME){
        locker.unlock();
        response_frame(seq);
        locker.lock();
    }
    for(auto iter=m_recv_frame_chache.begin();iter!=m_recv_frame_chache.end();)
    {
        if(iter->second->frame_seq>seq)break;
        auto seq_diff=seq-iter->second->frame_seq;
        if(seq_diff>UINT16_MAX/2){
            //比当前接收帧序号靠后的帧
            iter++;
        }
        else{
            if(seq_diff>MAX_ORDER_CACHE_SIZE){
                //移除无效帧
#ifdef DEBUG
                printf("******************graded_hybrid recv  erase cache frames %u *****%d**********\r\n",iter->second->frame_seq,iter->second->fragment_count-iter->second->m_fragment_seq_set.size());
#endif
                m_recv_frame_chache.erase(iter++);
            }
            else {
                if(!iter->second->is_finished()){
                    //接收未完全的缓存
                    break;
                }
                else {
                    //接收完成直接返回应用层
                    m_min_recv_frame_seq=iter->second->frame_seq;
                    auto &cache= iter->second;
                    shared_ptr<ProxyFrame>frame(new ProxyFrame(cache->m_buf_chache.get(),cache->frame_len,cache->media_type,cache->media_channel,m_stream_token,cache->frame_seq));
                    m_recv_frame_chache.erase(iter++);
                    locker.unlock();
                    if(m_recv_callback)m_recv_callback(frame);
                    locker.lock();
                }
            }
        }
    }
}
