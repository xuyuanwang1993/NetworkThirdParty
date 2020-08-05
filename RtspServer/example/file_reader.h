#ifndef FILE_READER_H
#define FILE_READER_H
#include <vector>
#include<unordered_map>
#include<string>
#include<memory>
#include<cstdio>
#ifndef PRINT_NONE
#define PRINT_NONE               "\033[m"
#endif
#ifndef PRINT_RED
#define PRINT_RED                "\033[0;32;31m"
#endif
#ifndef PRINT_LIGHT_RED
#define PRINT_LIGHT_RED          "\033[1;31m"
#endif
#ifndef PRINT_GREEN
#define PRINT_GREEN              "\033[0;32;32m"
#endif
#ifndef PRINT_BLUE
#define PRINT_BLUE               "\033[0;32;34m"
#endif
#ifndef PRINT_YELLOW
#define PRINT_YELLOW             "\033[1;33m"
#endif
#ifndef PRINT_BROWN
#define PRINT_BROWN              "\033[0;33m"
#endif
#ifndef PRINT_PURPLE
#define PRINT_PURPLE             "\033[0;35m"
#endif
#ifndef PRINT_CYAN
#define PRINT_CYAN               "\033[0;36m"
#endif


#define MICAGENT_DEBUG
#ifndef MICAGENT_DEBUG
#define MICAGENT_LOG1(fmt,...)
#else
#define MICAGENT_LOG1(fmt,...) do{\
    printf("[MICAGENT_LOG1 %s %s %d] " fmt "\r\n" PRINT_NONE,__FILE__,__FUNCTION__,__LINE__, ##__VA_ARGS__ );\
}while(0);
#endif
namespace micagent {
using std::string;
using std::unordered_map;
using std::vector;
using std::shared_ptr;
class file_reader_base{
    static const unsigned int DEFAULT_STREAM_BUF_SIZE=2000000;
public:
    file_reader_base(string fileName);
    file_reader_base()=delete;
    size_t readFrame(unsigned char *inBuf,unsigned int inBufSize,size_t index=0,bool jump=false);
    void show_a_frame_info(size_t index,size_t max_out_put_size)const;
    void show_all_frame_info(size_t max_out_put_size)const;
    size_t get_current_index()const{return m_last_index; }
    void dump_file_info(size_t items=0)const;
    virtual ~file_reader_base();
protected:
    size_t m_last_index;
    unordered_map<unsigned char,vector<size_t>>m_frame_info_map;
    vector<size_t>m_frame_begin_poses;
    size_t m_file_size;
private:
    FILE *m_fp;
    string m_file_name;
    void parase_stream_info();
};
}


#endif // FILE_READER_H
