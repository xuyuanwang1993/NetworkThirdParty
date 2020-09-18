#include "proxy_connection.h"
#include "proxy_server.h"
#include "MD5.h"
#include "rtsp_server.h"
#include "h264_source.h"
#include "h265_source.h"
#include "aac_source.h"
#include "g711a_source.h"
#define _GLIBCXX_USE_C99 1
using namespace micagent;
//{
//100,    //Not Need Authorization
//101,    //Ilegal Account
//102,		//Authorization Failed
//200,    //OK
//400,		//Bad Request
//401,		//Nonce Is Time Out
//402,		//Stream Is Set Up
//403,		//Stream Name Is In Use
//404,		//Stream Not Found
//500,		//Not Supported Command
//505,    //Internal Error
//506,		//Not SupportedTransmission Mode
//507,		//Unknown Media Type
//}
//{
//cmd : get_authorized_info,
//account : admin,
//seq : 0
//}
//{
//cmd : get_authorized_info_ack,
//status : 200
//info : ok
//seq : 1
//[nonce : -------]
//}

//{
//cmd : authorization
//account : admin,
//nonce : ------,
//password : md5(account+nonce+password),
//seq : 2
//}

//{
//cmd : authorization_ack
//status : 200
//info : ok
//seq : 3
//}

//{
//cmd : set_up_stream,
//stream_name : test,
//trans_mode : raw_udp,
//stream_info :[
//{
//media_name : h264
//media_channel : 0
//frame_rate : 25
//}
//]
//seq : 2
//}

//{
//cmd : set_up_stream_ack,
//status : 200
//info : ok
//seq : 3
//[stream_token : 0,]

//}

//{
//cmd : tear_down_stream
//stream_token : 0,
//seq : 3

//}

//{
//cmd : tear_down_stream_ack
//status : 200
//info : ok
//seq : 4
//}
proxy_connection::proxy_connection(SOCKET fd,proxy_server *server):tcp_connection (fd),m_proxy_server(server),m_recv_buf(new proxy_message(false)),m_send_buf(new proxy_message(true))\
  ,m_is_authorized(false),m_is_setup(false),m_stream_token(INVALID_MediaSessionId),m_last_alive_time(Timer::getTimeNow()),m_last_send_timestamp(p_timer_help::getTimesTamp()),m_last_recv_timestamp(m_last_send_timestamp)
{
    //初始化转发实例，默认只接受TCP包，进行控制命令处理
    m_proxy_interface.reset(new ProxyInterface(INVALID_MediaSessionId,RAW_TCP,[this](const void *buf,uint32_t buf_len){
        return this->send_message(static_cast<const char *>(buf),buf_len);},nullptr,[this](shared_ptr<ProxyFrame>frame){
                                return this->handle_proxy_frame(frame);
                            }));
}
bool proxy_connection::handle_read()
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_proxy_interface)return false;
    auto ret=m_recv_buf->read_fd(fd());
    if(ret==0){
        MICAGENT_LOG(LOG_INFO,"%s %d   ret %d",strerror(errno),errno,ret);
        return false;
    }
    if(ret<0){
        MICAGENT_LOG(LOG_INFO,"%s %d   ret %d",strerror(errno),errno,ret);
        if(errno==EAGAIN||errno == EINTR){
            return true;
        }
        else {
            return false;
        }
    }
    if(ret>0){
        auto size=m_recv_buf->get_packet_nums();
        shared_ptr<char>buf(new char[1500],std::default_delete<char[]>());
        for(uint32_t i=0;i<size;i++){
            memset(buf.get(),0,1500);
            auto len=m_recv_buf->read_packet(buf.get(),1500);
            if(len==0)break;
            //此处会调用帧输出回调函数
            //ProxyInterface::dump_header_info(buf.get(),len);
            if(!m_proxy_interface->protocol_input(buf.get(),len))break;
            m_last_alive_time=Timer::getTimeNow();
        }
    }
    return true;
}
bool proxy_connection::handle_write()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_send_buf->send_fd(fd())<0)return false;
    if(m_send_buf->get_first_packet_size()!=0){
        if(!m_channel->isWriting()){
            m_channel->enableWriting();
            auto loop=m_proxy_server->m_loop.lock();
            if(loop)loop->updateChannel(m_channel);
        }
    }
    else {
        if(m_channel->isWriting()){
            m_channel->disableWriting();
            auto loop=m_proxy_server->m_loop.lock();
            if(loop)loop->updateChannel(m_channel);
        }
    }
    return true;
}
proxy_connection::~proxy_connection()
{
    //移除媒体流
    if(m_is_setup){
        MICAGENT_LOG(LOG_INFO,"remove stream %u\r\n",m_stream_token);
        m_proxy_server->remove_udp_map(m_stream_token);
        auto rtsp_server=m_proxy_server->m_w_rtsp_server.lock();
        if(rtsp_server)rtsp_server->removeMediaSession(m_stream_token);
    }
}
bool proxy_connection::send_message(const char *buf,uint32_t buf_len)
{
    if(!m_send_buf->append(buf,buf_len))return false;
    if(!m_channel->isWriting()){
        m_channel->enableWriting();
        auto loop=m_proxy_server->m_loop.lock();
        if(loop)loop->updateChannel(m_channel);
    }
    return true;
}
bool proxy_connection::handle_proxy_frame(shared_ptr<ProxyFrame> frame)
{
    //未创建完成之前只处理控制帧
    if(!m_is_setup||frame->frame_type==CONTROL_COMMAND)
    {
        return handle_proxy_request(frame);
    }
    else {
        //MICAGENT_LOG(LOG_INFO,"media_data  %hu size %u %02x",frame->frame_seq,frame->data_len,frame->data_buf.get()[0]);
        return handle_proxy_media_data(frame);
    }
}

