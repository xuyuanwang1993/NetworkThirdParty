#ifndef IO_OUTPUT_H
#define IO_OUTPUT_H
#include <cstdio>
#include<cstdint>
namespace micagent {
using namespace std;
class io_output_base{
public:
    io_output_base(){}
    virtual ~io_output_base();
    io_output_base &operator<<(int8_t c){
        printf("%c",c);
        return *this;
    }
    io_output_base &operator<<(uint8_t c){
        printf("%02x",c);
        return *this;
    }
    io_output_base &operator<<(int16_t c){
        printf("%hd",c);
        return *this;
    }
    io_output_base &operator<<(uint16_t c){
        printf("%hu",c);
        return *this;
    }
    io_output_base &operator<<(int32_t c){
        printf("%d",c);
        return *this;
    }
    io_output_base &operator<<(uint32_t c){
        printf("%u",c);
        return *this;
    }
    io_output_base &operator<<(int64_t c){
        printf("%ld",c);
        return *this;
    }
    io_output_base &operator<<(uint64_t c){
        printf("%lu",c);
        return *this;
    }
    io_output_base &operator<<(signed long long int c){
        printf("%lld",c);
        return *this;
    }
    io_output_base &operator<<(unsigned long long int c){
        printf("%llu",c);
        return *this;
    }
    io_output_base &operator<<(double c){
        printf("%.6f",c);
        return *this;
    }
    io_output_base &operator<<(long double c){
        printf("%.12Lf",c);
        return *this;
    }
    io_output_base &operator<<(const char * str){
        if(str!=nullptr)printf("%s",str);
        return *this;
    }
    io_output_base &operator<<(const void * ptr){
        if(ptr==nullptr)printf("nil");
        else printf("%p",ptr);
        return *this;
    }
    io_output_base &operator<<(std::nullptr_t ){
        printf("nil");
        return *this;
    }
};
}
#endif // IO_OUTPUT_H
