#include "StdAfx.h"
#include "n_common.h"


// g++ -c *.cpp
// ar -rc libn_common.a *.o
#ifdef WIN32
nt_thread nthread_create(func_thread p_func, void* param, t_uint ui_size)
{
	DWORD dwthrid;
	nt_thread thr_hand;
	thr_hand = CreateThread(NULL, ui_size,(LPTHREAD_START_ROUTINE)p_func, param, 0, &dwthrid);
	if(NULL == thr_hand)		return NULL;
	ResumeThread(thr_hand);
	return thr_hand;
}
t_int nthread_waitexit(nt_thread thr_hand)
{
	if(WaitForSingleObject(thr_hand, INFINITE)!= WAIT_OBJECT_0)
		return -2;
	return 0;
}
t_void nthread_destroy(nt_thread thr_hand)
{
	CloseHandle(thr_hand);
}
#else
nt_thread nthread_create(func_thread p_func, void* param, t_uint ui_size)
{
	nt_thread thr_hand;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	if(ui_size) pthread_attr_setstacksize(&attr,ui_size);
	if( 0 != (pthread_create(&thr_hand, &attr, p_func, param)))
	{
		pthread_attr_destroy(&attr);
		return 0;
	}
	return thr_hand;
}

t_int nthread_waitexit(nt_thread thr_hand)
{
	pthread_join(thr_hand, NULL);
	return 0;
}
t_void nthread_destroy(nt_thread thr_hand)	{ }
#endif	// ifdef WIN32





// * ================================================================================ *
// @ nsocket_oriaddr
// * ================================================================================ *
t_int nsocket_oriaddr(struct sockaddr_in* addr, t_ushort u_port, t_pcstr s_ip)
{
	memset(addr, 0, sizeof(struct sockaddr_in));
	if(!s_ip || !s_ip[0])	addr->sin_addr.s_addr=htonl(INADDR_ANY);
	else					addr->sin_addr.s_addr=inet_addr(s_ip);
	addr->sin_family = AF_INET;
	addr->sin_port = htons(u_port);
	return 0;
}
/*
// * ================================================================================ *
// @ nsocket_getnamebyaddr
// * ================================================================================ *
t_pcstr nsocket_getnamebyaddr(t_pcstr s_ip, char* s_name)
{
	struct hostent* pHost;
	t_uint ipaddr;

	ipaddr = inet_addr(s_ip);
	if((pHost = gethostbyaddr((t_pcstr)(&ipaddr), 4, AF_INET)))
	{
		strncpy(s_name, pHost->h_name, 256-1);
		return s_name;
	}
	return "";
}

// * ================================================================================ *
// @ nsocket_getaddrbyname
// * ================================================================================ *
t_void nsocket_getaddrbyname(t_pcstr s_name, t_array* p_array)
{
	struct hostent* pHostent;
	t_char s_convert[256];
	t_char **pptr = NULL;

	if(!s_name || !s_name[0]) {
		gethostname(s_convert, sizeof(s_convert));
		pHostent = gethostbyname(s_convert);
	}
	else {
		pHostent = gethostbyname(s_name);
	}
	if(NULL != pHostent) {
		switch(pHostent->h_addrtype)
		{
		case AF_INET:
		case AF_INET6:
			pptr=pHostent->h_addr_list;
			for(;*pptr!=NULL;pptr++) {
//				inet_ntop(pHostent->h_addrtype, *pptr, s_convert, sizeof(s_convert));
				narray_add(p_array, s_convert, -1);
			}
			break;
		default:
			printf("unknown address type\n");
			break;
		}
	}
}
*/

