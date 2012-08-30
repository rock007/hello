#ifndef __CShareMemory_t_H__
#define __CShareMemory_t_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <errno.h>
#include <unistd.h>
#include "cipcbase_t.h"


#define RECREATE_WHEN_ATTACH_ERROR	1	//如果shm是属于一个进程私有的，可以在Attach时指定该标记


struct cshm_header_t
{
	int shmsize;
	char nouse[60];
};


template<class T>
class CShareMemory_t : public CIPCBase_t
{
public:
	CShareMemory_t()
	{
		pData=NULL;
		shMemSize=0;
	}
	CShareMemory_t(char* keypath, int id, int size, int af=SHM_R|SHM_W)
	: CIPCBase_t(keypath,id,af)
	{
		pData=NULL;
		shMemSize = sizeof(struct cshm_header_t) + sizeof(T);	//TANG 2009-7-7 13:56
	}	
	virtual ~CShareMemory_t(){}
	
private:
	unsigned char * pData;
	int     shMemSize;
	
public:
	T * 	GetPData() { return (T *)(pData + sizeof(struct cshm_header_t)); }
	int 	GetShMemSize() const;
	int 	InitPData();
	
	virtual int Create();
	virtual int Attach(int flag_recreate = 0);
	virtual int Release();
	virtual int Dettach();
	virtual int ForceRelease();
	int	IsExist();
};


template<class T>
int CShareMemory_t<T>::GetShMemSize() const
{
	if (pData)
		return sizeof(T);
	else
		return 0;
}

template<class T>
int CShareMemory_t<T>::InitPData()
{
	memset((void*)pData, 0, shMemSize);
	return 1;
}

template<class T>
int CShareMemory_t<T>::Create()
{
	key_t	shmkey;
	int		shmflg = IPC_CREAT; 

	if (strlen(KeyPath)==0 || KeyId==0 || IPC_Id >=0||shMemSize<=0)
	{
		if ( KeyPath[0] == 0 ) printf("KeyPath=0\n");
		if ( KeyId == 0 ) printf("KeyId=0\n");
		if ( IPC_Id >= 0 ) printf("IPC_Id>=0\n");
		if ( shMemSize <= 0 ) printf("shMemSize<=0\n");
		return 0;
	}
		
	shmkey = ftok(KeyPath, KeyId);
	if (shmkey == (key_t) -1) 
	{
		perror("ftok at CShareMemory_t<T>::Create' error: ");
		errorcode = errno;
		return 0;
	}

	IPC_Id = shmget(shmkey, 0, AccessFlag|shmflg);
	//TANG 2003.07.21
	if (IPC_Id >= 0) 
	{
		printf("pid(%u): Share memory already exists, IPC_Id = %d\n", (unsigned int)getpid(), IPC_Id);
		IPC_Id=-1;
		return 0;
	}

	IPC_Id = shmget(shmkey, shMemSize, shmflg|AccessFlag);
	if (IPC_Id < 0)
	{
		perror("shmget at 'CShareMemory_t<T>::Create' error: ");
		errorcode = errno;
		return 0;
	}

	//printf("SHM IPC_Id = %d\n",IPC_Id);

	//TANG 2002.07.05
#ifdef __SunOS__
	if ((pData = (unsigned char *) shmat(IPC_Id, 0, AccessFlag|SHM_SHARE_MMU)) == (void *) -1) 
#else
	if ((pData = (unsigned char *) shmat(IPC_Id, 0, 0)) == (void *) -1) 
#endif
	{
		shmctl(IPC_Id, IPC_RMID, 0);
		perror("shmat at 'CShareMemory_t<T>::Create' error: ");
		errorcode = errno;
		pData=NULL;
		return 0;
	}

	InitPData();
	
	//TANG 2002.07.24
	struct cshm_header_t * pheader = (struct cshm_header_t *) pData;
	pheader->shmsize = sizeof(T);
	
	//printf("CShareMemory_t<> process=%u create share memory id=%d size=%d\n", (unsigned int)getpid(), IPC_Id, shMemSize);
	return 1;
}

