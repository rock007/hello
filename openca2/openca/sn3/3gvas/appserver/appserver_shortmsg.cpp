#include "StdAfx.h"
#include "appserver.h"
#include "appserver_bussop.h"
#include <list>
#include <set>
using namespace std;

#define LOG CCLogFile.logwrite
extern LogFile CCLogFile;

extern list<sms_66610_t> sms66610;

extern t_int appserver_proc_rcvshortmsg(string& s_terminal, string& s_content);

extern int deal_sms_66610(const string & s_terminal);

#define CTCPP_CONNECT_REQ		1
#define CTCPP_CONNECT_RSP		-1
#define CTCPP_DISCONNECT_REQ	2
#define CTCPP_DISCONNECT_RSP	-2
#define CTCPP_ACTIVETEST_REQ	3
#define CTCPP_ACTIVETEST_RSP	-3
#define CTCPP_SUBMITSMS_REQ		4
#define CTCPP_SUBMITSMS_RSP		-4
#define CTCPP_DELIVERSMS_REQ	5
#define CTCPP_DELIVERSMS_RSP	-5
#define CTCPP_REPORTSMS_REQ		6
#define CTCPP_REPORTSMS_RSP		-6
#define CTCPP_UNKNOWN_TYPE		99
const char* s_ctcpprsp_succ="<body><result>0</result></body>";
#define CTCPP_ACTIVETEST_TIMEOUT	30
#define CTCPP_ACTIVETEST_TIMES		3

// * ================================================================================ *
// @ ctcpp_recvpack
// * ================================================================================ *
t_int ctcpp_recvpack(t_socket& sock, t_pchar s_rpack, t_int& i_len)
{
	t_pcstr s_find;
	
	if(0 == nsocket_is_datarecv(sock, 100)) 
	{
		if(100 != nsocket_recvlen(sock, s_rpack, 100, 1000)) 
		{
			LOG(LOG1, "ctcpp_recvpack() nsocket_recvlen sockfd=%d error\n", sock);
			nsocket_close(sock);
			sock = INVALID_SOCKET;
			return -1;
		}
		
		s_rpack[100] = 0;
		s_find = strstr(s_rpack, "<len>");
		if(!s_find) 
		{
			LOG(LOG1, "ctcpp_recvpack() nsocket_recvlen sockfd=%d error\n", sock);
			nsocket_close(sock);
			sock = INVALID_SOCKET;
			return -2;
		}
		
		s_find += strlen("<len>");
		i_len = atoi(s_find);
		if(i_len > 0) 
		{
			if(i_len != nsocket_recvlen(sock, s_rpack+100, i_len, 1000)) 
			{
				LOG(LOG1, "ctcpp_recvpack() nsocket_recvlen sockfd=%d error\n", sock);
				nsocket_close(sock);
				sock = INVALID_SOCKET;
				return 1;
			}
		}

		i_len += 100;
		s_rpack[i_len] = 0;
		
		if ( i_len != 100 ) LOG(LOG6, "ctcpp_recvpack() READ sockfd=%d, msg='%s'\n", sock, s_rpack);
		
		return 0;
	}
	
	return 1;
}

// * ================================================================================ *
// @ ctcpp_packtype
// * ================================================================================ *
t_int ctcpp_packtype(t_pcstr s_pack)
{
	t_pcstr s_find = strstr(s_pack, "<msgtype>");
	if(!s_find)	return CTCPP_UNKNOWN_TYPE;
	s_find += strlen("<msgtype>");
	return atoi(s_find);
}

// * ================================================================================ *
// @ ctcpp_sendpack
// * ================================================================================ *
t_int ctcpp_sendpack(t_socket& sock, t_pcstr s_sndpack, const int i_type, int loglevel = 6, int logline = __LINE__, char * logsourcefile = __FILE__)
{
	static t_char s_head[100], s_head1[101];
	static t_int i_seqid = 1;
	t_int i_sndlen;

	i_sndlen = (t_int)strlen(s_sndpack);
	memset(s_head, 'A', 100);
	sprintf(s_head1, "<head><len>%d</len><seqid>%d</seqid><msgtype>%d</msgtype></head>", i_sndlen, i_seqid, i_type);
	if(i_seqid++ > 2*1024*1024)	i_seqid = 1;
	memcpy(s_head, s_head1, strlen(s_head1));
	nsocket_sendlen(sock, s_head, 100, 100);
	if(i_sndlen > 0)	nsocket_sendlen(sock, s_sndpack, i_sndlen, 100);
	LOG(loglevel, logline, logsourcefile, "ctcpp_sendpack() SEND sockfd=%d, msg='%s%s'\n", sock, s_head, ((i_sndlen > 0) ? s_sndpack : ""));
	return 0;
}