// * ================================================================================ *
// @ nsocket_open_bind
// * ================================================================================ *
t_socket nsocket_open_bind(t_ushort u_bport, t_pcstr s_ip)
{
	t_socket sock;
	struct sockaddr_in addr;

	nsocket_oriaddr(&addr, u_bport, s_ip);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	{
		t_int useaddr = 1;
		t_int len = sizeof(int);
		nsocket_set_option(sock, SOL_SOCKET, SO_REUSEADDR, (t_pcstr)(&useaddr), len);
	}

	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		nsocket_close(sock);
		return INVALID_SOCKET;
	}
	if(listen(sock, 5) == SOCKET_ERROR)
	{
		nsocket_close(sock);
		return INVALID_SOCKET;
	}
	return sock;
}

// * ================================================================================ *
// @ nsocket_open_accept
// * ================================================================================ *
t_socket nsocket_open_accept(t_socket sersock)
{
	struct sockaddr_in addr;
	t_socket sock;

	t_socketlen addrlen = sizeof(addr);
	if(INVALID_SOCKET == (sock = accept(sersock, (struct sockaddr *)&addr, &addrlen)) )
	{
		return INVALID_SOCKET;
	}
	{
		t_int ttmout = 2000;
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char *)(&ttmout), sizeof(ttmout));
	}
	return sock;
}

// * ================================================================================ *
// @ nsocket_open_connect
// * ================================================================================ *
t_socket nsocket_open_connect(t_ushort port, t_pcstr ip)
{    
	unsigned int host = inet_addr(ip);
	
	if ( host == 0 ) return INVALID_SOCKET;
	if ( host == 0xFFFFFFFF ) return INVALID_SOCKET;
	
	if ( port == 0 ) return INVALID_SOCKET;
	
	struct sockaddr_in serv_adr;
	int	   setFlag; 
	struct linger linger;
	
	t_socket sock = 0;
	
	if ( (sock = socket(AF_INET, SOCK_STREAM, 6)) < 0 ) 
	{
		sock = 0;
		printf("TCP_CLIENT: socket() error\n");
		return INVALID_SOCKET;
	}
	
	bzero((char *)&serv_adr, sizeof(serv_adr));
	serv_adr.sin_family      = AF_INET;
	memcpy (&serv_adr.sin_addr, &host, sizeof(unsigned int));
	//serv_adr.sin_addr.s_addr = htonl(host);
	serv_adr.sin_port        = htons(port);
	
	#ifdef __WIN32__
	
	#else
	setFlag = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &setFlag, sizeof(setFlag)) < 0) 
	{
	    perror("setsockopt(SO_KEEPALIVE) error\n");
	    #ifdef __WIN32__
		closesocket(sock);
		#else
		close(sock);
		#endif
	    sock = 0;
	    printf("TCP_CLIENT: setsockopt() error\n");
		return INVALID_SOCKET;
	}
	
	linger.l_onoff = 1;		//TANG 1224
	linger.l_linger = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_LINGER, (char *) &linger, sizeof(linger)) < 0) 
	{
		perror("setsockopt(SO_LINGER) error\n");
		#ifdef __WIN32__
		closesocket(sock);
		#else
		close(sock);
		#endif
		sock = 0;
		printf("TCP_CLIENT: setsockopt() error\n");
		return INVALID_SOCKET;
	}
	#endif
	
	if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) < 0) 
	{
	    #ifdef __WIN32__
		closesocket(sock);
		#else
		close(sock);
		#endif
	    sock = 0;
	    //printf("TCP_CLIENT: connect() error\n");
	 	return INVALID_SOCKET;
	}             
	  
	return sock;
}

// * ================================================================================ *
// @ nsocket_is_datarecv
// * ================================================================================ *
t_int nsocket_is_datarecv(t_socket sock, t_int nmilsec)
{
	fd_set fdR;
	struct timeval tmout;

	if(nmilsec <= 0)			return 0;

	tmout.tv_sec = nmilsec/1000;
	tmout.tv_usec = (nmilsec%1000)*1000;

	FD_ZERO(&fdR); 
	FD_SET(sock, &fdR);
	if(select((t_int)(sock+1), &fdR, NULL, NULL, &tmout) == 0)
	{
		return 1;
	}
	return 0;
}