template<class T>
int CShareMemory_t<T>::Attach(int flag_recreate)
{
	key_t shmkey;
	
	shmkey = ftok(KeyPath, KeyId);
	if (shmkey == (key_t) -1) 
	{
		perror("ftok at 'CShareMemory_t<T>::Attach' error: ");
		errorcode = errno;
		return 0;
	}

	if ((IPC_Id = shmget(shmkey, 0, AccessFlag)) < 0) 
	{
		perror("shmget at 'CShareMemory_t<T>::Attach' error: ");
		errorcode = errno;
		
		//TANG 2009-7-7 14:01
		if ( flag_recreate )
		{
			ForceRelease();
			return Create();
		}
		
		return 0;
	}

	//TANG 2002.07.05
#ifdef __SunOS__
	if ((pData = (unsigned char *) shmat(IPC_Id, 0, AccessFlag|SHM_SHARE_MMU)) == (void *) -1)
#else
	if ((pData = (unsigned char *) shmat(IPC_Id, 0, 0)) == (void *) -1)
#endif
	{
		perror("shmat at 'CShareMemory_t<T>::Attach' error: ");
		errorcode = errno;
		pData=NULL;
		
		//TANG 2009-7-7 14:01
		if ( flag_recreate )
		{
			ForceRelease();
			return Create();
		}
		
		return 0;
	}

	//TANG 2002.07.24
	struct cshm_header_t * pheader = (struct cshm_header_t *) pData;
	if ( pheader->shmsize != sizeof(T) )
	{
		printf("CShareMemory_t<T>::Attach() shm size error, IPC_Id = %d, Create size:%d, Attach size:%d. 'make rebuild' needed.\n", 
			IPC_Id, pheader->shmsize, sizeof(T));
		Dettach();
		
		//TANG 2009-7-7 14:01
		if ( flag_recreate )
		{
			ForceRelease();
			return Create();
		}
		
		return 0;
	}

	return 1;
}

template<class T>
int CShareMemory_t<T>::Release()
{
	if (IPC_Id>=0)
	{
		if (shmctl(IPC_Id, IPC_RMID, 0) < 0)
		{
			errorcode = errno;
			IPC_Id=-1;
			pData=NULL;			//TANG 0528
			return 0;
		}
		IPC_Id=-1;
		pData=NULL;
	}
	
	return 1;
}


template<class T>
int CShareMemory_t<T>::ForceRelease()
{
	key_t shmkey;
	
	shmkey = ftok(KeyPath, KeyId);
	if (shmkey == (key_t) -1) 
	{
		perror("ftok at 'CShareMemory_t<T>::Attach' error: ");
		errorcode = errno;
		return 0;
	}

	if ((IPC_Id = shmget(shmkey, 0, AccessFlag)) < 0) 
	{
		perror("shmget at 'CShareMemory_t<T>::Attach' error: ");
		errorcode = errno;
		return 0;
	}
	
	Release();
	
	return 1;
}


template<class T>
int CShareMemory_t<T>::Dettach() 
{
	if (pData != NULL) 
	{
		shmdt((char *)pData);
		pData = NULL;			//TANG 2002.06.12
	}
	//IPC_Id = -1;				//TANG 2002.06.12
	return 1;
}


template<class T>
int CShareMemory_t<T>::IsExist() 
{
	struct shmid_ds ds;
	bzero(&ds, sizeof(struct shmid_ds));

	if ( IPC_Id >= 0 )
	{							//TANG 0313
		if ( shmctl (IPC_Id, IPC_STAT, &ds) < 0 ) 		//TANG 2002.06.05
		{
			errorcode = errno;
			return 0;
		}
		else
		{
			return 1;
		}
	}
	else
	{
		printf("shm.IsExist(): IPC_Id = %d, error\n", IPC_Id);
		return 0;
	}
}

#endif

