#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#ifdef __WIN32__
	#include <winsock2.h>
	#include <time.h>
	#include <sys\timeb.h>
	#include <string>
#else
	#include <sys/uio.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#include "logfile.h"
#include "tcpmsg.h"

#define myLOG1 if ( plog ) plog->logwrite
#define myLOG2 if ( pc->plog ) pc->plog->logwrite


#ifdef __WIN32__
	typedef int	ssize_t;
#else

#endif


#ifdef __WIN32__
	#define gettimeofday(tv, tz) \
		{ \
			struct timeb tb; \
			ftime(&tb); \
			(tv)->tv_sec = tb.time; \
			(tv)->tv_usec = tb.millitm * 1000; \
		}
	#define bzero(x, y) memset((x), 0, (y))
	#define errno WSAGetLastError()
#else

#endif


inline int is_ip(const char * addr)
{
	if ( addr == NULL || addr[0] == 0 ) return 0;
	
	for ( unsigned int i=0; i<strlen(addr); i++ )
	{
		if ( ( addr[i] >= '0' && addr[i] <= '9' ) || addr[i] == '.' )
		{
			
		}
		else
		{
			return 0;
		}
	}
	
	return 1;
}


#define TCP_RECV_MSG	0	//收到消息，msg和msglen有效
#define TCP_CLOSED		1	//对方关闭连接
#define TCP_MSG_TIMEOUT	2	//接收消息超时，自己主动关闭连接
#define TCP_MSG_ERROR	3	//接收消息格式错误
#define TCP_CONNECTED	4	//建立连接
#define TCP_COMPLETE	5	//自己主动关闭连接
#define TCP_CONNECT_FAILED	6	//连接失败
#define TCP_ERROR			7	//TCP异常


template <class MSGFORMAT>
class TCP_CLIENT
{
public:
	int connect_count_try;
	int connect_count_success;
	int connect_count_fail;
	int msg_send_count;
	int msg_recv_count;
	
private:
	TCPMSG<MSGFORMAT> tcpmsg;
	
	LogFile * plog;
	
	int sockfd;			//线程数据
	time_t t_connect;	//线程数据
	time_t t_hb;		//线程数据
	time_t t_recv;		//线程数据
	
	int re_connect_interval;
	int send_hb_interval;
	int recv_msg_timeout;
	
	string server_ip;
	int server_port;
	
	int flag_thread_state;	//0-线程没有运行，1-线程正在运行。不需要锁保护。
	int flag_thread_exit;	//通知线程退出时置1，线程退出前置0。可能不需要锁保护。
	
	unsigned char * loginmsg;
	int loginmsglen;
	
