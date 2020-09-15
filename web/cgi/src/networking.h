#ifndef _NETWORKING_H
#define _NETWORKING_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>

/*消息队列固定数据结构*/
typedef struct WEB_MSG{
    long type;//消息类型
    char buf[1024];//消息数据
}WEB_MSG;

/*用于接收设备发来的消息*/
typedef struct DEV_MSG{
    long type;//消息类型
    char buf[1024*32];//消息数据
}DEV_MSG;

typedef struct PipParam
{
	int dev_id;
	int ipc_id;
	int pos_x;
	int pos_y;
	int pos_w;
	int pos_h;
	int FW;
	int FH;
}PipParam;

/**********************与WEB端通讯时的消息****************************/
typedef enum WEB_MSGTYPE{W_PIPWIDGETOPEN = 1/*开窗*/, W_PIPWIDGETCLOSE = 2/*关窗*/,W_PIPWIDGETALLCLOSE,/*窗口全部关闭*/
			W_PIPWIDGETCHANGE/*开窗移动，大小改变*/,W_PIPWIDGETINFO/*获取开窗信息*/,
            W_ADDMESSAGERELEASE/*添加信息发布卡信息*/,W_DELETEMESSAGERELEASE/*删除信息发布卡信息*/,
            W_SETWINMODE/*设置一个屏开窗个数*/,W_GETWINMODE/*获取一个屏开窗个数*/,
            W_GETBOARDTYPE/*获取板卡型号*/,W_NETINFO/*获取板卡网络信息*/,W_DATASYNC/*数据同步*/,W_WALLSET/*获取幕墙信息*/,W_JOINTSET/*幕墙设置*/,
			W_SETDEVINFO/*设置板卡网络、视频输出信息*/,W_EXTERHDMION/*打开外部源*/, W_EXTERHDMIOFF/*关闭外部源*/, W_EXTERHDMICONFIG/*获取外部源信息*/,
			W_SCENERECORD/*场景记录*/, W_SCENEDEL/*场景删除*/, W_SCENELOOP/*场景轮巡*/, W_SCENELOOPSTOP/*轮巡停止*/,
            W_SCENECALL/*调入场景*/, W_SCENEINFO/*获取取场景信息*/,W_SCENESTATE/*获取当前的轮巡场景*/,
            W_PTZDIRECTION/*云台方向控制*/, W_PTZZOOM/*云台变倍*/,W_PTZSTOP/*云台停止*/,W_PTZSETPRE/*设置预置点*/,
            W_PTZTOPRE/*调预置点*/, W_PTZTRACING/*球机开始寻迹*/,W_PTZUNTRACING/*球机停止寻迹*/,
            W_SEARCHIPC/*搜索ipc*/,  W_ADDIPC/*添加ipc*/,  W_DELIPC/*删除ipc*/,W_EDITIPC/*编辑ipc*/,  W_CLEARIPC/*清空ipc*/,
        }WEB_MSGTYPE;


int GetBoardMsg_func(int msqid,WEB_MSGTYPE msg_type);

#endif
