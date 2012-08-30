#ifndef ACTIVE_STANDBY_H_
#define ACTIVE_STANDBY_H_

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>

//#include <sys/stat.h>
//#include <fcntl.h>
//#include <unistd.h>

#include "ping.h"
#include "tcpbuf.h"
#include "logfile.h"


struct RongYu								//define in SHARE MEMORY
{
	int					master;				//1 for Master, 0 for Slave
	int					active;				//1 for active, 0 for standby
	
	unsigned int 		activeIP;
	unsigned int		standbyIP;
	char 				adapterName[32];
	char 				deviceName[32];
	
	unsigned int		netPublic;
	unsigned int		netPrivate;
	unsigned int		backup_netPublic;
	unsigned int		backup_netPrivate;
	
	unsigned short		master_netPublicPort;
	unsigned short		master_netPrivatePort;
	
	struct pinginfo		pingC;

	int					HBconnection;		//1 for connected, 0 for broken
	int					DATAconnection;
	
	int					adapterLineOFF;		//1 for off, 0 for ok
	
	int					forwardDataFlag;
	//forwardDataFlag==1 && active==1, then you can forward message
	
	int					flag_copy_memory_done;	//TANG 2004.03.26
};



#define STANDBY_STATE	0
#define ACTIVE_STATE	1

//when TCP1 connected, Slave will send I_AM_ACTIVE/I_AM_STANDBY, Master will answer
//if Master send "YOU_ARE_STANDBY", it must wait "SLAVE_STANDBY_CONFIRM", then "flag_tcp1_decision_ok = 1;"
#define I_AM_ACTIVE 			0x21
#define I_AM_STANDBY 			0x22
#define YOU_ARE_ACTIVE 			0x23
#define YOU_ARE_STANDBY 		0x24
#define SLAVE_STANDBY_CONFIRM	0x25		//TANG 2003.07.18

//heart bit message
#define HB_REQUEST			0x31
#define HB_RESPONSE			0x32

//user command to let active change to standby
#define GIVEUP				0x51
#define CONFIRM				0x52
#define GIVEUP_OVER			0x53
#define UP_OVER				0x54

//inform other side not to check timeout
#define NOT_CHECK_TIMEOUT	0x60		//TANG 2004.03.24
#define CHECK_TIMEOUT		0x61


#define REDUN_HB_MSG_ID					0xFF
	#define REDUN_TIME_PARAM_ID	0xFF
#define COPYMEMORY_END_MSG_ID			0xFE
#define COPYMEMORY_END_CONFIRM_MSG_ID	0xFD
#define COPYMEMORY_BEGIN_MSG_ID			0xFC
	

class ActiveStandby
{
private:
	int hme_fd;
	int state;
	struct RongYu * rongyu;
	
	int udpsockfd;			//for ioctl() to get ip address
	int sockfdPublic;
	int listenfdPublic;		//for Master public address .. net adaptor 1
	int sockfdPrivate;
	int listenfdPrivate;	//for Master private address.. net adaptor 2
	
	struct timeval begin_listen_time;
	struct timeval t_send_hb;
	struct timeval t_recv_hb;
	struct timeval t_giveup;
	struct timeval t_change_state;
	
	struct timeval t_recv_hb_2;
	struct timeval t_send_hb_2;
	
	struct timeval t_part_of_msg;		//for readn() return 0
	
	int flag_tcp1_decision_ok;
	int flag_tcp2_need_data_transfer;
	
	int flag_need_check_timeout_tcp1;		//TANG 2004.03.24
	int flag_need_check_timeout_tcp2;
	
	uid_t normal_uid;
	uid_t root_uid;
	
	tcpbuf tcpbuffer;					//TANG 2003.07.18
	
	LogFile * pRedunLog;				//TANG 2003.07.29
	
private:
	int get_last_msg_time();
	int init_serv_sock(unsigned int ip, int port, int *sock);
	int sock_connect(unsigned int host, int port, int *sock, unsigned int localip = 0);
	int writen(int fd, void *vptr, int n);
	int readn(int sockfd, unsigned char *buf, int nbytes);
	void close_socket(int * sock);
	int sock_exception(int * sock);
	
	int modifyIP(int flag);
	int modifyIP(unsigned int removeIP, unsigned int addIP, int DeleteAllFlag=0);
	int checkLineStatus();
	int checkIPExist(unsigned int ip);
	
	int to_normal();
	int normal_change_to_root();
	int root_change_to_normal();
	
	char * string_addr(unsigned int ip)
	{
		struct sockaddr_in addr;
		bzero(&addr, sizeof(addr));	
		memcpy(&addr.sin_addr.s_addr, &ip, 4);
		return inet_ntoa(addr.sin_addr);
	}
	
public:
	ActiveStandby();
	virtual ~ActiveStandby();
	
	int Init(struct RongYu * rong, LogFile * plog = NULL);
	
	void Run(unsigned int ms_sleep = 300);//used in while(); there is select() in Run()
	
	int SetActiveToStandby();
	
	int RequestNotCheckTimeoutBegin();
	int RequestNotCheckTimeoutEnd();
	
	int send_msg_copy_memory_done();
	int send_msg_copy_memory_done_confirm();
	
	int send_msg_copy_memory_begin();
	
	virtual void copy_memory() = 0;
	virtual void deal_data_of_memory(unsigned char * buf, int len) = 0;
	virtual void deal_data_response(unsigned char * buf, int len) = 0;
	
	int writeData(void * buf, int len);		//return 0 for error
	
	int new_msg(unsigned char *msg, unsigned char id, unsigned char type);
	int get_p(unsigned char *msg, unsigned char param, short *len, unsigned char *btype, void *datap);
	int put_p(unsigned char *msg, unsigned char param, short len, unsigned char btype, void *datap);
	int reverseint(unsigned char * myint);
	int getlen(unsigned char * msg);
};


int system_ready_exit(struct RongYu * rongyu);


#endif