	unsigned char * hbmsg;
	int hbmsglen;

private:
	char * string_addr(unsigned int ip)
	{
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));	
		memcpy(&addr.sin_addr.s_addr, &ip, 4);
		return inet_ntoa(addr.sin_addr);
	}
	
	static void * thread_deal_tcp(void * arg);
	
	int sock_connect(int port, unsigned int host, int *sock, unsigned int localip = 0)
	{    
		if ( host == 0 ) return 0;
		if ( host == 0xFFFFFFFF ) return 0;
		
		if ( port == 0 ) return 0;
		
		struct sockaddr_in serv_adr;
		int	   setFlag; 
		struct linger linger;
		
		if ( (*sock = socket(AF_INET, SOCK_STREAM, 6)) < 0 ) 
		{
			*sock = 0;
			myLOG1(LOG1, "TCP_CLIENT: socket() error\n");
			return 0;
		}
		
		bzero((char *)&serv_adr, sizeof(serv_adr));
		serv_adr.sin_family      = AF_INET;
		memcpy (&serv_adr.sin_addr, &host, sizeof(unsigned int));
		//serv_adr.sin_addr.s_addr = htonl(host);
		serv_adr.sin_port        = htons(port);
		
		#ifdef __WIN32__
		
		#else
		setFlag = 1;
		if (setsockopt(*sock, SOL_SOCKET, SO_KEEPALIVE, &setFlag, sizeof(setFlag)) < 0) 
		{
		    perror("setsockopt(SO_KEEPALIVE) error\n");
		    #ifdef __WIN32__
			closesocket(*sock);
			#else
			close(*sock);
			#endif
		    *sock = 0;
		    myLOG1(LOG1, "TCP_CLIENT: setsockopt() error\n");
			return 0;
		}
		
		linger.l_onoff = 1;		//TANG 1224
		linger.l_linger = 0;
		if (setsockopt(*sock, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger)) < 0) 
		{
			perror("setsockopt(SO_LINGER) error\n");
			#ifdef __WIN32__
			closesocket(*sock);
			#else
			close(*sock);
			#endif
			*sock = 0;
			myLOG1(LOG1, "TCP_CLIENT: setsockopt() error\n");
			return 0;
		}
		#endif
		
		//TANG 2004.05.10
		if ( localip )
		{
			struct sockaddr_in local_adr;
			bzero(&local_adr, sizeof(local_adr));
		  	//
		  	local_adr.sin_family = AF_INET;
		  	local_adr.sin_port   = htons(0);
			memcpy(&local_adr.sin_addr.s_addr, &localip, sizeof(int));	//ip is already "network byte order"
			//
		  	if ( bind(*sock, (struct sockaddr *)&local_adr, sizeof(local_adr)) < 0) 
		  	{
		    	//bind error, but for TCP client, go on
		  	}
		}
		//END
		
		if (connect(*sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0) 
		{
		    #ifdef __WIN32__
			closesocket(*sock);
			#else
			close(*sock);
			#endif
		    *sock = 0;
		    //myLOG1(LOG1, "TCP_CLIENT: connect() error\n");
		 	return 0;
		}             
		  
		return 1;
	}

public:
	TCP_CLIENT()
	{
		plog = NULL;
		
		sockfd = 0;
		t_connect = 0;
		t_hb = 0;
		t_recv = 0;
		
		re_connect_interval = 0;
		send_hb_interval = 0;
		recv_msg_timeout = 0;
		
		connect_count_try = 0;
		connect_count_success = 0;
		connect_count_fail = 0;
		msg_send_count = 0;
		msg_recv_count = 0;
		
		server_ip = "";
		server_port = 0;
		
		flag_thread_state = 0;
		flag_thread_exit = 0;
		
		loginmsg = NULL;
		loginmsglen = 0;
		
		hbmsg = NULL;
		hbmsglen = 0;
	}
	
	~TCP_CLIENT()
	{
		loginmsglen = 0;
		hbmsglen = 0;
		
		if ( loginmsg )
		{
			free(loginmsg);
			loginmsg = NULL;
		}
		
		if ( hbmsg )
		{
			free(hbmsg);
			hbmsg = NULL;
		}
		
		Close();
		
		#ifdef __WIN32__
		WSACleanup( );
		#endif
	}
	
protected:
	static void (*callbackfunc)(int state, void * msg, int msglen);

public:
	int	SetpLog(LogFile * plog1);
	
	int	SetLoginMsg(const void * msg, int msglen);
	int	SetHBMsg(const void * msg, int msglen);
	
	int	SetCallBackFunc(void (*func)(int state, void * msg, int msglen));
	
	//启动线程
	int	Connect(const string & ip, int port, int recv_msg_timeout1 = 0, int send_hb_interval1 = 0, int re_connect_interval1 = 0);
	
	int Send(const void * msg, int msglen, int loglevel = 6);
	
	int GetSocket() { return sockfd; }
	
	int is_connected()
	{
		if ( sockfd > 0 ) return 1;
		return 0;
	}
	
	int is_thread_running()
	{
		return flag_thread_state;	//0-线程没有运行，1-线程正在运行
	}
	
	//结束线程
	int Close();
};


