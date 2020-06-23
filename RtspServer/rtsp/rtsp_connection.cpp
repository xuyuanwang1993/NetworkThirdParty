#include "rtsp_connection.h"
#include "rtsp_server.h"
#include "rtsp_helper.h"
#include "rtp_connection.h"
#include "network_util.h"
using namespace micagent;
bool rtsp_connection::handle_read()
{
    lock_guard<mutex>locker(m_mutex);
    auto ret=m_read_buf->read_fd(fd());
    if(ret<=0){
        MICAGENT_LOG(LOG_INFO,"%s %d",strerror(errno),errno);
        return false;
    }
    if(ret>0){
        auto size=m_read_buf->get_packet_nums();
        shared_ptr<char[]>buf(new char[8092]);
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
bool rtsp_connection::handle_write()
{
    lock_guard<mutex>locker(m_mutex);
     if(m_send_buf->send_fd(fd())<0)return false;
    if(m_send_buf->get_first_packet_size()!=0){
        if(!m_channel->isWriting()){
            m_channel->enableWriting();
            m_rtsp_server->m_loop->updateChannel(m_channel);
        }
    }
    else {
        if(m_channel->isWriting()){
            m_channel->disableWriting();
            m_rtsp_server->m_loop->updateChannel(m_channel);
        }
    }
    return true;
}
bool rtsp_connection::send_message(const string &buf)
{
    if(!m_send_buf->append(buf.c_str(),buf.length()))return false;
    if(!m_channel->isWriting()){
        m_channel->enableWriting();
        m_rtsp_server->m_loop->updateChannel(m_channel);
    }
    return true;
}
bool rtsp_connection::send_message(const char *buf,uint32_t buf_len)
{
    if(!m_send_buf->append(buf,buf_len))return false;
    if(!m_channel->isWriting()){
        m_channel->enableWriting();
        m_rtsp_server->m_loop->updateChannel(m_channel);
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
        media_session->addClient(fd(),m_rtpConnPtr);
        for(int chn=0; chn<MAX_MEDIA_CHANNEL; chn++)
        {
            auto source = media_session->get_media_source(static_cast<MediaChannelId>(chn));
            if(source != nullptr)
            {
                // 设置时钟频率
                m_rtpConnPtr->setClockRate(static_cast<MediaChannelId>(chn), source->getClockRate());
                // 设置媒体负载类型
                m_rtpConnPtr->setPayloadType(static_cast<MediaChannelId>(chn), source->getPayloadType());
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
                auto interface=NETWORK.get_net_interface_info(false);
                if(!interface.empty())local_ip=interface[0].ip;
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
                    if(m_rtcpChannels[channel_id])m_rtsp_server->m_loop->removeChannel(m_rtcpChannels[channel_id]);
                    m_rtcpChannels[channel_id].reset(new Channel(rtcp_fd));
                    m_rtcpChannels[channel_id]->set_cycle(true);
                    m_rtcpChannels[channel_id]->setReadCallback([](Channel *chn){
                        rtsp_connection::discard_rtcp_packet(chn->fd());
                        return true;
                    });
                    m_rtcpChannels[channel_id]->enableReading();
                    m_rtsp_server->m_loop->updateChannel(m_rtcpChannels[channel_id]);
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
            m_rtpConnPtr->startPlay();
            auto url=handle_map.find("url");
            if(url!=handle_map.end())m_url=url->second;
            m_url="";
            response=rtsp_helper::buildPlayRes(m_rtpConnPtr->get_rtp_Info(m_url).c_str(),session_id,cseq->second);
        }
    }while(0);
    return send_message(response);
handleCmdPlay:
    return false;
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
