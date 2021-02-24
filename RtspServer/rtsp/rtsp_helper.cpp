#define _GLIBCXX_USE_C99 1
#include "rtsp_helper.h"
#include <arpa/inet.h>
#include <random>
#include <cstring>
#include "MD5.h"
#include <algorithm>
#include "c_log.h"
#include "websocket_common.h"
#define LIB_STRING_FOR_MICAGENT  "micagent"
using namespace micagent;
const char rtsp_helper::kCRLF[] = "\r\n";
string rtsp_helper::buildOptionRes(const string & CSeq)
{
    char buf[256]={0};
    snprintf(buf, 256,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, GET_PARAMETER\r\n"
            "\r\n",CSeq.c_str());
    return string(buf);
}
 pair<bool ,map<string,string>> rtsp_helper::parseUrl(const string &url)
{
     char account[64]={0};
     char password[64]={0};
     char ip[32]={0};
     uint16_t port=554;
     char suffix[256]={0};
     bool match=true;
     if(sscanf(url.c_str()+7,"%[^:]:%[^@]@%[^:]:%hu/%s",account,password,ip,&port,suffix)==5){}
     else if(sscanf(url.c_str()+7,"%[^:]:%[^@]@%[^/]/%s",account,password,ip,suffix)==4){
         port=554;
     }
     else if(sscanf(url.c_str()+7,"%[^:]:%hu/%s",ip,&port,suffix)==3) {
         account[0]='\0';
         password[0]='\0';
     }
     else if (sscanf(url.c_str()+7,"%[^/]/%s",ip,suffix)==2) {
         account[0]='\0';
         password[0]='\0';
         port=554;
     }
     else {
         match=false;
     }
     uint32_t ip_n;
     map<string,string>ret;
     if(inet_pton(AF_INET,ip,&ip_n)!=1)match=false;
     if(match){
         ret.emplace("account",account);
         ret.emplace("password",password);
         ret.emplace("ip",ip);
         ret.emplace("port",to_string(port));
         ret.emplace("suffix",suffix);
     }
     return {match,ret};
}
pair<bool ,map<string,string>> rtsp_helper::parseCommand(const string &buf)
{
#ifdef DEBUG
    MICAGENT_LOG(LOG_INFO,"[%s] rtsp recv :%s",Logger::Instance().get_local_time().c_str(),buf.c_str());
#endif
    bool ret=false;
    map<string,string>save;
    const char *cmd_line_end=search(buf.c_str(),buf.c_str()+buf.length(),kCRLF,kCRLF+2);
    do{
        if(cmd_line_end==buf.c_str()+buf.length())break;
        if(!parseHeadrLine(string(buf.c_str(),cmd_line_end),save))break;
        auto cmd=save.find("cmd");
        if(cmd==save.end())break;
        string left(cmd_line_end+2,buf.c_str()+buf.length());
        if(!parseRequestLine(left,save))break;
        if(cmd->second=="OPTIONS")
        {
           if(! parseOptions(left,save))break;
        }
        else if (cmd->second=="DESCRIBE") {
            if(! parseDescribe(left,save))break;
        }
        else if (cmd->second=="SETUP") {
            if(! parseSetup(left,save))break;
        }
        else if (cmd->second=="PLAY") {
            if(! parsePlay(left,save))break;
        }
        else if (cmd->second=="TEARDOWN") {
            if(! parseTeardown(left,save))break;
        }
        else if (cmd->second=="GET_PARAMETER") {
            if(! parseGetparam(left,save))break;
        }
        else if (cmd->second=="GET") {
            //websocket
            MICAGENT_INFO("recv http message:%s",buf.c_str());
        }
        else if (cmd->second=="POST") {
            //websocket
            MICAGENT_INFO("recv http message:%s",buf.c_str());
        }
        else {
            break;
        }
        ret=true;
    }while(0);
    return {ret,save};
}
  string rtsp_helper::getDateHeader()
 {
      char buf[200];
      time_t tt = time(nullptr);
      strftime(buf, sizeof buf, "Date: %a, %b %d %Y %H:%M:%S GMT\r\n", gmtime(&tt));
      return buf;
 }
string rtsp_helper::getRandomNonce()
{
    char buf[33]={0};
    random_device rd;
    snprintf(buf,32,"%u%u",rd(),rd());
    auto tmp=Get_MD5_String(buf,33);
    string ret=tmp;
    free(tmp);
    return ret;
}
string rtsp_helper::buildDescribeRes(const string &Sdp,const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Content-Length: %lu\r\n"
            "Content-Type: application/sdp\r\n"
            "\r\n"
            "%s",
            CSeq.c_str(),
            Sdp.length(),
            Sdp.c_str());

    return string(buf);
}

