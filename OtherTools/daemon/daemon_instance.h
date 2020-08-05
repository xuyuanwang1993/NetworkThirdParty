#ifndef DAEMON_INSTANCE_H
#define DAEMON_INSTANCE_H
#include <string>
#include<cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <mutex>
#include<queue>
#include<condition_variable>
#include<map>
#include <vector>
namespace micagent {
using namespace std;
class daemon_instance{
    struct pro_check_task{
        uint32_t index;
        uint32_t wait_time;
        string path;
        string pro_name;
        string cmd_options;
    };
public:
    daemon_instance(string config_path,string mode);
    static bool single_run_test(string test_file_name);
    void run();
    ~daemon_instance(){

    }
    string get_fd_name()const{
        return m_fd_name;
    }
    static void generate_example_config();
private:
    void task_handle();
    void exit_handle();
    string m_fd_name;
    bool m_only_clear;
    mutex m_mutex;
    condition_variable m_conn;
    queue<pro_check_task> m_tasks;
    vector<pro_check_task> m_task_vec;
    map<pid_t,uint32_t> m_task_map;
};
}
#endif // DAEMON_INSTANCE_H
