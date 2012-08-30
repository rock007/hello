#ifndef _TCPMSG_H_
#define _TCPMSG_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __WIN32__
	#include <process.h>
	#define getpid _getpid
	#define errno WSAGetLastError()
#endif

#ifdef __WINSOCK1__
	#include <afxsock.h>
#else
	#ifdef __WIN32__
		#include <winsock2.h>
	#endif
#endif

#ifdef __WINSOCK2__
	#include <winsock2.h>
#endif

#ifdef __LINUX__
	#include <unistd.h>
	#include <netinet/in.h>
	#include <inttypes.h>
	#include <strings.h>
	#include <sys/socket.h>
	#include <pthread.h>
#endif

#ifdef __SunOS__
	#include <unistd.h>
	#include <netinet/in.h>
	#include <inttypes.h>
	#include <strings.h>
	#include <sys/socket.h>
	#include <pthread.h>
#endif

#include <list>
#include <map>

#ifdef __WIN32__
#include "mutex_win.h"
#else
#include <boost/thread/recursive_mutex.hpp>
#endif

using namespace std;

#include "logfile.h"


#define mLOG	if ( plog ) plog->logwrite


#define TCPMSG_BUFFER_SIZE		(32*1024 - 2*4)

#define CLOSE_SOCKET_MAX_WAIT_TIME	60


class msg_format_sms
{
public:
	int one_msg_len(void * buf, int total_msg_len)
	{
		return total_msg_len;
	}
	
	int mini_msg_len()
	{
		return 1;
	}
	
	int max_msg_len()
	{
		return 4*1024-1;
	}
};


class msg_format_spider
{
public:
	int one_msg_len(void * buf, int total_msg_len)
	{
		unsigned char * msg = (unsigned char *) buf;
		
		int len = 0;
		len = (int)(msg[1] << 8);
		len += (int)msg[0];
		return len;
	}
	
	int mini_msg_len()
	{
		return 4;
	}
	
	int max_msg_len()
	{
		return 8*1024-1;
	}
};


class msg_format_xml
{
public:
	int one_msg_len(void * buf, int total_msg_len)
	{
		char * msg = (char *) buf;
		
		//flash会发出如下消息
		if ( strncmp(msg, "<policy-file-request/>", strlen("<policy-file-request/>")) == 0 )
		{
			return total_msg_len;
			//return ( (int)strlen("<policy-file-request/>") + 1 );
		}
		
		if ( msg[0] != '<' || msg[1] != 'h' || msg[2] != 'e' || msg[3] != 'a' || msg[4] != 'd' || msg[5] != '>' ) 
		{
			return -1;
		}
		
		char tmpbuf[12] = {0};
		memcpy(tmpbuf, msg+6, 5);
		
		int len = atoi(tmpbuf);
		
		if ( len < 0 )
		{
			return -1;
		}
		
		return (len+18);
	}
	
	int mini_msg_len()
	{
		return 18;
	}
	
	int max_msg_len()
	{
		return 8*1024-1;
	}
};


struct wait_msg_s
{
    int sockfd;
    void * msg;
    int msglen;
    int loglevel;
};

struct sock_send_info_t
{
	int sockfd_msg_count;
	int sockfd_not_ready_write;
	time_t sockfd_will_close;	//为真表示在未来的某个时间点需要关闭socket
};

struct sock_recv_info_t
{
	int ifirst;
	int ilast;
	char buf[TCPMSG_BUFFER_SIZE];
};


template <class MSGFORMAT>
class TCPMSG
{
private:
	MSGFORMAT msgformat;
	
	LogFile * plog;					//TANG 2004.07.08
	
	map<int,sock_send_info_t> sockfds;
	sock_send_info_t sock_send_info_error;	//一般不使用，有错误时返回引用
	
	map<int,sock_recv_info_t> sockfdr;
	sock_recv_info_t sock_recv_info_error;	//一般不使用，有错误时返回引用
	
