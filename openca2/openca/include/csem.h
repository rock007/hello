#ifndef _CSEM_H__SIP_
#define _CSEM_H__SIP_

#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>

#define MAX_SEMS	256

class CSem
{
private:
	int 	SemID;
	char 	KeyPath[128];
	char 	FSemID;
	int 	nsems;
	int		semval[MAX_SEMS];
	
public:
	CSem();
	CSem(char * keypath, char fsemid='o', int nsem=1);
	~CSem();
	
	CSem& operator = (const CSem& sem)
	{
		KeyPath[0]=0;
		if (strlen(sem.KeyPath)<=128-1)
		{
			strncpy(KeyPath,sem.KeyPath,sizeof(KeyPath)-1);
			FSemID=sem.FSemID;
			nsems =sem.nsems;
		}
		for ( int i=0; i<MAX_SEMS; i++ )
		{
			semval[i] = sem.semval[i];
		}
		return *this;
	}
	
	int Create();
	int Attach();
	int Release();
	
	int IsExist();
	
	int P(int semnum);
	int V(int semnum);
	int Reset_Sem(int snum);
	int Get_Sem(int snum, int *val);
	
	int GetIPC_Id()
	{
		return SemID;
	}
	
	int Set_Sem_Val(int snum, int val);

};

#endif