// * ================================================================================ *
// @ nsocket_is_datasend
// * ================================================================================ *
t_int nsocket_is_datasend(t_socket sock, t_int nmilsec)
{
	fd_set fdR;
	struct timeval tmout;

	if(nmilsec <= 0)			return 0;

	tmout.tv_sec = nmilsec/1000;
	tmout.tv_usec = (nmilsec%1000)*1000;

	FD_ZERO(&fdR); 
	FD_SET(sock, &fdR);
	if(select((t_int)(sock+1), NULL, &fdR, NULL, &tmout) == 0)
	{
		return 1;
	}
	return 0;
}

// * ================================================================================ *
// @ nsocket_senddata
// * ================================================================================ *
t_int nsocket_senddata(t_socket sock, t_pcstr ssdata, t_int nsmaxlen, t_int nmilsec)
{
	if(0 == nsocket_is_datasend(sock, nmilsec))
	{
		return nsocket_send(sock, ssdata, nsmaxlen, 0);
	}
	return -7777777;
}

// * ================================================================================ *
// @ nsocket_senddata
// * ================================================================================ *
t_int nsocket_recvdata(t_socket sock, t_pchar srdata, t_int nrmaxlen, t_int nmilsec)
{
	if(0 == nsocket_is_datarecv(sock, nmilsec))
	{
		return nsocket_recv(sock, srdata, nrmaxlen, 0);
	}
	return -7777777;
}

// * ================================================================================ *
// @ nsocket_sendlen
// * ================================================================================ *
t_int nsocket_sendlen(t_socket sock, t_pcstr ssdata, t_int sndlen, t_int nmilsec)
{
	t_int nstart, ncur;
	for(nstart = 0; nstart < sndlen;)
	{
		ncur = nsocket_senddata(sock, ssdata+nstart, sndlen-nstart, nmilsec);
		if(ncur <= 0)	return ncur;
		nstart += ncur;
	}
	return sndlen;
}

// * ================================================================================ *
// @ nsocket_recvlen
// * ================================================================================ *
t_int nsocket_recvlen(t_socket sock, t_pchar srdata, t_int rcvlen, t_int nmilsec)
{
	t_int nstart, ncur;
	if(rcvlen <= 0)	return rcvlen;
	for(nstart = 0; nstart < rcvlen;) {
		ncur = nsocket_recvdata(sock, srdata+nstart, rcvlen-nstart, nmilsec);
		if(ncur <= 0)			return ncur;
		nstart += ncur;
	}
	return rcvlen;
}

// * ================================================================================ *
// @ nsocket_getip_local
// * ================================================================================ *
t_pcstr nsocket_getip_local(t_socket sock)
{
	struct sockaddr_in inaddr;
	t_socketlen addrlen = sizeof(inaddr);
	getsockname(sock, (struct sockaddr *)&inaddr, &addrlen);
	return inet_ntoa(inaddr.sin_addr);
}

// * ================================================================================ *
// @ nsocket_getport_local
// * ================================================================================ *
t_ushort nsocket_getport_local(t_socket sock)
{
	struct sockaddr_in inaddr;
	t_socketlen addrlen = sizeof(inaddr);
	getsockname(sock, (struct sockaddr *)&inaddr, &addrlen);
	return ntohs(inaddr.sin_port);
}

// * ================================================================================ *
// @ nsocket_getip_peer
// * ================================================================================ *
t_pcstr nsocket_getip_peer(t_socket sock)
{
	struct sockaddr_in inaddr;
	t_socketlen addrlen = sizeof(inaddr);
	getpeername(sock, (struct sockaddr *)&inaddr, &addrlen);
	return inet_ntoa(inaddr.sin_addr);
}

