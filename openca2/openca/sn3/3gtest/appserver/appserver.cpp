
#include "StdAfx.h"
#include "appserver.h"

#include "logfile.h"
#include "childprocess.h"

#include "tcp_server.h"

#define LOG CCLogFile.logwrite

#define CONFIG_FILE_PATH	"/home/spider/config/service16.ini"

LogFile CCLogFile("/home/spider/log/sn3", "service16.log");
LogFile * pLogFile = NULL;

#define SN3_SERVICE_NUMBER 16

rundata_appserver g_appserver;

ConfigFile myconfig;

TCP_SERVER<msg_format_xml> tcp_server1;

extern t_pvoid thread_sockserver(t_pvoid p_param);
extern t_pvoid thread_procdata_sendsmsg(t_pvoid p_param);

extern MYSQLDB DB1;


int get_test_timeout(const char * bussop_type)
{
	int timeout = 0;
	
	//版本查询
	if ( strcmp(bussop_type, "AA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_QUERY_VERSION"][1]);
	
	//版本升级
	if ( strcmp(bussop_type, "AB") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_UPDATE_VERSION"][1]);
	
	//版本回滚
	if ( strcmp(bussop_type, "AC") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_ROLLBACK_VERSION"][1]);
	
	//CQT终端呼叫核心网
	if ( strcmp(bussop_type, "BA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_CQT_T_S"][1]);
	
	//CQT终端呼叫终端
	if ( strcmp(bussop_type, "BB") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_CQT_T_T"][1]);
	
	//PESQ排队机呼叫终端
	if ( strcmp(bussop_type, "CA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_PESQ_S_T"][1]);
	
	//PING测试
	if ( strcmp(bussop_type, "DA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_PING"][1]);
	
	//FTP测试
	if ( strcmp(bussop_type, "EA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_FTP"][1]);
	
	//HTTP测试
	if ( strcmp(bussop_type, "FA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_HTTP"][1]);
	
	//CDMA参数采集
	if ( strcmp(bussop_type, "GA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_CDMA"][1]);
	
	//终端密码修改
	if ( strcmp(bussop_type, "HA") == 0 ) timeout = ToInt(myconfig["REAL_TIME_TEST_TIMEOUT_MODIFY_PASS"][1]);
	
	//终端重启
	if ( strcmp(bussop_type, "LA") == 0 ) timeout = 2*60;
	
	if ( timeout < 60 ) timeout = 3*60;
	
	return timeout;
}


// * ================================================================================ *
// @ _appserver_initial
// * ================================================================================ *
t_int _appserver_initial()
{
	nsocket_env_init();

	g_appserver.m_iport_shortmsg = ToInt(myconfig["SMSSERVER_PORT"][1]);
	g_appserver.m_sip_shortmsg = myconfig["SMSSERVER_IP"][1];
	g_appserver.m_iresendtimes_shortmsg = ToInt(myconfig["SMSSERVER_RESEND_TIMES"][1]);
	g_appserver.m_iresendtmout_shortmsg = ToInt(myconfig["SMSSERVER_RESEND_INTERVAL"][1]);
	g_appserver.m_imaxsendpersec_shortmsg = ToInt(myconfig["SMSSERVER_MAX_SEND_PER_SECOND"][1]);

	g_appserver.m_sdb_ip = myconfig["DATABASE_IP"][1];
	g_appserver.m_sdb_name = myconfig["DATABASE_NAME"][1];
	g_appserver.m_sdb_uname = myconfig["DATABASE_USER_NAME"][1];
	g_appserver.m_sdb_upwd = myconfig["DATABASE_PASSWD"][1];

	g_appserver.m_suser_shortmsg = myconfig["SMSSERVER_USER"][1];
	g_appserver.m_spwd_shortmsg = myconfig["SMSSERVER_PASS"][1];	//MD5
	
	g_appserver.p_index_terminal.Initial(MAXNUM_REALTEST);
	
	g_appserver.p_buf_smsg_snd = CBasicBuffer::CreateInstance(64*1024);
	g_appserver.p_buf_smsg_rcv = CBasicBuffer::CreateInstance(64*1024);

	appserver_opendb(g_appserver.m_sdb_name.c_str(), g_appserver.m_sdb_ip.c_str(), g_appserver.m_sdb_uname.c_str(), g_appserver.m_sdb_upwd.c_str());
	
	create_db_tables();
	insert_db_default_data();
	
	//appserver_execute("UPDATE BUSSOP SET STATUS='Y', RES_CODE=111 WHERE STATUS='B';");
	//appserver_execute("UPDATE CONFIG_TERMINAL_ROUTINE_CONTENT_EXTRA_PESQ SET EXEC_DATE='19000101';");

	g_appserver.m_runing = 1;
	
	return 0;
}

// * ================================================================================ *
// @ _appserver_destroy
// * ================================================================================ *
t_void _appserver_destroy()
{
	appserver_closedb();
	g_appserver.m_runing = 0;
	g_appserver.p_buf_smsg_rcv->Destroy();
	g_appserver.p_buf_smsg_snd->Destroy();
	n_sleep(3000);
}

// * ================================================================================ *
// @ realtest_next
// * ================================================================================ *
rundata_realtest* realtest_next(NPOSITION& pos)
{
	return g_appserver.p_index_terminal.Find(pos);
}

// * ================================================================================ *
// @ realtest_find
// * ================================================================================ *
rundata_realtest* realtest_find(const char* s_terminalid, const char c_buss)
{
	string s_realtest = s_terminalid;
	s_realtest += c_buss;
	return g_appserver.p_index_terminal.Find(s_realtest.c_str(), s_realtest.length());
}

// * ================================================================================ *
// @ realtest_insert
// * ================================================================================ *
rundata_realtest* realtest_insert(const char* s_terminalid, const char c_buss, const char* s_bussoptype)
{
	rundata_realtest* p_runtest;
	string s_realtest = s_terminalid;
	s_realtest += c_buss;
	p_runtest = g_appserver.p_index_terminal.Insert(s_realtest.c_str(), s_realtest.length());
	
	if(p_runtest)
	{
		int timeout = get_test_timeout(s_bussoptype);
		
		p_runtest->u_buss = c_buss;
		p_runtest->tm_start = time(NULL);
		p_runtest->tm_end = time(NULL) + timeout;
		
		LOG(LOG6, "realtest_insert(): terminal='%s', bussop='%c', timeout=%d\n", s_terminalid, c_buss, timeout);
	}
	
	return p_runtest;
}

// * ================================================================================ *
// @ realtest_delete
// * ================================================================================ *
void realtest_delete(const char* s_terminalid, const char c_buss)
{
	LOG(LOG6, "realtest_delete(): terminal='%s', bussop='%c'\n", s_terminalid, c_buss);
	string s_realtest = s_terminalid;
	s_realtest += c_buss;
	g_appserver.p_index_terminal.Delete(s_realtest.c_str(), s_realtest.length());
}


void deal_client_msg(int state, int sockfd, const string & sockname, const string & ip, int port, void * msg, int msglen)
{
	char * buf = (char *) msg;
	int buflen = msglen;
	
	if ( state == TCP_CLOSED )
	{
		LOG(LOG6, "server: 对方关闭连接, sockfd=%d, sockname='%s', ip='%s', port=%d\n\n", sockfd, sockname.c_str(), ip.c_str(), port);
		return;
	}
	
	if ( state == TCP_MSG_TIMEOUT )
	{
		LOG(LOG6, "server: 接收消息超时, sockfd=%d, sockname='%s', ip='%s', port=%d\n\n", sockfd, sockname.c_str(), ip.c_str(), port);
		return;
	}
	
	if ( state == TCP_MSG_ERROR )
	{
		LOG(LOG6, "server: 接收消息格式错误, sockfd=%d, sockname='%s', ip='%s', port=%d\n\n", sockfd, sockname.c_str(), ip.c_str(), port);
		return;
	}
	
	if ( state == TCP_CONNECTED )
	{
		LOG(LOG6, "server: 建立连接, sockfd=%d, sockname='%s', ip='%s', port=%d\n\n", sockfd, sockname.c_str(), ip.c_str(), port);
		return;
	}
	
	if ( state == TCP_COMPLETE )
	{
		LOG(LOG6, "server: 自己主动关闭连接, sockfd=%d, sockname='%s', ip='%s', port=%d\n\n", sockfd, sockname.c_str(), ip.c_str(), port);
		return;
	}
	
	if ( state == TCP_SERVER_CLOSED )
	{
		LOG(LOG6, "server: 服务器停止运行, sockfd=%d, sockname='%s', ip='%s', port=%d\n\n", sockfd, sockname.c_str(), ip.c_str(), port);
		return;
	}
	
	if ( state != TCP_RECV_MSG )
	{
		LOG(LOG1, "server: unknown call back\n\n");
		return;
	}
	
	if ( buflen <= 0 )
	{
		LOG(LOG1, "server: msglen=%d, error\n", buflen);
		return;
	}
	
	if ( strncmp(buf, "<policy-file-request/>", strlen("<policy-file-request/>")) == 0 )
	{
		LOG(LOG6, "service:send msg=[<policy-file-request/>] response\n");
		string policy = "<?xml version=\"1.0\" encoding=\"utf-8\" ?><cross-domain-policy><allow-access-from domain=\"*\" to-ports=\"*\" /></cross-domain-policy>";
		tcp_server1.Send(sockfd, policy.c_str(), policy.length()+1, 6);
		return;
	}
	
	ConfigLine xml;
	xml.read_xml_msg(buf);	//解析消息
	
	if ( xml["msg"] == "hb" )
	{
		tcp_server1.Send(sockfd, buf, strlen(buf), 100);
		return;
	}
	
	//<head>00059</head><msg>terminal_wifi_hb</msg><terminal>13701916487</terminal>
	if ( xml["msg"] == "terminal_wifi_hb" )
	{
		tcp_server1.Send(sockfd, buf, strlen(buf), 100);
		return;
	}
	
	LOG(LOG6, "server: recv msg, sockfd=%d, sockname='%s', ip='%s', port=%d, msg='%s'\n", sockfd, sockname.c_str(), ip.c_str(), port, buf);
	
	if ( xml["msg"] == "login" )
	{
		//答复
		string outmsg;
		xml_put_para(outmsg, "head", "00000");
		xml_put_para(outmsg, "msg",  xml["msg"]);
		xml_put_para(outmsg, "result", 0);
		tcp_server1.Send(sockfd, outmsg.c_str(), outmsg.size(), 6);
	}
	
	else
	{
		LOG(LOG1, "server: unknown msg = '%s', ignore\n", xml["msg"].c_str());
		return;
	}
	
	LOG(LOG6, "\n");
	return;
}


// * ================================================================================ *
// @ main
// * ================================================================================ *
int main()
{
	//open the log file
	pLogFile = &CCLogFile;
	CCLogFile.startlog(1);
	CCLogFile.SetLogSize(2*1000*1000);
	CCLogFile.SetLoglevel(1);
	CCLogFile.logopen();
	
	LOG(LOG1, "appserver: start\n");
	
	if ( myconfig.readfile(CONFIG_FILE_PATH) == 0 )
	{
		printf("appserver: read config file '%s' error\n", CONFIG_FILE_PATH);
		LOG(LOG1, "appserver: read config file '%s' error\n", CONFIG_FILE_PATH);
		sleep(2);
		exit(0);
	}
	
	CCLogFile.SetLoglevel(ToInt(myconfig["LogLevel"][1]));
	
	child_process_init(SN3_SERVICE_NUMBER);
	
	//application_envinit();
	//LOG(LOG1, "appserver: application_envinit() done\n");

	_appserver_initial();
	LOG(LOG1, "appserver: _appserver_initial() done\n");

	nthread_create(thread_sockserver, NULL, 64*1024);
	nthread_create(thread_procdata_sendsmsg, NULL, 64*1024);
	
	tcp_server1.SetpLog(&CCLogFile);
	tcp_server1.SetCallBackFunc(deal_client_msg);
	if ( ToInt(myconfig["AppServerListenPort"][1]) > 0 )
	{
		tcp_server1.Listen(ToInt(myconfig["AppServerListenPort"][1]), 180);
	}
	else
	{
		tcp_server1.Listen(4016, 180);
	}

	NPOSITION pos;
	rundata_realtest* p_runtest;
	
	string s_terminal, s_msg_type;
	
	time_t t_check_hb_timeout = time(NULL);
	time_t t_check_test_timeout = time(NULL);
	
	while(g_appserver.m_runing)
	{
		child_process_hb(SN3_SERVICE_NUMBER);
		
		pos = NULL;
		while(NULL != (p_runtest = realtest_next(pos))) 
		{
			if ( time(NULL) >= p_runtest->tm_end )
			{
				CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "terminal", s_terminal);
				CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "msg", s_msg_type);
				
				//PESQ的实时测试，ftp扫描单元可能直接修改数据库的测试结果为X
				CNString sql;
				sql.Format("select * from BUSSOP where BUSSOP_ID = '%s' and (STATUS != 'X' and STATUS != 'Y');", p_runtest->s_bussopid.c_str());
				DATASET dbresult;
				if ( DB1.db_select(sql.c_str(), dbresult, LOG6) > 0 )
				{
					LOG(LOG1, "测试超时 [%s] '%c', s_bussopid='%s', timeout=%ds\n\n", 
						s_terminal.c_str(), 
						p_runtest->u_buss, 
						p_runtest->s_bussopid.c_str(), 
						p_runtest->tm_end - p_runtest->tm_start
						);
					
					int i_rescode = 1;
					if ( dbresult[0]["STATUS"] == "W" ) i_rescode = 5001;
					
					appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), i_rescode);
				}
				
				t_uchar u_buss = p_runtest->u_buss;
				
				p_runtest = realtest_next(pos);
				
				realtest_delete(s_terminal.c_str(), u_buss);
			}
		}
		
		n_sleep(1000);
		
		if ( time(NULL) >= t_check_hb_timeout + 10*60 )
		{
			CNString sql;
			sql.Format("select * from CONFIG_TERMINAL where TERMINAL_STATE = 0 and HEARTBEAT_STATE = 0 and %u >= T_RECV_HEARTBEAT + %d;", 
					time(NULL), ToInt(myconfig["SMS_HEARTBEAT_TIMEOUT"][1]));
			
			DATASET dbresult;
			if ( DB1.db_select(sql.c_str(), dbresult, LOG6) > 0 )
			{
				//某终端心跳短信超时
				for ( int i=0; i<dbresult.size(); i++ )
				{
					string s_terminal = dbresult[i]["TERMINAL_ID"];
					string s_bussopid;
					string s_starttime = time_format_str(time(NULL), 1);
					_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "IA", s_starttime.c_str());
					appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "IA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
					insert_bussop_exception_result(s_bussopid.c_str(), "HEARTBEAT_TMOUT");
					update_terminal_heartbeat_timeout(s_terminal.c_str());
				}
			}
			
			t_check_hb_timeout = time(NULL);
		}
		
		if ( time(NULL) >= t_check_test_timeout + 10*60 )
		{
			CNString sql;
			sql.Format("select * from BUSSOP where (STATUS != 'X' and STATUS != 'Y') order by STARTTIME;");
			
			DATASET dbresult;
			if ( DB1.db_select(sql.c_str(), dbresult, LOG6) > 0 )
			{
				//测试超时
				for ( int i=0; i<dbresult.size(); i++ )
				{
					time_t t_test_start_time = time_string_to_time_t(dbresult[i]["STARTTIME"]);
					int timeout = get_test_timeout(dbresult[i]["BUSSOP_TYPE"].c_str());
					
					if ( time(NULL) >= t_test_start_time + timeout + 3*60 )
					{
						LOG(LOG1, "DB 测试超时 [%s] '%s', s_bussopid='%s', timeout=%ds\n\n", 
								dbresult[i]["TERMINAL_ID"].c_str(), 
								dbresult[i]["BUSSOP_TYPE"].c_str(), 
								dbresult[i]["BUSSOP_ID"].c_str(), 
								timeout
								);
						
						int i_rescode = 1;
						if ( dbresult[i]["STATUS"] == "W" ) i_rescode = 5001;
						
						appserver_dbupdate_bussop_error(dbresult[i]["BUSSOP_ID"].c_str(), i_rescode);
					}
				}
			}
			
			t_check_test_timeout = time(NULL);
		}
	}

	_appserver_destroy();
	
	tcp_server1.Close();
	
	LOG(LOG1, "appserver: exit\n");
	return 0;
}
