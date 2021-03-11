#include "websocket_client.h"
#include "MD5.h"
#include"Base64.h"
using namespace micagent;
websocket_client::websocket_client(shared_ptr<tcp_connection_helper>helper,const string &ip,uint16_t port,const string &api):http_client (helper,ip,port,api)\
  ,m_extern_ws_recv_frame_callback(nullptr),m_connection_status(WS_CONNECTION_CONNECTING)\
  ,m_ws_connection_success_callback(nullptr),m_ws_session_key(generate_session_key(this)),m_ws_recv_cache(new web_socket_buffer_cache(1000))
{
    m_send_cache.reset(new web_socket_buffer_cache(1000,true));
}
websocket_client::~websocket_client(){

}
bool websocket_client::send_websocket_data(const void *buf,uint32_t buf_len,WS_Frame_Header::WS_FrameType type)
{
    if(!check_is_send_cache_anyway()&&!ws_connection_is_setup())return false;
    lock_guard<mutex>locker(m_mutex);
    WS_Frame frame(buf,buf_len,type);
    auto data=web_socket_helper::WS_EncodeData(frame);
    if(!m_send_cache->append(data.first.get(),data.second))return false;
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
void websocket_client::websocket_init_callback(weak_ptr<websocket_client>client, const WS_FRAME_CALLBACK &cb, const CONNECTION_SUCCESS_CALLBACK &cb2)
{
    auto strong=client.lock();
    if(!strong)return;
    strong->set_http_recv_callback([client](pair<shared_ptr<uint8_t>,uint32_t>){
        auto strong=client.lock();
        if(strong){
            strong->handle_setup_packet();
        }
    });
    //when connection is set up,send the shake hand packet
    strong->set_http_connect_callback([client](){
        auto strong=client.lock();
        if(!strong)return ;
        strong->send_setup_packet();
    });
    //handle recv web socket frame
    strong->set_extern_websocket_frame_callback(cb);
    //when a websocket connction set up success,cb2 is called,so that you can know when to start send websocket data and when to recv datass
    strong->set_ws_connection_success_callback(cb2);
}
bool websocket_client::handle_read()
{
    if(m_connection_status==WS_CONNECTION_CLOSED)return false;
    else if (m_connection_status==WS_CONNECTION_CONNECTING) {
        char buf[2048]={0};
        auto len=::recv(m_tcp_channel->fd(),buf,2048,0);
        if(len==0)return false;
        if(len<0){
            if(errno==EAGAIN||errno==EINTR)return true;
            else {
                return false;
            }
        }
        else {
            if(m_http_recv_helper)m_http_recv_helper->update(buf,len);
        }
        return true;
    }
    else {
        auto len=m_ws_recv_cache->read_fd(m_tcp_channel->fd());
        if(len==0)return false;
        if(len<0){
            if(errno==EAGAIN||errno==EINTR)return true;
            else {
                return false;
            }
        }
        uint32_t packet_len;
        while((packet_len=m_ws_recv_cache->get_first_packet_size())!=0){
            shared_ptr<uint8_t>packet_buf(new uint8_t[packet_len+1],default_delete<uint8_t[]>());
            auto read_len=m_ws_recv_cache->read_packet(packet_buf.get(),packet_len);
            auto frame=web_socket_helper::WS_DecodeData(packet_buf.get(),read_len,false);
            usr_handle_websocket_frame(frame);
            do{
                if(!m_extern_ws_recv_frame_callback)break;
                auto helper=m_connection_helper.lock();
                if(!helper)break;
                auto loop=helper->get_loop().lock();
                if(!loop)break;
                auto callback=m_extern_ws_recv_frame_callback;
                loop->add_trigger_event([callback,frame](){
                    callback(frame);
                });
            }while(0);
        }
        return true;
    }
}
string websocket_client::generate_session_key(void *ptr)
{
    random_device rd;
    uint32_t value1=rd();
    uint32_t value2=rd();
    string session_key=to_string(value1);
    session_key+=to_string(reinterpret_cast<uint64_t>(ptr));
    session_key+=to_string(value2);
    uint8_t data[16]={0};
    MY_our_MD5DataRaw(reinterpret_cast<const uint8_t *>(session_key.c_str()),session_key.length(),data);
    session_key=base64Encode(data,16);
    return session_key;
}
bool websocket_client::key_is_match(const string &server_key)
{
    auto local_key=web_socket_helper::WS_Generate_Accept_Key(m_ws_session_key);
    return local_key==server_key;
}
bool websocket_client::handle_setup_packet()
{
    auto http_status=m_http_recv_helper->get_response_status();
    if(http_status.first!="101"){
        MICAGENT_ERROR("set up websocket connection failed!(%s %s)",http_status.first.c_str(),http_status.second.c_str());
        return false;
    }
    auto server_key=m_http_recv_helper->get_head_info(SERVER_KEY_STRING);
    if(!key_is_match(server_key)){
        MICAGENT_ERROR("set up websocket connection failed!(server key not match)");
        return false;
    }
    m_connection_status=WS_CONNECTION_CONNECTED;
    do{
        if(!m_ws_connection_success_callback)break;
        auto helper=m_connection_helper.lock();
        if(!helper)break;
        auto loop=helper->get_loop().lock();
        if(!loop)break;
        auto callback=m_ws_connection_success_callback;
        loop->add_trigger_event([callback](){
            callback();
        });
    }while(0);
    return true;
}
void websocket_client::send_setup_packet()
{
    map<string,string>head_map;
    head_map["Host"]=m_des_ip+":"+to_string(m_des_port);
    head_map["Upgrade"]="websocket";
    head_map["Connection"]="Upgrade";
    head_map["Sec-WebSocket-Key"]=m_ws_session_key;
    head_map["Sec-WebSocket-Version"]="13";
    send_http_packet(head_map,nullptr,0,nullptr,0,HTTP_GET_REQUEST);
}
void websocket_client::usr_handle_websocket_frame(const WS_Frame &frame)
{//just print
    web_socket_helper::WS_dump_frame(frame);
}
void websocket_client::util_test()
{
#if UTIL_TEST
    string url="ws://192.168.3.215:58554";
    //string url="ws://192.168.2.105:8001/echo_once/";
    auto url_info=parse_url_info(url,"ws");
    
    shared_ptr<EventLoop>loop(new EventLoop());
    shared_ptr<tcp_connection_helper>helper( tcp_connection_helper::CreateNew(loop));
    shared_ptr<websocket_client> client(new websocket_client(helper,url_info.ip,url_info.port,url_info.api));
    auto weak_client=weak_ptr<websocket_client>(client);
    auto cb=[weak_client](){
        auto func=[](const WS_Frame &frame)->bool{
            printf("%d %d ",frame.data.get()[0],frame.data.get()[1]);
            if(frame.data.get()[1]==0){
                printf("%s\r\n",string(reinterpret_cast<char *>(frame.data.get()+2),frame.data_len-2).c_str());
            }
            else if (frame.data.get()[1]==1) {
                printf("close!\r\n");
            }
            else {
                auto ptr=frame.data.get()+2;
                int channel=ptr[0];
                int timestamp=0;
                timestamp|=ptr[1];
                timestamp<<=8;
                timestamp|=ptr[2];
                timestamp<<=8;
                timestamp|=ptr[3];
                timestamp<<=8;
                timestamp|=ptr[4];
                printf("%d %d %02x\r\n",channel,timestamp,ptr[5]);
            }
            return true;
        };
        websocket_client::websocket_init_callback(weak_client,func,[weak_client](){
            MICAGENT_BACKTRACE("websocket_client connect success!");
            thread t3([weak_client](){
            auto client=weak_client.lock();
            if(client){
                //client->send_websocket_data("1111111",7,WS_Frame_Header::WS_TEXT_FRAME);
                string json_request="{\"stream_name\":\"micagent/3d5e384288000001/local_195\",\"account\":\"admin\",\"password\":\"micagent\"}";
                uint32_t send_len=2+json_request.size();
                shared_ptr<uint8_t>send_buf(new uint8_t[send_len],default_delete<uint8_t[]>());
                send_buf.get()[0]=1;
                send_buf.get()[1]=0;
                memcpy(send_buf.get()+2,json_request.c_str(),json_request.size());
                client->send_websocket_data(send_buf.get(),send_len,WS_Frame_Header::WS_BINARY_FRAME);
            }
            });
            t3.detach();
        });
    };
    client->open_connection(cb);
    loop->addTimer([weak_client](){
        auto client=weak_client.lock();
            if(client){
                uint32_t send_len=2;
                shared_ptr<uint8_t>send_buf(new uint8_t[send_len],default_delete<uint8_t[]>());
                send_buf.get()[0]=1;
                send_buf.get()[1]=1;
                client->send_websocket_data(send_buf.get(),send_len,WS_Frame_Header::WS_BINARY_FRAME);
            }
        return false;
    },5000);
//    sleep(500);
//    client->close_connection();
    while(getchar()!='8')continue;
#endif
}
