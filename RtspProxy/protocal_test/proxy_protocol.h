#ifndef PROXY_PROTOCOL_H
#define PROXY_PROTOCOL_H
#include<inttypes.h>
#include<memory>
#include<string.h>
#include<functional>
#include<mutex>
#include<map>
#include<set>
#include<queue>
//计算某个成员的地址偏移量
#define POFFSETOF(TYPE, MEMBER) ( reinterpret_cast<uint64_t>(&(static_cast<TYPE*>(nullptr))->MEMBER))
#ifndef IWORDS_BIG_ENDIAN
#ifdef _BIG_ENDIAN_
#if _BIG_ENDIAN_
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#if defined(__hppa__) || \
    defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
    (defined(__MIPS__) && defined(__MIPSEB__)) || \
    defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
    defined(__sparc__) || defined(__powerpc__) || \
    defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
#define IWORDS_BIG_ENDIAN 1
#endif
#endif
#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN  0
#endif
#endif
#define PROXY_FRAGMENT_SIZE 1400
//分片缓存检查开关
#define FRAGMENT_CACHE_CHECK 1
namespace micagent {
using namespace  std;
typedef enum{
    FRAME_ACK,
    FLUSH_FRAME,
    KEY_FRAME,
    NORMAL_FRAME,
}PFrameType;
typedef  enum{
    PPCMA = 8,
    PH264 = 96,
    PAAC  = 37,
    PH265 = 265,
    PNONE
}PMediaTYpe;
typedef enum{
    /*
 * 此模式下发送不缓存，直接发送
 * 接收不发送确认不缓存,组包完成直接返回给应用层
 */
    RAW_TCP,
    /*
 * 此模式下发送不缓存，直接发送
 * 接收排序缓存5帧，接收出错的包丢弃
 */
    RAW_UDP,
    /*
 * 此模式下发送不缓存，直接发送
 * 刷新帧之后的帧缓存15帧
 */
    RAW_HYBRID,
    /*
 * 发送缓存，确认刷新帧 刷新帧到达会丢弃所有未发送数据
 * 接收组包完成直接返回应用层，收到刷新帧回复
 */
    GRADED_TCP,
    /* 发送缓存，确认刷新帧 刷新帧到达会丢弃所有未发送数据
 * 刷新帧之后的帧缓存15帧,确认刷新帧
*/
    GRADED_HYBRID,
}PTransMode;
struct p_timer_help{
    //取后32位
    static uint32_t getTimesTamp(){
        auto timePoint = std::chrono::steady_clock::now();
        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count());
    }
};

