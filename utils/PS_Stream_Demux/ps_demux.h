#ifndef PS_DEMUX_H
#define PS_DEMUX_H
#include <cstdint>
#include<memory>
#include<list>
#include<cstring>
namespace micagent {
using namespace std;
/**
 * @brief The ps_demux class to demux ps stream,separate video frame and audio frame,get video type ,audio type,audio's param,get frame's pts
 * @author xuyuanwang
 */
enum ps_stream_type:uint8_t{
    PS_STREAM_AAC,
    PS_STREAM_PCM,
    PS_STREAM_H264,
    PS_STREAM_H265,
    PS_STREAM_UNKNOWN,
};
enum ps_frame_type:uint8_t{
    PS_FRAME_VIDEO,
    PS_FRAME_AUDIO,
    PS_FRAME_UNKNOWN,
};
struct ps_stream_frame{
    int64_t timestamp_ms;
    ps_frame_type frame_type;
    uint32_t frame_len;
    shared_ptr<uint8_t>frame_buf;
    ps_stream_frame(const void *input,uint32_t input_len,ps_frame_type type,int64_t timestamp=0)\
        :timestamp_ms(timestamp),frame_type(type),frame_len(input_len),frame_buf(new uint8_t[frame_len+1],default_delete<uint8_t[]>()){
        memset(frame_buf.get(),0,input_len);
        memcpy(frame_buf.get(),input,input_len);
    }
    void ps_frame_append(const void *input,uint32_t input_len){
        auto new_len=input_len+frame_len;
        uint8_t *new_buf=new uint8_t[new_len+1];
        //copy old
        memset(new_buf,0,new_len);
        memcpy(new_buf,frame_buf.get(),frame_len);
        //copy new
        memcpy(new_buf+frame_len,input,input_len);
        frame_len=new_len;
        frame_buf.reset(new_buf,default_delete<uint8_t[]>());
    }
};

struct ps_demux_output{
    list<shared_ptr<ps_stream_frame>>frame_list;
    bool is_probe_finished=false;
    //this flag will be set while stream param is changed
    bool is_config_changed=false;
};
struct ps_stream_param{
    ps_stream_type video_type;
    ps_stream_type audio_type;
    uint32_t audio_sample_rate;
    uint32_t audio_channels;
};
enum ps_demux_const:uint32_t{
    PS_PROBE_FRAMES=4,
    PS_CODE_STATUS_MASK=0xffffff,
    PS_START_CODE_VALUE=0x000001,
    //define stream id
    PS_STREAM_ID_PROGRAM_STREAM_MAP=0xBC,
    PS_STREAM_ID_PRIVATE_STREAM_1=0xBD, //ac3 audio
    PS_STREAM_ID_PADDING_STREAM=0XBE,
    PS_STREAM_ID_PRIVATE_STREAM_2=0XBF,
    PS_STREAM_ID_GBT3_AUDIO_START=0XC0,
    PS_STREAM_ID_GBT3_AUDIO_END=0XDF,
    PS_STREAM_ID_GBT2_VIDEO_START=0XE0,
    PS_STREAM_ID_GBT2_VIDEO_END=0XEF,
    PS_STREAM_ID_ECM_STREAM=0XF0,
    PS_STREAM_ID_EMM_STREAM=0XF1,
    PS_STREAM_ID_GB_T_XXXX6_DSMCC_STREAM=0XF2,
    PS_STREAM_ID_ISO_IEC_13522_STREAM=0XF3,
    PS_STREAM_ID_ITU_T_REC_H222_1_A=0XF4,
    PS_STREAM_ID_ITU_T_REC_H222_1_B=0XF5,
    PS_STREAM_ID_ITU_T_REC_H222_1_C=0XF6,
    PS_STREAM_ID_ITU_T_REC_H222_1_D=0XF7,
    PS_STREAM_ID_ITU_T_REC_H222_1_E=0XF8,
    PS_STREAM_ID_ANCILLARY_STREAM=0XF9,
    PS_STREAM_ID_RESERVERED_STREAM_START=0XFA,
    PS_STREAM_ID_RESERVERED_STREAM_END=0XFE,
    PS_STREAM_ID_PROGRAM_STREAM_DIRECTORY=0XFF,
    //filed length
    PS_STREAM_ESCR_LENGTH=6,
    PS_STREAM_ES_RATE_LENGTH=6,
    PS_STREAM_DSM_TRICK_MODE_LENGTH=1,
    PS_STREAM_ADDTIONAL_COPY_INFO_LENGTH=1,
    PS_STREAM_PES_CRC_LENGTH=2,
    PS_STREAM_PES_EXTENSION_LENGTH=1,
    PS_STREAM_PES_PRIVATE_DATA_LENGTH=16,
    PS_STREAM_PACK_HEADER_FIELD_LENGTH=1,
    PS_STREAM_PROGRAM_PACKER_SEQUENCE_COUNTER_LENGTH=2,
    PS_STREAM_P_STD_BUFFER_LENGTH=2,

