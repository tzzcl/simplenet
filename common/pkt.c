// 文件名 pkt.c
// 创建日期: 2015年

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <memory.h>
#include <string.h>
#include "pkt.h"

// son_sendpkt()由SIP进程调用, 其作用是要求SON进程将报文发送到重叠网络中. SON进程和SIP进程通过一个本地TCP连接互连.
// 在son_sendpkt()中, 报文及其下一跳的节点ID被封装进数据结构sendpkt_arg_t, 并通过TCP连接发送给SON进程. 
// 参数son_conn是SIP进程和SON进程之间的TCP连接套接字描述符.
// 当通过SIP进程和SON进程之间的TCP连接发送数据结构sendpkt_arg_t时, 使用'!&'和'!#'作为分隔符, 按照'!& sendpkt_arg_t结构 !#'的顺序发送.
// 如果发送成功, 返回1, 否则返回-1.
int son_sendpkt(int nextNodeID, sip_pkt_t* pkt, int son_conn)
{
	sendpkt_arg_t* send_seg=malloc(sizeof(sendpkt_arg_t));
	send_seg->nextNodeID=nextNodeID;
	memcpy(&send_seg->pkt,pkt,sizeof(sip_pkt_t));
	printf("Send %d\n",son_conn);
	print_pktseg(pkt);
	if (send(son_conn,"!&",2,0)<=0) goto bad;
	int nowlength=pkt->header.length;
	printf("Send son_sendpkt %d\n",4+PKT_HEADER_LENGTH+nowlength);
	if (send(son_conn,send_seg,4+PKT_HEADER_LENGTH+nowlength,0)<=0) goto bad;
	if (send(son_conn,"!#",2,0)<=0) goto bad;
        	printf("son_sendpkt: succeeded\n");
	free(send_seg);
	return 1;
 bad:
       	printf("son_sendpkt: failed\n");
	free(send_seg);
	return -1;
}

// son_recvpkt()函数由SIP进程调用, 其作用是接收来自SON进程的报文. 
// 参数son_conn是SIP进程和SON进程之间TCP连接的套接字描述符. 报文通过SIP进程和SON进程之间的TCP连接发送, 使用分隔符!&和!#. 
// 为了接收报文, 这个函数使用一个简单的有限状态机FSM
// PKTSTART1 -- 起点 
// PKTSTART2 -- 接收到'!', 期待'&' 
// PKTRECV -- 接收到'&', 开始接收数据
// PKTSTOP1 -- 接收到'!', 期待'#'以结束数据的接收 
// 如果成功接收报文, 返回1, 否则返回-1.
int recv_pktbyte(int connection,char* ch){
	char buf[2];memset(buf,0,sizeof(buf));
	int n=recv(connection,buf,1,0);
	if (n<=0) {
		//perror("Receive Error!");
		return -1;
	}
	ch[0]=buf[0];
	return 0;
}
int son_recvpkt(sip_pkt_t* pkt, int son_conn)
{
	int state=0;
  	char* head=0;
  	while (1){
		char now;
		int recv_state=recv_pktbyte(son_conn,&now);
		if (recv_state<0) return -1;
		switch (state){
			case 0:{
			if (now=='!') state=1;
			break;
		  	}
			case 1:{
			if (now=='&') {
				head=(char*)pkt;state=2;
			}
			else perror("head error!");
			break;
		  	}
			case 2:{
			if (now!='!') {
				head[0]=now;head++;
			}
			else state=3;
			break;
		  	}
			case 3:{
			if (now!='#') {head[0]='!';head++;head[0]=now;head++;state=2;}
			else state=4;
			break;
		  	}
		}
		if (state==4){
		puts("son_recvpkt:");
		print_pktseg(pkt);
		break;	
		}
  	}
  return 1;
}

// 这个函数由SON进程调用, 其作用是接收数据结构sendpkt_arg_t.
// 报文和下一跳的节点ID被封装进sendpkt_arg_t结构.
// 参数sip_conn是在SIP进程和SON进程之间的TCP连接的套接字描述符. 
// sendpkt_arg_t结构通过SIP进程和SON进程之间的TCP连接发送, 使用分隔符!&和!#. 
// 为了接收报文, 这个函数使用一个简单的有限状态机FSM
// PKTSTART1 -- 起点 
// PKTSTART2 -- 接收到'!', 期待'&' 
// PKTRECV -- 接收到'&', 开始接收数据
// PKTSTOP1 -- 接收到'!', 期待'#'以结束数据的接收
// 如果成功接收sendpkt_arg_t结构, 返回1, 否则返回-1.
int getpktToSend(sip_pkt_t* pkt, int* nextNode,int sip_conn)
{
	int state=0;
	sendpkt_arg_t* temp=malloc(sizeof(sendpkt_arg_t));
	memset(temp,0,sizeof(sendpkt_arg_t));
	char* head=0;
	while (1){
		char now;
		int recv_state=recv_pktbyte(sip_conn,&now);
		if (recv_state<0) return -1;
		switch (state){
			case 0:{
			if (now=='!') state=1;
			break;
		  	}
			case 1:{
			if (now=='&') {
				head=(char*)temp;state=2;
			}
			else perror("head error!");
			break;
		  	}
			case 2:{
			if (now!='!') {
				head[0]=now;head++;
			}
			else state=3;
			break;
		  	}
			case 3:{
			if (now!='#') {head[0]='!';head++;head[0]=now;head++;state=2;}
			else state=4;
			break;
		  	}
		}
		if (state==4){
		puts("Receive SendPkt:");
		printf("nextNodeID is %d\n",temp->nextNodeID);
		*nextNode=temp->nextNodeID;
		int nowlength=temp->pkt.header.length;
		print_pktseg(&(temp->pkt));
		memcpy(pkt,&(temp->pkt),PKT_HEADER_LENGTH+nowlength);
		
		break;	
		}
  	}
  	free(temp);
	return 1;
}

