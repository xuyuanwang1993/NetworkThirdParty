#include "media_session.h"
#include "rtsp_connection.h"
#define ENABLE_GOP_CACHE 1
using namespace micagent;
atomic<MediaSessionId> media_session::s_session_id(0);

proxy_session_base::~proxy_session_base()
{

}
shared_ptr<media_session>media_session::CreateNew(const string &rtsp_suffix,bool is_multicast)
{
    auto ret=make_shared<media_session>(rtsp_suffix);
    if(ret&&is_multicast)ret->initMulticast();
    return ret;
}
void media_session::setMediaSource(MediaChannelId id,shared_ptr<media_source>source)
{
    if(source)source->setSendFrameCallback([this](MediaChannelId channelId, RtpPacket pkt){
        int count=0;
        for(auto i=m_rtp_connections.begin();i!=m_rtp_connections.end();){
            auto ptr=i->second.lock();
            if(!ptr){
                m_rtp_connections.erase(i++);
                if(m_notice_client_nums_callback)m_notice_client_nums_callback(m_session_id,get_client_nums());
            }
            else {
                if((pkt.type==FRAME_I||pkt.type==FRAME_SPS||pkt.type==FRAME_VPS)&&ptr->is_Playing(channelId))ptr->set_see_idr();
                if(ptr->get_see_idr()||pkt.type!=FRAME_P)
                {
                    if(ptr->sendRtpPacket(channelId,pkt))
                    {
                        count++;
                        i++;
                        if(isMulticast())break;
                    }else {
                        m_rtp_connections.erase(i++);
                        if(m_notice_client_nums_callback)m_notice_client_nums_callback(m_session_id,get_client_nums());
                    }

                }else {
                    i++;
                }
            }
        }
        return count>0||m_rtp_connections.empty();});
    lock_guard<mutex>locker(m_mutex);
    m_media_source[id].reset();
    m_media_source[id]=source;
    m_sdp="";
}
void media_session::removeMediaSource(MediaChannelId id)
{
    lock_guard<mutex>locker(m_mutex);
    m_media_source[id].reset();
}

void media_session::setFrameRate(uint32_t frame_rate)
{
    lock_guard<mutex>locker(m_mutex);
    MICAGENT_LOG(LOG_INFO,"set frame %d",frame_rate);
    for(int n=0; n<MAX_MEDIA_CHANNEL; n++)
    {
        if(m_media_source[n])m_media_source[n]->setFrameRate(frame_rate);
    }
}

uint32_t media_session::getClientsNums()const
{
    lock_guard<mutex>locker(m_mutex);
    return m_rtp_connections.size();
}
bool media_session::updateFrame(MediaChannelId channel,const AVFrame &frame)
{
#ifdef SAVE_FILE_ACCESS
    static uint8_t start_code[4]={0x00,0x00,0x00,0x01};
    if(m_save_fp)
    {
        fwrite(start_code,1,4,m_save_fp);
        fwrite(frame.buffer.get(),1,frame.size,m_save_fp);
        fflush(m_save_fp);
    }
#endif
    lock_guard<mutex>locker(m_mutex);
    //转发
    for(auto iter=m_proxy_session_map.begin();iter!=m_proxy_session_map.end();)
    {
        auto strong=iter->lock();
        if(strong)
        {
            strong->proxy_frame(channel,frame);
            iter++;
        }
        else {
            m_proxy_session_map.erase(iter++);
        }
    }
    for(auto iter=m_websocket_connections.begin();iter!=m_websocket_connections.end();)
    {
        auto strong=iter->second.lock();
        if(strong)
        {
            auto ptr=strong.get();
            auto websocket_connection=dynamic_cast<rtsp_connection *>(ptr);
            if(websocket_connection)websocket_connection->websocket_forward_stream(channel,frame);
            iter++;
        }
        else {
            m_websocket_connections.erase(iter++);
            if(m_notice_client_nums_callback)m_notice_client_nums_callback(m_session_id,get_client_nums());
        }
    }
    if(m_has_new_client){
#if ENABLE_GOP_CACHE
        for(uint32_t i=0;i<MAX_MEDIA_CHANNEL;++i)
        {
            for(auto j:m_rtp_connections)
            {
                auto s_rtp_connection=j.second.lock();
                if(s_rtp_connection&&s_rtp_connection->is_Playing(static_cast<MediaChannelId>(i))&&!s_rtp_connection->get_got_gop()&&!s_rtp_connection->get_see_idr())
                {
                    if(!m_media_source[static_cast<MediaChannelId>(i)]->handleGopCache(static_cast<MediaChannelId>(i),s_rtp_connection)||isMulticast())break;
                }
            }
        }
        m_has_new_client=false;
#endif

    }
    if(m_media_source[channel])return m_media_source[channel]->handleFrame(channel,frame);
    else return false;
}

void media_session::initMulticast()
{
    do{
        if (m_is_multicast)break;
        m_multicast_ip = MulticastAddr::instance().getAddr();
        if (m_multicast_ip == "")break;
        std::random_device rd;
        m_multicastPort[channel_0] = htons(rd() & 0xfffe);
        m_multicastPort[channel_1] = htons(rd() & 0xfffe);
        m_is_multicast=true;
    }while(0);
}

