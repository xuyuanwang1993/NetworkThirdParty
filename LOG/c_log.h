#pragma once
#ifndef C_LOG_H
#define C_LOG_H
/*
 * compile with -std=c++11 [-DDEBUG]
 * complie with -DBACKTRACE and link with -rdynamic to open backtrace support
 * you can call print_backtrace to log the backtrace info
 */
#include <cstdio>
#include<cstdarg>
#include<string>
#include<mutex>
#include<condition_variable>
#include<queue>
#include<atomic>
#include<functional>
#include<thread>
#include<future>
#if defined(__linux) || defined(__linux__)
#elif defined(WIN32) || defined(_WIN32)
#endif
#if defined(__linux) || defined(__linux__)
#define DIR_DIVISION "/"
#elif defined(WIN32) || defined(_WIN32)
#define DIR_DIVISION "\\"
#pragma warning (disable: 4996)
#pragma 
#endif
#if defined(DEBUG) && 1
#define DEBUG_LOCK std::lock_guard<std::mutex>debug_locker(m_debug_mutex);
#else
#define DEBUG_LOCK
#endif

namespace micagent {
using namespace std;
enum LOG_LEVEL{
    LOG_DEBUG=0,
    LOG_INFO,
    LOG_WARNNING,
    LOG_ERROR,
    LOG_FATALERROR,
    LOG_BREAK_POINT,
    LOG_BACKTRACE,
};
using MAX_LOG_CACHE_CALLBACK=function<void (const string&)>;
class Logger{
#if defined(__linux) || defined(__linux__)
    const char LINE_END[2]={'\n','\0'};
#elif defined(WIN32) || defined(_WIN32)
    const char LINE_END[3]={'\r','\n','\0'};
#endif
    const uint64_t MAX_LOG_MESSAGE_SIZE=4*1024;//4k
    const int  MAX_TRACE_SIZE=256;
    const int  MIN_TRACE_SIZE=64;
    const LOG_LEVEL MIN_LOG_LEVEL=LOG_DEBUG;
    const LOG_LEVEL MAX_LOG_LEVEL=LOG_FATALERROR;
    const int MAX_LOG_FILE_SIZE=2*1024*1024;//2M
    const int MIN_LOG_FILE_SIZE=32*1024;//32K
    const int MIN_LOG_CACHE_CNT=0;
    const int MAX_LOG_CACHE_CNT=500;
    const int WAIT_TIME=1000;//ms
    const int MAX_WRITE_ERROR_TRY=100;
    const int MAX_LOG_QUEUE_SIZE=100000;
public:
    Logger &operator=(const Logger &) = delete;
    Logger(const Logger &) = delete;
    /*单例*/
    static Logger & Instance(){static Logger logger;return logger;}
    /*log接口*/
    void log(int level,const char *file,const char *func,int line,const char *fmt,...);
    /*设置log目录，当clear_flag 为true时 会清空原有log,目录无权限时会返回false*/
    bool set_log_path(const string &path,const string &proname);
    /*设置最小打印等级*/
    void set_minimum_log_level(int level);
    /*设置trace队列大小，不超过256，最小为64*/
    void set_dump_size(int size);
    /*设置是否在新建目录时清除log或log文件超出大小时删除原有log*/
    void set_clear_flag(bool clear);
    /*设置单个文件最大大小*/
    void set_log_file_size(long size);
    /*获取trace信息*/
    string get_dump_info()const;
    /*设置缓存区大小*/
    void set_log_cache_size(int cache_size);
    /*设置缓存满时的回调函数*/
    void set_log_cache_callback(const MAX_LOG_CACHE_CALLBACK &callback);
    /*标准输出开关*/
    void set_log_to_std(bool flag);
    /*获取当前本地时间*/
    static string get_local_time();
    /*获取log缓存*/
    string get_log_cache();
    void register_handle();
    void unregister_handle();
    bool get_register_status()const{
        DEBUG_LOCK
                return m_registered;
    }
    //获取堆栈信息
    void print_backtrace();
    //获取当前堆栈容量
    int get_dump_size()const{
        std::lock_guard<mutex> locker(m_dump_mutex);
        return m_dump_max_size;
    }
private:
    Logger();
    ~Logger();
    /*判断文件是否打开*/
    bool open_file();
    /*追加到文件中*/
    inline bool append_to_file(const string & log_message);
    /*重置文件信息*/
    void reset_file();
    /*log处理线程*/
    void run();
    shared_ptr<thread>m_thread;
    atomic<bool> m_registered;
    condition_variable m_conn;
    /*停止标识*/
    atomic<bool> m_stop;
    /*文件写入指针*/
    FILE *m_fp;
    /*log目录*/
    string m_save_path;
    /*程序名*/
    string m_pro_name;
    /*目录是否可写*/
    bool m_env_set;
    /*配置读写锁*/
    std::mutex m_mutex;
    /*LOG等级*/
    atomic<int> m_level;
    /*log清理标识*/
    bool m_clear_flag;
    /*log文件最大大小*/
    int64_t m_max_file_size;
    /*log缓存*/
    queue<shared_ptr<string>>  m_log_buf;
    /*trace 队列读写锁*/
    mutable mutex m_dump_mutex;
    /*trace 队列*/
    queue<shared_ptr<string>>m_dump_queue;
    /*trace 队列最大长度*/
    int m_dump_max_size;
    /*log缓存*/
    string m_log_cache;
    /*缓存log计数*/
    int m_log_cnt;
    /*最大缓存数*/
    int m_max_log_cnt;
    /*缓存满时调用的回调函数*/
    MAX_LOG_CACHE_CALLBACK m_cache_callback;
    /*cache读写锁*/
    mutex m_cache_mutex;
    /*写入错误累计*/
    int m_write_error_cnt;
    /*标准输出开关*/
    atomic_bool m_log_to_std;
#ifdef DEBUG
    mutable mutex m_debug_mutex;
#endif
};
}
//define log macro
#undef MICAGENT_LOG
#undef MICAGENT_DEBUG
#undef MICAGENT_INFO
#undef MICAGENT_WARNNING
#undef MICAGENT_ERROR
#undef MICAGENT_FATALERROR
#undef MICAGENT_BACKTRACE
#ifndef MICAGENT_ARM
#define MICAGENT_LOG(level,fmt,...) micagent::Logger::Instance().log(level,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#define MICAGENT_DEBUG(fmt,...) micagent::Logger::Instance().log(micagent::LOG_DEBUG,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#define MICAGENT_INFO(fmt,...) micagent::Logger::Instance().log(micagent::LOG_INFO,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#define MICAGENT_WARNNING(fmt,...) micagent::Logger::Instance().log(micagent::LOG_WARNNING,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#define MICAGENT_ERROR(fmt,...) micagent::Logger::Instance().log(micagent::LOG_ERROR,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#define MICAGENT_FATALERROR(fmt,...) micagent::Logger::Instance().log(micagent::LOG_FATALERROR,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#define MICAGENT_BACKTRACE(fmt,...) micagent::Logger::Instance().log(micagent::LOG_BACKTRACE,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#else
#define MICAGENT_LOG(level,fmt,...)
#define MICAGENT_DEBUG(fmt,...)
#define MICAGENT_INFO(fmt,...)
#define MICAGENT_WARNNING(fmt,...)
#define MICAGENT_ERROR(fmt,...)
#define MICAGENT_FATALERROR(fmt,...)
#define MICAGENT_BACKTRACE(fmt,...)
#endif
#ifdef DEBUG
#define MICAGENT_MARK(fmt,...) micagent::Logger::Instance().log(micagent::LOG_BREAK_POINT,__FILE__, __FUNCTION__,__LINE__, fmt, ##__VA_ARGS__)
#else
#define MICAGENT_MARK(fmt,...)
#endif
#endif // C_LOG_H
