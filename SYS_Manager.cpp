#include "stdafx.h"
#include "EditArea.h"
#include "SYS_Manager.h"
#include "QU_Manager.h"
#include <direct.h>	
#include <string>
#include <iostream>
using namespace std;
void ExecuteAndMessage(char * sql,CEditArea* editArea){//根据执行的语句类型在界面上显示执行结果。此函数需修改
	std::string s_sql = sql;
	if(s_sql.find("select") == 0){
		RC rc;
		SelResult res;
		Init_Result(&res);
		rc = Query(sql,&res);
		if(rc!=SUCCESS) return rc;
		int col_num = res.col_num;
		int row_num = res.row_num;
		char ** fields = new char *[col_num];
		int k=0;
		for(int i = col_num-1;i>=0;i--){
			fields[k] = new char[21];
			memset(fields[k],0,21);
			memcpy(fields[k++],res.fields[i],21);
		}
		char *** rows = new char**[row_num];
		for(int i = 0;i<row_num;i++){
			rows[i] = new char*[col_num];
			int k=0;
			for(int j = col_num-1;j>=0;j--){
				rows[i][k] = new char[100];
				memset(rows[i][k],0,100);
				rows[i][k++]=res.res[i][j];
			}
		}
		editArea->ShowSelResult(col_num,row_num,fields,rows);
		for(int i = 0;i<col_num;i++){
			delete[] fields[i];
		}
		delete[] fields;
		//Destory_Result(&res);
		return;
	}
	RC rc = execute(sql);
	int row_num = 0;
	char**messages;
	switch(rc){
	case SUCCESS:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "操作成功";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	case SQL_SYNTAX:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "有语法错误";
		editArea->ShowMessage(row_num,messages);
		delete[] messages;
		break;
	default:
		row_num = 1;
		messages = new char*[row_num];
		messages[0] = "功能未实现";
		editArea->ShowMessage(row_num,messages);
	delete[] messages;
		break;
	}
}

RC execute(char * sql){
	sqlstr *sql_str = NULL;
	RC rc;
	sql_str = get_sqlstr();
  	rc = parse(sql, sql_str);//只有两种返回结果SUCCESS和SQL_SYNTAX
	RC tmp=FAIL;
	if (rc == SUCCESS)
	{
		int i = 0;
		switch (sql_str->flag)
		{
			case 1:
			//判断SQL语句为select语句
				//AfxMessageBox("该模式不支持select语句！");
				//tmp=FAIL;
				//tmp=Select(sql_str->sstr.sel.nSelAttrs,sql_str->sstr.sel.selAttrs,sql_str->sstr.sel.nRelations,
					//sql_str->sstr.sel.relations,sql_str->sstr.sel.nConditions,sql_str->sstr.sel.conditions,)
				break;

			case 2:
			//判断SQL语句为insert语句
				tmp=Insert(sql_str->sstr.ins.relName,sql_str->sstr.ins.nValues,sql_str->sstr.ins.values);
				break;

			case 3:	
			//判断SQL语句为update语句
				tmp=Update(sql_str->sstr.upd.relName,sql_str->sstr.upd.attrName,
					&sql_str->sstr.upd.value,sql_str->sstr.upd.nConditions,sql_str->sstr.upd.conditions);
				break;

			case 4:					
			//判断SQL语句为delete语句
				tmp=Delete(sql_str->sstr.del.relName,sql_str->sstr.del.nConditions,sql_str->sstr.del.conditions);
				break;

			case 5:
			//判断SQL语句为createTable语句
				tmp=CreateTable(sql_str->sstr.cret.relName,sql_str->sstr.cret.attrCount,sql_str->sstr.cret.attributes);
				break;

			case 6:	
			//判断SQL语句为dropTable语句
				tmp=DropTable(sql_str->sstr.drt.relName);
				break;

			case 7:
			//判断SQL语句为createIndex语句
				tmp=_CreateIndex(sql_str->sstr.crei.indexName,sql_str->sstr.crei.relName,sql_str->sstr.crei.attrName);
				break;
	
			case 8:	
			//判断SQL语句为dropIndex语句
				tmp=DropIndex(sql_str->sstr.dri.indexName);
				break;
			
			case 9:
			//判断为help语句，可以给出帮助提示

				break;
		
			case 10: 
			//判断为exit语句，可以由此进行退出操作
				break;		
		}
	}else{
		AfxMessageBox(sql_str->sstr.errors);//弹出警告框，sql语句词法解析错误信息
		return rc;
	}
	return tmp;
}

