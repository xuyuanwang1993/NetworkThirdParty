#include "io_output.h"
using namespace micagent;
#define TEST_DECLARE(name) void name##_test()
#define TEST_SAMPLE(name,str) void name##_test(){str}
#define TEST_RUN(name) name##_test()
TEST_DECLARE(io_output);

int main()
{
    TEST_RUN(io_output);
    return 0;
}

TEST_SAMPLE(io_output,\
            io_output_base base;\
base<<(1ULL<<63)+1<<"\r\n";\
char *test=nullptr;\
base<<test<<"\r\n";\
)
