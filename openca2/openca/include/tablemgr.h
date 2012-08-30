/************************************************************************************
HASH TABLE usage guide:
1. You must be careful with the Init() function

2. In user application data struct(class T), must define "int next[how_many_key]" and 
	"int ipre" and "int inext"

3. maxhashentry(the 5th parameter of Init) must be a 质数 as follows
	it refers to the HASH_KEY_ARRAY size
   maxdata(the 4th parameter of Init) must be ((maxhashentry/8+1)*8)
	it refers to user application data struct array size

4. Insert() and Delete() function (notes below)

5. Select() and SelectNext() function to query database
	SelectPrefix() is part of 模糊查询, to select a prefix

6. UpdateKey() and UpdateKeyS() to modify the key
	UpdateKeyS() is recommended

7. ScanNext() and ScanPre() to scan all the data struct available

8. GetUsedCount() return sum of used unit

9. operator() return pointer to the class T

10.Validate() check the unit is used or not

11.init_classT() reserve system data in user application data struct, usage like below
	User application doesn't need to care this function

12.SetDirty() ClearDirty() CheckDirty()
	when you want to know whether the record is updated from database like ctree
	size of pbitmap[] must be defined as maxdata/4, not maxdata/8 any more

13.BasicTableMgr::Update()
	copies data from parameter to specified record, ipre, inext will be reserved.

14.TableMgr::Update()
	copies data from parameter to specified record, ipre, inext and next[] will
	be reserved.
	key field can NOT be modified
	
15.Copy()		//TANG 2002.07.22
	BasicTableMgr: copy data from pData[oldconst] to pData[index], ipre,inext reserved.
	TableMgr: copy data, but seems needing Insert() then...
************************************************************************************/
/*
{
  53ul,         97ul,         193ul,       389ul,       769ul,
  1543ul,       3079ul,       6151ul,      12289ul,     24593ul,
  49157ul,      98317ul,      196613ul,    393241ul,    786433ul,
  1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul,
  50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul, 
  1610612741ul, 3221225473ul, 4294967291ul
};
*/
#ifndef __TABLEMGR_T__
#define __TABLEMGR_T__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <set.h>

#ifdef __WIN32__
	#include <io.h>
#else
	#include <unistd.h>
	#include <pthread.h>
#endif

//#include <sys/varargs.h>		//comment by TANG 2003.11.21 for linux not support

#include "allocator.h"

#define MAX_NUMOF_KEY_TYPE	10

#define NULLNODEID	0


