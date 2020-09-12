#include "io_output.h"
#include "bits_helper.h"
#include "iobserver.h"
#include <thread>
#include<unistd.h>
#include<random>
using namespace micagent;
#define TEST_DECLARE(name) void name##_test()
#define TEST_RUN(name) name##_test()
TEST_DECLARE(io_output);
TEST_DECLARE(bits_helper);
TEST_DECLARE(observer);
int main()
{
    TEST_RUN(observer);
    return 0;
}
static  io_output_base base;
TEST_DECLARE(io_output)
{

    base<<(1ULL<<63)+1<<"\r\n";
    char *test=nullptr;
    base<<test<<"\r\n";
}
TEST_DECLARE(bits_helper)
{
    struct test1{
        int i :7;
        int j:5;
        int k:4;
        int q:2;
        int p:3;
        int d:11;
    };
    base<<"size :"<<sizeof (test1)<<"\r\n";
    test1 qwer;
    qwer.i=31;
    qwer.j=10;
    qwer.k=3;
    qwer.q=2;
    qwer.p=2;
    qwer.d=41;
    uint64_t tmp;
    bits_context context(&qwer,sizeof (qwer));
    context.read_bits(7,tmp);
    base<<"i: "<<tmp<<"\r\n";
    context.read_bits(5,tmp);
    base<<"j: "<<tmp<<"\r\n";
    context.read_bits(4,tmp);
    base<<"k: "<<tmp<<"\r\n";
    context.read_bits(2,tmp);
    base<<"q: "<<tmp<<"\r\n";
    context.read_bits(3,tmp);
    base<<"p: "<<tmp<<"\r\n";
    context.read_bits(11,tmp);
    base<<"d: "<<tmp<<"\r\n";
    context.write_bits(7,25,0);
    context.write_bits(5,10);
    context.write_bits(4,1);
    context.write_bits(2,3);
    context.write_bits(3,5);
    context.write_bits(11,128);
    memcpy(&qwer,context.get_src_buf().get(),context.get_buf_size());

    {
        bits_context context(&qwer,sizeof (qwer));
        context.read_bits(7,tmp);
        base<<"i: "<<tmp<<"\r\n";
        context.read_bits(5,tmp);
        base<<"j: "<<tmp<<"\r\n";
        context.read_bits(4,tmp);
        base<<"k: "<<tmp<<"\r\n";
        context.read_bits(2,tmp);
        base<<"q: "<<tmp<<"\r\n";
        context.read_bits(3,tmp);
        base<<"p: "<<tmp<<"\r\n";
        context.read_bits(11,tmp);
        base<<"d: "<<tmp<<"\r\n";
    }
    base<<bits_context::get_bit_mask(63,3)<<"\r\n";
}
TEST_DECLARE(observer)
{
    {
        iobserver<int> server;
        server.subscribe("test","test",[](const int &value){
            printf("test data %d was received!\r\n",value);
        });
        server.subscribe("test1","test",[](const int &value){
            printf("test1 data %d was received!\r\n",value);
        });
        server.notify("test",5);
        server.unsubscribe("test1","test");
        server.notify("test",6);
    }
    //multithread
    {
        class test_observer:public iobserver<string>{};
        test_observer server;
        auto func_subscribe=[&](){
            server.subscribe("test0","test0",[](const string&value){
                printf("test0 data %s was received!\r\n",value.c_str());
            });
            printf("add test0 test0!\r\n");
            sleep(1);
            server.subscribe("test1","test1",[](const string&value){
                printf("test1 data %s was received!\r\n",value.c_str());
            });
            printf("add test1 test1!\r\n");
            sleep(2);
            server.subscribe("test2","test2",[](const string&value){
                printf("test2 data %s was received!\r\n",value.c_str());
            });
            printf("add test2 test2!\r\n");
            sleep(1);
            server.subscribe("test2","test2",[](const string&value){
                printf("test_error data %s was received!\r\n",value.c_str());
            });
            printf("modify test2 test2!\r\n");
            sleep(1);
            server.unsubscribe("test0","test0");
            server.unsubscribe("test1","test1");
            server.unsubscribe("test2","test2");
            printf("delete all!\r\n");
        };
        auto func_data_source=[&](){
            random_device rd;
            for(int i=0;i<60;++i)
            {
                int mode=rd()%3;
                int string_value=rd()%10000;
                string base("test");
                base+=to_string(mode);
                string value(to_string(string_value));
                printf("send mode %s   value %s \r\n",base.c_str(),value.c_str());
                server.notify(base,value);
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        };
        thread t1(func_subscribe);
        thread t2(func_data_source);
        t1.join();
        t2.join();
    }

}
