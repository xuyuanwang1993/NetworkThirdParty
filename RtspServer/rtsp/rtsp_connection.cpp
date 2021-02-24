#define _GLIBCXX_USE_C99 1
#include "rtsp_connection.h"
#include "rtsp_server.h"
#include "rtsp_helper.h"
#include "rtp_connection.h"
#include "network_util.h"

using namespace micagent;
bool rtsp_connection::handle_read()
{
     lock_guard<mutex>locker(m_mutex);
    switch (m_connection_type) {
    case RTSP_BASE_CONNECTION:
        return rtsp_process_read();
    case RTSP_WEBSOCKET_CONNECTION:
        return websocket_process_read();
    }
}
bool rtsp_connection::rtsp_process_read()
{
    auto ret=m_read_buf->read_fd(fd());
    if(ret<=0){
        MICAGENT_LOG(LOG_INFO,"%s %d   fd:%d",strerror(errno),errno,fd());
        return false;
    }
    if(ret>0){
        auto size=m_read_buf->get_packet_nums();
        shared_ptr<char>buf(new char[8092],std::default_delete<char[]>());
        for(uint32_t i=0;i<size;i++){
            memset(buf.get(),0,8092);
            auto len=m_read_buf->read_packet(buf.get(),8092);
            if(len==0)break;
            if(!handle_rtsp_request(buf.get()))break;
            m_last_alive_time=Timer::getTimeNow();
        }
    }
    return true;
}
bool rtsp_connection::websocket_process_read(){
    auto len=m_websocket_recv_cache->read_fd(fd());
    if(len==0)return false;
    if(len<0){
        if(errno==EAGAIN||errno==EINTR)return true;
        else {
            return false;
        }
    }
    uint32_t packet_len;
    while((packet_len=m_websocket_recv_cache->get_first_packet_size())!=0){
        shared_ptr<uint8_t>packet_buf(new uint8_t[packet_len+1],default_delete<uint8_t[]>());
        auto read_len=m_websocket_recv_cache->read_packet(packet_buf.get(),packet_len);
        auto frame=web_socket_helper::WS_DecodeData(packet_buf.get(),read_len,true);
        websocket_handle_frame(frame);
    }
    return true;
}
bool rtsp_connection::websocket_handle_frame(const WS_Frame &frame)
{
    switch (frame.type) {
    case WS_Frame_Header::WS_CONTINUATION_FRAME:
    case WS_Frame_Header::WS_TEXT_FRAME:
    case WS_Frame_Header::WS_RESERVER_NO_CONTROL_3_FRAME:
    case WS_Frame_Header::WS_RESERVER_NO_CONTROL_4_FRAME:
    case  WS_Frame_Header::WS_RESERVER_NO_CONTROL_5_FRAME:
    case WS_Frame_Header::WS_RESERVER_NO_CONTROL_6_FRAME:
    case WS_Frame_Header::WS_RESERVER_NO_CONTROL_7_FRAME:
    case WS_Frame_Header::WS_CONNECTION_CLOSE_FRAME:
    case WS_Frame_Header::WS_PONG_FRAME:
    case WS_Frame_Header::WS_RESERVER_CONTROL_B_FRAME:
    case WS_Frame_Header::WS_RESERVER_CONTROL_C_FRAME:
    case WS_Frame_Header::WS_RESERVER_CONTROL_D_FRAME:
    case WS_Frame_Header::WS_RESERVER_CONTROL_E_FRAME:
    case WS_Frame_Header::WS_RESERVER_CONTROL_F_FRAME:
        //discard
        break;
    case WS_Frame_Header::WS_BINARY_FRAME:
        return websocket_handle_binary_frame(frame);
    case WS_Frame_Header::WS_PING_FRAME:
        return websocket_handle_ping_frame(frame);
    }
    return true;
}
bool rtsp_connection::websocket_handle_ping_frame(const WS_Frame&frame)
{
    return websocket_send_frame(frame.data.get(),frame.data_len,WS_Frame_Header::WS_PONG_FRAME);
}
bool rtsp_connection::websocket_handle_binary_frame(const WS_Frame &frame)
{
    if(frame.data_len<2)return false;
    auto *data=frame.data.get();
    if(data[0]!=RTSP_WEBSOCKET_VERSION){
        websocket_handle_close();
        return  false;
    }
    switch (data[1]) {
    case R_W_DESCRIBE:
        return websocket_handle_describe(data+2,frame.data_len-2);
    case R_W_CLOSE_STREAM:
        return websocket_handle_close_stream();
    default:
        break;
    }
    return true;
}
bool rtsp_connection::websocket_send_frame(const void *buf,uint32_t buf_len,WS_Frame_Header::WS_FrameType type)
{
    WS_Frame frame(buf,buf_len,type,true,false);
    auto data=web_socket_helper::WS_EncodeData(frame);
    if(!m_send_buf->append(data.first.get(),data.second))return false;
    if(!m_channel->isWriting()){
        m_channel->enableWriting();
        auto event_loop=m_rtsp_server->m_loop.lock();
        if(event_loop)event_loop->updateChannel(m_channel);
    }
    return true;
}
bool rtsp_connection::websocket_handle_close()
{
    return websocket_send_frame(nullptr,0,WS_Frame_Header::WS_CONNECTION_CLOSE_FRAME);
}
//{
//200:OK,//成功
//404:Stream is not found,//流不存在
//400:Authentication failed,//鉴权失败
//500:Server Internal error,//服务器内部错误
//}
bool rtsp_connection::websocket_handle_describe(const void *buf,uint32_t buf_len)
{
    CJsonObject object(string(static_cast<const char *>(buf),buf_len));
    string stream_name;
    if(!object.Get("stream_name",stream_name))return websocket_handle_close();
    string account(""),password("");
    object.Get("account",account);
    object.Get("password",password);

    int status_code=200;
    string status_str("OK");
    CJsonObject response;
    CJsonObject stream_info;
    do{
        {
            bool auth_failed=false;
            //check authentication info
            lock_guard<mutex>locker(m_rtsp_server->m_mtxAccountMap);
            if(!m_rtsp_server->m_Account_Map.empty()){
                if(account.empty())auth_failed=true;
                else {
                    auto iter=m_rtsp_server->m_Account_Map.find(account);
                    if(iter==m_rtsp_server->m_Account_Map.end())auth_failed=true;
                    else {
                        if(iter->second!=password)auth_failed=true;
                    }
                }
            }
            if(auth_failed){
                status_code=400;
                status_str="Authentication failed";
                break;
            }
        }
        {
            //find stream
            auto media_session=m_rtsp_server->lookMediaSession(stream_name);
            if(!media_session){
                status_code=404;
                status_str="Stream is not found";
                break;
            }
            //generate stream_info
            auto media_info=media_session->get_media_source_info();
            if(media_info.empty()){
                status_code=500;
                status_str="Server Internal error";
                break;
            }
            for(auto i:media_info){
                CJsonObject item;
                item.Add("stream_type",i.media_name);
                item.Add("channel",i.channel);
                item.Add("param",i.proxy_param);
                stream_info.Add(item);
            }
            auto conn=shared_from_this();
            m_websocket_status=WEBSOCKET_PLAYING;
            media_session->addWebsocketSink(fd(),conn);
        }
    }while(0);
    response.Add("status_code",status_code);
    response.Add("status_str",status_str);
    if(stream_info.IsEmpty())response.Add("stream_info",stream_info);
    auto ret_str=response.ToString();
    auto ret_len=ret_str.length()+2;
    shared_ptr<char>ret_buf(new char[ret_len+1],default_delete<char[]>());
    memset(ret_buf.get(),0,ret_len);
    ret_buf.get()[0]=RTSP_WEBSOCKET_VERSION;
    ret_buf.get()[1]=R_W_DESCRIBE;
    memcpy(ret_buf.get()+2,ret_str.c_str(),ret_str.length());
    return websocket_send_frame(ret_buf.get(),static_cast<uint32_t>(ret_len),WS_Frame_Header::WS_BINARY_FRAME);
}
bool rtsp_connection::websocket_handle_close_stream()
{
    m_websocket_status=WEBSOCKET_CLOSED;
    size_t ret_len=2;
    shared_ptr<char>ret_buf(new char[ret_len+1],default_delete<char[]>());
    memset(ret_buf.get(),0,ret_len);
    ret_buf.get()[0]=RTSP_WEBSOCKET_VERSION;
    ret_buf.get()[1]=R_W_CLOSE_STREAM;
    return websocket_send_frame(ret_buf.get(),static_cast<uint32_t>(ret_len),WS_Frame_Header::WS_BINARY_FRAME);
}
bool rtsp_connection::handle_write()
{
    lock_guard<mutex>locker(m_mutex);
    if(m_send_buf->send_fd(fd())<0)return false;
    m_last_alive_time=Timer::getTimeNow();
    if(m_send_buf->get_first_packet_size()!=0){
        if(!m_channel->isWriting()){
            m_channel->enableWriting();
            auto event_loop=m_rtsp_server->m_loop.lock();
            if(event_loop)event_loop->updateChannel(m_channel);
        }
    }
    else {
        if(m_channel->isWriting()){
            m_channel->disableWriting();
            auto event_loop=m_rtsp_server->m_loop.lock();
            if(event_loop)event_loop->updateChannel(m_channel);
        }
    }
    return true;
}
bool rtsp_connection::send_message(const string &buf)
{
    if(!m_send_buf->append(buf.c_str(),buf.length()))return false;
    if(!m_channel->isWriting()){
        m_channel->enableWriting();
        auto event_loop=m_rtsp_server->m_loop.lock();
        if(event_loop)event_loop->updateChannel(m_channel);
    }
    return true;
}
bool rtsp_connection::send_message(const char *buf,uint32_t buf_len)
{
    if(!m_send_buf->append(buf,buf_len))return false;
    if(!m_channel->isWriting()){
        m_channel->enableWriting();
        auto event_loop=m_rtsp_server->m_loop.lock();
        if(event_loop)event_loop->updateChannel(m_channel);
    }
    return true;
}
bool rtsp_connection::handle_rtsp_request(const string &buf)
{
    auto handle=rtsp_helper::parseCommand(buf);
    bool ret=false;
    do{
        if(!handle.first)break;
        auto &map_temp=handle.second;
        auto iter=map_temp.find("cmd");
        if(iter==end(map_temp))break;
        if(iter->second=="OPTIONS"){
            if(!handleCmdOption(map_temp))break;
        }
        else if (iter->second=="DESCRIBE") {
            if(!handleCmdDescribe(map_temp))break;
        }
        else if (iter->second=="SETUP") {
            if(!handleCmdSetup(map_temp))break;
        }
        else if (iter->second=="PLAY") {
            if(!handleCmdPlay(map_temp))break;
        }
        else if (iter->second=="TEARDOWN") {
            if(!handleCmdTeardown(map_temp))break;
        }
        else if (iter->second=="GET_PARAMETER") {
            if(!handleCmdGetParamter(map_temp))break;
        }
         else if (iter->second=="GET") {
            if(!handleWebsocketConnection(map_temp))break;
        }
        else if (iter->second=="POST") {
            if(!handleWebsocketConnection(map_temp))break;
        }
        else  {
            auto cseq=map_temp.find("CSeq");
            if(cseq!=end(map_temp)){
                send_message(rtsp_helper::buildUnsupportedRes(cseq->second));
            }
            break;
        }
        ret=true;
    }while(0);
    return ret;
}
bool rtsp_connection::handleCmdOption(map<string ,string>&handle_map)
{
    auto cseq=handle_map.find("CSeq");
    if(cseq==end(handle_map))return false;
    return send_message(rtsp_helper::buildOptionRes(cseq->second));
}
bool rtsp_connection::handleCmdDescribe(map<string ,string>&handle_map)
{
    auto cseq=handle_map.find("CSeq");
    auto url=handle_map.find("url");
    string response;
    do{
        if(cseq==handle_map.end()||url==handle_map.end())goto handleCmdDescribeError;
        auto url_parse=rtsp_helper::parseUrl(url->second);
        if(!url_parse.first)goto handleCmdDescribeError;
        auto suffix=url_parse.second.find("suffix");
        if(suffix==url_parse.second.end())goto handleCmdDescribeError;
        m_url=url->second;
        m_url_suffix=suffix->second;
        auto media_session=m_rtsp_server->lookMediaSession(suffix->second);
        if(!media_session){
            response=rtsp_helper::buildNotFoundRes(cseq->second);
            MICAGENT_LOG(LOG_DEBUG,"can't find %s",suffix->second.c_str());
            break;
        }
        if(!check_authorization_info(handle_map)){
            response=rtsp_helper::buildUnauthorizedRes(cseq->second);
            MICAGENT_LOG(LOG_DEBUG,"401 Unauthorized %s",url->second.c_str());
            break;
        }
        for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
        {
            auto source = media_session->get_media_source(static_cast<MediaChannelId>(chn));
            if(source != nullptr)
            {
                // 设置时钟频率
                m_rtpConnPtr->setClockRate(static_cast<MediaChannelId>(chn), source->getClockRate());
                // 设置媒体负载类型
                m_rtpConnPtr->setPayloadType(static_cast<MediaChannelId>(chn), source->getPayloadType());
                //同步时间戳
                m_rtpConnPtr->setBuildTimesTamp(static_cast<MediaChannelId>(chn), source->getLastSendTime());
            }
        }
        auto sdp_info=media_session->get_sdp_info();
        if(sdp_info==""){
            response=rtsp_helper::buildServerErrorRes(cseq->second);
        }
        else {
            response=rtsp_helper::buildDescribeRes(sdp_info,cseq->second);
        }
    }while(0);

    return send_message(response);
handleCmdDescribeError:
    return false;
}
bool rtsp_connection::handleCmdSetup(map<string ,string>&handle_map)
{
    auto cseq=handle_map.find("CSeq");
    string response;
    do{
        if(cseq==handle_map.end())goto handleCmdSetup;
        if(!m_is_authorized){
            response=rtsp_helper::buildUnauthorizedRes(cseq->second);
            MICAGENT_LOG(LOG_DEBUG,"401 Unauthorized ");
            break;
        }
        auto trackid=handle_map.find("trackid");
        if(trackid==handle_map.end())goto handleCmdSetup;
        MediaChannelId channel_id=static_cast<MediaChannelId>(stoi(trackid->second));
        auto session_ptr=m_rtsp_server->lookMediaSession(m_url_suffix);
        if(!session_ptr){
            response=rtsp_helper::buildServerErrorRes(cseq->second);
            break;
        }
        auto unicast=handle_map.find("unicast");
        if(unicast==handle_map.end())goto handleCmdSetup;
        if(unicast->second=="multicast"){
            if(!session_ptr->isMulticast()){
                response=rtsp_helper::buildUnsupportedRes(cseq->second);
                break;
            }
            auto multicast_ip=session_ptr->getMulticastIp();
            auto multicast_port=session_ptr->getMulticastPort(channel_id);
            if(!m_rtpConnPtr->setupRtpOverMulticast(channel_id,multicast_ip,multicast_port)){
                response=rtsp_helper::buildServerErrorRes(cseq->second);
                break;
            }
            else {
                string local_ip="0.0.0.0";
                auto interface=NETWORK.get_default_net_interface_info();
                if(!interface.ip.empty())local_ip=interface.ip;
                response=rtsp_helper::buildSetupMulticastRes(local_ip,multicast_ip.c_str(),multicast_port,m_rtpConnPtr->get_session_id(),cseq->second);
                break;
            }
        }
        else {
            auto transmode=handle_map.find("transmode");
            if(transmode==end(handle_map))goto handleCmdSetup;
            if(transmode->second=="RTP/AVP/TCP")
            {
                auto rtp_channel=handle_map.find("rtp_channel");
                auto rtcp_channel=handle_map.find("rtcp_channel");
                if(rtp_channel==handle_map.end()||rtcp_channel==handle_map.end())goto handleCmdSetup;
                uint16_t rtp_channel_num=static_cast<uint16_t>(stoul(rtp_channel->second));
                uint16_t rtcp_channel_num=static_cast<uint16_t>(stoul(rtcp_channel->second));
                if(!m_rtpConnPtr->setupRtpOverTcp(channel_id,rtp_channel_num,rtcp_channel_num)){
                    response=rtsp_helper::buildServerErrorRes(cseq->second);
                    break;
                }
                else {
                    response=rtsp_helper::buildSetupTcpRes(rtp_channel_num,rtcp_channel_num,m_rtpConnPtr->get_session_id(),cseq->second);
                }

            }
            else if (transmode->second=="RTP/AVP") {
                auto rtp_port=handle_map.find("rtp_port");
                auto rtcp_port=handle_map.find("rtcp_port");
                if(rtp_port==handle_map.end()||rtcp_port==handle_map.end())goto handleCmdSetup;
                uint16_t rtp_port_num=static_cast<uint16_t>(stoul(rtp_port->second));
                uint16_t rtcp_port_num=static_cast<uint16_t>(stoul(rtcp_port->second));
                if(!m_rtpConnPtr->setupRtpOverUdp(channel_id,rtp_port_num,rtcp_port_num)){
                    response=rtsp_helper::buildServerErrorRes(cseq->second);
                    break;
                }
                else {
                    auto rtcp_fd=m_rtpConnPtr->get_rtcp_fd(channel_id);
                    auto event_loop=m_rtsp_server->m_loop.lock();
                    if(m_rtcpChannels[channel_id]&&event_loop)event_loop->removeChannel(m_rtcpChannels[channel_id]);
                    m_rtcpChannels[channel_id].reset(new Channel(rtcp_fd));
                    m_rtcpChannels[channel_id]->set_cycle(true);
                    m_rtcpChannels[channel_id]->setReadCallback([](Channel *chn){
                        rtsp_connection::discard_rtcp_packet(chn->fd());
                        return true;
                    });
                    m_rtcpChannels[channel_id]->enableReading();
                    if(event_loop)event_loop->updateChannel(m_rtcpChannels[channel_id]);
                }
                auto r_rtp_port=m_rtpConnPtr->get_rtp_port(channel_id);
                auto r_rtcp_port=m_rtpConnPtr->get_rtcp_port(channel_id);
                response=rtsp_helper::buildSetupUdpRes(rtp_port_num,rtcp_port_num,r_rtp_port,r_rtcp_port,m_rtpConnPtr->get_session_id(),cseq->second);
            }
            else {
                response=rtsp_helper::buildUnsupportedRes(cseq->second);
                break;
            }
        }
    }while(0);
    return send_message(response);
handleCmdSetup:
    return false;
}
bool rtsp_connection::handleCmdPlay(map<string ,string>&handle_map)
{
    auto cseq=handle_map.find("CSeq");
    bool ret=false;
    string response;
    do{
        if(cseq==handle_map.end())goto handleCmdPlay;
        auto Session=handle_map.find("Session");
        if(Session==handle_map.end())goto handleCmdPlay;
        uint32_t session_id=static_cast<uint32_t>(stoul(Session->second));
        if(session_id!=m_rtpConnPtr->get_session_id()){
            response=rtsp_helper::buildServerErrorRes(cseq->second);
            break;
        }
        else {
            auto url=handle_map.find("url");
            if(url!=handle_map.end())m_url=url->second;
            response=rtsp_helper::buildPlayRes(m_rtpConnPtr->get_rtp_Info(m_url).c_str(),session_id,cseq->second,rtsp_server::CONNECTION_TIME_OUT/1000);
            ret=send_message(response);
            m_rtpConnPtr->startPlay();
            auto session_ptr=m_rtsp_server->lookMediaSession(m_url_suffix);
            if(session_ptr)
            {
                session_ptr->addClient(fd(),m_rtpConnPtr);
            }
            string peer_ip=NETWORK.get_peer_ip(fd());
            uint16_t peer_port=NETWORK.get_peer_port(fd());
            auto loop=m_rtsp_server->m_loop.lock();
            if(loop)
            {
                auto url=m_url;
                auto cb1=m_rtsp_server->m_new_connection_callback;
                loop->add_trigger_event([peer_ip,peer_port,url,cb1](){
                    if(cb1){
                        cb1(peer_ip,peer_port,url);
                    }
                });
            }
        }
    }while(0);
handleCmdPlay:
    return ret;
}
bool rtsp_connection::handleCmdTeardown(map<string ,string>&handle_map)
{
    m_rtpConnPtr->stopPlay();
    auto cseq=handle_map.find("CSeq");
    if(cseq!=handle_map.end())return send_message(rtsp_helper::buildTeardownRes(m_rtpConnPtr->get_session_id(),cseq->second));
    else {
        return send_message(rtsp_helper::buildTeardownRes(m_rtpConnPtr->get_session_id()));
    }
}
bool rtsp_connection::handleCmdGetParamter(map<string ,string>&handle_map)
{
    auto cseq=handle_map.find("CSeq");
    if(cseq!=handle_map.end())return send_message(rtsp_helper::buildGetParamterRes(m_rtpConnPtr->get_session_id(),cseq->second));
    else {
        return send_message(rtsp_helper::buildGetParamterRes(m_rtpConnPtr->get_session_id()));
    }
}
bool rtsp_connection::check_authorization_info(map<string ,string>&handle_map,const string &cmd_name)
{
    if(m_is_authorized)return true;
    {
        lock_guard<mutex>locker(m_rtsp_server->m_mtxAccountMap);
        if(m_rtsp_server->m_Account_Map.empty()){
            m_is_authorized=true;
            return true;
        }
    }

    auto username=handle_map.find("username");
    auto realm=handle_map.find("realm");
    auto nonce=handle_map.find("nonce");
    auto uri=handle_map.find("uri");
    auto response=handle_map.find("response");
    if(username==handle_map.end()||\
            realm==handle_map.end()||\
            nonce==handle_map.end()||\
            uri==handle_map.end()||\
            response==handle_map.end()){
        return false;
    }
    string password;
    {
        lock_guard<mutex>locker(m_rtsp_server->m_mtxAccountMap);
        auto iter=m_rtsp_server->m_Account_Map.find(username->second);
        if(iter==m_rtsp_server->m_Account_Map.end())return false;
        password=iter->second;
    }
    m_is_authorized=rtsp_helper::checkAuthentication(username->second,password,cmd_name,realm->second,nonce->second,uri->second,response->second);
    return m_is_authorized;
}
void rtsp_connection::discard_rtcp_packet(SOCKET rtcp_fd)
{
    char buf[1024] = {0};
    if(recv(rtcp_fd, buf, 1024, 0) > 0)
    {

    }
}
bool rtsp_connection::handleWebsocketConnection(map<string ,string>&handle_map)
{
    auto key=handle_map.find("Sec-WebSocket-Key");
    if(key==handle_map.end())return false;
    m_connection_type=RTSP_WEBSOCKET_CONNECTION;
    m_websocket_recv_cache.reset(new web_socket_buffer_cache(1000));
    return  send_message(rtsp_helper::buildWebsocketRes(key->second));
}
void rtsp_connection::websocket_forward_stream(MediaChannelId channel,const AVFrame &frame)
{
    if(m_websocket_status!=WEBSOCKET_PLAYING)return;
    auto ret_len=frame.size+2+1+4;
    shared_ptr<uint8_t>ret_buf(new uint8_t[ret_len+1],default_delete<uint8_t[]>());
    memset(ret_buf.get(),0,ret_len);
    ret_buf.get()[0]=RTSP_WEBSOCKET_VERSION;
    ret_buf.get()[1]=R_W_STREAM_DATA;
    ret_buf.get()[2]=channel;
    auto timestamp=frame.timestamp;
    ret_buf.get()[3]=(timestamp>>24)&0xff;
    ret_buf.get()[4]=(timestamp>>16)&0xff;
    ret_buf.get()[5]=(timestamp>>8)&0xff;
    ret_buf.get()[6]=(timestamp)&0xff;
    memcpy(ret_buf.get()+7,frame.buffer.get(),frame.size);
    websocket_send_frame(ret_buf.get(),static_cast<uint32_t>(ret_len),WS_Frame_Header::WS_BINARY_FRAME);
}
rtsp_connection::~ rtsp_connection()
{

}