	list<wait_msg_s> wait_msg_list;
	int malloc_count;
	
	boost::recursive_mutex mutex;
	
public:
	TCPMSG();
	~TCPMSG();
	
	int store(int sockfd, void * msg, int msglen);
	void move(int sockfd);
	int clear(int sockfd);
	int clear_recv_msg(int sockfd);
	int clear_send_msg(int sockfd);
	int getOneMsg(int sockfd, void * buffer, int buflen);	//return 0 or msglen
	int readOneMsg(int sockfd, void * buf, int buflen);
	int read_socket(int sockfd, void * buf, int nbytes);
	int sock_ready_read(int sock);
	int sock_ready_write(int sock);
	
	int SetpLog(LogFile * plog1);	//TANG 2004.07.08
	
	//char * getAllMsgHeader() { return (buf+ifirst); }
	//int    getAllMsgLength() { return (ilast-ifirst); }
	
	//TANG 2008-9-25 14:17
	//for write tcp msg
	int FD_SET2(fd_set * wfds);
	int write2list(int sockfd, const void * msg, int msglen, int loglevel = 6);//并不真的发送消息，而是写入list
	int list2socket(int & errorsocket);			//每调用一次，就尽力发送tcp消息；该函数应该被经常（每500ms）调用
	                                   			//return 0, 表示某个socket异常，在参数中返回socket值
	                                   			//调用程序应该检查异常的socket值，做相应处理，否则会有EBADF错误
	int notify_socket_closed(int sockfd);		//socket已经被关闭了，通知list删除待发的消息，否则就是EBADF错误
	int close_socket_when_msg_done(int sockfd, int wait_time = 1);	//要求类在发送完消息后，等待几秒，关闭socket
	                                                              	//等待的目的是希望由对方来关闭socket，以确保消息接收完整

private:							//TANG 2004.09.15
	TCPMSG(const TCPMSG& rhs);
	TCPMSG& operator=(const TCPMSG& rhs);
	
	sock_send_info_t & find_send_sock(int sockfd, int flag_find_only = 0);
	sock_recv_info_t & find_recv_sock(int sockfd, int flag_find_only = 0);
	
	int notify_socket_s_closed(int sockfd);
	int notify_socket_r_closed(int sockfd);
	
	int write2socket(int * sockfd, unsigned char * msgbuf, int len, int loglevel = 6); //return 0: 失败
	int writen(int fd, void *vptr, int n);	//for TCP only
};


template <class MSGFORMAT>
TCPMSG<MSGFORMAT>::TCPMSG()
{
	plog = NULL;
	malloc_count = 0;
}

template <class MSGFORMAT>
TCPMSG<MSGFORMAT>::~TCPMSG()
{
	//TANG 2008-9-25 14:17
	//释放list，包括list中为消息而malloc的内存
	for ( list<wait_msg_s>::iterator plist = wait_msg_list.begin(); plist != wait_msg_list.end(); ++plist )
	{
		if ( plist->msg != NULL )
		{
			free(plist->msg);
			plist->msg = NULL;
			malloc_count--;
		}
	}
	
	wait_msg_list.clear();
	
	sockfds.clear();
	sockfdr.clear();
}

template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::clear(int sockfd)
{
	clear_recv_msg(sockfd);
	clear_send_msg(sockfd);
	return 1;
}

template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::clear_recv_msg(int sockfd)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_recv_info_t & sockfdinfo = find_recv_sock(sockfd, 1);	//find only, not insert
	
	sockfdinfo.ifirst = sockfdinfo.ilast = 0;
	
	return 1;
}

