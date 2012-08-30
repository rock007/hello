
#include "StdAfx.h"
#include <errno.h>
#include "appserver.h"

#define LOG CCLogFile.logwrite
extern LogFile CCLogFile;

extern MYSQLDB DB1;

#define MAXNUM_CLIENT	32
struct sock_client
{
	t_socket sock;
	t_int    i_type;	// 0 WEB 1 PESQ 2 FTP 3 SUM
	time_t   tm_lrcvtm;
};

struct sock_index
{
	t_int i_pesq;
	t_int i_sum;
	t_int i_ftp;
};
static sock_client g_clisock[MAXNUM_CLIENT];
static sock_index g_clisock_index;
static t_socket g_sersock;

const char* g_smsg_pesqlogin = "<head>00021</head><msg>login_pesq</msg>";
const char* g_smsg_ftplogin = "<head>00020</head><msg>login_ftp</msg>";
const char* g_smsg_sumlogin = "<head>00020</head><msg>login_sum</msg>";
const char* g_smsg_hb = "<head>00013</head><msg>hb</msg>";


int _appserver_proc_pesqroutine_2()
{
	static time_t t_check = time(NULL);
	
	if ( time(NULL) >= t_check + 60 )
	{
		t_check = time(NULL);
		
		string t_curtime_str = time_format_str(time(NULL), 40);
		
		string sql = FormatString("SELECT \
CONFIG_TERMINAL_ROUTINE.TERMINAL_ID, \
CONFIG_TERMINAL_ROUTINE.STARTTIME, \
CONFIG_TERMINAL.PLAYFILE \
FROM CONFIG_TERMINAL_ROUTINE \
Left Outer Join CONFIG_TERMINAL ON CONFIG_TERMINAL_ROUTINE.TERMINAL_ID = CONFIG_TERMINAL.TERMINAL_ID \
WHERE CONFIG_TERMINAL_ROUTINE.TYPE = 'PESQ' \
AND CONFIG_TERMINAL_ROUTINE.STARTTIME = '%s';", t_curtime_str.c_str());
		
		DATASET dbresult;
		DB1.db_select(sql.c_str(), dbresult, LOG6);
		
		for ( int i=0; i<dbresult.size(); i++ )
		{
			static CClientMsg msg_pesq_a2p;
			msg_pesq_a2p.ori_init("CA_A2P");
			msg_pesq_a2p.ori_additem("terminal", dbresult[i]["TERMINAL_ID"].c_str());
			msg_pesq_a2p.ori_additem("starttime", time_format_str(time(NULL), 1).c_str());
			if ( dbresult[i]["PLAYFILE"].length() ) msg_pesq_a2p.ori_additem("srcfname", dbresult[i]["PLAYFILE"].c_str());
			else msg_pesq_a2p.ori_additem("srcfname", "AGAM1F01.WAV");
			msg_pesq_a2p.ori_additem("test_type", "R");
			msg_pesq_a2p.ori_end();
			
			pesqsock_send(msg_pesq_a2p.packet().c_str(), msg_pesq_a2p.packet().length());
		}
	}
	
	return 1;
}


