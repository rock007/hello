#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

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
	#include <sys/socket.h>
	#include <sys/uio.h>
	#include <sys/time.h>
#endif

#ifdef __WIN32__
#include "mutex_win.h"
#else
#include <boost/thread/recursive_mutex.hpp>
#endif

#include "logfile.h"
#include "tcpmsg.h"

#define sLOG1 if ( plog ) plog->logwrite
#define sLOG2 if ( pc->plog ) pc->plog->logwrite


#ifdef __WIN32__
	typedef int	ssize_t;
	typedef int socklen_t;
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


#define TCP_RECV_MSG	0	//收到消息，msg和msglen有效
#define TCP_CLOSED		1	//对方关闭连接
#define TCP_MSG_TIMEOUT	2	//接收消息超时，自己主动关闭连接
#define TCP_MSG_ERROR	3	//接收消息格式错误
#define TCP_CONNECTED	4	//建立连接
#define TCP_COMPLETE	5	//自己主动关闭连接
#define TCP_SERVER_CLOSED	10	//TCP SERVER结束运行


struct client_sock_info_t
{
	string ip;
	int port;
	time_t t_accept;
	
	time_t t_recv;
	
	int msgcount;	//发出消息++，收到消息--，值最小的表示最空闲。使用的前提是消息必须成对。
	
	string sockname;
};


template <class MSGFORMAT>
class TCP_SERVER
{
private:
	TCPMSG<MSGFORMAT> tcpmsg;
	
	LogFile * plog;
	
	map<int,client_sock_info_t> client_sock_info;
	map<string,int> client_sock_name;
	
	int recv_msg_timeout;
	
	int Listener;
	int server_port;
	
	int flag_thread_state;	//0-线程没有运行，1-线程正在运行。不需要锁保护。
	int flag_thread_exit;	//通知线程退出时置1，线程退出前置0。可能不需要锁保护。
	
	boost::recursive_mutex mutex;

private:
	char * string_addr(unsigned int ip)
	{
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));	
		memcpy(&addr.sin_addr.s_addr, &ip, 4);
		return inet_ntoa(addr.sin_addr);
	}
	
	static void * thread_deal_tcp(void * arg);
	
	int close_socket(int sockfd, const string & sockname);
	
	int init_serv_sock(int port, int *sock, unsigned int ip = 0);

public:
	TCP_SERVER()
	{
		plog = NULL;
		
		recv_msg_timeout = 0;
		
		Listener = 0;
		server_port = 0;
		
		flag_thread_state = 0;
		flag_thread_exit = 0;
	}
	
	~TCP_SERVER()
	{
		Close();
		
		client_sock_info.clear();
		client_sock_name.clear();
		
		#ifdef __WIN32__
		WSACleanup( );
		#endif
	}
	
protected:
	static void (*callbackfunc)(int state, int sockfd, const string & sockname, const string & ip, int port, void * msg, int msglen);

public:
	int	SetpLog(LogFile * plog1);
	
	int	SetCallBackFunc(void (*func)(int state, int sockfd, const string & sockname, const string & ip, int port, void * msg, int msglen));
	
	//启动线程
	int Listen(int port, int recv_msg_timeout1 = 0);
	
	int Send(int sockfd, const void * msg, int msglen, int loglevel = 6);
	int Send(const string & sockname, const void * msg, int msglen, int loglevel = 6);
	int SendToAll(const void * msg, int msglen, int loglevel = 6);
	int SendToIdleOne(const void * msg, int msglen, int loglevel = 6);	//msgcount值最小的表示最空闲
	
	int SetSocketName(int sockfd, const string & sockname);
	
	int CloseSocket(int sockfd);
	int CloseSocket(const string & sockname);
	
	//结束线程
	int Close();
};