// forwardpktToSIP()函数是在SON进程接收到来自重叠网络中其邻居的报文后被调用的. 
// SON进程调用这个函数将报文转发给SIP进程. 
// 参数sip_conn是SIP进程和SON进程之间的TCP连接的套接字描述符. 
// 报文通过SIP进程和SON进程之间的TCP连接发送, 使用分隔符!&和!#, 按照'!& 报文 !#'的顺序发送. 
// 如果报文发送成功, 返回1, 否则返回-1.
int forwardpktToSIP(sip_pkt_t* pkt, int sip_conn)
{
	sip_pkt_t* send_seg=malloc(sizeof(sip_pkt_t));
	memcpy(send_seg,pkt,sizeof(sip_pkt_t));
	printf("Send %d\n",sip_conn);
	if (send(sip_conn,"!&",2,0)<=0) goto bad;
	int nowlength=pkt->header.length;
	if (send(sip_conn,send_seg,PKT_HEADER_LENGTH+nowlength,0)<=0) goto bad;
	if (send(sip_conn,"!#",2,0)<=0) goto bad;
        	printf("forwardpktToSIP: succeeded\n");
	free(send_seg);
	return 1;
 bad:
       	printf("forwardpktToSIP: failed\n");
	free(send_seg);
	return -1;
}

// sendpkt()函数由SON进程调用, 其作用是将接收自SIP进程的报文发送给下一跳.
// 参数conn是到下一跳节点的TCP连接的套接字描述符.
// 报文通过SON进程和其邻居节点之间的TCP连接发送, 使用分隔符!&和!#, 按照'!& 报文 !#'的顺序发送. 
// 如果报文发送成功, 返回1, 否则返回-1.
int sendpkt(sip_pkt_t* pkt, int conn)
{
	sip_pkt_t* send_seg=malloc(sizeof(sip_pkt_t));
	memcpy(send_seg,pkt,sizeof(sip_pkt_t));
	printf("Send %d\n",conn);
	if (send(conn,"!&",2,0)<=0) goto bad;
	int nowlength=pkt->header.length;
	if (send(conn,send_seg,PKT_HEADER_LENGTH+nowlength,0)<=0) goto bad;
	if (send(conn,"!#",2,0)<=0) goto bad;
        	printf("sendpkt: succeeded\n");
	free(send_seg);
	return 1;
 bad:
       	printf("sendpkt: failed\n");
	free(send_seg);
	return -1;
}

// recvpkt()函数由SON进程调用, 其作用是接收来自重叠网络中其邻居的报文.
// 参数conn是到其邻居的TCP连接的套接字描述符.
// 报文通过SON进程和其邻居之间的TCP连接发送, 使用分隔符!&和!#. 
// 为了接收报文, 这个函数使用一个简单的有限状态机FSM
// PKTSTART1 -- 起点 
// PKTSTART2 -- 接收到'!', 期待'&' 
// PKTRECV -- 接收到'&', 开始接收数据
// PKTSTOP1 -- 接收到'!', 期待'#'以结束数据的接收 
// 如果成功接收报文, 返回1, 否则返回-1.
int recvpkt(sip_pkt_t* pkt, int conn)
{
  int state=0;
  	char* head=0;
  	while (1){
		char now;
		int recv_state=recv_pktbyte(conn,&now);
		if (recv_state<0) return -1;
		switch (state){
			case 0:{
			if (now=='!') state=1;
			break;
		  	}
			case 1:{
			if (now=='&') {
				head=(char*)pkt;state=2;
			}
			else perror("head error!");
			break;
		  	}
			case 2:{
			if (now!='!') {
				head[0]=now;head++;
			}
			else state=3;
			break;
		  	}
			case 3:{
			if (now!='#') {head[0]='!';head++;head[0]=now;head++;state=2;}
			else state=4;
			break;
		  	}
		}
		if (state==4){
		puts("recvpkt:");
		print_pktseg(pkt);
		break;	
		}
  	}
  return 1;
}
void print_pktseg(sip_pkt_t* seg){
	sip_hdr_t * header=&(seg->header);
	printf("-----------------------------------------------------\n"
               "|    src_nodeID: ""%5d    |    dest_nodeID: ""%5d   |\n"
               "|       LENGTH:""%5d      |         TYPE:%3d        |\n"
               "-----------------------------------------------------\n"
               ,header->src_nodeID,header->dest_nodeID,header->length,header->type);
}