// * ================================================================================ *
// @ _appserver_proc_pesqroutine
// * ================================================================================ *
/*
t_int _appserver_proc_pesqroutine()
{
	static CNString s_curdate, s_curtm, s_curdatetime;
	static time_t curtm, lsttm;
	struct tm* p_tm;

	curtm = time(NULL);
	if(lsttm == curtm)	return 0;
	lsttm = curtm;
	p_tm = localtime(&curtm);
	s_curdate.Format("%04d%02d%02d", p_tm->tm_year+1900, p_tm->tm_mon+1, p_tm->tm_mday);
	s_curtm.Format("%02d%02d%02d", p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
	s_curdatetime = s_curdate;
	s_curdatetime += s_curtm;

	if(s_curdate != g_appserver.m_scurdate) 
	{
		static CNString sn_sql, sn_temp;
		static CSQLAPIDBSelectResult mysql_result;
		static char** sql_result;
		
		//sn_sql.Format("SELECT TERMINAL_ID, STARTTIME, TYPE, SRCFNAME FROM CONFIG_TERMINAL_ROUTINE_CONTENT_EXTRA_PESQ WHERE EXEC_DATE!='%s' ORDER BY STARTTIME;",
		//	s_curdate.c_str());
		
		sn_sql.Format("SELECT TERMINAL_ID, STARTTIME, PARA1, URL FROM CONFIG_TERMINAL_ROUTINE WHERE TYPE = 'PESQ' ORDER BY STARTTIME;");
		
		appserver_select_start(sn_sql.c_str(), mysql_result);
		
		while(NULL != (sql_result = appserver_select_fetch(mysql_result))) 
		{
			sn_temp = sql_result[1];
			sn_temp.replace(":", "");
			sn_temp += "00";
			if(sn_temp.to_int32() >= s_curtm.to_int32()) 
			{
				NPOSITION pos = g_appserver.m_termrout_pesq.Push();
				g_appserver.m_termrout_pesq[pos].s_terminal = sql_result[0];
				g_appserver.m_termrout_pesq[pos].s_starttime_ori = sql_result[1];
				g_appserver.m_termrout_pesq[pos].s_type = sql_result[2];
				if ( sql_result[3] && sql_result[3][0] ) g_appserver.m_termrout_pesq[pos].s_srcfname = sql_result[3];
				else g_appserver.m_termrout_pesq[pos].s_srcfname = "AGAM1F01.WAV";

				static CNString sn_tmp_starttime;
				sn_tmp_starttime.Format("%s00", g_appserver.m_termrout_pesq[pos].s_starttime_ori.c_str());
				sn_tmp_starttime.replace(":", "");
				g_appserver.m_termrout_pesq[pos].s_starttime = sn_tmp_starttime.c_str();
				LOG(LOG3, "CONFIG_TERMINAL_ROUTINE_PESQ\n");
			}
		}
		
		appserver_select_end(mysql_result);
		
		//sn_sql.Format("UPDATE CONFIG_TERMINAL_ROUTINE_CONTENT_EXTRA_PESQ SET EXEC_DATE='%s';",
		//	s_curdate.c_str());
		//appserver_execute(sn_sql.c_str());
		
		g_appserver.m_scurdate = s_curdate;
	}

	// 处理发送消息
	NPOSITION pos = g_appserver.m_termrout_pesq.GetHeadPosition();
	while(pos) 
	{
		if(atoi(g_appserver.m_termrout_pesq[pos].s_starttime.c_str()) > s_curtm.to_int32())	break;
		
		static CClientMsg msg_pesq_a2p;
		msg_pesq_a2p.ori_init("CA_A2P");
		msg_pesq_a2p.ori_additem("terminal", g_appserver.m_termrout_pesq[pos].s_terminal.c_str());
		msg_pesq_a2p.ori_additem("starttime", s_curdatetime.c_str());
		msg_pesq_a2p.ori_additem("srcfname", g_appserver.m_termrout_pesq[pos].s_srcfname.c_str());
		msg_pesq_a2p.ori_additem("test_type", "R");
		msg_pesq_a2p.ori_end();
		
		LOG(LOG3, "EXEC CONFIG_TERMINAL_ROUTINE_PESQ SEND DATA\n %s\n",
			msg_pesq_a2p.packet().c_str());
		
		pesqsock_send(msg_pesq_a2p.packet().c_str(), msg_pesq_a2p.packet().length());
		
		g_appserver.m_termrout_pesq.MovePosition(pos);
		
		pos = g_appserver.m_termrout_pesq.GetHeadPosition();
	}
	
	return 0;
}
*/


// * ================================================================================ *
// @ nsock_find
// * ================================================================================ *
t_int nsock_find(t_int& i_cur, const t_int i_type)
{
	int i_res = i_cur;
	for(i_cur++; i_cur < MAXNUM_CLIENT; i_cur++) {
		if(i_type == g_clisock[i_cur].i_type)
			return i_cur;
	}
	for(i_cur = 0; i_cur <= i_res; i_cur++) {
		if(i_type == g_clisock[i_cur].i_type)
			return i_cur;
	}
	i_cur = -1;
	return -1;
}