template <class MSGFORMAT> 
void (* TCP_SERVER<MSGFORMAT>::callbackfunc)(int state, int sockfd, const string & sockname, const string & ip, int port, void * msg, int msglen) = NULL;


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::SetCallBackFunc(void (*func)(int state, int sockfd, const string & sockname, const string & ip, int port, void * msg, int msglen))
{
	callbackfunc = func;
	return 1;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::SetpLog(LogFile * plog1) 
{
	plog = plog1;
	tcpmsg.SetpLog(plog1);
	return 1;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::Send(int sockfd, const void * msg, int msglen, int loglevel)
{
	if ( msg == NULL ) return 0;
	if ( msglen <= 0 ) return 0;
	if ( sockfd <= 0 ) return 0;
	
	tcpmsg.write2list(sockfd, msg, msglen, loglevel);
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	map<int,client_sock_info_t>::iterator it = client_sock_info.find(sockfd);
	if ( it != client_sock_info.end() )
	{
		it->second.msgcount++;
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::Send(const string & sockname, const void * msg, int msglen, int loglevel)
{
	if ( msg == NULL ) return 0;
	if ( msglen <= 0 ) return 0;
	if ( sockname.empty() ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	map<string,int>::iterator it = client_sock_name.find(sockname);
	if ( it != client_sock_name.end() )
	{
		return Send(it->second, msg, msglen, loglevel);
	}
	else
	{
		return 0;
	}
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::SendToAll(const void * msg, int msglen, int loglevel)
{
	if ( msg == NULL ) return 0;
	if ( msglen <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	for ( map<int,client_sock_info_t>::iterator it = client_sock_info.begin(); it != client_sock_info.end(); ++it )
	{
		Send(it->first, msg, msglen, loglevel);
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::SendToIdleOne(const void * msg, int msglen, int loglevel)	//msgcount值最小的表示最空闲
{
	if ( msg == NULL ) return 0;
	if ( msglen <= 0 ) return 0;
	
	int sockfd = 0;
	int msgcount = 0x7FFFFFFF;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	for ( map<int,client_sock_info_t>::iterator it = client_sock_info.begin(); it != client_sock_info.end(); ++it )
	{
		if ( it->second.msgcount < msgcount )
		{
			msgcount = it->second.msgcount;
			sockfd = it->first;
		}
	}
	
	return Send(sockfd, msg, msglen, loglevel);;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::Close() 
{
	if ( flag_thread_state )
	{
		flag_thread_exit = 1;
		
		while ( flag_thread_exit )
		{
			sLOG1(LOG6, "TCP_SERVER: wait thread exit...\n");
			#ifdef __WIN32__
				Sleep(200);
			#else
				struct timeval tv_sleep;
				tv_sleep.tv_sec = 0;
				tv_sleep.tv_usec = 200 * 1000;
				select(0, NULL, NULL, NULL, &tv_sleep);
			#endif
		}
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::Listen(int port, int recv_msg_timeout1)
{
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
    
	server_port = port;
	recv_msg_timeout = recv_msg_timeout1;
	
	//create TCP listener
	if ( init_serv_sock(server_port, &Listener) == 0 )
	{
		sLOG1(LOG1, "TCP_SERVER: create TCP server at port %d error\n", server_port);
		printf("TCP_SERVER: create TCP server at port %d error\n", server_port);
		return 0;
	}
	else
	{
		sLOG1(LOG4, "TCP_SERVER: tcp port %d ok\n", server_port);
	}
	
	#ifdef __WIN32__
		if ( CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)thread_deal_tcp, (void*)this, 0, NULL) == NULL )
		{
			sLOG1(LOG1, "TCP_SERVER: CreateThread() error\n");
			return 0;
		}
	#else
		pthread_t thrid1;
		if ( pthread_create(&thrid1, NULL, thread_deal_tcp, (void*)this) != 0 )
	    {
			sLOG1(LOG1, "TCP_SERVER: pthread_create() error\n");
			return 0;
	    }
	#endif
	
	return 1;
}


template <class MSGFORMAT>
void * TCP_SERVER<MSGFORMAT>::thread_deal_tcp(void * arg)
{
	#ifdef __WIN32__
	
	#else
		pthread_detach(pthread_self());
	#endif
	
	TCP_SERVER<MSGFORMAT> * pc = (TCP_SERVER<MSGFORMAT> *) arg;
	
	//线程开始运行
	pc->flag_thread_state = 1;
	
	sLOG2(LOG6, "TCP_SERVER: thread start\n");
	
	fd_set fds_t;
	fd_set w_fds_t;
	struct timeval tvSelect;
	
	time_t tCheckSocket = time(NULL);
	
	struct sockaddr_in clt_adr;
	socklen_t cliLen;
	int newSock = 0;
	
	int retval = 0;
	unsigned char buf[8*1024] = {0};
	
	
	while ( pc->flag_thread_exit == 0 )
	{
		FD_ZERO(&fds_t);
		if ( pc->Listener > 0 ) FD_SET(pc->Listener, &fds_t);
		
		{
			boost::recursive_mutex::scoped_lock lock(pc->mutex);
			
			for ( map<int,client_sock_info_t>::iterator it = pc->client_sock_info.begin(); it != pc->client_sock_info.end(); ++it )
			{
				if ( it->first > 0 )
				{ 
					FD_SET(it->first, &fds_t);
				}
			}
		}
		
		
		FD_ZERO(&w_fds_t);
		pc->tcpmsg.FD_SET2(&w_fds_t);
		
		
		tvSelect.tv_sec = 0;
	    tvSelect.tv_usec = 200 * 1000;
	    
	    
        if ( select(FD_SETSIZE, &fds_t, &w_fds_t, NULL, &tvSelect) < 0 )
        {
        	int ierr = errno;
        	if (pc->flag_thread_exit) break;
        	
        	if ( ierr == EINTR )
        	{
				continue;
			}
			else
			{
				sLOG2(LOG1, "TCP_SERVER select() error=%d\n", ierr);
				break;
			}
		}
		
		
		//client
		{
			boost::recursive_mutex::scoped_lock lock(pc->mutex);
			
			for ( map<int,client_sock_info_t>::iterator it = pc->client_sock_info.begin(); it != pc->client_sock_info.end(); ++it )
			{
				if ( it->first > 0 && FD_ISSET(it->first, &fds_t) )
				{
					retval = 0;
					bzero(buf, sizeof(buf));
					
					#ifdef __WIN32__
					if ( (retval = recv(it->first, (char *)buf, sizeof(buf)-1, 0)) <= 0 )
					#else
					if ( (retval = read(it->first, buf, sizeof(buf)-1)) <= 0 )
					#endif
					{
						if ( retval < 0 ) if ( errno == EINTR ) continue;
						
						//read error
						if ( it->first > 0 )
						{
							sLOG2(LOG4, "TCP_SERVER: client close connection, sockfd=%d\n", it->first);
							if ( callbackfunc ) (*callbackfunc)(TCP_CLOSED, it->first, it->second.sockname, it->second.ip, it->second.port, NULL, 0);
							pc->close_socket(it->first, it->second.sockname);
							
							//map删除后，it无效了，不能++
							break;
						}
						
						continue;
					}
					
					pc->tcpmsg.store(it->first, buf, retval);
					
					while ( (retval = pc->tcpmsg.getOneMsg(it->first, buf, sizeof(buf)-1)) > 0 )
					{
						it->second.t_recv = time(NULL);
						it->second.msgcount--;
						
						if ( callbackfunc ) (*callbackfunc)(TCP_RECV_MSG, it->first, it->second.sockname, it->second.ip, it->second.port, buf, retval);
					}
				}
			}
		}
		
		
		//accept clients
		if ( pc->Listener > 0 && FD_ISSET(pc->Listener, &fds_t) )
		{
			bzero(&clt_adr, sizeof(clt_adr));
			cliLen = sizeof(struct sockaddr_in);
			if ( (newSock = accept(pc->Listener, (struct sockaddr*)&clt_adr, &cliLen)) < 0 )
			{
				//accept error
				int ierr = errno;
				perror("TCP_SERVER accept error");
				if (pc->flag_thread_exit) break;
				
				#ifdef __WIN32__
				if (ierr == EINTR)
				#else
				if (ierr == EINTR || ierr == ECONNABORTED)
				#endif
				{
					//ECONNABORTED	-- Posix.1g
					//EWOULDBLOCK 	-- Berkeley
					//EPROTO		-- SVR4
					continue;
				}else
				{
					sLOG2(LOG1, "TCP_SERVER accept() error=%d, EXIT now.\n", ierr);
					#ifdef __WIN32__
						Sleep(1000);
					#else
						sleep(1);
					#endif
					exit(1);
				}
			}
			else
			{
				//accept ok
				sLOG2(LOG4, "\nTCP_SERVER: connection to client '%s:%d' established, sockfd=%d\n", 
					inet_ntoa(clt_adr.sin_addr), ntohs(clt_adr.sin_port), 
					newSock);
				
				pc->tcpmsg.clear(newSock);
				
				client_sock_info_t client1;
				client1.ip = inet_ntoa(clt_adr.sin_addr);
				client1.port = ntohs(clt_adr.sin_port);
				client1.t_accept = time(NULL);
				client1.t_recv = time(NULL);
				client1.msgcount = 0;
				client1.sockname = "";
				
				{
					boost::recursive_mutex::scoped_lock lock(pc->mutex);
					
					pc->client_sock_info.insert(map<int,client_sock_info_t>::value_type(newSock, client1));
				}
				
				sLOG2(LOG6, "TCP_SERVER: client_sock_info.insert(%d)\n", newSock);
				
				if ( callbackfunc ) (*callbackfunc)(TCP_CONNECTED, newSock, "", inet_ntoa(clt_adr.sin_addr), ntohs(clt_adr.sin_port), NULL, 0);
			}
		}
		
		
		//发送消息
		int errorsocket = 0;
		if ( pc->tcpmsg.list2socket(errorsocket) == 0 )
		{
			//error
			if ( errorsocket > 0 )
			{
				sLOG2(LOG1, "TCP_SERVER: list2socket() error, sockfd=%d\n", errorsocket);
				pc->CloseSocket(errorsocket);
			}
		}
		
		
		if ( time(NULL) >= tCheckSocket + 5 )
		{
			boost::recursive_mutex::scoped_lock lock(pc->mutex);
			
			for ( map<int,client_sock_info_t>::iterator it = pc->client_sock_info.begin(); it != pc->client_sock_info.end(); )
			{
				if ( pc->recv_msg_timeout && time(NULL) >= it->second.t_recv + pc->recv_msg_timeout )
				{
					sLOG2(LOG4, "TCP_SERVER: sockfd=%d, HB timeout for %ld seconds, error\n", it->first, time(NULL) - it->second.t_recv);
					if ( callbackfunc ) (*callbackfunc)(TCP_MSG_TIMEOUT, it->first, it->second.sockname, it->second.ip, it->second.port, NULL, 0);
					pc->close_socket(it->first, it->second.sockname);
					
					//map删除后，it无效了，不能++
					it = pc->client_sock_info.begin();
					continue;
				}
				
				++it;
			}
			
			tCheckSocket = time(NULL);
		}
		
	}//end of while()
	
	
	//线程退出
	pc->flag_thread_state = 0;
	pc->flag_thread_exit = 0;
	sLOG2(LOG6, "TCP_SERVER: thread exit\n\n");
	
	if ( callbackfunc ) (*callbackfunc)(TCP_SERVER_CLOSED, 0, "", "", 0, NULL, 0);
	
	return NULL;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::close_socket(int sockfd, const string & sockname)
{
	if ( sockfd <= 0 ) return 0;
	
	sLOG1(LOG6, "TCP_SERVER: close sockfd=%d, sockname='%s'\n", sockfd, sockname.c_str());
	
	tcpmsg.notify_socket_closed(sockfd);
	
	#ifdef __WIN32__
	closesocket(sockfd);
	#else
	shutdown(sockfd, SHUT_WR);
	close(sockfd);
	#endif
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	client_sock_name.erase(sockname);
	client_sock_info.erase(sockfd);
	
	return 1;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::CloseSocket(int sockfd)
{
	if ( sockfd <= 0 ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	map<int,client_sock_info_t>::iterator it = client_sock_info.find(sockfd);
	if ( it != client_sock_info.end() )
	{
		if ( callbackfunc ) (*callbackfunc)(TCP_COMPLETE, it->first, it->second.sockname, it->second.ip, it->second.port, NULL, 0);
		return close_socket(sockfd, it->second.sockname);
	}
	else
	{
		if ( callbackfunc ) (*callbackfunc)(TCP_COMPLETE, sockfd, "", "", 0, NULL, 0);
		return close_socket(sockfd, "");
	}
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::CloseSocket(const string & sockname)
{
	if ( sockname.empty() ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	map<string,int>::iterator it = client_sock_name.find(sockname);
	if ( it != client_sock_name.end() )
	{
		if ( callbackfunc ) (*callbackfunc)(TCP_COMPLETE, it->second, sockname, "", 0, NULL, 0);
		return close_socket(it->second, sockname);
	}
	else
	{
		if ( callbackfunc ) (*callbackfunc)(TCP_COMPLETE, 0, sockname, "", 0, NULL, 0);
		return close_socket(0, sockname);
	}
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::SetSocketName(int sockfd, const string & sockname)
{
	if ( sockfd <= 0 ) return 0;
	if ( sockname.empty() ) return 0;
	
	boost::recursive_mutex::scoped_lock lock(mutex);
	
	for ( map<string,int>::iterator it = client_sock_name.begin(); it != client_sock_name.end(); ++it )
	{
		if ( it->first == sockname && it->second == sockfd )
		{
			return 1;
		}
		
		if ( it->first == sockname || it->second == sockfd )
		{
			sLOG1(LOG1, "TCP_SERVER: SetSocketName(%d,'%s') error, (%d,'%s') exist\n", 
				sockfd, sockname.c_str(), it->second, it->first.c_str());
			return 0;
		}
	}
	
	client_sock_name.insert(map<string,int>::value_type(sockname, sockfd));
	sLOG1(LOG6, "TCP_SERVER: client_sock_name.insert(sockfd=%d, sockname='%s')\n", sockfd, sockname.c_str());
	
	map<int,client_sock_info_t>::iterator it = client_sock_info.find(sockfd);
	if ( it != client_sock_info.end() )
	{
		it->second.sockname = sockname;
	}
	
	return 1;
}


template <class MSGFORMAT>
int TCP_SERVER<MSGFORMAT>::init_serv_sock(int port, int *sock, unsigned int ip)
{
	struct sockaddr_in serv_adr;
	struct linger linger;
    int	optval=1;  
    
	if ( (*sock = socket(AF_INET, SOCK_STREAM, 6)) < 0 ) 
	{
    	*sock = 0;
    	return 0;
  	}
	
	bzero((char *)&serv_adr, sizeof(serv_adr));
  	serv_adr.sin_family      = AF_INET;
  	//ip is already "network byte order"
  	if ( ip == 0 ) serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);  
  		else memcpy(&serv_adr.sin_addr.s_addr, &ip, 4);			//TANG 0430
	serv_adr.sin_port        = htons(port);
  	
  	#ifdef __WIN32__
	
	#else
	linger.l_onoff = 1;		//TANG 1228
	linger.l_linger = 0;
  	if (setsockopt(*sock, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger)) < 0) 
  	{
   		perror ("sockopt SO_LINGER error"); 
		close (*sock);
    	*sock = 0;
    	return 0;
	}

	optval = 1;
	if (setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) < 0) 
	{
		perror ("sockopt REUSEADDR error"); 
		close (*sock);
		*sock = 0;
		return 0;
	}
	
	optval = 1;
	if (setsockopt(*sock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) < 0) 
	{
		perror ("sockopt KEEPALIVE error"); 
		close (*sock);
		*sock = 0;
		return 0;
	}
	#endif

  	if (bind(*sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0) 
  	{
		close (*sock);
    	*sock = 0;
    	return 0;
  	}
	
    if (listen(*sock, 200) < 0)
    {
    	perror("init_serv_sock(): listen() error");
		close (*sock);
        *sock = 0;
        return 0;
    }
    
    return 1;
}


#endif