template <class T> class BasicTableMgr
{
protected:
	T* 		pData;
	unsigned char*	pBitmap;
	int		maxNumOfData;
	int*	pCnt;
	
	Allocator	m_allocator;
	int		whichSem;
public:
	BasicTableMgr():m_allocator(){pCnt = NULL;}
	BasicTableMgr(int nsem, T* pdata, unsigned char* pbitmap, int maxdata)
		:whichSem(nsem), pData(pdata), pBitmap(pbitmap), maxNumOfData(maxdata), 
		 m_allocator((maxdata/8), (char*)pbitmap) {pCnt = NULL;}
	BasicTableMgr(int* pcnt, int nsem, T* pdata, unsigned char* pbitmap, int maxdata)
		:pCnt(pcnt), whichSem(nsem), pData(pdata), pBitmap(pbitmap), maxNumOfData(maxdata), m_allocator((maxdata/8), (char*)pbitmap) {}
	virtual ~BasicTableMgr() {}

	int logTableError(int flag, const char* format, ...) {
		char filename[512] = {0};
		struct tm * ptm;
		char timebuf[64] = {0};
	
		char str[1024]={0};
		char buff[1024] = {0};
		va_list marker;
		va_start(marker,format);
		vsprintf(str, format, marker);
		va_end(marker);
	
		time_t now;
		time(&now);
		memset(timebuf,0,sizeof(timebuf));
		ptm = localtime(&now);

		strftime(timebuf,63,"%Y-%m-%d %H:%M:%S",ptm);

		sprintf(filename,"/export/home/openca/log/tableError.log");

		int fd = open(filename, O_WRONLY|O_CREAT|O_APPEND, 0777);
		if ( fd < 0 ){
			return 0;
		}
#ifdef __WIN32__
		sprintf(buff, "%s TABLE(%d):%s\n",timebuf, flag, str);
#else
		sprintf(buff, "%s pid(%d) threadid(%d) TABLE(%d):%s\n",timebuf,(int)getpid(), (int)pthread_self(),flag,str);
#endif
		write(fd, buff, strlen(buff));
		close(fd);
		return 1;
	}
	
	void Init(int nsem, T* pdata, unsigned char* pbitmap, int maxdata){
		pCnt = NULL;
		whichSem = nsem;
		pData = pdata;
		pBitmap = pbitmap;
		maxNumOfData = maxdata;
		m_allocator.init((maxdata/8), (char*)pbitmap);
		
		if ( (maxdata % 8) != 0 ){
			logTableError(nsem, "maxdata = %d can not be divided by 8, error\n", maxdata);
		}
	}
	
	void Init(int* pcnt, int nsem, T* pdata, unsigned char* pbitmap, int maxdata){
		pCnt = pcnt;
		whichSem = nsem;
		pData = pdata;
		pBitmap = pbitmap;
		maxNumOfData = maxdata;
		m_allocator.init((maxdata/8), (char*)pbitmap);

		if ( (maxdata % 8) != 0 ){
			logTableError(nsem, "maxdata = %d can not be divided by 8, error\n",maxdata);
		}
		//logTableError(nsem, "The unit size is:%d,maxNumOfDatais:%d\n",sizeof(T),maxNumOfData);
	}

	int	GetMaxNumOfData() {
		return maxNumOfData;
	}

	int	GetRealMaxNumOfData() {
		return maxNumOfData-1;
	}
	
	Allocator& GetAllocator(){
		return m_allocator;
	}
	
	/*
	//By zhulei, 2003-11-17
	int	AllocateOne(int pvflag = 1){
		int index;
#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.P(whichSem);
#endif
		index = m_allocator.allocone(whichSem);
		if ( index != 0 ){
			pData[index].ipre = 0;
			pData[index].inext = pData[0].inext;
			pData[(pData[0].inext)].ipre = index;
			pData[0].inext = index;
			if (pCnt) (*pCnt)++;
		}
#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.V(whichSem);
#endif
		if(!index){
			if (pCnt) logTableError(whichSem, "allocone == 0; *pCnt = %d\n",*pCnt);
			else logTableError(whichSem, "allocone == 0; no *pCnt\n");
		}
		return index;	//return 0 means error
	}
	*/

	//Begin,
	//This function is added by zhulei, to force the
	//table manager to allocate the specified record.
	//2001.06.17
	int	AllocateOne(int index = 0, int pvflag = 1) {
#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.P(whichSem);
#endif
		if (index == 0)
			index = m_allocator.allocone(whichSem);
		else {
			if ( !m_allocator.allocone(index, whichSem) ){
#ifdef __WIN32__

#else
				if (pvflag) m_allocator.sem.V(whichSem);
#endif
				return 0;
			}
		}
		if ( index != 0 ){
			pData[index].ipre = 0;
			pData[index].inext = pData[0].inext;
			pData[(pData[0].inext)].ipre = index;
			pData[0].inext = index;
			if (pCnt) (*pCnt)++;
		}
#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.V(whichSem);
#endif
		if(!index){
			if (pCnt) logTableError(whichSem, "allocone(%d) == 0; *pCnt = %d\n",index, *pCnt);
			else logTableError(whichSem, "allocone(%d) == 0; no *pCnt\n",index);
		}
		return index;	//return 0 means error
	}
	//End.

protected:
	int	ReleaseOne(int index, int pvflag = 1){
		if ( !Validate(index) ) return 0;
		
#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.P(whichSem);
#endif
		
		if ( !Validate(index) ) {
#ifdef __WIN32__

#else
			if (pvflag) m_allocator.sem.V(whichSem);
#endif
			return 0;
		}
		
		if ( m_allocator.freeone2(index) == 1 ){
			pData[(pData[index].ipre)].inext = pData[index].inext;
			pData[(pData[index].inext)].ipre = pData[index].ipre;
			if (pCnt) (*pCnt)--;
			memset((void*)(pData+index), 0, sizeof(T));
			
#ifdef __WIN32__

#else
			if (pvflag) m_allocator.sem.V(whichSem);
#endif
			return 1;
		}
		
#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.V(whichSem);
#endif
		return 0;
	}

public:
	virtual int	Delete(int index) {
		return ReleaseOne(index);
	}

	virtual int	ReleaseAll() {
#ifdef __WIN32__

#else
		m_allocator.sem.P(whichSem);
#endif
		memset(pData, 0, sizeof(T)*maxNumOfData);
		memset(pBitmap, 0, maxNumOfData/8); 
		pBitmap[0] = 0x80;	
		if (pCnt) (*pCnt) = 0;
#ifdef __WIN32__

#else
		m_allocator.sem.V(whichSem);
#endif
		return 1;
	}
	
	T&	operator[](int index){
		return (T&)(pData[index]);
	}
	
	T*	operator()(int index){
		if (pData == NULL)
			return (T*)NULL;
		
		if (index <= 0 || index >= maxNumOfData)
			return (T*)NULL;

		return (T*)(pData+index);
	}	
	
	int	Validate(int index)
	{
		if (index >= maxNumOfData || index <= 0)
			return 0;
		
		if (pBitmap[index/8] & (0x80 >> (index % 8)))
			return 1;
		else
			return 0;
	}

	//the order is REVERSE of INSERT order
	int ScanPre(int index=0)
	{	
		//TANG 2009-10-14 11:20
		//index和pData[index].inext表示下标，为0表示无效值；不为0则应该是一个正确的下标
		if ( index && !Validate(index) ) return 0;
		if ( pData[index].inext && !Validate(pData[index].inext) ) return 0;
		
		return (pData[index].inext);	//return 0 means the END
	}
	
	//the order is the same as INSERT order
	int ScanNext(int index=0)
	{	
		//TANG 2009-10-14 11:20
		//index和pData[index].ipre表示下标，为0表示无效值；不为0则应该是一个正确的下标
		if ( index && !Validate(index) ) return 0;
		if ( pData[index].ipre && !Validate(pData[index].ipre) ) return 0;
		
		return (pData[index].ipre);		//return 0 means the END
	}

	int     GetUsedCount() {
		int i,cnt = -1;
		/*int i,j;
		unsigned char mask = 0x01;
		for (i=0; i<maxNumOfData/8; i++) {
			if (pBitmap[i] & 0xff) {
				for (mask = 0x01,j=0; j<8; j++) {
					if (pBitmap[i] & mask)
						cnt++;
					mask <<= 1;
				}
			}
		}*/
		if (pCnt) return (*pCnt);

		for (cnt=0,i=pData[0].inext; i; cnt++,i=pData[i].inext);
		
		return cnt;
	}

	int		Recound() {
		int cnt,i;

		for (cnt=0,i=pData[0].inext; i; cnt++,i=pData[i].inext);

		if (pCnt) (*pCnt) = cnt;
		
		return cnt;
	}
	
	double  GetUsedRate() {
		return ((double)GetUsedCount()/(double)(maxNumOfData-1)*100);
	}

	//Begin,
	//Added by zhulei, for reserve the ipre and inext data.
	//2001.06.17
	virtual int init_classT(T& pData, T& newdata){
		//reserve system data defined in User Application Struct
		newdata.ipre  = pData.ipre;
		newdata.inext = pData.inext;
		
		return 1;
	}
	//End.

	//Begin,
	//Added by zhulei, for update data with input parameter.
	//T.ipre and T.inext will be reserved.
	//2001.06.17
	//Parameters:
	//	int index -- index of the record to be updated
	//	T& temp   -- source data
	virtual int Update(int index, T& newdata, int pvflag = 1) {
		if ( !Validate(index) ) return 0;

#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.P(whichSem);	//TANG 2002.07.22
#endif
		
		init_classT(pData[index], newdata);
		memcpy(&pData[index], &newdata, sizeof(T));
		
#ifdef __WIN32__

#else
		if (pvflag) m_allocator.sem.V(whichSem);
#endif
		return 1;
	}
	//End.
	
	//Add by TANG 2002.05.23
	virtual int Insert_r(int index, T& newdata) {
#ifdef __WIN32__

#else
		m_allocator.sem.P(whichSem);
#endif

		if ( AllocateOne(index, 0) == NULLNODEID ) {
#ifdef __WIN32__

#else
			m_allocator.sem.V(whichSem);
#endif
			return NULLNODEID;
		}
		Update(index, newdata, 0);

#ifdef __WIN32__

#else
		m_allocator.sem.V(whichSem);
#endif
		return index;
	}

	//Begin,
	//Added by TANG, for update data with input parameter.
	//T.ipre and T.inext will be reserved.
	//2002.07.19
	//Parameters:
	//	int index -- index of the record to be updated
	//	T& temp   -- source data
	virtual int Copy(int index, int oldconst) {
		T	newdata;
		
		if ( !Validate(index) || !Validate(oldconst) ) return 0;

		memcpy(&newdata, &pData[oldconst], sizeof(T));
		
		Update(index, newdata, 1);
		
		return 1;
	}
	//End.
	
	
	//TANG 20070821
	int check_list(int flag_print = 0)
	{
		set<int> set_validate;
		set<int> set_scannext;
		set<int> set_scanpre;
		
		unsigned int count = 0;
		int index = 0;
		for ( int i=1; i<maxNumOfData; i++ )
		{
			if ( Validate(i) )
			{
				count++;
				set_validate.insert(i);
				if ( set_validate.size() != count )
				{
					if ( flag_print ) printf("Table: check_list(): set_validate.size() != count, error\n");
					return 1;
				}
			}
		}
		
		count = 0;
		index = 0;
		while ( (index = ScanNext(index)) )
		{
			count++;
			set_scannext.insert(index);
			if ( set_scannext.size() != count )
			{
				if ( flag_print ) printf("Table: check_list(): set_scannext.size() != count, error\n");
				return 2;
			}
		}
		
		count = 0;
		index = 0;
		while ( (index = ScanPre(index)) )
		{
			count++;
			set_scanpre.insert(index);
			if ( set_scanpre.size() != count )
			{
				if ( flag_print ) printf("Table: check_list(): set_scanpre.size() != count, error\n");
				return 3;
			}
		}
		
		//检查set中的元素的数目是不是相同
		if ( set_validate.size() != set_scannext.size() )
		{
			if ( flag_print ) printf("Table: check_list(): set_validate.size() != set_scannext.size(), error\n");
			return 4;
		}
		
		if ( set_validate.size() != set_scanpre.size() )
		{
			if ( flag_print ) printf("Table: check_list(): set_validate.size() != set_scanpre.size(), error\n");
			return 5;
		}
		
		if ( set_scannext.size() != set_scanpre.size() )
		{
			if ( flag_print ) printf("Table: check_list(): set_scannext.size() != set_scanpre.size(), error\n");
			return 6;
		}
		
		//检查set中的元素是不是相同
		if ( !(set_validate == set_scannext) )
		{
			if ( flag_print ) printf("Table: check_list(): set_validate != set_scannext, error\n");
			return 7;
		}
		
		if ( !(set_validate == set_scanpre) )
		{
			if ( flag_print ) printf("Table: check_list(): set_validate != set_scanpre, error\n");
			return 8;
		}
		
		if ( !(set_scannext == set_scanpre) )
		{
			if ( flag_print ) printf("Table: check_list(): set_scannext != set_scanpre, error\n");
			return 9;
		}
		
		return 0;
	}
	
};


