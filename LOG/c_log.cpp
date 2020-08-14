#include "c_log.h"
#include <iomanip>
#include <chrono>
#include<memory>
#include<sstream>
#include<iostream>
#include<string.h>
#include<cstdio>
#include<cstdlib>
#if defined(__linux) || defined(__linux__)
#include <sys/stat.h>
#include<unistd.h>
#elif defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#include<io.h>
#endif
using namespace micagent;
constexpr static char log_strings[][20]={
    "DEBUG",
    "INFO",
    "WARNNING",
    "ERROR",
    "FATAL_ERROR",
    "BREAK_MARK",
};
#if defined(__linux) || defined(__linux__)
#define PRINT_NONE               "\033[m"
#define PRINT_RED                "\033[0;32;31m"
#define PRINT_LIGHT_RED          "\033[1;31m"
#define PRINT_GREEN              "\033[0;32;32m"
#define PRINT_BLUE               "\033[0;32;34m"
#define PRINT_YELLOW             "\033[1;33m"
#define PRINT_BROWN              "\033[0;33m"
#define PRINT_PURPLE             "\033[0;35m"
#define PRINT_CYAN               "\033[0;36m"
constexpr static char log_color[][20] = {
	PRINT_NONE,
	PRINT_GREEN,
	PRINT_YELLOW,
	PRINT_BROWN,
	PRINT_RED,
	PRINT_PURPLE,
};
#elif defined(WIN32) || defined(_WIN32)
#define FOREGROUND_BLUE      0x0001 // text color contains blue.
#define FOREGROUND_GREEN     0x0002 // text color contains green.
#define FOREGROUND_CYAN     0x0003
#define FOREGROUND_RED       0x0004 // text color contains red.
#define FOREGROUND_PURPLE	0x0005
#define FOREGROUND_YELLOW	0x0006
#define FOREGROUND_NONE	0x0007
constexpr static int log_color[] = {
	FOREGROUND_NONE,
	FOREGROUND_GREEN,
	FOREGROUND_YELLOW,
	FOREGROUND_CYAN,
	FOREGROUND_RED,
	FOREGROUND_PURPLE,
};
#endif
string Logger::get_local_time(){
    static mutex time_mutex;
    lock_guard<mutex>locker(time_mutex);
    std::ostringstream stream;
    auto now = chrono::system_clock::now();
    time_t tt = chrono::system_clock::to_time_t(now);

#if defined(WIN32) || defined(_WIN32)
    struct tm tm;
    localtime_s(&tm, &tt);
    stream << std::put_time(&tm, "%F %T");
#elif  defined(__linux) || defined(__linux__)
    char buffer[200] = {0};
    std::string timeString;
    std::strftime(buffer, 200, "%F %T", std::localtime(&tt));
    stream << buffer;
#endif
    return stream.str();
}
void Logger::log(int level,const char *file,const char *func,int line,const char *fmt,...){
    if(!get_register_status())return;
    //小于最小打印等级的log不处理
    {
        DEBUG_LOCK
         if(level<m_level||level>LOG_BREAK_POINT)return;
    }
    shared_ptr<char>buf(new char[MAX_LOG_MESSAGE_SIZE+1],std::default_delete<char[]>());
    buf.get()[MAX_LOG_MESSAGE_SIZE]='\0';
    {
        std::lock_guard<mutex> locker(m_mutex);
        int use_len=snprintf(buf.get(),MAX_LOG_MESSAGE_SIZE,"[%s][%s %s %d]",log_strings[level],file,func,line);
        va_list arg;
        va_start(arg, fmt);
        vsnprintf(buf.get() + use_len, MAX_LOG_MESSAGE_SIZE - use_len, fmt, arg);
        va_end(arg);
#ifdef DEBUG
        if(m_log_to_std){
            /*输出到标准输出*/
#if defined(__linux) || defined(__linux__)
            if(level<LOG_ERROR)fprintf(stdout,"%s%s%s%s",log_color[level],buf.get(),LINE_END,log_color[0]);
            else fprintf(stderr,"%s%s%s%s",log_color[level],buf.get(),LINE_END,log_color[0]);
#elif defined(WIN32) || defined(_WIN32)
            if (level < LOG_ERROR) {
                HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
                SetConsoleTextAttribute(handle, log_color[level]);
                fprintf(stdout, "%s%s",  buf.get(), LINE_END);
                SetConsoleTextAttribute(handle, FOREGROUND_INTENSITY|log_color[0]);
            }
            else {
                HANDLE handle = GetStdHandle(STD_ERROR_HANDLE);
                SetConsoleTextAttribute(handle, log_color[level]);
                fprintf(stderr, "%s%s", buf.get(), LINE_END);
                SetConsoleTextAttribute(handle, log_color[0]);
            }
#endif
        }
#endif
    }
    if(level==LOG_BREAK_POINT){
    /*断点信息存储*/
        lock_guard<mutex> locker(m_dump_mutex);
        while(m_dump_queue.size()>=m_dump_max_size)m_dump_queue.pop();
        m_dump_queue.push(make_shared<string>(buf.get()));
    }
    std::unique_lock<mutex> locker(m_mutex);
    if(m_log_buf.size()<MAX_LOG_QUEUE_SIZE)m_log_buf.push(make_shared<string>(buf.get()));
    m_conn.notify_all();
}
bool Logger::set_log_path(const string &path,const string &proname){
    std::lock_guard<mutex> locker(m_mutex);
    if(m_fp){
    fclose(m_fp);
    m_fp=nullptr;
    }
    if(m_clear_flag)reset_file();
    m_env_set=false;
    m_save_path=path;
    m_pro_name=proname;
#if defined(__linux) || defined(__linux__)
	if (m_save_path.empty()) {
		m_save_path = ".";
		m_save_path += DIR_DIVISION;
	}
	else mkdir(m_save_path.c_str(), 0777);
	if (access(m_save_path.c_str(), F_OK | W_OK) != -1)m_env_set = true;
#elif defined(WIN32) || defined(_WIN32)
	m_env_set = true;
	if (m_save_path.empty()) {
		m_save_path = ".";
		m_save_path += DIR_DIVISION;
	}
	else if( CreateDirectory(m_save_path.c_str(), nullptr)!=0)m_env_set=false;
#endif
    return m_env_set;
}
void Logger::set_minimum_log_level(int level){
    DEBUG_LOCK
    if(level<LOG_DEBUG)m_level.exchange(LOG_DEBUG);
    else if(level>LOG_FATALERROR)m_level.exchange(LOG_FATALERROR);
    else {
        m_level.exchange(level);
    }
}
void Logger::set_dump_size(int size){
    std::lock_guard<mutex> locker(m_dump_mutex);
    m_dump_max_size=size;
    if(m_dump_max_size<MIN_TRACE_SIZE)m_dump_max_size=MIN_TRACE_SIZE;
    else if(m_dump_max_size>MAX_TRACE_SIZE)m_dump_max_size=MAX_TRACE_SIZE;
}
void Logger::set_clear_flag(bool clear)
{
    lock_guard<mutex> locker(m_mutex);
    m_clear_flag=clear;
}
void Logger::set_log_file_size(long size)
{
lock_guard<mutex> locker(m_mutex);
    m_max_file_size=size;
    if(m_max_file_size<MIN_LOG_FILE_SIZE)m_max_file_size=MIN_LOG_FILE_SIZE;
    else if(m_max_file_size>MAX_LOG_FILE_SIZE)m_max_file_size=MAX_LOG_FILE_SIZE;
}
string Logger::get_dump_info()const{
    std::lock_guard<mutex> locker(m_dump_mutex);
    string ret_string;
    auto tmp_queue(m_dump_queue);
    while(!tmp_queue.empty()){
    ret_string+=*tmp_queue.front().get();
    ret_string+=LINE_END;
    tmp_queue.pop();
    }
    return ret_string;
}
Logger::Logger():m_registered(false),m_stop(false),m_fp(nullptr),m_save_path(string(".")+DIR_DIVISION),m_env_set(false),m_level(MIN_LOG_LEVEL),m_clear_flag(false)\
,m_max_file_size(MIN_LOG_FILE_SIZE),m_dump_max_size(MIN_TRACE_SIZE)\
,m_log_cnt(0),m_max_log_cnt(MIN_LOG_CACHE_CNT),m_cache_callback(nullptr),m_write_error_cnt(0),m_log_to_std(false)
{

}
void Logger::register_handle()
{
    if(!get_register_status()){
        {
            DEBUG_LOCK
            m_registered.exchange(true);
            m_stop.exchange(false);
        }
        unique_lock<mutex>locker(m_mutex);
        m_thread.reset(new thread(&Logger::run,this));
    }
}
void Logger::unregister_handle()
{
    {
        DEBUG_LOCK
        m_stop.exchange(true);
    }
    {
        unique_lock<mutex>locker(m_mutex);
        if(m_fp)fclose(m_fp);
        m_env_set=false;
        m_conn.notify_all();
    }
    if(m_thread&&m_thread->joinable())m_thread->join();
    {
        DEBUG_LOCK
        m_registered.exchange(false);
    }
}
Logger::~Logger(){
    if(get_register_status())unregister_handle();
}
bool Logger::open_file()
{
    bool ret=false;
    do{
        if(!m_env_set||m_save_path.empty())break;
        if(!m_fp){
            auto time_string=get_local_time();
			for (auto& i : time_string)if (i == ' ')i = '#'; else if (i == ':')i = '-';
            string file_name=m_save_path+DIR_DIVISION+time_string+"#"+m_pro_name+".log";
            m_fp=fopen(file_name.c_str(),"w+");
            if(m_fp)ret=true;
        }
        else{
            auto len=ftell(m_fp);
            if(len>m_max_file_size){
                fclose(m_fp);
                m_fp=nullptr;
                if(m_clear_flag)reset_file();
                auto time_string=get_local_time();
				for (auto& i : time_string)if (i == ' ')i = '#'; else if (i == ':')i = '-';
                string file_name=m_save_path+DIR_DIVISION+time_string+"#"+m_pro_name+".log";
                m_fp=fopen(file_name.c_str(),"w+");
                if(m_fp)ret=true;
            }
            else ret=true;
        }
    }while(0);
    return ret;
}
bool Logger::append_to_file(const string & log_message)
{
    bool ret=true;
    do{
        auto ret=fwrite(log_message.c_str(),1,log_message.size(),m_fp);
        fflush(m_fp);
        if(ret<log_message.size())ret=false;
    }while(0);
    return ret;
}
void Logger::run(){
    std::unique_lock<mutex> locker(m_mutex);
    while(!m_stop){
        if(m_log_buf.empty())m_conn.wait_for(locker,std::chrono::milliseconds(WAIT_TIME));
        if(m_log_buf.empty())continue;
        auto time_str=get_local_time();
        while(!m_log_buf.empty()){
        string log_info="[";
            log_info=log_info+time_str+"]"+*m_log_buf.front()+LINE_END;
            {/*添加cache*/
                lock_guard<mutex> cache_locker(m_cache_mutex);
                if(m_max_log_cnt>0){
                    m_log_cache+=log_info;
                    m_log_cnt++;
                }
                if(m_log_cnt>=m_max_log_cnt){
                    if(m_cache_callback)m_cache_callback(m_log_cache);
                    m_log_cache.clear();
                    m_log_cnt=0;
                }
            }
            {/*存至文件*/
                if(!open_file()||!append_to_file(log_info))m_write_error_cnt++;
                if(m_write_error_cnt>MAX_WRITE_ERROR_TRY&&m_env_set)reset_file();
            }
            m_log_buf.pop();
        }
        m_conn.wait(locker);
    }
}
void Logger::set_log_cache_size(int cache_size)
{
    std::lock_guard<mutex>locker(m_cache_mutex);
    m_max_log_cnt=cache_size;
    if(m_max_log_cnt<MIN_LOG_CACHE_CNT)m_max_log_cnt=MIN_LOG_CACHE_CNT;
    else if(m_max_log_cnt>MAX_LOG_CACHE_CNT)m_max_log_cnt=MAX_LOG_CACHE_CNT;
}
void Logger::set_log_cache_callback(const MAX_LOG_CACHE_CALLBACK &callback)
{
    std::lock_guard<mutex>locker(m_cache_mutex);
    m_cache_callback=callback;
}
void Logger::set_log_to_std(bool flag)
{
    m_log_to_std.exchange(flag);
}
string Logger::get_log_cache()
{
    std::lock_guard<mutex>locker(m_cache_mutex);
    std::string ret(m_log_cache);
    m_log_cache.clear();
    m_log_cnt=0;
    return ret;
}
void Logger::reset_file(){
	m_write_error_cnt = 0;
	string file_name = m_save_path;
	if (!m_save_path.empty())file_name += DIR_DIVISION;
	file_name += "*.log";
#if defined(__linux) || defined(__linux__)
	string cmd = "rm -rf ";
	cmd += file_name;
#elif defined(WIN32) || defined(_WIN32)
	string cmd = "del ";
	cmd += file_name;
#endif
	system(cmd.c_str());
}
