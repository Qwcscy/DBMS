#include "StdAfx.h"
#include "QU_Manager.h"
void Init_Result(SelResult * res){
	res->next_res = NULL;
}

void Destory_Result(SelResult * res){
	for(int i = 0;i<res->row_num;i++){
		for(int j = 0;j<res->col_num;j++){
			delete res->res[i][j];
		}
		/*delete[] res->res[i];*/
	}
	if(res->next_res != NULL){
		Destory_Result(res->next_res);
	}
}

RC Query(char * sql,SelResult * res)
{
	RC tmp;
	sqlstr * sqlType = NULL;
	sqlType = get_sqlstr();
	tmp = parse(sql, sqlType);
	if(SUCCESS!=tmp)
	{
		return tmp;
	}
	selects tmpSel=sqlType->sstr.sel;
	string tableName="student";
	tmp=Select(tmpSel.nSelAttrs,tmpSel.selAttrs,tmpSel.nRelations,tmpSel.relations,
		tmpSel.nConditions,tmpSel.conditions,res);
	return tmp;
}
//对没有携带表名的属性检查其他表内是否出现了同名属性,有返回SUCCESS
RC checkIfHasAttr(string attrName,map<string,vector<AttrNode>> &attrMap)
{
	map<string,vector<AttrNode>>::iterator it=attrMap.begin();
	for(;it!=attrMap.end();++it)
	{
		for(int i=0;i<it->second.size();++i)
			if(attrName==it->second.at(i).attrName)//找到
				return SUCCESS;
	}
	return FAIL;
}
//检查得到的名是否在from后面出现
RC checkIfHasTable(char* tableName,int nRelations,char** relations)
{
	for(int i=0;i<nRelations;++i)
		if(strcmp(tableName,relations[i])==0)
			return SUCCESS;
	return FIELD_NOT_FOUND;
}
RC getAttrMap(int nSelAttrs,RelAttr **selAttrs,map<string,vector<AttrNode>> &attrMap,int nRelations,char** relations)
{
	RM_FileHandle* tabHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileScan* tabScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_Record* tmpRec=(RM_Record*)malloc(sizeof(RM_Record));
	RM_Record* finalRec=NULL;
	tmpRec->bValid=false;
	tabScan->bOpen=false;
	tabHandle->bOpen=false;
	RC tmp;
	Con cons[2];
	cons[0].attrType = chars;
	cons[0].bLhsIsAttr = 1;
	cons[0].LattrOffset = 21;
	cons[0].LattrLength = 21;
	cons[0].compOp = EQual;
	cons[0].bRhsIsAttr = 0;

	cons[1].attrType = chars;
	cons[1].bLhsIsAttr = 1;
	cons[1].LattrOffset = 0;
	cons[1].LattrLength = 21;
	cons[1].compOp = EQual;
	cons[1].bRhsIsAttr = 0;
	tmp = RM_OpenFile("SYSCOLUMNS.xx", tabHandle);
	if (tmp!= SUCCESS) return tmp;
	for(int i=0;i<nSelAttrs;++i)
	{
		AttrNode attrNode;
		int nConditons,attrTimes=0;
		string tableName;
		cons[0].Rvalue=selAttrs[i]->attrName;
		if(selAttrs[i]->relName==NULL)//该属性没有携带表名
		{
			for(int i=0;i<nRelations;++i) 
			{
				cons[1].Rvalue=relations[i];
				tmp=OpenScan(tabScan,tabHandle,2,cons);
				if(tmp!=SUCCESS) return tmp;
				tmp=GetNextRec(tabScan,tmpRec);
				if(tmp==SUCCESS)
				{
					attrTimes++;
					finalRec=tmpRec;
				}
				CloseScan(tabScan);
			}
			if(attrTimes>1)
			{
				AfxMessageBox("属性存在多个表中但未指定表名!");
				return FIELD_AMBIGUITY;
			}
			if(attrTimes==0)
			{
				AfxMessageBox("属性不存在所给表中！");
				return FIELD_NOT_FOUND;
			}
			char tmp[21];
			strcpy(tmp,finalRec->pData);
			tableName=tmp;
		}	
		else
		{
			tableName=selAttrs[i]->relName;
			cons[1].Rvalue=selAttrs[i]->relName;
			tmp=OpenScan(tabScan,tabHandle,2,cons);
			if(tmp!=SUCCESS) return tmp;
			tmp=GetNextRec(tabScan,tmpRec);
			if(tmp!=SUCCESS) 
			{	
				AfxMessageBox("表内无记录或属性不存在!");
				return tmp;
			}
			//检查得到的名是否在from后面出现
			if(checkIfHasTable((char*)tableName.c_str(),nRelations,relations)!=SUCCESS)
			{
				AfxMessageBox("未在所给的表中找到字段!");
				return FIELD_NOT_FOUND;
			}
			finalRec=tmpRec;
		}
		strcpy(attrNode.attrName,selAttrs[i]->attrName);
		memcpy(&attrNode.type,finalRec->pData+42,sizeof(int));
		memcpy(&attrNode.offset,finalRec->pData+50,sizeof(int));
		memcpy(&attrNode.length,finalRec->pData+46,sizeof(int));
		if(*(finalRec->pData+54)=='1')
		{
			attrNode.ifHasIndex=true;//记录索引
			strcpy(attrNode.indexName,finalRec->pData+55);
		}
		else attrNode.ifHasIndex=false;
		if(attrMap.count(tableName)==0)
		{
			vector<AttrNode> attrVec;
			attrMap[tableName]=attrVec;
		}	
		attrMap[tableName].push_back(attrNode);
		CloseScan(tabScan);
	}
	RM_CloseFile(tabHandle);
	free(tabHandle);
	free(tabScan);
	free(tmpRec);
	return SUCCESS;
}


