#ifndef _CLIENTGOU_H_
#define _CLIENTGOU_H_


struct Mgcp_license_name_t
{
	unsigned int servAddr;
	unsigned int port;
	int procId;
	void * buf;
	int buflen;
	time_t * curtime;
};


void *licenseThread(void* p);


#endif

