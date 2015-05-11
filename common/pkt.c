// �ļ��� pkt.c
// ��������: 2015��

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <memory.h>
#include <string.h>
#include "pkt.h"

// son_sendpkt()��SIP���̵���, ��������Ҫ��SON���̽����ķ��͵��ص�������. SON���̺�SIP����ͨ��һ������TCP���ӻ���.
// ��son_sendpkt()��, ���ļ�����һ���Ľڵ�ID����װ�����ݽṹsendpkt_arg_t, ��ͨ��TCP���ӷ��͸�SON����. 
// ����son_conn��SIP���̺�SON����֮���TCP�����׽���������.
// ��ͨ��SIP���̺�SON����֮���TCP���ӷ������ݽṹsendpkt_arg_tʱ, ʹ��'!&'��'!#'��Ϊ�ָ���, ����'!& sendpkt_arg_t�ṹ !#'��˳����.
// ������ͳɹ�, ����1, ���򷵻�-1.
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

// son_recvpkt()������SIP���̵���, �������ǽ�������SON���̵ı���. 
// ����son_conn��SIP���̺�SON����֮��TCP���ӵ��׽���������. ����ͨ��SIP���̺�SON����֮���TCP���ӷ���, ʹ�÷ָ���!&��!#. 
// Ϊ�˽��ձ���, �������ʹ��һ���򵥵�����״̬��FSM
// PKTSTART1 -- ��� 
// PKTSTART2 -- ���յ�'!', �ڴ�'&' 
// PKTRECV -- ���յ�'&', ��ʼ��������
// PKTSTOP1 -- ���յ�'!', �ڴ�'#'�Խ������ݵĽ��� 
// ����ɹ����ձ���, ����1, ���򷵻�-1.
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

// ���������SON���̵���, �������ǽ������ݽṹsendpkt_arg_t.
// ���ĺ���һ���Ľڵ�ID����װ��sendpkt_arg_t�ṹ.
// ����sip_conn����SIP���̺�SON����֮���TCP���ӵ��׽���������. 
// sendpkt_arg_t�ṹͨ��SIP���̺�SON����֮���TCP���ӷ���, ʹ�÷ָ���!&��!#. 
// Ϊ�˽��ձ���, �������ʹ��һ���򵥵�����״̬��FSM
// PKTSTART1 -- ��� 
// PKTSTART2 -- ���յ�'!', �ڴ�'&' 
// PKTRECV -- ���յ�'&', ��ʼ��������
// PKTSTOP1 -- ���յ�'!', �ڴ�'#'�Խ������ݵĽ���
// ����ɹ�����sendpkt_arg_t�ṹ, ����1, ���򷵻�-1.
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

// forwardpktToSIP()��������SON���̽��յ������ص����������ھӵı��ĺ󱻵��õ�. 
// SON���̵����������������ת����SIP����. 
// ����sip_conn��SIP���̺�SON����֮���TCP���ӵ��׽���������. 
// ����ͨ��SIP���̺�SON����֮���TCP���ӷ���, ʹ�÷ָ���!&��!#, ����'!& ���� !#'��˳����. 
// ������ķ��ͳɹ�, ����1, ���򷵻�-1.
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

// sendpkt()������SON���̵���, �������ǽ�������SIP���̵ı��ķ��͸���һ��.
// ����conn�ǵ���һ���ڵ��TCP���ӵ��׽���������.
// ����ͨ��SON���̺����ھӽڵ�֮���TCP���ӷ���, ʹ�÷ָ���!&��!#, ����'!& ���� !#'��˳����. 
// ������ķ��ͳɹ�, ����1, ���򷵻�-1.
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

// recvpkt()������SON���̵���, �������ǽ��������ص����������ھӵı���.
// ����conn�ǵ����ھӵ�TCP���ӵ��׽���������.
// ����ͨ��SON���̺����ھ�֮���TCP���ӷ���, ʹ�÷ָ���!&��!#. 
// Ϊ�˽��ձ���, �������ʹ��һ���򵥵�����״̬��FSM
// PKTSTART1 -- ��� 
// PKTSTART2 -- ���յ�'!', �ڴ�'&' 
// PKTRECV -- ���յ�'&', ��ʼ��������
// PKTSTOP1 -- ���յ�'!', �ڴ�'#'�Խ������ݵĽ��� 
// ����ɹ����ձ���, ����1, ���򷵻�-1.
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
