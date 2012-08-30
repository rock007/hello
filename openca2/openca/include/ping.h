#ifndef PING_H__
#define PING_H__

#include <sys/time.h>
#include <unistd.h>

#define HOST_NUMBER_PING	16

/* function prototypes */
int  init_v4(int * sockfd);
void proc_v4(char *ptr, ssize_t len, struct timeval *tvrecv, struct sockaddr * sarecv, struct timeval * tv);
void send_v4(int sockfd, unsigned int hostip);
void *loopping(void * arg);
void tv_sub(struct timeval *, struct timeval *);
unsigned short	in_cksum(unsigned short *, int);


struct pinginfo {
	struct timeval 	recvtm;
	unsigned int	ip[HOST_NUMBER_PING];
};


#endif