//从指定的表中查询字段并返回
RC getFieldsFromTable(string &tableName,vector<AttrNode> &attrVec,map<string,vector<vector<char*>>> &recMap)
{
	if(recMap.count(tableName)==0)
	{
		vector<vector<char*>> tmp;
		recMap[tableName]=tmp;
	}
	RC tmp;
	RM_FileHandle* recfileHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	recfileHandle->bOpen=false;
	RM_FileScan* recfileScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	recfileScan->bOpen=false;
	RM_Record* tmpRec=(RM_Record*)malloc(sizeof(RM_Record));
	tmpRec->bValid=false;
	tmp=RM_OpenFile((char*)tableName.c_str(),recfileHandle);
	if(SUCCESS!=tmp) return tmp;
	tmp=OpenScan(recfileScan,recfileHandle,0,NULL);
	if(SUCCESS!=tmp) return tmp;
	while(GetNextRec(recfileScan,tmpRec)==SUCCESS)
	{
		vector<char*> tmpVec;
		for(int i=0;i<attrVec.size();++i)
		{
			char* t=new char[100];
			if(attrVec[i].type==chars)
			{
				memcpy(t,tmpRec->pData+attrVec[i].offset,attrVec[i].length);
				t[attrVec[i].length]='\0';
			}
			else if(attrVec[i].type==ints)
			{
				int temp;
				memcpy(&temp,tmpRec->pData+attrVec[i].offset,attrVec[i].length);
				sprintf(t,"%d",temp);
			}
			else if(attrVec[i].type==floats)
			{
				float temp;
				memcpy(&temp,tmpRec->pData+attrVec[i].offset,attrVec[i].length);
				sprintf(t,"%f",temp);
			}
			tmpVec.push_back(t);
		}
		if(tmpVec.size()!=0)
			recMap[tableName].push_back(tmpVec);
	}
	RM_CloseFile(recfileHandle);
	CloseScan(recfileScan);
	free(recfileHandle);
	free(recfileScan);
	free(tmpRec);
	return SUCCESS;
}