struct ProxyFrame{
    uint8_t media_type;
    uint8_t media_channel;
    uint32_t stream_token;
    uint32_t data_len;
    uint32_t timestamp;
    uint16_t frame_seq;
    shared_ptr<char[]>data_buf;
    ProxyFrame(const void *buf=nullptr,uint32_t len=0,uint8_t type=0,uint8_t channel=0,uint32_t token=0,uint16_t seq=0):\
        media_type(type),media_channel(channel),stream_token(token),data_len(len),timestamp(p_timer_help::getTimesTamp()),data_buf(new char[data_len+1]),frame_seq(seq){
        if(data_len>0&&buf)memcpy(data_buf.get(),buf,data_len);
    }
};
struct ProxyHeader{
    uint8_t check_byte;//$ 固定字符
    uint8_t media_type;//媒体类型
    uint8_t media_channel;//媒体通道
    uint8_t frame_type;//帧类型
    uint32_t stream_token;//流标识
    uint32_t frame_len;//当前帧总长度
    uint16_t frame_seq;//帧序列号
    uint16_t fragment_count;//分片总数
    uint16_t fragment_seq;//分片序列号 从0开始
    uint16_t data_len;//当前帧数据部分长度
    uint32_t timestamp;//发送时间戳
};
using POUTPUT_CALLBACK=function<bool(const void *,uint32_t buf_len)>;
using PFRAME_CALLBACK=function<void(shared_ptr<ProxyFrame>)>;
struct PStreamParse{
    shared_ptr<char[]>buf_cache;
    uint16_t last_buf_len;
    PStreamParse():buf_cache(new char[PROXY_FRAGMENT_SIZE+sizeof(ProxyHeader)+1]),last_buf_len(0){

    }
    void get_data_len(const void *p, uint16_t *l)
    {
#if IWORDS_BIG_ENDIAN
        *l = *(const unsigned char*)(p + 1);
        *l = *(const unsigned char*)(p + 0) + (*w << 8);
#else
        *l = *(const uint16_t*)p;
#endif
    }
    queue<pair<shared_ptr<char[]>,uint16_t>>insert_buf(const char *buf,uint16_t buf_len)
    {
        queue<pair<shared_ptr<char[]>,uint16_t>> ret;
        if(buf_len==0)return  ret;
        uint16_t new_data_len=last_buf_len+buf_len;
        shared_ptr<char>new_buf(new char[new_data_len]);
        if(last_buf_len>0)memcpy(new_buf.get(),buf_cache.get(),last_buf_len);
        memcpy(new_buf.get()+last_buf_len,buf,buf_len);
        if(new_buf.get()[0]!='$'){
#ifdef DEBUG
            printf("false stream input  %c %d!\r\n",new_buf.get()[0],new_buf.get()[0]);
#endif
            return ret;
        }
        uint16_t use_len=0;
        while(new_data_len>=sizeof (ProxyHeader))
        {
            get_data_len(new_buf.get()+use_len+POFFSETOF(ProxyHeader,data_len),&last_buf_len);
            last_buf_len+=sizeof (ProxyHeader);
            if(new_data_len>=last_buf_len)
            {
                shared_ptr<char []>data(new char[last_buf_len+1]);
                memcpy(data.get(),new_buf.get()+use_len,last_buf_len);
                ret.push(make_pair(data,last_buf_len));
                use_len+=last_buf_len;
                new_data_len-=last_buf_len;
            }
            else {
                break;
            }
        }
        last_buf_len=0;
        if(new_data_len>0)
        {
            last_buf_len=new_data_len;
            memcpy(buf_cache.get(),new_buf.get()+use_len,last_buf_len);
        }
        return ret;
    }
};

