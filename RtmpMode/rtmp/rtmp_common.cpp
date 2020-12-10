#include "rtmp_common.h"
using namespace micagent;
 Rtmp_Url rtmp_helper::Parse_Rtmp_Url(const string &url)
 {
     Rtmp_Url ret;
     ret.raw_url=url;
     return ret;
 }
