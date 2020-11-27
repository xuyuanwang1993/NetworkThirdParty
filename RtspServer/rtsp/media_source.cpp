#include "media_source.h"
using namespace micagent;
media_source::~media_source()
{

}
uint32_t media_source::find_next_video_nal_pos(const uint8_t *buf, uint32_t buf_len, uint32_t now_pos)
{
    //if available buflen <4 ,there isn't a nal slice.
    if(buf_len<now_pos||buf_len-now_pos<4)return buf_len;
    //first in
    if(now_pos==0&&buf[0]!=0x00)return 0;
    //find start code
    uint32_t next_pos=now_pos;
    int flag=(buf[next_pos]<<24)|(buf[next_pos+1]<<16)|(buf[next_pos+2]<<8)|(buf[next_pos+3]);
    next_pos+=4;
    while(next_pos<buf_len)
    {
        if(flag==0x00000001)break;
        flag=(flag<<8)|(buf[next_pos++]);
    }
    if(flag!=0x00000001||next_pos>=buf_len)return buf_len;
    return next_pos;
}
