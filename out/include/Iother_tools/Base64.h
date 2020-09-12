#ifndef BASE64_H
#define BASE64_H
#include <string>
#include<map>
#include<memory>
namespace micagent{
using namespace std;
    std::string base64Encode(const void *input_buf,uint32_t buf_size);
    pair<shared_ptr<uint8_t>,uint32_t> base64Decode(const std::string & origin_string);
};
#endif // BASE64_H