template <class T> class TableMgr : public BasicTableMgr <T>
{
protected:
	int*		pHash[MAX_NUMOF_KEY_TYPE];
	void 		(*fpBuildKey[MAX_NUMOF_KEY_TYPE])(T& data, char* key);
	int		maxNumOfHashEntry;
	int		numOfKeyType;

	//by zhulei, 2003-11-14
	int ReleaseOne(int index)
	{
		logTableError(BasicTableMgr<T>::whichSem, "Don't call TableMgr::ReleaseOne(%d) in your code!\n", index);
		return 1;	
	}

public:
	TableMgr() {}
	TableMgr(int nsem, T* pdata, unsigned char* pbitmap, int maxdata, int maxhashentry, int numkeytype, ...)
		:BasicTableMgr <T> (nsem, pdata, pbitmap, maxdata),
		 maxNumOfHashEntry(maxhashentry), numOfKeyType(numkeytype)
		 {
			int i;
			va_list marker;
			va_start(marker,numkeytype);
			for (i=0; i<numOfKeyType; i++){
				pHash[i] = va_arg(marker,int*);
			}
			for (i=0; i<numOfKeyType; i++){
				fpBuildKey[i] = (void(*)(T& data,char* key))va_arg(marker,void*);
			}
			va_end(marker);
		}
		 
	virtual ~TableMgr() {}

	int GetMaxConflict(int whichKey = 0)
	{
		int max = 0, cur = 0;

		if (BasicTableMgr<T>::GetUsedCount() == 0) {
			return 0;
		}

		for (int i=0; i<maxNumOfHashEntry; i++) {
			int index;
			if ((index = pHash[whichKey][i]) == 0) {
				continue;
			}
			cur = 0;
			while (index = BasicTableMgr<T>::pData[index].next[whichKey]) {
				cur++;
			}
			if (cur > max) {
				max = cur;
			}
		}
		return max;
	}
	
	int GetTotalConflict(int whichKey = 0)
	{
		int max = 0;

		if (BasicTableMgr<T>::GetUsedCount() == 0) {
			return 0;
		}

		for (int i=0; i<maxNumOfHashEntry; i++) {
			int index;
			if ((index = pHash[whichKey][i]) == 0) {
				continue;
			}
			while (index = BasicTableMgr<T>::pData[index].next[whichKey]) {
				max++;
			}
		}
		return max;
	}
	
	void Init(int nsem, T* pdata, unsigned char* pbitmap, int maxdata, int maxhashentry, int numkeytype, ...){
		
		int i;
		va_list marker;
		BasicTableMgr <T> ::Init(nsem, pdata, pbitmap, maxdata);
		
		maxNumOfHashEntry = maxhashentry;
		numOfKeyType = numkeytype;
		va_start(marker,numkeytype);
		for (i=0; i<numOfKeyType; i++){
			pHash[i] = va_arg(marker,int*);
		}
		for (i=0; i<numOfKeyType; i++){
			fpBuildKey[i] = (void(*)(T& data,char* key))va_arg(marker,void*);
		}
		va_end(marker);
	}
	
	void Init(int* pcnt, int nsem, T* pdata, unsigned char* pbitmap, int maxdata, int maxhashentry, int numkeytype, ...){
		
		int i;
		va_list marker;
		BasicTableMgr <T> ::Init(pcnt, nsem, pdata, pbitmap, maxdata);
		
		maxNumOfHashEntry = maxhashentry;
		numOfKeyType = numkeytype;
		va_start(marker,numkeytype);
		for (i=0; i<numOfKeyType; i++){
			pHash[i] = va_arg(marker,int*);
		}
		for (i=0; i<numOfKeyType; i++){
			fpBuildKey[i] = (void(*)(T& data,char* key))va_arg(marker,void*);
		}
		va_end(marker);
	}
	
	int GetKey(int index, char* key, int whichkeytype=0){
		if (whichkeytype >= numOfKeyType || !BasicTableMgr<T>::Validate(index))
			return 0;
		
		(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[index],key);
		return 1;
	}
	
	//add by TANG 20041026
	int GetKey(T& data, char* key, int whichkeytype = 0)
	{
		if ( whichkeytype >= numOfKeyType ) return 0;
		
		(*fpBuildKey[whichkeytype])(data, key);
		return 1;
	}
	
	size_t hashval(const char* key){
		size_t h = 0; 
	
		for ( ; *key; ++key){
			h = 37 * h +  *key;
		}
		
		return (h);
	}
	
	size_t getentry(size_t h){
		return (h % maxNumOfHashEntry);
	}
	
	size_t hashfun(const char* key){
		size_t h = 0;
	
		h = hashval(key);
		h = getentry(h);
	
		return (h);
	}
	
	//this function will not affect pData, only modify newdata, added by Tang
	//Begin,
	//Modified by zhulei
	//Add "virtual" key word in front of the member function to set it be a 
	//virtual member function.
	//2001.06.17
	//End.
	virtual int init_classT(T& pData, T& newdata){
		//reserve system data defined in User Application Struct
		newdata.ipre  = pData.ipre;
		newdata.inext = pData.inext;
		for (int i=0; i<numOfKeyType; i++) { newdata.next[i] = pData.next[i]; }	
		
		return 1;
	}
	
private:
	int	_insert(int index, int whichkeytype){
		char key[1024]={0};
		int nodeindex,i;
		
		if (whichkeytype >= numOfKeyType){
			//printf("whichkeytype(%d) >= numOfKeyType(%d)\n",whichkeytype,numOfKeyType);
			return NULLNODEID;
		}
		(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[index],key);
		if ( key==NULL || *key==0 ) return 0;	//TANG
		nodeindex = hashfun(key);
		//printf("pHash[%d][%d] = %d, %u\n", whichkeytype, nodeindex, pHash[whichkeytype][nodeindex], &(pHash[whichkeytype][nodeindex]));


		if ((i = pHash[whichkeytype][nodeindex]) == NULLNODEID){
			pHash[whichkeytype][nodeindex] = index;
		}
		else{
			//commented by TANG 2004.07.08
			//printf("\n\n  ***  INFOMATION: HASH TABLE collision, key=%s, NOT error.  ***\n", key);
			//printf("nodeindex = %d, pHash[%d][%d]=%d ***\n\n", nodeindex, whichkeytype, nodeindex, pHash[whichkeytype][nodeindex]);
			while (BasicTableMgr<T>::pData[i].next[whichkeytype] != NULLNODEID){
				i = BasicTableMgr<T>::pData[i].next[whichkeytype];
			}
			BasicTableMgr<T>::pData[i].next[whichkeytype] = index;
		}
		return 1;
	}

public:	
	//for one primary key table, one record for one key
	int	Select(char* key, int whichkeytype = 0){
		char	tempkey[1024] = {0};
		int nodeindex,i;
		
		if ( key==NULL || *key==0 ) return 0;	//TANG
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); 
#endif
		
		nodeindex = hashfun(key);
		i = pHash[whichkeytype][nodeindex];
			
		int loopcount = 0;
		while (i != NULLNODEID) {	
			memset(tempkey, 0, sizeof(tempkey));
			(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[i],tempkey);
			if (strcmp(tempkey, key) == 0) break;
			i = BasicTableMgr<T>::pData[i].next[whichkeytype];
			
			loopcount++;							//TANG 0306
			if ( loopcount > BasicTableMgr<T>::maxNumOfData ){
				logTableError(BasicTableMgr<T>::whichSem, "TableMgr.h: Select() loop error, i = %d, whichSem = %d, key = '%s', whichkeytype = %d\n", i, BasicTableMgr<T>::whichSem, key, whichkeytype);
				//the two lines added by yinhaifeng in 2003.11.14
				//logTableError(whichSem,"rebuildKey should be used.\n");
				//rebuildKey(whichkeytype);
				i = 0;		//Zhu Lei 20031114
				break;
			}
		}

#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
		
		return i;
	}
	
	//the key is NOT primary key, several records for one key
	int	SelectNext(char* key, int index, int whichkeytype = 0){
		char	tempkey[1024];
		int nodeindex,i;
		
		if ( key==NULL || *key==0 ) return 0;	//TANG
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); 
#endif
		
		//Add by TANG 0410
		//maybe some record will be selected more than once
		if ( index ){
			if ( !BasicTableMgr<T>::Validate(index) ){ index = 0; }		//pData[index] been deleted!
			else{
				memset(tempkey, 0, sizeof(tempkey));
				(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[index], tempkey);
				if (strcmp(tempkey, key) ) index = 0;	//pData[index] different to key
			}
		}
		//Add end
		
		if ( index == NULLNODEID ){	//when index==NULLNODEID, will find the first record
			nodeindex = hashfun(key);
			i = pHash[whichkeytype][nodeindex];
		}else{
			i = BasicTableMgr<T>::pData[index].next[whichkeytype];
		}
		
		int loopcount = 0;
		while (i != NULLNODEID) {	
			memset(tempkey, 0, sizeof(tempkey));
			(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[i],tempkey);
			if (strcmp(tempkey, key) == 0) break;
			i = BasicTableMgr<T>::pData[i].next[whichkeytype];
			
			loopcount++;							//TANG 0306
			if ( loopcount > BasicTableMgr<T>::maxNumOfData ){
				logTableError(BasicTableMgr<T>::whichSem, "SelectNext() loop error, i = %d, key = '%s'\n", i, key);
				//the three lines added by yinhaifeng in 2003.11.14
				//logTableError(whichSem,"rebuildKey should be used.\n");
				//rebuildKey(whichkeytype);
				i = 0;//the 0 should be returned.
				break;
			}
		}
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
			
		return i;
	}

	//primary key contains two keys, one record for one primary key
	int	Select(char* key, int whichkeytype, char* key2, int whichkeytype2){
		char	tempkey[1024], tempkey2[1024];
		int nodeindex,i;
		
		if ( key==NULL || *key==0 || key2==NULL || *key2==0 ) return 0;	//TANG
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); 
#endif
		
		nodeindex = hashfun(key);
		i = pHash[whichkeytype][nodeindex];
			
		while (i != NULLNODEID) {	
			memset(tempkey, 0, sizeof(tempkey));
			memset(tempkey2, 0, sizeof(tempkey2));
			(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[i],tempkey);
			(*fpBuildKey[whichkeytype2])(BasicTableMgr<T>::pData[i],tempkey2);
			if ( (strcmp(tempkey2, key2) == 0) && (strcmp(tempkey, key) == 0) ) break;
			i = BasicTableMgr<T>::pData[i].next[whichkeytype];
		}

