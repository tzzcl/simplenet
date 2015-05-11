
#include "seg.h"

//STCP进程使用这个函数发送sendseg_arg_t结构(包含段及其目的节点ID)给SIP进程.
//参数sip_conn是在STCP进程和SIP进程之间连接的TCP描述符.
//如果sendseg_arg_t发送成功,就返回1,否则返回-1.
int sip_sendseg(int sip_conn, int dest_nodeID, seg_t* segPtr)
{
	sendseg_arg_t *seg_arg = malloc(sizeof(sendseg_arg_t));
	if (send(sip_conn,"!&",2,0)<=0) goto bad;
	if (send(sip_conn,&dest_nodeID,sizeof(int),0)<=0) goto bad;
	stcp_hdr_t *header=&(segPtr->seg.header);
	header->checksum=0;
	header->checksum=checksum(&segPtr->seg);
	if (send(sip_conn,segPtr,HEADER_LENGTH+header->length,0)<=0) goto bad;
	header->checksum=0;
        printf("%s: succeeded\n",__FUNCTION__);
	print_seg(segPtr);
	return 1;
bad:
        printf("%s: failed\n",__FUNCTION__);
	return -1;
	/*seg_arg->nodeID = dest_nodeID;
	memcpy(&seg_arg->seg,segPtr,sizeof(seg_t));
	stcp_hdr_t *header=&(seg_arg->seg.header);
	header->checksum=0;
	header->checksum=checksum(&seg_arg->seg);
	if (send(sip_conn,seg_arg,4+HEADER_LENGTH+segPtr->header.length,0)<=0) goto bad;
	if (send(sip_conn,"!#",2,0)<=0) goto bad;
        printf("%s: succeeded\n",__FUNCTION__);
	print_seg(&seg_arg->seg);
	free(seg_arg);
	return 1;
bad:
        printf("%s: failed\n",__FUNCTION__);
	free(seg_arg);
	return -1;*/
}

//STCP进程使用这个函数来接收来自SIP进程的包含段及其源节点ID的sendseg_arg_t结构.
//参数sip_conn是STCP进程和SIP进程之间连接的TCP描述符.
//当接收到段时, 使用seglost()来判断该段是否应被丢弃并检查校验和.
//下行有修正！
	//如果成功接收到sendseg_arg_t就返回0, 否则返回1.
