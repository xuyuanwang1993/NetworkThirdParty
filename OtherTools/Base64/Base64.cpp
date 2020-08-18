#include "Base64.h"
#include <mutex>
#include <iostream>
using namespace std;
using namespace micagent;
std::string micagent::base64Encode(const void *input_buf, uint32_t buf_size)
{
    static const char base64Char[] ="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string ret("");
    if(buf_size==0) return ret;
    auto buf=reinterpret_cast<const uint8_t*>(input_buf);
    unsigned const numOrig24BitValues = buf_size/3;
    bool havePadding = buf_size > numOrig24BitValues*3;//判断是否有余数
    bool havePadding2 = buf_size == numOrig24BitValues*3 + 2;//判断余数是否等于2
    unsigned const numResultBytes = 4*(numOrig24BitValues + havePadding);//计算最终结果的size
    ret.resize(numResultBytes);//确定返回字符串大小
    unsigned i;
    for (i = 0; i < numOrig24BitValues; ++i) {
      ret[4*i+0] = base64Char[(buf[3*i]>>2)&0x3F];
      ret[4*i+1] = base64Char[(((buf[3*i]&0x3)<<4) | (buf[3*i+1]>>4))&0x3F];
      ret[4*i+2] = base64Char[((buf[3*i+1]<<2) | (buf[3*i+2]>>6))&0x3F];
      ret[4*i+3] = base64Char[buf[3*i+2]&0x3F];
    }
    //处理不足3位的情况
    //余1位需在后面补齐2个'='
    //余2位需补齐一个'='
    if (havePadding) {
      ret[4*i+0] = base64Char[(buf[3*i]>>2)&0x3F];
      if (havePadding2) {
        ret[4*i+1] = base64Char[(((buf[3*i]&0x3)<<4) | (buf[3*i+1]>>4))&0x3F];
        ret[4*i+2] = base64Char[(buf[3*i+1]<<2)&0x3F];
      } else {
        ret[4*i+1] = base64Char[((buf[3*i]&0x3)<<4)&0x3F];
        ret[4*i+2] = '=';
      }
      ret[4*i+3] = '=';
    }
    return ret;
}

pair<shared_ptr<uint8_t>,uint32_t> micagent::base64Decode(const std::string & origin_string)
{
    static char base64DecodeTable[256];
    static std::once_flag oc_init;
    std::call_once(oc_init,[&](){
        int i;//初始化映射表
        for (i = 0; i < 256; ++i) base64DecodeTable[i] = (char)0x80;
        for (i = 'A'; i <= 'Z'; ++i) base64DecodeTable[i] = 0 + (i - 'A');
        for (i = 'a'; i <= 'z'; ++i) base64DecodeTable[i] = 26 + (i - 'a');
        for (i = '0'; i <= '9'; ++i) base64DecodeTable[i] = 52 + (i - '0');
        base64DecodeTable[(unsigned char)'+'] = 62;
        base64DecodeTable[(unsigned char)'/'] = 63;
        base64DecodeTable[(unsigned char)'='] = 0;
    });
    if(origin_string.empty()) return make_pair(shared_ptr<uint8_t>(),0);
    int k=0;
    int paddingCount = 0;
    auto buf_size=origin_string.length();
    uint32_t ret_size;
    int const jMax = buf_size - 3;
    ret_size=3*buf_size/4;
    shared_ptr<uint8_t>ret_buf(new uint8_t[ret_size],std::default_delete<uint8_t[]>());
    for(int j=0;j<jMax;j+=4)
    {
        char inTmp[4], outTmp[4];
        for (int i = 0; i < 4; ++i) {
          inTmp[i] = origin_string[i+j];
          if (inTmp[i] == '=') ++paddingCount;
          outTmp[i] = base64DecodeTable[(unsigned char)inTmp[i]];
          if ((outTmp[i]&0x80) != 0) outTmp[i] = 0; // this happens only if there was an invalid character; pretend that it was 'A'
        }
        ret_buf.get()[k++]=(outTmp[0]<<2) | (outTmp[1]>>4);
        ret_buf.get()[k++] = (outTmp[1]<<4) | (outTmp[2]>>2);
        ret_buf.get()[k++] = (outTmp[2]<<6) | outTmp[3];
    }
    return make_pair(ret_buf,ret_size-paddingCount);
}
