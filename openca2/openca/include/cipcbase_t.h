#ifndef _CIPCBASE_T__
#define _CIPCBASE_T__

#include <string.h>

#define CIPCBASE_T_MAX_PATH 	128

class CIPCBase_t
{
public:
	CIPCBase_t();
	CIPCBase_t(char* kpath,int id,int af);
	virtual ~CIPCBase_t() {}

protected:
	char KeyPath[CIPCBASE_T_MAX_PATH];
	char KeyId;
	int AccessFlag;
	int errorcode;
	int IPC_Id;

public:
	char* GetKeyPath() const;
	char GetKeyId() const;
	int GetAccessFlag() const;
	int GetErrorCode() const;
	
	int SetKeyPath(char* kpath);
	int SetKeyId(char keyid);
	int SetAccessFlag(int af);

	int GetIPC_Id() const;
	
	virtual int Create()=0;
	virtual int Attach(int flag_recreate = 0)=0;
	virtual int Release()=0;
	virtual int Dettach()=0;
};

#endif