#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
		
		return i;
	}

	//Updated by LXG
	//Select(...) and SelectNext(...) have no key number's limit
#ifndef CORBA_MGCP_KEY_LEN
	int	Select(char keys[][1024], int keyTypes[], int keyNum)
#else
	int	Select(char keys[][CORBA_MGCP_KEY_LEN], int keyTypes[], int keyNum)
#endif
	{
#ifndef CORBA_MGCP_KEY_LEN
		char tempKey[1024];
#else
		char tempKey[CORBA_MGCP_KEY_LEN];
#endif
		int nodeIndex, index;
		bool isProperIndex;
		
		int i;
		for(i = 0; i < keyNum; i++)
		{
			if(*keys[i] = 0)return 0;
		}
		
		nodeIndex = hashfun(keys[0]);
		index = pHash[keyTypes[0]][nodeIndex];
			
		while (index != NULLNODEID)
		{
			isProperIndex = 1;
			for(i = 0; i < keyNum; i++)
			{
				(*fpBuildKey[keyTypes[i]])(BasicTableMgr<T>::pData[index],tempKey);
				if(strcmp(tempKey, keys[i]) != 0)
				{
					isProperIndex = 0;
					break;
				}
			}
			if(isProperIndex) break;
			else index = BasicTableMgr<T>::pData[index].next[keyTypes[0]];
		}
		return index;
	}
	
