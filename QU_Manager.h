#ifndef __QUERY_MANAGER_H_
#define __QUERY_MANAGER_H_
#include <vector>
#include <map>
#include <string>
#include "str.h"
#include "RM_Manager.h"
using namespace std;
typedef struct SelResult{
	int col_num;
	int row_num;
	AttrType type[20];	//��������ֶε���������
	int length[20];		//��������ֶ�ֵ�ĳ���
	char fields[20][20];//����ʮ���ֶ���������ÿ���ֶεĳ��Ȳ�����20
	vector<vector<char*>> res;
	SelResult * next_res;
}SelResult;

typedef struct AtrrNode{
	AttrType type;
	int length;
	int offset;
	char attrName[21];
	bool ifHasIndex;
	char indexName[21];
}AttrNode;

void Init_Result(SelResult * res);
void Destory_Result(SelResult * res);

RC Query(char * sql,SelResult * res);

RC Select(int nSelAttrs,RelAttr **selAttrs,int nRelations,char **relations,int nConditions,Condition *conditions,SelResult * res);
#endif