    PS_STREAM_PES_EXTENSTION_FLAG_2_LENGTH=2,
    //PS_START CODE define
    PS_PACKET_START_CODE=0x000001ba,
    PS_PACKET_VIDEO_START=0x00000001,
    //AAC
    PS_INVALID_AAC_INDEX=15,
    PS_INVALID_AAC_SAMPRATE= 0,
    //FRAME SIZE
    PS_MAX_MEDIA_FRAME_SIZE=10*1024*1024,
};
struct ps_stream_filter{
    ps_stream_filter(uint32_t max_cache_size=1024*1024,uint32_t max_cache_counts=1);
    shared_ptr<uint8_t>cache_buf;
    const uint32_t buf_len;
    uint32_t use_len;
    uint32_t last_status;
    const uint32_t cache_counts;
    list<pair<shared_ptr<uint8_t>,uint32_t>>m_frame_cache;
    uint32_t last_find_pos;
    bool have_find_header;
    bool append(const void *buf,uint32_t input_buf_len);
    pair<shared_ptr<uint8_t>,uint32_t>read_frame();
    void clear(){
        use_len=0;
        last_status=0;
        have_find_header=false;
    }
};

class ps_demux{
public:
    ps_demux():m_find_system_header(false),m_probe_finished(false),\
        m_last_video_pts(0),m_last_audio_pts(0)\
      ,m_video_type(PS_STREAM_UNKNOWN),m_audio_type(PS_STREAM_UNKNOWN),m_audio_sample_rate(0),m_audio_channels(0){}
    virtual ~ps_demux(){}
    ps_demux(const ps_demux &)=delete ;
    ps_stream_param get_ps_stream_param()const{
        ps_stream_param param;
        if(m_video_confirmed)param.video_type=m_video_type;
        else {
            param.video_type=PS_STREAM_UNKNOWN;
        }
        if(m_audio_confirmed)param.audio_type=m_audio_type;
        else {
            param.audio_type=PS_STREAM_UNKNOWN;
        }
        param.audio_channels=m_audio_channels;
        param.audio_sample_rate=m_audio_sample_rate;
        return param;
    }
    ps_demux_output parse_ps_stream(const void *ps_frame_buf,uint32_t buf_len);
    static  uint32_t decode_u32(const uint8_t input[4]);
    static uint8_t get_aac_index(uint32_t samprate);
    static uint32_t get_aac_samprate(uint8_t index);
private:
    void get_payload_type(uint8_t key_byte);
    bool search_start_code(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    bool parse_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len,ps_demux_output &output);
    int64_t get_pes_pts(const uint8_t input[5]);
protected:
    bool skip_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_program_map(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_video_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len,ps_demux_output &output);
    virtual bool parse_audio_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len,ps_demux_output &output);
    virtual bool parse_reservered_pes_packet(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_private_stream_1(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len,ps_demux_output &output);
    virtual bool parse_padding_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_private_stream_2(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_ecm_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_emm_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_gb_t_xxxx6_dsmcc_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_iso_iec_13522_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_itu_t_recv_h222_1_a(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_itu_t_recv_h222_1_b(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_itu_t_recv_h222_1_c(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_itu_t_recv_h222_1_d(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_itu_t_recv_h222_1_e(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_ancillary_stream(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
    virtual bool parse_program_stream_directory(const uint8_t *buf_start,uint32_t &start_pos,uint32_t total_len);
private:
    bool m_find_system_header;
    bool m_probe_finished;
    int64_t m_last_video_pts;
    int64_t m_last_audio_pts;
    ps_stream_type m_video_type;
    bool m_video_confirmed;
    ps_stream_type m_audio_type;
    bool m_audio_confirmed;
    uint32_t m_audio_sample_rate;
    uint32_t m_audio_channels;
};
}
#endif // PS_DEMUX_H