RC CreateTable(char *relName, int attrCount, AttrInfo *attributes)
{
	int attroffset=0;
	int charmount=0;
	RM_FileHandle *table_filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	table_filehandle->bOpen=false;
	RM_FileHandle *column_filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	column_filehandle->bOpen=false;
	RID *column_rid=(RID*)malloc(sizeof(RID));
	column_rid->bValid=false;
	RID *table_rid=(RID*)malloc(sizeof(RID));
	column_rid->bValid=false;
	RC tmp;
	tmp=RM_OpenFile("SYSCOLUMNS.xx",column_filehandle);
	if(tmp!=SUCCESS){
		AfxMessageBox("系统列文件打开失败");
		return tmp;
	}
	for(int i=0;i<attrCount;i++)
	{
		char column[76];
		char ix_flag='0';
		strcpy(column,relName);
		strcpy(column+21,attributes[i].attrName);
		memcpy(column+42,&attributes[i].attrType,4);
		memcpy(column+46,&attributes[i].attrLength,4);
		memcpy(column+50,&attroffset,4);
		memcpy(column+54,&ix_flag,1);
		tmp=InsertRec(column_filehandle,column,column_rid);
		if (tmp!=SUCCESS)
		{
			AfxMessageBox("插入列记录失败");
			return tmp;
		}
		if(attributes[i].attrType==ints)
		{
			attroffset=attroffset+sizeof(int);
		}
		if(attributes[i].attrType==chars)
		{
			charmount++;
			attroffset=attroffset+21;
		}
		if(attributes[i].attrType==floats)
		{
			attroffset=attroffset+sizeof(float);
		}
	}
	tmp=RM_CloseFile(column_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭列文件失败");
		return tmp;
	}
	tmp=RM_CreateFile(relName,attroffset);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("创建表文件失败");
		return tmp;
	}
	//char* table=relName+attrCount;
	tmp=RM_OpenFile("SYSTABLES.xx",table_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开行文件失败");
		return tmp;
	}
	char *table=(char*)malloc(sizeof(char)*25);
	strcpy(table,relName);
	memcpy(table+21,&attrCount,sizeof(int));
	tmp=InsertRec(table_filehandle,table,table_rid);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("插入行记录失败");
		return tmp;
	}
	tmp=RM_CloseFile(table_filehandle);
	if(tmp!=SUCCESS){
		AfxMessageBox("关闭行文件失败");
		return tmp;
	}
	free(table);
	free(table_filehandle);
	free(column_filehandle);
	free(table_rid);
	free(column_rid);
	return SUCCESS;
}


RC DropTable(char *relName)
{
	DeleteFile(relName);
	//remove(relName);//删掉该表的记录文件
	//还需要删除SYSTABLES和SYSTABLES中的对应的数据
	RM_FileHandle *table_filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileHandle *column_filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileScan *table_mfilescan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_FileScan *column_mfilescan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_Record *table_rec=(RM_Record*)malloc(sizeof(RM_Record));
	RM_Record *column_rec=(RM_Record*)malloc(sizeof(RM_Record));
	RC tmp;
	int attrcount=0;
	tmp=RM_OpenFile("SYSTABLES.xx",table_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开行文件失败");
		return tmp;
	}
	tmp=RM_OpenFile("SYSCOLUMNS.xx",column_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开列文件失败");
		return tmp;
	}
	Con *con=(Con*)malloc(sizeof(Con));
	con->bLhsIsAttr=1;
	con->bRhsIsAttr=0;
	con->attrType=chars;
	con->LattrLength=21;
	con->LattrOffset=0;
	con->compOp=EQual;
	/*con->Rvalue=(char*)malloc(sizeof(char));
	strcpy((char*)con->Rvalue,relName);*/
	con->Rvalue=relName;
	tmp=OpenScan(table_mfilescan,table_filehandle,1,con);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开行文件扫描失败");
		return tmp;
	}
	tmp=GetNextRec(table_mfilescan,table_rec);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("找不到符合条件的记录");
		return tmp;
	}
	memcpy(&attrcount,table_rec->pData+21,sizeof(int));
	tmp=DeleteRec(table_filehandle,table_rec->rid);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("删除行文件记录失败");
		return tmp;
	}
	tmp=OpenScan(column_mfilescan,column_filehandle,1,con);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开列文件扫描失败");
		return tmp;
	}
	for (int i=0;i<attrcount;i++)
	{
		tmp=GetNextRec(column_mfilescan,column_rec);
		if (tmp!=SUCCESS)
		{
			AfxMessageBox("找不到适合条件的记录");
			return tmp;
		}
		tmp=DeleteRec(column_filehandle,column_rec->rid);
		if (tmp!=SUCCESS)
		{
			AfxMessageBox("删除列文件记录失败");
			return tmp;
		}
	}
	tmp=RM_CloseFile(column_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭列文件失败");
		return tmp;
	}
	tmp=RM_CloseFile(table_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭行文件失败");
		return tmp;
	}
	tmp=CloseScan(table_mfilescan);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭行文件扫描失败");
		return tmp;
	}
	tmp=CloseScan(column_mfilescan);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭列文件扫描失败");
		return tmp;
	}
	free(con);
	free(table_mfilescan);
	free(table_filehandle);
	free(table_rec);
	free(column_mfilescan);
	free(column_filehandle);
	free(column_rec);
	return SUCCESS;
}

