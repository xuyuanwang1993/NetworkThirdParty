#ifndef RTSP_HELPER_H
#define RTSP_HELPER_H
#include <map>
#include<string>
#define REALM_INFO "micagent@meanning_cloud"
namespace micagent {
using namespace std;
class rtsp_helper{
public:
static pair<bool ,map<string,string>> parseUrl(const string &url);
static pair<bool ,map<string,string>> parseCommand(const string &buf);
static string buildOptionRes(const string & CSeq="0");
static string buildDescribeRes( const string &Sdp,const string & CSeq="0");
static string buildSetupMulticastRes( const string &url_ip,const char* strMulticastIp, uint16_t port, uint32_t sessionId,const string & CSeq="0");
static string buildSetupTcpRes(uint16_t rtpChn, uint16_t rtcpChn, uint32_t sessionId,const string & CSeq="0");
static string buildSetupUdpRes(uint16_t rtp_port,uint16_t rtcp_port, uint16_t local_rtp_port, uint16_t local_rtcp_port, uint32_t sessionId,const string & CSeq="0");
static string buildPlayRes(const char* rtpInfo, uint32_t sessionId,const string & CSeq="0",int time_out=120);
static string buildTeardownRes(uint32_t sessionId,const string & CSeq="0");
static string buildGetParamterRes(uint32_t sessionId,const string & CSeq="0");
static string buildNotFoundRes(const string & CSeq="0");
static string buildUnauthorizedRes(const string & CSeq="0");
static string buildServerErrorRes(const string & CSeq="0");
static string buildUnsupportedRes(const string & CSeq="0");
static string buildRtspHeaderRes(std::string head_info,const string & CSeq="0");

static string getDateHeader();
static string getRandomNonce();

static bool checkAuthentication(const string &account,const string &password,const string &cmd_name,const string &realm,const string &nonce,const string &uri,const string &response);
private:
static bool parseHeadrLine(const string &buf,map<string,string>&save);
static bool parseRequestLine(const string &buf,map<string,string>&save);
static bool parseGetparam(const string &buf,map<string,string>&save);
static bool parseOptions(const string &buf,map<string,string>&save);
static bool parseDescribe(const string &buf,map<string,string>&save);
static bool parseSetup(const string &buf,map<string,string>&save);
static bool parsePlay(const string &buf,map<string,string>&save);
static bool parseTeardown(const string &buf,map<string,string>&save);
private:
static const char kCRLF[];
};
}
#endif // RTSP_HELPER_H
