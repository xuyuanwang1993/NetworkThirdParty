#include "kcp_proxy_server.h"
#include "kcp_proxy_connection.h"
#include "kcp_proxy_interface.h"
using namespace micagent;
kcp_proxy_server::kcp_proxy_server()
{
}
void kcp_proxy_server::init_proxy_server(uint16_t local_port,sockaddr_in *public_server_addr)
{

}
void kcp_proxy_server::init_public_server()
{

}
kcp_proxy_server::~kcp_proxy_server()
{

}
bool kcp_proxy_server::add_connection(shared_ptr<kcp_proxy_connection>connection)
{

}
bool kcp_proxy_server::remove_connection(weak_ptr<kcp_proxy_connection>connection)
{

}
bool kcp_proxy_server::send_application_data(weak_ptr<kcp_proxy_connection>connection,const void *data,uint32_t data_len)
{

}
bool kcp_proxy_server::accept_connection(shared_ptr<kcp_proxy_connection>connection)
{

}
void kcp_proxy_server::handle_read(const ikcp_raw_udp_packet_s&data)
{

}
void kcp_proxy_server::check_all_connections()
{

}
void kcp_proxy_server::response_error_info(const ikcp_proxy_header_s &header,const ikcp_raw_udp_packet_s&data,const string error_info)
{

}