RC CreateDB(char *dbpath,char *dbname)
{
	RC rc1,rc2;
	//创建系统表文件和系统列文件
	//SetCurrentDirectory(dbpath);
	string path=dbpath;
	string systable=path+"\\SYSTABLES.xx";
	string syscolumns=path+"\\SYSCOLUMNS.xx";
	rc1 = RM_CreateFile((char*)systable.c_str(),25);
	rc2 = RM_CreateFile((char*)syscolumns.c_str(),76);
	if (rc1 != SUCCESS || rc2!=SUCCESS)
	{
		AfxMessageBox("创建系统文件失败！");
		return rc1==SUCCESS?rc1:rc2;
	}
	return SUCCESS;
}
void deleteFile(char* dbName)
{
	CFileFind finder;
    CString path;
    path.Format("%s/*.*",dbName);
    BOOL bWorking = finder.FindFile(path);
    while(bWorking)
    {
        bWorking = finder.FindNextFile();
        if(finder.IsDirectory() && !finder.IsDots())//处理文件夹
        {
            deleteFile(finder.GetFilePath().GetBuffer(0));//递归删除文件夹
            if(!RemoveDirectory(finder.GetFilePath()))
			{
				AfxMessageBox(finder.GetFilePath());
			}
        }
        else//处理文件
        {
            DeleteFile(finder.GetFilePath());
        }
    }
}
RC DropDB(char *dbname){
	deleteFile(dbname);
	if(!RemoveDirectory(dbname))
	{
		AfxMessageBox("删除失败");
	}
	return SUCCESS;
}

RC OpenDB(char *dbname){
	SetCurrentDirectory(dbname);
	return SUCCESS;
}


RC CloseDB(){
	return SUCCESS;
}

bool CanButtonClick(){//需要重新实现
	//如果当前有数据库已经打开
	return true;
	//如果当前没有数据库打开
	//return false;
}

/*该函数在关系relName的属性attrName上创建名为indexName的索引。
函数首先检查在标记属性上是否已经存在一个索引，如果存在，则返回一个非零的错误码。
否则，创建该索引。创建索引的工作包括：①创建并打开索引文件；②逐个扫描被索引的记录，并向索引文件中插入索引项；③关闭索引。*/
RC _CreateIndex(char *indexName,char *relName, char *attrName)
{
	RC tmp;
	RM_FileHandle* colHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	colHandle->bOpen=false;
	RM_FileScan* fileScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	fileScan->bOpen=false;

	//打开列文件
	tmp=RM_OpenFile("SYSCOLUMNS.xx",colHandle);
	if(tmp!=SUCCESS)
		return tmp;
	//设置扫描条件
	Con* con=(Con*)malloc(sizeof(Con)*2);
	con->attrType=chars;
	con->bLhsIsAttr=1;
	con->bRhsIsAttr=0;
	con->compOp=EQual;
	con->LattrLength=21;
	con->LattrOffset=0;
	con->Rvalue=relName;

	(con+1)->attrType=chars;
	(con+1)->bLhsIsAttr=1;
	(con+1)->bRhsIsAttr=0;
	(con+1)->compOp=EQual;
	(con+1)->LattrLength=21;
	(con+1)->LattrOffset=21;
	(con+1)->Rvalue=attrName;

	OpenScan(fileScan,colHandle,2,con);

	RM_Record* colRec=(RM_Record*)malloc(sizeof(RM_Record));
	colRec->bValid=false;
	tmp=GetNextRec(fileScan,colRec); //扫描一条符合条件的记录
	if(tmp!=SUCCESS) //未找到
	{
		cout<<"cannot find column record:table name:"<<relName<<"attr name:"<<attrName<<endl;
		return FLIED_NOT_EXIST;
	}
	if(*(colRec->pData+52)!='0')  //若该列上已经有索引
	{
		return INDEX_EXIST;
	}
	*(colRec->pData+54)='1';  //标记为有索引
	memset(colRec->pData+55,'\0',21);
	strcpy(colRec->pData+55,indexName);

	UpdateRec(colHandle,colRec); //更新记录列记录文件

	CloseScan(fileScan);
	free(fileScan);
	RM_CloseFile(colHandle);
	free(colHandle);
	free(con);

	//创建索引文件
	AttrType* attrType=(AttrType*)malloc(sizeof(int));
	memcpy(attrType,colRec->pData+42,sizeof(int));
	int attrLength;
	memcpy(&attrLength,colRec->pData+46,sizeof(int));
	CreateIndex(indexName,*attrType,attrLength);	
	free(attrType);

	//打开索引文件
	IX_IndexHandle* indexHandle=(IX_IndexHandle*)malloc(sizeof(IX_IndexHandle));
	indexHandle->bOpen=false;
	OpenIndex(indexName,indexHandle);

	//扫描所有的记录，将索引的记录生成索引项并写入索引文件
	RM_FileHandle* recHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	recHandle->bOpen=false;
	RM_FileScan* recScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	recScan->bOpen=false;
	RM_Record* rec=(RM_Record*)malloc(sizeof(RM_Record));
	rec->bValid=false;
	RM_OpenFile(relName,recHandle);
	OpenScan(recScan,recHandle,0,NULL);
	int attrOffset;
	memcpy(&attrOffset,colRec->pData+42+sizeof(int)*2,sizeof(int));
	free(colRec);
	char* attrValue=(char*)malloc(attrLength);
	//插入索引项
	while(SUCCESS==GetNextRec(recScan,rec))
	{
		memcpy(attrValue,rec->pData+attrOffset,attrLength);
		InsertEntry(indexHandle,attrValue,rec->rid);
	}
	free(attrValue);
	CloseScan(recScan);
	RM_CloseFile(recHandle);
	CloseIndex(indexHandle);
	free(indexHandle);
	free(recScan);
	free(recHandle);
	return SUCCESS;
}
RC DropIndex (char *indexName)
{
	RC tmp;
	IX_IndexHandle* indexHandle=(IX_IndexHandle*)malloc(sizeof(IX_IndexHandle));
	indexHandle->bOpen=false;
	tmp=OpenIndex(indexName,indexHandle);
	if(tmp!=SUCCESS) //索引不存在
	{
		return INDEX_NOT_EXIST;
	}
	CloseIndex(indexHandle);
	free(indexHandle);

	RM_FileHandle* colHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	colHandle->bOpen=false;
	RM_FileScan* recScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	recScan->bOpen=false;
	RM_Record* localRec=(RM_Record*)malloc(sizeof(localRec));
	localRec->bValid=false;
	tmp=RM_OpenFile("SYSCOLUMNS.xx",colHandle);
	if(tmp!=SUCCESS)
	{
		AfxMessageBox("系统列文件打开失败");
		return tmp;
	}
	//设置扫描条件
	Con* con=(Con*)malloc(sizeof(Con));
	con->attrType=chars;
	con->bLhsIsAttr=1;
	con->bRhsIsAttr=0;
	con->compOp=EQual;
	con->LattrLength=21;
	con->LattrOffset=42+3*sizeof(int)+sizeof(char);
	con->Rvalue=indexName;
	tmp=OpenScan(recScan,colHandle,1,con);
	if(SUCCESS!=tmp)
	{
		AfxMessageBox("系统列文件扫描失败");
		return tmp;
	}
	tmp=GetNextRec(recScan,localRec);
	if(tmp==SUCCESS)
	{
		*(localRec->pData+42+3*sizeof(int))='0';
		memset(localRec->pData+42+4*sizeof(int)+sizeof(char),'0',21); //将索引删除
		UpdateRec(colHandle,localRec);//更新记录
	}
	CloseScan(recScan);
	free(recScan);
	RM_CloseFile(colHandle);
	free(colHandle);
	free(localRec);
	free(con);
	DeleteFile(indexName);
	return SUCCESS;
}