//无条件表查询
RC selectWithoutCon(map<string,vector<AtrrNode>> &attrMap,map<string,vector<vector<char*>>> &recMap)
{	
	RC tmp;
	map<string,vector<AtrrNode>>::iterator it=attrMap.begin();
	for(;it!=attrMap.end();++it)
	{
		tmp=getFieldsFromTable((string)it->first,attrMap[it->first],recMap);
		if(SUCCESS!=tmp) return tmp;
	}
	return SUCCESS;
}
vector<vector<char*>> getDiKar(vector<vector<char*>> &res1,vector<vector<char*>> &res2)
{
	if(res1.size()==0) return res2;
	if(res2.size()==0) return res1;
	vector<vector<char*>>::iterator it=res1.begin();
	vector<vector<char*>> res;
	for(int i=0;i<res1.size();++i)
	{
		for(int j=0;j<res2.size();++j)
		{
			res.push_back(res1[i]);
			for(int k=0;k<res2[j].size();k++)
				res[res.size()-1].push_back(res2[j][k]);
		}
	}
	return res;
}
/*检查数组relations内的表是否都存在,是返回SUCCESS，否则返回TABLE_NOT_EXIST*/
RC ifHasTable(int nRelations,char** relations)
{
	RC tmp;
	RM_FileHandle* tabHnadle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	tabHnadle->bOpen=false;
	RM_FileScan* tabScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	tabScan->bOpen=false;
	RM_Record* tmpRec=(RM_Record*)malloc(sizeof(RM_Record));
	tmpRec->bValid=false;
	tmp=RM_OpenFile("SYSTABLES.xx",tabHnadle);
	if(SUCCESS!=tmp)
	{
		AfxMessageBox("打开系统表文件失败！");
		return tmp;
	}
	Con condition;
	condition.bLhsIsAttr = 1;
	condition.attrType = chars;
	condition.LattrOffset = 0;
	condition.bRhsIsAttr = 0;
	condition.compOp = EQual;

	for(int i=0;i<nRelations;++i)
	{
		condition.Rvalue=*(relations+i);
		condition.RattrLength=strlen(*(relations+i))+1;
		tmp=OpenScan(tabScan,tabHnadle,1,&condition);
		if(SUCCESS!=tmp)
		{
			AfxMessageBox("打开系统表文件扫描失败！");
			return tmp;
		}
		tmp=GetNextRec(tabScan,tmpRec);
		if(SUCCESS!=tmp)
			return TABLE_NOT_EXIST;
		CloseScan(tabScan);
	}
	RM_CloseFile(tabHnadle);
	free(tabHnadle);
	free(tabScan);
	free(tmpRec);
	return SUCCESS;
}
RC Select(int nSelAttrs,RelAttr **selAttrs,int nRelations,char **relations,int nConditions,Condition *conditions,SelResult * res)
{	
	SelResult* tmpRes=res;
	RC tmp;
	tmp=ifHasTable(nRelations,relations);//先检查所有的表是否存在
	if(SUCCESS!=tmp)
	{
		AfxMessageBox("表不存在！");
		return tmp;
	}
	map<string,vector<AtrrNode>> attrMap;
	map<string,vector<vector<char*>>> recMap;
	tmp=getAttrMap(nSelAttrs,selAttrs,attrMap,nRelations,relations);
	if(tmp!=SUCCESS) return FAIL;
	if(nConditions==0)//无条件
	{
		tmp=selectWithoutCon(attrMap,recMap);
		if(tmp!=SUCCESS) return FAIL;
	}	
	map<string,vector<AttrNode>>::iterator it=attrMap.begin();
	tmpRes->row_num=0;
	tmpRes->col_num=0;
	for(;it!=attrMap.end();++it)  //初始化结果集字段
	{
		for(int i=0;i<it->second.size();++i)
		{
			strcpy(tmpRes->fields[tmpRes->col_num],it->second[i].attrName);
			tmpRes->length[tmpRes->col_num]=it->second[i].length;
			tmpRes->type[tmpRes->col_num]=it->second[i].type;
			tmpRes->col_num++;
		}
	}
	//求结果的笛卡尔积
	vector<vector<char*>> resVec;
	map<string,vector<vector<char*>>>::iterator it1=recMap.begin();
	for(;it1!=recMap.end();it1++)
		resVec=getDiKar(resVec,it1->second);
	//合并到最终的结果集
	for(int i=0;i<resVec.size();++i)
	{
		if(tmpRes->row_num>=100) //记录数超过100
		{
			tmpRes->next_res=new SelResult();
			tmpRes=tmpRes->next_res;
			tmpRes->row_num=0;
			tmpRes->next_res=NULL;
		}
		res->res.push_back(resVec[i]);
		tmpRes->row_num++;
	}
	return SUCCESS;
}

