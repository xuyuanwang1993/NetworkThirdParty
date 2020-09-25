#include "websocket_common.h"
#include <cstdint>
#include <arpa/inet.h>
#include<memory>
#include<Base64.h>
#define WS_SHA1_CIRCULAR_SHIFT(bits,word) ((((word) << (bits)) & 0xFFFFFFFF) | ((word) >> (32-(bits))))
using namespace micagent;
void web_socket_helper::WS_sha1_reset(WS_sha1_context *context) // 初始化动作
{
    context->length_low             = 0;
    context->length_high            = 0;
    context->message_block_index    = 0;

    context->message_digest[0]      = 0x67452301;
    context->message_digest[1]      = 0xEFCDAB89;
    context->message_digest[2]      = 0x98BADCFE;
    context->message_digest[3]      = 0x10325476;
    context->message_digest[4]      = 0xC3D2E1F0;

    context->computed   = 0;
    context->corrupted  = 0;
}


int web_socket_helper::WS_sha1_result(WS_sha1_context *context) // 成功返回1，失败返回0
{
    if (context->corrupted) {
        return 0;
    }
    if (!context->computed) {
        WS_sha1_pad_message(context);
        context->computed = 1;
    }
    return 1;
}


void web_socket_helper::WS_sha1_input(WS_sha1_context *context,const char *message_array,unsigned length)
{
    if (!length) return;

    if (context->computed || context->corrupted){
        context->corrupted = 1;
        return;
    }

    while(length-- && !context->corrupted){
        context->message_block[context->message_block_index++] = (*message_array & 0xFF);

        context->length_low += 8;

        context->length_low &= 0xFFFFFFFF;
        if (context->length_low == 0){
            context->length_high++;
            context->length_high &= 0xFFFFFFFF;
            if (context->length_high == 0) context->corrupted = 1;
        }

        if (context->message_block_index == 64){
            WS_sha1_process_message_block(context);
        }
        message_array++;
    }
}