string rtsp_helper::buildSetupMulticastRes(const string &url_ip,const char* strMulticastIp, uint16_t port, uint32_t sessionId,const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Transport: RTP/AVP;multicast;destination=%s;source=%s;port=%u-0;ttl=255\r\n"
            "Session: %u\r\n"
            "\r\n",
            CSeq.c_str(),
            strMulticastIp,
            url_ip.c_str(),
            port,
            sessionId);

    return string(buf);
}

string rtsp_helper::buildSetupUdpRes( uint16_t rtp_port,uint16_t rtcp_port, uint16_t local_rtp_port, uint16_t local_rtcp_port, uint32_t sessionId,const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
            "Session: %u\r\n"
            "\r\n",
            CSeq.c_str(),
            rtp_port,
            rtcp_port,
            local_rtp_port,
            local_rtcp_port,
            sessionId);

    return string(buf);
}

string rtsp_helper::buildSetupTcpRes(uint16_t rtpChn, uint16_t rtcpChn, uint32_t sessionId,const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n"
            "Session: %u\r\n"
            "\r\n",
            CSeq.c_str(),
            rtpChn, rtcpChn,
            sessionId);

    return string(buf);
}

string rtsp_helper::buildPlayRes(const char* rtpInfo, uint32_t sessionId, const string & CSeq, int time_out)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Range: npt=0.000-\r\n"
            "Session: %u; timeout=%d\r\n",
            CSeq.c_str(),
            sessionId,time_out);

    if (rtpInfo != nullptr&&rtpInfo[0]!='\0')
    {
        snprintf(buf + strlen(buf), 4096 - strlen(buf), "%s\r\n", rtpInfo);
    }

    snprintf(buf + strlen(buf), 4096 - strlen(buf), "\r\n");
    return string(buf);
}

string rtsp_helper::buildTeardownRes( uint32_t sessionId,const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Session: %u\r\n"
            "\r\n",
            CSeq.c_str(),
            sessionId);

    return string(buf);
}

string rtsp_helper::buildGetParamterRes(uint32_t sessionId,const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 200 OK\r\n"
            "CSeq: %s\r\n"
            "Session: %u\r\n"
            "Content-Length: %lu\r\n"
            "\r\n%s",
            CSeq.c_str(),
            sessionId,strlen(LIB_STRING_FOR_MICAGENT),LIB_STRING_FOR_MICAGENT);
    return string(buf);
}

string rtsp_helper::buildNotFoundRes(const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 404 Stream Not Found\r\n"
            "CSeq: %s\r\n"
            "\r\n",
            CSeq.c_str());

    return string(buf);
}
string rtsp_helper::buildUnauthorizedRes(const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
             "RTSP/1.0 401 Unauthorized\r\n"
             "CSeq: %s\r\n"
             "WWW-Authenticate: Digest realm=\"%s\", nonce=\"%s\"\r\n"
             "%s"
             "\r\n",
            CSeq.c_str(),REALM_INFO,getRandomNonce().c_str(),getDateHeader().c_str());
    return string(buf);
}

string rtsp_helper::buildServerErrorRes(const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 500 Internal Server Error\r\n"
            "CSeq: %s\r\n"
            "\r\n",
            CSeq.c_str());

    return string(buf);
}
string rtsp_helper::buildRtspHeaderRes(std::string head_info,const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 %s\r\n"
            "CSeq: %s\r\n"
            "%s"
            "\r\n",
            head_info.c_str(),CSeq.c_str(),getDateHeader().c_str());

    return string(buf);
}

string rtsp_helper::buildWebsocketRes(const string &key)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n"
            "Sec-WebSocket-Protocol: chat\r\n"
            "\r\n",
           web_socket_helper::WS_Generate_Accept_Key(key).c_str() );
    return string(buf);
}

