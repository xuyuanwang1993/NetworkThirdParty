#ifndef MEDIA_SESSION_H
#define MEDIA_SESSION_H
#include "c_log.h"
#include "media.h"
#include<atomic>
#include "rtp_connection.h"
#include <unordered_set>
#include <random>
#include "network_util.h"
#include "media_source.h"
#include <list>
//#define SAVE_FILE_TEST
#ifdef SAVE_FILE_TEST
#define SAVE_FILE_ACCESS
#define SAVE_FILE_PRFIX "stream_"
#endif
namespace micagent {
class MulticastAddr
{
public:
    static MulticastAddr& instance()
    {
        static MulticastAddr s_multi_addr;
        return s_multi_addr;
    }

    std::string getAddr()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::string addrPtr;
        struct sockaddr_in addr;
        memset(&addr,0,sizeof (addr));
        std::random_device rd;
        for (int n = 0; n <= 10; n++)
        {
            uint32_t range = 0xE8FFFFFF - 0xE8000100;
            addr.sin_addr.s_addr = htonl(0xE8000100 + (rd()) % range);
            addrPtr = inet_ntoa(addr.sin_addr);

            if (m_addrs.find(addrPtr) != m_addrs.end())
            {
                addrPtr.clear(); //地址已被使用
            }
            else
            {
                m_addrs.insert(addrPtr);
                break;
            }
        }

        return addrPtr;
    }

    void release(std::string addr)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_addrs.erase(addr);
    }

private:
    std::mutex m_mutex;
    std::unordered_set<std::string> m_addrs;
};
struct media_source_info{
    string media_name;
    MediaChannelId channel;
    uint32_t frame_rate;
    string proxy_param;
};
class proxy_session_base{
public:
    proxy_session_base(){}
    virtual ~proxy_session_base();
    virtual void proxy_frame(MediaChannelId id,const AVFrame &frame)=0;
    virtual void open_connection(const vector<media_source_info>&media_source_info)=0;
};

class media_session{
public:
    static shared_ptr<media_session>CreateNew(const string &rtsp_suffix,bool is_multicast=false);
    void setMediaSource(MediaChannelId id,shared_ptr<media_source>source);
    void removeMediaSource(MediaChannelId id);

    void setFrameRate(uint32_t frame_rate);


    string getSuffix(){return m_suffix;}
    MediaSessionId getSessionid()const{return m_session_id;}
    uint32_t getClientsNums()const;
    bool updateFrame(MediaChannelId channel,const AVFrame &frame);

    bool isMulticast() const{return m_is_multicast;}
    std::string getMulticastIp() const{ return m_multicast_ip; }
    uint16_t getMulticastPort(MediaChannelId channelId) const{return m_multicastPort[channelId];}

    bool addClient(SOCKET rtspfd, std::shared_ptr<rtp_connection> rtpConnPtr);
    void notice_new_connection();
    void  addProxySession(shared_ptr<proxy_session_base> session);
    void removeClient(SOCKET rtspfd);

    string get_sdp_info (const string & version="");

    shared_ptr<media_source>get_media_source(MediaChannelId id)const{
        lock_guard<mutex>locker(m_mutex);
        return m_media_source[id];
    }
    ~media_session();
    media_session(string rtsp_suffix);
private:
    vector<media_source_info>get_media_source_info()const ;
    void initMulticast();
    static MediaSessionId generate_session_id(){
        if(s_session_id==INVALID_MediaSessionId)s_session_id++;
        return s_session_id++;
    }
private:
    static atomic<MediaSessionId> s_session_id;
    string m_suffix;
    MediaSessionId m_session_id;
    mutable mutex m_mutex;
    vector<shared_ptr<media_source>>m_media_source;

    map<SOCKET,weak_ptr<rtp_connection>>m_rtp_connections;
    list<weak_ptr<proxy_session_base>> m_proxy_session_map;
    bool m_is_multicast;
    string m_multicast_ip;
    uint16_t m_multicastPort[MAX_MEDIA_CHANNEL];

    string m_sdp;

    bool m_has_new_client;
#ifdef  SAVE_FILE_ACCESS
    FILE *m_save_fp;
#endif
};
}


#endif // MEDIA_SESSION_H
