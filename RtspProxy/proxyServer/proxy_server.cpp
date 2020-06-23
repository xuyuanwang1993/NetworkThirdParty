#include "proxy_server.h"
#include "proxy_connection.h"
#include "proxy_protocol.h"
using namespace micagent;
proxy_server::proxy_server(uint16_t listen_port,uint32_t netinterface_index):tcp_server(listen_port,netinterface_index),m_remove_timer_id(INVALID_TIMER_ID)
{
    Network_Util::Instance().get_net_interface_info();
    //初始化udp包socket
    SOCKET udp_sock=NETWORK.build_socket(UDP);
    if(udp_sock==INVALID_SOCKET)throw runtime_error("socket can't build!");
    NETWORK.make_noblocking(udp_sock);
    NETWORK.set_reuse_port(udp_sock);
    NETWORK.bind(udp_sock,listen_port);
    m_udp_channel.reset(new Channel(udp_sock));
}
proxy_server::~proxy_server()
{
    //移除额外加入到其它模块的资源
    //定时器会自动移除
    if(m_udp_channel&&m_loop)m_loop->removeChannel(m_udp_channel);
    if(m_udp_data_handle_thread&&m_udp_data_handle_thread->joinable())m_udp_data_handle_thread->join();
}
shared_ptr<tcp_connection>proxy_server::new_connection(SOCKET fd)
{
    return make_shared<proxy_connection>(fd,this);
}
void proxy_server::remove_invalid_connection()
{
    lock_guard<mutex>locker(m_mutex);
    auto time_now=Timer::getTimeNow();
    for(auto iter=m_connections.begin();iter!=m_connections.end();){
        auto time_diff=proxy_connection::diff_alive_time(dynamic_cast<proxy_connection *>(iter->second.get()),time_now);
        if(time_diff>CONNECTION_TIME_OUT){
            iter->second->unregister_handle(m_loop);
            m_connections.erase(iter++);
        }
        else {
            iter++;
        }
    }
}
void proxy_server::udp_data_input(const pair<shared_ptr<char[]>,uint16_t>&buf_pair)
{
    //根据stream_token进行分流，分别输入不同的转发连接中
    auto stream_token=ProxyInterface::get_stream_token(buf_pair.first.get(),buf_pair.second);
    lock_guard<mutex>locker(m_mutex);
    auto iter=m_udp_connections_map.find(stream_token);
    if(iter!=m_udp_connections_map.end())
    {
        auto connections_iter=m_connections.find(iter->second);
        if(connections_iter!=m_connections.end())
        {
            auto connection_ptr=connections_iter->second;
            auto connection=dynamic_cast<proxy_connection *>(connection_ptr.get());
            if(connection)connection->input_udp_data(buf_pair.first.get(),buf_pair.second);
        }
        else {
            m_udp_connections_map.erase(iter);
        }
    }
}
