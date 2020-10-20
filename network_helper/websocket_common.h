#ifndef WEBSOCKET_COMMON_H
#define WEBSOCKET_COMMON_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <functional>
#include<random>
#include<memory>
#include<map>
#include "c_log.h"
namespace micagent {
using namespace std;
enum WS_CONNECTION_STATUS{
    WS_CONNECTION_CONNECTING,//wait connect
    WS_CONNECTION_CONNECTED,//connected
    WS_CONNECTION_CLOSED,//closed connection
};

// http://tools.ietf.org/html/rfc6455#section-5.2  Base Framing Protocol
//
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-------+-+-------------+-------------------------------+
// |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
// |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
// |N|V|V|V|       |S|             |   (if payload len==126/127)   |
// | |1|2|3|       |K|             |                               |
// +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
// |     Extended payload length continued, if payload len == 127  |
// + - - - - - - - - - - - - - - - +-------------------------------+
// |                               |Masking-key, if MASK set to 1  |
// +-------------------------------+-------------------------------+
// | Masking-key (continued)       |          Payload Data         |
// +-------------------------------- - - - - - - - - - - - - - - - +
// :                     Payload Data continued ...                :
// + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
// |                     Payload Data continued ...                |
// +---------------------------------------------------------------+
struct WS_Frame_Header{
    enum WS_FrameType
    {
        WS_CONTINUATION_FRAME=0x0,
        WS_TEXT_FRAME = 0x01,
        WS_BINARY_FRAME = 0x02,
        WS_RESERVER_NO_CONTROL_3_FRAME=0x03,
        WS_RESERVER_NO_CONTROL_4_FRAME=0x04,
        WS_RESERVER_NO_CONTROL_5_FRAME=0x05,
        WS_RESERVER_NO_CONTROL_6_FRAME=0x06,
        WS_RESERVER_NO_CONTROL_7_FRAME=0x07,
        WS_CONNECTION_CLOSE_FRAME=0x08,
        WS_PING_FRAME = 0x09,
        WS_PONG_FRAME = 0x0A,
        WS_RESERVER_CONTROL_B_FRAME=0x0B,
        WS_RESERVER_CONTROL_C_FRAME=0X0C,
        WS_RESERVER_CONTROL_D_FRAME=0X0D,
        WS_RESERVER_CONTROL_E_FRAME=0X0E,
        WS_RESERVER_CONTROL_F_FRAME=0X0F,
    };
    //if it is the last frame fragment,set this bit to 1
    uint8_t FIN:1;
    //always be 0
    uint8_t RSV:3;
    //WS_FrameType description the application's frame type
    uint8_t opcode:4;
    //if the frame is encode with mask this bit must be 1
    //usually,mask is must used by websocket_client to send a frame
    uint8_t MASK:1;
    //raw_len <126 payload_len=raw_len
    //raw_len<=65536 payload_len=126
    //raw_len>65536 payload_len=127
    uint8_t payload_len:7;
    //126=<raw_len<=65536 len_for_below_or_equal_65536=raw_len
    //raw_len>65536 len_for_larger_than_65536=raw_len
    union {
        uint16_t len_for_below_or_equal_65536;
        uint64_t len_for_larger_than_65536;
    }extended_payload_len;
    //if payload_len<126 then payload's len=payload_len padding_payload_len=0
    //if payload_len<=65536 then payload's len=len_for_below_or_equal_65536 padding_payload_len=2
    //if payload_len>65536 then payload's len=len_for_larger_than_65536 padding_payload_len=8
    uint8_t padding_payload_len;
    //if MASK bit is set,this filed will be filled
    char Masking_key[4];
    /**
     * @brief generate_random_mask_key generate a random mask key
     */
    static void generate_random_mask_key(char (*buf)[4]){
        random_device rd;
        (*buf)[0]=static_cast<char>(rd()%256);
        (*buf)[1]=static_cast<char>(rd()%256);
        (*buf)[2]=static_cast<char>(rd()%256);
        (*buf)[3]=static_cast<char>(rd()%256);
    }
    WS_Frame_Header():FIN(0),RSV(0),opcode(0),MASK(0),payload_len(0),padding_payload_len(0){

    }
};
struct WS_Frame{
    bool is_fin;//FIN filed
    bool use_mask;//MASK filed
    WS_Frame_Header::WS_FrameType type;//opcode filed
    uint32_t data_len;//raw data's len
    shared_ptr<uint8_t>data;//copy from the raw data
    WS_Frame(const void *input=nullptr,uint32_t input_len=0,WS_Frame_Header::WS_FrameType data_type=WS_Frame_Header::WS_CONNECTION_CLOSE_FRAME,bool _is_fin=true,bool _use_mask=true):\
    is_fin(_is_fin),use_mask(_use_mask),type(data_type),data_len(input_len),data(new uint8_t[data_len+1],default_delete<uint8_t[]>())
    {
        if(data_len>0){
            memcpy(data.get(),input,data_len);
        }
    }
};

class web_socket_helper{
    static constexpr int SHA1_SIZE=128;
    struct WS_sha1_context{
        unsigned        message_digest[5];
        unsigned        length_low;
        unsigned        length_high;
        unsigned char   message_block[64];
        int             message_block_index;
        int             computed;
        int             corrupted;
    };
public:
    /**
     * @brief WS_sha1_hash sha1_hash the source string
     * @param source
     * @return
     */
    static string WS_sha1_hash(const string &source);
    /**
     * @brief WS_tolower convert to lowercase
     * @param c
     * @return
     */
    static int WS_tolower(int c);
    /**
     * @brief WS_htoi convert from  a hex string to int
     * @param s
     * @param start
     * @param len
     * @return
     */
    static int WS_htoi(const string &s,int start,int len);
    /**
     * @brief WS_EncodeData encode a WS_Frame to transmission packet
     * @param frame
     * @return
     */
    static pair<shared_ptr<uint8_t>,uint32_t> WS_EncodeData(const WS_Frame &frame );
    /**
     * @brief WS_DecodeData decode a transmission packet
     * @param input
     * @param input_len
     * @param discard_no_mask if this it set,all packet that contains not mask will be abandoned
     * @return
     */
    static WS_Frame WS_DecodeData(const void *input,uint32_t input_len,bool discard_no_mask=true );
    /**
     * @brief WS_Get_guid return the websocket's magic string
     * @return
     */
    static const char * WS_Get_guid(){
        return "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    }
    /**
     * @brief WS_Generate_Accept_Key generate the server's accept key with specific client key
     * @param client_key
     * @return
     */
    static string WS_Generate_Accept_Key( string client_key);
    /**
     * @brief WS_util_test test member function
     */
    static void WS_util_test();
    /**
     * @brief WS_dump_frame dump a WS_Frame
     * @param frame
     * @param max_print_len
     */
    static void WS_dump_frame(const WS_Frame &frame,uint32_t max_print_len=100){
        uint32_t str_print_len=frame.data_len>max_print_len?max_print_len:frame.data_len;
        if(str_print_len>0)MICAGENT_DEBUG("WS_Frame fin(%d) mask(%d) type(%d) data_len(%u) str(%s)",frame.is_fin,frame.use_mask,frame.type,frame.data_len\
        ,string(reinterpret_cast<const char *>(frame.data.get()),str_print_len).c_str());
        else {
            MICAGENT_DEBUG("WS_Frame fin(%d) mask(%d) type(%d) data_len(%u) str is empty!",frame.is_fin,frame.use_mask,frame.type,frame.data_len);
        }
    }
private:
    //sha1 handle
    static void WS_sha1_process_message_block(WS_sha1_context*);
    static void WS_sha1_pad_message(WS_sha1_context*);
    static void WS_sha1_reset(web_socket_helper::WS_sha1_context *context);
    static int WS_sha1_result(WS_sha1_context *context);
    static void WS_sha1_input(WS_sha1_context *context,const char *message_array,unsigned length);
};
using WS_FRAME_CALLBACK=function<bool(const WS_Frame &frame)>;
}
#endif // WEBSOCKET_COMMON_H
