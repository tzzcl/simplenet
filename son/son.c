//文件名: son/son.c
//
//描述: 这个文件实现SON进程
//SON进程首先连接到所有邻居, 然后启动listen_to_neighbor线程, 每个该线程持续接收来自一个邻居的进入报文, 并将该报文转发给SIP进程. 
//然后SON进程等待来自SIP进程的连接. 在与SIP进程建立连接之后, SON进程持续接收来自SIP进程的sendpkt_arg_t结构, 并将接收到的报文发送到重叠网络中.  
//
//创建日期: 2015年

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <sys/utsname.h>
#include <assert.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "son.h"
#include "../topology/topology.h"
#include "neighbortable.h"

//你应该在这个时间段内启动所有重叠网络节点上的SON进程
#define SON_START_DELAY 20

/**************************************************************/
//声明全局变量
/**************************************************************/

//将邻居表声明为一个全局变量 
nbr_entry_t* nt; 
//将与SIP进程之间的TCP连接声明为一个全局变量
int sip_conn; 

/**************************************************************/
//实现重叠网络函数
/**************************************************************/

// 这个线程打开TCP端口CONNECTION_PORT, 等待节点ID比自己大的所有邻居的进入连接,
// 在所有进入连接都建立后, 这个线程终止.
void* waitNbrs(void* arg) {
	int nbrNum = topology_getNbrNum();
	//你需要编写这里的代码.
	int total=0;
	int thisID=topology_getMyNodeID();
	for (int i=0;i<nbrNum;i++)
		if (thisID<nt[i].nodeID) total++;
	int listenfd=0;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t clilen;
	if ((listenfd=socket(AF_INET, SOCK_STREAM,0))<0) 
	{
		puts("create socket error!");
	}
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(CONNECTION_PORT);
	const int on = 1;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	bind(listenfd,(struct sockaddr *)&servaddr, sizeof(servaddr));
	
	listen(listenfd,total);
	printf("Wait For %d Neighbor Init Success!\n",total);	
	while (total>0)
	{
		int conn=accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
		int nodeID=topology_getNodeIDfromip(&cliaddr.sin_addr);
		printf("%d Accept %d connection\n",thisID,nodeID);
		if (nt_addconn(nt,nodeID,conn)<0)
			puts("Add Connection Error!");
		total--;
	}

	return 0;
}

// 这个函数连接到节点ID比自己小的所有邻居.
// 在所有外出连接都建立后, 返回1, 否则返回-1.
int connectNbrs() {
	//你需要编写这里的代码.
	int nbrNum = topology_getNbrNum();
	int thisID=topology_getMyNodeID();
	printf("%d neighbors %d thisID\n",nbrNum,thisID);
	for (int i=0;i<nbrNum;i++)
	if (thisID>nt[i].nodeID)
		{
			int sockfd;
	  		struct sockaddr_in serv_addr;
	  		if ((sockfd = socket(AF_INET,SOCK_STREAM,0))<0) {
	    		perror("socket error");
	    		return -1;
	 		 }
	  		memset(&serv_addr,0,sizeof(serv_addr));
	  		serv_addr.sin_family=AF_INET;
	  		serv_addr.sin_port=htons(CONNECTION_PORT);
	  		memcpy(&serv_addr.sin_addr,&nt[i].nodeIP,sizeof(in_addr_t));
	  		if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0) {
	    		perror("connect error");
	    		return -1;
	  		}
	  		else {
	  			printf("%d node connected to %d node\n",thisID,nt[i].nodeID);
	  			if (nt_addconn(nt,nt[i].nodeID,sockfd)<0)
					puts("Add Connection Error!");
			}
		}
  	return 1;
}

//每个listen_to_neighbor线程持续接收来自一个邻居的报文. 它将接收到的报文转发给SIP进程.
//所有的listen_to_neighbor线程都是在到邻居的TCP连接全部建立之后启动的.
void* listen_to_neighbor(void* arg) {
	//你需要编写这里的代码.
	int now=(*(int*)arg);
	sip_pkt_t* data=malloc(sizeof(sip_pkt_t));
	memset(data,0,sizeof(sip_pkt_t));
	while (1){
		if (recvpkt(data,nt[now].conn)<0) 
		{
			close(nt[now].conn);
			//forwardpktToSIP(build_failpkt(nt[now].nodeID),sip_conn);
			break;
		}
		forwardpktToSIP(data,sip_conn);
	}
	free(data);
}

