#include "io_output.h"
#include "bits_helper.h"
using namespace micagent;
#define TEST_DECLARE(name) void name##_test()
#define TEST_RUN(name) name##_test()
TEST_DECLARE(io_output);
TEST_DECLARE(bits_helper);
int main()
{

    TEST_RUN(bits_helper);
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

