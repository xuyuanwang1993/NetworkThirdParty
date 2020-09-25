#ifndef HTTP_SEND_BUF_H
#define HTTP_SEND_BUF_H
#include "buffer_handle.h"
namespace micagent{
class http_send_buf:public buffer_handle{
public:
http_send_buf(uint32_t capacity);
~http_send_buf()override{}
protected:
 bool insert_packet(const char *buf,uint32_t buf_len)override;
};
}
#endif // HTTP_SEND_BUF_H
