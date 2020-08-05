#include <cstdio>
#include <unistd.h>
#include<random>
#include <exception>
using namespace std;
int main()
{
    fprintf(stderr,"normal_test\r\n");
    random_device rd;
    int sleep_time=rd()%10+15;
    while(1)
    {
        sleep(sleep_time);
        throw "exit with a exception";
    }
}
