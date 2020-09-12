#ifndef PROXYBUFHANDLE_H
#define PROXYBUFHANDLE_H
#include "buffer_handle.h"
#include "proxy_protocol.h"
namespace micagent{
class proxy_message:public buffer_handle{
    static constexpr uint32_t MAX_TCP_MSS_CACHE=10000;
public:
    proxy_message(bool is_send=true);
    ~proxy_message();
protected:
    bool insert_packet(const char *buf,uint32_t buf_len);
private:
    bool m_is_send;
    shared_ptr<PStreamParse> m_recv_parse;
};
}
#endif // PROXYBUFHANDLE_H