template <class MSGFORMAT> 
void (* TCP_CLIENT<MSGFORMAT>::callbackfunc)(int state, void * msg, int msglen) = NULL;


template <class MSGFORMAT>
int TCP_CLIENT<MSGFORMAT>::SetCallBackFunc(void (*func)(int state, void * msg, int msglen))
{
	callbackfunc = func;
	return 1;
}


template <class MSGFORMAT>
int TCP_CLIENT<MSGFORMAT>::SetpLog(LogFile * plog1) 
{
	plog = plog1;
	tcpmsg.SetpLog(plog1);
	return 1;
}


template <class MSGFORMAT>
int TCP_CLIENT<MSGFORMAT>::SetLoginMsg(const void * msg, int msglen) 
{
	if ( msg == NULL ) return 0;
	if ( msglen <= 0 ) return 0;
	
	loginmsglen = 0;
	
	if ( loginmsg )
	{
		free(loginmsg);
		loginmsg = NULL;
	}
	
	loginmsg = (unsigned char *) malloc(msglen+1);
	if ( loginmsg )
	{
		memcpy(loginmsg, msg, msglen+1);
		loginmsglen = msglen;
		return 1;
	}
	
	return 0;
}


template <class MSGFORMAT>
int TCP_CLIENT<MSGFORMAT>::SetHBMsg(const void * msg, int msglen) 
{
	if ( msg == NULL ) return 0;
	if ( msglen <= 0 ) return 0;
	
	hbmsglen = 0;
	
	if ( hbmsg )
	{
		free(hbmsg);
		hbmsg = NULL;
	}
	
	hbmsg = (unsigned char *) malloc(msglen+1);
	if ( hbmsg )
	{
		memcpy(hbmsg, msg, msglen+1);
		hbmsglen = msglen;
		return 1;
	}
	
	return 0;
}


template <class MSGFORMAT>
int TCP_CLIENT<MSGFORMAT>::Send(const void * msg, int msglen, int loglevel)
{
	if ( msg == NULL ) return 0;
	if ( msglen <= 0 ) return 0;
	if ( sockfd <= 0 ) return 0;
	
	tcpmsg.write2list(sockfd, msg, msglen, loglevel);
	msg_send_count++;
	
	return 1;
}