RC Update(char *relName,char *attrName,Value *value,int nConditions,Condition *conditions)
{
	RC tmp;
	RM_FileHandle* tabHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	tabHandle->bOpen=false;
	RM_FileScan* tabScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	tabScan->bOpen=false;
	RM_Record* tabRec=(RM_Record*)malloc(sizeof(RM_Record));
	tabRec->bValid=false;

	//先判断该表是否存在
	tmp=RM_OpenFile("SYSTABLES.xx",tabHandle);
	if(SUCCESS!=tmp)
	{
		AfxMessageBox("系统表文件打开失败");
		return tmp;
	}
	Con* con=(Con*)malloc(sizeof(Con));
	con->attrType=chars;
	con->bLhsIsAttr=1;
	con->bRhsIsAttr=0;
	con->compOp=EQual;
	con->LattrLength=21;
	con->LattrOffset=0;
	con->Rvalue=relName;
	tmp=OpenScan(tabScan,tabHandle,1,con);
	if(SUCCESS!=tmp)
	{
		AfxMessageBox("系统表文件扫描失败");
		return tmp;
	}
	tmp=GetNextRec(tabScan,tabRec);
	if(SUCCESS==tmp) //表存在
	{
		RM_FileHandle* colHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
		colHandle->bOpen=false;
		RM_FileScan* colScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
		colScan->bOpen=false;
		RM_Record* colRec=(RM_Record*)malloc(sizeof(RM_Record));
		colRec->bValid=false;
		tmp=RM_OpenFile("SYSCOLUMNS.xx",colHandle);
		if(SUCCESS!=tmp)
		{
			AfxMessageBox("系统列文件打开失败");
			return tmp;
		}
		con=(Con*)realloc(con,2*sizeof(Con));
		(con+1)->attrType=chars;
		(con+1)->bLhsIsAttr=1;
		(con+1)->bRhsIsAttr=0;
		(con+1)->compOp=EQual;
		(con+1)->LattrLength=21;
		(con+1)->LattrOffset=21;
		(con+1)->Rvalue=attrName;
		tmp=OpenScan(colScan,colHandle,2,con);
		if(SUCCESS!=tmp)
		{
			AfxMessageBox("系统列文件扫描失败");
			return tmp;
		}
		tmp=GetNextRec(colScan,colRec);
		if(SUCCESS!=tmp)
		{
			AfxMessageBox("该属性不存在！");
			return tmp;
		}
		CloseScan(colScan);


		int attrLength,attrOffset,ifHasIndex;
		char* indexName=NULL;
		//memset(indexName,0,21);
		memcpy(&attrLength,colRec->pData+42+sizeof(int),sizeof(int));
		memcpy(&attrOffset,colRec->pData+42+2*sizeof(int),sizeof(int));
		if(*(colRec->pData+42+3*sizeof(int))=='1') //判断该属性上是否有索引
		{
			indexName=(char*)malloc(21);
			strcpy(indexName,colRec->pData+42+3*sizeof(int)+sizeof(char));
			ifHasIndex=1;
		}
		else ifHasIndex=0;
		//设置查询条件
		Con* cons=(Con*)malloc(sizeof(Con)*nConditions);
		RM_Record* tmpRec=(RM_Record*)malloc(sizeof(RM_Record));
		for(int i=0;i<nConditions;++i)
		{
			if(conditions[i].bLhsIsAttr==1&&conditions[i].bRhsIsAttr==0)//左边是属性右边是值
			{
				(con+1)->Rvalue=conditions[i].lhsAttr.attrName;
			}
			else if(conditions[i].bLhsIsAttr==0&&conditions[i].bRhsIsAttr==1)//左边是值右边是属性
			{
				(con+1)->Rvalue=conditions[i].rhsAttr.attrName;
			}
			else if(conditions[i].bLhsIsAttr==1&&conditions[i].bRhsIsAttr==1)//两边都是属性
			{
				(con+1)->Rvalue=conditions[i].lhsAttr.attrName;
				OpenScan(colScan,colHandle,2,con);
				GetNextRec(colScan,tmpRec);
				(con+1)->Rvalue=conditions[i].rhsAttr.attrName;
				CloseScan(colScan);
			}
			OpenScan(colScan,colHandle,2,con);
			GetNextRec(colScan,colRec);

			(cons+i)->bLhsIsAttr=conditions[i].bLhsIsAttr;
			(cons+i)->bRhsIsAttr=conditions[i].bRhsIsAttr;
			(cons+i)->compOp=conditions[i].op;
			if(conditions[i].bLhsIsAttr==1&&conditions[i].bRhsIsAttr==0)
			{
				memcpy(&(cons+i)->LattrLength,colRec->pData+42+sizeof(int),sizeof(int));
				memcpy(&(cons+i)->LattrOffset,colRec->pData+42+2*sizeof(int),sizeof(int));
				(cons+i)->attrType=conditions[i].rhsValue.type;
				(cons+i)->Rvalue=conditions[i].rhsValue.data;
			}
			else if(conditions[i].bLhsIsAttr==0&&conditions[i].bRhsIsAttr==1)
			{
				memcpy(&(cons+i)->RattrLength,colRec->pData+42+sizeof(int),sizeof(int));
				memcpy(&(cons+i)->RattrOffset,colRec->pData+42+2*sizeof(int),sizeof(int));
				(cons+i)->attrType=conditions[i].lhsValue.type;
				(cons+i)->Lvalue=conditions[i].lhsValue.data;
			}
			else if(conditions[i].bLhsIsAttr==1&&conditions[i].bRhsIsAttr==1)
			{
				memcpy(&(cons+i)->LattrLength,tmpRec->pData+42+sizeof(int),sizeof(int));
				memcpy(&(cons+i)->LattrOffset,tmpRec->pData+42+2*sizeof(int),sizeof(int));
				memcpy(&(cons+i)->RattrLength,colRec->pData+42+sizeof(int),sizeof(int));
				memcpy(&(cons+i)->RattrOffset,colRec->pData+42+2*sizeof(int),sizeof(int));
				memcpy(&(cons+i)->attrType,colRec->pData+42,sizeof(int));
			}
			CloseScan(colScan);
		}
		RM_CloseFile(colHandle);
		free(colHandle);
		free(colScan);
		free(colRec);
		free(con);
		free(tmpRec);


		//打开数据文件
		RM_FileHandle* dataHandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
		dataHandle->bOpen=false;
		RM_FileScan* dataScan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
		dataScan->bOpen=false;
		RM_Record* dataRec=(RM_Record*)malloc(sizeof(RM_Record));
		dataRec->bValid=false;
		IX_IndexHandle* indexHandle=NULL;
		tmp=RM_OpenFile(relName,dataHandle);
		if(SUCCESS!=tmp)
		{
			AfxMessageBox("数据文件打开失败");
			return tmp;
		}
		tmp=OpenScan(dataScan,dataHandle,nConditions,cons);
		if(SUCCESS!=tmp)
		{
			AfxMessageBox("数据文件扫描失败");
			return tmp;
		}
		if(ifHasIndex)//如果有索引
		{
			indexHandle=(IX_IndexHandle*)malloc(sizeof(IX_IndexHandle));
			OpenIndex(indexName,indexHandle);
		}
		char* oldValue=(char*)malloc(attrLength);
		while(GetNextRec(dataScan,dataRec)==SUCCESS)
		{	
			if(ifHasIndex)
			{
				memcpy(oldValue,dataRec->pData+attrOffset,attrLength);
				DeleteEntry(indexHandle,oldValue,dataRec->rid);
				InsertEntry(indexHandle,value->data,dataRec->rid);
			}
			memcpy(dataRec->pData+attrOffset,value->data,attrLength);
			UpdateRec(dataHandle,dataRec);
		}
		RM_CloseFile(dataHandle);
		CloseScan(dataScan);
		free(dataHandle);
		free(dataScan);
		free(dataRec);
		free(cons);
		free(indexName);
	}
	RM_CloseFile(tabHandle);
	CloseScan(tabScan);
	free(tabHandle);
	free(tabScan);
	free(tabRec);
	return SUCCESS;
}


