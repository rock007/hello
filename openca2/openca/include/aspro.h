#ifndef _ASPRO_H_
#define _ASPRO_H_

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>

#include "ping.h"


#define iADAPTER	2		//count of public ethernet card


struct RongYu		//define in SHARE MEMORY
{
	//user given parameter
		int					master;				//1 for Master, 0 for Slave
		
		unsigned int 		activeIP;
		unsigned int		standbyIP;

		char 				adapterName[iADAPTER][32];		// 'eri0'
		char 				deviceName[iADAPTER][32];		// '/dev/eri'
		
		unsigned int		netPublic[iADAPTER];
		unsigned int		netPrivate;

		unsigned int		backup_netPublic[iADAPTER];
		unsigned int		backup_netPrivate;
		
		unsigned short		master_netPublicPort[iADAPTER];
		unsigned short		master_netPrivatePort;
		
		struct pinginfo		pingC;
		
		int					switchState;
	//end of user given parameter

	int					active;				//1 for active, 0 for standby
	
	int					HBconnection[iADAPTER];		//1 for connected, 0 for broken
	int					DATAconnection;
	
	int					adapterLineOFF[iADAPTER];	//1 for off, 0 for ok
	
	int					forwardDataFlag;
	//forwardDataFlag==1 && active==1, then you can forward message
	
	int					currentAdapter;		//1 or 2 is ok, and 0 for error
};



#define STANDBY_STATE	0
#define ACTIVE_STATE	1

//when TCP1 connected, Slave will send I_AM_ACTIVE/I_AM_STANDBY, Master will answer
#define I_AM_ACTIVE 		0x21
#define I_AM_STANDBY 		0x22
#define YOU_ARE_ACTIVE 		0x23
#define YOU_ARE_STANDBY 	0x24

//heart bit message
#define HB_REQUEST			0x31
#define HB_RESPONSE			0x32

//user command to let active change to standby
#define GIVEUP				0x51
#define CONFIRM				0x52
#define GIVEUP_OVER			0x53
#define UP_OVER				0x54


#define REDUN_HB_MSG_ID		0xFF
#define REDUN_TIME_PARAM_ID	0xFF
	

class ActiveStandby
{
private:
	int hme_fd[iADAPTER];
	int state;
	struct RongYu * rongyu;
	
	int udpsockfd;			//for ioctl() to get ip address

	int sockfdPublic[iADAPTER];
	int listenfdPublic[iADAPTER];	//for Master public address .. net adaptor public

	int sockfdPrivate;
	int listenfdPrivate;	//for Master private address.. net adaptor private
	
	struct timeval begin_listen_time[iADAPTER];
	struct timeval t_send_hb[iADAPTER];
	struct timeval t_recv_hb[iADAPTER];
	
	int flag_tcp_public_timeout[iADAPTER];
	int flag_tcp_public_broken[iADAPTER];
	int flag_tcp_public_no_connection[iADAPTER];

	struct timeval t_giveup;
	struct timeval t_change_state;
	
	struct timeval t_recv_hb_2;
	struct timeval t_send_hb_2;
	
	struct timeval t_part_of_msg;		//for readn() return 0
	
	int flag_tcp1_decision_ok;
	int flag_tcp2_need_data_transfer;
	
	uid_t normal_uid;
	uid_t root_uid;
	
private:
	int get_last_msg_time();
	int init_serv_sock(unsigned int ip, int port, int *sock);
	int sock_connect(unsigned int host, int port, int *sock);
	int writen(int fd, void *vptr, int n);
	int readn(int sockfd, unsigned char *buf, int nbytes);
	void close_socket(int * sock);
	int sock_exception(int * sock);
	
	int modifyIP(int flag);
	int modifyIP(int adapter, unsigned int removeIP, unsigned int addIP, int DeleteAllFlag=0);
	int checkLineStatus(int adapter);
	int checkIPExist( unsigned int ip, int adapter = (-1) );
	int setCurrentAdapter();
	
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
	
	int Init(struct RongYu * rong);
	
	void Run(unsigned int ms_sleep = 300);//used in while(); there is select() in Run()
	
	int SetActiveToStandby();
	
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


#endif

