#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include<sys/file.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread>
#include "daemon_instance.h"
#include <sys/wait.h>
#include <map>
using namespace micagent;
using namespace std;
using neb::CJsonObject;
#define BUF_LEN_FOR_PID 64
static int write_pid_into_fd(int fd, pid_t pid,string file_name)
{

    int ret = -1;
    char buf[BUF_LEN_FOR_PID] = {0};

    /* Move cursor to the start of file. */
    lseek(fd, 0, SEEK_SET);

    sprintf(buf, "%d", pid);
    ret = write(fd, buf, strlen(buf));
    if(ret <= 0) { /* Write fail or write 0 byte */
        if(ret == -1)
        {
            char error_info[256]={0};
            sprintf(error_info,"Write %s fail\n",file_name.c_str());
            perror(error_info);
        }
        ret = -1;
    } else {
        printf("Create %s ok, pid=%d\n", file_name.c_str(),pid);
        ret = 0;
    }

    return ret;
}

/*
 * Create MY_PID_FILE, write pid into it.
 *
 * @return: 0 is ok, -1 is error.
 */
static int create_pid_file(pid_t pid,string file_name)
{
    int fd, ret;

    fd = open(file_name.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0666);  /* rw-rw-rw- */
    if(fd == -1) {
        char error_info[256]={0};
        sprintf(error_info,"Create %s fail\n",file_name.c_str());
        perror(error_info);
        return -1;
    }

    ret = flock(fd, LOCK_EX);
    if(ret == -1) {
        char error_info[256]={0};
        sprintf(error_info,"flock %s fail\n",file_name.c_str());
        perror(error_info);
        close(fd);
        return -1;
    }
    ret = write_pid_into_fd(fd, pid,file_name);

    flock(fd, LOCK_UN);
    close(fd);

    return ret;
}

/*
 * If pid file already exists, check the pid value in it.
 * If pid from file is still running, this program need exit();
 * If it is not running, write current pid into file.
 *
 * @return: 0 is ok, -1 is error.
 */
static int check_pid_file(int fd, pid_t pid,string file_name)
{
    int ret = -1;
    pid_t old_pid;
    char buf[BUF_LEN_FOR_PID] = {0};

    ret = flock(fd, LOCK_EX);
    if(ret == -1) {
        char error_info[256]={0};
        sprintf(error_info,"flock %s fail\n",file_name.c_str());
        perror(error_info);
        close(fd);
        return -1;
    }

    ret = read(fd, buf, sizeof(buf)-1);
    if(ret < 0) {  /* read error */
        char error_info[256]={0};
        sprintf(error_info,"read from %s fail\n",file_name.c_str());
        perror(error_info);
        ret = -1;
    } else if(ret > 0) {  /* read ok */
        old_pid = atol(buf);

        /* Check if old_pid is running */
        ret = kill(old_pid, 0);
        if(ret < 0) {
            if(errno == ESRCH) { /* old_pid is not running. */
                ret = write_pid_into_fd(fd, pid,file_name);
            } else {
                perror("send signal fail\n");
                ret = -1;
            }
        } else {  /* running */
            printf("Program already exists, pid=%d\n", old_pid);
            ret = -1;
        }
    } else if(ret == 0) { /* read 0 byte from file */
        ret = write_pid_into_fd(fd, pid,file_name);
    }

    flock(fd, LOCK_UN);

    return ret;
}

/*
 * It will create the only one pid file for app.
 *
 * @return: 0 is ok, -1 is error.
 */