RC Insert(char *relName,int nValues,Value *values)
{
	RC tmp;
	RM_FileHandle *filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	tmp=RM_OpenFile(relName,filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开该表文件失败");
		return tmp;
	}
	char *pdata=(char*)malloc(sizeof(char)*21*nValues);
	int offset=0;
	RID *rid=(RID*)malloc(sizeof(RID));
	for (int i=nValues;i>=0;i--)
	{
		switch(values[i].type){
		case ints:
			memcpy(pdata+offset,values[i].data,sizeof(int));
			offset=offset+sizeof(int);
			break;
		case floats:
			memcpy(pdata+offset,values[i].data,sizeof(float));
			offset=offset+sizeof(float);
			break;
		case chars:
			memcpy(pdata+offset,values[i].data,21);
			offset=offset+21;
			break;
		}
	}
	tmp=InsertRec(filehandle,pdata,rid);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("插入记录失败");
		return tmp;
	}
	free(pdata);
	free(filehandle);
	free(rid);
	RM_FileHandle *column_filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileScan *column_filescan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_Record *column_record=(RM_Record*)malloc(sizeof(RM_Record));
	IX_IndexHandle *indexhandle=(IX_IndexHandle*)malloc(sizeof(IX_IndexHandle));
	//IX_IndexScan *indexscan=(IX_IndexScan*)malloc(sizeof(IX_IndexScan));
	tmp=RM_OpenFile("SYSCOLUMNS.XX",column_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开列文件失败");
		return tmp;
	}
	Con *con=(Con*)malloc(sizeof(Con));
	con->bLhsIsAttr=1;
	con->bRhsIsAttr=0;
	con->attrType=chars;
	con->LattrLength=21;
	con->LattrOffset=0;
	con->compOp=EQual;
	con->Rvalue=relName;
	//strcmp((char*)con->Rvalue,relName);
	tmp=OpenScan(column_filescan,column_filehandle,1,con);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开列文件扫描失败");
		return tmp;
	}
	for (int i=0;i<nValues;i++)
	{
		tmp=GetNextRec(column_filescan,column_record);
		if (tmp!=SUCCESS)
		{
			AfxMessageBox("获取记录失败");
			return tmp;
		}
		char ix_flag='0';
		char* indexname=(char*)malloc(sizeof(char)*21);
		memcpy(&ix_flag,column_record->pData+54,sizeof(char));
		if (ix_flag=='1')
		{
			//values[i].data;
			memcpy(indexname,column_record->pData+55,21);
			tmp=OpenIndex(indexname,indexhandle);
			if (tmp!=SUCCESS)
			{
				AfxMessageBox("打开索引失败");
				return tmp;
			}
			tmp=InsertEntry(indexhandle,values[i].data,rid);
			if (tmp!=SUCCESS)
			{
				AfxMessageBox("插入索引成功");
				return tmp;
			}
		}
		free(indexname);
	}
	free(column_filehandle);
	free(column_filescan);
	free(column_record);
	free(indexhandle);
	//free(indexscan);
	return SUCCESS;
}
RC Delete(char *relName,int nConditions,Condition *conditions){
	RC tmp;
	Con *con=(Con*)malloc(sizeof(Con)*nConditions);
	Con *column_con=(Con*)malloc(sizeof(Con)*2);
	Con *column_con1=(Con*)malloc(sizeof(Con)*2);
	Con *column_con2=(Con*)malloc(sizeof(Con)*2);
	RM_FileHandle *column_filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileHandle *filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileHandle *table_filehandle=(RM_FileHandle*)malloc(sizeof(RM_FileHandle));
	RM_FileScan *column_filescan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_FileScan *table_filescan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_FileScan *filescan=(RM_FileScan*)malloc(sizeof(RM_FileScan));
	RM_Record *column_rec0=(RM_Record*)malloc(sizeof(RM_Record));
	RM_Record *column_rec=(RM_Record*)malloc(sizeof(RM_Record));
	RM_Record *column_rec1=(RM_Record*)malloc(sizeof(RM_Record));
	RM_Record *column_rec2=(RM_Record*)malloc(sizeof(RM_Record));
	RM_Record *record=(RM_Record*)malloc(sizeof(RM_Record));
	RM_Record *table_rec=(RM_Record*)malloc(sizeof(RM_Record));
	int attrcount=0;
	int ix_amount=0;
	tmp=RM_OpenFile("SYSTABLES.xx",table_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开行文件失败");
		return tmp;
	}
	tmp=RM_OpenFile("SYSCOLUMNS.xx",column_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开列文件失败");
		return tmp;
	}
	Con *tc_con=(Con*)malloc(sizeof(Con));
	tc_con->bLhsIsAttr=1;
	tc_con->bRhsIsAttr=0;
	tc_con->attrType=chars;
	tc_con->LattrLength=21;
	tc_con->LattrOffset=0;
	tc_con->compOp=EQual;
	tc_con->Rvalue=relName;
	tmp=OpenScan(table_filescan,table_filehandle,1,tc_con);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开行文件扫描失败");
		return tmp;
	}
	tmp=GetNextRec(table_filescan,table_rec);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("找不到符合条件的记录");
		return tmp;
	}
	tmp=CloseScan(table_filescan);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭索引扫描失败");
		return tmp;
	}
	tmp=RM_CloseFile(table_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭索引扫描失败");
		return tmp;
	}
	memcpy(&attrcount,table_rec->pData+21,sizeof(int));
	/*tmp=DeleteRec(table_filehandle,table_rec->rid);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("删除行文件记录失败");
		return tmp;
	}*/
	char **indexname=(char**)malloc(sizeof(char*)*attrcount);
	int *attrlength=(int*)malloc(sizeof(int)*attrcount);
	int *attroffset=(int*)malloc(sizeof(int)*attrcount);
	tmp=OpenScan(column_filescan,column_filehandle,1,tc_con);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开列文件扫描失败");
		return tmp;
	}
	for (int i=0;i<attrcount;i++)
	{
		tmp=GetNextRec(column_filescan,column_rec0);
		if (tmp!=SUCCESS)
		{
			AfxMessageBox("找不到适合条件的记录");
			return tmp;
		}
		char ix_flag='0';
		memcpy(&ix_flag,column_rec0->pData+54,1);
		if (ix_flag==1)
		{
			memcpy(indexname[ix_amount],column_rec0->pData+55,21);
			memcpy(&attrlength[ix_amount],column_rec0->pData+46,4);
			memcpy(&attroffset[ix_amount],column_rec0->pData+42,4);
			ix_amount++;
		}
	}
	tmp=CloseScan(column_filescan);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭索引扫描失败");
		return tmp;
	}
	free(column_rec0);
	//RM_Record *column_rec=(RM_Record*)malloc(sizeof(RM_Record));
	for (int i=0;i<nConditions;i++)
	{
		con[i].bLhsIsAttr=conditions[i].bLhsIsAttr;
		con[i].bRhsIsAttr=conditions[i].bRhsIsAttr;
		 if (conditions[i].bLhsIsAttr==0&&conditions[i].bRhsIsAttr==1)//左边是值，右边是属性
		 {
			 for (int j=0;j<2;j++)
			 {
				 column_con[j].bLhsIsAttr=1;
				 column_con[j].bRhsIsAttr=0;
				 column_con[j].attrType=chars;
				 column_con[j].LattrLength=21;
				 column_con[j].LattrOffset=0+j*21;
				 column_con[j].compOp=EQual;
				 if (j==0)
				 {
					 column_con[j].Rvalue=relName;
				 }
				 else column_con[j].Rvalue=conditions[i].rhsAttr.attrName;
			 }
			 tmp=OpenScan(column_filescan,column_filehandle,2,column_con);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("打开列文件扫描失败");
				 return tmp;
			 }
			 tmp=GetNextRec(column_filescan,column_rec);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("搜索列文件失败");
				 return tmp;
			 }
			 tmp=CloseScan(column_filescan);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("关闭索引扫描失败");
				 return tmp;
			 }
			 con[i].attrType=conditions[i].lhsValue.type;
			 memcpy(&con[i].RattrLength,column_rec->pData+46,4);
			 memcpy(&con[i].RattrOffset,column_rec->pData+50,4);
			 con[i].compOp=conditions[i].op;
			 con[i].Lvalue=conditions[i].lhsValue.data;
		 } 
		 else if (conditions[i].bRhsIsAttr==0&&conditions[i].bLhsIsAttr==1)//右边是值，左边是属性
		 {
			 for (int j=0;j<2;j++)
			 {
				 column_con[j].bLhsIsAttr=1;
				 column_con[j].bRhsIsAttr=0;
				 column_con[j].attrType=chars;
				 column_con[j].LattrLength=21;
				 column_con[j].LattrOffset=0+j*21;
				 column_con[j].compOp=EQual;
				 if (j==0)
				 {
					 column_con[j].Rvalue=relName;
				 }
				 else column_con[j].Rvalue=conditions[i].lhsAttr.attrName;
			 }
			 tmp=OpenScan(column_filescan,column_filehandle,2,column_con);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("打开列文件扫描失败");
				 return tmp;
			 }
			 tmp=GetNextRec(column_filescan,column_rec);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("搜索列文件失败");
				 return tmp;
			 }
			 tmp=CloseScan(column_filescan);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("关闭索引扫描失败");
				 return tmp;
			 }
			 con[i].attrType=conditions[i].rhsValue.type;
			 memcpy(&con[i].LattrLength,column_rec->pData+46,4);
			 memcpy(&con[i].LattrOffset,column_rec->pData+50,4);
			 con[i].compOp=conditions[i].op;
			 con[i].Rvalue=conditions[i].rhsValue.data;
		 }
		 else if(conditions[i].bLhsIsAttr==1&&conditions[i].bRhsIsAttr==1){//左右都是属性
			 for (int j=0;j<2;j++)
			 {
				 column_con1[j].bLhsIsAttr=1;
				 column_con1[j].bRhsIsAttr=0;
				 column_con1[j].attrType=chars;
				 column_con1[j].LattrLength=21;
				 column_con1[j].LattrOffset=0+j*21;
				 column_con1[j].compOp=EQual;
				 if (j==0)
				 {
					 column_con1[j].Rvalue=relName;
				 }
				 else column_con1[j].Rvalue=conditions[i].lhsAttr.attrName;
			 }
			 tmp=OpenScan(column_filescan,column_filehandle,2,column_con1);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("打开列文件扫描失败");
				 return tmp;
			 }
			 tmp=GetNextRec(column_filescan,column_rec1);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("搜索列文件失败");
				 return tmp;
			 }
			 tmp=CloseScan(column_filescan);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("关闭索引扫描失败");
				 return tmp;
			 }
			 for (int j=0;j<2;j++)
			 {
				 column_con2[j].bLhsIsAttr=1;
				 column_con2[j].bRhsIsAttr=0;
				 column_con2[j].attrType=chars;
				 column_con2[j].LattrLength=21;
				 column_con2[j].LattrOffset=0+j*21;
				 column_con2[j].compOp=EQual;
				 if (j==0)
				 {
					 column_con2[j].Rvalue=relName;
				 }
				 else column_con2[j].Rvalue=conditions[i].rhsAttr.attrName;
			 }
			 tmp=OpenScan(column_filescan,column_filehandle,2,column_con2);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("打开列文件扫描失败");
				 return tmp;
			 }
			 tmp=GetNextRec(column_filescan,column_rec2);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("搜索列文件失败");
				 return tmp;
			 }
			 tmp=CloseScan(column_filescan);
			 if (tmp!=SUCCESS)
			 {
				 AfxMessageBox("关闭索引扫描失败");
				 return tmp;
			 }
			 memcpy(&con[i].attrType,column_rec1->pData+42,4);
			 memcpy(&con[i].LattrLength,column_rec1->pData+46,4);
			 memcpy(&con[i].LattrOffset,column_rec1->pData+50,4);
			 memcpy(&con[i].RattrLength,column_rec2->pData+46,4);
			 memcpy(&con[i].RattrOffset,column_rec2->pData+50,4);
			 con[i].compOp=conditions[i].op;
		 }
		 else{
			 return SQL_SYNTAX;
		 }
	}
	tmp=RM_CloseFile(column_filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭列文件失败");
		return tmp;
	}
	tmp=RM_OpenFile(relName,filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开表文件失败");
		return tmp;
	}
	tmp=OpenScan(filescan,filehandle,nConditions,con);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("打开表文件扫描失败");
		return tmp;
	}
	IX_IndexHandle *indexhandle=(IX_IndexHandle*)malloc(sizeof(IX_IndexHandle));
	//IX_IndexScan *indexscan=(IX_IndexScan*)malloc(sizeof(IX_IndexScan));
	while (true)
	{
		tmp=GetNextRec(filescan,record);
		if (tmp!=SUCCESS)break;
		DeleteRec(filehandle,record->rid);
		for (int i=0;i<ix_amount;i++)
		{
			tmp=OpenIndex(indexname[ix_amount],indexhandle);
			if (tmp!=SUCCESS)
			{
				AfxMessageBox("打开索引失败");
				return tmp;
			}
			char* pdata=(char*)malloc(sizeof(char));
			memcpy(pdata,record->pData+attroffset[ix_amount],attrlength[ix_amount]);
			tmp=DeleteEntry(indexhandle,pdata,record->rid);
			if (tmp!=SUCCESS)
			{
				AfxMessageBox("删除索引失败");
				return tmp;
			}
			tmp=CloseIndex(indexhandle);
			if (tmp!=SUCCESS)
			{
				AfxMessageBox("关闭索引失败");
				return tmp;
			}
			free(pdata);
		}
	}
	tmp=CloseScan(filescan);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭扫描失败");
		return tmp;
	}
	tmp=RM_CloseFile(filehandle);
	if (tmp!=SUCCESS)
	{
		AfxMessageBox("关闭文件失败");
		return tmp;
	}
	free(con);
	free(column_con1);
	free(column_con2);
	free(column_filehandle);
	free(filehandle);
	free(table_filehandle);
	free(column_filescan);
	free(table_filescan);
	free(filescan);
	free(column_rec);
	free(column_rec1);
	free(column_rec2);
	free(record);
	free(table_rec);
	free(tc_con);
	free(indexname);
	free(attrlength);
	free(attroffset);
	free(indexhandle);
}