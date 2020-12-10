#include "rtmp_server.h"
using namespace micagent;
void rtmp_url_parse_test()
{
    string url1="rtmp://192.168.2.111:1935/test/test_stream";
    rtmp_helper::Parse_Rtmp_Url(url1);
}
int main()
{
    rtmp_url_parse_test();
    return  0;
}