template <class MSGFORMAT>
int TCP_CLIENT<MSGFORMAT>::Close() 
{
	if ( flag_thread_state )
	{
		flag_thread_exit = 1;
		
		time_t t11 = time(NULL);
		
		while ( flag_thread_exit )
		{
			myLOG1(LOG6, "TCP_CLIENT: wait thread exit...\n");
			#ifdef __WIN32__
				Sleep(200);
			#else
				struct timeval tv_sleep;
				tv_sleep.tv_sec = 0;
				tv_sleep.tv_usec = 200 * 1000;
				select(0, NULL, NULL, NULL, &tv_sleep);
			#endif
			
			if ( time(NULL) >= t11 + 5 )
			{
				//避免死循环
				myLOG1(LOG1, "TCP_CLIENT: don't wait thread exit\n");
				break;
			}
		}
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCP_CLIENT<MSGFORMAT>::Connect(const string & ip, int port, int recv_msg_timeout1, int send_hb_interval1, int re_connect_interval1)
{
	if ( ip.length() == 0 ) return 0;
	if ( port <= 0 ) return 0;
	
	connect_count_try = 0;
	connect_count_success = 0;
	connect_count_fail = 0;
	msg_send_count = 0;
	msg_recv_count = 0;
	
	server_ip = ip;
	server_port = port;
	recv_msg_timeout = recv_msg_timeout1;
	send_hb_interval = send_hb_interval1;
	re_connect_interval = re_connect_interval1;
	
	#ifdef __WIN32__
    {
		WORD wVersionRequested;
		WSADATA wsaData;
		int err;
		  
		wVersionRequested = MAKEWORD( 2, 0 );
		 
		err = WSAStartup( wVersionRequested, &wsaData );
		if ( err != 0 ) {
		    /* Tell the user that we couldn't find a usable */
		    /* WinSock DLL.                                  */
		    return 0;
		}
		 
		/* Confirm that the WinSock DLL supports 2.0.*/
		/* Note that if the DLL supports versions greater    */
		/* than 2.0 in addition to 2.0, it will still return */
		/* 2.0 in wVersion since that is the version we      */
		
		/* requested.                                        */
		 
		if ( LOBYTE( wsaData.wVersion ) != 2 ||
		        HIBYTE( wsaData.wVersion ) != 0 ) {
		    /* Tell the user that we couldn't find a usable */
		    /* WinSock DLL.                                  */
		    WSACleanup( );
		    return 0; 
		}
		 
		/* The WinSock DLL is acceptable. Proceed. */
	}
    #endif
	
	#ifdef __WIN32__
		if ( CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread_deal_tcp, (void*)this, 0, NULL) == NULL )
		{
			myLOG1(LOG1, "TCP_CLIENT: CreateThread() error\n");
			return 0;
		}
	#else
		pthread_t thrid1;
		if ( pthread_create(&thrid1, NULL, thread_deal_tcp, (void*)this) != 0 )
		{
			myLOG1(LOG1, "TCP_CLIENT: pthread_create() error\n");
			return 0;
		}
	#endif
	
	return 1;
}


template <class MSGFORMAT>
void * TCP_CLIENT<MSGFORMAT>::thread_deal_tcp(void * arg)
{
	#ifdef __WIN32__
	
	#else
		pthread_detach(pthread_self());
	#endif
	
	TCP_CLIENT<MSGFORMAT> * pc = (TCP_CLIENT<MSGFORMAT> *) arg;
	
	//线程开始运行
	pc->flag_thread_state = 1;
	
	pc->sockfd = 0;
	pc->t_connect = 0;	//t_connect为0表示线程第一次连接，要不要重连以及重连的时间间隔由re_connect_interval来控制
	pc->t_hb = 0;
	pc->t_recv = 0;
	
	myLOG2(LOG6, "TCP_CLIENT: thread start\n");
	
	
	fd_set fds_t;
	fd_set w_fds_t;
	struct timeval tvSelect;
	
	int retval = 0;
	unsigned char buf[8*1024] = {0};
	
	int connect_fail_count = 0;
	
	
	while ( pc->flag_thread_exit == 0 )
	{
		//re-connect()
		if ( pc->sockfd <= 0 && pc->server_port > 0 
			&& ( pc->t_connect == 0 || (pc->re_connect_interval && time(NULL) >= pc->t_connect + pc->re_connect_interval) )
			)
		{
			string server_true_ip = pc->server_ip;
			
			//DNS查询IP
			if ( !is_ip(server_true_ip.c_str()) )
			{
				#ifdef __WIN32__
				{
					struct hostent * hostname;   
					struct in_addr l_Serip;   
					
					if ( !(hostname = gethostbyname(server_true_ip.c_str())) )	//主机名
					{
						myLOG2(LOG1, "TCP_CLIENT: DNS无法获取主机IP地址, error\n");
						server_true_ip = "";
					}
					else
					{
						l_Serip.s_addr = *((unsigned int *) hostname->h_addr_list[0]);	//整型的IP地址
						server_true_ip = inet_ntoa(l_Serip);	//转换成字符的IP地址
						myLOG2(LOG4, "TCP_CLIENT: DNS QUERY: %s -> %s\n", pc->server_ip.c_str(), server_true_ip.c_str());
					}
				}
				#endif
			}
			
			pc->connect_count_try++;
			
			if ( server_true_ip.length() > 0 && pc->sock_connect(pc->server_port, inet_addr(server_true_ip.c_str()), &pc->sockfd) )
			{
				pc->connect_count_success++;
				
				//connect success
				myLOG2(LOG4, "TCP_CLIENT: connect to '%s:%d' ok, sockfd=%d\n", server_true_ip.c_str(), pc->server_port, pc->sockfd);
				pc->t_hb = time(NULL);
				pc->t_recv = time(NULL);
				pc->tcpmsg.clear(pc->sockfd);
				if (pc->flag_thread_exit) break;
				
				connect_fail_count = 0;
				
				//send Login
				if ( pc->loginmsglen )
				{
					pc->tcpmsg.write2list(pc->sockfd, pc->loginmsg, pc->loginmsglen, 6);
					pc->msg_send_count++;
				}
				
				if ( callbackfunc ) (*callbackfunc)(TCP_CONNECTED, (char *)server_true_ip.c_str(), pc->server_port);
			}
			else
			{
				pc->connect_count_fail++;
				
				//connect failed
				pc->sockfd = 0;
				if (pc->flag_thread_exit) break;
				
				//连续的连接失败就不需要反馈了，避免太多信息
				if ( connect_fail_count == 0 )
				{
					if ( server_true_ip.length() == 0 ) server_true_ip = pc->server_ip;
					
					myLOG2(LOG4, "TCP_CLIENT: connect to '%s:%d' failed\n", server_true_ip.c_str(), pc->server_port);
					
					if ( callbackfunc ) (*callbackfunc)(TCP_CONNECT_FAILED, (char *)server_true_ip.c_str(), pc->server_port);
				}
				
				connect_fail_count++;
			}
			
			pc->t_connect = time(NULL);
		}
		
		
		//Heart Bit msg if needed
		if ( pc->sockfd > 0 && pc->send_hb_interval && time(NULL) >= pc->t_hb + pc->send_hb_interval )
		{
			if ( pc->hbmsglen )
			{
				pc->tcpmsg.write2list(pc->sockfd, pc->hbmsg, pc->hbmsglen, 100);
				pc->msg_send_count++;
			}
			
			pc->t_hb = time(NULL);
		}
		
		
		FD_ZERO(&fds_t);
		if ( pc->sockfd > 0 ) FD_SET(pc->sockfd, &fds_t);
		
		FD_ZERO(&w_fds_t);
		pc->tcpmsg.FD_SET2(&w_fds_t);
		
		
		tvSelect.tv_sec = 0;
	    tvSelect.tv_usec = 200 * 1000;
	    
	    
	    #ifdef __WIN32__
	    	if ( pc->sockfd <= 0 )
	    	{
	    		Sleep(1000);
	    		continue;
	    	}
	    #endif
	    
	    
        if ( select(pc->sockfd+1, &fds_t, &w_fds_t, NULL, &tvSelect) < 0 )
        {
        	int ierr = errno;
        	if (pc->flag_thread_exit) break;
        	
        	if ( ierr == EINTR )
        	{
				continue;
			}
			else
			{
				myLOG2(LOG1, "TCP_CLIENT select() error=%d\n", ierr);
				pc->tcpmsg.notify_socket_closed(pc->sockfd);
				#ifdef __WIN32__
				closesocket(pc->sockfd);
				#else
				shutdown(pc->sockfd, SHUT_WR);
				close(pc->sockfd);
				#endif
				pc->sockfd = 0;
				
				if ( callbackfunc ) (*callbackfunc)(TCP_ERROR, NULL, 0);
				
				continue;
			}
		}
		
		
		if (pc->flag_thread_exit) break;
		
		
		if ( pc->sockfd > 0 && FD_ISSET(pc->sockfd, &fds_t) )
		{
			retval = 0;
			bzero(buf, sizeof(buf));
			
			#ifdef __WIN32__
			if ( (retval = recv(pc->sockfd, (char *)buf, sizeof(buf)-1, 0)) <= 0 )
			#else
			if ( (retval = read(pc->sockfd, buf, sizeof(buf)-1)) <= 0 )
			#endif
			{
				if ( retval < 0 ) if ( errno == EINTR ) continue;
				
				//read error
				if ( pc->sockfd > 0 )
				{
					myLOG2(LOG4, "TCP_CLIENT: other side close sockfd=%d\n", pc->sockfd);
					pc->tcpmsg.notify_socket_closed(pc->sockfd);
					#ifdef __WIN32__
					closesocket(pc->sockfd);
					#else
					shutdown(pc->sockfd, SHUT_WR);
					close(pc->sockfd);
					#endif
					pc->sockfd = 0;
					
					if ( callbackfunc ) (*callbackfunc)(TCP_CLOSED, NULL, 0);
				}
				
				continue;
			}
			
			pc->tcpmsg.store(pc->sockfd, buf, retval);
			
			while ( (retval = pc->tcpmsg.getOneMsg(pc->sockfd, buf, sizeof(buf)-1)) > 0 )
			{
				pc->t_recv = time(NULL);
				
				pc->msg_recv_count++;
				
				if ( callbackfunc ) (*callbackfunc)(TCP_RECV_MSG, buf, retval);
			}
		}
		
		
		//发送消息
		int errorsocket = 0;
		if ( pc->tcpmsg.list2socket(errorsocket) == 0 )
		{
			//error
			if ( errorsocket > 0 )
			{
				if ( pc->sockfd != errorsocket )
				{
					myLOG2(LOG1, "TCP_CLIENT: list2socket() error, sockfd=%d, errorsocket=%d\n", pc->sockfd, errorsocket);
				}
				
				myLOG2(LOG1, "TCP_CLIENT: list2socket() error, sockfd=%d\n", pc->sockfd);
				pc->tcpmsg.notify_socket_closed(pc->sockfd);
				#ifdef __WIN32__
				closesocket(pc->sockfd);
				#else
				shutdown(pc->sockfd, SHUT_WR);
				close(pc->sockfd);
				#endif
				pc->sockfd = 0;
				
				if ( callbackfunc ) (*callbackfunc)(TCP_ERROR, NULL, 0);
				
				continue;
			}
		}
		
		
		//check HB timeout
		if ( pc->sockfd > 0 && pc->recv_msg_timeout && time(NULL) >= pc->t_recv + pc->recv_msg_timeout )
		{
			myLOG2(LOG4, "TCP_CLIENT: sockfd=%d timeout for %ld seconds, error, close socket\n", pc->sockfd, time(NULL) - pc->t_recv);
			pc->tcpmsg.notify_socket_closed(pc->sockfd);
			#ifdef __WIN32__
			closesocket(pc->sockfd);
			#else
			shutdown(pc->sockfd, SHUT_WR);
			close(pc->sockfd);
			#endif
			pc->sockfd = 0;
			
			if ( callbackfunc ) (*callbackfunc)(TCP_MSG_TIMEOUT, NULL, 0);
		}
		
	}//end of while()
	
	
	if ( pc->sockfd > 0 )
	{
		myLOG2(LOG4, "TCP_CLIENT: close sockfd=%d\n", pc->sockfd);
		pc->tcpmsg.notify_socket_closed(pc->sockfd);
		#ifdef __WIN32__
		closesocket(pc->sockfd);
		#else
		shutdown(pc->sockfd, SHUT_WR);
		close(pc->sockfd);
		#endif
		pc->sockfd = 0;
		
		if ( callbackfunc ) (*callbackfunc)(TCP_COMPLETE, NULL, 0);
	}
	
	
	//线程退出
	pc->flag_thread_state = 0;
	pc->flag_thread_exit = 0;
	myLOG2(LOG6, "TCP_CLIENT: thread exit\n\n");
	
	return NULL;
}


#endif