// * ================================================================================ *
// @ pesqsock_send
// * ================================================================================ *
t_int pesqsock_send(t_pcstr s_pack, const int i_packlen)
{
	nsock_find(g_clisock_index.i_pesq, 1);
	if(-1 == g_clisock_index.i_pesq)	return -1;
	nsocket_sendlen(g_clisock[g_clisock_index.i_pesq].sock, s_pack, i_packlen, 300);
	LOG(LOG4, "pesqsock_send() send sockfd=%d, msg='%s'\n", g_clisock[g_clisock_index.i_pesq].sock, s_pack);
	return 0;
}

// * ================================================================================ *
// @ sumsock_send
// * ================================================================================ *
t_int sumsock_send(t_pcstr s_pack, const int i_packlen)
{
	nsock_find(g_clisock_index.i_sum, 3);
	if(-1 == g_clisock_index.i_sum)	return -1;
	nsocket_sendlen(g_clisock[g_clisock_index.i_sum].sock, s_pack, i_packlen, 300);
	LOG(LOG4, "sumsock_send() nsocket_sendlen sockfd=%d, msg='%s'\n", g_clisock[g_clisock_index.i_sum].sock, s_pack);
	return 0;
}

// * ================================================================================ *
// @ ftpsock_send
// * ================================================================================ *
t_int ftpsock_send(t_pcstr s_pack, const int i_packlen)
{
	if(-1 != g_clisock_index.i_ftp) 
	{
		nsocket_sendlen(g_clisock[g_clisock_index.i_ftp].sock, s_pack, i_packlen, 1000);
		LOG(LOG4, "ftpsock_send() nsocket_sendlen sockfd=%d, msg='%s'\n", g_clisock[g_clisock_index.i_ftp].sock, s_pack);
		return 0;
	}
	return -1;
}


// * ================================================================================ *
// @ clisock_close
// * ================================================================================ *
t_int clisock_close(t_int i_client)
{
	if(g_clisock_index.i_ftp == i_client) {
		LOG(LOG2, "FTPSCAN exist\n");
		g_clisock_index.i_ftp = -1;
	}
	nsocket_close(g_clisock[i_client].sock);
	g_clisock[i_client].sock = INVALID_SOCKET;
	return 0;
}

