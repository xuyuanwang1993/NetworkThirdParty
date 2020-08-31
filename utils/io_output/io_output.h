#ifndef IO_OUTPUT_H
#define IO_OUTPUT_H
#include <cstdio>
#include<cstdint>
#include <iostream>
#include<string>
namespace micagent {
using namespace std;
class io_output_base{
public:
    struct const_string{
        const string const_str;
        const_string(const string &str="\r\n"):const_str(str){

        }
    };
    io_output_base(){}
    virtual ~io_output_base();
    io_output_base &operator<<(char  c){
#ifndef DISABLE_PRINT
        printf("%c",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(unsigned char  c){
#ifndef DISABLE_PRINT
        printf("%02x",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(short int c){
#ifndef DISABLE_PRINT
        printf("%hd",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(unsigned short int c){
#ifndef DISABLE_PRINT
        printf("%hu",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(int c){
#ifndef DISABLE_PRINT
        printf("%d",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(unsigned int c){
#ifndef DISABLE_PRINT
        printf("%u",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(long c){
#ifndef DISABLE_PRINT
        printf("%ld",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(unsigned long c){
#ifndef DISABLE_PRINT
        printf("%lu",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(signed long long int c){
#ifndef DISABLE_PRINT
        printf("%lld",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(unsigned long long int c){
#ifndef DISABLE_PRINT
        printf("%llu",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(double c){
#ifndef DISABLE_PRINT
        printf("%.6f",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(float c){
#ifndef DISABLE_PRINT
        printf("%.6f",static_cast<double>(c));
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(long double c){
#ifndef DISABLE_PRINT
        printf("%.12Lf",c);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(const char * str){
#ifndef DISABLE_PRINT
        if(str!=nullptr)printf("%s",str);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(const void * ptr){
#ifndef DISABLE_PRINT
        if(ptr==nullptr)printf("nil");
        else printf("%p",ptr);
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(std::nullptr_t ){
#ifndef DISABLE_PRINT
        printf("nil");
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(std::string str){
#ifndef DISABLE_PRINT
        printf("%s",str.c_str());
        return *this;
#else
        return *this;
#endif
    }
    io_output_base &operator<<(const const_string & str){
#ifndef DISABLE_PRINT
        printf("%s",str.const_str.c_str());
        return *this;
#else
        return *this;
#endif
    }
};
}
#endif // IO_OUTPUT_H
