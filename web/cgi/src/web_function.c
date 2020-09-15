#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#include "networking.h"

void Get_Mode_Handle();
void Post_Mode_Handle();
void Init_Send_MsgList();
void Init_Recv_MsgList();
void Init_Sem_singnel();
static WEB_MSG s_MSG;
static key_t s_key;	//用于发送
static int s_msqid;	//用于发送
static key_t r_key;	//用于接收
static int r_msqid;	//用于接收
static key_t sem_key;
static int semid;

int main(int argc,char **argv)
{
	char *request_method = NULL;
	printf("Content-type:text/html;charset=utf-8\n\n");	//response header

	if(!getenv("REQUEST_METHOD"))
	{
		printf("error:getenv false");
		return -1;
	}
	Init_Send_MsgList();
	Init_Recv_MsgList();
	Init_Sem_singnel();
	request_method = getenv("REQUEST_METHOD");

	if(!strcmp(request_method,"GET"))
	{        
		Get_Mode_Handle();
	}
	else if(!strcmp(request_method,"POST"))
	{
		Post_Mode_Handle();
	}
	else{
		printf("error:request_method false");		
	}
	
	fflush(stdout);
	return 0;	
}

void Init_Send_MsgList()
{
#if 1
    //1.获取key
    s_key = ftok("/home/microcreat/log",66);
    if(s_key==-1){
        perror("ftok");
        exit(-1);
    }

    //2.通过key获取消息队列
    s_msqid = msgget(s_key,0);
    if(s_msqid==-1){
        perror("msgget");
        exit(-2);
    }
#endif
}

void Init_Recv_MsgList()
{
#if 1
    //1.获取key
    r_key = ftok("/home/microcreat/log",77);
    if(r_key==-1){
        perror("ftok");
        exit(-1);
    }

    //2.通过key获取消息队列
    r_msqid = msgget(r_key,0);
    if(r_msqid==-1){
        perror("msgget");
        exit(-2);
    }
#endif
}

void Init_Sem_singnel()
{
	    //1.获取key
    sem_key = ftok("/home/microcreat/mvr/web",'a');
    if(sem_key==-1){
        perror("sem ftok");
        exit(-1);
    }

    //2.通过key获取信号量集
    semid = semget(sem_key,0,0);
    if(semid==-1){
        perror("semget");
        exit(-2);
    }
}

void Get_Mode_Handle()
{
	char *UserInput = NULL;
	int data_len;
	UserInput = getenv("QUERY_STRING");
	data_len = strlen(UserInput);
	if(data_len == 0||UserInput == NULL)
	{
		printf("error:GET no data");
		return;
	}
	//FILE* fd = fopen("msg.txt","a+");
	//fwrite(UserInput,data_len,1,fd);
	//fclose(fd);

	printf("receive-msg(%s)aa",UserInput);    

    //printf("success(test)");

    return;
}