template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::clear_send_msg(int sockfd)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_send_info_t & sockfdinfo = find_send_sock(sockfd, 1);	//find only, not insert
	
	list<wait_msg_s>::iterator plist = wait_msg_list.begin();
	while ( plist != wait_msg_list.end() )
	{
		if ( plist->sockfd == sockfd )
		{
			mLOG(plist->loglevel, "clear_send_msg(): sockfd=%d, msglen=%d, sock_r=%d, sock_s=%d, total_msg=%d, sockfd_msg=%d, malloc=%d\n", 
					plist->sockfd, plist->msglen, sockfdr.size(), sockfds.size(), wait_msg_list.size()-1, sockfdinfo.sockfd_msg_count-1, malloc_count-1);
				
			if ( plist->msg != NULL )
			{
				free(plist->msg);
				plist->msg = NULL;
				malloc_count--;
			}
			
			sockfdinfo.sockfd_msg_count--;
			plist = wait_msg_list.erase(plist);
			continue;
		}
		
		plist++;	
	}
	
	if ( sockfdinfo.sockfd_msg_count != 0 )
	{
		//error
		mLOG(LOG1, "clear_send_msg(): sockfd_msg_count[%d] = %d, error\n", sockfd, sockfdinfo.sockfd_msg_count);
	}
	
	sockfdinfo.sockfd_msg_count = 0;
	sockfdinfo.sockfd_will_close = 0;
	sockfdinfo.sockfd_not_ready_write = 0;
	
	return 1;
}

