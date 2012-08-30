
#include "StdAfx.h"
#include "ftpscan.h"

#include "logfile.h"
#include "childprocess.h"
#include "config_file_t.h"

#include "sock.h"
#include "tcp_server.h"

#define LOG CCLogFile.logwrite

LogFile CCLogFile("/home/spider/log/sn3", "service17.log");
LogFile * pLogFile = NULL;

#define SN3_SERVICE_NUMBER 17

rundata_ftpscan g_ftpscan;

ConfigFile myconfig;

int ftpscan_start_seconds = 0;
int ftpscan_stop_seconds = 0;

TCP_SERVER<msg_format_xml> tcp_server1;

void deal_client_msg(int state, int sockfd, const string & sockname, const string & ip, int port, void * msg, int msglen);


int set_ftpscan_time(const string & time1, const string & time2)
{
	ConfigLine line1;
	line1.readline(time1, ":");
	
	ConfigLine line2;
	line2.readline(time2, ":");
	
	ftpscan_start_seconds = ToInt(line1[0]) * 60 * 60 + ToInt(line1[1]) * 60;
	ftpscan_stop_seconds  = ToInt(line2[0]) * 60 * 60 + ToInt(line2[1]) * 60;
	
	return 1;
}


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


int write2socket(int * sockfd, char * msgbuf, int len, int loglevel = 6)
{
	if( sockfd == NULL ) return 0;
	if ( *sockfd <= 0 ) return 0;
	if ( len <= 0 ) return 0;
	
	if ( writen(*sockfd, msgbuf, len) )
	{
		//LOG(loglevel, __LINE__, __FILE__, "alarm: write msg %d bytes ok\n", len);
		//LOG(loglevel, len, msgbuf);
		return 1;
	}
	else
	{
		//LOG(LOG4, "alarm: writen() sockfd=%d, error=%d\n", *sockfd, errno);
		//LOG(1, len, msgbuf);
		shutdown(*sockfd, SHUT_WR);
		close(*sockfd);
		*sockfd = 0;
		return 0;
	}
}


void send_msg_reload_config(char * ip, int port)
{
	char ach[256] = "<head>00024</head><msg>reload_config</msg>";
	
	int smssock=0;
	int ret=0;
	if ( (ret=sock_connect(port, inet_addr(ip), &smssock)) <= 0 )
	{		
		printf("connect to server failed, error\n");
		return;
	}
	
	if ( write2socket(&smssock, ach, strlen(ach)) == 0 )
	{
		printf("write to server failed, error\n");
		shutdown(smssock, SHUT_WR);
		close(smssock);
		return;
	}
	
	//如果服务器不给答复，就会阻塞在下面的read函数上
	if ( sock_ready_read(smssock, 5000) )	//wait 5 seconds
	{
		memset(ach,0,sizeof(ach));
		ret = read(smssock,&ach[0],sizeof(ach));
		printf("cmd reload config done\n\n");
	}
	
	shutdown(smssock, SHUT_WR);
	close(smssock);
	smssock = 0;
	return ;
}


int main(int argc, char ** argv)
{
	//open the log file
	pLogFile = &CCLogFile;
	CCLogFile.startlog(1);
	CCLogFile.SetLogSize(2*1000*1000);
	CCLogFile.SetLoglevel(1);
	CCLogFile.logopen();
	
	if ( myconfig.readfile(CONFIG_FILE_PATH) == 0 )
	{
		printf("ftpscan: read config file '%s' error\n", CONFIG_FILE_PATH);
		LOG(LOG1, "ftpscan: read config file '%s' error\n", CONFIG_FILE_PATH);
		sleep(2);
		exit(0);
	}
	
	//读配置文件后要根据配置文件内容做处理
	set_ftpscan_time(myconfig["FTP_SCAN_TIME"][2], myconfig["FTP_SCAN_TIME"][3]);
	
	CCLogFile.SetLoglevel(ToInt(myconfig["LogLevel"][1]));
	
	
	//TANG 2010-9-4 11:02
	//处理命令行参数
	if ( argc == 3 && strcmp(argv[1], "reload") == 0 && strcmp(argv[2], "config") == 0 )
	{
		send_msg_reload_config("127.0.0.1", (( ToInt(myconfig["CalculateUnitListenPort"][1]) > 0 ) ? ToInt(myconfig["CalculateUnitListenPort"][1]) : 4017));
		exit(0);
	}
	
	
	LOG(LOG1, "ftpscan: start\n");
	
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

