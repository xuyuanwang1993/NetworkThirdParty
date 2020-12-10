#ifndef RTMP_COMMON_H
#define RTMP_COMMON_H
#include "event_loop.h"
namespace micagent {
using namespace std;
/*rtmp authorize callback*/
using RTMP_AUTHORIZE_CALLBACK=function<bool(const string &,const string &)>;
/*define rtmp medias type*/
enum RTMP_MEDIA_TYPE:uint8_t{
    RTMP_NONE,//unknown,default value
    RTMP_VIDEO,//video
    RTMP_AUDIO,//audio
    RTMP_OTHER_MEDIAS,//maybe some captions
};
enum RTMP_CODE_ID:uint8_t{
    RTMP_G711A=0x7,
    RTMP_AAC=0xa,//10
    RTMP_H264=0x7,
    RTMP_G711U=0x8,
    RTMP_H265=0xc,//12
};

enum RTMP_MODE:uint8_t{
    RTMP_SERVER,
    RTMP_PUSHER,
    RTMP_CLIENT,
};
enum RTMP_DEFINES:uint32_t{
    RTMP_VERSION=3,
    DEFAULT_RTMP_CONNECTION_TIMEOUT_TIME=5000,//5s
    RTMP_SERVER_RESOURCE_CHECK_INTERVAL=2000,//2s
    RTMP_CHUNK_EXTERN_TIMESTAMP_FLAG=0xffffff,
    RTMP_DEFAULT_CHUNK_SIZE=1400,
};

struct Rtmp_Url{
    string protocal="";
    string domain="";
    string ip="";
    uint16_t port=1935;
    string stream_name="";
    string app_name="";
    string app_stream_name="";
    string authorization_string="";
    string raw_url="";
};
enum RTMP_HANDSHAKE_STATUS:uint8_t{
    RTMP_WAIT_C0C1=0x1,
    RTMP_WAIT_S0S1=0x10,
    RTMP_RECV_C0=0x02,
    RTMP_RECV_C1=0x04,
    RTMP_RECV_C2=0x08,
    RTMP_RECV_S0=0x20,
    RTMP_RECV_S1=0x40,
    RTMP_RECV_S2=0x80,
    RTMP_RECV_C0C1=0X07,
    RTMP_RECV_S0S1=0X70,
    RTMP_RECV_C0C1C2=0xf,
    RTMP_RECV_S0S1S2=0xf0,
    RTMP_HANDSHAKE_COMPLETE=0xff,
};
//C0/S0    1 byte       version
//C1/S1    1536byte  send_timestamp(4 big-endian)+0_fills(4)+random_area(1528)
//C2/S2    1536byte  send_timestamp(4 big-endian)+S1/C1 send_timestamp(4 big-endian)+S1/C1 random_area(1528)
//
struct handshake_info{
    static constexpr uint32_t buf_len=1+1536+1536;
    RTMP_HANDSHAKE_STATUS handshake_status;
    shared_ptr<uint8_t>send_buf;
    uint32_t recv_len=0;
    shared_ptr<uint8_t>recv_buf;
};
enum rtmp_chunk_type:uint8_t{
    RTMP_CHUNK_0=0,
    RTMP_CHUNK_1=1,
    RTMP_CHUNK_2=2,
    RTMP_CHUNK_3=3,
};
static constexpr uint8_t rtmp_msg_header_len[4]={11,7,3,0};
struct rtmp_chunk_bask_header{
    rtmp_chunk_type chunk_type;
    uint8_t cs_id_byte1;
    uint8_t cs_id_byte2;
    uint8_t cs_id_byte3;
    //cs_id_byte1(3-63) cs_id=cs_id_byte1
    //cs_id_byte1=0        cs_id=64+cs_id_byte2
    //cs_id_byte1=1        cd_id=64+cs_id_byte2+cs_id_byte3*256
    //cs_id_byte1=2        reserved,application's message
    uint32_t cs_id;//3-65599
};
enum rtmp_chunk_message_type:uint8_t{
    RTMP_CHUNK_MESSAGE_TYPE_RESERVERED                =0x00,
    RTMP_CHUNK_MESSAGE_TYPE_CHUNK_SIZE =0x01,
    RTMP_CHUNK_MESSAGE_TYPE_BYTES_READ_REPORT=0x03,
    RTMP_CHUNK_MESSAGE_TYPE_CONTROL            =0x04,
    RTMP_CHUNK_MESSAGE_TYPE_SERVER_BW          =0x05,
    RTMP_CHUNK_MESSAGE_TYPE_CLIENT_BW          =0x06,
    RTMP_CHUNK_MESSAGE_TYPE_AUDIO              =0x08,
    RTMP_CHUNK_MESSAGE_TYPE_VIDEO              =0x09,
    RTMP_CHUNK_MESSAGE_TYPE_FLEX_STREAM_SEND  = 0x0F,
    RTMP_CHUNK_MESSAGE_TYPE_FLEX_SHARED_OBJECT= 0x10,
    RTMP_CHUNK_MESSAGE_TYPE_FLEX_MESSAGE       =0x11,
    RTMP_CHUNK_MESSAGE_TYPE_INFO               =0x12,
    RTMP_CHUNK_MESSAGE_TYPE_SHARED_OBJECT     = 0x13,
    RTMP_CHUNK_MESSAGE_TYPE_INVOKE             =0x14,
    RTMP_CHUNK_MESSAGE_TYPE_FLASH_VIDEO        =0x16,
};

/**
 * @brief The rtmp_chunk_0 struct
 * notify a new stream
 * set the absolute timestamp
 * payload may be followed with some rtmp_chunk_3 fragmentation with the local chunk size
 */
struct rtmp_chunk_0{
    uint32_t timestamp;//3 byte big-endian ,absolute timestamps
    uint32_t message_length;//3 byte
    rtmp_chunk_message_type type_id;//1 byte
    uint32_t message_stream_id;//4 byte
};
/**
 * @brief The rtmp_chunk_1 struct
 * the first chunk  of a message after a stream is notfied
 */
struct rtmp_chunk_1{
    uint32_t timestamp_diff;//3 byte big-endian ,absolute timestamps
    uint32_t new_message_length;//3 byte
    rtmp_chunk_message_type type_id;//1 byte
};
/**
 * @brief The rtmp_chunk_2 struct
 * notify a new message
 * only update the timestamp
 * usually,a constant-length message packet may use this chunk
 */
struct rtmp_chunk_2{
    uint32_t timestamp_diff;//3 byte big-endian ,absolute timestamps
};
/**
 * @brief The rtmp_chunk_3 struct
 * if a message is too large,this will may be used.
 */
struct rtmp_chunk_3{
    //empty
};
/**
 * rtmp extended timestamp
 * when timestamp or timestamp_diff is set to 0xffffff,
 * this field is used,actual timestamp=timestamp_diff/timestamp + extended timestamp
 */
 struct rtmp_message_packet{
     rtmp_chunk_bask_header header;
     rtmp_chunk_0 chunk_base;
     uint32_t extended_timestamp;
     shared_ptr<uint8_t>payload;
     uint32_t cs_id;//caculate
     uint32_t timestamp;//caculate
     uint32_t paylod_filled_len;
     //just for send
     shared_ptr<uint8_t>raw_data;
     uint32_t raw_data_len;
 };

class rtmp_helper{
public:
    /**
     * @brief Parse_Rtmp_Url parse a rtmp url
     * @param url
     * @return
     * @details this function may return the error  by throwing an exception
     */
    static Rtmp_Url Parse_Rtmp_Url(const string &url);
};
}
#endif // RTMP_COMMON_H