// * ================================================================================ *
// @ clisock_rcvpack
// * ================================================================================ *
t_int clisock_rcvpack(t_int i_client)
{
	static t_char s_rpack[1024];
	static t_int i_plen;
	if(18 != nsocket_recvlen(g_clisock[i_client].sock, s_rpack, 18, 1000)) {
		clisock_close(i_client);
		return 0;
	}
	s_rpack[18] = 0;
	i_plen = CClientMsg::ansy_getitem(s_rpack, "head");
	if(i_plen != nsocket_recvlen(g_clisock[i_client].sock, s_rpack+18, i_plen, 1000)) {
		clisock_close(i_client);
		return 0;
	}
	i_plen += 18;
	s_rpack[i_plen] = 0;

	string s_msgtype;
	CClientMsg::ansy_getitem(s_rpack, "msg", s_msgtype);
	
	if(("HB" == s_msgtype) || ("hb" == s_msgtype)) 
	{
		nsocket_sendlen(g_clisock[i_client].sock, g_smsg_hb, strlen(g_smsg_hb), 1000);
		return 0;
	}
	
	LOG(LOG4, "READ g_clisock[%d].i_type=%d, sockfd=%d, msg='%s'\n", i_client, g_clisock[i_client].i_type, g_clisock[i_client].sock, s_rpack);
	
	if("login_pesq" == s_msgtype) 
	{
		LOG(LOG2, "PESQ login  %s\n", s_rpack);
		if(g_appserver.m_pesq_number.length() <= 0)	CClientMsg::ansy_getitem(s_rpack, "number", g_appserver.m_pesq_number);
		g_clisock[i_client].i_type = 1;
		nsocket_sendlen(g_clisock[i_client].sock, g_smsg_pesqlogin, strlen(g_smsg_pesqlogin), 1000);
		LOG(LOG4, "SEND sockfd=%d, msg='%s'\n", g_clisock[i_client].sock, g_smsg_pesqlogin);
		return 0;
	}
	else if("login_ftp" == s_msgtype) 
	{
		LOG(LOG2, "FTPSCAN login %s\n", s_rpack);
		g_clisock[i_client].i_type = 2;
		if(-1 != g_clisock_index.i_ftp)			clisock_close(g_clisock_index.i_ftp);
		g_clisock_index.i_ftp = i_client;
		nsocket_sendlen(g_clisock[i_client].sock, g_smsg_ftplogin, strlen(g_smsg_ftplogin), 1000);
		LOG(LOG4, "SEND sockfd=%d, msg='%s'\n", g_clisock[i_client].sock, g_smsg_ftplogin);
		return 0;
	}
	else if("login_sum" == s_msgtype) 
	{
		LOG(LOG2, "SUM login  %s\n", s_rpack);
		g_clisock[i_client].i_type = 3;
		nsocket_sendlen(g_clisock[i_client].sock, g_smsg_sumlogin, strlen(g_smsg_sumlogin), 1000);
		LOG(LOG4, "SEND sockfd=%d, msg='%s'\n", g_clisock[i_client].sock, g_smsg_sumlogin);
		return 0;
	}
	else 
	{
		if(1 == g_clisock[i_client].i_type) 
		{
			return appserver_proc_pesqpack(s_rpack);
		}
		else if(2 == g_clisock[i_client].i_type) 
		{
			return appserver_proc_ftppack(s_rpack);

		}
		else if(3 == g_clisock[i_client].i_type) 
		{
			return appserver_proc_sumpack(s_rpack);

		}
		else 
		{
			t_int i_ret;
			string s_resultdes;
			string s_bussopid;
			string s_tid;
			
			appserver_proc_realtimebussop(s_rpack, s_msgtype.c_str(), i_ret, s_resultdes, s_bussopid, s_tid);
			
			string outmsg = s_rpack;
			xml_put_para(outmsg, "bussopid", s_bussopid.c_str());
			xml_put_para(outmsg, "tid", s_tid.c_str());
			xml_put_para(outmsg, "result", i_ret);
			xml_put_para(outmsg, "result_des", s_resultdes.c_str());
			nsocket_sendlen(g_clisock[i_client].sock, outmsg.c_str(), outmsg.length(), 1000);
			LOG(LOG4, "SEND sockfd=%d, msg='%s'\n", g_clisock[i_client].sock, outmsg.c_str());
			
			/*
			if( (6 != s_msgtype.length()) || strcmp(s_msgtype.c_str()+2, "_W2A")) 	// 不支持的WEB业务消息
			{
				LOG(LOG1, "clisock_rcvpack() s_msgtype error\n");
				clisock_close(i_client);
				return 1;
			}
			
			t_int i_ret;
			string s_resultdes;
			string s_bussopid;
			s_msgtype.replace(2, 4, "");

			if(('a' <= s_msgtype.c_str()[1]) &&  ('z' >= s_msgtype.c_str()[1])) 
			{
				appserver_proc_webpack_cancel(s_rpack, s_msgtype.c_str(), i_ret, s_resultdes);
				static CClientMsg clientmsg;
				s_msgtype += "_A2W";
				clientmsg.ori_init(s_msgtype.c_str());
				clientmsg.ori_additem("retsult", i_ret);
				clientmsg.ori_additem("result_des", s_resultdes.c_str());
				clientmsg.ori_end();
				nsocket_sendlen(g_clisock[i_client].sock, clientmsg.packet().c_str(), clientmsg.packet().length(), 1000);
				LOG(LOG4, "SEND sockfd=%d, msg='%s'\n", g_clisock[i_client].sock, clientmsg.packet().c_str());
			}
			else if(('A' <= s_msgtype.c_str()[1]) &&  ('Z' >= s_msgtype.c_str()[1])) 
			{
				appserver_proc_realtimebussop(s_rpack, s_msgtype.c_str(), i_ret, s_resultdes, s_bussopid);
				
				static CClientMsg clientmsg;
				s_msgtype += "_A2W";
				clientmsg.ori_init(s_msgtype.c_str());
				clientmsg.ori_additem("retsult", i_ret);
				clientmsg.ori_additem("result_des", s_resultdes.c_str());
				clientmsg.ori_additem("bussop_id", s_bussopid.c_str());
				clientmsg.ori_end();
				nsocket_sendlen(g_clisock[i_client].sock, clientmsg.packet().c_str(), clientmsg.packet().length(), 1000);
				LOG(LOG4, "SEND sockfd=%d, msg='%s'\n", g_clisock[i_client].sock, clientmsg.packet().c_str());
			}
			*/
		}
	}
	return 0;
}