string rtsp_helper::buildUnsupportedRes(const string & CSeq)
{
    char buf[4096]={0};
    snprintf(buf, 4096,
            "RTSP/1.0 461 Unsupported transport\r\n"
            "CSeq: %s\r\n"
            "\r\n",
            CSeq.c_str());

    return string(buf);
}
bool rtsp_helper::checkAuthentication(const string &account,const string &password,const string &cmd_name,const string &realm,const string &nonce,const string &uri,const string &response)
{
    std::string part1=account+":"+realm+":"+password;
    char * key1=Get_MD5_String(part1.c_str(),part1.size());
    std::string part2=cmd_name+":"+uri;
    char * key2=Get_MD5_String(part2.c_str(),part2.size());
    std::string part3=key1;
    part3=part3+":"+nonce+":"+key2;
    char *key3=Get_MD5_String(part3.c_str(),part3.size());
    bool ret=response==key3;
    free(key1);
    free(key2);
    free(key3);
    return ret;
}
 bool rtsp_helper::parseHeadrLine(const string &buf,map<string,string>&save)
{
     char method[64] = {0};
    char url[512] = {0};
    char version[64] = {0};
    if(sscanf(buf.c_str(), "%s %s %s", method, url, version) != 3)return false;
    save.emplace("cmd",method);
    save.emplace("url",url);
    save.emplace("version",version);
    return true;
}
bool rtsp_helper::parseRequestLine(const string &buf,map<string,string>&save)
{
    const char *begin=buf.c_str();
    const char *buf_end=buf.c_str()+buf.size();
    const char *end=nullptr;
    while(begin<buf_end){
        end=search(begin,buf_end,kCRLF,kCRLF+2);
        if(end==begin||end==buf_end)break;
        string tmp(begin,end);
        begin=end+2;
        char key[64]={0};
        char value[512]={0};
        if(tmp.size()>580){
            MICAGENT_LOG(LOG_INFO,"false input :%s",tmp.c_str());
            break;
        }
        if(sscanf(tmp.c_str(),"%[^:]: %[^\r\n]",key,value)!=2){
            return false;
        }
        save.emplace(key,value);
    }
    return save.find("CSeq")!=std::end(save);
}
bool rtsp_helper::parseGetparam(const string &buf,map<string,string>&save)
{
    return parseRequestLine(buf,save);
}
bool rtsp_helper::parseOptions(const string &buf,map<string,string>&save)
{
    return parseRequestLine(buf,save);
}
bool rtsp_helper::parseDescribe(const string &buf,map<string,string>&save)
{
    bool ret=false;
    do{
        if(!parseRequestLine(buf,save))break;
        auto iter=save.find("Authorization");
        if(iter!=save.end()){
            char tmp_buf[512]={0};
            if(sscanf(iter->second.c_str(),"%*s %[^\r\n]",tmp_buf)!=1)break;
            char username[128]={0};
            char realm[128]={0};
            char nonce[64]={0};
            char uri[256]={0};
            char response[64]={0};
            if(sscanf(tmp_buf,"username=\"%[^\"]\", realm=\"%[^\"]\", nonce=\"%[^\"]\", uri=\"%[^\"]\", response=\"%[^\"]",\
                      username,realm,nonce,uri,response)!=5){
                      ret=true;
                      break;
            }
            save.emplace("username",username);
            save.emplace("realm",realm);
            save.emplace("nonce",nonce);
            save.emplace("uri",uri);
            save.emplace("response",response);
        }
        ret=true;
    }while(0);
    return ret;
}
bool rtsp_helper::parseSetup(const string &buf,map<string,string>&save)
{
    bool ret=false;
    do{
        if(!parseRequestLine(buf,save))break;
        char transmode[32]={0};
        uint16_t rtp_port=0;
        uint16_t rtcp_port=0;
        int rtp_channel=0;
        int rtcp_channel=1;
        uint16_t trackid=0;
        auto track_iter=save.find("url");
        auto tmp=strstr(track_iter->second.c_str(),"track");
        if(track_iter==save.end()||!tmp||sscanf(tmp,"track%hu",&trackid)!=1){
            MICAGENT_LOG(LOG_FATALERROR,"miss track %s",buf.c_str());
        }
        auto port_iter=save.find("Transport");
        if(port_iter==end(save))break;
        if(sscanf(port_iter->second.c_str(),"%[^;]",transmode)!=1)break;
        string unicast="unicast";
        if(search(port_iter->second.c_str(),port_iter->second.c_str()+port_iter->second.length(),unicast.c_str(),\
        unicast.c_str()+unicast.length())==port_iter->second.c_str()+port_iter->second.length()){
            unicast="multicast";
        }
        tmp=strstr(port_iter->second.c_str(),"client_port");
        if(tmp)sscanf(tmp,"client_port=%hu-%hu",&rtp_port,&rtcp_port);
        tmp=strstr(port_iter->second.c_str(),"interleaved");
        if(tmp)sscanf(tmp,"interleaved=%d-%d",&rtp_channel,&rtcp_channel);
        save.emplace("transmode",transmode);
        save.emplace("rtp_port",to_string(rtp_port));
        save.emplace("rtcp_port",to_string(rtcp_port));
        save.emplace("rtp_channel",to_string(rtp_channel));
        save.emplace("rtcp_channel",to_string(rtcp_channel));
        save.emplace("trackid",to_string(trackid));
        save.emplace("unicast",unicast);
        ret=true;
    }while(0);
    return ret;
}
bool rtsp_helper::parsePlay(const string &buf,map<string,string>&save)
{
    bool ret=false;
    do{
        if(!parseRequestLine(buf,save))break;
        ret=true;
    }while(0);
    return ret;
}
bool rtsp_helper::parseTeardown(const string &buf,map<string,string>&save)
{
    bool ret=false;
    do{
        if(!parseRequestLine(buf,save))break;
        ret=true;
    }while(0);
    return ret;
}
