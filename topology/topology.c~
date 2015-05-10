//文件名: topology/topology.c
//
//描述: 这个文件实现一些用于解析拓扑文件的辅助函数 
//
//创建日期: 2015年

#include "topology.h"
#include "../common/constants.h"

//这个函数返回指定主机的节点ID.
//节点ID是节点IP地址最后8位表示的整数.
//例如, 一个节点的IP地址为202.119.32.12, 它的节点ID就是12.
//如果不能获取节点ID, 返回-1.
int node_table[4]={185,186,187,188};
char hostname_table[4][20]={"csnetlab_1","csnetlab_2","csnetlab_3","csnetlab_4"};
int topology_getNodeIDfromname(char* hostname) 
{
    for (int i=0;i<4;i++)
      if (strcmp(hostname_table[i],hostname)==0)
        return node_table[i];	
    return -1;
}

//这个函数返回指定的IP地址的节点ID.
//如果不能获取节点ID, 返回-1.
int topology_getNodeIDfromip(struct in_addr* addr)
{
	return (addr->s_addr&0xff000000)>>24;
}

in_addr_t topology_getIP(int nodeID)
{
      for (int i=0;i<4;i++)
      {
          if (nodeID==node_table[i])
          {
              struct hostent* temp=gethostbyname(hostname_table[i]);
              printf("***%s\n",temp->h_name);
              in_addr_t addr;
              memcpy(&addr,temp->h_addr,sizeof(in_addr_t));
              return addr;
          }
      }
}
//这个函数返回本机的节点ID
//如果不能获取本机的节点ID, 返回-1.
int topology_getMyNodeID()
{
      char hostName[255];
      gethostname(hostName,255);
      puts(hostName);
      for (int i=0;i<4;i++)
        if (strcmp(hostName,hostname_table[i])==0)
          return node_table[i];
	return -1;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回邻居数.
int topology_getNbrNum()
{
    char hostName[255];
    gethostname(hostName,255);
    FILE* file=fopen("..//topology//topology.dat","r");
    int total=0;
    while (1)
    {
        char first[20],second[20];
        int cost;
        if (fscanf(file,"%s %s %d",first,second,&cost)==EOF) break;
        if (strcmp(first,hostName)==0||strcmp(second,hostName)==0)
        {
            total++;
        }
    }
    //printf("%d node has %d neighbors\n",topology_getMyNodeID(),total);
    return total;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回重叠网络中的总节点数.
int topology_getNodeNum()
{ 
	return 4;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回一个动态分配的数组, 它包含重叠网络中所有节点的ID. 
int* topology_getNodeArray()
{
  int * new_table=(int*)malloc(sizeof(int)*4);
  for (int i=0;i<4;i++)
    new_table[i]=node_table[i];
  return new_table;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回一个动态分配的数组, 它包含所有邻居的节点ID.
int* topology_getNbrArray()
{
    char hostName[255];
    gethostname(hostName,255);
    FILE* file=fopen("..//topology//topology.dat","r");
    int total=0;
    int nbrNum=topology_getNbrNum();
    int *new_table=(int*)malloc(sizeof(int)*nbrNum);
    for (int i=0;i<nbrNum;i++)
      new_table[i]=0;
    while (1)
    {
        char first[20],second[20];
        int cost;
        if (fscanf(file,"%s %s %d",first,second,&cost)==EOF) break;
        if (strcmp(first,hostName)==0)
        {
            for (int i=0;i<4;i++)
            {
              if (strcmp(second,hostname_table[i])==0)
              {
                new_table[total++]=node_table[i];
                break;
              }
            }
        }
        else if (strcmp(second,hostName)==0)
        {
            for (int i=0;i<4;i++)
            {
              if (strcmp(first,hostname_table[i])==0)
              {
                new_table[total++]=node_table[i];
                break;
              }
            }
        }
    }
    return new_table;
 	//return 0;
}


//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回指定两个节点之间的直接链路代价. 
//如果指定两个节点之间没有直接链路, 返回INFINITE_COST.
unsigned int topology_getCost(int fromNodeID, int toNodeID)
{
   FILE* file=fopen("..//topology//topology.dat","r");
    while (1)
    {
        char first[20],second[20];
        int cost;
        if (fscanf(file,"%s %s %d",first,second,&cost)==EOF) break;
        int firstid,secondid;
        for (int i=0;i<4;i++){
          if (strcmp(hostname_table[i],first)==0)
            firstid=node_table[i];
          if (strcmp(hostname_table[i],second)==0)
            secondid=node_table[i];
        }
         if (firstid==fromNodeID&&secondid==toNodeID)
          return cost; 
         if (firstid==toNodeID&&secondid==fromNodeID)
          return cost; 
    }
    return INFINITE_COST;
}
