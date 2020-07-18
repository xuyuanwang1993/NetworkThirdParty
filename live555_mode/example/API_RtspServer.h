#ifndef API_RTSPSERVER_H
#define API_RTSPSERVER_H
//-lrt -lpthread -ldl -lm
#ifdef WIN32
#ifdef VSPROJECT_EXPORTS
#define RTSPSERVER_API __declspec(dllexport)
#else
#define RTSPSERVER_API __declspec(dllimport)
#endif
#else
#define RTSPSERVER_API
#endif // WIN32
#define RTSPSERVER_LIB_VERSION "2019.07.17"
#include <thread>
#include <memory>
#include <iostream>
#include <string>
#include <atomic>
#include<unordered_map>
#include<chrono>
#include<mutex>
namespace micagent{
    class rtsp_server;
    class EventLoop;
};
using std::endl;
using std::cout;
using std::unordered_map;
 class RTSPSERVER_API Api_rtsp_server{
public:
    enum MediaType
    {
        DEFAULT=0,
        PCMA = 8,
        H264 = 96,
        AAC  = 37,
        H265 = 265,
    };
    typedef struct _Rtsp_Handle{
        std::shared_ptr<micagent::rtsp_server > server;//服务器外部引用，不允许外部修改
        std::shared_ptr<micagent::EventLoop> event_loop;//任务调度器
        std::atomic_bool is_run;//判断服务器是否正在运行
        _Rtsp_Handle (){
            is_run=false;
        }
    }Rtsp_Handle;
    //添加鉴权信息，可添加多个账户 默认账户密码admin@micagent
    static bool Api_Rtsp_Server_AddAuthorization(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,std::string username,std::string password);
    static void  Api_Rtsp_Server_Init_And_Start(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint16_t port=554);
    //返回为0代表创建失败
    struct Media_Info{
        MediaType viedo_type;
        MediaType audio_type;
        int frameRate;
        uint32_t sampleRate;
        uint32_t channels;
        bool hasADTS;
        Media_Info(){
            viedo_type=DEFAULT;
            audio_type=DEFAULT;
            frameRate=25;
            sampleRate=44100;
            channels=2;
            hasADTS=true;
        }
    };
//此函数的返回值为取消会话和数据传输所需传入的sessionid
    static uint32_t Api_Rtsp_Server_Add_Stream(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,std::string stream_name,Media_Info media_info);
    static void Api_Rtsp_Server_Remove_Stream(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id);
 typedef  enum
    {
        UNKNOWN_FRAME=0x00,
        VIDEO_FRAME_I = 0x01,
        VIDEO_FRAME_P = 0x02,
        VIDEO_FRAME_B = 0x03,
        AUDIO_FRAME   = 0x11,
    }Frame_type;
    static bool Api_Rtsp_Push_Frame(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id,const void *tmp_buf,int buf_size,int channel_id=0,int64_t micro_time_now=0);
    static void Api_Rtsp_Add_Frame_Control(std::weak_ptr<Api_rtsp_server::Rtsp_Handle> handle,uint32_t session_id,uint32_t frame_rate);
private:
    Api_rtsp_server(){}
    ~Api_rtsp_server(){}
};

#endif // API_RTSPSERVER_H
