#include "rtsp_pusher.h"
#include "MD5.h"
using namespace micagent;
rtsp_pusher::rtsp_pusher(weak_ptr<tcp_connection_helper> helper, PTransMode mode, const string &des_name, const string &des_ip , uint16_t des_port, const string &user_name="", const string &password=""):\
    proxy_session_base(),m_connection_helper(helper),m_mode(mode),m_des_name(des_name),m_des_ip(des_ip),m_des_port(des_port),m_user_name(user_name),m_pass_word(password)\
  ,m_media_info_set(false),m_media_info(MAX_MEDIA_CHANNEL,PNONE),m_is_connecting(false),m_udp_fd(INVALID_SOCKET),m_last_time_out(MIN_WAIT_TIME),m_is_authorized(false),\
    m_is_setup(false),m_is_closed(false),m_stream_token(INVALID_MediaSessionId),m_seq(0),m_recv_buf(new proxy_message(false)),m_send_buf(new proxy_message(true))
{
    memset(&m_des_addr,0,sizeof (m_des_addr));
    m_des_addr.sin_family=AF_INET;
    m_des_addr.sin_addr.s_addr=inet_addr(m_des_ip.c_str());
    m_des_addr.sin_port=htons(m_des_port);
    if(mode==RAW_UDP||mode==RAW_HYBRID||mode==GRADED_HYBRID)
    {
        m_udp_fd=NETWORK.build_socket(UDP);
    }
    m_proxy_interface.reset(new ProxyInterface(INVALID_MediaSessionId,RAW_TCP,[this](const void *buf,uint32_t buf_len){
        //TCP协议包发送处理
        return this->send_message(static_cast<const char *>(buf),buf_len);},[this](const void *buf,uint32_t buf_len){
        //UDP数据包发送处理
        if(m_udp_fd==INVALID_SOCKET)return false;
        return NETWORK.time_out_sendto(m_udp_fd,buf,buf_len,0,(struct sockaddr *)&m_des_addr,sizeof(struct sockaddr_in),1)==buf_len;
    },[this](shared_ptr<ProxyFrame>frame){
                                return this->handle_proxy_request(frame);
                            }));
}
void rtsp_pusher::proxy_frame(MediaChannelId id,const AVFrame &frame)
{
    lock_guard<mutex>locker(m_mutex);
        if(m_is_setup){
            shared_ptr<ProxyFrame>proxy_frame(new ProxyFrame(frame.buffer.get(),frame.size,m_media_info[id],0,id));
            proxy_frame->timestamp=static_cast<uint32_t>(frame.timestamp);
            m_proxy_interface->send_frame(proxy_frame);
        }
}
void rtsp_pusher::parse_source_info(const vector<media_source_info>&media_source_info)
{

    if(!media_source_info.empty())
    {
        for(auto i: media_source_info)
        {
            PMediaTYpe type=PNONE;
            if(i.media_name=="h264_source")type=PH264;
            else if(i.media_name=="h265_source")type=PH265;
            else if(i.media_name=="aac_source")type=PAAC;
            else if(i.media_name=="g711a_source")type=PPCMA;
            m_media_info[i.channel]=type;
            CJsonObject object;
            object.Add("media_name",i.media_name);
            object.Add("media_channel",i.channel);
            object.Add("frame_rate",i.frame_rate);
            object.Add("proxy_param",i.proxy_param);
            m_media_json_info.Add(object);
        }
        m_media_info_set=true;
    }
}
void rtsp_pusher::open_connection(const vector<media_source_info>&media_source_info)
{
    lock_guard<mutex>locker(m_mutex);
    m_is_closed=false;
    if(!m_media_info_set)parse_source_info(media_source_info);
    reset_connection();
}
void rtsp_pusher::close_connection()
{
    lock_guard<mutex>locker(m_mutex);
    m_is_closed=true;
    if(m_tcp_channel)send_tear_down_stream();
}
void rtsp_pusher::reset_net_info(string ip,uint16_t des_port)
{
    lock_guard<mutex>locker(m_mutex);
    m_des_ip=ip;
    m_des_port=des_port;
    m_des_addr.sin_addr.s_addr=inet_addr(m_des_ip.c_str());
    m_des_addr.sin_port=htons(m_des_port);
    reset_connection();
}
void rtsp_pusher::reset_connection()
{
    if(!m_media_info_set||m_is_connecting||m_is_closed)return;
    m_is_connecting=true;
    if(m_tcp_channel){
        auto connection_helper=m_connection_helper.lock();
        if(connection_helper){

            auto loop=connection_helper->get_loop();
            auto event_loop=loop.lock();
            if(event_loop)event_loop->removeChannel(m_tcp_channel);
        }
        m_tcp_channel.reset();
    }
    m_is_authorized=false;
    m_is_setup=false;
    m_stream_token=INVALID_MediaSessionId;
    m_seq=0;
    weak_ptr<rtsp_pusher>weak_this(shared_from_this());
    auto connection_helper=m_connection_helper.lock();
    if(connection_helper){
        connection_helper->OpenConnection(m_des_ip,m_des_port,[weak_this](CONNECTION_STATUS status,SOCKET fd){
            auto strong=weak_this.lock();
            if(strong){
                strong->handle_connect(status,fd);
            }
            else {
                MICAGENT_LOG(LOG_INFO,"puhser released!");
                if(status==CONNECTION_SUCCESS){
                    if(fd!=INVALID_SOCKET)NETWORK.close_socket(fd);
                }
            }
        },m_last_time_out);
        m_last_time_out+=1000;
        m_last_time_out=m_last_time_out>MAX_WAIT_TIME?MAX_WAIT_TIME:m_last_time_out;
    }
}
void rtsp_pusher::handle_connect(CONNECTION_STATUS status,SOCKET fd)
{
    if(status==CONNECTION_SYS_ERROR)throw runtime_error("build proxy task error!");
    else if(status==CONNECTION_SUCCESS){
        //连接成功后的处理
        lock_guard<mutex>locker(m_mutex);
        if(m_is_closed||m_tcp_channel){
            NETWORK.close_socket(fd);
            return ;
        }
        Network_Util::Instance().set_ignore_sigpipe(fd);
        Network_Util::Instance().set_tcp_keepalive(fd,true);
        m_last_time_out=MIN_WAIT_TIME;
        m_is_connecting=false;
        m_tcp_channel.reset(new Channel(fd));
        weak_ptr<rtsp_pusher>weak_this(shared_from_this());
        m_tcp_channel->setReadCallback([weak_this](Channel *chn){
            auto strong=weak_this.lock();
            if(!strong)return false;
            lock_guard<mutex>locker(strong->m_mutex);
            if(!strong->m_proxy_interface)return false;
            auto ret=strong->m_recv_buf->read_fd(chn->fd());
            if(ret==0){
                MICAGENT_LOG(LOG_INFO,"%s %d  fd(%d)",strerror(errno),errno,chn->fd());
                return false;
            }
            if(ret>0){
                auto size=strong->m_recv_buf->get_packet_nums();
                shared_ptr<char>buf(new char[8092],std::default_delete<char[]>());
                for(uint32_t i=0;i<size;i++){
                    memset(buf.get(),0,8092);
                    auto len=strong->m_recv_buf->read_packet(buf.get(),8092);
                    if(len==0)break;
                    if(!strong->m_proxy_interface->protocol_input(buf.get(),len))break;
                }
            }
            return true;
        });
        m_tcp_channel->setWriteCallback([weak_this](Channel *chn){
            auto strong=weak_this.lock();
            if(!strong)return false;
            lock_guard<mutex>locker(strong->m_mutex);
            if(strong->m_send_buf->send_fd(chn->fd())<0)return false;
            if(strong->m_send_buf->get_first_packet_size()!=0){
                if(!chn->isWriting()){
                    chn->enableWriting();
                    auto connection_helper=strong->m_connection_helper.lock();
                    if(connection_helper)
                    {
                        auto event_loop=connection_helper->get_loop().lock();
                        if(event_loop){
                            event_loop->updateChannel(strong->m_tcp_channel);
                        }
                    }
                }
            }
            else {
                if(chn->isWriting()){
                    chn->disableWriting();
                    auto connection_helper=strong->m_connection_helper.lock();
                    if(connection_helper)
                    {
                        auto event_loop=connection_helper->get_loop().lock();
                        if(event_loop){
                            event_loop->updateChannel(strong->m_tcp_channel);
                        }
                    }
                }
            }
            return true;
        });
        m_tcp_channel->setCloseCallback([weak_this](Channel *chn){
        (void)chn;
            auto strong=weak_this.lock();
            if(!strong)return false;
            unique_lock<mutex>locker(strong->m_mutex);
            strong->reset_connection();
            return true;
        });
        m_tcp_channel->setErrorCallback([weak_this](Channel *chn){
            (void)chn;
            auto strong=weak_this.lock();
            if(!strong)return false;
            unique_lock<mutex>locker(strong->m_mutex);
            strong->reset_connection();
            return true;
        });
        m_tcp_channel->enableReading();
        auto connection_helper=m_connection_helper.lock();
        if(!connection_helper)return;
        auto loop=connection_helper->get_loop().lock();
        if(loop){
            m_proxy_interface.reset(new ProxyInterface(INVALID_MediaSessionId,RAW_TCP,[this](const void *buf,uint32_t buf_len){
                //TCP协议包发送处理
                return this->send_message(static_cast<const char *>(buf),buf_len);},[this](const void *buf,uint32_t buf_len){
                //UDP数据包发送处理
                if(m_udp_fd==INVALID_SOCKET)return false;
                return NETWORK.time_out_sendto(m_udp_fd,buf,buf_len,0,(struct sockaddr *)&m_des_addr,sizeof(struct sockaddr_in),1)==buf_len;
            },[this](shared_ptr<ProxyFrame>frame){
                                        return this->handle_proxy_request(frame);
                                    }));
            loop->updateChannel(m_tcp_channel);
            send_get_authorized_info();
        }
    }
    else {
        lock_guard<mutex>locker(m_mutex);
        weak_ptr<rtsp_pusher>weak_this(shared_from_this());
        auto connection_helper=m_connection_helper.lock();
        if(!connection_helper)return;
        auto loop=connection_helper->get_loop().lock();
        if(!loop)throw runtime_error("event loop was quit!");
        MICAGENT_LOG(LOG_DEBUG,"%d connect %s  %hu  error for %s!  wait %u ms!",fd,m_des_ip.c_str(),m_des_port,strerror(errno),m_last_time_out);
        if(fd!=INVALID_SOCKET)NETWORK.close_socket(fd);
        loop->addTimer([weak_this](){
            auto strong=weak_this.lock();
            if(strong){
                lock_guard<mutex>locker(strong->m_mutex);
                auto wait_next=strong->m_last_time_out+1000;
                strong->m_last_time_out=MIN_WAIT_TIME;
                strong->m_is_connecting=false;
                strong->reset_connection();
                strong->m_last_time_out=wait_next>MAX_WAIT_TIME?MAX_WAIT_TIME:wait_next;
            }
            return false;
        },m_last_time_out);
    }
}
bool rtsp_pusher::handle_proxy_request(const shared_ptr<ProxyFrame>&frame)
{
    if(frame->data_len==0)return false;
    CJsonObject object(string(frame->data_buf.get(),frame->data_len));
    string cmd;
    if(!object.Get("cmd",cmd)){
        MICAGENT_LOG(LOG_ERROR,"%s",frame->data_buf.get());
        return false;
    }
    MICAGENT_LOG(LOG_INFO,"%s",object.ToFormattedString().c_str());
    if(cmd=="get_authorized_info_ack")
    {
        handle_get_authorized_info_ack(object);
    }
    else if(cmd=="authorization_ack")
    {
        handle_authorization_ack(object);
    }
    else if(cmd=="set_up_stream_ack")
    {
        handle_set_up_stream_ack(object);
    }
    else if (cmd=="tear_down_stream_ack") {
        handle_tear_down_stream_ack();
    }
    else {
        MICAGENT_LOG(LOG_ERROR,"unknown command %s",object.ToFormattedString().c_str());
    }
    return true;
}
bool rtsp_pusher::send_message(const char *buf,uint32_t buf_len)
{
    if(!m_send_buf->append(buf,buf_len))return false;
    if(!m_tcp_channel->isWriting()){
        m_tcp_channel->enableWriting();
        auto connection_helper=m_connection_helper.lock();
        if(!connection_helper)return false;
        auto loop=connection_helper->get_loop().lock();
        if(!loop)return false;
        loop->updateChannel(m_tcp_channel);
    }
    return true;
}
//发送指令集
void rtsp_pusher::send_get_authorized_info()
{
    if(!m_proxy_interface)return;
    CJsonObject object;
    object.Add("cmd","get_authorized_info");
    object.Add("account",m_user_name);
    object.Add("seq",m_seq++);
    auto request(object.ToString());
    m_proxy_interface->send_control_command(request.c_str(),request.length());
}
void rtsp_pusher::send_authorization(string nonce)
{
    if(!m_proxy_interface)return;
    CJsonObject object;
    object.Add("cmd","authorization");
    object.Add("account",m_user_name);
    object.Add("nonce",nonce);
    auto pass_string=m_user_name+nonce+m_pass_word;
    auto tmp=Get_MD5_String(pass_string.c_str(),pass_string.length());
    object.Add("password",tmp);
    free(tmp);
    object.Add("seq",m_seq++);
    auto request(object.ToString());
    m_proxy_interface->send_control_command(request.c_str(),request.length());
}
static constexpr char s_mode_string[][32]={
    "raw_tcp",
    "raw_udp",
    "raw_hybrid",
    "graded_tcp",
    "graded_hybrid",
};
void rtsp_pusher::send_set_up_stream()
{
    if(!m_proxy_interface||!m_is_authorized||m_is_setup)return;
    CJsonObject object;
    object.Add("cmd","set_up_stream");
    object.Add("stream_name",m_des_name);
    object.Add("trans_mode",s_mode_string[m_mode]);
    object.Add("stream_info",m_media_json_info);
    object.Add("seq",m_seq++);
    auto request(object.ToString());
    m_proxy_interface->send_control_command(request.c_str(),request.length());
}
void rtsp_pusher::send_tear_down_stream()
{
    if(!m_proxy_interface||!m_is_setup)return;
    CJsonObject object;
    object.Add("cmd","tear_down_stream");
    object.Add("stream_token",m_stream_token);
    object.Add("seq",m_seq++);
    auto request(object.ToString());
    m_proxy_interface->send_control_command(request.c_str(),request.length());
}

