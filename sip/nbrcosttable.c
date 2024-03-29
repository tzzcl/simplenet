
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "nbrcosttable.h"
#include "../common/constants.h"
#include "../topology/topology.h"

//这个函数动态创建邻居代价表并使用邻居节点ID和直接链路代价初始化该表.
//邻居的节点ID和直接链路代价提取自文件topology.dat. 
nbr_cost_entry_t* nbrcosttable_create()
{
  int m=topology_getMyNodeID();
  if (m==-1) {
	printf("%s: getMyNodeID failed\n",__FUNCTION__);
	return NULL;
  }
  int n=topology_getNbrNum();
  if (n==0) return NULL;
  nbr_cost_entry_t *nct=malloc(sizeof(nbr_cost_entry_t)*n);
  int *a=topology_getNbrArray();
  for (int i=0;i<n;i++) {
	nct->nodeID=a[i];
	nct->cost=topology_getCost(m,a[i]);
	printf("%s: nct->cost: %d\n",__FUNCTION__,nct->cost);
	nct++;
  }
  free(a);
  return nct-n;
}

//这个函数删除邻居代价表.
//它释放所有用于邻居代价表的动态分配内存.
void nbrcosttable_destroy(nbr_cost_entry_t* nct)
{
  for (int n=topology_getNbrNum();n>0;n--) {
	free(nct);
	nct++;
  }
  return;
}

//这个函数用于获取邻居的直接链路代价.
//如果邻居节点在表中发现,就返回直接链路代价.否则返回INFINITE_COST.
unsigned int nbrcosttable_getcost(nbr_cost_entry_t* nct, int nodeID)
{
  for (int n=topology_getNbrNum();n>0;n--) {
	if (nct->nodeID==nodeID) return nct->cost;
	nct++;
  }
  return INFINITE_COST;
}

//这个函数打印邻居代价表的内容.
void nbrcosttable_print(nbr_cost_entry_t* nct)
{
  printf("Neighbor Cost Table, NODE %d:\nNODEID\tCOST\n",topology_getMyNodeID());
  for (int n=topology_getNbrNum();n>0;n--) {
	printf("%d\t%d\n",nct->nodeID,nct->cost);
	nct++;
  }
  return;
}
