#include "c_log.h"
#include "iostream"
#include<random>
#if defined(__linux) || defined(__linux__)
#elif defined(WIN32) || defined(_WIN32)
#include <Windows.h>
#endif

using namespace std;
#include <mutex>
#include<thread>
#include<algorithm>
int main(){
	micagent::Logger::Instance().register_handle();
	atomic_int sleep_time(1000);
	std::cout << micagent::Logger::get_local_name() << endl;
	atomic_bool is_exit(false);
    std::thread t1([&](){
    int count1=0;
        while(!is_exit){
            MICAGENT_LOG(micagent::LOG_INFO," thread one %d ",count1++);
            MICAGENT_LOG(micagent::LOG_DEBUG," thread one %d ",count1++);
            this_thread::sleep_for(chrono::milliseconds(sleep_time));
        }
    });
    std::thread t2([&](){
    int count=0;
        while(!is_exit){
           MICAGENT_LOG(micagent::LOG_INFO," thread two %d ",count++);
            MICAGENT_LOG(micagent::LOG_ERROR," thread two %d ",count++);
            this_thread::sleep_for(chrono::milliseconds(sleep_time));
            MICAGENT_MARK(" test mark %d!",count);
        }
    });

    auto func=[](){
        cout<<"help info!"<<endl;
        cout<<"'h' to show this page! "<<endl;
        cout<<"'a' set_log_path /tmp/log"<<endl;
        cout<<"'b' set_log_path log"<<endl;
        cout<<"'c' set_minimum_log_level error"<<endl;
        cout<<"'d' set_minimum_log_level info"<<endl;
        cout<<"'e' set_dump_size 20"<<endl;
        cout<<"'f' set_clear_flag false"<<endl;
        cout<<"'g' set_clear_flag true"<<endl;
        cout<<"'i' set_log_file_size 30k"<<endl;
        cout<<"'j' set_log_file_size 200000"<<endl;
        cout<<"'k' set_log_cache_size 5"<<endl;
        cout<<"'l' set_log_cache_size 20"<<endl;
        cout<<"'m' set_log_cache_callback print ------"<<endl;
        cout<<"'n' set_log_cache_callback print *****"<<endl;
        cout<<"'o' get_log_cache"<<endl;
        cout<<"'p' get_dump_info "<<endl;
        cout<<"'q' set_dump_size 5"<<endl;
        cout<<"'r' set_log_to_std false"<<endl;
        cout<<"'s' set_log_to_std true"<<endl;
        cout<<"'t' sleep_time 10ms"<<endl;
        cout<<"'u' sleep_time 100ms"<<endl;
        cout<<"'v' sleep_time 1000ms"<<endl;
        cout<<"'w' set_log_cache_callback nullptr"<<endl;
        cout<<"***********************************************"<<endl;
    };
    auto callback1=[](const string &str){
        cout<<"-----------------------------"<<endl;
    };
    auto callback2=[](const string &str){
        cout<<"*********************"<<endl;
    };
    func();
    while(1){
        char c=getchar();
        fflush(stdin);
        while(getchar()!='\n');
        switch (c) {
        case 'h':
            func();
        case 'a':
#if defined(__linux) || defined(__linux__)
			micagent::Logger::Instance().set_log_path("/tmp/", "log");
#elif defined(WIN32) || defined(_WIN32)
			micagent::Logger::Instance().set_log_path("C:\\Temp", "log");
#endif
            break;
        case 'b':
            micagent::Logger::Instance().set_log_path(std::string(),"log");
            break;
        case 'c':
            micagent::Logger::Instance().set_minimum_log_level(micagent::LOG_ERROR);
            break;
        case 'd':
            micagent::Logger::Instance().set_minimum_log_level(micagent::LOG_INFO);
            break;
        case 'e':
            micagent::Logger::Instance().set_dump_size(20);
            break;
        case 'f':
            micagent::Logger::Instance().set_clear_flag(false);
            break;
        case 'g':
            micagent::Logger::Instance().set_clear_flag(true);
            break;
        case 'i':
            micagent::Logger::Instance().set_log_file_size(30);
            break;
        case 'j':
            micagent::Logger::Instance().set_log_file_size(200000);
            break;
        case 'k':
            micagent::Logger::Instance().set_log_cache_size(5);
            break;
        case 'l':
            micagent::Logger::Instance().set_log_cache_size(20);
            break;
        case 'm':
            micagent::Logger::Instance().set_log_cache_callback(callback1);
            break;
        case 'n':
            micagent::Logger::Instance().set_log_cache_callback(callback2);
            break;
        case 'o':
            std::cout<<micagent::Logger::Instance().get_log_cache()<<endl;;
            break;
        case 'p':
            std::cout<<micagent::Logger::Instance().get_dump_info()<<endl;
            break;
        case 'q':
            micagent::Logger::Instance().set_dump_size(5);
            break;
        case 'r':
            micagent::Logger::Instance().set_log_to_std(false);
            break;
        case 's':
            micagent::Logger::Instance().set_log_to_std(true);
            break;
        case 't':
            sleep_time.exchange(10);
            break;
        case 'u':
            sleep_time.exchange(100);
            break;
        case 'v':
            sleep_time.exchange(1000);
            break;
        case 'w':
            micagent::Logger::Instance().set_log_cache_callback(nullptr);
            break;
        case 'x':
            goto MY_END;
            break;
        default:
            cout<<"unkown command ("<<c<<")"<<endl;
            break;
        }
        cout<<" input your choice:"<<endl;
    }
MY_END:
    //micagent::Logger::Instance().unregister_handle();
    is_exit.exchange(true);
    t1.join();
    t2.join();
    return 0;
}