template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::store(int sockfd, void * msg, int msglen)
{
	if ( sockfd <= 0 ) return 0;
	if ( msglen <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_recv_info_t & sockfdinfo = find_recv_sock(sockfd);
	
	if ( TCPMSG_BUFFER_SIZE - sockfdinfo.ilast > msglen )
	{
		memcpy(sockfdinfo.buf + sockfdinfo.ilast, msg, msglen);
		sockfdinfo.ilast += msglen;
		sockfdinfo.buf[sockfdinfo.ilast] = 0;
		return 1;
	}
	else
	{
		move(sockfd);
		
		if ( TCPMSG_BUFFER_SIZE - sockfdinfo.ilast > msglen )
		{
			memcpy(sockfdinfo.buf + sockfdinfo.ilast, msg, msglen);
			sockfdinfo.ilast += msglen;
			sockfdinfo.buf[sockfdinfo.ilast] = 0;
			return 1;
		}
		else
		{
			mLOG(LOG1, "TCPMSG<MSGFORMAT>::store() error TCPMSG_BUFFER_SIZE=%d and ifirst=%d ilast=%d and msglen=%d\n", 
				TCPMSG_BUFFER_SIZE, sockfdinfo.ifirst, sockfdinfo.ilast, msglen);
			return 0;
		}
	}
}

template <class MSGFORMAT>
void TCPMSG<MSGFORMAT>::move(int sockfd)
{
	if ( sockfd <= 0 ) return;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_recv_info_t & sockfdinfo = find_recv_sock(sockfd);
	
	if ( sockfdinfo.ifirst == sockfdinfo.ilast ) sockfdinfo.ifirst = sockfdinfo.ilast = 0;
	if ( sockfdinfo.ifirst == 0 ) return;
	
	int len = sockfdinfo.ilast - sockfdinfo.ifirst;
	memmove(sockfdinfo.buf, sockfdinfo.buf + sockfdinfo.ifirst, len);
	
	sockfdinfo.ifirst = 0;
	sockfdinfo.ilast = len;
	sockfdinfo.buf[sockfdinfo.ilast] = 0;
	
	return;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::getOneMsg(int sockfd, void * buffer, int buflen)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_recv_info_t & sockfdinfo = find_recv_sock(sockfd);
	
	if ( sockfdinfo.ilast - sockfdinfo.ifirst < msgformat.mini_msg_len() ) return 0;
	
	if ( buflen <= 0 )
	{
		mLOG(LOG1, "TCPMSG<MSGFORMAT>::getOneMsg() buffer length = %d error\n", buflen);
		return 0;
	}
	memset(buffer, 0, buflen);
	
	int msglen = msgformat.one_msg_len(sockfdinfo.buf + sockfdinfo.ifirst, sockfdinfo.ilast - sockfdinfo.ifirst);
	if ( msglen < 0 || msglen > msgformat.max_msg_len() || msglen < msgformat.mini_msg_len() )
	{
		mLOG(LOG1, "TCPMSG<MSGFORMAT>::getOneMsg() calculate length for one msg is %d, error, clear tcp buf\n", msglen);
		sockfdinfo.ifirst = sockfdinfo.ilast = 0;
		return 0;
	}
	
	if ( sockfdinfo.ilast - sockfdinfo.ifirst >= msglen )
	{
		if ( msglen > buflen )
		{
			mLOG(LOG1, "TCPMSG<MSGFORMAT>::getOneMsg() buffer too small for one msg len %d, error, discard the msg\n", msglen);
			sockfdinfo.ifirst += msglen;	//discard this msg
			return 0;
		}
		memcpy(buffer, sockfdinfo.buf + sockfdinfo.ifirst, msglen);
		*( (unsigned char *)buffer + msglen ) = 0;

		sockfdinfo.ifirst += msglen;
		if ( sockfdinfo.ifirst == sockfdinfo.ilast ) 
		{
			sockfdinfo.ifirst = sockfdinfo.ilast = 0;
		}
		else if ( sockfdinfo.ifirst > sockfdinfo.ilast )
		{
			mLOG(LOG1, "TCPMSG<MSGFORMAT>::getOneMsg() ifrist = %d > ilast = %d, error\n", sockfdinfo.ifirst, sockfdinfo.ilast);
			sockfdinfo.ifirst = sockfdinfo.ilast = 0;
		}
		
		if ( msglen < 0 )
		{
			mLOG(LOG1, "TCPMSG<MSGFORMAT>::getOneMsg() error the return value msglen = %d\n",msglen);
		}
		return msglen;
	}
	
	return 0;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::read_socket(int sockfd, void * buf, int nbytes)
{
	if ( nbytes <= 0 ) 
	{
		printf("read_socket(sockfd:%d): buffer size nbytes = %d error\n", sockfd, nbytes);
		return 0;
	}
	
	int retval = 0;
	#ifdef __WIN32__
	retval = recv(sockfd, (char *)buf, nbytes, 0);
	#else
	retval = read(sockfd, buf, nbytes);
	#endif
	return retval;
}


//return -1  for error, socket must be closed
//return 0   for no available message now
//return > 0 for message ok
template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::readOneMsg(int sockfd, void * buf, int buflen)
{
	int retval = 0;
	
	if ( buflen <= 0 )
	{
		mLOG(LOG1, "TCPMSG<MSGFORMAT>::readOneMsg() buffer length = %d, error\n", buflen);
		return 0;
	}
	
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_recv_info_t & sockfdinfo = find_recv_sock(sockfd);
	
	if ( sockfdinfo.ilast - sockfdinfo.ifirst < TCPMSG_BUFFER_SIZE*3/4 )
	{
		if ( sock_ready_read(sockfd) )
		{
			//read msg
			memset(buf, 0, buflen);
			retval = read_socket(sockfd, buf, buflen);
			if ( retval < 0 )
			{
				//read error
				int ierr = errno;
				if ( ierr == EINTR )
				{
					return 0;
				}
				#ifdef __LINUX__
				mLOG(LOG6, "TCPMSG<MSGFORMAT>::read_socket(): sockfd(%d) pid(%d) threadid(%d) read error=%d\n", sockfd, (int)getpid(), (int)pthread_self(), ierr);
				#endif
				return -1;
			}
			else if ( retval == 0 )
			{
				#ifdef __LINUX__
				mLOG(LOG6, "TCPMSG<MSGFORMAT>::read_socket(): sockfd(%d) pid(%d) threadid(%d) read = 0\n", sockfd, (int)getpid(), (int)pthread_self());
				#endif
				return -1;
			}
			store(sockfd, buf, retval);
		}
	}
	
	retval = 0;
	memset(buf, 0, buflen);
	int ret = getOneMsg(sockfd, buf, buflen) ;
	if ( ret < 0 )
	{
		mLOG(LOG1, "TCPMSG<MSGFORMAT>::readOneMsg(): getOneMsg() ret(%d) error\n",ret);
	}
	return ret;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::sock_ready_read(int sock)
{
   fd_set            rdfds;
   struct timeval    time_val;
   
   time_val.tv_sec  = 0;
   time_val.tv_usec = 0;		//non-block
   
   if ( sock <= 0 ) return 0;

   FD_ZERO(&rdfds);
   FD_SET(sock, &rdfds);
   
   if ( select(sock+1, &rdfds, NULL, NULL, &time_val) > 0 ) 
   {
		if (FD_ISSET (sock, &rdfds)) 
		{
			return 1;
		}
   }
   return 0;
}

template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::sock_ready_write(int sock)
{
	fd_set            wrfds;
	struct timeval    time_val;
	
	time_val.tv_sec  = 0;
	time_val.tv_usec = 0; //TANG
	
	if ( sock <= 0 ) return 0;
	
	FD_ZERO(&wrfds);
	FD_SET(sock, &wrfds);
	
	if ( select(sock+1, (fd_set *) NULL, &wrfds, (fd_set *) NULL, &time_val) < 0 )
	{
		return -1;
	}
	
	if (FD_ISSET (sock, &wrfds)) 
	{
		return 1;
	}
	
	return 0;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::SetpLog(LogFile * plog1) 
{
	plog = plog1;
	return 1;
}


template <class MSGFORMAT>
sock_send_info_t & TCPMSG<MSGFORMAT>::find_send_sock(int sockfd, int flag_find_only)
{
	map<int,sock_send_info_t>::iterator it = sockfds.find(sockfd);
	if ( it != sockfds.end() ) return it->second;
	
	if ( flag_find_only ) return sock_send_info_error;
	
	static sock_send_info_t sockfdinfo;
	sockfdinfo.sockfd_msg_count = 0;
	sockfdinfo.sockfd_not_ready_write = 0;
	sockfdinfo.sockfd_will_close = 0;
	sockfds.insert(map<int,sock_send_info_t>::value_type(sockfd, sockfdinfo));
	
	mLOG(LOG6, "find_send_sock(): sockfds.insert(%d)\n", sockfd);
	
	it = sockfds.find(sockfd);
	if ( it != sockfds.end() ) return it->second;
	
	mLOG(LOG1, "find_send_sock() sockfds.find(%d) error\n", sockfd);
	return sock_send_info_error;
}


template <class MSGFORMAT>
sock_recv_info_t & TCPMSG<MSGFORMAT>::find_recv_sock(int sockfd, int flag_find_only)
{
	map<int,sock_recv_info_t>::iterator it = sockfdr.find(sockfd);
	if ( it != sockfdr.end() ) return it->second;
	
	if ( flag_find_only ) return sock_recv_info_error;
	
	static sock_recv_info_t sockfdinfo;
	sockfdinfo.ifirst = 0;
	sockfdinfo.ilast = 0;
	sockfdr.insert(map<int,sock_recv_info_t>::value_type(sockfd, sockfdinfo));
	
	mLOG(LOG6, "find_recv_sock(): sockfdr.insert(%d)\n", sockfd);
	
	it = sockfdr.find(sockfd);
	if ( it != sockfdr.end() ) return it->second;
	
	mLOG(LOG1, "find_recv_sock() sockfdr.find(%d) error\n", sockfd);
	return sock_recv_info_error;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::write2list(int sockfd, const void * msg, int msglen, int loglevel)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_send_info_t & sockfdinfo = find_send_sock(sockfd);
	
	struct wait_msg_s msg1;
	memset(&msg1, 0, sizeof(msg1));
	
	msg1.sockfd = sockfd;
	msg1.msg = malloc(msglen+1);
	msg1.msglen = msglen;
	msg1.loglevel = loglevel;
	
	if( msg1.msg == NULL )
	{
		mLOG(LOG1, "write2list(): malloc() error\n");
		return 0;
	}
	
	malloc_count++;
	
	memcpy(msg1.msg, msg, msglen);
	((unsigned char *)msg1.msg)[msglen] = 0;
	
	wait_msg_list.push_back(msg1);
	sockfdinfo.sockfd_msg_count++;
	
	mLOG(msg1.loglevel, "write2list(): sockfd=%d, msglen=%d, sock_r=%d, sock_s=%d, total_msg=%d, sockfd_msg=%d, malloc=%d\n", 
					msg1.sockfd, msg1.msglen, sockfdr.size(), sockfds.size(), wait_msg_list.size(), sockfdinfo.sockfd_msg_count, malloc_count);
	if ( ((char *)msg1.msg)[0] == '<' && ((char *)msg1.msg)[1] == 'h' && ((char *)msg1.msg)[2] == 'e' && ((char *)msg1.msg)[3] == 'a' && ((char *)msg1.msg)[4] == 'd' && ((char *)msg1.msg)[5] == '>' )
	{
		mLOG(msg1.loglevel, "  msg = '%s'\n", ((char *)msg1.msg));
	}
	else
	{
		mLOG(msg1.loglevel, msg1.msglen, (unsigned char *)msg1.msg);
	}
	
	return 1;
}


//return 0, 表示某个socket异常，在参数中返回socket值
template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::list2socket(int & errorsocket)
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	errorsocket = 0;
	
	list<int> socket_will_delete;
	
	//检查是否有socket需要关闭
	for ( map<int,sock_send_info_t>::iterator it = sockfds.begin(); it != sockfds.end(); ++it )
	{
		if ( it->second.sockfd_msg_count == 0 && it->second.sockfd_will_close )
		{
			if ( time(NULL) >= it->second.sockfd_will_close )
			{
				//时间到，可以关闭socket了
				mLOG(LOG6, "list2socket(): close sockfd = %d\n", it->first);
				
				it->second.sockfd_will_close = 0;
				socket_will_delete.push_back(it->first);
				
				#ifdef __WIN32__
				closesocket(it->first);
				#else
				shutdown(it->first, SHUT_WR);
				close(it->first);
				#endif
			}
			else
			{
				//等待对方关闭socket
				if ( sock_ready_read(it->first) )
				{
					//read msg
					char buf[512] = {0};
					int retval = read_socket(it->first, buf, sizeof(buf));
					if ( retval <= 0 )
					{
						//read error
						mLOG(LOG6, "list2socket(): other side close sockfd = %d\n", it->first);
						
						it->second.sockfd_will_close = 0;
						socket_will_delete.push_back(it->first);
						
						#ifdef __WIN32__
						closesocket(it->first);
						#else
						shutdown(it->first, SHUT_WR);
						close(it->first);
						#endif
					}
				}
			}
		}
	}
	
	if ( !socket_will_delete.empty() )
	{
		for ( list<int>::iterator plist = socket_will_delete.begin(); plist != socket_will_delete.end(); ++plist )
		{
			notify_socket_closed(*plist);
		}
		
		socket_will_delete.clear();
	}
	
	for ( map<int,sock_send_info_t>::iterator it = sockfds.begin(); it != sockfds.end(); ++it )
	{
		it->second.sockfd_not_ready_write = 0;
	}
	
	list<wait_msg_s>::iterator plist = wait_msg_list.begin();
	while ( plist != wait_msg_list.end() )
	{
		sock_send_info_t & sockfdinfo = find_send_sock(plist->sockfd);
		
		if ( sockfdinfo.sockfd_not_ready_write == 0 )	//该socket目前还是可写的
		{
			int ret = sock_ready_write(plist->sockfd);
			
			if ( ret == -1 )
			{
				//error
				int ierr = errno;
				if ( ierr == EBADF )
				{
					//socket无效
					errorsocket = plist->sockfd;
					mLOG(LOG1, "list2socket(): sock_ready_write() sockfd=%d, error = EBADF\n", plist->sockfd);
					notify_socket_closed(plist->sockfd);
					return 0;
				}
				else if ( ierr == EINTR )
				{
					//似乎可以忽略
				}
				else
				{
					errorsocket = plist->sockfd;
					mLOG(LOG1, "list2socket(): sock_ready_write() sockfd=%d, error = %d\n", plist->sockfd, ierr);
					notify_socket_closed(plist->sockfd);
					return 0;
				}
			}
			
			else if ( ret == 0 )
			{
				//socket不可写
				sockfdinfo.sockfd_not_ready_write = 1;
				//mLOG(LOG6, "list2socket(): sockfd = %d not ready write\n", plist->sockfd);
			}
			
			else
			{
				//socket可写
				if ( plist->msg != NULL )
				{
					int tmpsockfd = plist->sockfd;
					if ( write2socket(&tmpsockfd, (unsigned char *)plist->msg, plist->msglen, 100) == 0 )
					{
						errorsocket = plist->sockfd;
						mLOG(LOG1, "list2socket(): write2socket() sockfd=%d, error\n", plist->sockfd);
						notify_socket_closed(plist->sockfd);
						return 0;
					}
					
					mLOG(plist->loglevel, "list2socket(): sockfd=%d, msglen=%d, sock_r=%d, sock_s=%d, total_msg=%d, sockfd_msg=%d, malloc=%d\n", 
									plist->sockfd, plist->msglen, sockfdr.size(), sockfds.size(), wait_msg_list.size()-1, sockfdinfo.sockfd_msg_count-1, malloc_count-1);
					if ( ((char *)plist->msg)[0] == '<' && ((char *)plist->msg)[1] == 'h' && ((char *)plist->msg)[2] == 'e' && ((char *)plist->msg)[3] == 'a' && ((char *)plist->msg)[4] == 'd' && ((char *)plist->msg)[5] == '>' )
					{
						mLOG(plist->loglevel, "  msg = '%s'\n", ((char *)plist->msg));
					}
					else
					{
						mLOG(plist->loglevel, plist->msglen, (unsigned char *)plist->msg);
					}
					
					free(plist->msg);
					plist->msg = NULL;
					malloc_count--;
				}
				
				sockfdinfo.sockfd_msg_count--;
				
				if ( sockfdinfo.sockfd_msg_count == 0 && sockfdinfo.sockfd_will_close && sockfdinfo.sockfd_will_close <= CLOSE_SOCKET_MAX_WAIT_TIME )
				{
					//list中没有待发送的消息
					mLOG(LOG6, "list2socket(): will close sockfd=%d after %d seconds\n", plist->sockfd, sockfdinfo.sockfd_will_close);
					sockfdinfo.sockfd_will_close += time(NULL);
					/*
					mLOG(plist->loglevel, "list2socket(): close sockfd = %d\n", plist->sockfd);
					sockfdinfo.sockfd_will_close = 0;
					shutdown(plist->sockfd, SHUT_WR);
					close(plist->sockfd);
					plist->sockfd = 0;
					*/
				}
				
				plist = wait_msg_list.erase(plist);
				continue;
			}
		}
		
		plist++;
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::notify_socket_closed(int sockfd)
{
	if ( sockfd <= 0 ) return 0;
	
	clear_recv_msg(sockfd);
	clear_send_msg(sockfd);
	
	notify_socket_s_closed(sockfd);
	notify_socket_r_closed(sockfd);
	
	return 1;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::close_socket_when_msg_done(int sockfd, int wait_time)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	sock_send_info_t & sockfdinfo = find_send_sock(sockfd);
	
	if ( wait_time <= 0 ) wait_time = 1;
	if ( wait_time > CLOSE_SOCKET_MAX_WAIT_TIME ) wait_time = CLOSE_SOCKET_MAX_WAIT_TIME;
	
	if ( sockfdinfo.sockfd_msg_count > 0 )
	{
		//list中有待发送的消息
		sockfdinfo.sockfd_will_close = wait_time;
	}
	else
	{
		//list中没有待发送的消息
		mLOG(LOG6, "close_socket_when_msg_done(): will close sockfd=%d after %d seconds\n", sockfd, wait_time);
		sockfdinfo.sockfd_will_close = time(NULL) + wait_time;
		/*
		mLOG(LOG6, "close_socket_when_msg_done(): close sockfd = %d\n", sockfd);
		sockfdinfo.sockfd_will_close = 0;
		shutdown(sockfd, SHUT_WR);
		close(sockfd);
		sockfd = 0;
		*/
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::FD_SET2(fd_set * wfds)
{
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	for ( map<int,sock_send_info_t>::iterator it = sockfds.begin(); it != sockfds.end(); ++it )
	{
		if ( it->second.sockfd_msg_count ) FD_SET(it->first, wfds);
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::write2socket(int * sockfd, unsigned char * msgbuf, int len, int loglevel)
{
	if ( *sockfd <= 0 ) return 0;
	if ( len <= 0 ) return 0;
	
	if ( writen(*sockfd, msgbuf, len) )
	{
		mLOG(loglevel, "write2socket(): write msg %d bytes ok\n", len);
		mLOG(loglevel, len, msgbuf);
		return 1;
	}
	else
	{
		mLOG(LOG1, "write2socket(): writen() error=%d, msg is:\n", errno);
		mLOG(1, len, msgbuf);
		/*
		#ifdef __WIN32__
		closesocket(*sockfd);
		#else
		shutdown(*sockfd, SHUT_WR);
		close(*sockfd);
		#endif
		*sockfd = 0;
		*/
		return 0;
	}
}


//writen() return 0 for error
template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::writen(int fd, void *vptr, int n)	//for TCP only
{
	if ( fd <= 0 ) return 0;	//TANG 2003.07.14
	
	size_t		nleft;
	int			nwritten;
	char		*ptr;
	
	int			ntimes;			//TANG 0914 for EINTR(SIGPIPE) and EPIPE

	//if (sock_exception (fd)) 
	//{
	//	return 0;
	//}
	//if (!sock_ready_write (fd)) 
	//{
	//	//printf("writen() sock_ready_write() error\n");
	//	return 0;
	//}
	
	ntimes = 0;
	ptr = (char*)vptr;
	nleft = (size_t) n;
	
	while (nleft > 0) 
	{
		#ifdef __WIN32__
		if ( (nwritten = send(fd, ptr, nleft, 0)) <= 0) 
		#else
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) 
		#endif
		{
			if ( errno == EINTR )
			{
				nwritten = 0;		/* and call write() again */
				
				ntimes++;
				if ( ntimes > 10 )
				{
					return 0;
				}
			}
			else
			{
				//perror("tcp socket write() error");
				return 0;			/* error */
			}
		}
		
		nleft -= nwritten;
		ptr   += nwritten;
	}
	
	return(n);
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::notify_socket_s_closed(int sockfd)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	if ( sockfds.erase(sockfd) == 0 ) //如果删除了会返回1，否则返回0
	{
		mLOG(LOG1, "sockfds.erase(%d) error\n", sockfd);
	}
	else
	{
		mLOG(LOG6, "sockfds.erase(%d)\n", sockfd);
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCPMSG<MSGFORMAT>::notify_socket_r_closed(int sockfd)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	if ( sockfdr.erase(sockfd) == 0 ) //如果删除了会返回1，否则返回0
	{
		mLOG(LOG1, "sockfdr.erase(%d) error\n", sockfd);
	}
	else
	{
		mLOG(LOG6, "sockfdr.erase(%d)\n", sockfd);
	}
	
	return 1;
}


#endif

