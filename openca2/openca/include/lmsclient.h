#ifndef _LMSCLIENT_H_
#define _LMSCLIENT_H_


struct lms_info_t
{
	char 	licensefname[128];	//like "/export/home/openca/config/license.dat.GK"
	
	unsigned int servAddr;
	unsigned int port;
	
	int 	product_id;			//1: OpenCA;  2: GK;
	char 	product_version[16];
	
	void * buf;
	int buflen;
	
	time_t * curtime;
	
	char	serial_number[64];		//can be empty
	char	key_value[128];			//can be empty
};


int lmsactivation(struct lms_info_t * p);



int32_t             // <O> 0: ok
get_cpu_id_info(unsigned char *data_buf	// <O> fecth cpu_id_info into buf. if NULL, print
										//     buf_size must > 73
               );

int32_t             // <O> 0: ok
get_nic_mac_info (char *if_name,	// <I> Network Card Interface name, such as: eth0/eth1
									//     when NULL, use default(eth0)
				  unsigned char *data_buf	// <O> fecth nic_mac_addr into buf. if NULL, print
											//     buf_size must > 73
				 );

int get_host_info(char * if_name, unsigned char * data_buf, int buflen);


int encrypt_time_file(char * file, int filelen, char * cffile, char * sharekey);
time_t decrypt_time_file(char * file, char * sharekey);


void * check_hide_file(void * arg);



#endif

