#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <termios.h>
#include <linux/route.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <linux/reboot.h>
#include <sys/reboot.h>

#include "networking.h"



/*********************************************************
* 获取板卡发来的信息，并发送到串口(web) msj 2019.10.18
*********************************************************/
int GetBoardMsg_func(int msqid,WEB_MSGTYPE msg_type)
{
	DEV_MSG dev_msg;
	int i = 0;
	int res = 0;
	/*先阻塞等待板卡的第一个消息*/
	for(i = 0;i<10;i++)
	{
		usleep(1000*10);
		res = msgrcv(msqid,&dev_msg,sizeof(dev_msg.buf),(long)msg_type,IPC_NOWAIT);//0
		if(res==-1){
			continue;
		}
		else{
			break;
		}
		if(i == 9){
			printf("error:time out!");
			return -1;
		}
	}
	printf("%s",dev_msg.buf);
	memset(dev_msg.buf,0,sizeof(dev_msg.buf));
	usleep(1000);
#if 1
	/*再非阻塞接收板卡的剩余消息，若消息为空则直接跳出*/
	while(1)
	{
		res = msgrcv(msqid,&dev_msg,sizeof(dev_msg.buf),(long)msg_type,IPC_NOWAIT);//msg_type,IPC_NOWAIT
		if(res == -1){
			break;
		}
		printf("%s",dev_msg.buf);
		memset(dev_msg.buf,0,sizeof(dev_msg.buf));
		usleep(1000);
	}
#endif
	return 0;
}