class ProxyInterface{
    static constexpr uint16_t MAX_ORDER_CACHE_SIZE=5;
    static constexpr uint16_t MAX_WINDOW_SIZE=MAX_ORDER_CACHE_SIZE*2;
    static constexpr uint16_t MAX_FRAGMENT_CACHE_SIZE=20000;
    static constexpr uint16_t MAX_WAIT_CONFIRMED_PACKETS=100;
    struct ProxyFrameCache{
        uint8_t media_type;
        uint8_t media_channel;
        uint8_t frame_type;
        uint32_t frame_len;
        uint16_t frame_seq;
        uint16_t fragment_count;
        uint32_t timestamp;
        set<uint16_t>m_fragment_seq_set;
        shared_ptr<char[]>m_buf_chache;
        ProxyFrameCache(const ProxyHeader&head):media_type(head.media_type),\
            media_channel(head.media_type),\
            frame_type(head.frame_type),\
            frame_len(head.frame_len),\
            frame_seq(head.frame_seq),\
            fragment_count(head.fragment_count),\
            timestamp(head.timestamp),\
            m_buf_chache(new char[frame_len+1]){

        }
        bool is_finished(){return m_fragment_seq_set.size()==fragment_count;}
        void insert_fragment(const ProxyHeader&head,const void *buf){
            if(m_fragment_seq_set.find(head.fragment_seq)==m_fragment_seq_set.end()){
                m_fragment_seq_set.insert(head.fragment_seq);
                memcpy(m_buf_chache.get()+head.fragment_seq*PROXY_FRAGMENT_SIZE,buf,head.data_len);
            }
        }
    };
    struct ProxyFragmentPacket{
        uint16_t data_len;
        shared_ptr<char[]>buf;
        bool using_tcp;
        ProxyFragmentPacket(uint16_t _data_len,bool _tcp=true):data_len(_data_len),buf(new char[data_len+1]),using_tcp(_tcp){
            buf.get()[0]='$';
        }
    };

public:
    ProxyInterface(uint32_t token,PTransMode mode,POUTPUT_CALLBACK tcp_callback,POUTPUT_CALLBACK udp_callback,PFRAME_CALLBACK recv_callback=nullptr);
    bool send_frame(shared_ptr<ProxyFrame> frame);
    bool protocol_input(const void *buf,uint32_t buf_len);
private:
    static PFrameType inline get_264_type(char first_byte){
        int nalu_type=first_byte&0x1f;
        if(nalu_type==5)return  FLUSH_FRAME;
        else if(nalu_type==7||nalu_type==8)return KEY_FRAME;
        else {
            return NORMAL_FRAME;
        }
    }
    static PFrameType inline get_265_type(char first_byte){
        int nalu_type=(first_byte&0x7E)>>1;
        if(nalu_type>=16&&nalu_type<=21)return  FLUSH_FRAME;
        else if(nalu_type==32||nalu_type==33||nalu_type==34)return KEY_FRAME;
        else {
            return NORMAL_FRAME;
        }
    }
    static PFrameType inline  get_frame_type(int media_type,const char *buf){
        switch (media_type) {
        case PPCMA:
        case PAAC:
            return NORMAL_FRAME;
        case PH264:
            return get_264_type(buf[0]);
        case PH265:
            return get_265_type(buf[0]);
        default:
            return  NORMAL_FRAME;
        }
    }
    bool inline circle_compare(uint16_t last,uint16_t present){
        return  present>=last||(last-present)>UINT16_MAX/2;
    }
    void response_frame(uint16_t seq);
    static constexpr uint16_t P_INVALID_SEQ=UINT16_MAX;
    static constexpr uint16_t P_FRAME_OFFSET=PROXY_FRAGMENT_SIZE-1;
    static constexpr size_t P_HEADER_SIZE=sizeof (ProxyHeader);
    bool raw_tcp_handle_packet(shared_ptr<ProxyFrame> frame);
    bool raw_udp_handle_packet(shared_ptr<ProxyFrame> frame);
    bool raw_hybrid_handle_packet(shared_ptr<ProxyFrame> frame);
    bool graded_tcp_handle_packet(shared_ptr<ProxyFrame> frame);
    bool graded_hybrid_handle_packet(shared_ptr<ProxyFrame> frame);

    void raw_tcp_handle_packet(uint16_t seq);
    void raw_udp_handle_packet(uint16_t seq);
    void raw_hybrid_handle_packet(uint16_t seq);
    void graded_tcp_handle_packet(uint16_t seq);
    void graded_hybrid_handle_packet(uint16_t seq);

    inline void dump_ProxyFrameCache_info(shared_ptr<ProxyFrameCache> cache)
    {
        printf("timestamp : %u  frame_size %u frame_seq : %hu set_size:%hu\r\n",cache->frame_seq,cache->frame_len,cache->fragment_count,cache->m_fragment_seq_set.size());
        for(auto i:cache->m_fragment_seq_set)
        {
            printf("%hu ",i);
        }
        printf("\r\n");
    }
    mutex m_mutex;
    uint32_t m_stream_token;
    PTransMode m_mode;
    POUTPUT_CALLBACK m_tcp_callback;
    POUTPUT_CALLBACK m_udp_callback;
    PFRAME_CALLBACK m_recv_callback;
    //control部分 send
    /**
     * @brief m_packet_chache
     */
    queue<shared_ptr<ProxyFragmentPacket>> m_packet_chache;
    uint16_t m_last_send_frame_seq;
    uint16_t m_last_wait_confirmed_frame_seq;
    uint16_t m_last_confirmed_flush_frame_seq;
    //control部分 recv
    map<uint16_t,shared_ptr<ProxyFrameCache>>m_recv_frame_chache;
    uint16_t m_min_recv_frame_seq;
};
}
#endif // PROXY_PROTOCOL_H
