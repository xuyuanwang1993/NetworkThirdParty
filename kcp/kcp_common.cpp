#include "kcp_common.h"
using namespace micagent;
bool ikcp_proxy_helper::encode_ikcp_proxy_header(void *buf,uint32_t available_len,const ikcp_proxy_header_s&header)
{
    if(available_len<IKCP_PROXY_HEADER_LEN)return false;
    memset(buf,0,IKCP_PROXY_HEADER_LEN);
    uint8_t *p_buf=static_cast<uint8_t *>(buf);
    p_buf[0]=header.protocal_version;
    p_buf[1]=header.cmd;
    p_buf[2]=(header.sn>>8)&0xff;
    p_buf[3]=(header.sn)&0xff;
    p_buf[4]=(header.timestamp>>24)&0xff;
    p_buf[5]=(header.timestamp>>16)&0xff;
    p_buf[6]=(header.timestamp>>8)&0xff;
    p_buf[7]=(header.timestamp)&0xff;
    p_buf[8]=(header.conv_id>>24)&0xff;
    p_buf[9]=(header.conv_id>>16)&0xff;
    p_buf[10]=(header.conv_id>>8)&0xff;
    p_buf[11]=(header.conv_id)&0xff;
    return true;
}
bool ikcp_proxy_helper::decode_ikcp_proxy_header(const void*buf,uint32_t available_len,ikcp_proxy_header_s &header)
{
    if(available_len<IKCP_PROXY_HEADER_LEN)return false;
    const uint8_t *p_buf=static_cast<const uint8_t *>(buf);
    memset(&header,0,sizeof (header));
    header.protocal_version=p_buf[0];
    header.cmd=p_buf[1];
    header.sn=p_buf[2];
    header.sn= ((p_buf[3])|(header.sn<<8))&0xffff;
    header.timestamp=p_buf[4];
    header.timestamp= ((p_buf[5])|(header.timestamp<<8));
    header.timestamp= ((p_buf[6])|(header.timestamp<<8));
    header.timestamp= ((p_buf[7])|(header.timestamp<<8));
    header.conv_id=p_buf[8];
    header.conv_id= ((p_buf[9])|(header.conv_id<<8));
    header.conv_id= ((p_buf[10])|(header.conv_id<<8));
    header.conv_id= ((p_buf[11])|(header.conv_id<<8));
    return true;
}
bool ikcp_proxy_helper::encode_ikcp_config(void *buf,uint32_t available_len,const ikcp_config_s&config)
{
    if(available_len<IKCP_CONFIG_LEN)return false;
    memset(buf,0,IKCP_CONFIG_LEN);
    uint8_t *p_buf=static_cast<uint8_t *>(buf);
    p_buf[0]=(config.window_size>>8)&0xff;
    p_buf[1]=(config.window_size)&0xff;
    p_buf[2]=(config.interval>>6)&0xff;
    p_buf[3]|=(config.interval<<2)&0xfc;
    p_buf[3]|=(config.nodelay<<1)&0x2;
    p_buf[3]|=(config.nc)&0x1;
    p_buf[4]=config.resend;
    return true;
}
bool ikcp_proxy_helper::decode_ikcp_config(const void*buf,uint32_t available_len,ikcp_config_s &config)
{
    if(available_len<IKCP_CONFIG_LEN)return false;
    memset(&config,0,sizeof (config));
    const uint8_t *p_buf=static_cast<const uint8_t *>(buf);
    config.window_size=p_buf[0];
    config.window_size=(p_buf[1]|(config.window_size<<8))&0xffff;
    config.interval=p_buf[2];
    config.interval=((p_buf[3]>>2)|(config.interval<<6))&0x3fff;
    config.nodelay=(p_buf[3]>>1)&0x1;
    config.nc=p_buf[3]&0x1;
    config.resend=p_buf[4];
    return true;
}