void web_socket_helper::WS_sha1_process_message_block(WS_sha1_context *context)
{
    const unsigned K[] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
    int         t;
    unsigned    temp;
    unsigned    W[80];
    unsigned    A, B, C, D, E;

    for(t = 0; t < 16; t++) {
        W[t] = ((unsigned) context->message_block[t * 4]) << 24;
        W[t] |= ((unsigned) context->message_block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) context->message_block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) context->message_block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)  W[t] = WS_SHA1_CIRCULAR_SHIFT(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);

    A = context->message_digest[0];
    B = context->message_digest[1];
    C = context->message_digest[2];
    D = context->message_digest[3];
    E = context->message_digest[4];

    for(t = 0; t < 20; t++) {
        temp =  WS_SHA1_CIRCULAR_SHIFT(5,A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = WS_SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    for(t = 20; t < 40; t++) {
        temp = WS_SHA1_CIRCULAR_SHIFT(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = WS_SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    for(t = 40; t < 60; t++) {
        temp = WS_SHA1_CIRCULAR_SHIFT(5,A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = WS_SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    for(t = 60; t < 80; t++) {
        temp = WS_SHA1_CIRCULAR_SHIFT(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = WS_SHA1_CIRCULAR_SHIFT(30,B);
        B = A;
        A = temp;
    }
    context->message_digest[0] = (context->message_digest[0] + A) & 0xFFFFFFFF;
    context->message_digest[1] = (context->message_digest[1] + B) & 0xFFFFFFFF;
    context->message_digest[2] = (context->message_digest[2] + C) & 0xFFFFFFFF;
    context->message_digest[3] = (context->message_digest[3] + D) & 0xFFFFFFFF;
    context->message_digest[4] = (context->message_digest[4] + E) & 0xFFFFFFFF;
    context->message_block_index = 0;
}

void web_socket_helper::WS_sha1_pad_message(WS_sha1_context* context)
{
    if (context->message_block_index > 55) {
        context->message_block[context->message_block_index++] = 0x80;
        while(context->message_block_index < 64)  context->message_block[context->message_block_index++] = 0;
        WS_sha1_process_message_block(context);
        while(context->message_block_index < 56) context->message_block[context->message_block_index++] = 0;
    } else {
        context->message_block[context->message_block_index++] = 0x80;
        while(context->message_block_index < 56) context->message_block[context->message_block_index++] = 0;
    }
    context->message_block[56] = (context->length_high >> 24 ) & 0xFF;
    context->message_block[57] = (context->length_high >> 16 ) & 0xFF;
    context->message_block[58] = (context->length_high >> 8 ) & 0xFF;
    context->message_block[59] = (context->length_high) & 0xFF;
    context->message_block[60] = (context->length_low >> 24 ) & 0xFF;
    context->message_block[61] = (context->length_low >> 16 ) & 0xFF;
    context->message_block[62] = (context->length_low >> 8 ) & 0xFF;
    context->message_block[63] = (context->length_low) & 0xFF;

    WS_sha1_process_message_block(context);
}

string web_socket_helper::WS_sha1_hash(const string &source)
{
    WS_sha1_context    sha;
    WS_sha1_reset(&sha);
    WS_sha1_input(&sha, source.c_str(), source.length());

    if (!WS_sha1_result(&sha)){
        MICAGENT_ERROR("SHA1 ERROR: Could not compute message digest");
        return nullptr;
    } else {
        shared_ptr<char>buf(new char[SHA1_SIZE+1],default_delete<char []>());
        memset(buf.get(),0,SHA1_SIZE+1);
        sprintf(buf.get(), "%08X%08X%08X%08X%08X", sha.message_digest[0],sha.message_digest[1],
                sha.message_digest[2],sha.message_digest[3],sha.message_digest[4]);
        return buf.get();
    }
}
int web_socket_helper::WS_tolower(int c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + 'a' - 'A';
    }
    else
    {
        return c;
    }
}

int web_socket_helper::WS_htoi(const string&s,int start,int len)
{
    int i,j;
    int n = 0;
    if (s[0] == '0' && (s[1]=='x' || s[1]=='X')) //判断是否有前导0x或者0X
    {
        i = 2;
    }
    else
    {
        i = 0;
    }
    i+=start;
    j=0;
    for (; (s[i] >= '0' && s[i] <= '9')
         || (s[i] >= 'a' && s[i] <= 'f') || (s[i] >='A' && s[i] <= 'F');++i)
    {
        if(j>=len)
        {
            break;
        }
        if (WS_tolower(s[i]) > '9')
        {
            n = 16 * n + (10 + WS_tolower(s[i]) - 'a');
        }
        else
        {
            n = 16 * n + (WS_tolower(s[i]) - '0');
        }
        j++;
    }
    return n;
}
WS_Frame web_socket_helper::WS_DecodeData(const void *input, uint32_t input_len, bool discard_no_mask )
{
    if (input_len < 2) {
        return WS_Frame();
    }
    auto raw_buf=reinterpret_cast<const uint8_t *>(input);
    WS_Frame_Header header;
    uint32_t decode_pos=0;
    header.FIN = (raw_buf[decode_pos] & 0x80) == 0x80;//get fin
    header.opcode=raw_buf[decode_pos]&0xf;//get opcode
    decode_pos++;
    header.MASK = (raw_buf[decode_pos] & 0x80) == 0x80; //  get mask
    if ((!header.MASK )&&discard_no_mask) {
        MICAGENT_ERROR("WS_DecodeData active discard frame!\r\n");
        return WS_Frame();
    }
    header.payload_len = raw_buf[decode_pos] & 0x7F; // get payload_Len
    decode_pos++;
    uint32_t data_len=0;

    if(header.payload_len <126){
        data_len=static_cast<uint32_t>(header.payload_len);
    }
    else if (header.payload_len==126) {
        data_len|=static_cast<uint32_t>(raw_buf[decode_pos++]<<8);
        data_len|=static_cast<uint32_t>(raw_buf[decode_pos++]<<0);
    }
    else if(header.payload_len==127) {
        decode_pos++;//discard
        decode_pos++;//discard
        decode_pos++;//discard
        decode_pos++;//discard
        data_len|=static_cast<uint32_t>(raw_buf[decode_pos++]<<24);
        data_len|=static_cast<uint32_t>(raw_buf[decode_pos++]<<16);
        data_len|=static_cast<uint32_t>(raw_buf[decode_pos++]<<8);
        data_len|=static_cast<uint32_t>(raw_buf[decode_pos++]<<0);
    }
    else {
        //never execute
    }
    if(header.MASK)
    {
        header.Masking_key[0]=static_cast<char>(raw_buf[decode_pos++]);
        header.Masking_key[1]=static_cast<char>(raw_buf[decode_pos++]);
        header.Masking_key[2]=static_cast<char>(raw_buf[decode_pos++]);
        header.Masking_key[3]=static_cast<char>(raw_buf[decode_pos++]);
    }
    //check data_len
    auto left_len=input_len-decode_pos;
    if(left_len!=data_len){
        MICAGENT_ERROR("false websocket frame received!discard it!\r\n");
        return WS_Frame();
    }
    shared_ptr<uint8_t>buf(new uint8_t[data_len+1],default_delete<uint8_t[]>());
    if(data_len>0){
        //copy data
        memcpy(buf.get(),raw_buf+decode_pos,data_len);
        //decode data
        if(header.MASK)
        {
            for(uint32_t i=0;i<data_len;++i)
            {
                buf.get()[i]^=header.Masking_key[i&0x3];
            }
        }
    }
    return WS_Frame(buf.get(),data_len,static_cast<WS_Frame_Header::WS_FrameType>(header.opcode),header.FIN==1,header.MASK==1);
}
 pair<shared_ptr<uint8_t>,uint32_t> web_socket_helper::WS_EncodeData(const WS_Frame &frame )
{
    WS_Frame_Header header;
    memset(&header,0,sizeof (header));
    header.FIN=frame.is_fin;
    header.MASK=frame.use_mask;
    header.opcode=(frame.type)&0xf;
    if(frame.data_len<126){
        header.payload_len|=frame.data_len;
        header.padding_payload_len=0;
    }
    else if (frame.data_len<=65536) {
        header.payload_len|=126;
        header.padding_payload_len=2;
        header.extended_payload_len.len_for_below_or_equal_65536|=frame.data_len;
    }
    else  {
        header.payload_len|=127;
        header.padding_payload_len=8;
        header.extended_payload_len.len_for_larger_than_65536|=frame.data_len;
    }
    if(header.MASK){
        WS_Frame_Header::generate_random_mask_key(&header.Masking_key);
    }
    uint32_t final_buf_size=2/*base*/+header.padding_payload_len/*payload_len extended*/+(header.MASK?4:0)+frame.data_len;
    shared_ptr<uint8_t>final_buf(new uint8_t[final_buf_size+1],default_delete<uint8_t[]>());
    memset(final_buf.get(), 0, final_buf_size);
    // set fin and opcode
    uint32_t operation_pos=0;

    final_buf.get()[operation_pos]=static_cast<uint8_t>(header.opcode);
    if(header.FIN)final_buf.get()[operation_pos]|=0x80;
    operation_pos++;//FIN and opcode
    //set MASK
    if(header.MASK)final_buf.get()[operation_pos]|=0x80;
    //filled payload_len filed
    final_buf.get()[operation_pos]|=header.payload_len;

    if(header.payload_len<126){
        //just do nothing
    }
    else if (header.payload_len==126) {
        final_buf.get()[operation_pos+1]=(header.extended_payload_len.len_for_below_or_equal_65536>>8)&0xff;
        final_buf.get()[operation_pos+2]=(header.extended_payload_len.len_for_below_or_equal_65536>>0)&0xff;
    }
    else {
        final_buf.get()[operation_pos+1]=(header.extended_payload_len.len_for_larger_than_65536>>56)&0xff;
        final_buf.get()[operation_pos+2]=(header.extended_payload_len.len_for_larger_than_65536>>48)&0xff;
        final_buf.get()[operation_pos+3]=(header.extended_payload_len.len_for_larger_than_65536>>40)&0xff;
        final_buf.get()[operation_pos+4]=(header.extended_payload_len.len_for_larger_than_65536>>32)&0xff;
        final_buf.get()[operation_pos+5]=(header.extended_payload_len.len_for_larger_than_65536>>24)&0xff;
        final_buf.get()[operation_pos+6]=(header.extended_payload_len.len_for_larger_than_65536>>16)&0xff;
        final_buf.get()[operation_pos+7]=(header.extended_payload_len.len_for_larger_than_65536>>8)&0xff;
        final_buf.get()[operation_pos+8]=(header.extended_payload_len.len_for_larger_than_65536>>0)&0xff;
    }
    operation_pos++;//payload_len
    operation_pos+=header.padding_payload_len;//extended_payload_len
    //fill Masking_key
    if(header.MASK){
        final_buf.get()[operation_pos]=static_cast<uint8_t>(header.Masking_key[0]);
        final_buf.get()[operation_pos+1]=static_cast<uint8_t>(header.Masking_key[1]);
        final_buf.get()[operation_pos+2]=static_cast<uint8_t>(header.Masking_key[2]);
        final_buf.get()[operation_pos+3]=static_cast<uint8_t>(header.Masking_key[3]);
        operation_pos+=4;//Masking_key
    }
    //copy data
    if(frame.data_len>0)
    {
        memcpy(final_buf.get()+operation_pos,frame.data.get(),frame.data_len);
    }
    //encode data with mask
    if(header.MASK){
        for(uint32_t i=0;i<frame.data_len;++i){
            final_buf.get()[i+operation_pos]^=header.Masking_key[i&0x3];
        }
    }
    return {final_buf,final_buf_size};
}
string web_socket_helper::WS_Generate_Accept_Key(string client_key)
{
    string server_key("");
    if(client_key.empty()) {
        MICAGENT_ERROR("client_key is nullptr.\n");
        return server_key;
    }
    client_key+=WS_Get_guid(); /* 构建新key */
    auto sha1_data_tmp = WS_sha1_hash(client_key); /* hash 加密 */
    auto data_len=sha1_data_tmp.length()/2+1;
    shared_ptr<char>sha1_data(new char[data_len],default_delete<char[]>());
    memset(sha1_data.get(),0,data_len);
    int i=0;
    for(i=0; i<sha1_data_tmp.length(); i+=2)
        sha1_data.get()[i/2] = WS_htoi(sha1_data_tmp, i, 2);
    server_key = base64Encode(sha1_data.get(), data_len-1);

    return server_key;
}
void web_socket_helper::WS_util_test()
{
#if UTIL_TEST
    string client_key="mt4rAIJjS6DIXlo4qbwaFw==";
    auto server_key=WS_Generate_Accept_Key(client_key);
    MICAGENT_WARNNING("server key :%s",server_key.c_str());
    WS_Frame frame(client_key.c_str(),client_key.length(),WS_Frame_Header::WS_TEXT_FRAME);
    auto ret=WS_EncodeData(frame);
    MICAGENT_WARNNING("encode  use mask size %u",ret.second);
    for(uint32_t i=0;i<ret.second;++i)printf("%02x ",ret.first.get()[i]);
    printf("\r\n");
    frame.use_mask=false;
    frame.is_fin=true;
    auto ret2=WS_EncodeData(frame);
    MICAGENT_WARNNING("encode not use mask size %u",ret2.second);
    for(uint32_t i=0;i<ret2.second;++i)printf("%02x ",ret2.first.get()[i]);
    printf("\r\n");
    auto ret3=WS_DecodeData(ret.first.get(),ret.second);
    MICAGENT_DEBUG("decode mask data:%d %d %d %d %s",ret3.is_fin,ret3.use_mask,ret3.type,ret3.data_len,ret3.data.get());
    auto ret4=WS_DecodeData(ret2.first.get(),ret2.second,false);
    MICAGENT_DEBUG("decode unmask data:%d %d %d %d %s",ret4.is_fin,ret4.use_mask,ret4.type,ret4.data_len,ret4.data.get());
    auto ret5=WS_DecodeData(ret2.first.get(),ret2.second);
    MICAGENT_DEBUG("decode unmask data with discard:%d %d %d %d %s",ret5.is_fin,ret5.use_mask,ret5.type,ret5.data_len,ret5.data.get());
    static constexpr uint8_t test_data[13]={0x81,0x87,0xe9,0xb0,0x16,0x51,0xd8,0x81,0x27,0x60,0xd8,0x81,0x27};
    auto ret6=WS_DecodeData(test_data,13);
    MICAGENT_DEBUG("decode unmask data with discard:%d %d %d %d %s",ret6.is_fin,ret6.use_mask,ret6.type,ret6.data_len,ret6.data.get());
#endif
}