int recv_onebyte(int connection,char* ch){
	char buf[2];memset(buf,0,sizeof(buf));
	int n=recv(connection,buf,1,0);
	if (n<=0) {
		//perror("Receive Error!");
		return -1;
	}
	ch[0]=buf[0];
	return 0;
}
int sip_recvseg(int sip_conn, int* src_nodeID, seg_t* segPtr)
{
  int state=0;
  char* head=0;
  sendseg_arg_t *seg_arg=malloc(sizeof(sendseg_arg_t));
  while (1){
	char now;
	int recv_state=recv_onebyte(sip_conn,&now);
	if (recv_state<0) {free(seg_arg);return -1;}
	switch (state){
		case 0:{
			if (now=='!') state=1;
			break;
		  }
		case 1:{
			if (now=='&') {
				head=(char*)seg_arg;state=2;
			}
			else puts("head error!");
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
		/*if (head-(char*)seg_arg>sizeof(sendseg_arg_t)){
			printf("%s: bad recv length %d\n",head-(char*)seg_arg);
			goto bad;
		}*/
		*src_nodeID = seg_arg->nodeID;
		printf("Receive Seg From Node %d:\n",*src_nodeID);
		memcpy(segPtr,&seg_arg->seg,sizeof(seg_t));
		print_seg(segPtr);
		break;	
	}
  }
  if (seglost(segPtr)) goto bad;
//add by 121220313<formalhhh@qq.com>
  if (checkchecksum(segPtr)==-1) {
	puts("Checksum Error!");
	goto bad;
  }
  free(seg_arg);
  return 0;
bad:
  free(seg_arg);
  return 1;
}

//SIP进程使用这个函数接收来自STCP进程的包含段及其目的节点ID的sendseg_arg_t结构.
//参数stcp_conn是在STCP进程和SIP进程之间连接的TCP描述符.
//如果成功接收到sendseg_arg_t就返回1, 否则返回-1.
int getsegToSend(int stcp_conn, int* dest_nodeID, seg_t* segPtr)
{
  int state=0;
  char* head=0;
  sendseg_arg_t *seg_arg=malloc(sizeof(sendseg_arg_t));
  while (1){
	char now;
	int recv_state=recv_onebyte(stcp_conn,&now);
	if (recv_state<0) {free(seg_arg);return -1;}
	switch (state){
		case 0:{
			if (now=='!') state=1;
			break;
		  }
		case 1:{
			if (now=='&') {
				head=(char*)seg_arg;state=2;
			}
			else puts("head error!");
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
		/*if (head-(char*)seg_arg>sizeof(sendseg_arg_t)){
			printf("%s: bad recv length %d\n",__FUNCTION__,head-(char*)seg_arg);
			goto bad;
		}*/
		*dest_nodeID = seg_arg->nodeID;
		printf("Seg sending to %d:\n",*dest_nodeID);
		memcpy(segPtr,&seg_arg->seg,sizeof(seg_t));
		print_seg(segPtr);
		break;	
	}
  }
/******** not necessary for interprocess connect *********
  if (seglost(segPtr)) goto bad;
//add by 121220313<formalhhh@qq.com>
  if (checkchecksum(segPtr)==-1) {
	puts("Checksum Error!");
	goto bad;
  }*/
  free(seg_arg);
  return 0;
bad:
  free(seg_arg);
  return 1;
}

//SIP进程使用这个函数发送包含段及其源节点ID的sendseg_arg_t结构给STCP进程.
//参数stcp_conn是STCP进程和SIP进程之间连接的TCP描述符.
//如果sendseg_arg_t被成功发送就返回1, 否则返回-1.
int forwardsegToSTCP(int stcp_conn, int src_nodeID, seg_t* segPtr)
{
	sendseg_arg_t *seg_arg = malloc(sizeof(sendseg_arg_t));
	if (send(stcp_conn,"!&",2,0)<=0) goto bad;
	if (send(stcp_conn,&src_nodeID,sizeof(int),0)<=0) goto bad;
	stcp_hdr_t *header=&(segPtr->header);
	if (send(sip_conn,segPtr,HEADER_LENGTH+header->length,0)<=0) goto bad;
	printf("%s: succeeded\n",__FUNCTION__);
	print_seg(segPtr);
	return 1;
bad:
        printf("%s: failed\n",__FUNCTION__);
	return -1;
/*
	if (send(stcp_conn,"!&",2,0)<=0) goto bad;
	seg_arg->nodeID = src_nodeID;
	memcpy(&seg_arg->seg,segPtr,sizeof(seg_t));
	//stcp_hdr_t *header=&(seg_arg->seg.header);
	/*header->checksum=0;
	header->checksum=checksum(&seg_arg->seg);*/
/*	if (send(stcp_conn,seg_arg,4+HEADER_LENGTH+segPtr->header.length,0)<=0) goto bad;
	if (send(stcp_conn,"!#",2,0)<=0) goto bad;
        printf("%s: succeeded\n",__FUNCTION__);
	print_seg(&seg_arg->seg);
	free(seg_arg);
	return 1;
bad:
        printf("%s: failed\n",__FUNCTION__);
	free(seg_arg);
	return -1;*/
}

// 一个段有PKT_LOST_RATE/2的可能性丢失, 或PKT_LOST_RATE/2的可能性有着错误的校验和.
// 如果数据包丢失了, 就返回1, 否则返回0. 
// 即使段没有丢失, 它也有PKT_LOST_RATE/2的可能性有着错误的校验和.
// 我们在段中反转一个随机比特来创建错误的校验和.
int seglost(seg_t* segPtr) {
	int random = rand()%100;
	if(random<PKT_LOSS_RATE*100) {
		//50%可能性丢失段
		if(rand()%2==0) {
			printf("seg lost!!!\n");
      return 1;
		}
		//50%可能性是错误的校验和
		else {
			//获取数据长度
			int len = sizeof(stcp_hdr_t)+segPtr->header.length;
			//获取要反转的随机位
			int errorbit = rand()%(len*8);
			//反转该比特
			char* temp = (char*)segPtr;
			temp = temp + errorbit/8;
			*temp = *temp^(1<<(errorbit%8));
			return 0;
		}
	}
	return 0;
}

//这个函数计算指定段的校验和.
//校验和计算覆盖段首部和段数据. 你应该首先将段首部中的校验和字段清零, 
//如果数据长度为奇数, 添加一个全零的字节来计算校验和.
//校验和计算使用1的补码.
unsigned short checksum(seg_t* segment) {
  //sizeof(short) is 2
	unsigned long cksum=0;
	unsigned short* buffer=(unsigned short*)segment;
	int size=HEADER_LENGTH+segment->header.length;
	
	while(size >1)
    {
        cksum+=*(buffer++);
        size -=sizeof(unsigned short);
    }
    if(size){
		cksum+=(unsigned short)(*(unsigned char *)buffer)<<8;
	}
	while ((cksum>>16)){
    cksum = (cksum >> 16) + (cksum & 0xffff);
	}
    segment->header.checksum = (unsigned short)(~cksum);
    return (unsigned short)(~cksum);
}

//这个函数检查段中的校验和, 正确时返回1, 错误时返回-1.
int checkchecksum(seg_t* segment) {
	int temp=segment->header.checksum;
	int r=(checksum(segment)==0);
	segment->header.checksum=temp;
	if (r)return 1;
	return -1;
}
void print_seg(seg_t* seg){/*
	printf("segment:\n");
	stcp_hdr_t * header=&(seg->header);
	printf("src_port is %d\n",header->src_port);
	printf("dest_port is %d\n",header->dest_port);
	printf("seq_num is %d\n",header->seq_num);
	printf("ack_num is %d\n",header->ack_num);
	printf("length is %d\n",header->length);
	printf("type is %d\n",header->type);
	printf("rcv_win is %d\n",header->rcv_win);
	printf("checksum is %d\n",header->checksum);*/
	stcp_hdr_t * header=&(seg->header);
	printf(	"-----------------------------------------------------\n"
		"|  SRC: %3d  |  DST: %3d  | SEQ:" "%6d | ACK:" "%6d |\n"
		"|       LENGTH:""%5d      |        TYPE:%3d         |\n"
		"|                  CHECKSUM: 0x%04x                 |\n"
		"-----------------------------------------------------\n"
		,header->src_port,header->dest_port\
		,header->seq_num,header->ack_num\
		,header->length,header->type\
		,header->checksum);
}