static int init_pid_file(string file_name)
{
    pid_t pid;
    int fd, ret;

    pid = getpid();

    fd = open(file_name.c_str(), O_RDWR);
    if(fd == -1) {  /* open file fail */
        if(errno == ENOENT) {  /* No such file. Create one for this program. */
            ret = create_pid_file(pid,file_name);
        } else {
            char error_info[256]={0};
            sprintf(error_info,"open %s fail\n",file_name.c_str());
            perror(error_info);
            ret = -1;
        }
    } else {  /* pid file already exists */
        ret = check_pid_file(fd, pid,file_name);
        close(fd);
    }

    return ret;
}
daemon_instance::daemon_instance(string config_path,string mode):m_fd_name("/tmp/daemon_instance")
{
    m_only_clear=mode=="exit";
    auto object=CJsonObject::CreateInstance(config_path);
    if(!object->Get("program_save_path",this->m_fd_name)){
        throw invalid_argument("config with no program_save_path");
    };
    CJsonObject task_list;
    if(!object->Get("task_list",task_list)){
        throw invalid_argument("config with no task_list");
    };
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    auto count = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if(count > 0){
        buffer[count] = '\n';
        buffer[count + 1] = 0;
    }
    else {
        abort();
    }
    for (auto i = count; i >=0; --i)
    {//获取当前路径
        if (buffer[i] == '/')
        {
            buffer[i+1] = '\0';
            break;
        }
    }
    string pwd_path=buffer;
    auto size=task_list.GetArraySize();
    for(int i=0;i<size;i++)
    {
        pro_check_task task;
        CJsonObject json_task=task_list[i];
        if(!json_task.Get("path",task.path))
        {
            throw invalid_argument("config with no path");
        }
        if(!json_task.Get("pro_name",task.pro_name))
        {
            throw invalid_argument("config with no pro_name");
        }
        if(task.pro_name==PORT_CHECK_SHELL){
            task.path=pwd_path;
        }
        if(!json_task.Get("cmd_options",task.cmd_options))
        {
            throw invalid_argument("config with no cmd_options");
        }
        if(!json_task.Get("wait_time",task.wait_time))
        {
            throw invalid_argument("config with no wait_time");
        }
        task.index=i;
        m_task_vec.push_back(task);
        m_tasks.push(task);
    }
}
bool daemon_instance::single_run_test(string file_name)
{
    return init_pid_file(file_name)==0;
}
void daemon_instance::run()
{
    for(auto i:m_task_vec)
    {
        string cmd="killall -9 ";
        cmd+=i.pro_name;
        system(cmd.c_str());
    }
    printf("%d\r\n",m_only_clear);
    if(!m_only_clear){
        thread t([this](){
            this->exit_handle();
        });
        t.detach();
        task_handle();
    }

}
void daemon_instance::task_handle()
{
    unique_lock<mutex>locker(m_mutex);
    while(1)
    {
        if(m_tasks.empty())
        {
            m_conn.wait(locker);
        }
        queue<pro_check_task>tmp;
        while(!m_tasks.empty())
        {
            pro_check_task task=m_tasks.front();
            m_tasks.pop();
            if(task.wait_time==0)
            {
                string cmd=task.path+task.pro_name+" "+task.cmd_options;
                pid_t pid=fork();
                if(pid==0)
                {
                    //printf("%s    run over!\r\n",cmd.c_str());
                    execl("/bin/sh","sh","-c",cmd.c_str(),nullptr);
                    exit(EXIT_SUCCESS);
                }
                else {
                    m_task_map.emplace(pid,task.index);
                }
            }
            else {
                task.wait_time--;
                tmp.push(task);
            }
        }
        while(!tmp.empty()){
            m_tasks.push(tmp.front());
            tmp.pop();
        }
        locker.unlock();
        sleep(1);
        locker.lock();
    }
}
void daemon_instance::exit_handle()
{
    int status;
    pid_t pid;
    unique_lock<mutex>locker(m_mutex);
    while(1)
    {
        if(locker.owns_lock())locker.unlock();
        pid=wait(&status);
        if(pid>0)
        {
            locker.lock();
            auto iter=m_task_map.find(pid);
            if(iter!=m_task_map.end())
            {
                uint32_t task_index=iter->second;
                m_task_map.erase(iter);
                m_tasks.push(m_task_vec[task_index]);
            }
        }
    }
}
void daemon_instance::generate_example_config()
{
    CJsonObject object;
    object.Add("program_save_path",PROGRAM_LOCK_PATH);
    CJsonObject object_task_list;
    CJsonObject task;
    task.Add("path","/");
    task.Add("pro_name","test.sh");
    task.Add("cmd_options","");
    task.Add("wait_time",5);
    object_task_list.Add(task);
    object.Add("task_list",object_task_list);
    FILE *fp=fopen("example_config.json","w+");
    if(fp)
    {
        auto str=object.ToFormattedString();
        fwrite(object.ToFormattedString().c_str(),1,str.size(),fp);
        fclose(fp);
    }
}
void daemon_instance::modify_run_config(const string &path, neb::CJsonObject &new_entry)
{

    string pwd_path=get_pwd_path();
#ifdef DEBUG
    printf("present work path:%s\r\n",pwd_path.c_str());
#endif
    auto object=CJsonObject::CreateInstance(path);
    do{
        //check
        string program_save_path;
        if(!object->Get("program_save_path",program_save_path))break;
        CJsonObject task_list;
        if(!object->Get("task_list",task_list))break;
        struct tmp_opt{
            string path;
            uint32_t wait_time;
        };

        map<pair<string,string>,tmp_opt>m_old_tasks;
        {//init
            auto array_size=task_list.GetArraySize();
            for(decltype (array_size)i=0;i<array_size;++i)
            {
                CJsonObject entry;
                if(!task_list.Get(i,entry))continue;
                string pro_name;
                if(!entry.Get("pro_name",pro_name))continue;
                string options;
                if(!entry.Get("cmd_options",options))continue;
                tmp_opt opt;
                if(!entry.Get("path",opt.path))continue;
                if(!entry.Get("wait_time",opt.wait_time))continue;
                m_old_tasks.emplace(make_pair(pro_name,options),opt);
            }
        }
        {//update
            auto array_size=new_entry.GetArraySize();
            for(decltype (array_size)i=0;i<array_size;++i)
            {
                CJsonObject entry;
                if(!new_entry.Get(i,entry))continue;
                string pro_name;
                if(!entry.Get("pro_name",pro_name))continue;
                string options;
                if(!entry.Get("cmd_options",options))continue;
                tmp_opt opt;
                opt.path=pwd_path;
                if(!entry.Get("wait_time",opt.wait_time))continue;
                auto iter=m_old_tasks.find(make_pair(pro_name,options));
                if(iter!=m_old_tasks.end())m_old_tasks.erase(iter);
                m_old_tasks.emplace(make_pair(pro_name,options),opt);
            }
        }
        {//save
            CJsonObject new_list;
            for(auto i:m_old_tasks)
            {
                CJsonObject entry;
                entry.Add("pro_name",i.first.first);
                entry.Add("cmd_options",i.first.second);
                entry.Add("path",i.second.path);
                entry.Add("wait_time",i.second.wait_time);
                new_list.Add(entry);
            }
            object->Replace("task_list",new_list);
            object->SaveToFile();
        }
        return;
    }while(0);
    {//create new one
        CJsonObject new_config;
        new_config.SetSavePath(path);
        new_config.Add("program_save_path",PROGRAM_LOCK_PATH);
        CJsonObject task_list;
        auto array_size=new_entry.GetArraySize();
        for(decltype (array_size)i=0;i<array_size;++i)
        {
            CJsonObject entry;
            if(!new_entry.Get(i,entry))continue;
            string pro_name;
            if(!entry.Get("pro_name",pro_name))continue;
            string cmd_options;
            if(!entry.Get("cmd_options",cmd_options))continue;
            uint32_t wait_time;
            if(!entry.Get("wait_time",wait_time))continue;
            entry.Add("pro_name",pro_name);
            entry.Add("cmd_options",cmd_options);
            entry.Add("path",pwd_path);
            entry.Add("wait_time",wait_time);
            task_list.Add(entry);
        }
        if(task_list.IsEmpty())
        {
            new_config.AddEmptySubArray("task_list");
        }
        else {
            new_config.Add("task_list",task_list);
        }
        new_config.SaveToFile();
    }
}
 string daemon_instance::get_pwd_path()
{
     char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    auto count = readlink("/proc/self/exe", buffer, sizeof(buffer));
    if(count > 0){
        buffer[count] = '\n';
        buffer[count + 1] = 0;
    }
    else {
        abort();
    }
    for (auto i = count; i >=0; --i)
    {//获取当前路径
        if (buffer[i] == '/')
        {
            buffer[i+1] = '\0';
            break;
        }
    }
    return buffer;
}