bool media_session::addClient(SOCKET rtspfd, std::shared_ptr<rtp_connection> rtpConnPtr)
{
    lock_guard<mutex>locker(m_mutex);
    m_has_new_client=true;
    auto iter=m_rtp_connections.find(rtspfd);
    if(iter!=end(m_rtp_connections)){
        m_rtp_connections.erase(iter);
    }
    auto ret=m_rtp_connections.emplace(rtspfd,weak_ptr<rtp_connection>(rtpConnPtr));
    if(m_notice_client_nums_callback)m_notice_client_nums_callback(m_session_id,get_client_nums());
    return ret.second;
}
void media_session::addWebsocketSink(SOCKET rtspfd, weak_ptr<tcp_connection> websocket_sink)
{
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_websocket_connections.find(rtspfd);
    if(iter==end(m_websocket_connections)){
        m_websocket_connections.emplace(rtspfd,websocket_sink);
        if(m_notice_client_nums_callback)m_notice_client_nums_callback(m_session_id,get_client_nums());
    }
}
void media_session::notice_new_connection()
{
    {
        lock_guard<mutex>locker(m_mutex);
        m_has_new_client=true;
    }
}
void  media_session::addProxySession(shared_ptr<proxy_session_base> session, bool proxy)
{
    session->open_connection(get_media_source_info());
    if(proxy){
        lock_guard<mutex>locker(m_mutex);
        m_proxy_session_map.push_back(session);
    }
}
void media_session::removeClient(SOCKET rtspfd)
{
    lock_guard<mutex>locker(m_mutex);
    m_rtp_connections.erase(rtspfd);
    if(m_notice_client_nums_callback)m_notice_client_nums_callback(m_session_id,get_client_nums());
}
string media_session::get_sdp_info (const string & version)
{
    lock_guard<mutex>locker(m_mutex);
    if(!m_sdp.empty())return m_sdp;
    if(m_media_source.empty())
        return "";
    std::string ip("0.0.0.0");
    auto interface= Network_Util::Instance().get_default_net_interface_info();
    ip=interface.ip;
    char buf[2048] = {0};

    snprintf(buf, sizeof(buf),
             "v=0\r\n"
             "o=- 9%ld 1 IN IP4 %s\r\n"
             "t=0 0\r\n"
             "a=control:*\r\n" ,
             (long)std::time(NULL), ip.c_str());

    if(version != "")
    {
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                 "s=%s\r\n",
                 version.c_str());
    }

    if(m_is_multicast)
    {
        snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                 "a=type:broadcast\r\n"
                 "a=rtcp-unicast: reflection\r\n");
    }

    for (uint32_t chn=0; chn<m_media_source.size(); chn++)
    {
        if(m_media_source[chn])
        {
            if(m_is_multicast)
            {
                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                         "%s\r\n",
                         m_media_source[chn]->getMediaDescription(m_multicastPort[chn]).c_str());

                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                         "c=IN IP4 %s/255\r\n",
                         m_multicast_ip.c_str());
            }
            else
            {
                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                         "%s\r\n",
                         m_media_source[chn]->getMediaDescription(0).c_str());
            }

            snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                     "%s\r\n",
                     m_media_source[chn]->getAttribute().c_str());

            //getAttributeFmtp will add "\r\n" if it's not empty
            auto fmtp=m_media_source[chn]->getAttributeFmtp();
            if(!fmtp.empty()){
                snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                         "%s",
                         fmtp.c_str());
            }
            snprintf(buf+strlen(buf), sizeof(buf)-strlen(buf),
                     "a=control:track%d\r\n", chn);
        }
    }

    m_sdp = buf;
    return m_sdp;
}
vector<media_source_info>media_session::get_media_source_info()const
{
    vector<media_source_info> ret;
    for (uint32_t chn=0; chn<m_media_source.size(); chn++)
    {
        media_source_info source_info;
        source_info.channel=static_cast<MediaChannelId>(chn);
        if(m_media_source[chn])
        {
            source_info.frame_rate=m_media_source[chn]->getFrameRate();
            source_info.media_name=m_media_source[chn]->get_media_name();
            source_info.proxy_param=m_media_source[chn]->get_proxy_param();
            ret.push_back(source_info);
        }
    }
    return ret;
}
media_session::media_session(string rtsp_suffix):m_suffix(rtsp_suffix),m_session_id(generate_session_id()),m_is_multicast(false)\
  ,m_multicast_ip(""),m_sdp(""),m_has_new_client(false),m_notice_client_nums_callback(nullptr)
{
#ifdef SAVE_FILE_ACCESS
    m_save_fp=fopen((string(SAVE_FILE_PRFIX)+to_string(m_session_id)).c_str(),"w+");
    if(!m_save_fp)
    {
        throw std::runtime_error("exec this program with root access!");
    }
#endif
    m_media_source.resize(MAX_MEDIA_CHANNEL);
    for(int n=0; n<MAX_MEDIA_CHANNEL; n++)
    {
        m_multicastPort[n] = 0;
    }
}
media_session::~media_session()
{
    if(m_is_multicast)MulticastAddr::instance().release(m_multicast_ip);
#ifdef SAVE_FILE_ACCESS
    if(m_save_fp)fclose(m_save_fp);
#endif
}
