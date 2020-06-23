#include <string>
#include<iostream>
#include<map>
#include "priority_queue.h"
using namespace std;
using namespace micagent;
int main(int argc,char *argv[])
{
    priority_queue<int>test(5,10000,3);
    random_device rd;
    int items=rd()%75+50;
    for(int i=0;i<items;i++){
        int val=rd()%100000;
        uint8_t priority=rd()%6;
        cout<<"try insert  priority:"<<(int)priority<<" val:"<<val<<endl;
        if(test.push(priority,val)){
            cout<<"success!"<<endl;
        }
        else {
            cout<<"failed!"<<endl;
        }
    }
    int save;
    while(test.pop(save));
    return 0;
}

