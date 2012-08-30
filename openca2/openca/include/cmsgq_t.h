#ifndef __CMSGQ_H__
#define __CMSGQ_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <string.h>
#include <errno.h>
#include "cipcbase_t.h"

#define CMSGQ_MAX_BUF		256

//calss T must be the follows:
//{
//	long   			rcv_id;			// used by ipcs library, application need not set it
//	long   			snd_id; 		// used by ipcs library, application need not set it
//	whateverStruct	whateverName; 	// application data store here, must be a struct
//}

template <class T> class CMsgQueue_t : public CIPCBase_t
{
public:
	CMsgQueue_t() {
		memset(&SndCmd,0,sizeof(T));
		memset(&RcvCmd,0,sizeof(T));
	}
	
	CMsgQueue_t(char* kpath,int id,long snderid,long rcverid=0,int af=0200|0400)
	: CIPCBase_t(kpath,id,af)
	{
		SnderId=snderid;
		RcverId=rcverid;
		memset(&SndCmd,0,sizeof(T));
		memset(&RcvCmd,0,sizeof(T));
	}
	
	virtual ~CMsgQueue_t() {}
	
private:
	long SnderId;
	long RcverId;
	
public:
	T SndCmd;
	T RcvCmd;
	
public:
	void SetKey(char* kpath,int id,long snderid,long rcverid=0,int af=0200|0400)
	{
		SnderId=snderid;
		RcverId=rcverid;
		memset(&SndCmd,0,sizeof(T));
		memset(&RcvCmd,0,sizeof(T));
		CIPCBase_t::SetKeyPath(kpath);
		CIPCBase_t::SetKeyId(id);
		CIPCBase_t::SetAccessFlag(af);
	}
	
 	long GetSnderId() const {
 		return SnderId;
 	}
 	
	long GetRcverId() const {
		return RcverId;
	}
	
	T* GetRcvMsg() {
		T* p=&RcvCmd;
		return p;
	}
	
	int GetMsgDataLen() const {
		return ( sizeof(T) - sizeof(long) - sizeof(long) );
	}
	
	int SetSnderId(long sid) {
		if (sid==0) return 0;
		SnderId=sid;
		return 1;
	}
	
	int SetRcverId(long rid) {
		if (rid==0) return 0;
		RcverId=rid;
		return 1;
	}

	virtual int Create() {
		key_t key;
		int   flag = IPC_CREAT;
	
		if (strlen(KeyPath)==0 || KeyId==0 || IPC_Id >=0)
			return 0;
	
		key = ftok(KeyPath, KeyId);
		if (key == (key_t) -1) {
			perror ("ftok at 'CMsgQueue_t::Create' error: ");
			return 0;
		}
		if ((IPC_Id = msgget(key, AccessFlag|flag)) < 0) {
			IPC_Id = msgget(key, AccessFlag);
			if (IPC_Id < 0) {
				perror ("msgget at 'CMsgQueue_t::Creaet' queue error: ");
				return 0;
			}
		}
		//printf("MSGQ id = %d\n", IPC_Id);

		return 1;
	}
	
	virtual int Attach(int flag_recreate = 0) {
		key_t key;
	
		if (strlen(KeyPath)==0 || KeyId==0 )	//|| IPC_Id >=0)	//TANG 1107
			return 0;
	
		key = ftok(KeyPath, KeyId);
		if (key == (key_t) -1) {
			perror ("ftok at 'CMsgQueue_t::Attach' error: ");
			return 0;
		}
	
		if ((IPC_Id = msgget(key, AccessFlag)) < 0) {
			perror ("msgget at 'CMsgQueue_t::Attach' queue error: ");
			return 0;
		}
		
		return 1;
	}
	
	virtual int Release() {
		
		if (IPC_Id>=0)
		{
			msgctl (IPC_Id, IPC_RMID, NULL);
			IPC_Id=-1;
		}
		return 1;
	}
	
	virtual int Dettach() {
		IPC_Id = -1;
		return 1;
	}

	//int ClearMsgQueue() {						//TANG 1119
	int ClearMsgQueue(int clearall_flag = 0) {
		char rcv_buf[CMSGQ_MAX_BUF];
		
		if ( clearall_flag ){
			while(1) {
				if(msgrcv(IPC_Id, rcv_buf, CMSGQ_MAX_BUF, 0, IPC_NOWAIT)<0) {
					if( errno != ENOMSG ) {
						errorcode = errno;
						return 0;
					}
					else {
						return 1;
					}
				}
			}
		}else{
			while(1) {
				//if(msgrcv(IPC_Id, rcv_buf, CMSGQ_MAX_BUF, RcverId, IPC_NOWAIT)<0) {
				//clear the message I send out.
				if(msgrcv(IPC_Id, rcv_buf, CMSGQ_MAX_BUF, SnderId, IPC_NOWAIT)<0) {

					if( errno != ENOMSG ) {
						errorcode = errno;
						return 0;
					}
					else {
						return 1;
					}
				}
			}
		}//End of if (flag)
	}
	
	int GetMsgNum() {
		struct msqid_ds ds;
		bzero(&ds, sizeof(struct msqid_ds));
	
		if (msgctl (IPC_Id, IPC_STAT, &ds) == -1) {
			errorcode = errno;
			return -1;
		}
		return  ds.msg_qnum;
	}

	int IsExist(){
		struct msqid_ds ds;
		bzero(&ds, sizeof(struct msqid_ds));
	
		if (msgctl (IPC_Id, IPC_STAT, &ds) == -1) {
			errorcode = errno;
			return 0;
		}else{
			return 1;
		}
	}
	
	//modified by TANG 2002.06.04
	//don't use SndCmd in this function, for it's public, not fit for multi-thread
	//MT-safe
	int SndMsg(T* buff, long rcverId=0, int wt=0) {
		size_t nbytes;
		int    flag;
		//T* p;
	
		if (IPC_Id<0) {
			printf("SndMsg:: IPC_Id = %d\n", IPC_Id);
			return 0;
		}
	
		//p=&SndCmd;
		//memcpy(p, buff, sizeof(T));	

		if (wt) {
			flag = 0;
		} else {
			flag = IPC_NOWAIT;
		}
	
		if (RcverId==0)	{
			printf("SndMsg::RcverId = %d\n", (int)RcverId);
			return 0;
		}
	
		//SndCmd.rcv_id = (rcverId)?rcverId:RcverId;
		//SndCmd.snd_id = SnderId;
		buff->rcv_id = (rcverId)?rcverId:RcverId;
		buff->snd_id = SnderId;
		nbytes = sizeof(T) - sizeof(long);
		//if (msgsnd (IPC_Id,&SndCmd , nbytes, flag) < 0) {
		if (msgsnd (IPC_Id, buff, nbytes, flag) < 0) {
			printf("msgsnd() errno=%d IPC_Id=%d\n",errno, IPC_Id);
			errorcode = errno;
			return 0;
		}
		return 1;
	}
	
	//NOT MT-safe
	int SndMsg(long rcverId = 0) {
		size_t nbytes;
		int    flag;
		
		if (IPC_Id<0 || RcverId==0 || SnderId==0)
			return 0;
	
		flag = IPC_NOWAIT;
	
		SndCmd.rcv_id = (rcverId)?rcverId:RcverId;
		SndCmd.snd_id = SnderId;
		nbytes = sizeof(T) - sizeof(long);
		if (msgsnd (IPC_Id,&SndCmd , nbytes, flag) < 0) {
			printf("msgsnd() errno=%d IPC_Id=%d\n",errno, IPC_Id);
			errorcode = errno;
			return 0;
		}
		return 1;
	}
	
	//NOT MT-safe
	int RcvMsg(long snderId=0,int wt=0) {
		int flag ;
		
		if (IPC_Id<0) return 0;
	
		if (wt) {
			flag = 0;
		} else {
			flag = IPC_NOWAIT;
		}
	
		if (SnderId==0)	return 0;

		memset(&RcvCmd,0,sizeof(T));	//TANG 2002.07.17
		if (msgrcv (IPC_Id, &RcvCmd, (sizeof(T) - sizeof(long)), (snderId)?snderId:SnderId,  flag) < 0 ) {
			errorcode = errno;
			return 0;
		}
		return 1;
	}

	//TANG 2002.07.17
	//MT-safe
	int RcvMsg(T* buff, long snderId=0, int wt=0) {
		int flag ;
		
		if (IPC_Id<0) return 0;
	
		if (wt) {
			flag = 0;
		} else {
			flag = IPC_NOWAIT;
		}
	
		if (SnderId==0)	return 0;

		memset(buff,0,sizeof(T));
		if (msgrcv (IPC_Id, buff, (sizeof(T) - sizeof(long)), (snderId)?snderId:SnderId,  flag) < 0 ) {
			errorcode = errno;
			return 0;
		}
		return 1;
	}

};

#endif