void Post_Mode_Handle()
{
    int ByteLength;//
    ByteLength = atoi(getenv("CONTENT_LENGTH"));

    char *word;

    word = (char *)malloc(ByteLength+1);

    fread(word,ByteLength+1,ByteLength+1,stdin);
    word[ByteLength]=0;

    //printf("post:%s",word);
    struct sembuf sem_buf;
    sem_buf.sem_num = 0;//下标0
    sem_buf.sem_op = -1;//P操作
    sem_buf.sem_flg = 0;//阻塞
    semop(semid,&sem_buf,1);//首元素的地址就是数组首地址
    
	if(strstr(word,"cmd : PipWidgetOpen")!=NULL) //开窗
	{
        s_MSG.type = (long)W_PIPWIDGETOPEN;
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

	}
    else if(strstr(word,"cmd : PipWidgetClose")!=NULL)    //关窗
    {
        s_MSG.type = (long)(W_PIPWIDGETCLOSE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

    }
	else if(strstr(word,"cmd : PipWidgetAllClose")!=NULL)    //关闭所有窗
    {
        s_MSG.type = (long)(W_PIPWIDGETALLCLOSE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

    }
    else if(strstr(word,"cmd : PipWidgetChange")!=NULL)   //开窗移动，大小改变，显示最上层
    {
        s_MSG.type = (long)(W_PIPWIDGETCHANGE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

    }
    else if(strstr(word,"cmd : PipWidgetInfo")!=NULL)   //获取开窗信息
    {
        s_MSG.type = (long)(W_PIPWIDGETINFO);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
		
		GetBoardMsg_func(r_msqid,W_PIPWIDGETINFO);
    }
	else if(strstr(word,"cmd : AddMessageRelease")!=NULL)   //添加信息发布卡信息
    {
        s_MSG.type = (long)(W_ADDMESSAGERELEASE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
		
		GetBoardMsg_func(r_msqid,W_ADDMESSAGERELEASE);
    }
	else if(strstr(word,"cmd : DeleteMessageRelease")!=NULL)   //删除信息发布卡信息
    {
        s_MSG.type = (long)(W_DELETEMESSAGERELEASE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : SetWinMode")!=NULL)   //设置一个屏开窗个数
    {
        s_MSG.type = (long)(W_SETWINMODE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : GetWinMode")!=NULL)   //获取一个屏开窗个数
    {
        s_MSG.type = (long)(W_GETWINMODE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
		//printf("i am coming GetWinMode\r\n");
		GetBoardMsg_func(r_msqid,W_GETWINMODE);
    }
	else if(strstr(word,"cmd : GetBoardType")!=NULL)   //获取板卡型号
    {
        s_MSG.type = (long)(W_GETBOARDTYPE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
		//printf("i am coming GetBoardType\r\n");
		GetBoardMsg_func(r_msqid,W_GETBOARDTYPE);
    }
	else if(strstr(word,"cmd : NetInfo")!=NULL)   //获取板卡网络信息
    {
        s_MSG.type = (long)(W_NETINFO);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

		GetBoardMsg_func(r_msqid,W_NETINFO);
    }
	else if(strstr(word,"cmd : DataSync")!=NULL)   //数据同步
    {
        s_MSG.type = (long)(W_DATASYNC);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
		//printf("i am coming:%s\r\n",s_MSG.buf);
		GetBoardMsg_func(r_msqid,W_DATASYNC);
    }
	else if(strstr(word,"cmd : WallSet")!=NULL)   //获取幕墙信息
    {
        s_MSG.type = (long)(W_WALLSET);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
		//printf("i am coming WallSet\r\n");
		GetBoardMsg_func(r_msqid,W_WALLSET);
    }
	else if(strstr(word,"cmd : JointSet")!=NULL)   //设置幕墙
    {
        s_MSG.type = (long)(W_JOINTSET);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : SetDevInfo")!=NULL)   //设置板卡网络、视频输出信息
    {
        s_MSG.type = (long)(W_SETDEVINFO);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : ExterHdmiOn")!=NULL)   //打开外部源
    {
        s_MSG.type = (long)(W_EXTERHDMION);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : ExterHdmiOff")!=NULL)   //关闭外部源
    {
        s_MSG.type = (long)(W_EXTERHDMIOFF);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : ExternHdmiConfig")!=NULL)   //获取外部源信息
    {
        s_MSG.type = (long)(W_EXTERHDMICONFIG);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
		GetBoardMsg_func(r_msqid,W_EXTERHDMICONFIG);
    }
	else if(strstr(word,"cmd : SceneRecord")!=NULL)   //场景记录
    {
        s_MSG.type = (long)(W_SCENERECORD);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : SceneDel")!=NULL)   //场景删除
    {
        s_MSG.type = (long)(W_SCENEDEL);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : SceneLoop")!=NULL)   //场景轮巡
    {
        s_MSG.type = (long)(W_SCENELOOP);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : SceneLoopStop")!=NULL)   //轮巡停止
    {
        s_MSG.type = (long)(W_SCENELOOPSTOP);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
	else if(strstr(word,"cmd : SceneCall")!=NULL)   //调入场景
    {
        s_MSG.type = (long)(W_SCENECALL);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

    }
	else if(strstr(word,"cmd : SceneInfo")!=NULL)   //获取取场景信息
    {
        s_MSG.type = (long)(W_SCENEINFO);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_SCENEINFO);
    }
	else if(strstr(word,"cmd : SceneState")!=NULL)   //获取当前的轮巡场景
    {
        s_MSG.type = (long)(W_SCENESTATE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_SCENESTATE);
    }
    else if(strstr(word,"cmd : PtzDirection")!=NULL)   //云台方向控制
    {
        s_MSG.type = (long)(W_PTZDIRECTION);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_PTZDIRECTION);
    }
    else if(strstr(word,"cmd : PtzZoom")!=NULL)   //云台变倍
    {
        s_MSG.type = (long)(W_PTZZOOM);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_PTZZOOM);
    }
    else if(strstr(word,"cmd : PtzStop")!=NULL)   //云台停止
    {
        s_MSG.type = (long)(W_PTZSTOP);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_PTZSTOP);
    }
    else if(strstr(word,"cmd : PtzSetPre")!=NULL)   //设置预置点
    {
        s_MSG.type = (long)(W_PTZSETPRE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_PTZSETPRE);
    }
    else if(strstr(word,"cmd : PtzToPre")!=NULL)   //调预置点
    {
        s_MSG.type = (long)(W_PTZTOPRE);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_PTZTOPRE);
    }
    else if(strstr(word,"cmd : PtzTracing")!=NULL)   //球机开始寻迹
    {
        s_MSG.type = (long)(W_PTZTRACING);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_PTZTRACING);
    }
    else if(strstr(word,"cmd : PtzUnTracing")!=NULL)   //球机停止寻迹
    {
        s_MSG.type = (long)(W_PTZUNTRACING);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_PTZUNTRACING);
    }
    else if(strstr(word,"cmd : SearchIpc")!=NULL)   //搜索ipc
    {
        s_MSG.type = (long)(W_SEARCHIPC);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
        GetBoardMsg_func(r_msqid,W_SEARCHIPC);
    }
    else if(strstr(word,"cmd : AddIpc")!=NULL)   //添加ipc
    {
        s_MSG.type = (long)(W_ADDIPC);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

    }
    else if(strstr(word,"cmd : DelIpc")!=NULL)   //删除ipc
    {
        s_MSG.type = (long)(W_DELIPC);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }
    }
    else if(strstr(word,"cmd : EditIpc")!=NULL)   //编辑ipc
    {
        s_MSG.type = (long)(W_EDITIPC);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

    }
    else if(strstr(word,"cmd : ClearIpc")!=NULL)   //清空ipc
    {
        s_MSG.type = (long)(W_CLEARIPC);
        strcpy(s_MSG.buf,word);
        int ret = msgsnd(s_msqid,&s_MSG,sizeof(s_MSG.buf),IPC_NOWAIT); //0:阻塞 IPC_NOWAIT:表示不阻塞(队列满了立即返回错误)
        if(ret == -1){
                printf("error:send msg error");
				return;
        }

    }
    else
    {
    	printf("error not the msg type:%s",word);
    }
    sem_buf.sem_op = 1;//V操作
    semop(semid,&sem_buf,1);//首元素的地址就是数组首地址

    free(word);
    return;
}


