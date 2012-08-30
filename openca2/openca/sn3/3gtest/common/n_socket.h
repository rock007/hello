/* -*- head file -*-*/

#ifndef N_SOCKET_HEAD_DEFINE_
#define N_SOCKET_HEAD_DEFINE_


#ifdef FD_SETSIZE
#undef FD_SETSIZE
#define FD_SETSIZE	1024
#endif

#include "n_type.h"

#ifdef WIN32
	#include <errno.h>
	#include <stdio.h>
	#include <winsock2.h>
	#include <WS2tcpip.h>

	typedef SOCKET		t_socket;
	typedef int			t_socketlen;

#ifndef _WIN32_WCE
	#pragma comment(lib, "Ws2_32.lib")
#endif

#else	// * linux *
	#include <stdio.h>
	#include <unistd.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>

	typedef int			t_socket;
	typedef socklen_t	t_socketlen;
	#define SOCKET_ERROR	-1
	#define INVALID_SOCKET	-1
#endif

// * ================================================== *
// @ basic socket operation
// * ================================================== *
	#define nsocket_get_option(s,level,optname,optval,optlen)	getsockopt(s,level,optname,(char*)optval,optlen)
	#define nsocket_set_option(s,level,optname,optval,optlen)	setsockopt(s,level,optname,(const char*)optval,optlen)
	#define nsocket_accept(s, addr, addrlen)					accept(s, (struct sockaddr *) addr, (int *)addrlen)
	#define nsocket_recv(s, rbuf, rlen, flag)					recv(s, (char *)rbuf, rlen, flag)
	#define nsocket_send(s, sbuf, slen, flag)					send(s, (const char *)sbuf, slen, flag)
#ifdef WIN32
	#define nsocket_env_init() { WSADATA wsaData; WSAStartup( MAKEWORD( 2, 2 ), &wsaData );}
	#define nsocket_env_clear() WSACleanup()
	#define nsocket_ioctl(s,cmd,argp,unused) ioctlsocket(s,cmd,(unsigned long*)argp)
	#define nsocket_close(s)	{ shutdown(s, 2); closesocket(s); }
#else
	#define nsocket_env_init()
	#define nsocket_env_clear()
	#define nsocket_ioctl(s,cmd,argp,argp_size) ioctl(s,cmd,argp,argp_size)
	#define nsocket_close(s)	{ shutdown(s, 2); close(s); }
#endif

/*
// * =========================================================================================== *
	extern t_void nsocket_getaddrbyname(t_pcstr s_name, t_array* p_array);
	extern t_pcstr nsocket_getnamebyaddr(t_pcstr s_ip, char* s_name);
// * =========================================================================================== *
*/

// * =========================================================================================== *
	extern t_int nsocket_oriaddr(struct sockaddr_in* addr, t_ushort u_port, t_pcstr s_ip);
	extern t_socket nsocket_open_bind(t_ushort u_bport, t_pcstr s_ip);
	extern t_socket nsocket_open_accept(t_socket sersock);
	extern t_socket nsocket_open_connect(t_ushort nport, t_pcstr sip);

	extern t_int nsocket_is_datarecv(t_socket sock, t_int nmilsec);
	extern t_int nsocket_is_datasend(t_socket sock, t_int nmilsec);

	extern t_int nsocket_recvdata(t_socket sock, t_pchar srdata, t_int nrmaxlen, t_int nmilsec);
	extern t_int nsocket_sendlen(t_socket sock, t_pcstr ssdata, t_int sndlen, t_int nmilsec);
	extern t_int nsocket_recvlen(t_socket sock, t_pchar srdata, t_int rcvlen, t_int nmilsec);
// * =========================================================================================== *

// * =========================================================================================== *
	extern t_pcstr nsocket_getip_local(t_socket sock);
	extern t_ushort nsocket_getport_local(t_socket sock);
	extern t_pcstr nsocket_getip_peer(t_socket sock);
	extern t_ushort nsocket_getport_peer(t_socket sock);
// * =========================================================================================== *

#endif