bool proxy_connection::handle_proxy_request(const shared_ptr<ProxyFrame>&frame)
{
    if(frame->data_len==0)return false;
    CJsonObject object(string(frame->data_buf.get(),frame->data_len));
    string cmd;
    if(!object.Get("cmd",cmd))return false;
    MICAGENT_LOG(LOG_INFO,"server recv %s",string(frame->data_buf.get(),frame->data_len).c_str());
    if(cmd=="get_authorized_info")
    {
        handle_get_authorized_info(object);
    }
    else if(cmd=="authorization")
    {
        handle_authorization(object);
    }
    else if(cmd=="set_up_stream")
    {
        handle_set_up_stream(object);
    }
    else if (cmd=="tear_down_stream") {
        handle_tear_down_stream(object);
    }
    else {
        CJsonObject object2;
        build_json_response(cmd,0,P_NOT_SUPPORTED_COMMAND,"Not Supported Command",object2);
        auto response=object2.ToString();
        m_proxy_interface->send_control_command(response.c_str(),response.length());
    }
    return true;
}
bool proxy_connection::handle_proxy_media_data(const shared_ptr<ProxyFrame>&frame)
{
    //媒体数据帧处理
    auto rtsp_server=m_proxy_server->m_w_rtsp_server.lock();
    if(!rtsp_server)return false;
    else {
        AVFrame new_frame(frame->data_len);
        memcpy(new_frame.buffer.get(),frame->data_buf.get(),frame->data_len);
        new_frame.timestamp=frame->timestamp;
        //MICAGENT_LOG(LOG_INFO,"%02x      %u %u",new_frame.buffer.get()[0],frame->data_len,frame->timestamp);
        return  rtsp_server->updateFrame(m_stream_token,static_cast<MediaChannelId>(frame->media_channel),new_frame);
    }
}
void proxy_connection::handle_get_authorized_info(CJsonObject &object)
{
    CJsonObject response;
    do{
        string account;
        uint32_t seq;
        if(!object.Get("seq",seq)||!object.Get("account",account))
        {
            build_json_response("get_authorized_info_ack",seq,P_BAD_REQUEST,"Bad Request",response);
            break;
        }
        if(m_is_authorized){
            build_json_response("get_authorized_info_ack",seq,P_NOT_NEED_AUTHORIZATION,"Not Need Authorization",response);
            break;
        }
        auto rtsp_server=m_proxy_server->m_w_rtsp_server.lock();
        if(!rtsp_server)
        {
            build_json_response("get_authorized_info_ack",seq,P_INTERNAL_ERROR,"Internal Error",response);
            break;
        }
        string password;
        if(!rtsp_server->getAuthorizationPassword(account,password))
        {
            build_json_response("get_authorized_info_ack",seq,P_ILEGAL_ACCOUNT,"Ilegal Account",response);
            break;
        }
        if(password.empty())
        {
            m_is_authorized=true;
            build_json_response("get_authorized_info_ack",seq,P_NOT_NEED_AUTHORIZATION,"Not Need Authorization",response);
            break;
        }
        build_json_response("get_authorized_info_ack",seq,P_OK,"OK",response);
        response.Add("nonce",to_string(Timer::getTimeNow()));
    }while(0);
    auto res=response.ToString();
    m_proxy_interface->send_control_command(res.c_str(),res.length());
}
void proxy_connection::handle_authorization(CJsonObject &object)
{
    CJsonObject response;
    do{
        if(m_is_authorized){
            build_json_response("authorization_ack",0,P_OK,"OK",response);
            break;
        }
        string account;
        string nonce;
        string password;
        uint32_t seq;
        if(!object.Get("seq",seq)||!object.Get("account",account)||!object.Get("nonce",nonce)||!object.Get("password",password))
        {
            build_json_response("authorization_ack",seq,P_BAD_REQUEST,"Bad Request",response);
            break;
        }
        auto rtsp_server=m_proxy_server->m_w_rtsp_server.lock();
        if(!rtsp_server)
        {
            build_json_response("authorization_ack",seq,P_INTERNAL_ERROR,"Internal Error",response);
            break;
        }
        string a_password;
        if(!rtsp_server->getAuthorizationPassword(account,a_password))
        {
            build_json_response("authorization_ack",seq,P_ILEGAL_ACCOUNT,"Ilegal Account",response);
            break;
        }
        if(a_password.empty())
        {
            m_is_authorized=true;
            build_json_response("authorization_ack",seq,P_OK,"OK",response);
            break;
        }
        int64_t diff_time=Timer::getTimeNow()-stol(nonce);
        if(diff_time<0||diff_time>MAX_NONCE_DIFF)
        {
            build_json_response("authorization_ack",seq,P_NONCE_IS_TIME_OUT,"Nonce Is Time Out",response);
            break;
        }
        auto text=account+nonce+a_password;
        auto tmp=Get_MD5_String(text.c_str(),text.length());
        bool match=password==tmp;
        free(tmp);
        if(!match)
        {
            build_json_response("authorization_ack",seq,P_AUTHORIZATION_FAILED,"Authorization Failed",response);
            break;
        }
        m_is_authorized=true;
        build_json_response("authorization_ack",seq,P_OK,"OK",response);
    }while(0);
    auto res=response.ToString();
    m_proxy_interface->send_control_command(res.c_str(),res.length());
}
void proxy_connection::handle_set_up_stream(CJsonObject &object)
{
    CJsonObject response;
    do{
        if(!m_is_authorized){
            build_json_response("set_up_stream_ack",0,P_AUTHORIZATION_FAILED,"Authorization Failed",response);
            break;
        }
        if(m_is_setup){
            build_json_response("set_up_stream_ack",0,P_STREAM_IS_SET_UP,"Stream Is Set Up",response);
            response.Add("stream_token",m_stream_token);
            break;
        }
        auto rtsp_server=m_proxy_server->m_w_rtsp_server.lock();
        if(!rtsp_server)
        {
            build_json_response("set_up_stream_ack",0,P_INTERNAL_ERROR,"Internal Error",response);
            break;
        }
        uint32_t seq;
        string stream_name;
        string trans_mode;
        CJsonObject stream_info;
        if(!object.Get("seq",seq)||!object.Get("stream_name",stream_name)||!object.Get("trans_mode",trans_mode)||!object.Get("stream_info",stream_info))
        {
            build_json_response("set_up_stream_ack",seq,P_BAD_REQUEST,"Bad Request",response);
            break;
        }
        PTransMode mode;
        if(trans_mode=="raw_udp")mode=RAW_UDP;
        else if (trans_mode=="raw_tcp") mode=RAW_TCP;
        else if (trans_mode=="raw_hybrid") mode=RAW_HYBRID;
        else if(trans_mode=="graded_tcp")mode=GRADED_TCP;
        else if(trans_mode=="graded_hybrid")mode=GRADED_HYBRID;
        else {
            build_json_response("set_up_stream_ack",seq,P_NOT_SUPPORTED_TRANSMISSION_MODE,"Not Supported Transmission Mode",response);
            break;
        }
        auto session=media_session::CreateNew(stream_name);
        auto media_size=stream_info.GetArraySize();
        if(media_size>MAX_MEDIA_CHANNEL)
        {
            build_json_response("set_up_stream_ack",seq,P_UNKNOWN_MEDIA_TYPE,"Unknown Media Type",response);
            break;
        }
        for(int i=0;i<media_size;i++)
        {
            CJsonObject media_object;
            string media_name;
            uint32_t media_channel=0;
            if(!stream_info.Get(i,media_object)||!media_object.Get("media_name",media_name)||!media_object.Get("media_channel",media_channel)\
                    ||media_channel>MAX_MEDIA_CHANNEL)
            {
                build_json_response("set_up_stream_ack",seq,P_BAD_REQUEST,"Bad Request",response);
                break;
            }
            MediaChannelId channel_id=static_cast<MediaChannelId>(media_channel);
            shared_ptr<media_source>source;
            if(media_name=="h264_source")
            {
                uint32_t frame_rate=25;
                if(media_object.Get("frame_rate",frame_rate))
                {
                    source.reset(new h264_source(frame_rate));
                }
                else {
                    source.reset(new h264_source());
                }
            }
            else if(media_name=="h265_source")
            {
                uint32_t frame_rate=25;
                if(media_object.Get("frame_rate",frame_rate))
                {
                    source.reset(new h265_source(frame_rate));
                }
                else {
                    source.reset(new h265_source());
                }
            }
            else if(media_name=="aac_source")
            {
                string proxy_param;
                if(media_object.Get("proxy_param",proxy_param))
                {
                    uint32_t samprate;
                    uint32_t channels;
                    int has_adts;
                    if(sscanf(proxy_param.c_str(),"%u:%u:%d",&samprate,&channels,&has_adts)!=3)
                    {
                        source.reset(new aac_source());
                    }
                    else {
                        source.reset(new aac_source(samprate,channels,has_adts));
                    }
                }
                else {
                    source.reset(new aac_source());
                }
            }
            else if(media_name=="g711a_source")
            {
                source.reset(new g711a_source());
            }
            else {
                MICAGENT_LOG(LOG_INFO,"Unknown media type %s",media_name.c_str());
            }
            if(source)session->setMediaSource(channel_id,source);
        }
        m_stream_token=rtsp_server->addMediaSession(session);
        if(m_stream_token==INVALID_MediaSessionId)
        {
            build_json_response("set_up_stream_ack",seq,P_STREAM_NAME_IS_IN_USE,"Stream Name Is In Use",response);
            break;
        }
        m_proxy_interface.reset(new ProxyInterface(m_stream_token,mode,[this](const void *buf,uint32_t buf_len){
            return this->send_message(static_cast<const char *>(buf),buf_len);},nullptr,[this](shared_ptr<ProxyFrame>frame){
                                    return this->handle_proxy_frame(frame);
                                }));
        if(mode==RAW_UDP||mode==RAW_HYBRID||mode==GRADED_HYBRID)
        {
            //增加映射
            m_proxy_server->add_udp_map(m_stream_token,fd());
        }
        m_is_setup=true;
        build_json_response("set_up_stream_ack",seq,P_OK,"OK",response);
        response.Add("stream_token",m_stream_token);
        response.Add("stream_name",stream_name);
    }while(0);
    auto res=response.ToString();
    m_proxy_interface->send_control_command(res.c_str(),res.length());
}
void proxy_connection::handle_tear_down_stream(CJsonObject &object)
{
    CJsonObject response;
    do{
        if(!m_is_authorized){
            build_json_response("tear_down_stream_ack",0,P_AUTHORIZATION_FAILED,"Authorization Failed",response);
            break;
        }
        auto rtsp_server=m_proxy_server->m_w_rtsp_server.lock();
        if(!rtsp_server)
        {
            build_json_response("tear_down_stream_ack",0,P_INTERNAL_ERROR,"Internal Error",response);
            break;
        }
        uint32_t seq=0;
        uint32_t stream_token=INVALID_MediaSessionId;
        if(!object.Get("seq",seq)||!object.Get("stream_token",stream_token))
        {
            build_json_response("tear_down_stream_ack",seq,P_BAD_REQUEST,"Bad Request",response);
            break;
        }
        if(stream_token!=m_stream_token)
        {
            build_json_response("tear_down_stream_ack",seq,P_STREAM_NOT_FOUND,"Stream Not Found",response);
            break;
        }
        build_json_response("tear_down_stream_ack",seq,P_OK,"OK",response);
        m_proxy_server->remove_connection(fd());
    }while(0);
    auto res=response.ToString();
    m_proxy_interface->send_control_command(res.c_str(),res.length());
}
