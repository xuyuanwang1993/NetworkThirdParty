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
    CONTROL_COMMAND,
    FRAME_ACK,
    FLUSH_FRAME,
    KEY_FRAME,
    NORMAL_FRAME,
}PFrameType;
typedef  enum{
    PNONE=0,
    PPCMA = 8,
    PH264 = 96,
    PAAC  = 37,
    PH265 = 265,
}PMediaTYpe;
typedef  enum{
    P_NOT_NEED_AUTHORIZATION=100,    //Not Need Authorization
    P_ILEGAL_ACCOUNT=101,    //Ilegal Account
    P_AUTHORIZATION_FAILED=102,		//Authorization Failed
    P_OK=200,    //OK
    P_BAD_REQUEST=400,		//Bad Request
    P_NONCE_IS_TIME_OUT=401,		//Nonce Is Time Out
    P_STREAM_IS_SET_UP=402,		//Stream Is Set Up
    P_STREAM_NAME_IS_IN_USE=403,		//Stream Name Is In Use
    P_STREAM_NOT_FOUND=404,		//Stream Not Found
    P_NOT_SUPPORTED_COMMAND=500,		//Not Supported Command
    P_INTERNAL_ERROR=505,    //Internal Error
    P_NOT_SUPPORTED_TRANSMISSION_MODE=506,		//Not SupportedTransmission Mode
    P_UNKNOWN_MEDIA_TYPE=507,		//Unknown Media Type
}PStatusCode;
typedef enum{
    /*
 * 此模式下发送不缓存，直接发送
 * 接收不发送确认不缓存,组包完成直接返回给应用层
 */
    RAW_TCP=0,
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
//读取32位系统本地时间
struct p_timer_help{
    //取后32位
    static uint32_t getTimesTamp(){
        auto timePoint = std::chrono::steady_clock::now();
        return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(timePoint.time_since_epoch()).count());
    }
};
//转发帧结构定义
struct ProxyFrame{
    //媒体类型 控制帧为PNONE
    uint8_t media_type;
    //PFrameType
    uint8_t frame_type;
    //媒体数据通道信息  暂时只支持两个数据通道
    uint8_t media_channel;
    //流在服务器上的编号
    uint32_t stream_token;
    //数据部分长度
    uint32_t data_len;
    //帧时间戳
    uint32_t timestamp;
    //帧序列号
    uint16_t frame_seq;
    //帧数据缓存
    shared_ptr<char>data_buf;
    ProxyFrame(const void *buf=nullptr,uint32_t len=0,uint8_t type=PNONE,uint8_t _frame_type=CONTROL_COMMAND,uint8_t channel=0,uint32_t token=0,uint16_t seq=0,uint32_t time_stamp=0):\
        media_type(type),frame_type(_frame_type),media_channel(channel),stream_token(token),data_len(len),timestamp(time_stamp),frame_seq(seq),data_buf(new char[data_len+1]\
                                                                                                                                                                 ,std::default_delete<char[]>()){
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
//协议数据输出回调函数
using POUTPUT_CALLBACK=function<bool(const void *,uint32_t buf_len)>;
//应用层帧数据输出回调函数
using PFRAME_CALLBACK=function<void(shared_ptr<ProxyFrame>)>;
//tcp按流传输的数据解析
struct PStreamParse{
    //未完成的分包数据缓存
    shared_ptr<char >buf_cache;
    //缓存长度
    uint16_t last_buf_len;
    PStreamParse():buf_cache(new char[PROXY_FRAGMENT_SIZE+sizeof(ProxyHeader)+1],std::default_delete<char[]>()),last_buf_len(0){

    }
    //协议解析当前包的数据长度
    void get_data_len(const void *p, uint16_t *l)
    {
#if IWORDS_BIG_ENDIAN
        *l = *(const unsigned char*)(p + 1);
        *l = *(const unsigned char*)(p + 0) + (*w << 8);
#else
        *l = *(const uint16_t*)p;
#endif
    }
    /**
     * @brief insert_buf  输入tcp流协议数据
     * @param buf
     * @param buf_len
     * @return 返回完整的协议数据包
     */
    queue<pair<shared_ptr<char>,uint16_t>>insert_buf(const char *buf,uint16_t buf_len)
    {
        queue<pair<shared_ptr<char>,uint16_t>> ret;
        if(buf_len==0)return  ret;
        uint16_t new_data_len=last_buf_len+buf_len;
        shared_ptr<char>new_buf(new char[new_data_len],std::default_delete<char[]>());
        if(last_buf_len>0)memcpy(new_buf.get(),buf_cache.get(),last_buf_len);
        memcpy(new_buf.get()+last_buf_len,buf,buf_len);
        last_buf_len=0;
        if(new_buf.get()[0]!='$'){
            //不符合规则的包丢弃
            return ret;
        }
        uint16_t use_len=0;
        while(new_data_len>=sizeof (ProxyHeader))
        {
            if(new_buf.get()[use_len]!='$')break;
            uint16_t packet_len=0;
            get_data_len(new_buf.get()+use_len+POFFSETOF(ProxyHeader,data_len),&packet_len);
            packet_len+=sizeof (ProxyHeader);
            if(new_data_len>=packet_len)
            {
                shared_ptr<char >data(new char[packet_len+1],std::default_delete<char[]>());
                memcpy(data.get(),new_buf.get()+use_len,packet_len);
                ret.push(make_pair(data,packet_len));
                use_len+=packet_len;
                new_data_len-=packet_len;
            }
            else {
                break;
            }
        }
        if(new_data_len>PROXY_FRAGMENT_SIZE+sizeof(ProxyHeader))
        {//packet recv error
            return ret;
        }
        last_buf_len=new_data_len;
        if(last_buf_len>0)
        {
            memcpy(buf_cache.get(),new_buf.get()+use_len,last_buf_len);
        }
        return ret;
    }
};

class ProxyInterface{
    //接收缓存帧队列长度
    static constexpr uint16_t MAX_ORDER_CACHE_SIZE=5;
    //接收缓存窗口最大大小
    static constexpr uint16_t MAX_WINDOW_SIZE=MAX_ORDER_CACHE_SIZE*2;
    //发送缓存最大分片数目
    static constexpr uint16_t MAX_FRAGMENT_CACHE_SIZE=20000;
    //发送缓存最大分片数目
    static constexpr uint16_t MAX_GRADED_CACHE_SIZE=100;
    //帧最大长度
    static constexpr uint32_t MAX_FRAME_SIZE=10*1024*1024;
    //接收缓存帧结构定义
    struct ProxyFrameCache{
        //应用层媒体类型
        uint8_t media_type;
        //应用层媒体数据通道编号
        uint8_t media_channel;
        //协议帧类型
        uint8_t frame_type;
        //帧总长度
        uint32_t frame_len;
        //帧序列号
        uint16_t frame_seq;
        //分片总数
        uint16_t fragment_count;
        //帧发送时间戳
        uint32_t timestamp;
        //已接收的分片序列号set
        set<uint16_t>m_fragment_seq_set;
        //帧缓存
        shared_ptr<char>m_buf_chache;
        ProxyFrameCache(const ProxyHeader&head):media_type(head.media_type),\
            media_channel(head.media_channel),\
            frame_type(head.frame_type),\
            frame_len(head.frame_len),\
            frame_seq(head.frame_seq),\
            fragment_count(head.fragment_count),\
            timestamp(head.timestamp),\
            m_buf_chache(new char[frame_len+1],std::default_delete<char[]>()){

        }
        //判断帧是否组包完成
        bool is_finished(){return m_fragment_seq_set.size()==fragment_count;}
        //根据分片序号在指定位置插入数据
        void insert_fragment(const ProxyHeader&head,const void *buf){
            if(m_fragment_seq_set.find(head.fragment_seq)==m_fragment_seq_set.end()){
                m_fragment_seq_set.insert(head.fragment_seq);
                memcpy(m_buf_chache.get()+head.fragment_seq*PROXY_FRAGMENT_SIZE,buf,head.data_len);
            }
        }
    };
    //发送协议数据缓存结构定义
    struct ProxyFragmentPacket{
        //数据长度
        uint16_t data_len;
        //数据缓存
        shared_ptr<char>buf;
        //是否使用tcp传输
        bool using_tcp;
        ProxyFragmentPacket(uint16_t _data_len,bool _tcp=true):data_len(_data_len),buf(new char[data_len+1],std::default_delete<char[]>()),using_tcp(_tcp){
            buf.get()[0]='$';
        }
    };

public:
    ProxyInterface(uint32_t token,PTransMode mode,POUTPUT_CALLBACK tcp_callback,POUTPUT_CALLBACK udp_callback,PFRAME_CALLBACK recv_callback=nullptr);
    /**
     * @brief send_frame 发送媒体数据帧,非媒体数据将被视为normal_frame
     * @param frame
     * @return
     */
    bool send_frame(shared_ptr<ProxyFrame> frame);
    /**
     * @brief send_control_command 控制命令发送，以tcp方式发送
     * @param buf
     * @param buf_len
     * @return
     */
    bool send_control_command(const void *buf,uint16_t buf_len);
    /**
     * @brief protocol_input 协议数据输入,应输入从底层socket接收到的原始数据
     * @param buf
     * @param buf_len
     * @return
     */
    bool protocol_input(const void *buf,uint32_t buf_len);
    //修改传输模式
    void change_trans_mode(PTransMode mode){m_mode=mode;}
    //从协议包中读取流token
    static  uint32_t get_stream_token(const void *buf,uint32_t buf_len);
    //输出头信息
    static void dump_header_info(const void *buf,uint32_t buf_len);
private:
    //264帧分析
    static PFrameType inline get_264_type(char first_byte){
        int nalu_type=first_byte&0x1f;
        if(nalu_type==5)return  FLUSH_FRAME;
        else if(nalu_type==7||nalu_type==8)return KEY_FRAME;
        else {
            return NORMAL_FRAME;
        }
    }
    //265帧分析
    static PFrameType inline get_265_type(char first_byte){
        int nalu_type=(first_byte&0x7E)>>1;
        if(nalu_type>=16&&nalu_type<=21)return  FLUSH_FRAME;
        else if(nalu_type==32||nalu_type==33||nalu_type==34)return KEY_FRAME;
        else {
            return NORMAL_FRAME;
        }
    }
    //判断帧的类型
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
    //循环序列号比较
    bool inline circle_compare(uint16_t last,uint16_t present){
        return  present>=last||(last-present)>UINT16_MAX/2;
    }
    inline void update_min_recv_frame_seq(uint16_t seq){
        m_min_recv_frame_seq=seq;
    }
    inline void cache_handle_recv_packet(uint16_t seq)
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
                if(seq_diff>=MAX_ORDER_CACHE_SIZE){
                    if(!iter->second->is_finished()){
                        m_miss_packet=true;
                        //接收未完全的缓存
#ifdef DEBUG
                        printf("******************cache recv  erase cache frames %u *****%lu**********\r\n",iter->second->frame_seq,iter->second->fragment_count-iter->second->m_fragment_seq_set.size());
#endif
                        update_min_recv_frame_seq(iter->second->frame_seq);
                        m_recv_frame_chache.erase(iter++);
                        continue;
                    }
                    else {
                        //接收完成直接返回应用层
                        m_min_recv_frame_seq=iter->second->frame_seq;
                        auto &cache= iter->second;
                        shared_ptr<ProxyFrame>frame(new ProxyFrame(cache->m_buf_chache.get(),cache->frame_len,cache->media_type,cache->frame_type,cache->media_channel,m_stream_token,cache->frame_seq,cache->timestamp));
                        m_recv_frame_chache.erase(iter++);
                        if(frame->frame_type==FLUSH_FRAME)
                        {
                            m_miss_packet=false;
                        }
//                        if(m_miss_packet&&frame->frame_type==NORMAL_FRAME){
//                            printf("miss packet %hu\r\n",frame->frame_seq);
//                            continue;
//                        }
                        locker.unlock();
                        if(m_recv_callback)m_recv_callback(frame);
                        locker.lock();
                    }
                }
                else {
                    //缓存MAX_ORDER_CACHE_SIZE数目的帧ss
                    break;
                }
            }

        }
    }
    //帧接收应答
    void response_frame(uint16_t seq);
    static constexpr uint16_t P_INVALID_SEQ=UINT16_MAX;
    //帧分片数辅助计算常量
    static constexpr uint16_t P_FRAME_OFFSET=PROXY_FRAGMENT_SIZE-1;
    //协议头长度常量
    static constexpr size_t P_HEADER_SIZE=sizeof (ProxyHeader);
    //发送处理
    bool raw_tcp_handle_packet(shared_ptr<ProxyFrame> frame);
    bool raw_udp_handle_packet(shared_ptr<ProxyFrame> frame);
    bool raw_hybrid_handle_packet(shared_ptr<ProxyFrame> frame);
    bool graded_tcp_handle_packet(shared_ptr<ProxyFrame> frame);
    bool graded_hybrid_handle_packet(shared_ptr<ProxyFrame> frame);
    //接收处理
    void raw_tcp_handle_packet(uint16_t seq);
    void raw_udp_handle_packet(uint16_t seq);
    void raw_hybrid_handle_packet(uint16_t seq);
    void graded_tcp_handle_packet(uint16_t seq);
    void graded_hybrid_handle_packet(uint16_t seq);
    //线程锁
    mutex m_mutex;
    //流编号，初始化传入
    uint32_t m_stream_token;
    //传输模式，初始化传入，可被后续修改
    PTransMode m_mode;
    //稳定传输回调输出函数
    POUTPUT_CALLBACK m_tcp_callback;
    //不稳定传输回调输出函数
    POUTPUT_CALLBACK m_udp_callback;
    //帧接收回调函数
    PFRAME_CALLBACK m_recv_callback;
    //control部分 send
    /**
     * @brief m_packet_chache 发送部分分片缓存
     */
#ifdef DELAY_SEND_CACHE
    queue<shared_ptr<ProxyFragmentPacket>> m_packet_chache;
#endif
    /**
     * @brief m_last_send_frame_seq 本次发送序列号
     */
    uint16_t m_last_send_frame_seq;
    //最近一次确认的帧序号
    uint16_t m_last_confirmed_flush_frame_seq;
    //control部分 recv
    /**
     * @brief m_recv_frame_chache 帧接收缓存
     */
    map<uint16_t,shared_ptr<ProxyFrameCache>>m_recv_frame_chache;
    //最近一次接收的完整帧的帧序号
    uint16_t m_min_recv_frame_seq;
    bool m_miss_packet;
};
}
#endif // PROXY_PROTOCOL_H
