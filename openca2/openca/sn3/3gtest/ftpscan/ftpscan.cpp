
#include "StdAfx.h"
#include "ftpscan.h"

#include "logfile.h"
#include "childprocess.h"
#include "config_file_t.h"

#include "tcp_server.h"

#define LOG CCLogFile.logwrite

#define CONFIG_FILE_PATH	"/home/spider/config/service17.ini"

LogFile CCLogFile("/home/spider/log/sn3", "service17.log");
LogFile * pLogFile = NULL;

#define SN3_SERVICE_NUMBER 17

rundata_ftpscan g_ftpscan;

ConfigFile myconfig;

TCP_SERVER<msg_format_xml> tcp_server1;

void deal_client_msg(int state, int sockfd, const string & sockname, const string & ip, int port, void * msg, int msglen);

// * ================================================================================ *
// @ _ftpscan_initial
// * ================================================================================ *
int _ftpscan_initial()
{
	nsocket_env_init();
	
	g_ftpscan.m_asip = myconfig["APPSERVER_IP"][1];
	g_ftpscan.m_asport = ToInt(myconfig["APPSERVER_PORT"][1]);

	g_ftpscan.m_sdb_ip = myconfig["DATABASE_IP"][1];
	g_ftpscan.m_sdb_name = myconfig["DATABASE_NAME"][1];
	g_ftpscan.m_sdb_uname = myconfig["DATABASE_USER_NAME"][1];
	g_ftpscan.m_sdb_upwd = myconfig["DATABASE_PASSWD"][1];
	
	g_ftpscan.m_spath = myconfig["FTP_SCAN_PATH"][1] + "/";
	g_ftpscan.m_itmdelay_scan = ToInt(myconfig["FTP_SCAN_INTERVAL"][1]);
	
	g_ftpscan.p_buf_command = CBasicBuffer::CreateInstance(64*1024);
	g_ftpscan.p_buf_sql = CBasicBuffer::CreateInstance(1024*1024);
		
	g_ftpscan.m_bruning = 1;
	
	LOG(LOG6, "ftpscan: scan path '%s'\n", g_ftpscan.m_spath.c_str());
	
	return 0;
}

// * ================================================================================ *
// @ _ftpscan_destroy
// * ================================================================================ *
void _ftpscan_destroy()
{
	g_ftpscan.m_bruning = 0;
	g_ftpscan.p_buf_sql->Destroy();
	g_ftpscan.p_buf_command->Destroy();
	n_sleep(3000);
}


int main()
{
	//open the log file
	pLogFile = &CCLogFile;
	CCLogFile.startlog(1);
	CCLogFile.SetLogSize(2*1000*1000);
	CCLogFile.SetLoglevel(1);
	CCLogFile.logopen();
	
	LOG(LOG1, "ftpscan: start\n");
	
	if ( myconfig.readfile(CONFIG_FILE_PATH) == 0 )
	{
		printf("ftpscan: read config file '%s' error\n", CONFIG_FILE_PATH);
		LOG(LOG1, "ftpscan: read config file '%s' error\n", CONFIG_FILE_PATH);
		sleep(2);
		exit(0);
	}
	
	CCLogFile.SetLoglevel(ToInt(myconfig["LogLevel"][1]));
	
	child_process_init(SN3_SERVICE_NUMBER);
	
	//application_envinit();

	_ftpscan_initial();
	
	nthread_create(thread_ftpsock, NULL, 64*1024);
	nthread_create(thread_ftp_procfile, NULL, 64*1024);
	
	tcp_server1.SetpLog(&CCLogFile);
	tcp_server1.SetCallBackFunc(deal_client_msg);
	if ( ToInt(myconfig["CalculateUnitListenPort"][1]) > 0 )
	{
		tcp_server1.Listen(ToInt(myconfig["CalculateUnitListenPort"][1]), 180);
	}
	else
	{
		tcp_server1.Listen(4017, 180);
	}
	
	while(1)
	{
		child_process_hb(SN3_SERVICE_NUMBER);
		
		n_sleep(1000);
	}
	
	_ftpscan_destroy();
	
	tcp_server1.Close();
	
	LOG(LOG1, "ftpscan: exit\n");
	return 0;
}

