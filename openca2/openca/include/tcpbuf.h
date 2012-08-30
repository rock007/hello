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
	int max_tcp_connection_count;	//表示下面的int数组的大小，FD_SETSIZE是合适的值
	int * sockfd_msg_count;			//数组[]
	int * sockfd_not_ready_write;	//数组[]
	time_t * sockfd_will_close;		//数组[]，为真表示在未来的某个时间点需要关闭socket
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
	int write2list(int sockfd, void * msg, int msglen, int loglevel = 6);//并不真的发送消息，而是写入list
	int list2socket(int & errorsocket);			//每调用一次，就尽力发送tcp消息；该函数应该被经常（每500ms）调用
	                                   			//return 0, 表示某个socket异常，在参数中返回socket值
	                                   			//调用程序应该检查异常的socket值，做相应处理，否则会有EBADF错误
	int notify_socket_closed(int sockfd);		//socket已经被关闭了，通知list删除待发的消息，否则就是EBADF错误
	int close_socket_when_msg_done(int sockfd, int wait_time = 1);	//要求类在发送完消息后，等待几秒，关闭socket
	                                                              	//等待的目的是希望由对方来关闭socket，以确保消息接收完整

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

