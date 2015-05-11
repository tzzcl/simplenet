//�ļ���: server/stcp_server.c
//
//����: ����ļ�����STCP�������ӿ�ʵ��. 
//
//��������: 2015��

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/select.h>
#include <strings.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include "stcp_server.h"
#include "../topology/topology.h"
#include "../common/constants.h"

//����tcbtableΪȫ�ֱ���
server_tcb_t* tcbtable[MAX_TRANSPORT_CONNECTIONS];
//������SIP���̵�����Ϊȫ�ֱ���
int sip_conn;

server_tcb_t* server_table[MAX_TRANSPORT_CONNECTIONS];
int time_out[MAX_TRANSPORT_CONNECTIONS];
pthread_t seghandle;
pthread_t out_handler[MAX_TRANSPORT_CONNECTIONS];
volatile int conn_stcp=0;
int listens;
pthread_mutex_t listens_mutex;
socklen_t clilen;
struct sockaddr_in cliaddr;
/*********************************************************************/
//
//STCP APIʵ��
//
/*********************************************************************/

// ���������ʼ��TCB��, ��������Ŀ���ΪNULL. �������TCP�׽���������conn��ʼ��һ��STCP���ȫ�ֱ���, 
// �ñ�����Ϊsip_sendseg��sip_recvseg���������. ���, �����������seghandler�߳�����������STCP��.
// ������ֻ��һ��seghandler.
void stcp_server_init(int conn) 
{
  for (int i=0;i<MAX_TRANSPORT_CONNECTIONS;i++){
    	server_table[i]=NULL;
	}
  	pthread_mutex_init(&listens_mutex,NULL);
	memset(time_out,0,sizeof(time_out));
	conn_stcp=conn;
	listens=-1;
	printf("%s: conn_stcp <- sip_conn %d\n:",__FUNCTION__,conn_stcp);
	pthread_create(&seghandle,NULL,seghandler,NULL);
  return;
}

