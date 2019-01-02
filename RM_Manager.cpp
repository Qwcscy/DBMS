#include "stdafx.h"
#include "RM_Manager.h"
#include "str.h"
#include "cJSON.h"
#include <iostream>
#include <string>
using namespace std;
RC OpenScan(RM_FileScan *rmFileScan,RM_FileHandle *fileHandle,int conNum,Con *conditions)//��ʼ��ɨ��
{
	rmFileScan->bOpen=true;
	rmFileScan->conNum=conNum;
	rmFileScan->pRMFileHandle=fileHandle;
	rmFileScan->conditions=conditions;
	rmFileScan->PageHandle=(PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	rmFileScan->PageHandle->pFrame=(Frame*)malloc(sizeof(Frame));
	int pageNum=fileHandle->pf_fileHandle->pFileSubHeader->nAllocatedPages;
	char* pf_bitmap=fileHandle->pf_fileHandle->pBitmap;
	char* rf_bitmap=NULL;
	if(pageNum<=2) //�޷��������ҳ
	{
		rmFileScan->pn=0;
		rmFileScan->sn=0;
	}
	else
	{
		for(int i=2;i<pageNum;++i)
		{
			if((*(pf_bitmap+i/8) & (1<<(i%8)))!=0) //�ҵ���һ���Ѿ������ҳ
			{
				rmFileScan->pn=i;
				GetThisPage(fileHandle->pf_fileHandle,i,rmFileScan->PageHandle);
				GetData(rmFileScan->PageHandle,&rf_bitmap);  //��ȡ��¼ҳ����ʼ��ַ
				for(int j=0;j<fileHandle->rfileSubHeader->recordsPerPage;++j)   //����һ����Ч�ļ�¼
				{
					if((*(rf_bitmap+j/8) & (1<<(j%8)))!=0)
					{
						rmFileScan->sn=j;
						return SUCCESS;
					}
				}
				return PF_FILEERR;
			}
		}
	}
	return SUCCESS; 
}
RC CloseScan(RM_FileScan *rmFileScan)
{
	rmFileScan->bOpen = false;
	rmFileScan->pRMFileHandle=NULL;
	rmFileScan->conNum=0;
	rmFileScan->conditions=NULL;
	if(rmFileScan->PageHandle!=NULL)
	{
		UnpinPage(rmFileScan->PageHandle);
		free(rmFileScan->PageHandle);
		rmFileScan->PageHandle=NULL;
	}
	return SUCCESS;
}
template<class T>
bool compareCon(CompOp compOp,T l_value,T r_value)
{
	switch (compOp)
	{
		case EQual:
			return l_value==r_value;
		case LEqual:
			return l_value<=r_value;
		case GEqual:
			return l_value>=r_value;
		case LessT:
			return l_value<r_value;
		case GreatT:
			return l_value>r_value;
		case NEqual:
			return l_value!=r_value;
		case NO_OP:
			return true;
		default:return false;
	}
}
RC GetNextRec(RM_FileScan *rmFileScan,RM_Record *rec)
{
	if(!rmFileScan->bOpen)
	{
		cout<<"scan error:FileScan is closed"<<endl;
		return PF_FILEERR;
	}
	if(rmFileScan->pn==0) //�޼�¼
	{
		return RM_EOF;
	}
	RM_Record* tmpRec=rec;
	tmpRec->rid=(RID*)malloc(sizeof(RID));
	char* pData;
	bool flag=false;
	while (true)
	{
		tmpRec->rid->pageNum=rmFileScan->pn;
		tmpRec->rid->slotNum=rmFileScan->sn;
		GetData(rmFileScan->PageHandle,&pData);
		tmpRec->pData=pData+rmFileScan->pRMFileHandle->rfileSubHeader->firstRecordOffset+
			rmFileScan->pRMFileHandle->rfileSubHeader->recordSize*tmpRec->rid->slotNum;
		int note=0;
		if(rmFileScan->conNum==0) //������ĿΪ0���򷵻ص�ǰ��¼
			note=rmFileScan->conNum;
		else
		{
			Con* tmpCon;
			for(;note<rmFileScan->conNum;++note)
			{
				tmpCon=rmFileScan->conditions+note;
				switch(tmpCon->attrType)
				{
					case ints:
						int* intLattr,*intRattr;
						if(tmpCon->bLhsIsAttr==1) //���������
							intLattr=(int*)(tmpRec->pData+tmpCon->LattrOffset);
						else 
							intLattr=(int*)tmpCon->Lvalue; //�����ֵ
						if(tmpCon->bRhsIsAttr==1) //�ұ�������
							intRattr=(int*)(tmpRec->pData+tmpCon->RattrOffset);
						else 
							intRattr=(int*)tmpCon->Rvalue; //�����ֵ
						flag=compareCon(tmpCon->compOp,*intLattr,*intRattr);
						break;

					case floats:
						float* floatLattr,*floatRattr;
						if(tmpCon->bLhsIsAttr==1) //���������
							floatLattr=(float*)(tmpRec->pData+tmpCon->LattrOffset);
						else 
							floatLattr=(float*)tmpCon->Lvalue; //�����ֵ
						if(tmpCon->bRhsIsAttr==1) //���������
							floatRattr=(float*)(tmpRec->pData+tmpCon->RattrOffset);
						else 
							floatRattr=(float*)tmpCon->Rvalue; //�����ֵ
						flag=compareCon(tmpCon->compOp,*floatLattr,*floatRattr);
						break;

					case chars:
						char* charLattr,*charRattr;
						if(tmpCon->bLhsIsAttr==1)
						{
							charLattr=(char*)malloc(100*sizeof(char));
							strcpy(charLattr,tmpRec->pData+tmpCon->LattrOffset);
						}
						else
							charLattr=(char*)tmpCon->Lvalue;
						if(tmpCon->bRhsIsAttr==1)
						{
							charRattr=(char*)malloc(100*sizeof(char));
							strcpy(charRattr,tmpRec->pData+tmpCon->RattrOffset);
						}
						else
							charRattr=(char*)tmpCon->Rvalue;
						string str1=charLattr;
						string str2=charRattr;
						flag=compareCon(tmpCon->compOp,str1,str2);
						break;
					//default:
						//flag=false;break;
				}
				if(!flag)
					break;
			}
		}
		//��¼��һ��Ҫɨ���ҳ�źͲ�ۺ�
		int nextSn=rmFileScan->sn+1;
		char* r_bitmap;
		GetData(rmFileScan->PageHandle,&r_bitmap);
		for(;nextSn<rmFileScan->pRMFileHandle->rfileSubHeader->recordsPerPage;nextSn++)
		{
			if((*(r_bitmap+nextSn/8) & (1<<(nextSn%8)))!=0) //�ҵ���һ����Ч��¼
			{
				rmFileScan->sn=nextSn;
				break;
			}
		}
		if(nextSn>=rmFileScan->pRMFileHandle->rfileSubHeader->recordsPerPage) //�ڵ�ǰҳδ�ҵ���Ч��¼
		{
			int nextPage=rmFileScan->pn+1;
			char* p_bitmap=rmFileScan->pRMFileHandle->pf_fileHandle->pBitmap;
			for(;nextPage<rmFileScan->pRMFileHandle->pf_fileHandle->pFileSubHeader->nAllocatedPages;nextPage++)
			{
				if((*(p_bitmap+nextPage/8)) & (1<<(nextPage%8))!=0)
				{
					rmFileScan->pn=nextPage;
					break;
				}
			}
			if(nextPage<rmFileScan->pRMFileHandle->pf_fileHandle->pFileSubHeader->nAllocatedPages) //�ҵ�һ�������ҳ
			{
				GetData(rmFileScan->PageHandle,&p_bitmap);
				for(int i=0;i<rmFileScan->pRMFileHandle->rfileSubHeader->recordsPerPage;++i)
				{
					if((*(p_bitmap+i/8) & (1<<(i%8)))!=0)
					{
						rmFileScan->sn=i;
						break;
					}
				}
			}
			else //δ�ҵ������ҳ
			{
				rmFileScan->pn = 0;
				rmFileScan->sn = 0;
			}
		}
		if(note<rmFileScan->conNum) //���û�м���������������˵����ǰ�ļ�¼����������������������һ��
		{
			flag=false;
			RC tmp=GetNextRec(rmFileScan,tmpRec);
			if(tmp==SUCCESS)
				break;
			else
				return tmp;
		}
		else
			break;
	}
	tmpRec->bValid=true;
	tmpRec->rid->bValid=true;
	rec=tmpRec;
	return SUCCESS;
}

RC GetRec (RM_FileHandle *fileHandle,RID *rid, RM_Record *rec) 
{
	if(!rid->bValid)
	{
		cout<<"get record error:pageNum:"<<rid->pageNum<<",slotNum:"<<rid->slotNum<<"is valid"<<endl;
		return PF_FILEERR;
	}
	PF_PageHandle* pf_fileHandle=(PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	RM_Record* tmpRec=rec;
	char* pData;
	char* record;
	GetThisPage(fileHandle->pf_fileHandle,rid->pageNum,pf_fileHandle);
	GetData(pf_fileHandle,&pData);
	record=pData+(fileHandle->rfileSubHeader->firstRecordOffset)+
		rid->slotNum*fileHandle->rfileSubHeader->recordSize;
	memcpy(tmpRec->pData,record,fileHandle->rfileSubHeader->recordSize);
	tmpRec->bValid=true;
	tmpRec->rid=rid;
	rec=tmpRec;
	UnpinPage(pf_fileHandle);
	free(pf_fileHandle);
	return SUCCESS;
}

RC InsertRec(RM_FileHandle *fileHandle,char *pData, RID *rid)
{
	int pageNum=fileHandle->pf_fileHandle->pFileSubHeader->nAllocatedPages;
	char* pf_bitmap=(char*)fileHandle->pf_fileHandle->pBitmap;
	char* rf_bitmap=(char*)fileHandle->rBitmap;
	char* _pData=NULL;
	rid->bValid=false;
	PF_PageHandle* pageHandle=(PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	for(int i=2;i<pageNum;++i)
	{
		if((*(pf_bitmap+i/8) & (1<<(i%8)))!=0) //�ҵ�һ���ѷ����ҳ
		{
			if((*(rf_bitmap+i/8) & (1<<(i%8)))!=0)  //�ҵ�һ���Ѿ����䵫���˵�ҳ
				continue;
			GetThisPage(fileHandle->pf_fileHandle,i,pageHandle);
			GetData(pageHandle,&_pData);
			for(int j=0;j<fileHandle->rfileSubHeader->recordsPerPage;++j)
			{
				if(((*(_pData+j/8)) & (1<<(j%8)))==0)  //�ҵ�û�м�¼�Ĳ�
				{
					rid->bValid=true;
					rid->pageNum=i;
					rid->slotNum=j;
					break;
				}
			}
			if(rid->bValid)
				break;
		}
	}
	if(!rid->bValid) //���е�ҳ���Ѿ����ˣ��·���һ��ҳ
	{
		AllocatePage(fileHandle->pf_fileHandle,pageHandle);
		GetData(pageHandle,&_pData);
		GetPageNum(pageHandle,&rid->pageNum);
		rid->bValid=true;
		rid->slotNum=0;
		char bit=~(1<<(rid->pageNum%8));
		*(rf_bitmap+rid->pageNum/8)&=bit;
	}
	//д��Ҫ����ļ�¼
	RM_FileSubHeader* subHeader=fileHandle->rfileSubHeader;
	char* offset=_pData+subHeader->firstRecordOffset+subHeader->recordSize*rid->slotNum;
	memcpy(offset,pData,subHeader->recordSize);
	*(_pData+rid->slotNum/8)|=(1<<(rid->slotNum%8)); //��Ǽ�¼����Ч
	subHeader->nRecords++;
	MarkDirty(pageHandle);//��ҳ�޸Ĺ�����
	
	if(rid->slotNum>=subHeader->recordsPerPage-1) //��������¼��ҳ��
	{
		*(rf_bitmap+rid->pageNum/8)|=(1<<(rid->pageNum%8)); //��Ǹ�ҳΪ��ҳ
		MarkDirty(fileHandle->pf_pageHandle); //��¼��Ϣ����ҳ�޸Ĺ�
	}
	UnpinPage(pageHandle);
	free(pageHandle);
	return SUCCESS;
}

RC DeleteRec(RM_FileHandle *fileHandle,const RID *rid)
{
	if(!rid->bValid)
	{
		cout<<"delete record error:pageNum:"<<rid->pageNum<<",slotNum:"<<rid->slotNum<<"is valid"<<endl;
		return PF_FILEERR;
	}
	PF_PageHandle* pageHandle = (PF_PageHandle *)malloc(sizeof(PF_PageHandle));
	char* pData;
	char bit=~(1<<(rid->pageNum%8));
	*(fileHandle->rBitmap+rid->pageNum/8)&=bit;  //ҳ����Ϊ����ҳ
	GetThisPage(fileHandle->pf_fileHandle,rid->pageNum,pageHandle);
	GetData(pageHandle,&pData);
	bit=~(1<<(rid->slotNum%8));
	*(pData+rid->slotNum/8)&=bit;   //�ü�¼�ۼ�����Ч
	fileHandle->rfileSubHeader->nRecords--;
	int i=0;
	for (;i<fileHandle->rfileSubHeader->recordsPerPage;i++)
		if ((*(pData+i/8)&(1<<(i%8)))!=0)  //�ҵ�һ���ǿ��в۾��˳�
			break;
	if(i>=fileHandle->rfileSubHeader->recordsPerPage)  //û�зǿ��вۣ�ɾ����ҳ��
		DisposePage(fileHandle->pf_fileHandle, rid->pageNum);
	if(fileHandle->rfileSubHeader->nRecords==0) //����ҳ�޼�¼
		DisposePage(fileHandle->pf_fileHandle,rid->pageNum);
	MarkDirty(pageHandle);
	MarkDirty(fileHandle->pf_pageHandle);
	UnpinPage(pageHandle);
	free(pageHandle);
	return SUCCESS;
}

RC UpdateRec (RM_FileHandle *fileHandle,const RM_Record *rec)
{
	if(!rec->bValid)
	{
		cout<<"update record error:pageNum:"<<rec->rid->pageNum<<",slotNum:"<<rec->rid->slotNum<<"is valid"<<endl;
		return PF_FILEERR;
	}
	char* pData;
	PF_PageHandle* pageHandle = (PF_PageHandle *)malloc(sizeof(PF_PageHandle));
	GetThisPage(fileHandle->pf_fileHandle,rec->rid->pageNum,pageHandle);
	GetData(pageHandle,&pData);
	pData+=fileHandle->rfileSubHeader->firstRecordOffset+rec->rid->slotNum*fileHandle->rfileSubHeader->recordSize;
	memcpy(pData,rec->pData,fileHandle->rfileSubHeader->recordSize);
	UnpinPage(pageHandle);
	MarkDirty(pageHandle);
	free(pageHandle);
	return SUCCESS;
}

RC RM_CreateFile (char *fileName, int recordSize)
{
	PF_FileHandle* fileHandle=(PF_FileHandle*)malloc(sizeof(PF_FileHandle));
	PF_PageHandle* pageHandle=(PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	PF_PageHandle* pageHandle1=(PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	RM_FileSubHeader* fileSubHeader=(RM_FileSubHeader*)malloc(sizeof(RM_FileSubHeader));
	char* bitmap;
	char* pData;
	int recordNum=0;
	int bitmapLength=0;
	CreateFile(fileName);
	openFile(fileName,fileHandle);
	GetThisPage(fileHandle,0,pageHandle);
	MarkDirty(pageHandle);
	AllocatePage(fileHandle,pageHandle1);

	fileSubHeader->nRecords=0;
	fileSubHeader->recordSize=recordSize;
	recordNum=(PF_PAGESIZE-sizeof(RM_FileSubHeader))/recordSize;
	bitmapLength=(recordNum+7)/8;  //����ȡ��
	while(recordNum*recordSize>PF_PAGESIZE)
	{
		recordNum--;
		bitmapLength=(recordNum+7)/8;
	}
	fileSubHeader->recordsPerPage=recordNum;
	fileSubHeader->firstRecordOffset=bitmapLength;
	GetData(pageHandle1,&pData);
	memcpy(pData,(char*)fileSubHeader,sizeof(RM_FileSubHeader));
	MarkDirty(pageHandle1);

	UnpinPage(pageHandle);
	UnpinPage(pageHandle1);
	CloseFile(fileHandle);
	free(fileHandle);
	free(pageHandle);
	free(pageHandle1);
	free(fileSubHeader);
	return SUCCESS;
}
RC RM_OpenFile(char *fileName, RM_FileHandle *fileHandle)
{
	RM_FileHandle* rfileHandle=fileHandle;
	PF_FileHandle* pf_fileHandle=(PF_FileHandle*)malloc(sizeof(PF_FileHandle));
	PF_PageHandle* pageHandle=(PF_PageHandle*)malloc(sizeof(PF_PageHandle));
	char* pData;
	RC tmp;
	tmp=openFile(fileName,pf_fileHandle);
	if(tmp==PF_FILEERR)
	{
		cout<<"open file "<<fileName<<"failed:PF_FILEERR"<<endl;
		return PF_FILEERR;
	}
	rfileHandle->bOpen=true;
	rfileHandle->pf_fileHandle=pf_fileHandle;
	GetThisPage(pf_fileHandle,1,pageHandle);
	GetData(pageHandle,&pData);
	rfileHandle->pf_pageHandle=pageHandle;
	rfileHandle->rBitmap=pData+sizeof(RM_FileSubHeader);
	rfileHandle->rfileSubHeader=(RM_FileSubHeader*)pData;
	fileHandle=rfileHandle;
	return SUCCESS;
}

RC RM_CloseFile(RM_FileHandle *fileHandle)
{
	RC tmp;
	UnpinPage(fileHandle->pf_pageHandle);
	tmp=CloseFile(fileHandle->pf_fileHandle);
	if(tmp==PF_FILEERR)
	{
		cout<<"close file error:PF_FILEERR"<<endl;
		return PF_FILEERR;
	}
	return SUCCESS;
}