// * ================================================================================ *
// @ ctcpp_sendsubmitmsg
// * ================================================================================ *
const CNString& ctcpp_create_submitmsg(CNString& s_submitmsg, t_pcstr s_terminal, t_pcstr s_content)
{
	static t_int i_shormsgid=1;
	s_submitmsg.Format("<body><to>%s</to><from>%s</from><needreport>0</needreport><msgfmt>15</msgfmt><msg>%s</msg><smsid>%d</smsid></body>",
		s_terminal, g_appserver.m_suser_shortmsg.c_str(), s_content, i_shormsgid);
	if(i_shormsgid++ > 2*1024*1024)	i_shormsgid = rand()%100;
	return s_submitmsg;
}


struct resend_shortmsg
{
	string s_bussopid;	//TANG 2010-4-12 15:56
	string s_terminalid;
	string s_content;
	t_int  i_seqid;
	string i_deltype;
	time_t i_tmlsend;
};

CNLink<resend_shortmsg> g_linkshortmsg_resend;

// * ================================================================================ *
// @ _shortmsg_send_prepare
// * ================================================================================ *
t_int _shortmsg_send_prepare(string& s_terminal, string& s_content, string& s_bussopid)
{
	int len1 = g_linkshortmsg_resend.GetLength();
	
	ConfigLine line1;
	line1.readline(s_content, "|");
	
	NPOSITION pos = NULL;
	
	if ( line1[0] == "22210" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "22211";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	else if ( line1[0] == "00110" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "00111";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	else if ( line1[0] == "77730" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "77731";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	else if ( line1[0] == "77740" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "77741";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	else if ( line1[0] == "00120" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "00121";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	else if ( line1[0] == "00112" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "00113";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	else if ( line1[0] == "77732" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "77733";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	else if ( line1[0] == "00200" )
	{
		pos = g_linkshortmsg_resend.AddQueue();
		g_linkshortmsg_resend[pos].i_deltype = "00201";
		g_linkshortmsg_resend[pos].i_seqid = 00;
		g_linkshortmsg_resend[pos].i_tmlsend = time(NULL);
		g_linkshortmsg_resend[pos].s_terminalid = s_terminal;
		g_linkshortmsg_resend[pos].s_content = s_content;
		g_linkshortmsg_resend[pos].s_bussopid = s_bussopid;
		s_content += "|00";
	}
	
	int len2 = g_linkshortmsg_resend.GetLength();
	
	if ( len1 != len2 )
	{
		int count=0;
		NPOSITION pos = g_linkshortmsg_resend.GetHeadPosition();
		while(pos) 
		{
			LOG(LOG6, "g_linkshortmsg_resend[%d] (s_terminalid='%s', s_content='%s', i_seqid=%d, i_deltype=%s)\n", 
				count++, 
				g_linkshortmsg_resend[pos].s_terminalid.c_str(), 
				g_linkshortmsg_resend[pos].s_content.c_str(), 
				g_linkshortmsg_resend[pos].i_seqid, 
				g_linkshortmsg_resend[pos].i_deltype.c_str()
				);
			pos = g_linkshortmsg_resend.GetNextPos(pos);
		}
	}
	
	return 0;
}

// * ================================================================================ *
// @ _shortmsg_resend
// * ================================================================================ *
t_int _shortmsg_resend(t_socket sock)
{
	NPOSITION pos = g_linkshortmsg_resend.GetHeadPosition();
	time_t curtm = time(NULL);
	
	while(pos) 
	{
		if((curtm - g_linkshortmsg_resend[pos].i_tmlsend) >= g_appserver.m_iresendtmout_shortmsg)
		{
			g_linkshortmsg_resend[pos].i_seqid++;
			if(g_linkshortmsg_resend[pos].i_seqid > g_appserver.m_iresendtimes_shortmsg) 
			{
				LOG(LOG6, "g_linkshortmsg_resend.size=%d, DELETE(s_terminalid='%s', s_content='%s', i_seqid=%d, i_deltype=%s) for resend timeout\n", 
					g_linkshortmsg_resend.GetLength()-1, 
					g_linkshortmsg_resend[pos].s_terminalid.c_str(), 
					g_linkshortmsg_resend[pos].s_content.c_str(), 
					g_linkshortmsg_resend[pos].i_seqid, 
					g_linkshortmsg_resend[pos].i_deltype.c_str()
					);
				
				NPOSITION pos1 = g_linkshortmsg_resend.GetNextPos(pos);
				g_linkshortmsg_resend.MovePosition(pos);
				pos = pos1;
				continue;
			}
			
			static CNString s_content, s_shortmsg;
			s_content.Format("%s|%02d", g_linkshortmsg_resend[pos].s_content.c_str(), g_linkshortmsg_resend[pos].i_seqid);
			
			// 创建ctcpp消息
			ctcpp_create_submitmsg(s_shortmsg, g_linkshortmsg_resend[pos].s_terminalid.c_str(), s_content.c_str());
			
			// 发送ctcpp消息
			ctcpp_sendpack(sock, s_shortmsg.c_str(), CTCPP_SUBMITSMS_REQ, LOG6);
			insert_db_bussop_sms(g_linkshortmsg_resend[pos].s_bussopid.c_str(), g_linkshortmsg_resend[pos].s_terminalid.c_str(), s_content.c_str(), 'S');
			
			g_linkshortmsg_resend[pos].i_tmlsend = curtm;
		}
		
		pos = g_linkshortmsg_resend.GetNextPos(pos);
	}
	
	return 0;
}
// * ================================================================================ *
// @ _shortmsg_recv_prepare
// * ================================================================================ *
t_int _shortmsg_recv_prepare(string& s_terminal, string& s_content)
{
	//去掉最前面的YR1|
	//s_content.erase(0, s_content.find("|")+1);
	
	ConfigLine line1;
	line1.readline(s_content, "|");
	
	NPOSITION pos = g_linkshortmsg_resend.GetHeadPosition();
	while(pos) 
	{
		if ( ( s_terminal == g_linkshortmsg_resend[pos].s_terminalid ) 
			&& ( line1[0] == g_linkshortmsg_resend[pos].i_deltype ) 
			)
		{
			LOG(LOG6, "g_linkshortmsg_resend.size=%d, DELETE(s_terminalid='%s', s_content='%s', i_seqid=%d, i_deltype=%s) for recv response sms\n", 
				g_linkshortmsg_resend.GetLength()-1, 
				g_linkshortmsg_resend[pos].s_terminalid.c_str(), 
				g_linkshortmsg_resend[pos].s_content.c_str(), 
				g_linkshortmsg_resend[pos].i_seqid, 
				g_linkshortmsg_resend[pos].i_deltype.c_str()
				);
			
			NPOSITION pos1 = g_linkshortmsg_resend.GetNextPos(pos);
			g_linkshortmsg_resend.MovePosition(pos);
			pos = pos1;
			continue;
		}
		
		pos = g_linkshortmsg_resend.GetNextPos(pos);
	}
	
	return 0;
}

// * ================================================================================ *
// @ thread_procdata_sendsmsg
// * ================================================================================ *
t_pvoid thread_procdata_sendsmsg(t_pvoid p_param)
{
	t_socket sock = INVALID_SOCKET;
	t_char s_rpack[4096];
	t_int i_plen, i_login=0;
	time_t tm_lrcvdata; t_int i_hbtimes = 0;
	
	time_t t_check_list = time(NULL);
	
	while(g_appserver.m_runing) 
	{
		if(INVALID_SOCKET == sock) 
		{
			i_login = 0;
			
			LOG(LOG6, "sms try to connect '%s:%d'\n", g_appserver.m_sip_shortmsg.c_str(), g_appserver.m_iport_shortmsg);
			sock = nsocket_open_connect(g_appserver.m_iport_shortmsg, g_appserver.m_sip_shortmsg.c_str());
			
			tm_lrcvdata = time(NULL);
			i_hbtimes = 0;
			
			if(INVALID_SOCKET != sock) 
			{
				LOG(LOG6, "sms connect to '%s:%d' ok\n", g_appserver.m_sip_shortmsg.c_str(), g_appserver.m_iport_shortmsg);
				
				sprintf(s_rpack, "<body><user>%s</user><pass>%s</pass><type>2</type></body>", g_appserver.m_suser_shortmsg.c_str(), g_appserver.m_spwd_shortmsg.c_str());
				ctcpp_sendpack(sock, s_rpack, CTCPP_CONNECT_REQ, LOG6);
			}
			else
			{
				LOG(LOG6, "sms connect to '%s:%d' fail\n", g_appserver.m_sip_shortmsg.c_str(), g_appserver.m_iport_shortmsg);
				sleep(10);
			}
		}
		
		if(INVALID_SOCKET != sock) 
		{
			if(time(NULL)-tm_lrcvdata >= CTCPP_ACTIVETEST_TIMEOUT) 
			{
				i_hbtimes++;
				if(i_hbtimes >= CTCPP_ACTIVETEST_TIMES)	{ nsocket_close(sock);	sock = INVALID_SOCKET; continue;}
				ctcpp_sendpack(sock, "", CTCPP_ACTIVETEST_REQ, LOG7);
				tm_lrcvdata = time(NULL);
			}
			
			if(1 == i_login) 
			{
				// 重发消息
				_shortmsg_resend(sock);

				if(0 == g_appserver.p_buf_smsg_snd->pack_pop(s_rpack, i_plen)) 
				{
					static CNString s_shortmsg;
					static string s_terminal, s_content, s_bussopid;

					s_rpack[i_plen] = 0;
					CClientMsg::ansy_getitem(s_rpack, "num", s_terminal);
					CClientMsg::ansy_getitem(s_rpack, "msg", s_content);
					CClientMsg::ansy_getitem(s_rpack, "bussopid", s_bussopid);

					LOG(LOG5, "send Prepare SMS [%s] [%s]\n", s_terminal.c_str(), s_content.c_str());

					// 消息发送前准备--重发控制
					_shortmsg_send_prepare(s_terminal, s_content, s_bussopid);

					// 创建ctcpp消息
					ctcpp_create_submitmsg(s_shortmsg, s_terminal.c_str(), s_content.c_str());
					
					// 发送ctcpp消息
					ctcpp_sendpack(sock, s_shortmsg.c_str(), CTCPP_SUBMITSMS_REQ, LOG6);
					insert_db_bussop_sms(s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'S');

					//LOG(LOG5, "send SHORT MESSAGE is %s\n", s_shortmsg.c_str());
				}
			}
			
			if(0 == ctcpp_recvpack(sock, s_rpack, i_plen)) 
			{
				tm_lrcvdata = time(NULL);
				i_hbtimes = 0;
				
				switch(ctcpp_packtype(s_rpack)) 
				{
					case CTCPP_CONNECT_RSP:
						{
							LOG(LOG6, "CTCPP_CONNECT_RSP %s\n", s_rpack);
							
							string s_result;
							CClientMsg::ansy_getitem(s_rpack, "result", s_result);
							if(s_result.c_str()[0] != '0') 
							{
								LOG(LOG2, "登录短信服务器失败\n");
								nsocket_close(sock);
								sock = INVALID_SOCKET;
								sleep(3);
							}
							else 
							{
								LOG(LOG2, "登录短信服务器成功\n");
								i_login = 1;
							}
						}
						break;
					case CTCPP_DELIVERSMS_REQ:
						{
							LOG(LOG6, "CTCPP_DELIVERSMS_REQ %s\n", s_rpack);
							
							static string s_terminal, s_content;
							ctcpp_sendpack(sock, s_ctcpprsp_succ, CTCPP_DELIVERSMS_RSP, LOG6);
							
							CClientMsg::ansy_getitem(s_rpack, "from", s_terminal);
							CClientMsg::ansy_getitem(s_rpack, "msg", s_content);
							
							LOG(LOG4, "处理短信 [%s] [%s]\n", s_terminal.c_str(), s_content.c_str());
							
							// 消息接收后处理的准备 -- 重发控制处理及处理消息头部
							_shortmsg_recv_prepare(s_terminal, s_content);
							
							appserver_proc_rcvshortmsg(s_terminal, s_content);
						}
						break;
					case CTCPP_DISCONNECT_RSP:
					case CTCPP_ACTIVETEST_RSP:
					case CTCPP_SUBMITSMS_RSP:
						break;
					default:
						LOG(LOG1, "unknown msg %s\n", s_rpack);
						break;
				}
			}
		}
		
		if ( time(NULL) >= t_check_list + 60 )
		{
			//检查66610超时
			if ( sms66610.size() > 0 )
			{
				if ( time(NULL) >= sms66610.begin()->tinsert + 30*60 )
				{
					//第一条短信收到后30分钟，还没有收完全部短信，不等了
					LOG(LOG1, "66610 timeout\n");
					deal_sms_66610(sms66610.begin()->phone);
				}
			}
			
			t_check_list = time(NULL);
		}
	}
	
	return NULL;
}