#ifndef CORBA_MGCP_KEY_LEN
	int	SelectNext(char keys[][1024], int keyTypes[], int keyNum, int preIndex)
#else
	int	SelectNext(char keys[][CORBA_MGCP_KEY_LEN], int keyTypes[], int keyNum, int preIndex)
#endif
	{
#ifndef CORBA_MGCP_KEY_LEN
		char tempKey[1024];
#else
		char tempKey[CORBA_MGCP_KEY_LEN];
#endif
		int nodeIndex, index;
		bool isProperIndex;

		int i;
		for(i = 0; i < keyNum; i++)
		{
			if(*keys[i] = 0)return 0;
		}
		
		if(preIndex  == NULLNODEID )	//when preIndex==NULLNODEID, will find the first record
		{
			nodeIndex = hashfun(keys[0]);
			index = pHash[keyTypes[0]][nodeIndex];
		}
		else
		{
			index = BasicTableMgr<T>::pData[preIndex].next[keyTypes[0]];;
		}
					
		while (index != NULLNODEID)
		{
			isProperIndex = 1;
			for(i = 0; i < keyNum; i++)
			{
				(*fpBuildKey[keyTypes[i]])(BasicTableMgr<T>::pData[index],tempKey);
				if(strcmp(tempKey, keys[i]) != 0)
				{
					isProperIndex = 0;
					break;
				}
			}
			if(isProperIndex) break;
			else index = BasicTableMgr<T>::pData[index].next[keyTypes[0]];
		}
		return index;
	}
	//End updated by LXG


	//select a prefix
	//such as: "6307" is a primary key in a table, I using "63071371" can find that record.
	int	SelectPrefix(char* tkey, int whichkeytype = 0){
		char	tempkey[1024] = {0};
		char	key[1024] = {0};
		int 	nodeindex,i,j;
		
		if ( tkey==NULL || *tkey==0 ) return 0;	//TANG
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); 
#endif
		
		strncpy(key, tkey, sizeof(key)-1);
		for ( j=strlen(tkey); j>0; j-- ){
			key[j]=0;
			nodeindex = hashfun(key);
			i = pHash[whichkeytype][nodeindex];
				
			while (i != NULLNODEID) {	
				memset(tempkey, 0, sizeof(tempkey));
				(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[i],tempkey);
				//if (strcmp(tempkey, key) == 0) break;
				if (strcmp(tempkey, key) == 0) {
#ifdef __WIN32__

#else
					BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
					return i;		//TANG
				}
				i = BasicTableMgr<T>::pData[i].next[whichkeytype];
			}
		}

#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
		
		return NULLNODEID;
	}
	