// * ================================================================================ *
// @ thread_sockclient
// * ================================================================================ *
t_pvoid thread_sockclient(t_pvoid p_param)
{
	t_int i, flag_add;
	fd_set fd_read;
	struct timeval sel_tmout;
	time_t curtm;

	while(g_appserver.m_runing) 
	{
		//nzq的做法：每天读一次数据库，保存在链表中，检查并删除链表
		//_appserver_proc_pesqroutine();
		
		//新的做法：每分钟读一次数据库
		//_appserver_proc_pesqroutine_2();
		
		curtm = time(NULL);
		flag_add = 0;
		FD_ZERO(&fd_read);
		for(i = 0; i < MAXNUM_CLIENT; i++) 
		{
			if(INVALID_SOCKET != g_clisock[i].sock) 
			{
				if( (curtm-g_clisock[i].tm_lrcvtm) > 120) 	// 判断超时 -- 代码习惯不好，应该单独处理，不过这样可以提高程序效率
				{
					clisock_close(i);
				}
				else 
				{
					flag_add = 1;
					FD_SET(g_clisock[i].sock, &fd_read);
				}
			}
		}
		
		if(0 == flag_add) { n_sleep(300); continue; }

		// 调用select
		sel_tmout.tv_sec = 0;
		sel_tmout.tv_usec = 50;
		if(select(FD_SETSIZE, &fd_read, NULL, NULL, &sel_tmout) < 0)
		{
			int i_err = errno;
			if(EINTR == i_err)	continue;
			else
			{
#ifdef WIN32
				LOG(LOG1, "SYSTEM ERROR SELECT_ERR[%d]\n", WSAGetLastError());
#else
				LOG(LOG1, "SYSTEM ERROR SELECT_ERR[%d]\n", errno);
#endif
				break;
			}
		}
		
		for(i = 0; i < MAXNUM_CLIENT; i++) 
		{
			if(INVALID_SOCKET != g_clisock[i].sock) 
			{
				if(FD_ISSET(g_clisock[i].sock, &fd_read)) 
				{
					g_clisock[i].tm_lrcvtm = time(NULL);
					clisock_rcvpack(i);
				}
			}
		}
	}
	
	return NULL;
}

// * ================================================================================ *
// @ thread_sockserver
// * ================================================================================ *
t_pvoid thread_sockserver(t_pvoid p_param)
{
	t_int i;
	t_socket sock_cli;

	g_sersock = INVALID_SOCKET;
	for(i = 0; i < MAXNUM_CLIENT; i++)
		g_clisock[i].sock = INVALID_SOCKET;
	g_clisock_index.i_sum = g_clisock_index.i_ftp = g_clisock_index.i_pesq = -1;

	nthread_create(thread_sockclient, NULL, 12*1024);
	
	while(g_appserver.m_runing) 
	{
		if(INVALID_SOCKET == g_sersock)
			g_sersock = nsocket_open_bind(51234, "");

		sock_cli = nsocket_open_accept(g_sersock);
		if(INVALID_SOCKET == sock_cli) 
		{
			LOG(LOG1, "nsocket_open_accept() error\n");
			nsocket_close(sock_cli);
			nsocket_close(g_sersock);
			g_sersock = INVALID_SOCKET;
			n_sleep(3000);
			continue;
		}
		
		for(i = 0; i < MAXNUM_CLIENT; i++) 
		{
			if(INVALID_SOCKET == g_clisock[i].sock) 
			{
				g_clisock[i].i_type = 0;
				g_clisock[i].tm_lrcvtm = time(NULL);
				g_clisock[i].sock = sock_cli;
				break;
			}
		}
		
		if(MAXNUM_CLIENT == i)
		{
			LOG(LOG1, "SYSTEM ERRORS connect is max\n");
			nsocket_close(sock_cli);
			n_sleep(1000);
			continue;
		}
		
		LOG(LOG3, "new connect come in  %d\n", i);
	}
	return NULL;
}
