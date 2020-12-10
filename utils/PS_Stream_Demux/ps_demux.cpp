#include "ps_demux.h"
using namespace micagent;
#define PS_DEBUG 1
#if PS_DEBUG
#define PS_DEMUX_LOG(fmt,...) do{\
    printf("[%s %s %d]" fmt " \r\n",__FILE__,__func__,__LINE__, ##__VA_ARGS__);\
    }while(0);
#else
#define PS_DEMUX_LOG(fmt,...)
#endif
#pragma pack (1)
typedef union littel_endian_size_s {
    unsigned short int	length;
    unsigned char		byte[2];
} littel_endian_size;
typedef struct pack_start_code_s {
    unsigned char stream_id[1];
} pack_start_code;
typedef struct program_stream_pack_header_s {
    pack_start_code PackStart;// 4
    unsigned char Buf[9];
    unsigned char stuffinglen;
} program_stream_pack_header;
typedef struct program_stream_map_s {
    pack_start_code PackStart;
    littel_endian_size PackLength;//we mast do exchange
    //program_stream_info_length
    //info
    //elementary_stream_map_length
    //elem
} program_stream_map_start;
typedef struct program_stream_map_pes_packet {
    program_stream_map_start map_start;
    char				PackInfo1[2];
    littel_endian_size	stream_info_len;
} program_stream_map_pes;
#pragma pack ()
//end gb28181 defines
ps_stream_filter::ps_stream_filter(uint32_t max_cache_size,uint32_t max_cache_counts):buf_len(max_cache_size),use_len(0),last_status(0),cache_counts(max_cache_counts==0?1:max_cache_counts)\
,last_find_pos(0),have_find_header(false)
{
    cache_buf.reset(new uint8_t[buf_len+1],default_delete<uint8_t[]>());
}
bool ps_stream_filter::append(const void *buf, uint32_t input_buf_len)
{
    if(input_buf_len>buf_len){
        PS_DEMUX_LOG("ps_stream_filter buffer overflow!");
        return  false;
    }
    if(buf_len-use_len<input_buf_len){
        PS_DEMUX_LOG("ps_stream_filter buffer overflow!");
        //clear status
        clear();
    }
    uint32_t check_len=use_len;
    //copy data
    memcpy(cache_buf.get()+use_len,buf,input_buf_len);
    use_len+=input_buf_len;
    while(check_len<use_len)
    {
        last_status=(last_status<<8)|cache_buf.get()[check_len];
        if(last_status==PS_PACKET_START_CODE){
            if(have_find_header){
                //save a complete ps frame
                if(m_frame_cache.size()>=cache_counts&&!m_frame_cache.empty()){
                    m_frame_cache.pop_front();
                }
                uint32_t new_frame_len=check_len-4-last_find_pos;
                shared_ptr<uint8_t>new_frame_buf(new uint8_t[new_frame_len+1],default_delete<uint8_t[]>());
                memcpy(new_frame_buf.get(),cache_buf.get()+last_find_pos,new_frame_len);
                m_frame_cache.push_back(make_pair(new_frame_buf,new_frame_len));
            }
            if(check_len>3){
                have_find_header=true;
                last_find_pos=check_len-4;
            }
        }
        check_len++;
    }
    if(have_find_header){
        //copy buf
        auto left_len=use_len-last_find_pos;
        shared_ptr<uint8_t>tmp_buf(new uint8_t[left_len+1],default_delete<uint8_t[]>());
        memcpy(tmp_buf.get(),cache_buf.get()+last_find_pos,left_len);
        last_find_pos=0;
        use_len=left_len;
        memcpy(cache_buf.get(),tmp_buf.get(),left_len);
    }
    return !m_frame_cache.empty();
}
pair<shared_ptr<uint8_t>, uint32_t> ps_stream_filter::read_frame()
{
    if(m_frame_cache.empty())return  {nullptr,0};
    auto ret=m_frame_cache.front();
    m_frame_cache.pop_front();
    return ret;
}
constexpr static size_t ps_map_header_len=sizeof (program_stream_map_start);
constexpr static size_t ps_header_len=sizeof (program_stream_pack_header);
constexpr static size_t pes_program_map_len=sizeof (program_stream_map_pes);
ps_demux_output ps_demux::parse_ps_stream(const void *ps_frame_buf,uint32_t buf_len)
{
    uint32_t pos=0;
    const auto  frame=static_cast<const uint8_t *>(ps_frame_buf);
    const auto length=buf_len;

    if(!m_probe_finished){
        if(m_find_system_header&&(m_video_confirmed||m_audio_confirmed)){
            m_probe_finished=true;
        }
    }
    ps_demux_output output;
    output.is_probe_finished=m_probe_finished;
    output.is_config_changed=false;
    while(search_start_code(frame,pos,length)){
#if PS_DEBUG
        //PS_DEMUX_LOG("%x %x %x %x ",frame[0+pos],frame[1+pos],frame[2+pos],frame[3+pos]);
#endif
        //去除ps头
        if(frame[pos]==0XBA){
            if(pos+ps_header_len>length)break;
            auto PsHead = reinterpret_cast<const program_stream_pack_header*>(frame+pos);
            unsigned char pack_stuffing_length = PsHead->stuffinglen & 0x07;
            pos+= ps_header_len +pack_stuffing_length;
        }
        //去除系统头
        else if (frame[pos]==0XBB) {
            if(pos+ps_map_header_len>length)break;
            auto PSMPack = reinterpret_cast<const program_stream_map_start*>(frame+pos);
            littel_endian_size psm_length;
            psm_length.byte[0] = PSMPack->PackLength.byte[1];
            psm_length.byte[1] = PSMPack->PackLength.byte[0];
            pos+=psm_length.length+ps_map_header_len;
        }
        else {
            if(!parse_pes_packet(frame,pos,length,output))break;
        }
    }//while
    return output;
}
uint32_t ps_demux::decode_u32(const uint8_t input[4] )
{
    return ((input[0]<<24)|(input[1]<<16)|(input[2]<<8)|input[3])&0xffffffff;
}
void ps_demux::get_payload_type(uint8_t key_byte)
{
    switch (key_byte) {
    case 0x10://MPEG-4
        PS_DEMUX_LOG(" stream_type is MPEG-4...\n");
        break;
    case 0x1b://h264
        if(m_video_type!=PS_STREAM_H264){
            PS_DEMUX_LOG("stream_type is H.264...\n");
            m_video_confirmed=false;
        }
        m_video_type=PS_STREAM_H264;
        break;
    case 0x80://SVAC
        PS_DEMUX_LOG("stream_type is SVAC...\n");
        break;
    case 0x24:
        if(m_video_type!=PS_STREAM_H265)PS_DEMUX_LOG("stream_type is H.265...\n");
        if(m_video_type!=PS_STREAM_H265)m_video_confirmed=false;
        m_video_type=PS_STREAM_H265;
        break;
    case 0x91:
        if(m_audio_type!=PS_STREAM_PCM)PS_DEMUX_LOG("stream_type is G711ulaw...\n");
        if(m_audio_type!=PS_STREAM_PCM)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_PCM;
        break;
    case 0x90:
        if(m_audio_type!=PS_STREAM_PCM)PS_DEMUX_LOG("stream_type is G711alaw...\n");
        if(m_audio_type!=PS_STREAM_PCM)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_PCM;
        break;
    case 0x96:
        if(m_audio_type!=PS_STREAM_PCM)PS_DEMUX_LOG("stream_type is G726...\n");
        if(m_audio_type!=PS_STREAM_PCM)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_PCM;
        break;
    case 0x0f:
        if(m_audio_type!=PS_STREAM_AAC)PS_DEMUX_LOG("stream_type is aac...\n");
        if(m_audio_type!=PS_STREAM_AAC)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_AAC;
        break;
    case 0x9c:
        if(m_audio_type!=PS_STREAM_PCM)PS_DEMUX_LOG("stream_type is pcm...\n");
        if(m_audio_type!=PS_STREAM_PCM)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_PCM;
        break;
    case 0x92:
        if(m_audio_type!=PS_STREAM_PCM)PS_DEMUX_LOG("stream_type is G722.1...\n");
        if(m_audio_type!=PS_STREAM_PCM)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_PCM;
        break;
    case 0x93:
        if(m_audio_type!=PS_STREAM_PCM)PS_DEMUX_LOG("stream_type is G723...\n");
        if(m_audio_type!=PS_STREAM_PCM)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_PCM;
        break;
    case 0x99:
        if(m_audio_type!=PS_STREAM_PCM)PS_DEMUX_LOG("stream_type is G729...\n");
        if(m_audio_type!=PS_STREAM_PCM)m_audio_confirmed=false;
        m_audio_type=PS_STREAM_PCM;
        break;
    default:
        PS_DEMUX_LOG("stream_type %02x is unknown...\n",key_byte);
        break;
    }
}
bool ps_demux::search_start_code(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    uint32_t code_status=PS_CODE_STATUS_MASK;
    bool find=false;
    if(total_len-start_pos<6) return  find;
    while(start_pos<total_len)
    {
        code_status=((code_status<<8)|buf_start[start_pos++])&PS_CODE_STATUS_MASK;
        if(code_status==PS_START_CODE_VALUE){
            find=true;
            break;
        }
    }
    return find;
}
bool ps_demux::parse_pes_packet(const uint8_t *buf_start, uint32_t &start_pos, uint32_t total_len, ps_demux_output &output)
{
    //PS_DEMUX_LOG("%u %02x %02x %02x %02x %02x",total_len,buf_start[start_pos],buf_start[start_pos+1],buf_start[start_pos+2],buf_start[start_pos+3],buf_start[start_pos+4]);
    if(buf_start[start_pos]>=PS_STREAM_ID_GBT2_VIDEO_START&&buf_start[start_pos]<=PS_STREAM_ID_GBT2_VIDEO_END)
    {
        return parse_video_pes_packet(buf_start,start_pos,total_len,output);
    }
    else if (buf_start[start_pos]>=PS_STREAM_ID_GBT3_AUDIO_START&&buf_start[start_pos]<=PS_STREAM_ID_GBT3_AUDIO_END) {
        return  parse_audio_pes_packet(buf_start,start_pos,total_len,output);
    }
    else if (buf_start[start_pos]>=PS_STREAM_ID_RESERVERED_STREAM_START&&buf_start[start_pos]<=PS_STREAM_ID_RESERVERED_STREAM_END) {
        return parse_reservered_pes_packet(buf_start,start_pos,total_len);
    }
    else {
        switch (buf_start[start_pos]) {
        case  PS_STREAM_ID_PROGRAM_STREAM_MAP:
            return  parse_program_map(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_PRIVATE_STREAM_1:
            return parse_private_stream_1(buf_start,start_pos,total_len,output);
        case  PS_STREAM_ID_PADDING_STREAM:
            return parse_padding_stream(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_PRIVATE_STREAM_2:
            return parse_private_stream_2(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ECM_STREAM:
            return parse_ecm_stream(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_EMM_STREAM:
            return parse_emm_stream(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_GB_T_XXXX6_DSMCC_STREAM:
            return parse_gb_t_xxxx6_dsmcc_stream(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ISO_IEC_13522_STREAM:
            return parse_iso_iec_13522_stream(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ITU_T_REC_H222_1_A:
            return parse_itu_t_recv_h222_1_a(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ITU_T_REC_H222_1_B:
            return parse_itu_t_recv_h222_1_b(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ITU_T_REC_H222_1_C:
            return parse_itu_t_recv_h222_1_c(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ITU_T_REC_H222_1_D:
            return parse_itu_t_recv_h222_1_d(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ITU_T_REC_H222_1_E:
            return parse_itu_t_recv_h222_1_e(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_ANCILLARY_STREAM:
            return parse_ancillary_stream(buf_start,start_pos,total_len);
        case  PS_STREAM_ID_PROGRAM_STREAM_DIRECTORY:
            return parse_program_stream_directory(buf_start,start_pos,total_len);
        default:
            PS_DEMUX_LOG("unknown stream_id %02x!",buf_start[start_pos]);
            return false;
        }
    }
}
int64_t ps_demux::get_pes_pts(const uint8_t input[5] )
{
    int64_t pts=0;
    //读取第30-32位
    pts|=((input[0]>>1)&0x7)<<30;
    //读取第22-29位
    pts|=(input[1]&0xff)<<22;
    //读取第15-21位
    pts|=((input[2]>>1)&0x7f)<<15;
    //读取第7-14位
    pts|=(input[3]&0xff)<<7;
    //读取第0-6位
    pts|=((input[4]>>1)&0x7f);
    return pts;
}
bool ps_demux::parse_program_map(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    //流映射信息，可以从此处获得视音频信息
    //part 1: 1 byte header 0xBC
    //part 2: 2 byte  紧跟着的map部分总长度
    //part 3: 2 byte 无关紧要的信息
    //part 4: 2 byte 描述字段的长度n
    //part 5: n byte 描述字段
    //part 6: 2 byte 流信息部分的长度
    //part 7: 多个流信息
    //单个流信息结构
    //part 1 : 1 byte 流类型
    //part 2 : 1 byte 流映射id
    //part3 : 2 byte 描述字段长度n
    //part4 : n byte 描述字段
    bool ret=false;
    do{
        if(start_pos+pes_program_map_len>total_len)break;
        auto PSMPack = reinterpret_cast<const program_stream_map_pes*>(buf_start+start_pos);
        //获取map部分总长度
        littel_endian_size psm_length;
        psm_length.byte[0] = PSMPack->map_start.PackLength.byte[1];
        psm_length.byte[1] = PSMPack->map_start.PackLength.byte[0];
        if(start_pos+ps_map_header_len+psm_length.length>total_len)break;
        //此方法是从PSM包对应的byte位中取出stream_type
        m_find_system_header=true;
        //获取描述部分长度
        littel_endian_size info_len;
        info_len.byte[1] = PSMPack->stream_info_len.byte[0];
        info_len.byte[0] = PSMPack->stream_info_len.byte[1];
        uint32_t psm_offset=pes_program_map_len;
        psm_offset+=info_len.length;
        littel_endian_size stream_map_len;
        stream_map_len.byte[1] = buf_start[start_pos+psm_offset];
        stream_map_len.byte[0] = buf_start[start_pos+psm_offset+1];
        psm_offset+=2;
        while(1)
        {//parse stream info
            if(psm_offset+4>=ps_map_header_len+psm_length.length)break;
            uint8_t stream_type = buf_start[start_pos+psm_offset];
            get_payload_type(stream_type);
            psm_offset+=2;
            littel_endian_size map_info_len;
            map_info_len.byte[1] = buf_start[start_pos+psm_offset];
            map_info_len.byte[0] = buf_start[start_pos+psm_offset+1];
            psm_offset+=2+map_info_len.length;
        }
        start_pos+=psm_length.length+ps_map_header_len;
        ret=true;
    }while(0);
    return ret;
}
bool ps_demux::skip_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    //following two byte is the data length
    start_pos++;
    uint16_t len=0;
    len=buf_start[start_pos];
    len=((len<<8)|buf_start[start_pos+1])&0xffff;
    start_pos+=2;
    start_pos+=len;
    return  start_pos<total_len;
}
/*stream pes packet
 *stream_id  1byte
 *pes packet length 2 byte  -> following pes packet's length
 *discarded 1byte
 *header flags 1 byte
 * header 's length 1 byte
 * header info+header's stuff byte
 * pes raw data
 */
bool ps_demux::parse_video_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len,ps_demux_output &output)
{

    start_pos++;//move to pes length field
    uint16_t pes_len=0;
    pes_len=buf_start[start_pos];
    pes_len=((pes_len<<8)|buf_start[start_pos+1])&0xffff;
    start_pos+=2;//move to discarded byte
    //check overflow
    uint32_t pes_end_pos=start_pos+pes_len;
    if(pes_end_pos>total_len){
        //perhaps miss some rtp packets
        //PS_DEMUX_LOG("PES packet overflow %u %u!",pes_end_pos,total_len);
        pes_end_pos=total_len;
    }
    start_pos++;//discard this copy rigth byte
    auto header_flag=buf_start[start_pos];
    start_pos++;//move to header len field
    uint8_t pts_dts_flags=(header_flag>>6)&0x3;
    uint8_t ESCR_flag=(header_flag>>5)&0x1;
    uint8_t ES_rate_flag=(header_flag>>4)&0x1;
    uint8_t DSM_trick_mode_flag=(header_flag>>3)&0x1;
    uint8_t additional_copy_info_flag=(header_flag>>2)&0x1;
    uint8_t PES_CRC_flag=(header_flag>>1)&0x1;
    uint8_t PES_extension_flag=header_flag&0x1;
    uint8_t header_len=buf_start[start_pos];
    ++start_pos;//move to header field
    uint32_t header_end=start_pos+header_len;
    if(header_end>pes_end_pos){
        PS_DEMUX_LOG("PES header overflow!");
        return false;
    }
    if(pts_dts_flags>=0x2){
        //read pts
        //check first byte must be pts_dts_flags start
        uint8_t check=(buf_start[start_pos]>>4)&0xf;
        if(check!=pts_dts_flags){
            PS_DEMUX_LOG("PES header parse pts check error!");
            return false;
        }
        m_last_video_pts=get_pes_pts(buf_start+start_pos);
        if(pts_dts_flags==0x3)
        {//check dts
            uint8_t check=(buf_start[start_pos]>>4)&0xf;
            if(check!=0x1){
                PS_DEMUX_LOG("PES header parse dts check error!");
                return false;
            }
        }
    }
    if(ESCR_flag){
        //discard ESCR
        start_pos+=PS_STREAM_ESCR_LENGTH;
    }
    if(ES_rate_flag){
        //discard ES_RATE
        start_pos+=PS_STREAM_ES_RATE_LENGTH;
    }
    if(DSM_trick_mode_flag){
        //discard DSM_trick_mode
        start_pos+=PS_STREAM_DSM_TRICK_MODE_LENGTH;
    }
    if(additional_copy_info_flag){
        start_pos+=PS_STREAM_ADDTIONAL_COPY_INFO_LENGTH;
    }
    if(PES_CRC_flag){
        start_pos+=PS_STREAM_PES_CRC_LENGTH;
    }
    if(PES_extension_flag){
        if(start_pos>header_end){
            PS_DEMUX_LOG("PES header parse PES_extension check error!");
            return false;
        }
        uint8_t PES_private_data_flag=(buf_start[start_pos]>>7)&0x1;
        uint8_t pack_header_field_flag=(buf_start[start_pos]>>6)&0x1;
        uint8_t program_packet_sequence_counter_flag=(buf_start[start_pos]>>5)&0x1;
        uint8_t STD_buffer_flag=(buf_start[start_pos]>>4)&0x1;
        uint8_t PES_extension_flag_2=(buf_start[start_pos])&0x1;
        start_pos+=PS_STREAM_PES_EXTENSION_LENGTH;
        if(PES_private_data_flag){
            start_pos+=PS_STREAM_PES_PRIVATE_DATA_LENGTH;
        }
        if(pack_header_field_flag){
            if(start_pos>header_end){
                PS_DEMUX_LOG("PES header parse pack_header_field check error!");
                return false;
            }
            uint8_t pack_field_length=buf_start[start_pos];
            pack_field_length=12;
            start_pos+=pack_field_length+PS_STREAM_PACK_HEADER_FIELD_LENGTH;
        }
        if(program_packet_sequence_counter_flag){
            start_pos+=PS_STREAM_PROGRAM_PACKER_SEQUENCE_COUNTER_LENGTH;
        }
        if(STD_buffer_flag){
            start_pos+=PS_STREAM_P_STD_BUFFER_LENGTH;
        }
        if(PES_extension_flag_2){
            if(start_pos>header_end){
                PS_DEMUX_LOG("PES header parse PES_extension check error!");
                return false;
            }
            uint8_t PES_extension_field_length=buf_start[start_pos]&0x7f;

            start_pos+=PS_STREAM_PES_EXTENSTION_FLAG_2_LENGTH+PES_extension_field_length;
        }
    }
    if(start_pos>header_end){
        PS_DEMUX_LOG("PES header parse check error!");
        return false;
    }
    auto packet_start=header_end;
    auto packet_len=pes_end_pos-packet_start;
    if(packet_len>PS_MAX_MEDIA_FRAME_SIZE){
        PS_DEMUX_LOG("PES header parse packet over flow !");
        return false;
    }
    if(!m_video_confirmed){
        m_video_confirmed=true;
        output.is_config_changed=true;
    }
    //save packet
    do{
        if(output.frame_list.empty())
        {
            if(packet_len<4)break;
            auto start_code=decode_u32(buf_start+packet_start);
            if(start_code!=PS_PACKET_VIDEO_START)break;
            output.frame_list.push_back(make_shared<ps_stream_frame>(buf_start+packet_start,packet_len,PS_FRAME_VIDEO,static_cast<int64_t>((m_last_video_pts>>1)/45.0)));
        }
        else {
            if(packet_len<4){
                if(output.frame_list.rbegin()->get()->frame_type!=PS_FRAME_VIDEO)break;
                output.frame_list.rbegin()->get()->ps_frame_append(buf_start+packet_start,packet_len);
            }
            else {
                auto start_code=decode_u32(buf_start+packet_start);
                if(start_code!=PS_PACKET_VIDEO_START){
                    output.frame_list.rbegin()->get()->ps_frame_append(buf_start+packet_start,packet_len);
                }
                else {
                    output.frame_list.push_back(make_shared<ps_stream_frame>(buf_start+packet_start,packet_len,PS_FRAME_VIDEO,static_cast<int64_t>((m_last_video_pts>>1)/45.0)));
                }
            }
        }
    }while(0);

#if 0
    auto stuffing_len=header_end-start_pos;
    PS_DEMUX_LOG("%u %u %u",stuffing_len,packet_len,total_len);

    //    FILE *fp=fopen("test.h264","a+");
    //    if(fp){
    //        fwrite(buf_start+packet_start,1,packet_len,fp);
    //        fclose(fp);
    //    }
    if(packet_len>20)packet_len=20;
    for(uint32_t i=0;i<packet_len;i++)
    {
        printf("%02x ",buf_start[packet_start+i]);
    }
    printf("\r\n");
#endif
    start_pos+=packet_len;
    return true;
}
bool ps_demux::parse_audio_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len,ps_demux_output &output)
{
    start_pos++;//move to pes length field
    uint16_t pes_len=0;
    pes_len=buf_start[start_pos];
    pes_len=((pes_len<<8)|buf_start[start_pos+1])&0xffff;
    start_pos+=2;//move to discarded byte
    //check overflow
    uint32_t pes_end_pos=start_pos+pes_len;
    if(pes_end_pos>total_len){
        //perhaps miss some rtp packets
        PS_DEMUX_LOG("PES packet overflow %u %u!",pes_end_pos,total_len);
        pes_end_pos=total_len;
    }
    if(m_audio_type==PS_STREAM_UNKNOWN){
        start_pos=pes_end_pos;
        PS_DEMUX_LOG("PES packet discard unknown audio packet!");
        return true;
    }
    start_pos++;//discard this copy rigth byte
    auto header_flag=buf_start[start_pos];
    start_pos++;//move to header len field
    uint8_t pts_dts_flags=(header_flag>>6)&0x3;
    uint8_t ESCR_flag=(header_flag>>5)&0x1;
    uint8_t ES_rate_flag=(header_flag>>4)&0x1;
    uint8_t DSM_trick_mode_flag=(header_flag>>3)&0x1;
    uint8_t additional_copy_info_flag=(header_flag>>2)&0x1;
    uint8_t PES_CRC_flag=(header_flag>>1)&0x1;
    uint8_t PES_extension_flag=header_flag&0x1;
    uint8_t header_len=buf_start[start_pos];
    ++start_pos;//move to header field
    uint32_t header_end=start_pos+header_len;
    if(header_end>pes_end_pos){
        PS_DEMUX_LOG("PES header overflow!");
        return false;
    }
    if(pts_dts_flags>=0x2){
        //read pts
        //check first byte must be pts_dts_flags start
        uint8_t check=(buf_start[start_pos]>>4)&0xf;
        if(check!=pts_dts_flags){
            PS_DEMUX_LOG("PES header parse pts check error!");
            return false;
        }
        m_last_audio_pts=get_pes_pts(buf_start+start_pos);
        if(pts_dts_flags==0x3)
        {//check dts
            uint8_t check=(buf_start[start_pos]>>4)&0xf;
            if(check!=0x1){
                PS_DEMUX_LOG("PES header parse dts check error!");
                return false;
            }
        }
    }
    if(ESCR_flag){
        //discard ESCR
        start_pos+=PS_STREAM_ESCR_LENGTH;
    }
    if(ES_rate_flag){
        //discard ES_RATE
        start_pos+=PS_STREAM_ES_RATE_LENGTH;
    }
    if(DSM_trick_mode_flag){
        //discard DSM_trick_mode
        start_pos+=PS_STREAM_DSM_TRICK_MODE_LENGTH;
    }
    if(additional_copy_info_flag){
        start_pos+=PS_STREAM_ADDTIONAL_COPY_INFO_LENGTH;
    }
    if(PES_CRC_flag){
        start_pos+=PS_STREAM_PES_CRC_LENGTH;
    }
    if(PES_extension_flag){
        if(start_pos>header_end){
            PS_DEMUX_LOG("PES header parse PES_extension check error!");
            return false;
        }
        uint8_t PES_private_data_flag=(buf_start[start_pos]>>7)&0x1;
        uint8_t pack_header_field_flag=(buf_start[start_pos]>>6)&0x1;
        uint8_t program_packet_sequence_counter_flag=(buf_start[start_pos]>>5)&0x1;
        uint8_t STD_buffer_flag=(buf_start[start_pos]>>4)&0x1;
        uint8_t PES_extension_flag_2=(buf_start[start_pos])&0x1;
        start_pos+=PS_STREAM_PES_EXTENSION_LENGTH;
        if(PES_private_data_flag){
            start_pos+=PS_STREAM_PES_PRIVATE_DATA_LENGTH;
        }
        if(pack_header_field_flag){
            if(start_pos>header_end){
                PS_DEMUX_LOG("PES header parse pack_header_field check error!");
                return false;
            }
            uint8_t pack_field_length=buf_start[start_pos];
            start_pos+=pack_field_length+PS_STREAM_PACK_HEADER_FIELD_LENGTH;
        }
        if(program_packet_sequence_counter_flag){
            start_pos+=PS_STREAM_PROGRAM_PACKER_SEQUENCE_COUNTER_LENGTH;
        }
        if(STD_buffer_flag){
            start_pos+=PS_STREAM_P_STD_BUFFER_LENGTH;
        }
        if(PES_extension_flag_2){
            if(start_pos>header_end){
                PS_DEMUX_LOG("PES header parse PES_extension check error!");
                return false;
            }
            uint8_t PES_extension_field_length=buf_start[start_pos]&0x7f;

            start_pos+=PS_STREAM_PES_EXTENSTION_FLAG_2_LENGTH+PES_extension_field_length;
        }
    }
    if(start_pos>header_end){
        PS_DEMUX_LOG("PES header parse check error!");
        return false;
    }
    auto packet_start=header_end;
    auto packet_len=pes_end_pos-packet_start;
    if(packet_len>PS_MAX_MEDIA_FRAME_SIZE){
        PS_DEMUX_LOG("PES header parse packet over flow !");
        return false;
    }
    start_pos+=packet_len;
    if(!m_audio_confirmed){
        m_audio_confirmed=true;
        output.is_config_changed=true;
    }
    //save packet
    do{
        if(m_audio_type==PS_STREAM_AAC){
            auto old_sample_rate=m_audio_sample_rate;
            auto old_channels=m_audio_channels;
            m_audio_sample_rate=get_aac_samprate((buf_start[packet_start+2]>>2)&0xf);
            m_audio_channels=static_cast<uint32_t>(((buf_start[packet_start+2]&0x1)<<2)|((buf_start[packet_start+3]>>6)&0x3));
            if(old_sample_rate!=m_audio_sample_rate||old_channels!=m_audio_channels){
                output.is_config_changed=true;
            }
        }
        else {
            m_audio_channels=1;
            m_audio_sample_rate=8000;
        }
        if(m_audio_sample_rate>0)output.frame_list.push_back(make_shared<ps_stream_frame>(buf_start+packet_start,packet_len,PS_FRAME_AUDIO,static_cast<int64_t>((m_last_audio_pts>>1)*1000/(m_audio_sample_rate/2))));
    }while(0);
    return true;
}
bool ps_demux::parse_reservered_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_private_stream_1(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len,ps_demux_output &)
{//ac3 audio
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_padding_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_private_stream_2(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_ecm_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_emm_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_gb_t_xxxx6_dsmcc_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_iso_iec_13522_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_itu_t_recv_h222_1_a(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_itu_t_recv_h222_1_b(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_itu_t_recv_h222_1_c(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_itu_t_recv_h222_1_d(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_itu_t_recv_h222_1_e(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_ancillary_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
bool ps_demux::parse_program_stream_directory(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len)
{
    return  skip_pes_packet(buf_start,start_pos,total_len);
}
static uint32_t AACSampleRate[16] =
{
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0 /*reserved */
};
uint8_t ps_demux::get_aac_index(uint32_t samprate)
{
    for(uint8_t index=0;index<16;index++){
        if(samprate==AACSampleRate[index])return index;
    }
    return PS_INVALID_AAC_INDEX;
}
uint32_t ps_demux::get_aac_samprate(uint8_t index)
{
    if(index>15)return PS_INVALID_AAC_SAMPRATE;
    return  AACSampleRate[index];
}
