#ifndef __MGCP_SOCK_H__
#define __MGCP_SOCK_H__

//#define  TRUE	1
//#define  FALSE	0
#define SOCKET_READ_TIMEOUT		1
#define SOCKET_WRITE_TIMEOUT	2

int sock_exception (int sock);
int sock_ready_read (int sock);
int sock_ready_write (int sock);
int init_serv_sock (int port, int *sock);

int init_serv_udpsock(int port, int *sock, unsigned int ip = 0);

//int sock_connect (int port, char *host, int *sock);
//int sock_connect(int port, unsigned int host, int *sock);
int sock_connect (int port, char * host, int *sock, unsigned int localip = 0);
int sock_connect (int port, unsigned int host, int *sock, unsigned int localip = 0);
int readn (int sockfd, unsigned char *buf, int nbytes);
int writen (int fd, void *vptr, int n);
int build_msg(char* buf,char* rbuf);
int send_msg(int sockfd,unsigned char *buf);
int getOwnIp();

int init_cli_udpsock(int *sock);

void send_udp_alarm_softswitch(int sockfd, unsigned int ip, int port, char * status);
void send_udp_alarm_process_hb(int sockfd, unsigned int ip, int port, char * processname);
void send_udp_alarm_process_restart(int sockfd, unsigned int ip, int port, char * processname);

void send_udp_alarm_license(int sockfd, unsigned int ip, int port, char * error);

void send_udp_alarm_tgw(int sockfd, unsigned int ip, int port, char * domainname);

void send_udp_alarm_running_error(int sockfd, unsigned int ip, int port, char * error);

#endif

/*
sock.h and sock.c also defined in:
	H323Agent:	H323Agent/h323libsrc/sock.cxx 				H323Agent/h323libsrc/lib_sock.h
	SipAgent:	opencalib/libsrc/sock/sock.c  				opencalib/include/sock.h
	CCM CC:		openca2/openca/sip/libs/sock.C 				openca2/openca/sip/include/sock.h
	CCM 323:	openca2/openca/h323gwpro/libsrc/sock.cxx 	openca2/openca/h323gwpro/libsrc/include/lib_sock.h
	CCM MGCP:	openca2/openca/libsrc/sock/sock.c 			openca2/openca/include/sock.h
	gk:			openca2/openca/gk/lib/sock.cxx				openca2/openca/gk/include/sock.h
	sn3:		gk
	ms:			gk
*/
