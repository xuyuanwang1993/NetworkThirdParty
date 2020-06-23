#include <string>
#include<iostream>
#include<map>
#include "rc4.h"
using namespace std;
using namespace micagent;
int main(int argc,char *argv[])
{
    //auto func=[]()->int64_t{return 0;};
    rc4_interface test([]()->int64_t{return 0;});
    const uint32_t size=118;
    //char test_string[size]="erwerefwrtgefwqretwfvedtwfceytweerewrewryewrgeywfewtefwtgecfwqecwfqecfqwyecfctqweffwqtyedeqetwedwqededqeduewetfwewewq";
    string test2(size,'a');
    const char *test_string=test2.c_str();
    char *out_string=(char *)calloc(1,test2.size()+8);
    char *test_string2=(char *)calloc(1,test2.size());
    printf("%d \r\n",test.encrypt(test_string,out_string,size,size+8));
    printf("encrypt %s \r\n",out_string);
    rc4_interface test3([]()->int64_t{return 0;});
    printf("%d \r\n",test3.decrypt(out_string,test_string2,size+8,size,rc4_interface::generate_key()));
    printf("decrypt %s\r\n",test_string2);
    printf("%d  \r\n",strcmp(test_string,test_string2));
    return 0;
}