private:
	int	_delete(int index, int whichkeytype){
		int nodeindex,i;
		char key[1024] = {0};
		
		if ( index==0 ) return NULLNODEID;
		
		(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[index],key);
		
		if ( key[0] == 0 ) return 1;	//TANG
		
		nodeindex = hashfun(key);

		i = pHash[whichkeytype][nodeindex];

		if (i == index){
			pHash[whichkeytype][nodeindex] = BasicTableMgr<T>::pData[i].next[whichkeytype];
		}
		else{
			while (BasicTableMgr<T>::pData[i].next[whichkeytype] != index && BasicTableMgr<T>::pData[i].next[whichkeytype] != NULLNODEID)
				i = BasicTableMgr<T>::pData[i].next[whichkeytype];
			
			if (BasicTableMgr<T>::pData[i].next[whichkeytype] != NULLNODEID)
				BasicTableMgr<T>::pData[i].next[whichkeytype] = BasicTableMgr<T>::pData[index].next[whichkeytype];
			else {
				logTableError(BasicTableMgr<T>::whichSem, "_delete(): error here, whichkeytype = %d,key = '%s'\n", whichkeytype,key);
				return NULLNODEID;
			}
		}
		
		BasicTableMgr<T>::pData[index].next[whichkeytype] = 0;
		return 1;
	}

public:
	
	/*
	//Remarked by zhulei, 2003-11-17
	//You must call AllocateOne() to get index, then put your data (including the key) 
	//into pData[index], then call this function
	//If return 0, Insert has released the data buffer. By zhulei, 2003-11-14
	int Insert(int index){
		int i, result=0;
		
		if (index == 0) return 0;

#ifdef __WIN32__

#else
		m_allocator.sem.P(whichSem); //Added by zhulei, 2001.09.28
#endif
		
		for (i=0; i<numOfKeyType; i++){
			result+=_insert(index, i);
		}
		
		if (!result)
			BasicTableMgr<T>::ReleaseOne(index, 0);

#ifdef __WIN32__

#else
		m_allocator.sem.V(whichSem); //Added by zhulei, 2001.09.28
#endif
		
		return ((result) ? index : 0);	//TANG
	}
	*/

	//You must use a struct as the parameter and the struct must including the key
	//You don't need to call AllocateOne() and ReleaseOne() yourself
	int Insert(T& tempdata){
		int index, result=0, i;
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28
#endif
		
		if ( (index = BasicTableMgr<T>::AllocateOne(0, 0)) == NULLNODEID ) {
#ifdef __WIN32__

#else
			BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
			//printf("TABLE[%d] AllocateOne retrun 0\n",whichSem);
			return NULLNODEID;
		}
		

		Update(index, tempdata, 0);
		
		for (i=0; i<numOfKeyType; i++){
			result+=_insert(index, i);
		}
		
		if ( result == 0 ) {	//TANG
			BasicTableMgr<T>::ReleaseOne(index, 0);
#ifdef __WIN32__

#else
			BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28
#endif
			
			//printf("TABLE[%d] numOfKeyType(%d) All _insert retrun 0\n",whichSem,numOfKeyType);
			return 0;
		}
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28
#endif
		return index;
	}

	//Added by zhulei, 2002.05.22
	//insert the struct tempdata into a specified place pData[index]
	virtual int Insert_r(int index, T& tempdata) {
		int result=0, i;

#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28
#endif
		
		if ( BasicTableMgr<T>::AllocateOne(index, 0) == NULLNODEID ) {
#ifdef __WIN32__

#else
			BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
			return NULLNODEID;
		}


		Update(index, tempdata, 0);
		
		for (i=0; i<numOfKeyType; i++){
			result+=_insert(index, i);
		}
		
		if ( result == 0 ) {	//TANG
			BasicTableMgr<T>::ReleaseOne(index, 0);
#ifdef __WIN32__

#else
			BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28
#endif
			
			return 0;
		}
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28
#endif
		return index;
	}

	
	int Delete(int index){	//when return 0, something wrong, maybe memory not free
		int i;
		
		if ( !BasicTableMgr<T>::Validate(index) ) return 0;
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28 
#endif
		
		//TANG 2003.11.27
		if ( !BasicTableMgr<T>::Validate(index) ) {
#ifdef __WIN32__

#else
			BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem);
#endif
			return 0;
		}

		for (i=0; i<numOfKeyType; i++){
			if (_delete(index, i) == NULLNODEID){
				
				//remarked by zhulei, 2003-11-14
#ifdef __WIN32__

#else
				//m_allocator.sem.V(whichSem); //Added by zhulei, 2001.09.28 
#endif
				
				logTableError(BasicTableMgr<T>::whichSem, "_delete(%d,%d) error, Memory not free.\n", index, i);
				
				//remarked by zhulei, 2003-11-14
				//return NULLNODEID;	
			}
		}
		
		BasicTableMgr<T>::ReleaseOne(index, 0);
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28 
#endif
		
		return 1;
	}
	
	//update one key
	//pData[index] must be equal to datanew except the KEY value which will be updated
	//1. you must use Select() to get index
	//2. then datanew=pData[index]	(pData[index] is NOT changed)
	//3. datanew.somekeyfield = newvalue
	//4. UpdateKey(...)
	int UpdateKey(int index, T& datanew, int whichkeytype = 0){	//TANG
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28 
#endif
		
		if ( _delete(index, whichkeytype)==0 ) {
#ifdef __WIN32__

#else
			BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem);
#endif
			return 0;
		}
		
		Update(index, datanew, 0);
		////init_classT(pData[index], datanew);
		////pData[index] = datanew;
		if ( _insert(index, whichkeytype)==0 ) {
#ifdef __WIN32__

#else
			BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem);
#endif
			return 0;
		}

