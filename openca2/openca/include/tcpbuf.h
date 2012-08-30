#ifndef _TCPBUF_H_
#define _TCPBUF_H_

#ifdef __WIN32__
        #include <winsock2.h>
	#include <list>
	using namespace std;
#else
	#include <list.h>
	using namespace std;
#endif

#include "logfile.h"


struct wait_msg_t
{
    int sockfd;
    void * msg;
    int msglen;
    int loglevel;
};


class tcpbuf
{
protected:
	int ifirst;
	int ilast;
	char * buf;
	int bNetworkOrder;
	LogFile * plog;					//TANG 2004.07.08
	
	//TANG 2008-9-25 14:17
	//for write tcp msg
	int max_tcp_connection_count;	//��ʾ�����int����Ĵ�С��FD_SETSIZE�Ǻ��ʵ�ֵ
	int * sockfd_msg_count;			//����[]
	int * sockfd_not_ready_write;	//����[]
	time_t * sockfd_will_close;		//����[]��Ϊ���ʾ��δ����ĳ��ʱ�����Ҫ�ر�socket
	list<wait_msg_t> wait_msg_list;
	int malloc_count;
	
public:
	//tcpbuf();
	tcpbuf(int isNetworkOrder = 0, int tcp_connection_count = 1);
	virtual ~tcpbuf();
	
	virtual int store(void * msg, int msglen);
	void move();
	int clearMsg();
	int clear();
	virtual int getOneMsg(void * buffer, int buflen);	//return 0 or msglen
	virtual int one_msg_len(unsigned char * msg);
	virtual int mini_msg_len();
	virtual int readOneMsg(int sockfd, void * buf, int buflen);
	int sock_ready_read(int sock);
	int sock_ready_write(int sock);
	int write2socket(int * sockfd, unsigned char * msgbuf, int len, int loglevel = 6);
	int writen(int fd, void *vptr, int n);	//for TCP only
	
	int SetpLog(LogFile * plog1);	//TANG 2004.07.08
	
	char * getAllMsgHeader() { return (buf+ifirst); }
	int    getAllMsgLength() { return (ilast-ifirst); }
	
	//TANG 2008-9-25 14:17
	//for write tcp msg
	int FD_SET2(fd_set * wfds);
	int write2list(int sockfd, void * msg, int msglen, int loglevel = 6);//������ķ�����Ϣ������д��list
	int list2socket(int & errorsocket);			//ÿ����һ�Σ��;�������tcp��Ϣ���ú���Ӧ�ñ�������ÿ500ms������
	                                   			//return 0, ��ʾĳ��socket�쳣���ڲ����з���socketֵ
	                                   			//���ó���Ӧ�ü���쳣��socketֵ������Ӧ�����������EBADF����
	int notify_socket_closed(int sockfd);		//socket�Ѿ����ر��ˣ�֪ͨlistɾ����������Ϣ���������EBADF����
	int close_socket_when_msg_done(int sockfd, int wait_time = 1);	//Ҫ�����ڷ�������Ϣ�󣬵ȴ����룬�ر�socket
	                                                              	//�ȴ���Ŀ����ϣ���ɶԷ����ر�socket����ȷ����Ϣ��������

private:							//TANG 2004.09.15
	tcpbuf(const tcpbuf& rhs);
	tcpbuf& operator=(const tcpbuf& rhs);
};


int close_socket(int * sockfd, tcpbuf * ptcpbuf);
int read_socket(int sockfd, void * buf, int nbytes);	//return <= 0 means error



class tcpbuf_xml : public tcpbuf
{
public:
	virtual int one_msg_len(unsigned char * msg);
	virtual int mini_msg_len();
};

int xml_get_parameter_int(char * buf, char * str1, char * str2);
int xml_get_parameter_string(char * buf, char * str1, char * str2, char * para, int paralen);
int xml_get_parameter_string(const string & buf, const string & str1, const string & str2, string & para);

int xml_put_parameter(char * buf, char * key, int para);
int xml_put_parameter(char * buf, char * key, char * para);
int xml_put_parameter(char * buf, char * key, const string & para);
int xml_put_parameter(char * buf, char * para);

int xml_put_parameter(string & buf, const string & key, double para);
int xml_put_parameter(string & buf, const string & key, int para);
int xml_put_parameter(string & buf, const string & key, const string & para);
int xml_put_parameter(string & buf, const string & para);

#endif

