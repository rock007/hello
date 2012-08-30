#ifndef _SPIDER_MSG_H_
#define _SPIDER_MSG_H_

#define SPIDERMSG_SHAREKEY_LENGTH	32
#define SPIDERMSG_MAX_LEN			4096

#define PARA_TYPE_STRING			0x01
#define PARA_TYPE_INT				0x02
#define PARA_TYPE_STRUCT			0x03	//not recommend


class spidermsg
{
private:
	char sharekey[SPIDERMSG_SHAREKEY_LENGTH];
	
public:
	unsigned char * msg;
	int msglen;
	
public:
	spidermsg();
	spidermsg(char * key);
	~spidermsg();
	
	int begin_msg(int id, int type);
	int put_p(int paraid, int paratype, void * para, int paralen);
	int put_p_int(int paraid, int para);
	int put_p_string(int paraid, char * para);
	int get_p(unsigned char * msgbuf, int paraid, void * para, int & paralen);
	int end_msg();
	
	int check(unsigned char * msgbuf, int buflen);
	
	int set_sharekey(char * key);
};


#endif