//处理返回数据
void rtsp_pusher::handle_get_authorized_info_ack(CJsonObject &object)
{//未创建的流才处理此包,可用此包作为心跳包
    if(m_is_setup)return;
    uint32_t status;
    string info;
    uint32_t seq;
    if(!object.Get("status",status)||!object.Get("info",info)||!object.Get("seq",seq)){
        return;
    }
    if(status==P_NOT_NEED_AUTHORIZATION)
    {//无需鉴权,
        m_is_authorized=true;
        send_set_up_stream();
    }
    else if (status==P_OK) {
        string nonce;
        if(!object.Get("nonce",nonce)){
            //错误封包，不处理
        }
        else {

            send_authorization(nonce);
        }
    }
    else {
        //错误处理，无,等待服务器超时断开连接
    }
}
void rtsp_pusher::handle_authorization_ack(CJsonObject &object)
{
    if(m_is_setup)return;
    uint32_t status;
    string info;
    uint32_t seq;
    if(!object.Get("status",status)||!object.Get("info",info)||!object.Get("seq",seq)){
        return;
    }
    if(status==P_OK)
    {
        m_is_authorized=true;
        send_set_up_stream();
    }
    else {
        //错误处理，无,等待服务器超时断开连接
    }
}
void rtsp_pusher::handle_set_up_stream_ack(CJsonObject &object)
{
    if(m_is_setup)return;
    uint32_t status;
    string info;
    uint32_t seq;
    if(!object.Get("status",status)||!object.Get("info",info)||!object.Get("seq",seq)){
        return;
    }
    if(status==P_OK||status==P_STREAM_IS_SET_UP)
    {
        if(!object.Get("stream_token",m_stream_token))
        {

        }
        else {
            //流创建成功的处理
            m_is_setup=true;
            //重置m_proxy_interface
            m_proxy_interface.reset(new ProxyInterface(m_stream_token,m_mode,[this](const void *buf,uint32_t buf_len){
                //TCP协议包发送处理
                return this->send_message(static_cast<const char *>(buf),buf_len);},[this](const void *buf,uint32_t buf_len){
                //UDP数据包发送处理
                if(m_udp_fd==INVALID_SOCKET)return false;
                return NETWORK.time_out_sendto(m_udp_fd,buf,buf_len,0,(struct sockaddr *)&m_des_addr,sizeof(struct sockaddr_in),1)==buf_len;
            },[this](shared_ptr<ProxyFrame>frame){
                                        return this->handle_proxy_request(frame);
                                    }));
        }
    }
    else {
        //错误处理，无,等待服务器超时断开连接
    }
}
void rtsp_pusher::handle_tear_down_stream_ack()
{
    //复位连接
    reset_connection();
}
