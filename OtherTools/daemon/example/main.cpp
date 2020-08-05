#include <string>
#include<iostream>
#include<map>
#include "daemon_instance.h"
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include<sys/types.h>
#include<errno.h>
#include<stdio.h>
#include<string.h>
#define FD_PATH "/tmp/daemon_instance"
using namespace std;
using namespace micagent;
int main(int argc,char *argv[])
{
    //daemon_instance::generate_example_config();
    if(argc!=3)
    {
        printf("run with options :<run/exit> <config_path> \r\n");
        exit(-1);
    }
    string mode=argv[1];
    if(mode!="run"&&mode!="exit")
    {
        printf("run with options :<run/exit> <config_path> \r\n");
        exit(-1);
    }
    daemon_instance instance(argv[2],mode);
    if(!daemon_instance::single_run_test(instance.get_fd_name()))
    {
        exit(EXIT_FAILURE);
    }
    int status;
    while(1)
    {
        auto pid=fork();
        if(pid==0)
        {
            instance.run();
            exit(EXIT_SUCCESS);
        }
        else {
            pid_t pid_exit;
            pid_exit=wait(&status);
            printf("child pid %d for daemon_instance run exit!(%s)\r\n",pid_exit,strerror(errno));
            if(mode=="exit")exit(EXIT_SUCCESS);
            sleep(5);
        }
    }
    return 0;
}