// * ================================================================================ *
// @ nsocket_getport_peer
// * ================================================================================ *
t_ushort nsocket_getport_peer(t_socket sock)
{
	struct sockaddr_in inaddr;
	t_socketlen addrlen = sizeof(inaddr);
	getpeername(sock, (struct sockaddr *)&inaddr, &addrlen);
	return ntohs(inaddr.sin_port);
}


#ifdef WIN32
int n_file_copy(const char* s_srcfname, const char* s_dstfname)
{
	return CopyFile(s_srcfname, s_dstfname, FALSE);
}
int n_file_move(const char* s_srcfname, const char* s_dstfname)
{
	return MoveFile(s_srcfname, s_dstfname) ? 0 : 1;
}
int n_file_del(const char* s_fname)
{
	return DeleteFile(s_fname) ? 0 : 1;
}

int n_dir_create(const char* s_path)
{
	return CreateDirectory(s_path, NULL) ? 0 : 1;
}

int n_dir_findfile(const char* s_path, func_ndir_findfile_callback p_func, void* p_param)
{
	string s_ffind = s_path;
	s_ffind += "*.*"; 
	WIN32_FIND_DATA finddata;

	HANDLE hand_find = FindFirstFile(s_ffind.c_str(), &finddata);
	if(INVALID_HANDLE_VALUE == hand_find)	return -1;
	while(1) {
		if(p_func) {
			if((strcmp(finddata.cFileName,"."))&&(strcmp(finddata.cFileName,"..")))
				p_func(finddata.cFileName, p_param);
		}
		if(FALSE == FindNextFile(hand_find, &finddata))
			break;
	}
	FindClose(hand_find);
	return 0;
}

#else
#include <sys/types.h>
#include <dirent.h>
int linux_system_call(const char* s_data)
{
	return system(s_data);
}

int n_file_copy(const char* s_srcfname, const char* s_dstfname)
{
	string str = "cp";
	str += " ";
	str += s_srcfname;
	str += " ";
	str += s_dstfname;
	linux_system_call(str.c_str());
}

int n_file_move(const char* s_srcfname, const char* s_dstfname)
{
	rename(s_srcfname, s_dstfname);
	return 1;
	/*
	string str = "mv -f";
	str += " ";
	str += s_srcfname;
	str += " ";
	str += s_dstfname;
	str += " > /dev/null 2>&1";
	linux_system_call(str.c_str());
	*/
}

int n_file_del(const char* s_fname)
{
	remove(s_fname);
	return 1;
	/*
	string str = "rm -f ";
	str += s_fname;
	str += " > /dev/null 2>&1";
	linux_system_call(str.c_str());
	*/
}

int n_dir_create(const char* s_path)
{
	DIR* p_dir;
	p_dir = opendir(s_path);
	if(!p_dir) {
		string s_cmd = "mkdir ";
		s_cmd += s_path;
		linux_system_call(s_cmd.c_str());
		return 0;
	}
	closedir(p_dir);
	return 0;
}

int n_dir_findfile(const char* s_path, func_ndir_findfile_callback p_func, void* p_param)
{
	DIR* p_dir;
	struct dirent *s_dir;
	p_dir = opendir(s_path);
	if(!p_dir)	return 1;
	while(NULL != (s_dir = readdir(p_dir)))
	{
		if((strcmp(s_dir->d_name,".")==0)||(strcmp(s_dir->d_name,"..")==0))   continue;
		if(p_func)	p_func(s_dir->d_name, p_param);
	}
	closedir(p_dir);
}

#endif