#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28 
#endif
		return 1;
	}

	//update some keys
	//the 3th parameter numkeytype refers to how many keys you want to update
	//pData[index] must be equal to datanew except the KEY value which will be updated
	//1. you must use Select() to get index
	//2. then datanew=pData[index]	(pData[index] is NOT changed)
	//3. datanew.somekeyfield1 = newKEYvalue1
	//4. datanew.somekeyfield2 = newKEYvalue2
	//5. ...
	//6. UpdateKeyS(...)
	//7. if the parameter numkeytype equals TableMgr::numKeyType, 
	//   caller will not have to fill the parameter list with 
	//   each key type (0, 1, 2, ...), in fact, even if caller
	//   fills those parameter, they will be omited.
	int UpdateKeyS(int index, T& datanew, int numkeytype, ...){
		int i;
		int whichkeytype[MAX_NUMOF_KEY_TYPE];
		
		if (index == 0) return 0;
		
		if (numkeytype == numOfKeyType) {
			for (i=0; i<numOfKeyType; i++){
				whichkeytype[i] = i;
			}
		}
		else {
			va_list marker;
			va_start(marker,numkeytype);
			for (i=0; i<numkeytype; i++){
				whichkeytype[i] = va_arg(marker,int);
			}
			va_end(marker);
		}	
			
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28 
#endif
		
		for ( i=0; i<numkeytype; i++ ){		
			if ( _delete(index, whichkeytype[i])==0 ) {
#ifdef __WIN32__

#else
				BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem);
#endif
				return 0;
			}
		}
		
		Update(index, datanew, 0);
		////init_classT(pData[index], datanew);
		////pData[index] = datanew;

		for ( i=0; i<numkeytype; i++ ){		
			if ( _insert(index, whichkeytype[i])==0 ) {
#ifdef __WIN32__

#else
				BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem);
#endif
				return 0;
			}
		}		
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); //Added by zhulei, 2001.09.28 
#endif
		return 1;
	}

		
	//when you update a record, call this function
	int	SetDirty(int index) {
		int pos;
		int bit;
		unsigned char mask = 0x80;
		unsigned char* mapptr = BasicTableMgr<T>::pBitmap + BasicTableMgr<T>::maxNumOfData/8;
		
		if ( index<1 || index>=BasicTableMgr<T>::maxNumOfData ) return 0;
		
		pos = index / 8;
		bit = index % 8;
	
		mapptr[pos] |= mask >> (bit); 
		return 1;
	}
	
	int ClearDirty(int index) {
		int pos;
		int bit;
		unsigned char mask = 0x80;
		unsigned char* mapptr = BasicTableMgr<T>::pBitmap + BasicTableMgr<T>::maxNumOfData/8;
		
		if ( index<1 || index>=BasicTableMgr<T>::maxNumOfData ) return 0;
		
		pos = index / 8;
		bit = index % 8;
		
		mapptr[pos] &= ~(mask>>(bit)); 
		return 1;
	}
	
	int CheckDirty(int index) {
		int pos;
		int bit;
		unsigned char mask = 0x80;
		unsigned char* mapptr = BasicTableMgr<T>::pBitmap + BasicTableMgr<T>::maxNumOfData/8;
		
		if ( !BasicTableMgr<T>::Validate(index) ) {
			ClearDirty(index);
			return 0;
		}
		
		if ( index<1 || index>=BasicTableMgr<T>::maxNumOfData ) return 0;
		
		pos = index / 8;
		bit = index % 8;
	
		return ((mapptr[pos] & (mask >> (bit))) != 0x00);
	}

	//Begin,
	//Added by zhulei, for update data with input parameter.
	//T.ipre, T.inext and T.next[] will be reserved.
	//2001.06.17
	//Parameters:
	//	int index -- index of the record to be updated
	//	T& newdata   -- source data
	virtual int Update(int index, T& newdata, int pvflag = 1) {
		if ( !BasicTableMgr<T>::Validate(index) ) return 0;
		
#ifdef __WIN32__

#else
		if (pvflag) BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem);
#endif
		
		init_classT(BasicTableMgr<T>::pData[index], newdata);
		memcpy(&BasicTableMgr<T>::pData[index], &newdata, sizeof(T));
		
#ifdef __WIN32__

#else
		if (pvflag) BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem);