// ����������ҷ�����TCB�����ҵ���һ��NULL��Ŀ, Ȼ��ʹ��malloc()Ϊ����Ŀ����һ���µ�TCB��Ŀ.
// ��TCB�е������ֶζ�����ʼ��, ����, TCB state������ΪCLOSED, �������˿ڱ�����Ϊ�������ò���server_port. 
// TCB������Ŀ������Ӧ��Ϊ�����������׽���ID�������������, �����ڱ�ʶ�������˵�����. 
// ���TCB����û����Ŀ����, �����������-1.
void server_table_init(server_tcb_t* server_t,unsigned int server_port){
	server_t->server_nodeID=topology_getMyNodeID();
	server_t->server_portNum=server_port;
	server_t->client_nodeID=0;
	server_t->client_portNum=0;
	server_t->state=CLOSED;
	server_t->expect_seqNum=0;
	server_t->recvBuf=(char*)malloc(RECEIVE_BUF_SIZE );
	memset(server_t->recvBuf,0,sizeof(char)*RECEIVE_BUF_SIZE );
	server_t->usedBufLen=0;
	server_t->bufMutex=(pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(server_t->bufMutex,NULL);
}
int stcp_server_sock(unsigned int server_port)
{
    for (int i=0;i<MAX_TRANSPORT_CONNECTIONS;i++){
		if (server_table[i]==NULL){
			server_table[i]=(server_tcb_t*)malloc(sizeof(server_tcb_t));
			server_table_init(server_table[i],server_port);
			return i;
		}
	}
	return -1;
}

// �������ʹ��sockfd���TCBָ��, �������ӵ�stateת��ΪLISTENING. ��Ȼ��������ʱ������æ�ȴ�ֱ��TCB״̬ת��ΪCONNECTED 
// (���յ�SYNʱ, seghandler�����״̬��ת��). �ú�����һ������ѭ���еȴ�TCB��stateת��ΪCONNECTED,  
// ��������ת��ʱ, �ú�������1. �����ʹ�ò�ͬ�ķ�����ʵ�����������ȴ�.
int stcp_server_accept(int sockfd) 
{/*
	pthread_mutex_lock(&listens_mutex);
	if (listens==-1){
		int ret=accept(conn_stcp,(struct sockaddr *)&cliaddr,&clilen);
		fprintf(stderr,"%s: accept sock to %d > conn_stcp\n",__FUNCTION__,ret);
		conn_stcp=ret;
		listens=0;
	}
	pthread_mutex_unlock(&listens_mutex);*/
	server_table[sockfd]->state=LISTENING;
	while (server_table[sockfd]->state!=CONNECTED) ;
	return 1;
}

// ��������STCP�ͻ��˵�����. �������ÿ��RECVBUF_POLLING_INTERVALʱ��
// �Ͳ�ѯ���ջ�����, ֱ���ȴ������ݵ���, ��Ȼ��洢���ݲ�����1. ����������ʧ��, �򷵻�-1.
int stcp_server_recv(int sockfd, void* buf, unsigned int length) 
{
	int recv_length=0;
	while (recv_length<length){
		
		pthread_mutex_lock(server_table[sockfd]->bufMutex);
		int buflen=server_table[sockfd]->usedBufLen;
		int nowlen=length-recv_length;
		int recv_cp=0;
		if (buflen>nowlen) recv_cp=nowlen;else recv_cp=buflen;
		printf("%d %d %d %d\n",sockfd,buflen,nowlen,recv_cp);
		memcpy(buf,server_table[sockfd]->recvBuf,recv_cp);
		recv_length+=recv_cp;
		buf+=recv_cp;
		server_table[sockfd]->usedBufLen-=recv_cp;
		memmove(server_table[sockfd]->recvBuf,server_table[sockfd]->recvBuf+recv_cp,server_table[sockfd]->usedBufLen);
		pthread_mutex_unlock(server_table[sockfd]->bufMutex);
		sleep(RECVBUF_POLLING_INTERVAL);
	}
	return 0;
}

// �����������free()�ͷ�TCB��Ŀ. ��������Ŀ���ΪNULL, �ɹ�ʱ(��λ����ȷ��״̬)����1,
// ʧ��ʱ(��λ�ڴ����״̬)����-1.
int stcp_server_close(int sockfd) 
{
	free(server_table[sockfd]->recvBuf);
    free(server_table[sockfd]);
	server_table[sockfd]=NULL;
	return 1;
}

// ������stcp_server_init()�������߳�. �������������Կͻ��˵Ľ�������. seghandler�����Ϊһ������sip_recvseg()������ѭ��, 
// ���sip_recvseg()ʧ��, ��˵����SIP���̵������ѹر�, �߳̽���ֹ. ����STCP�ε���ʱ����������״̬, ���Բ�ȡ��ͬ�Ķ���.
// ��鿴�����FSM���˽����ϸ��.
void trans(seg_t* seg){
	stcp_hdr_t * header=&(seg->header);
	header->src_port=ntohl(header->src_port);
	header->dest_port=ntohl(header->dest_port);
	header->seq_num=ntohl(header->seq_num);
	header->ack_num=ntohl(header->ack_num);
	header->length=ntohs(header->length);
	header->type=ntohs(header->type);
	header->rcv_win=ntohs(header->rcv_win);
	header->checksum=ntohs(header->checksum);
}
int equal_cands(stcp_hdr_t* client,server_tcb_t* server){
	return client->dest_port==server->server_portNum;
}

void init_seg(seg_t* seg,server_tcb_t* server,int state){
	memset(seg,0,sizeof(seg_t));
	stcp_hdr_t * header=&(seg->header);
	header->src_port=server->server_portNum;
	header->dest_port=server->client_portNum;
	header->seq_num=0;
	header->ack_num=0;
	header->length=0;
	header->type=state;
	header->rcv_win=0;
	header->checksum=0;
}
void* out_handle(void* arg){
	int i=(int)arg;
	while (1){
	sleep(CLOSEWAIT_TIMEOUT);
	int now=clock()/CLOCKS_PER_SEC;
	if (now-time_out[i]>=CLOSEWAIT_TIMEOUT){
		while (server_table[i]->usedBufLen>0) ;
		//stcp_server_close(i);
		server_table[i]->state=CLOSED;
		printf("Sock %d turn into closed\n",i);
		break;
	}
	}

}
void* seghandler(void* arg) 
{
  seg_t * recv_seg=(seg_t*)malloc(sizeof(seg_t));
  seg_t * send_seg=(seg_t*)malloc(sizeof(seg_t));
 
  memset(recv_seg,0,sizeof(seg_t));
  int correct=0,wrong=0;
  while (1){
	int now_time=clock()/CLOCKS_PER_SEC;
//	int state=sip_recvseg(conn_stcp,recv_seg);
        int src;
        int state=sip_recvseg(conn_stcp,&src,recv_seg);
	if (state==0) correct++;else if (state==1) wrong++;
	//printf("* %d %d\n",correct,wrong);
	if (state<0) {
		/*puts("son closed!");
		for (int i=0;i<MAX_TRANSPORT_CONNECTIONS;i++){
			if (server_table[i]!=NULL) stcp_server_close(i);
		}
		break;*/
	}
	else if (state==0){
		//print_seg(recv_seg);
		for (int i=0;i<MAX_TRANSPORT_CONNECTIONS;i++){
			if (server_table[i]!=NULL){
				//printf("%d\n",i);
				if (equal_cands(&(recv_seg->header),server_table[i])){
					switch (server_table[i]->state){
						case CLOSED:{
						//error
						perror("Error Socket");
						break;
						}
						case LISTENING:{
						//receive SYN,send SYN ACK	
							puts("A");
						if (recv_seg->header.type==SYN){
							printf("%d sock receive SYN,turned into connected\n",i);
							server_table[i]->client_nodeID=src;
							server_table[i]->client_portNum=recv_seg->header.src_port;
							init_seg(send_seg,server_table[i],SYNACK);
//							sip_sendseg(conn_stcp,send_seg);
							sip_sendseg(conn_stcp,server_table[i]->client_nodeID,send_seg);
							server_table[i]->state=CONNECTED;
						}
						break;
						}
						case CONNECTED:{
						//receive SYN,send SYN ACK
						if (recv_seg->header.type==SYN){
							printf("%d sock receive SYN,keep connected\n",i);
							server_table[i]->client_portNum=recv_seg->header.src_port;
							init_seg(send_seg,server_table[i],SYNACK);
//							sip_sendseg(conn_stcp,send_seg);
							sip_sendseg(conn_stcp,server_table[i]->client_nodeID,send_seg);
							server_table[i]->state=CONNECTED;
						}
						else if (recv_seg->header.type==FIN){
							printf("%d sock receive FIN,turn into CLOSEWAIT\n",i);
							server_table[i]->client_portNum=recv_seg->header.src_port;
							init_seg(send_seg,server_table[i],FINACK);
//							sip_sendseg(conn_stcp,send_seg);
							sip_sendseg(conn_stcp,server_table[i]->client_nodeID,send_seg);
							server_table[i]->state=CLOSEWAIT;
							time_out[i]=clock()/CLOCKS_PER_SEC;
							pthread_create(&out_handler[i],NULL,out_handle,(void*)i);
						}
						else if (recv_seg->header.type==DATA){
							//printf("%d***%d\n",recv_seg->header.seq_num,server_table[i]->expect_seqNum);
							if (recv_seg->header.seq_num==server_table[i]->expect_seqNum){
								pthread_mutex_lock(server_table[i]->bufMutex);
								server_table[i]->expect_seqNum+=recv_seg->header.length;
								memcpy(server_table[i]->recvBuf+server_table[i]->usedBufLen,recv_seg->data,recv_seg->header.length);
								server_table[i]->usedBufLen=server_table[i]->usedBufLen+recv_seg->header.length;
								pthread_mutex_unlock(server_table[i]->bufMutex);
							}
							init_seg(send_seg,server_table[i],DATAACK);
							send_seg->header.ack_num=server_table[i]->expect_seqNum;
//							sip_sendseg(conn_stcp,send_seg);
							sip_sendseg(conn_stcp,server_table[i]->client_nodeID,send_seg);

						}
						break;
						}
						case CLOSEWAIT:{
						//receive FIN send FIN ACK
						if (recv_seg->header.type==FIN){
							printf("%d sock receive FIN,keep CLOSEWAIT\n",i);
							server_table[i]->client_portNum=recv_seg->header.src_port;
							time_out[i]=clock()/CLOCKS_PER_SEC;
							init_seg(send_seg,server_table[i],FINACK);
//							sip_sendseg(conn_stcp,send_seg);
							sip_sendseg(conn_stcp,server_table[i]->client_nodeID,send_seg);
							server_table[i]->state=CLOSEWAIT;
						}
						break;
						}
					}
					break;
				}
			}
		}
	}

  }
  return 0;
}