//这个函数打开TCP端口SON_PORT, 等待来自本地SIP进程的进入连接. 
//在本地SIP进程连接之后, 这个函数持续接收来自SIP进程的sendpkt_arg_t结构, 并将报文发送到重叠网络中的下一跳. 
//如果下一跳的节点ID为BROADCAST_NODEID, 报文应发送到所有邻居节点.
void waitSIP() {
	//你需要编写这里的代码.
	int nbrNum = topology_getNbrNum();
	int listenfd=0;
	struct sockaddr_in servaddr,cliaddr;
	socklen_t clilen;
	if ((listenfd=socket(AF_INET, SOCK_STREAM,0))<0) 
	{
		puts("create socket error!");
	}
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(SON_PORT);
	const int on = 1;
	setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));
	bind(listenfd,(struct sockaddr *)&servaddr, sizeof(servaddr));
	puts("SON Wait For SIP Server Init Success!");	
	listen(listenfd,1);
	while(1){
	sip_conn=accept(listenfd,(struct sockaddr *)&cliaddr,&clilen);
	puts("Accept SIP Process!");
	int nextNode=0;
	sip_pkt_t* recv_seg=malloc(sizeof(sip_pkt_t));
	memset(recv_seg,0,sizeof(sip_pkt_t));
	while (1){
		if (getpktToSend(recv_seg,&nextNode,sip_conn)!=1)
			continue;
		if (nextNode==BROADCAST_NODEID){
			//char *flags=malloc(nbrNum);
			//memset(flags,0,nbrNum);
			for (int i=0;i<nbrNum;i++)
			{
				if (sendpkt(recv_seg,nt[i].conn)==-1) {
					//flags[i]=1;
				}
			}/*
			for (int i=0;i<nbrNum;i++) {
				if (flags[i]==1) {
					forwardpktToSIP(build_failpkt(nt[i].nodeID),sip_conn);
				}
			}
			free(flags);*/
		}
		else{
			for (int i=0;i<nbrNum;i++)
			{
				if (nt[i].nodeID==nextNode)
				{
					if (sendpkt(recv_seg,nt[i].conn)==-1) {
						//forwardpktToSIP(build_failpkt(nt[i].nodeID),sip_conn);
					}
					break;
				}
			}
		}
	}
	printf("####SIP stop!! listening for next SIP\n");
	}
}

//这个函数停止重叠网络, 当接收到信号SIGINT时, 该函数被调用.
//它关闭所有的连接, 释放所有动态分配的内存.
void son_stop() {
	//你需要编写这里的代码.
	nt_destroy(nt);
	close(sip_conn);
	exit(0);
}

int main() {
	//启动重叠网络初始化工作
	printf("Overlay network: Node %d initializing...\n",topology_getMyNodeID());	

	//创建一个邻居表
	nt = nt_create();
	//将sip_conn初始化为-1, 即还未与SIP进程连接.
	sip_conn = -1;
	
	//注册一个信号句柄, 用于终止进程
	signal(SIGINT, son_stop);

	//打印所有邻居
	int nbrNum = topology_getNbrNum();
	int i;
	for(i=0;i<nbrNum;i++) {
		printf("Overlay network: neighbor %d:%d\n",i+1,nt[i].nodeID);
	}

	//启动waitNbrs线程, 等待节点ID比自己大的所有邻居的进入连接
	pthread_t waitNbrs_thread;
	pthread_create(&waitNbrs_thread,NULL,waitNbrs,(void*)0);

	//等待其他节点启动
	sleep(SON_START_DELAY);
	
	//连接到节点ID比自己小的所有邻居
	connectNbrs();

	//等待waitNbrs线程返回
	pthread_join(waitNbrs_thread,NULL);	

	//此时, 所有与邻居之间的连接都建立好了
	
	//创建线程监听所有邻居
	for(i=0;i<nbrNum;i++) {
		int* idx = (int*)malloc(sizeof(int));
		*idx = i;
		pthread_t nbr_listen_thread;
		pthread_create(&nbr_listen_thread,NULL,listen_to_neighbor,(void*)idx);
	}
	printf("Overlay network: node initialized...\n");
	printf("Overlay network: waiting for connection from SIP process...\n");

	//等待来自SIP进程的连接
	waitSIP();
}
