#include "upnpmapper.h"
#include <iostream>
#include<queue>
using namespace std;
using namespace micagent;
void print_help(){
    cout<<"add udp/tcp <internal_ip> <internal_port> <external_port> <description><alive_time>\r\n";
    cout<<"get_port udp/tcp  <external_port> \r\n";
    cout<<"get_ip \r\n";
    cout<<"delete udp/tcp  <external_port> \r\n";
    cout<<"help to show this page \r\n";
    cout<<"exit to exit this program \r\n";
}
queue<string>spilt_by_space(const string&buf){
    int search_begin=0;
    queue<string>ret;
    while(search_begin<buf.size()){
        auto pos=buf.find_first_of(' ',search_begin);
        if(pos==string::npos){
            ret.push(buf.substr(search_begin));
            break;
        }
        ret.push(buf.substr(search_begin,pos-search_begin));
        search_begin=buf.find_first_not_of(' ',pos+1);
        if(search_begin==string::npos){
            break;
        }
    }
    return ret;
}
int main(int argc,char *argv[])
{
    if(argc<2)exit(-1);
    EventLoop eventloop(0,1);
    UpnpMapper::Instance().Init(&eventloop,argv[1]);
    print_help();
    while(1){
        char cmd[1024]={0};
        cin.getline(cmd,1024);
        auto cmd_queue=spilt_by_space(cmd);
        if(cmd_queue.empty())continue;
        string cmd_str=cmd_queue.front();
        cmd_queue.pop();
        if(cmd_str=="exit")break;
        else if (cmd_str=="help") {
            print_help();
        }
        else if(cmd_str=="delete"){
            if(cmd_queue.size()<2)cout<<"false delete command:"<<cmd<<endl;
            else {
                string type=cmd_queue.front();
                cmd_queue.pop();
                string port=cmd_queue.front();
                UpnpMapper::Instance().Api_deleteportMapper(type=="udp"?UDP:TCP,stoi(port),[type,port](bool ret){
                    if(ret){
                        cout<<"delete "<<type<<" "<<port<<" success!\r\n";
                    }
                    else {
                        cout<<"delete "<<type<<" "<<port<<" fail!\r\n";
                    }
                });
            }
        }
        else if (cmd_str=="get_ip") {
            UpnpMapper::Instance().Api_GetNewexternalIP([](bool ret){
                if(ret){
                    cout<<"external ip is "<<UpnpMapper::Instance().APi_getexternalIP()<<endl;
                }
                else {
                    cout<<"get external ip fail!\r\n";
                }
            });
        }
        else if (cmd_str=="get_port") {
            if(cmd_queue.size()<2)cout<<"false get_port command:"<<cmd<<endl;
            else {
                string type=cmd_queue.front();
                cmd_queue.pop();
                string port=cmd_queue.front();
                UpnpMapper::Instance().Api_GetSpecificPortMappingEntry(type=="udp"?UDP:TCP,stoi(port),[type,port](bool ret){
                    if(ret){
                        cout<<"get_port "<<type<<" "<<port<<" success!\r\n";
                    }
                    else {
                        cout<<"get_port "<<type<<" "<<port<<" fail!\r\n";
                    }
                });
            }
        }
        else if (cmd_str=="add") {
            if(cmd_queue.size()<5)cout<<"false add command:"<<cmd<<endl;
            else {
                string type=cmd_queue.front();
                cmd_queue.pop();
                string internal_ip=cmd_queue.front();
                cmd_queue.pop();
                string internal_port=cmd_queue.front();
                cmd_queue.pop();
                string external_port=cmd_queue.front();
                cmd_queue.pop();
                string description=cmd_queue.front();
                cmd_queue.pop();
                string alive_time="0";
                if(!cmd_queue.empty())alive_time=cmd_queue.front();
                UpnpMapper::Instance().Api_addportMapper(type=="udp"?UDP:TCP,internal_ip,stoi(internal_port),\
                stoi(external_port),description,[type,internal_ip,internal_port,external_port,description](bool ret){
                    if(ret){
                        printf("add %s %s %s %s %s success!\r\n",type.c_str(),internal_ip.c_str(),internal_port.c_str(),external_port.c_str(),description.c_str());
                    }
                    else {
                        printf("add %s %s %s %s %s fail!\r\n",type.c_str(),internal_ip.c_str(),internal_port.c_str(),external_port.c_str(),description.c_str());
                    }
                },stoi(alive_time));
            }
        }
    }
    eventloop.stop();
    return 0;
}