#endif
		return 1;
	}
	//End.

	//Begin,
	//Added by TANG, for update data with input parameter.
	//T.ipre, T.inext and T.next[] will be reserved.
	//2002.07.19
	//Parameters:
	//	int index -- index of the record to be updated
	//	T& newdata   -- source data
	virtual int Copy(int index, int oldconst) {
		T	newdata;
		
		if ( !BasicTableMgr<T>::Validate(index) || !BasicTableMgr<T>::Validate(oldconst) ) return 0;
		
		memcpy(&newdata, &BasicTableMgr<T>::pData[oldconst], sizeof(T));
		
		Update(index, newdata, 1);
		
		return 1;
	}
	//End.

	virtual int	ReleaseAll() {
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem);
#endif
		memset(BasicTableMgr<T>::pData, 0, sizeof(T)*BasicTableMgr<T>::maxNumOfData);
		memset(BasicTableMgr<T>::pBitmap, 0, BasicTableMgr<T>::maxNumOfData/4); 
		BasicTableMgr<T>::pBitmap[0] = 0x80;	
		if (BasicTableMgr<T>::pCnt) (*BasicTableMgr<T>::pCnt) = 0;
		for (int i=0; i<numOfKeyType; i++) {
			memset(pHash[i], 0, sizeof(int)*maxNumOfHashEntry);
		}
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem);
#endif
		return 1;
	}

	/*
		this function is added by yinhaifeng for rebuid the key in 2003.10.23
		you can rebuild the whichkeytype key in any time when you load this 
	function.
		return value : always return 1;
	*/
	int rebuildKey(int whichkeytype)
	{
		int nodeindex,index,i;
		char key[1024]={0};
		
		memset(key,0,1024);
		if(whichkeytype < 0){
			return NULLNODEID;
		}
	
		if (whichkeytype >= numOfKeyType){
			return NULLNODEID;
		}
		
		int *pmyNext = (int *)malloc(sizeof(int)*BasicTableMgr<T>::maxNumOfData);
		int *pmyHash = (int *)malloc(sizeof(int)*maxNumOfHashEntry);
		memset(pmyNext,0,sizeof(int)*BasicTableMgr<T>::maxNumOfData);
		memset(pmyHash,0,sizeof(int)*maxNumOfHashEntry);
		
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.P(BasicTableMgr<T>::whichSem); 
#endif
		
		for(index = BasicTableMgr<T>::ScanNext(0); index>0;index = BasicTableMgr<T>::ScanNext(index)){

			(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[index],key);
			if ( key==NULL || *key==0 ) {
				continue;
			}
			nodeindex = hashfun(key);
			//printf("index = %d,nodeindex=%d,key = %s\n",index,nodeindex,key);
			if ((i = pmyHash[nodeindex]) == NULLNODEID){
				pmyHash[nodeindex] = index;
			}
			else{
				while (pmyNext[i] != NULLNODEID){
					i = pmyNext[i];
				}
				pmyNext[i] = index;
			}
		}
		
		for(index = BasicTableMgr<T>::ScanNext(0); index>0;index = BasicTableMgr<T>::ScanNext(index)){
			BasicTableMgr<T>::pData[index].next[whichkeytype] = pmyNext[index];
		}
		for(index = 0; index < maxNumOfHashEntry; index ++){
			pHash[whichkeytype][index] = pmyHash[index];
		}
#ifdef __WIN32__

#else
		BasicTableMgr<T>::m_allocator.sem.V(BasicTableMgr<T>::whichSem); 
#endif
		
		free(pmyHash);
		free(pmyNext);
		
		return 1;
	}
	
	/*
		1. all the hash value and the next value should be validate dataindex.
		2. all the dataindex should be in the hash value and the next value.
		3. all the key that beuild by itself should be select back.
		return value:	1 -- all the data is right
		return value:	0 -- some data is error rebuild should be called.
	*/
	int checkHashKey(int whichkeytype)
	{
		int retValue;
		retValue = 1;
		int *pmyBitMap = (int *)malloc(sizeof(int)*BasicTableMgr<T>::maxNumOfData);
		memset(pmyBitMap,0,sizeof(int)*BasicTableMgr<T>::maxNumOfData);


		if(whichkeytype < 0){
			return NULLNODEID;
		}
	
		if (whichkeytype >= numOfKeyType){
			return NULLNODEID;
		}

		for(int index=0; index<maxNumOfHashEntry; index++){
			if(0 == pHash[whichkeytype][index]){
				continue;
			}
			int hashIndex = pHash[whichkeytype][index];
			while(hashIndex){
				//printf("hashIndex[%d] = %d\n",index,hashIndex);
				if(!BasicTableMgr<T>::Validate(hashIndex)){
					//1. all the hash value and the next value should be validate dataindex.
					printf("1 error 1 -- hashIndex(%d).\n",hashIndex);
					retValue = 0;
					break;
				}
				pmyBitMap[hashIndex] = 1;
				hashIndex = BasicTableMgr<T>::pData[hashIndex].next[whichkeytype];
			}
		}
		//printf("1 test\n" );
		for(int i = i; i< BasicTableMgr<T>::maxNumOfData;i++){
			if(BasicTableMgr<T>::Validate(i)){
				//printf("2 pmyBitMap[%d] = %d\n",i,pmyBitMap[i]);
				if(pmyBitMap[i] == 0){
					printf("error 2. i(%d)\n",i);
					//2. all the dataindex should be in the hash value and the next value.
					retValue = 0;
				}
			}
		}
		//printf("2 test\n" );
		
		char key[1024];
		memset(key,0,1024);
		for(int index = BasicTableMgr<T>::ScanNext(0); index>0;index = BasicTableMgr<T>::ScanNext(index)){
			memset(key,0,1024);
			(*fpBuildKey[whichkeytype])(BasicTableMgr<T>::pData[index],key);
			if ( key==NULL || *key==0 ) {
				printf("error 3.\n");
				retValue = 0;
			}
			//printf("3 index = %d,key = %s\n",index,key);
			int p = Select(key,whichkeytype);
			while(p != index && BasicTableMgr<T>::Validate(p)){
				p = SelectNext(key,p,whichkeytype);
			}
			
			if(!BasicTableMgr<T>::Validate(p)){
				printf("error 4 index(%d) p(%d).\n",index,p);
				retValue = 0;
			}
		}
		//printf("3 test\n" );
		return retValue;
	}
	
	
	//TANG 20070821
	int check_hash_key(int flag_print = 0)
	{
		struct timeval tv_sleep;
		
		char tmpkey[1024] = {0};
		
		for ( int index=1; index<BasicTableMgr<T>::maxNumOfData; index++ )
		{
			if ( BasicTableMgr<T>::Validate(index) )
			{
				if ( flag_print ) printf("Table: check_hash_key(): table[%d]\n", index);
				
				//检查该条记录的各个key是否有效
				for ( int i=0; i<numOfKeyType; i++ )
				{
					//构造key
					memset(tmpkey, 0, sizeof(tmpkey));
					GetKey(index, tmpkey, i);
					
					if ( tmpkey[0] == 0 )
					{
						if ( flag_print ) printf("Table: check_hash_key(): table[%d].key[%d]='%s', ignore\n", index, i, tmpkey);
						continue;
					}
					
					int keyok = 0;
					
					int loopcount = 0;
					
					//根据key查找
					int tryindex = 0;
					while ( (tryindex=SelectNext(tmpkey, tryindex, i)) )
					{
						if ( index == tryindex )
						{
							//能够根据key找到记录，说明记录有效
							keyok = 1;
							if ( flag_print ) printf("Table: check_hash_key(): table[%d].key[%d]='%s', ok\n", index, i, tmpkey);
							break;
						}
						else
						{
							//if ( flag_print ) printf("Table: check_hash_key(): try table[%d]\n", tryindex);
						}
						
						loopcount++;
						if ( loopcount > BasicTableMgr<T>::maxNumOfData )
						{
							printf("Table: check_hash_key(): table[%d].key[%d]='%s', SelectNext() loop error\n", index, i, tmpkey);
							break;
						}
					}
					
					if ( keyok == 0 )
					{
						//根据key找不到记录，说明内存有问题
						if ( flag_print ) printf("Table: check_hash_key(): table[%d].key[%d]='%s', error\n", index, i, tmpkey);
						return index;
					}
				}
				
				//TANG 20070906
				//usleep()似乎有时候会使得进程阻塞，而不会恢复运行，问题尚不确定
				//usleep(1);
				tv_sleep.tv_sec = 0;
				tv_sleep.tv_usec = 20*1000;
				select(0, NULL, NULL, NULL, &tv_sleep);
			}
		}
		
		return 0;
	}
	
};

#endif //__TABLEMGR_T__


