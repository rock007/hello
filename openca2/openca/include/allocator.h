/*************************************************************************
Only one monitor program:
	Allocator::SetSem(char * keypath, char fsemid='o', int nsem);
	Allocator::sem_Create();
	...
	Allocator::sem_Release();

Other program, there are three ways to use this class: ( 3 is recommended )
1.	Allocator someobject(int size, char * mp, int cur=0);
	...
	Allocator::SetSem(char * keypath, char fsemid='o');
	Allocator::sem_Attach();

2.	Allocator someobject(char * keypath, char fsemid='o');
	...
	someobject.init(int size, char * mp, int cur=0);
	
3.	Allocator someobject;
	...
	Allocator::SetSem(char * keypath, char fsemid='o');
	Allocator::sem_Attach();
	someobject.init(int size, char * mp, int cur=0);
*************************************************************************/
#ifndef ALLOCATOR_HXX_SIP_
#define ALLOCATOR_HXX_SIP_

#ifdef __WIN32__

#else
	#include "csem.h"
#endif


class Allocator
{
public:
	Allocator() {}
	Allocator(char * keypath, char fsemid='o', int nsem=1) {	//for compatibility
#ifdef __WIN32__

#else
		Allocator::sem = CSem(keypath, fsemid, nsem);
#endif
	}
	Allocator(int size, char * mp, int cur=0)
	{
		mapsize = size;	//mapsize is char array size
		mapptr 	= mp;
		curpos 	= cur;
		mapptr[0] |= 0x80;
	}
	~Allocator() {}

	int init(int size, char * mp, int cur=0)	//for compatibility
	{
		mapsize = size;	//mapsize is char array size
		mapptr 	= mp;
		curpos 	= cur;
		mapptr[0] |= 0x80;
#ifdef __WIN32__
		return 1;
#else
		return sem.Attach();
#endif
	}
	
#ifdef __WIN32__

#else
	static int sem_Set_Sem_Val(int snum, int val) { return sem.Set_Sem_Val(snum, val); }
	static int sem_Create() { return sem.Create(); }
	static int sem_Attach() { return sem.Attach(); }
	static int sem_Release() { return sem.Release(); }
	static int sem_IsExist() { return sem.IsExist(); }
	static int sem_GetSem(int snum, int *val){sem.Attach(); return sem.Get_Sem(snum, val);}
    static int sem_GetIPC_Id() { return sem.GetIPC_Id(); }
	static void SetSem(char * keypath, char fsemid='o', int nsem=1){
		Allocator::sem = CSem(keypath, fsemid, nsem);
	}
#endif
	
	int allocone(int semnum=0);	// return 0 means error
	int freeone2(int index);
	//Begin,
	//This function is added by zhulei, to force Allocator to 
	//allocate a specified record.
	int allocone(int index, int semnum);	// return 0 means error
	//End.

#ifdef __WIN32__

#else
	static 	CSem sem;
#endif
	
protected:
	char 	*mapptr;
	int 	mapsize;
	int 	curpos;
};

#endif // ALLOCATOR_HXX


