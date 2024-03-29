
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../common/constants.h"
#include "../topology/topology.h"
#include "dvtable.h"

//这个函数动态创建距离矢量表.
//距离矢量表包含n+1个条目, 其中n是这个节点的邻居数,剩下1个是这个节点本身.
//距离矢量表中的每个条目是一个dv_t结构,它包含一个源节点ID和一个有N个dv_entry_t结构的数组, 其中N是重叠网络中节点总数.
//每个dv_entry_t包含一个目的节点地址和从该源节点到该目的节点的链路代价.
//距离矢量表也在这个函数中初始化.从这个节点到其邻居的链路代价使用提取自topology.dat文件中的直接链路代价初始化.
//其他链路代价被初始化为INFINITE_COST.
//该函数返回动态创建的距离矢量表.
dv_t* dvtable_create()
{
  int m = topology_getMyNodeID();
  if (m==-1) {
	printf("%s: getMyNodeID failed\n",__FUNCTION__);
	return NULL;
  }
  int n = topology_getNbrNum(),N = topology_getNodeNum();
  dv_t *dvtable = malloc(sizeof(dv_t)*(n+1));
  int *a = topology_getNbrArray(),*A = topology_getNodeArray();
  for (int i=0;i<n;i++) {
	dvtable->nodeID = a[i];
	dv_entry_t *dve = malloc(sizeof(dv_entry_t)*N);
	dvtable->dvEntry = dve;
	for (int j=0;j<N;j++) {
		dve->nodeID = A[j];
		dve->cost = topology_getCost(a[i],A[j]);
		dve++;
	}
	dvtable++;
  }
  dvtable->nodeID = m;
  dv_entry_t *dve=malloc(sizeof(dv_entry_t)*N);
  dvtable->dvEntry = dve;
  for (int j=0;j<N;j++) {
	dve->nodeID = A[j];
	//if (m==A[j]) dve->cost = 0;
	dve->cost = topology_getCost(m,A[j]);
	dve++;
  }
  free(a);
  free(A);
  return dvtable-n;
}

//这个函数删除距离矢量表.
//它释放所有为距离矢量表动态分配的内存.
void dvtable_destroy(dv_t* dvtable)
{
  int n = topology_getNbrNum(),N = topology_getNodeNum();
  for (int i=0;i<=n;i++) {
	for (int j=0;j<N;j++) {
		free(dvtable->dvEntry);
		dvtable->dvEntry++;
	}
	free(dvtable);
	dvtable++;
  }
  return;
}

//这个函数设置距离矢量表中2个节点之间的链路代价.
//如果这2个节点在表中发现了,并且链路代价也被成功设置了,就返回1,否则返回-1.
int dvtable_setcost(dv_t* dvtable,int fromNodeID,int toNodeID, unsigned int cost)
{
  int n = topology_getNbrNum(),N = topology_getNodeNum();
  for (int i=0;i<=n;i++) {
	if (dvtable->nodeID==fromNodeID) {
		dv_entry_t *dve = dvtable->dvEntry;
		for (int j=0;j<N;j++) {
			if (dve->nodeID==toNodeID) {
				dve->cost=cost;
				return 1;
			}
			dve++;
		}
	}
	dvtable++;
  }
  return -1;
}

//这个函数返回距离矢量表中2个节点之间的链路代价.
//如果这2个节点在表中发现了,就返回链路代价,否则返回INFINITE_COST.
unsigned int dvtable_getcost(dv_t* dvtable, int fromNodeID, int toNodeID)
{
  int n = topology_getNbrNum(),N = topology_getNodeNum();
  for (int i=0;i<=n;i++) {
	if (dvtable->nodeID==fromNodeID) {
		dv_entry_t *dve = dvtable->dvEntry;
		for (int j=0;j<N;j++) {
			if (dve->nodeID==toNodeID) return dve->cost;
			dve++;
		}
	}
	dvtable++;
  }
  return INFINITE_COST;
}

//这个函数打印距离矢量表的内容.
void dvtable_print(dv_t* dvtable)
{
  printf("Distance Vector Table, NODE %d:\n",topology_getMyNodeID());
  int n = topology_getNbrNum(),N = topology_getNodeNum();
  printf("COST");
  dv_entry_t *dve=dvtable->dvEntry;
  for (int j=0;j<N;j++) {
	printf("\t%d",dve->nodeID);
	dve++;
  }
  for (int i=0;i<=n;i++) {
	printf("\n%d",dvtable->nodeID);
	dv_entry_t *dve=dvtable->dvEntry;
	for (int j=0;j<N;j++) {
		printf("\t%d",dve->cost);
		dve++;
	}
	dvtable++;
  }/*
  dvtable-=n+1;
  printf("\n-------------------------");
  for (int i=0;i<=n;i++) {
	printf("\n%d",dvtable->nodeID);
	dv_entry_t *dve=dvtable->dvEntry;
	for (int j=0;j<N;j++) {
		printf("\t%d",dvtable_getcost(dvtable-i,dvtable->nodeID,dve->nodeID));
		dve++;
	}
	dvtable++;
  }*/
  printf("\n");
  return;
}

void dvtable_delete(dv_t* dvtable,int nodeID) {
  int n = topology_getNbrNum(),N = topology_getNodeNum();
  for (int i=0;i<=n;i++) {
	if (dvtable->nodeID==nodeID) {
		dv_entry_t *dve = dvtable->dvEntry;
		for (int j=0;j<N;j++) {
			dve->cost = INFINITE_COST;
			dve++;
		}
		continue;
	}
	for (int j=0;j<N;j++) {
		dv_entry_t *dve = dvtable->dvEntry;
		if (dve->nodeID==nodeID) {
			dve->cost = INFINITE_COST;
		}
		dve++;		
	}
	dvtable++;
  }
}