static log_datastr	g_logdata;
#ifdef WIN32
#include <io.h>
// * ================================================================================ *
// @ nlog_initial
// * ================================================================================ *
int nlog_initial(const char* s_logfname, const int i_level)
{
	g_logdata.m_iflag = 77;
	if(!s_logfname)	s_logfname = "";
	strncpy(g_logdata.m_sfname, s_logfname, sizeof(g_logdata.m_sfname)-1);
	g_logdata.m_ilevel = i_level;
	g_logdata.m_pfile = fopen(g_logdata.m_sfname, "a");
	if(g_logdata.m_pfile)
		g_logdata.m_icursize = filelength(fileno(g_logdata.m_pfile));
	return 0;
}
#else
#include <sys/stat.h>
// * ================================================================================ *
// @ nlog_initial
// * ================================================================================ *
int nlog_initial(const char* s_logfname, const int i_level)
{
	g_logdata.m_iflag = 77;
	if(!s_logfname)	s_logfname = "";
	strncpy(g_logdata.m_sfname, s_logfname, sizeof(g_logdata.m_sfname)-1);
	g_logdata.m_ilevel = i_level;
	g_logdata.m_pfile = fopen(g_logdata.m_sfname, "a");
	if(g_logdata.m_pfile) {
		struct  stat  f_stat;    
		if(stat(g_logdata.m_sfname, &f_stat) != -1)		g_logdata.m_icursize = (int)f_stat.st_size;
		else																					g_logdata.m_icursize = 0;
	}	
	return 0;
}
#endif

// * ================================================================================ *
// @ nlog_logit
// * ================================================================================ *
int nlog_logit(const int loglevel, const int i_line, const char *s_fname, const char *sformat, ...)
{
	// 判断初始化标记
	if(77 != g_logdata.m_iflag)	nlog_initial(NLOG_DEFAULT_FILENAME, 0);
	// 判断日志级别
	if(g_logdata.m_ilevel > loglevel)	return 1;

	// 移动文件
	if(g_logdata.m_pfile) {
		if(g_logdata.m_icursize > NLOG_MAXFILE_LEN) {
			time_t curtm;
			struct tm* ptm;
			time(&curtm);
			ptm = localtime(&curtm);
			char s_dstfname[1024];

			fclose(g_logdata.m_pfile);

			sprintf(s_dstfname, "%s.%04d%02d%02d_%02d%02d%02d", g_logdata.m_sfname, ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
			n_file_move(g_logdata.m_sfname, s_dstfname);
			g_logdata.m_pfile = fopen(g_logdata.m_sfname, "a");
			g_logdata.m_icursize = 0;
		}
	}

	// 写入日志前缀 
	{
		const char* s_find;
		while(NULL != (s_find = strstr(s_fname, "\\"))) s_fname = (s_find+1);
		while(NULL != (s_find = strstr(s_fname, "/"))) s_fname = (s_find+1);

		time_t curtm;
		struct tm* ptm;
		time(&curtm);
		ptm = localtime(&curtm);
		if(g_logdata.m_pfile)	fprintf(g_logdata.m_pfile, "[%02d:%02d:%02d] [%d %s] ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec, i_line, s_fname);
		else					fprintf(stdout, "[%02d:%02d:%02d] [%d %s] ", ptm->tm_hour, ptm->tm_min, ptm->tm_sec, i_line, s_fname);
	}

	// 写入日志数据
	int n;
	char* sdata = NULL;
	va_list ap;
	va_start (ap, sformat);
	int taille=64;
	while (1)
	{
		sdata=(char *)realloc(sdata, taille);
		if (sdata == NULL)
		{
			return -1;
		}
		n = vsnprintf (sdata, taille, sformat, ap);

		if (n >= 0 && n < taille)
		{
			break;
		}

		taille += 64;
	}
	if(g_logdata.m_pfile)	{ fprintf(g_logdata.m_pfile, "%s\n", sdata); fflush(g_logdata.m_pfile); }
	else					fprintf(stdout, "%s\n", sdata);
	va_end (ap);
	// 写入完成处理
	g_logdata.m_icursize += strlen(sdata)+100;
	return 0;
}


// * ================================================================================ *
// @ application_envinit
// * ================================================================================ *
t_int application_envinit()
{
#ifndef WIN32
	setpgrp();
	switch(fork())
	{
	case 0:
		break;
	default:
		exit(0);
		break;
	}
#endif
	return 0;
}
