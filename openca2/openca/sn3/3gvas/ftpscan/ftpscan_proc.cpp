#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "StdAfx.h"
#include "ftpscan.h"
#include "../appserver/appserver_db.h"

#include "sock.h"
#include "tcp_server.h"

#define LOG CCLogFile.logwrite
extern LogFile CCLogFile;

extern ConfigFile myconfig;

extern TCP_SERVER<msg_format_xml> tcp_server1;

extern MYSQLDB DB1;

extern int ftpscan_start_seconds;
extern int ftpscan_stop_seconds;

extern int set_ftpscan_time(const string & time1, const string & time2);

t_int _process_routine_vas(t_pcstr s_directory, t_pcstr s_fname, t_char r_or_t = 'R', t_pcstr bussopid = "");

static t_pcstr g_smsg_login = "<head>00020</head><msg>login_ftp</msg>";
static t_pcstr g_smsg_hb = "<head>00013</head><msg>hb</msg>";

t_socket g_sock;

int g_zip_name_value = 0;

long filelen(const char * file1)
{
	struct stat filestat;
	memset(&filestat, 0, sizeof(filestat));
	if ( stat(file1, &filestat) < 0 ) return 0;
	return filestat.st_size;
}

// * ================================================================================ *
// @ _climsg_recvpacket
// * ================================================================================ *
t_int _climsg_recvpacket(t_socket sock, t_pchar s_packet, t_int& i_packlen)
{
	if(18 != nsocket_recvlen(sock, s_packet, 18, 100)) {
		return -1;
	}
	s_packet[18] = 0;
	i_packlen = atoi(s_packet+strlen("<head>"));
	if(i_packlen != nsocket_recvlen(sock, s_packet, i_packlen, 100)) {
		return -2;
	}
	s_packet[i_packlen] = 0;

	return 0;
}

// * ================================================================================ *
// @ clisock_sendpack
// * ================================================================================ *
t_int clisock_sendpack(t_pcstr s_packet, t_int i_packlen)
{
	if(INVALID_SOCKET != g_sock) 
	{
		int ret = nsocket_sendlen(g_sock, s_packet, i_packlen, 1000);
		LOG(LOG6, "clisock_sendpack() send msg '%s'\n", s_packet);
		if(i_packlen == ret ) return 0;
	}
	return -1;
}

// * ================================================================================ *
// @ thread_ftpsock
// * ================================================================================ *
t_pvoid thread_ftpsock(t_pvoid p_param)
{
	time_t tm_curtm, tm_lstrcv, tm_hb;

	t_char s_packet[4096];
	t_int i_packlen;

	string s_msg;

	g_sock = INVALID_SOCKET;

	while(g_ftpscan.m_bruning) 
	{
		tm_curtm = time(NULL);
		
		if(INVALID_SOCKET != g_sock) 
		{
			if(0 == nsocket_is_datarecv(g_sock, 1)) 
			{
				if(0 != _climsg_recvpacket(g_sock, s_packet, i_packlen)) 
				{ 
					LOG(LOG1, "thread_ftpsock() close sockfd for read error\n\n");
					nsocket_close(g_sock); 
					g_sock = INVALID_SOCKET;
					continue; 
				}
				
				tm_hb = tm_lstrcv = tm_curtm;
				
				s_msg = "";
				CClientMsg::ansy_getitem(s_packet, "msg", s_msg);
				
				if ( ("HB" != s_msg) && ("hb" != s_msg) && ("login_ftp" != s_msg)) 
				{
					LOG(LOG6, "thread_ftpsock() recv msg '%s'\n", s_packet);
					g_ftpscan.p_buf_command->pack_push(s_packet, i_packlen);
				}
			}
			
			if( (tm_curtm-tm_hb) >= 60 ) 
			{ 
				tm_hb = tm_curtm; nsocket_sendlen(g_sock, g_smsg_hb, strlen(g_smsg_hb), 100); 
			}
			
			if( (tm_curtm-tm_lstrcv) > 120)	
			{ 
				LOG(LOG1, "thread_ftpsock() close sockfd for timeout\n\n");
				nsocket_close(g_sock); 
				g_sock = INVALID_SOCKET; 
				continue; 
			}
		}
		else 
		{
			if( INVALID_SOCKET != (g_sock = nsocket_open_connect(g_ftpscan.m_asport, g_ftpscan.m_asip.c_str()))) 
			{
				LOG(LOG1, "ftpscan: connect to '%s:%d' ok\n", g_ftpscan.m_asip.c_str(), g_ftpscan.m_asport);
				
				tm_hb = tm_lstrcv = time(NULL);
				nsocket_sendlen(g_sock, g_smsg_login, strlen(g_smsg_login), 1000);
				LOG(LOG6, "ftpscan: send login msg '%s'\n", g_smsg_login);
			}
			else
			{
				LOG(LOG6, "ftpscan: connect to '%s:%d' fail\n", g_ftpscan.m_asip.c_str(), g_ftpscan.m_asport);
				sleep(10);
			}
		}
	}
	return NULL;
}


// * ================================================================================ *
// @ 初始化目录
// * ================================================================================ *
t_int _ftpprocfile_initialdir()
{
	switch(g_ftpscan.m_cbuss)
	{
	case BUSS_CQT:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"cqt/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_cqt/";
		break;
	case BUSS_FTP:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"ftp/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_ftp/";
		break;
	case BUSS_HTTP:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"http/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_http/";
		break;
	case BUSS_PING:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"ping/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_ping/";
		break;
	case BUSS_CDMA:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"cdma/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_cdma/";
		break;
	case BUSS_PESQ:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"pesq/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_pesq/";
		break;
	case BUSS_FETION:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"fetion/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_fetion/";
		break;
	case BUSS_VAS:
		g_ftpscan.m_sfullpath = g_ftpscan.m_spath+"vas/";
		g_ftpscan.m_smovepath = g_ftpscan.m_spath+"history_vas/";
		break;
	default:
		return 1;
	}
	return 0;
}
// * ================================================================================ *
// @ 移动文件
// * ================================================================================ *
t_int _ftpprocfile_movefile(t_pcstr s_fname)
{
	string s_destfname, s_srcfname;

	s_srcfname = g_ftpscan.m_sfullpath + s_fname + ".txt";
	s_destfname = g_ftpscan.m_smovepath + s_fname + ".txt";
	n_file_move(s_srcfname.c_str(), s_destfname.c_str());
	LOG(LOG6, "n_file_move() '%s' -> '%s'\n", s_srcfname.c_str(), s_destfname.c_str());
	
	s_srcfname.erase(s_srcfname.length()-4, 4);
	s_srcfname += ".zip";
	s_destfname.erase(s_destfname.length()-4, 4);
	s_destfname += ".zip";
	n_file_move(s_srcfname.c_str(), s_destfname.c_str());
	LOG(LOG6, "n_file_move() '%s' -> '%s'\n", s_srcfname.c_str(), s_destfname.c_str());
	
	s_srcfname.erase(s_srcfname.length()-4, 4);
	s_srcfname += ".wav";
	s_destfname.erase(s_destfname.length()-4, 4);
	s_destfname += ".wav";
	n_file_move(s_srcfname.c_str(), s_destfname.c_str());
	LOG(LOG6, "n_file_move() '%s' -> '%s'\n", s_srcfname.c_str(), s_destfname.c_str());
	
	s_srcfname.erase(s_srcfname.length()-4, 4);
	s_srcfname += ".end";
	n_file_del(s_srcfname.c_str());
	LOG(LOG6, "n_file_del() '%s'\n", s_srcfname.c_str());
	
	s_srcfname.erase(s_srcfname.length()-4, 4);
	s_srcfname += ".start";
	n_file_del(s_srcfname.c_str());
	LOG(LOG6, "n_file_del() '%s'\n", s_srcfname.c_str());
	
	return 0;
}

// * ================================================================================ *
// @ _process_realtime_cqt
// * ================================================================================ *
t_int _process_realtime_cqt(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	LOG(LOG6, "_process_realtime_cqt() directory='%s', file='%s', s_bussopid='%s', filesize=%d\n", s_directory, s_fname, s_bussopid, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para;

	t_int i_times, i_losts, i_time_total;

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		if ( check_bussopid_complete(s_bussopid) > 0 ) return 0;
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());

	i_curpos_line = 0; i_line = 0;
	i_time_total = 0; i_losts = 0; i_times = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) ||
				(s_para.get_length() > 0) ) 
			{
					i_para++;
					if(5 == i_para) i_times = s_para.to_int32();
			}
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ';') < s_rline.get_length()) ||
				(s_para.get_length() > 0))
			{
					i_para++;
					if( (5 == i_para) && ('y' != s_para[0]))	i_losts++;
					else if(6 == i_para)				i_time_total -= s_para.to_int32();
					else if(7 == i_para)				i_time_total += s_para.to_int32();
			}
		}
	}

	t_int i_lostfre = (t_int)((double)i_losts/i_times)*100;
	t_int i_connfre = 100 - i_lostfre;
	t_int i_avgtm = i_time_total/i_times;

	appserver_dbinsert_bussop_result_cqt(s_bussopid, i_connfre, i_lostfre, i_avgtm);
	
	appserver_dbupdate_bussop_success(s_bussopid);
	
	return 0;
}


// * ================================================================================ *
// @ _process_realtime_ftp
// * ================================================================================ *
/*
第一行：本机号码|FTP-Realtime|测试次数|超时|URL 第一行用“|”间隔
如：
13764582377|20090927180724|2|10|ftp://symtree:jason010101@222.186.191.102/wwwroot/B/Basetech.lib
记录行格式 ： 下载开始时间，下载结束时间，成功标识（OK标识成功、FAIL标识失败）
3463.943000,3482.552000,OK
*/
t_int _process_realtime_ftp(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	LOG(LOG6, "_process_realtime_ftp() directory='%s', file='%s', s_bussopid='%s', filesize=%d\n", s_directory, s_fname, s_bussopid, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static CNString s_test_url, s_test_succflag, s_test_starttime, s_test_endtime;

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		if ( check_bussopid_complete(s_bussopid) > 0 ) return 0;
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'FTP_FAIL_RATE_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_FAIL_RATE;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE, LOG6);
	int total_count = 0;
	int fail_count = 0;
	
	sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'FTP_SPEED_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG6);
	int speed_count[16] = {0};
	
	int filesize = 3*1024*1024;

	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				i_para++;
				if(5 == i_para) s_test_url = s_para;
			}
			
			//根据url中的文件名获取文件大小 ftp://spider:spider@spider.8800.org/ftp/testup.txt
			if ( s_test_url.get_length() > 0 )
			{
				char * pp = strchr(s_test_url.c_str()+6, '/');
				if ( pp )
				{
					string filename = ToString("/home/spider") + pp;
					filesize = filelen(filename.c_str());
					LOG(LOG6, "ftp file '%s' -> '%s', size is %d\n", s_test_url.c_str(), filename.c_str(), filesize);
					if ( filesize == 0 ) filesize = 3*1024*1024;
				}
			}
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ',') < s_rline.get_length()) || (s_para.get_length() > 0))
			{
				i_para++;
				if(1 == i_para)					s_test_starttime = s_para;
				else if(2 == i_para)			s_test_endtime = s_para;
				else if(3 == i_para)			s_test_succflag = s_para;
			}
			
			if(3 == i_para)
			{
				appserver_dbinsert_bussop_result_ftp(s_bussopid, ++i_seq, s_test_url.c_str(), ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str()), filesize, (0 <= s_test_succflag.find(0, "OK")) ? 'S' : 'F');
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				total_count++;
				if ( CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size() > 0 )
				{
					if ( s_test_succflag != "OK" )
					{
						fail_count++;
					}
				}
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED.size() > 0 )
				{
					//ftp的速率计算
					int rate = (ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str()) <= 0.000001) ? 0 : (int)(filesize * 1.0 / (ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str())));
					LOG(LOG6, "ftp file size = %d, time = %.3f, speed = %d\n", filesize, ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str()), rate);
					
					for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
					{
						if ( s_test_succflag != "OK" )
						{
							
						}
						else
						{
							if (   rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MINVALUE"]) 
								&& rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MAXVALUE"]) 
								)
							{
								speed_count[jj]++;
							}
						}
					}
				}
			}
		}
	}
	
	appserver_dbupdate_bussop_success(s_bussopid);
	
	LOG(LOG6, "total_count=%d, fail_count=%d, speed_count[0]=%d, speed_count[1]=%d, speed_count[2]=%d\n", 
		total_count, fail_count, speed_count[0], speed_count[1], speed_count[2]);
	
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			//插入BUSSOP_EXCEPTION_RESULT表
			insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[0]["BUSSOPEXEC_ID"].c_str());
			break;
		}
		else
		{
			int rate = 100 * fail_count / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			break;
		}
		else
		{
			int rate = 100 * speed_count[jj] / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	
	return 0;
}

// * ================================================================================ *
// @ _process_realtime_ping
// * ================================================================================ *
/*
第一行：本机号码|PING-Realtime|测试次数|PING包大小（单位Bytes）|PING时间间隔（单位为秒）|超时时间（单位为秒）|URL(IP) 
后面的测试内容全部用分号间隔：
测试记录格式：
IP + 包大小（字节为单位）＋时延＋TTL
64.233.189.99,100,85,243
64.233.189.99,100,48,243
64.233.189.99,100,49,243
。。。
备注：无论每次测试，测试了多少次，每一个PING的结果，都按照这个来记录一次。
*/
t_int _process_realtime_ping(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	LOG(LOG6, "_process_realtime_ping() directory='%s', file='%s', s_bussopid='%s', filesize=%d\n", s_directory, s_fname, s_bussopid, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static CNString s_test_ip, s_test_packsize, s_test_tmdelay, s_test_ttl;

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		if ( check_bussopid_complete(s_bussopid) > 0 ) return 0;
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'PING_FAIL_RATE_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_FAIL_RATE;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE, LOG6);
	int total_count = 0;
	int fail_count = 0;
	
	sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'PING_SPEED_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG6);
	int speed_count[16] = {0};

	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			;
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ',') < s_rline.get_length()) || (s_para.get_length() > 0))
			{
				i_para++;
				if(1 == i_para)					s_test_ip = s_para;
				else if(2 == i_para)			s_test_packsize = s_para;
				else if(3 == i_para)			s_test_tmdelay = s_para;
				else if(4 == i_para)			s_test_ttl = s_para;
			}
			if(4 == i_para)
			{
				appserver_dbinsert_bussop_result_ping(s_bussopid, ++i_seq, ( ( ToInt(s_test_tmdelay.c_str()) == 0 ) ? 'F' : 'S' ), s_test_ip.c_str(), s_test_packsize.c_str(), s_test_tmdelay.c_str(), s_test_ttl.c_str());
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				total_count++;
				if ( CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size() > 0 )
				{
					if ( ToInt(s_test_tmdelay.c_str()) == 0 )
					{
						fail_count++;
					}
				}
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED.size() > 0 )
				{
					for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
					{
						if ( ToInt(s_test_tmdelay.c_str()) == 0 )
						{
							
						}
						else
						{
							if (   ToInt(s_test_tmdelay.c_str()) >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MINVALUE"]) 
								&& ToInt(s_test_tmdelay.c_str()) <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MAXVALUE"]) 
								)
							{
								speed_count[jj]++;
							}
						}
					}
				}
			}
		}
	}
	
	appserver_dbupdate_bussop_success(s_bussopid);
	
	LOG(LOG6, "total_count=%d, fail_count=%d, speed_count[0]=%d, speed_count[1]=%d, speed_count[2]=%d\n", 
		total_count, fail_count, speed_count[0], speed_count[1], speed_count[2]);
	
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			//插入BUSSOP_EXCEPTION_RESULT表
			insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[0]["BUSSOPEXEC_ID"].c_str());
			break;
		}
		else
		{
			int rate = 100 * fail_count / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			break;
		}
		else
		{
			int rate = 100 * speed_count[jj] / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	
	return 0;
}

t_int _process_realtime_pesq(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	LOG(LOG6, "_process_realtime_pesq() directory='%s', file='%s', s_bussopid='%s', filesize=%d\n", s_directory, s_fname, s_bussopid, filelen(s_fname));
	
	return 2;
}

// * ================================================================================ *
// @ _process_realtime_http
// * ================================================================================ *
/*
每次HTTP测试 ， 成功标识：1标识成功 0标识失败或者超时
，文件中记录格式如下：Jason：增加下载的文件大小，DNS过程的大小为0。
第一行：本机号码|HTTP-Realtime|测试次数|超时|URL
如：
13764582377|20090927022425|10|10|http://www.google.com
http://www.google.com,0,2.427,OK

请求URL, 下载所用时间，下载的文件大小，成功标识
请求URL, 下载所用时间，下载的文件大小，成功标识
请求URL, 下载所用时间，下载的文件大小，成功标识
*/
t_int _process_realtime_http(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	LOG(LOG6, "_process_realtime_http() directory='%s', file='%s', s_bussopid='%s', filesize=%d\n", s_directory, s_fname, s_bussopid, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static CNString s_test_url, s_test_usetime, s_test_filesize, s_test_flag;

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		if ( check_bussopid_complete(s_bussopid) > 0 ) return 0;
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'HTTP_FAIL_RATE_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_FAIL_RATE;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE, LOG6);
	int total_count = 0;
	int success_count = 0;
	
	sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'HTTP_SPEED_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG6);
	int total_speed_count = 0;
	int speed_count[16] = {0};
	
	char log_type = 'H';

	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			;
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ',') < s_rline.get_length()) || (s_para.get_length() > 0))
			{
				i_para++;
				if(1 == i_para)					s_test_url = s_para;
				else if(2 == i_para)			s_test_usetime = s_para;
				else if(3 == i_para)			s_test_filesize = s_para;
				else if(4 == i_para)			s_test_flag = s_para;
			}
			
			//----1----
			if(1 == i_para)
			{
				log_type = 'D';
				total_count++;
			}
			
			//http://www.yahoo.com,7.173,125444,OK
			if(4 == i_para)
			{
				appserver_dbinsert_bussop_result_http(s_bussopid, i_seq, (0 <= s_test_flag.find(0, "OK")) ? 'S' : 'F', s_test_url.c_str(), s_test_usetime.c_str(), s_test_filesize.c_str(), log_type);
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				total_speed_count++;
				if ( CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size() > 0 )
				{
					if ( log_type == 'D' && s_test_flag == "OK" )
					{
						success_count++;
					}
				}
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED.size() > 0 )
				{
					//http的速率计算
					int rate = (ToFloat(s_test_usetime.c_str()) <= 0.000001) ? 0 : (int)(ToInt(s_test_filesize.c_str()) * 1.0 / ToFloat(s_test_usetime.c_str()));
					LOG(LOG6, "http file size = %d, time = %.3f, speed = %d\n", ToInt(s_test_filesize.c_str()), ToFloat(s_test_usetime.c_str()), rate);
					
					for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
					{
						if ( s_test_flag != "OK" )
						{
							
						}
						else
						{
							if (   rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MINVALUE"]) 
								&& rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MAXVALUE"]) 
								)
							{
								speed_count[jj]++;
							}
						}
					}
				}
				
				log_type = 'H';
			}
			else
				i_seq++;
		}
	}
	
	appserver_dbupdate_bussop_success(s_bussopid);
	
	LOG(LOG6, "total_count=%d, fail_count=%d, total_speed_count=%d, speed_count[0]=%d, speed_count[1]=%d, speed_count[2]=%d\n", 
		total_count, total_count - success_count, total_speed_count, speed_count[0], speed_count[1], speed_count[2]);
	
	if ( success_count > total_count ) success_count = total_count;
	
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			//插入BUSSOP_EXCEPTION_RESULT表
			insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[0]["BUSSOPEXEC_ID"].c_str());
			break;
		}
		else
		{
			int rate = 100 * (total_count - success_count) / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
	{
		if ( total_speed_count == 0 )
		{
			break;
		}
		else
		{
			int rate = 100 * speed_count[jj] / total_speed_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	
	return 0;
}

t_int _process_realtime_fetion(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	LOG(LOG6, "_process_realtime_fetion() directory='%s', file='%s', s_bussopid='%s', filesize=%d\n", s_directory, s_fname, s_bussopid, filelen(s_fname));
	
	return 0;
}

t_int _process_realtime_vas(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	return _process_routine_vas(s_directory, s_fname, 'T', s_bussopid);
}

#define MAX_CDMACODE_DATA	100

// * ================================================================================ *
// @ _process_realtime_cdma
// * ================================================================================ *
/*
第一行：本机号码|CDMAPARA--Realtime
第二行：参数1   参数2	参数3	。。。。。。  参数N（取决于《参数映射表》中到底写了几个参数）                                                  状态
第一次  值1		值1		值1					值1				I/C/D
第二次	值2		值2		值2					值2				I/C/D
第三次  值3		值3		值3					值3				I/C/D
第四次	值4		值4		值4					值4				I/C/D
。。。
第N次	值N		值N		值N					值N				I/C/D
第N＋1次	三个邻小区信息										I/C/D
*/
t_int _process_realtime_cdma(t_pcstr s_directory, t_pcstr s_fname, t_pcstr s_bussopid)
{
	LOG(LOG6, "_process_realtime_cdma() directory='%s', file='%s', s_bussopid='%s', filesize=%d\n", s_directory, s_fname, s_bussopid, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static CNString s_cdmapara[MAX_CDMACODE_DATA];
	static CNString s_cdmavalue[MAX_CDMACODE_DATA];
	static t_int i_cdmapara, i_cdmavalue, i_temp;

	static CNString s_cdmasql, s_insertsql, s_tempsql;

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		if ( check_bussopid_complete(s_bussopid) > 0 ) return 0;
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOP_TYPE = 'GA' and DISABLE = 0 order by BUSSOPEXEC_TYPE, BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG7);
	int exception_count[MAX_CDMACODE_DATA] = {0};
	int total_count = 0;
	
	ConfigFile paramirror;
	string s_ini = g_ftpscan.m_spath + "i/" + "tmpparamirror.ini";
	n_file_del(s_ini.c_str());
	if ( make_terminal_cdmapara_txt(testfile[0][0].c_str(), s_ini.c_str()) <= 0 )
	{
		
	}
	else
	{
		paramirror.readfile(s_ini.c_str(), " \t|");
	}
	
	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) ;
		else if(2 == i_line) 
		{
			i_cdmapara = 0;
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				s_cdmapara[i_cdmapara] = s_para;
				i_cdmapara++;
			}
			s_cdmasql.Format("INSERT INTO BUSSOP_RESULT_CDMAPARAM (BUSSOP_ID, SEQ, STATUS");
			for(i_temp = 0; i_temp < i_cdmapara; i_temp++) 
			{
				if(NULL == _cdmapara_db_convert(s_cdmapara[i_temp])) 
				{
					LOG(LOG1, "文件 %s 解析失败\n", s_fname);
					return -1;
				}
				s_cdmasql += ", ";
				s_cdmasql += s_cdmapara[i_temp];
			}
			s_cdmasql += ") VALUES ('";
			s_cdmasql += s_bussopid;
			s_cdmasql += "'";
		}
		else 
		{
			i_cdmavalue = 0;
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				s_cdmavalue[i_cdmavalue] = s_para;
				i_cdmavalue++;
			}
			if(i_cdmavalue != (i_cdmapara+1)) break;
			else 
			{
				i_seq++;
				s_insertsql = s_cdmasql;
				s_tempsql.Format(", %d, '%s'", i_seq, s_cdmavalue[i_cdmavalue-1].c_str());
				s_insertsql += s_tempsql;
				for(i_temp = 0; i_temp < i_cdmapara; i_temp++) 
				{
					s_insertsql += ", '";
					s_insertsql += s_cdmavalue[i_temp];
					s_insertsql += "'";
				}
				s_insertsql += ");";
			}
			appserver_execute(s_insertsql.c_str());
			
			total_count++;
			
			for(i_temp = 0; i_temp < i_cdmapara; i_temp++)
			{
				//i_temp是参数的下标
				//testfile[1][i_temp]是参数名，如CHN
				//s_cdmavalue[i_temp]是参数的值
				
				string value_low, value_high;
				if ( strcmp(s_cdmavalue[i_cdmavalue-1].c_str(), "I") == 0 )
				{
					value_low = paramirror[testfile[1][i_temp].c_str()][3];
					value_high = paramirror[testfile[1][i_temp].c_str()][2];
				}
				else
				{
					value_low = paramirror[testfile[1][i_temp].c_str()][5];
					value_high = paramirror[testfile[1][i_temp].c_str()][4];
				}
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				if ( value_low.empty() || value_low == "NULL" || value_high.empty() || value_high == "NULL" )
				{
					//不需要判断
				}
				else
				{
					if ( ToInt(s_cdmavalue[i_temp].c_str()) >= ToInt(value_low) && ToInt(s_cdmavalue[i_temp].c_str()) <= ToInt(value_high) )
					{
						//正常
					}
					else
					{
						//异常
						exception_count[i_temp]++;
						LOG(LOG6, "测试结果异常 state=%s, para:'%s'='%s', IDLE:['%s','%s'], ACTIVE:['%s','%s']\n", 
							s_cdmavalue[i_cdmavalue-1].c_str(), 
							testfile[1][i_temp].c_str(), s_cdmavalue[i_temp].c_str(), 
							paramirror[testfile[1][i_temp].c_str()][3].c_str(), paramirror[testfile[1][i_temp].c_str()][2].c_str(), 
							paramirror[testfile[1][i_temp].c_str()][5].c_str(), paramirror[testfile[1][i_temp].c_str()][4].c_str()
							);
					}
				}
			}
		}
	}
	
	appserver_dbupdate_bussop_success(s_bussopid);
	
	for ( i_temp=0; i_temp<MAX_CDMACODE_DATA; i_temp++ )
	{
		if ( total_count && exception_count[i_temp] )
		{
			int rate = 100 * exception_count[i_temp] / total_count;
			LOG(LOG6, "total_count=%d, para='%s', exception_count=%d, rate=%d\n", 
				total_count, testfile[1][i_temp].c_str(), exception_count[i_temp], rate);
			
			for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
			{
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_TYPE"] != testfile[1][i_temp] ) continue;
				
				if (   rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) 
					&& rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) 
					)
				{
					//插入BUSSOP_EXCEPTION_RESULT表
					insert_bussop_exception_result(s_bussopid, CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
					break;
				}
			}
		}
	}
	
	//需要修正某些CDMA参数的值
	//BestSINR Walsh EV_RxRLPtHRPUTiNSTANT EV_TxRLPtHRPUTINSTANT
	correct_cdma_para(s_bussopid);
	
	return 0;
}

// * ================================================================================ *
// @ _process_routine_cqt
// * ================================================================================ *
/*
第一行：本机号码|CQT-routine|主叫|被叫|次数|呼叫保持时间
后面的测试内容全部用逗号间隔，
第二行：呼叫时间（时分秒），主叫号码，被叫号码，呼叫保持时间，呼叫成功与否（Y是成功F是失败），呼叫失败原因（如果是Y，则为00），呼叫建立时延
第三行：同上。
*/
t_int _process_routine_cqt(t_pcstr s_directory, t_pcstr s_fname)
{
	LOG(LOG6, "_process_routine_cqt() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	
	static string s_bussopid;
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para;

	static CNString s_terminal, s_starttime;
	t_int i_times, i_losts, i_time_total;

	if(strlen(s_fname) < 18)						return -1;
	s_starttime = (s_fname+strlen(s_fname)-18);
	s_starttime.dellen(s_starttime.get_length()-4, 4);

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	if(!strstr(s_fcontent.c_str(), "CQT-Routine"))
	{
		//实时测试log文件
		LOG(LOG6, "实时测试log文件\n");
		string s_bussopid;
		get_realtime_bussopid(s_bussopid, testfile[0][0].c_str(), "BA", time(NULL));
		return _process_realtime_cqt(s_directory, s_fname, s_bussopid.c_str());
	}
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	i_curpos_line = 0; i_line = 0;
	i_time_total = 0; i_losts = 0; i_times = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) ||
				(s_para.get_length() > 0) ) 
			{
					i_para++;
					if(1 == i_para)		
					{
						s_terminal = s_para;
						_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "BA", s_starttime.c_str());
					}
					else if(5 == i_para) i_times = s_para.to_int32();
			}
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ';') < s_rline.get_length()) ||
				(s_para.get_length() > 0))
			{
					i_para++;
					if( (5 == i_para) && ('y' != s_para[0]))	i_losts++;
					else if(6 == i_para)			i_time_total -= s_para.to_int32();
					else if(7 == i_para)				i_time_total += s_para.to_int32();
			}
		}
	}

	t_int i_lostfre = (t_int)((double)i_losts/i_times)*100;
	t_int i_connfre = 100 - i_lostfre;
	t_int i_avgtm = i_time_total/i_times;

	appserver_dbinsert_bussop_result_cqt(s_bussopid.c_str(), i_connfre, i_lostfre, i_avgtm);
	
	appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "BA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');

	return 0;
}

// * ================================================================================ *
// @ _process_routine_ftp
// * ================================================================================ *
/*
第一行：本机号码|FTP-Routine|测试次数|超时|URL 第一行用“|”间隔
记录行格式 ： 下载开始时间，下载结束时间，成功标识
18916503447|FTP-Routine|1|10|ftp://gztest:gztest@116.228.215.12/3M|0|5
1787.098000,1826.623000,OK
*/
t_int _process_routine_ftp(t_pcstr s_directory, t_pcstr s_fname)
{
	LOG(LOG6, "_process_routine_ftp() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static string s_bussopid;
	static CNString s_terminal, s_starttime, s_times, s_tmout, s_direction, s_numprocess;
	static CNString s_test_url, s_test_succflag, s_test_starttime, s_test_endtime;

	if(strlen(s_fname) < 18)						return -1;
	s_starttime = (s_fname+strlen(s_fname)-18);
	s_starttime.dellen(s_starttime.get_length()-4, 4);

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	if(!strstr(s_fcontent.c_str(), "FTP-Routine"))
	{
		//实时测试log文件
		LOG(LOG6, "实时测试log文件\n");
		string s_bussopid;
		get_realtime_bussopid(s_bussopid, testfile[0][0].c_str(), "EA", time(NULL));
		return _process_realtime_ftp(s_directory, s_fname, s_bussopid.c_str());
	}
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'FTP_FAIL_RATE_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_FAIL_RATE;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE, LOG6);
	int total_count = 0;
	int fail_count = 0;
	
	sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'FTP_SPEED_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG6);
	int speed_count[16] = {0};
	
	int filesize = 3*1024*1024;

	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				i_para++;
				if(1 == i_para)	
				{
					s_terminal = s_para;
					_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "EA", s_starttime.c_str());
				}
				else if(3 == i_para) s_times = s_para;
				else if(4 == i_para) s_tmout = s_para;
				else if(5 == i_para) s_test_url = s_para;
				else if(6 == i_para) s_direction = s_para;
				else if(7 == i_para) s_numprocess = s_para;
			}
			
			//根据url中的文件名获取文件大小 ftp://spider:spider@spider.8800.org/ftp/testup.txt
			if ( s_test_url.get_length() > 0 )
			{
				char * pp = strchr(s_test_url.c_str()+6, '/');
				if ( pp )
				{
					string filename = ToString("/home/spider") + pp;
					filesize = filelen(filename.c_str());
					LOG(LOG6, "ftp file '%s' -> '%s', size is %d\n", s_test_url.c_str(), filename.c_str(), filesize);
					if ( filesize == 0 ) filesize = 3*1024*1024;
				}
			}
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ',') < s_rline.get_length()) || (s_para.get_length() > 0))
			{
				i_para++;
				if(1 == i_para)					s_test_starttime = s_para;
				else if(2 == i_para)			s_test_endtime = s_para;
				else if(3 == i_para)			s_test_succflag = s_para;
			}
			
			if(3 == i_para)
			{
				appserver_dbinsert_bussop_result_ftp(s_bussopid.c_str(), ++i_seq, s_test_url.c_str(), ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str()), filesize, (0 <= s_test_succflag.find(0, "OK")) ? 'S' : 'F');
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				total_count++;
				if ( CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size() > 0 )
				{
					if ( s_test_succflag != "OK" )
					{
						fail_count++;
					}
				}
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED.size() > 0 )
				{
					//ftp的速率计算
					int rate = (ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str()) <= 0.000001) ? 0 : (int)(filesize * 1.0 / (ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str())));
					LOG(LOG6, "ftp file size = %d, time = %.3f, speed = %d\n", filesize, ToFloat(s_test_endtime.c_str()) - ToFloat(s_test_starttime.c_str()), rate);
					
					for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
					{
						if ( s_test_succflag != "OK" )
						{
							
						}
						else
						{
							if (   rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MINVALUE"]) 
								&& rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MAXVALUE"]) 
								)
							{
								speed_count[jj]++;
							}
						}
					}
				}
			}
		}
	}
	
	appserver_dbinsert_bussop_content_ftp(s_bussopid.c_str(), s_test_url.c_str(), s_times.c_str(), s_tmout.c_str(), s_direction.c_str(), s_numprocess.c_str());
	
	appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "EA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
	
	LOG(LOG6, "total_count=%d, fail_count=%d, speed_count[0]=%d, speed_count[1]=%d, speed_count[2]=%d\n", 
		total_count, fail_count, speed_count[0], speed_count[1], speed_count[2]);
	
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			//插入BUSSOP_EXCEPTION_RESULT表
			insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[0]["BUSSOPEXEC_ID"].c_str());
			break;
		}
		else
		{
			int rate = 100 * fail_count / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			break;
		}
		else
		{
			int rate = 100 * speed_count[jj] / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	
	return 0;
}

t_int _process_routine_pesq(t_pcstr s_directory, t_pcstr s_fname)
{
	LOG(LOG6, "_process_routine_pesq() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	
	string siglefname = s_fname;
	siglefname = siglefname.substr(siglefname.find_last_of("/") + 1, string::npos);
	
	ConfigLine line;
	line.readline(siglefname, "_.");
	
	char flag_routine;
	string source_name, caller, called, flag_terminal, timestr;
	
	//T_AGAM1F01.WAV_13611957836_18916526178_1_20100201182512.wav
	if ( line[0] == "T" )
	{
		//排队机 实时
		flag_routine = BUSSOPTYPE_REALTIME;
		source_name = line[1] + "." + line[2];
		caller = line[3];
		called = line[4];
		flag_terminal = "no";
		timestr = line[6];
	}
	
	//R_AGAM1F01.WAV_13611957836_18916526178_1_20091214182206.wav
	else if ( line[0] == "R" )
	{
		//排队机 例行
		flag_routine = BUSSOPTYPE_ROUTINE;
		source_name = line[1] + "." + line[2];
		caller = line[3];
		called = line[4];
		flag_terminal = "no";
		timestr = line[6];
	}
	
	//PESQRoutine_22345_18916520759_2_20100104152755.wav
	else if ( line[0] == "PESQRoutine" )
	{
		//终端 例行
		flag_routine = BUSSOPTYPE_ROUTINE;
		source_name = "";
		caller = line[1];
		called = line[2];
		flag_terminal = "yes";
		timestr = line[4];
	}
	
	//PESQRealtime_02154157270_18930352087_2_20100309165327.wav
	else if ( line[0] == "PESQRealtime" )
	{
		//终端 实时
		flag_routine = BUSSOPTYPE_REALTIME;
		source_name = "";
		caller = line[1];
		called = line[2];
		flag_terminal = "yes";
		timestr = line[4];
	}
	
	else
	{
		LOG(LOG1, "_process_routine_pesq() unknown file='%s', error\n", s_fname);
		return 1;
	}
	
	if ( called.empty() )
	{
		//无法获取正确信息
		return 1;
	}
	
	time_t t1 = time_string_to_time_t(timestr);
	
	string s_bussopid;
	
	//根据录音文件名中的信息从数据库获取bussopid，如果没有则插入一条
	CNString sql;
	sql.Format("SELECT BUSSOP.*, BUSSOP_RESULT_PESQ.pDone, BUSSOP_RESULT_PESQ.tDone FROM BUSSOP \
Left Outer Join BUSSOP_RESULT_PESQ ON BUSSOP.BUSSOP_ID = BUSSOP_RESULT_PESQ.BUSSOP_ID \
where TERMINAL_ID = '%s' and INIVATE_TYPE = '%c' and BUSSOP_TYPE = 'CA' and ( STATUS != 'X' and STATUS != 'Y' ) \
order by STARTTIME;", 
			called.c_str(), flag_routine);
	
	DATASET dbresult;
	if ( DB1.db_select(sql.c_str(), dbresult, LOG6) > 0 )
	{
		int timedec = ToInt(myconfig["SamePESQTestTime"][1]);
		
		for ( int i=0; i<dbresult.size(); i++ )
		{
			if ( flag_terminal == "yes" && dbresult[i]["tDone"].length() && ToInt(dbresult[i]["tDone"]) == 1 ) continue;
			if ( flag_terminal == "no"  && dbresult[i]["pDone"].length() && ToInt(dbresult[i]["pDone"]) == 1 ) continue;
			
			if ( dbresult.size() == 1 )
			{
				s_bussopid = dbresult[i]["BUSSOP_ID"];
				break;
			}
			
			time_t t2 = time_string_to_time_t(dbresult[i]["STARTTIME"]);
			
			if ( t1 == -1 || t2 == -1 ) continue;
			
			int tmpdec = ( t1 > t2 ) ? t1-t2 : t2-t1;
			LOG(LOG6, "_process_routine_pesq() 时间差=%d\n", tmpdec);
			
			if ( tmpdec < timedec )
			{
				//时间比较接近，算是匹配
				s_bussopid = dbresult[i]["BUSSOP_ID"];
				timedec = tmpdec;
			}
		}
	}
	
	if ( s_bussopid.empty() )
	{
		_ori_bussopid(s_bussopid, flag_routine, called.c_str(), "CA", timestr.c_str());
		appserver_dbinsert_bussop(s_bussopid.c_str(), flag_routine, "CA", called.c_str(), timestr.c_str(), "", 'B');
	}
	
	//插入数据库表BUSSOP_RESULT_PESQ，后面收到答复时更新表
	sql.Format("select * from BUSSOP_RESULT_PESQ where BUSSOP_ID = '%s';", s_bussopid.c_str());
	DATASET dbresult2;
	if ( DB1.db_select(sql.c_str(), dbresult2, LOG6) == 0 )
	{
		sql.Format("INSERT INTO BUSSOP_RESULT_PESQ (BUSSOP_ID, CALLER, CALLED, SRCFNAME) VALUES ('%s', '%s', '%s', '%s');", 
 				s_bussopid.c_str(), caller.c_str(), called.c_str(), source_name.c_str());
		DB1.db_execute(sql.c_str(), LOG6);
	}
	
	if ( source_name.length() == 0 )
	{
		source_name = dbresult2[0]["SRCFNAME"];
	}
	
	if ( source_name.length() == 0 )
	{
		source_name = "AGAM1F01.WAV";
	}
	
	//数据库中已经有了对应的bussopid，现在需要运算单元
	string outmsg;
	xml_put_para(outmsg, "head", "00000");
	xml_put_para(outmsg, "msg",  "calculate_call");
	xml_put_para(outmsg, "Caller",  caller);
	xml_put_para(outmsg, "Called",  called);
	xml_put_para(outmsg, "CallID",  s_bussopid);
	xml_put_para(outmsg, "CallBeginTime", time_format_str(t1));
	xml_put_para(outmsg, "CallEndTime",   time_format_str(t1+37));
	xml_put_para(outmsg, "SourceFile",    source_name);
	xml_put_para(outmsg, "RecordFile",    siglefname);
	tcp_server1.SendToIdleOne(outmsg.c_str(), outmsg.size(), 6);
	
	return 3;
}

// * ================================================================================ *
// @ _process_routine_ping
// * ================================================================================ *
/*
第一行：本机号码|PING-Routine|测试次数|PING包大小（单位Bytes）|PING时间间隔（单位为秒）|超时时间（单位为秒）|URL(IP) 
后面的测试内容全部用分号间隔：
测试记录格式：
IP + 包大小（字节为单位）＋时延＋TTL
64.233.189.99,100,85,243
64.233.189.99,100,48,243
64.233.189.99,100,49,243
。。。
备注：无论每次测试，测试了多少次，每一个PING的结果，都按照这个来记录一次。
如：
13310123251|PING-Routine|10|50|2|8|www.baidu.com
119.75.213.61,50,1882,58
119.75.213.61,50,2094,58
119.75.213.61,50,1308,58
119.75.213.61,50,2350,58
119.75.213.61,50,2233,58
*/
t_int _process_routine_ping(t_pcstr s_directory, t_pcstr s_fname)
{
	LOG(LOG6, "_process_routine_ping() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static string s_bussopid;
	static CNString s_terminal, s_starttime;
	static CNString s_test_ip, s_test_packsize, s_test_tmdelay, s_test_ttl;
	static CNString s_times, s_packsize, s_tminter, s_tmout, s_url;

	if(strlen(s_fname) < 18)							return -1;
	s_starttime = (s_fname+strlen(s_fname)-18);
	s_starttime.dellen(s_starttime.get_length()-4, 4);

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	if(!strstr(s_fcontent.c_str(), "PING-Routine"))
	{
		//实时测试log文件
		LOG(LOG6, "实时测试log文件\n");
		string s_bussopid;
		get_realtime_bussopid(s_bussopid, testfile[0][0].c_str(), "DA", time(NULL));
		return _process_realtime_ping(s_directory, s_fname, s_bussopid.c_str());
	}
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'PING_FAIL_RATE_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_FAIL_RATE;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE, LOG6);
	int total_count = 0;
	int fail_count = 0;
	
	sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'PING_SPEED_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG6);
	int speed_count[16] = {0};

	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				i_para++;
				if(1 == i_para)	
				{
					s_terminal = s_para;
					_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "DA", s_starttime.c_str());
				}
				else if(3 == i_para)	s_times = s_para;
				else if(4 == i_para)	s_packsize = s_para;
				else if(5 == i_para)	s_tminter = s_para;
				else if(6 == i_para)	s_tmout = s_para;
				else if(7 == i_para)	s_url = s_para;
			}
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ',') < s_rline.get_length()) || (s_para.get_length() > 0))
			{
				i_para++;
				if(1 == i_para)					s_test_ip = s_para;
				else if(2 == i_para)			s_test_packsize = s_para;
				else if(3 == i_para)			s_test_tmdelay = s_para;
				else if(4 == i_para)			s_test_ttl = s_para;
			}
			if(4 == i_para)
			{
				appserver_dbinsert_bussop_result_ping(s_bussopid.c_str(), ++i_seq, ( ( ToInt(s_test_tmdelay.c_str()) == 0 ) ? 'F' : 'S' ), s_test_ip.c_str(), s_test_packsize.c_str(), s_test_tmdelay.c_str(), s_test_ttl.c_str());
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				total_count++;
				if ( CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size() > 0 )
				{
					if ( ToInt(s_test_tmdelay.c_str()) == 0 )
					{
						fail_count++;
					}
				}
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED.size() > 0 )
				{
					for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
					{
						if ( ToInt(s_test_tmdelay.c_str()) == 0 )
						{
							
						}
						else
						{
							if (   ToInt(s_test_tmdelay.c_str()) >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MINVALUE"]) 
								&& ToInt(s_test_tmdelay.c_str()) <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MAXVALUE"]) 
								)
							{
								speed_count[jj]++;
							}
						}
					}
				}
			}
		}
	}
	
	appserver_dbinsert_bussop_content_ping(s_bussopid.c_str(), s_times.c_str(), s_packsize.c_str(), s_tminter.c_str(), s_tmout.c_str(), s_url.c_str());
	
	appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "DA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
	
	LOG(LOG6, "total_count=%d, fail_count=%d, speed_count[0]=%d, speed_count[1]=%d, speed_count[2]=%d\n", 
		total_count, fail_count, speed_count[0], speed_count[1], speed_count[2]);
	
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			//插入BUSSOP_EXCEPTION_RESULT表
			insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[0]["BUSSOPEXEC_ID"].c_str());
			break;
		}
		else
		{
			int rate = 100 * fail_count / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			break;
		}
		else
		{
			int rate = 100 * speed_count[jj] / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	
	return 0;
}

// * ================================================================================ *
// @ _process_routine_http
// * ================================================================================ *
/*
每次HTTP测试 ， 成功标识：OK标识成功 FAIL标识失败或者超时
，文件中记录格式如下：
每测次只需表达测试标识即可，总体测试信息已经在第一行给出（征求各位意见，待确认）
第一行：本机号码|HTTP--Routine|测试次数|超时|URL
如：
13764582377|HTTP--Routine|10|10|http://www.google.com
――――1――――
http://www.google.com,0,2.427,OK
请求URL, 下载所用时间，下载的文件大小，成功标识
请求URL, 下载所用时间，下载的文件大小，成功标识
请求URL, 下载所用时间，下载的文件大小，成功标识
。。。
――――2――――
http://www.google.com,0,2.427,OK
请求URL, 下载所用时间，下载的文件大小，成功标识
请求URL, 下载所用时间，下载的文件大小，成功标识
请求URL, 下载所用时间，下载的文件大小，成功标识
。。。
如：
13310123251|HTTP-Routine|5|8|http://www.yahoo.com
----1----
----2----
http://www.yahoo.com,5.389,126082,OK
http://l.yimg.com/au.yimg.com/i/search/yahoo7_os.xml,1.088,342,OK
http://au.rss.news.yahoo.com/top_stories.xml,1.39,7294,OK
http://au.rss.news.yahoo.com/national.xml,1.501,7631,OK
*/
t_int _process_routine_http(t_pcstr s_directory, t_pcstr s_fname)
{
	LOG(LOG6, "_process_routine_http() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static string s_bussopid;
	static CNString s_terminal, s_starttime;
	static CNString s_test_url, s_test_usetime, s_test_filesize, s_test_flag;
	static CNString s_url, s_times, s_tmout;

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	if(!strstr(s_fcontent.c_str(), "HTTP-Routine"))
	{
		//实时测试log文件
		LOG(LOG6, "实时测试log文件\n");
		string s_bussopid;
		get_realtime_bussopid(s_bussopid, testfile[0][0].c_str(), "FA", time(NULL));
		return _process_realtime_http(s_directory, s_fname, s_bussopid.c_str());
	}
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	if(strlen(s_fname) < 18)							return -1;
	s_starttime = (s_fname+strlen(s_fname)-18);
	s_starttime.dellen(s_starttime.get_length()-4, 4);
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'HTTP_FAIL_RATE_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_FAIL_RATE;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE, LOG6);
	int total_count = 0;
	int success_count = 0;
	
	sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'HTTP_SPEED_EVDO' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG6);
	int total_speed_count = 0;
	int speed_count[16] = {0};
	
	char log_type = 'H';
	
	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				i_para++;
				if(1 == i_para)	
				{
					s_terminal = s_para;
					_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "FA", s_starttime.c_str());
				}
				else if(3 == i_para) s_times = s_para;
				else if(4 == i_para) s_tmout = s_para;
				else if(5 == i_para) s_url = s_para;
			}
		}
		else 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, ',') < s_rline.get_length()) || (s_para.get_length() > 0))
			{
				i_para++;
				if(1 == i_para)					s_test_url = s_para;
				else if(2 == i_para)			s_test_usetime = s_para;
				else if(3 == i_para)			s_test_filesize = s_para;
				else if(4 == i_para)			s_test_flag = s_para;
			}
			
			//----1----
			if(1 == i_para)
			{
				log_type = 'D';
				total_count++;
			}
			
			//http://www.yahoo.com,7.173,125444,OK
			if(4 == i_para)
			{
				appserver_dbinsert_bussop_result_http(s_bussopid.c_str(), i_seq, (0 <= s_test_flag.find(0, "OK")) ? 'S' : 'F', s_test_url.c_str(), s_test_usetime.c_str(), s_test_filesize.c_str(), log_type);
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				total_speed_count++;
				if ( CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size() > 0 )
				{
					if ( log_type == 'D' && s_test_flag == "OK" )
					{
						success_count++;
					}
				}
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED.size() > 0 )
				{
					//http的速率计算
					int rate = (ToFloat(s_test_usetime.c_str()) <= 0.000001) ? 0 : (int)(ToInt(s_test_filesize.c_str()) * 1.0 / ToFloat(s_test_usetime.c_str()));
					LOG(LOG6, "http file size = %d, time = %.3f, speed = %d\n", ToInt(s_test_filesize.c_str()), ToFloat(s_test_usetime.c_str()), rate);
					
					for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
					{
						if ( s_test_flag != "OK" )
						{
							
						}
						else
						{
							if (   rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MINVALUE"]) 
								&& rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MAXVALUE"]) 
								)
							{
								speed_count[jj]++;
							}
						}
					}
				}
				
				log_type = 'H';
			}
			else
				i_seq++;
		}
	}
	
	appserver_dbinsert_bussop_content_http(s_bussopid.c_str(), s_url.c_str(), s_times.c_str(), s_tmout.c_str());
	
	appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "FA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
	
	LOG(LOG6, "total_count=%d, fail_count=%d, total_speed_count=%d, speed_count[0]=%d, speed_count[1]=%d, speed_count[2]=%d\n", 
		total_count, total_count - success_count, total_speed_count, speed_count[0], speed_count[1], speed_count[2]);
	
	if ( success_count > total_count ) success_count = total_count;
	
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_FAIL_RATE.size(); jj++ )
	{
		if ( total_count == 0 )
		{
			//插入BUSSOP_EXCEPTION_RESULT表
			insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[0]["BUSSOPEXEC_ID"].c_str());
			break;
		}
		else
		{
			int rate = 100 * (total_count - success_count) / total_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_FAIL_RATE[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
	{
		if ( total_speed_count == 0 )
		{
			break;
		}
		else
		{
			int rate = 100 * speed_count[jj] / total_speed_count;
			if ( rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) && rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) )
			{
				//插入BUSSOP_EXCEPTION_RESULT表
				insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
				break;
			}
		}
	}
	
	return 0;
}

t_int _process_routine_fetion(t_pcstr s_directory, t_pcstr s_fname)
{
	LOG(LOG6, "_process_routine_fetion() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	
	static CNString s_fcontent;
	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		return 1;
	}
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	ConfigFile testfile(s_fname, "\t ");
	
	string tablename;
	int KQI = ToInt(testfile[0][0]);
	if ( KQI / 1000 == 11 ) tablename = "BUSSOP_RESULT_NEWS";
	if ( KQI / 1000 == 12 ) tablename = "BUSSOP_RESULT_FETION";
	if ( KQI / 1000 == 13 ) tablename = "BUSSOP_RESULT_MUSIC";
	if ( KQI / 1000 == 14 ) tablename = "BUSSOP_RESULT_3GP";
	if ( KQI / 1000 == 15 ) tablename = "BUSSOP_RESULT_TV";
	if ( KQI / 1000 == 16 ) tablename = "BUSSOP_RESULT_MISSCALL";
	
	string s_terminal = testfile[0][1];
	char r_or_t = testfile[0][2][0];
	string s_starttime = testfile[0][3];
	string s_endtime = testfile[0][4];
	int network_type = ToInt(testfile[0][5]);
	
	string s_bussopid;
	_ori_bussopid(s_bussopid, r_or_t, s_terminal.c_str(), testfile[0][0].c_str(), s_starttime.c_str());
	
	for ( int i=1; i<testfile.size(); i++ )
	{
		if ( testfile[i].size() == 0 ) continue;
		
		int i_seq = ToInt(testfile[i][0]);
		string result = testfile[i][1];
		string starttime = testfile[i][2];
		string delay = testfile[i][3];
		string value = testfile[i][4];
		
		appserver_dbinsert_bussop_result_fetion(tablename.c_str(), s_bussopid.c_str(), i_seq, (result=="OK")?'S':'F', starttime.c_str(), ToFloat(delay), value.c_str());
	}
	
	appserver_dbinsert_bussop(s_bussopid.c_str(), r_or_t, network_type, KQI, s_terminal.c_str(), "", 
			s_starttime.c_str(), s_endtime.c_str(), "", 'X', 0, s_fname);
	
	return 0;
}

t_int _process_routine_vas(t_pcstr s_directory, t_pcstr s_fname, t_char r_or_t, t_pcstr bussopid)
{
	LOG(LOG6, "_process_routine_vas() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	if ( r_or_t == 'T' )
	{
		LOG(LOG6, "_process_routine_vas() file for realtime test\n");
	}
	else
	{
		LOG(LOG6, "_process_routine_vas() file for routine test\n");
	}
	
	static CNString s_fcontent;
	if(0 != s_fcontent.readfile(s_fname, 4096))
	{
		//文件不存在
		return 1;
	}
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	ConfigFile testfile(s_fname, "\t ");
	
	string filename = s_fname;
	string onlyfile = filename.substr(filename.find_last_of("/") + 1, string::npos);
	if ( onlyfile.length() == 0 ) onlyfile = filename;
	
	ConfigLine fline;
	fline.readline(onlyfile, "_.");
	
	//TANG 2010-9-5 12:30
	//检查文件名格式是否正确
	if ( fline.size() != 5 )
	{
		//error
		LOG(LOG6, "_process_routine_vas() file='%s', file name format error, ignore\n", s_fname);
		appserver_dbinsert_error_log(onlyfile.c_str(), "文件名格式错误");
		return -1;
	}
	
	//TANG 2010-9-5 12:30
	//检查文件内容格式是否正确
	for ( int i=1; i<testfile.size(); i++ )
	{
		if ( testfile[i].size() > 0 && testfile[i].size() != 6 )
		{
			//error
			LOG(LOG6, "_process_routine_vas() file='%s', content format error, ignore\n", s_fname);
			appserver_dbinsert_error_log(onlyfile.c_str(), "文件内容格式错误");
			return -1;
		}
	}
	
	string s_terminal = "";
	int total_count = ToInt(testfile[0][0]);
	string s_starttime = testfile[0][1];
	string s_endtime = testfile[0][2];
	
	string model = fline[0];
	string address = fline[2];
	string operater = fline[3];
	
	string s_bussopid;
	if ( r_or_t == 'T' )
	{
		s_bussopid = bussopid;
	}
	else
	{
		_ori_bussopid(s_bussopid, r_or_t, s_terminal.c_str(), "", s_starttime.c_str());
	}
	
	//insert REPORT_STAT
	int report_id = appserver_dbinsert_report_stat(total_count, s_starttime.c_str(), s_endtime.c_str(), 
								model.c_str(), address.c_str(), operater.c_str(), s_terminal.c_str(), 
								s_bussopid.c_str());
	
	for ( int i=1; i<testfile.size(); i++ )
	{
		if ( testfile[i].size() == 0 ) continue;
		
		string KQI_ID = testfile[i][0];
		string KQI_NAME = testfile[i][1];
		int i_seq = ToInt(testfile[i][2]);
		string starttime = testfile[i][3];
		string endtime   = testfile[i][4];
		string value     = testfile[i][5];
		
		//insert REPORT_STAT_DETAIL
		appserver_dbinsert_report_stat_detail(report_id, s_terminal.c_str(), KQI_ID.c_str(), KQI_NAME.c_str(), 
					i_seq, starttime.c_str(), endtime.c_str(), value.c_str());
	}
	
	if ( r_or_t == 'T' )
	{
		//appserver_dbupdate_bussop_success(s_bussopid.c_str());
	}
	else
	{
		appserver_dbinsert_bussop(s_bussopid.c_str(), r_or_t, 3, 0, s_terminal.c_str(), "", 
				s_starttime.c_str(), s_endtime.c_str(), "", 'X', 0, s_fname);
	}
	
	return 0;
}

// * ================================================================================ *
// @ _process_routine_cdma
// * ================================================================================ *
/*
第一行：本机号码|CDMAPARA--Routine|NetFailSMSReport=?
第二行：参数1   参数2	参数3	。。。。。。  参数N（取决于《参数映射表》中到底写了几个参数）                                                  状态
后面从第三行开始的测试内容，全部用“|”分割。
时间1（时分秒）  值1		值1		值1					值1				I/C/D
时间2（时分秒）	值2		值2		值2					值2				I/C/D
时间3（时分秒）  值3		值3		值3					值3				I/C/D
时间4（时分秒）	值4		值4		值4					值4				I/C/D
。。。
时间N（时分秒）	值N		值N		值N					值N				I/C/D
时间N＋1（时分秒）	三个邻小区信息										I/C/D
*/
t_int _process_routine_cdma(t_pcstr s_directory, t_pcstr s_fname)
{
	LOG(LOG6, "_process_routine_cdma() directory='%s', file='%s', filesize=%d\n", s_directory, s_fname, filelen(s_fname));
	
	static CNString s_fcontent, s_rline, s_para;
	static t_int i_curpos_line, i_curpos_para, i_line, i_para, i_seq;

	static string s_bussopid;
	static CNString s_terminal, s_starttime;
	
	static CNString s_cdmapara[MAX_CDMACODE_DATA];
	static CNString s_cdmavalue[MAX_CDMACODE_DATA];
	static t_int i_cdmapara, i_cdmavalue, i_temp;
	
	static CNString s_cdmasql, s_insertsql, s_tempsql;

	if(0 != s_fcontent.readfile(s_fname, 1024))
	{
		//文件不存在
		return 1;
	}
	
	ConfigFile testfile(s_fname, "|,");
	
	if(!strstr(s_fcontent.c_str(), "CDMA-Routine"))
	{
		//实时测试log文件
		LOG(LOG6, "实时测试log文件\n");
		string s_bussopid;
		get_realtime_bussopid(s_bussopid, testfile[0][0].c_str(), "GA", time(NULL));
		return _process_realtime_cdma(s_directory, s_fname, s_bussopid.c_str());
	}
	
	CCLogFile.logwrite2(6, s_fcontent.c_str());
	
	if(strlen(s_fname) < 18)							return -1;
	s_starttime = (s_fname+strlen(s_fname)-18);
	s_starttime.dellen(s_starttime.get_length()-4, 4);
	
	CNString sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOP_TYPE = 'GA' and DISABLE = 0 order by BUSSOPEXEC_TYPE, BUSSOPEXEC_LEVEL;";
	DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
	DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG7);
	int exception_count[MAX_CDMACODE_DATA] = {0};
	int total_count = 0;
	
	ConfigFile paramirror;
	string s_ini = g_ftpscan.m_spath + "i/" + "tmpparamirror.ini";
	n_file_del(s_ini.c_str());
	if ( make_terminal_cdmapara_txt(testfile[0][0].c_str(), s_ini.c_str()) <= 0 )
	{
		
	}
	else
	{
		paramirror.readfile(s_ini.c_str(), " \t|");
	}

	i_curpos_line = 0; i_line = 0; i_seq = 0;
	while(s_fcontent.read_line(s_rline, i_curpos_line) < s_fcontent.get_length()) 
	{
		i_line++;
		i_curpos_para = i_para = 0;
		if(1 == i_line) 
		{
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				i_para++;
				if(1 == i_para)	
				{
					s_terminal = s_para;
					_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "GA", s_starttime.c_str());
				}
			}
		}
		else if(2 == i_line) 
		{
			i_cdmapara = 0;
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				s_cdmapara[i_cdmapara] = s_para;
				i_cdmapara++;
			}
			s_cdmasql.Format("INSERT INTO BUSSOP_RESULT_CDMAPARAM (BUSSOP_ID, SEQ, STATUS");
			for(i_temp = 0; i_temp < i_cdmapara; i_temp++) 
			{
				if(NULL == _cdmapara_db_convert(s_cdmapara[i_temp])) 
				{
					LOG(LOG1, "文件 %s 解析失败\n", s_fname);
					return -1;
				}
				s_cdmasql += ", ";
				s_cdmasql += s_cdmapara[i_temp];
			}
			s_cdmasql += ") VALUES ('";
			s_cdmasql += s_bussopid.c_str();
			s_cdmasql += "'";
		}
		else 
		{
			i_cdmavalue = 0;
			while( (s_rline.read_para(s_para, i_curpos_para, '|') < s_rline.get_length()) || (s_para.get_length() > 0) ) 
			{
				s_cdmavalue[i_cdmavalue] = s_para;
				i_cdmavalue++;
			}
			if(i_cdmavalue != (i_cdmapara+1)) break;
			else 
			{
				i_seq++;
				s_insertsql = s_cdmasql;
				s_tempsql.Format(", %d, '%s'", i_seq, s_cdmavalue[i_cdmavalue-1].c_str());
				s_insertsql += s_tempsql;
				for(i_temp = 0; i_temp < i_cdmapara; i_temp++) 
				{
					s_insertsql += ", '";
					s_insertsql += s_cdmavalue[i_temp];
					s_insertsql += "'";
				}
				s_insertsql += ");";
			}
			appserver_execute(s_insertsql.c_str());
			
			total_count++;
			
			for(i_temp = 0; i_temp < i_cdmapara; i_temp++)
			{
				//i_temp是参数的下标
				//testfile[1][i_temp]是参数名，如CHN
				//s_cdmavalue[i_temp]是参数的值
				
				string value_low, value_high;
				if ( strcmp(s_cdmavalue[i_cdmavalue-1].c_str(), "I") == 0 )
				{
					value_low = paramirror[testfile[1][i_temp].c_str()][3];
					value_high = paramirror[testfile[1][i_temp].c_str()][2];
				}
				else
				{
					value_low = paramirror[testfile[1][i_temp].c_str()][5];
					value_high = paramirror[testfile[1][i_temp].c_str()][4];
				}
				
				//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
				if ( value_low.empty() || value_low == "NULL" || value_high.empty() || value_high == "NULL" )
				{
					//不需要判断
				}
				else
				{
					if ( ToInt(s_cdmavalue[i_temp].c_str()) >= ToInt(value_low) && ToInt(s_cdmavalue[i_temp].c_str()) <= ToInt(value_high) )
					{
						//正常
					}
					else
					{
						//异常
						exception_count[i_temp]++;
						LOG(LOG6, "测试结果异常 state=%s, para:'%s'='%s', IDLE:['%s','%s'], ACTIVE:['%s','%s']\n", 
							s_cdmavalue[i_cdmavalue-1].c_str(), 
							testfile[1][i_temp].c_str(), s_cdmavalue[i_temp].c_str(), 
							paramirror[testfile[1][i_temp].c_str()][3].c_str(), paramirror[testfile[1][i_temp].c_str()][2].c_str(), 
							paramirror[testfile[1][i_temp].c_str()][5].c_str(), paramirror[testfile[1][i_temp].c_str()][4].c_str()
							);
					}
				}
			}
		}
	}
	
	appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "GA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
	
	for ( i_temp=0; i_temp<MAX_CDMACODE_DATA; i_temp++ )
	{
		if ( total_count && exception_count[i_temp] )
		{
			int rate = 100 * exception_count[i_temp] / total_count;
			LOG(LOG6, "total_count=%d, para='%s', exception_count=%d, rate=%d\n", 
				total_count, testfile[1][i_temp].c_str(), exception_count[i_temp], rate);
			
			for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
			{
				if ( CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_TYPE"] != testfile[1][i_temp] ) continue;
				
				if (   rate >= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MIN"]) 
					&& rate <= ToInt(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["TIMES_MAX"]) 
					)
				{
					//插入BUSSOP_EXCEPTION_RESULT表
					insert_bussop_exception_result(s_bussopid.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
					break;
				}
			}
		}
	}
	
	//需要修正某些CDMA参数的值
	//BestSINR Walsh EV_RxRLPtHRPUTiNSTANT EV_TxRLPtHRPUTINSTANT
	correct_cdma_para(s_bussopid.c_str());
	
	return 0;
}


// * ================================================================================ *
// @ _func_ftpprocfile_routine
// * ================================================================================ *
t_int _func_ftpprocfile_routine(const char* s_fname_in, void* p_param)
{
	if(!strstr(s_fname_in, ".end"))	return 0;
	
	LOG(LOG6, "_func_ftpprocfile_routine() file='%s'\n", s_fname_in);

	t_int i_ret;
	static CNString sn_fname;
	string sn_srcfname;

	sn_fname = s_fname_in;
	sn_fname.replace(".end", "");
	sn_srcfname = g_ftpscan.m_sfullpath.c_str();
	sn_srcfname += sn_fname.c_str();

	{
		string zipfile = sn_srcfname.c_str() + ToString(".zip");
		LOG(LOG6, "_func_ftpprocfile_routine() file='%s', filesize=%d\n", zipfile.c_str(), filelen(zipfile.c_str()));
		
		static CNString s_tmp_unzip;
		s_tmp_unzip.Format("unzip -o %s.zip -d %s > /dev/null 2>&1", sn_srcfname.c_str(), g_ftpscan.m_sfullpath.c_str());
		linux_system_call(s_tmp_unzip.c_str());
		LOG(LOG6, "linux_system_call() cmd = '%s'\n", s_tmp_unzip.c_str());
	}
	
	sn_srcfname += ".txt";

	switch(g_ftpscan.m_cbuss)
	{
	case BUSS_CQT:
		i_ret = _process_routine_cqt(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	case BUSS_FTP:
		i_ret = _process_routine_ftp(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	case BUSS_PING:
		i_ret = _process_routine_ping(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	case BUSS_HTTP:
		i_ret = _process_routine_http(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	case BUSS_CDMA:
		i_ret = _process_routine_cdma(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	case BUSS_FETION:
		i_ret = _process_routine_fetion(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	case BUSS_VAS:
		i_ret = _process_routine_vas(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	case BUSS_PESQ:
		sn_srcfname.erase(sn_srcfname.length()-4, 4);
		sn_srcfname += ".wav";
		i_ret = _process_routine_pesq(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str());
		break;
	default:
		LOG(LOG1, "ROUTINE_PROC_FILE UNKNOWN_TYPE:%c [%s] \n", g_ftpscan.m_cbuss, sn_fname.c_str());
		break;
	}

	LOG(LOG6, "_func_ftpprocfile_routine() [%s] [ret:%d]\n", sn_fname.c_str(), i_ret);

	//0-处理成功，1-文件不存在，2-实时测试文件，3-尚未完成处理，-1错误
	if(0 == i_ret || 1 == i_ret || -1 == i_ret) 
	{
		_ftpprocfile_movefile(sn_fname.c_str());
	}
	
	LOG(LOG6, "\n");
	return i_ret;
}

// * ================================================================================ *
// @ _func_ftpprocfile_realtime
// * ================================================================================ *
t_int _func_ftpprocfile_realtime(t_pcstr s_fname_in, t_pcstr s_bussopid, t_pcstr s_terminal)
{
	LOG(LOG6, "_func_ftpprocfile_realtime() file='%s', s_bussopid='%s', s_terminal='%s'\n", s_fname_in, s_bussopid, s_terminal);
	
	t_int i_ret = 1;
	static CNString sn_srcfname, sn_fname;

	_ftpprocfile_initialdir();

	sn_fname = s_fname_in;
	sn_fname.replace(".txt", "");
	sn_fname.replace(".zip", "");
	sn_srcfname = g_ftpscan.m_sfullpath.c_str();
	sn_srcfname += sn_fname;

	{
		string zipfile = sn_srcfname.c_str() + ToString(".zip");
		LOG(LOG6, "_func_ftpprocfile_realtime() file='%s', filesize=%d\n", zipfile.c_str(), filelen(zipfile.c_str()));
		
		static CNString s_tmp_unzip;
		s_tmp_unzip.Format("unzip -o %s.zip -d %s > /dev/null 2>&1", sn_srcfname.c_str(), g_ftpscan.m_sfullpath.c_str());
		linux_system_call(s_tmp_unzip.c_str());
		LOG(LOG6, "linux_system_call() cmd = '%s'\n", s_tmp_unzip.c_str());
	}
	
	sn_srcfname += ".txt";
	
	switch(g_ftpscan.m_cbuss)
	{
	case BUSS_CQT:
		i_ret = _process_realtime_cqt(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	case BUSS_FTP:
		i_ret = _process_realtime_ftp(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	case BUSS_PING:
		i_ret = _process_realtime_ping(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	case BUSS_HTTP:
		i_ret = _process_realtime_http(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	case BUSS_CDMA:
		i_ret = _process_realtime_cdma(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	case BUSS_FETION:
		i_ret = _process_realtime_fetion(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	case BUSS_VAS:
		i_ret = _process_realtime_vas(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	case BUSS_PESQ:
		i_ret = _process_realtime_pesq(g_ftpscan.m_sfullpath.c_str(), sn_srcfname.c_str(), s_bussopid);
		break;
	default:
		LOG(LOG1, "REALTIME_PROC_FILE UNKNOWN_TYPE [%c] [%s]\n", g_ftpscan.m_cbuss, sn_srcfname.c_str());
		break;
	}

	LOG(LOG6, "_func_ftpprocfile_realtime() [%s] [ret:%d]\n", sn_fname.c_str(), i_ret);

	//0-处理成功，1-文件不存在，2-实时测试文件，3-尚未完成处理，-1错误
	if(0 == i_ret || 1 == i_ret || -1 == i_ret) 
	{
		_ftpprocfile_movefile(sn_fname.c_str());
	}
	
	LOG(LOG6, "\n");
	return i_ret;
}



// * ================================================================================ *
// @ thread_ftp_procfile
// * ================================================================================ *
t_pvoid thread_ftp_procfile(t_pvoid p_param)
{
	t_char s_rpack[4096];
	t_int  i_plen, i_ret;
	time_t tm_cur, tm_lstscan = time(NULL);

	string s_msg, s_fname, s_bussopid, s_terminal, s_authpwd, s_ini;
	CClientMsg msg_rsp;

	appserver_opendb(g_ftpscan.m_sdb_name.c_str(), g_ftpscan.m_sdb_ip.c_str(), g_ftpscan.m_sdb_uname.c_str(), g_ftpscan.m_sdb_upwd.c_str());

	while(g_ftpscan.m_bruning)
	{
		n_sleep(10);
		tm_cur = time(NULL);

		if(0 == g_ftpscan.p_buf_command->pack_pop(s_rpack, i_plen)) 
		{
			s_rpack[i_plen] = 0;
			
			LOG(LOG6, "thread_ftp_procfile() recv msg '%s'\n", s_rpack);
			
			CClientMsg::ansy_getitem(s_rpack, "msg", s_msg);
			CClientMsg::ansy_getitem(s_rpack, "filename", s_fname);
			CClientMsg::ansy_getitem(s_rpack, "bussopid", s_bussopid);
			CClientMsg::ansy_getitem(s_rpack, "terminal", s_terminal);
			CClientMsg::ansy_getitem(s_rpack, "authpwd", s_authpwd);
			
			//升级配置文件
			if ( s_msg == "HB_1_A2F" )
			{
				char zipname[32] = {0};
				sprintf(zipname, "a%02x", ++g_zip_name_value);
				if ( g_zip_name_value >= 0xFF ) g_zip_name_value = 0;
				
				s_ini = g_ftpscan.m_spath + "i/" + "config.ini";
				n_file_del(s_ini.c_str());
				
				if ( make_terminal_config_ini(s_terminal.c_str(), s_ini.c_str()) <= 0 )
				{
					i_ret = 1;
					goto GET_CONFIG_ERROR;
				}
				
				s_ini = g_ftpscan.m_spath + "i/" + "routine.ini";
				n_file_del(s_ini.c_str());
				
				if ( make_terminal_routine_ini(s_terminal.c_str(), s_ini.c_str()) <= 0 )
				{
					i_ret = 1;
					goto GET_CONFIG_ERROR;
				}
				
				s_ini = g_ftpscan.m_spath + "i/" + "paramirror.ini";
				n_file_del(s_ini.c_str());
				
				if ( make_terminal_cdmapara_txt(s_terminal.c_str(), s_ini.c_str()) <= 0 )
				{
					i_ret = 1;
					goto GET_CONFIG_ERROR;
				}
				
				s_ini = g_ftpscan.m_spath + "i/";
				static CNString s_tmp_unzip;
				s_tmp_unzip.Format("cd %s; zip %s config.ini routine.ini paramirror.ini > /dev/null 2>&1", s_ini.c_str(), zipname);
				linux_system_call(s_tmp_unzip.c_str());
				LOG(LOG6, "linux_system_call() cmd = '%s'\n", s_tmp_unzip.c_str());
				
				if ( myconfig["FTPSERVER_PORT"][1] == "21" )
				{
					s_ini = "ftp://" + myconfig["FTPSERVER_UNAME"][1] + ":" + myconfig["FTPSERVER_PASSWD"][1] + "@" + myconfig["FTPSERVER_IP"][1] + "/" + myconfig["FTPSERVER_PATH"][1] + "/i/" + zipname + ".zip";
				}
				else
				{
					s_ini = "ftp://" + myconfig["FTPSERVER_UNAME"][1] + ":" + myconfig["FTPSERVER_PASSWD"][1] + "@" + myconfig["FTPSERVER_IP"][1] + ":" + myconfig["FTPSERVER_PORT"][1] + "/" + myconfig["FTPSERVER_PATH"][1] + "/i/" + zipname + ".zip";
				}
				
				i_ret = 0;
				
				GET_CONFIG_ERROR:
				
				s_msg.erase(s_msg.length()-3, 3);
				s_msg += "F2A";
				
				if(0 == i_ret) 
				{
					msg_rsp.ori_init(s_msg.c_str());
					msg_rsp.ori_additem("terminal", s_terminal.c_str());
					msg_rsp.ori_additem("bussopid", s_bussopid.c_str());
					msg_rsp.ori_additem("authpwd", s_authpwd.c_str());
					msg_rsp.ori_additem("url", s_ini.c_str());
					msg_rsp.ori_additem("result", 0);
					msg_rsp.ori_end();
				}
				else 
				{
					msg_rsp.ori_init(s_msg.c_str());
					msg_rsp.ori_additem("terminal", s_terminal.c_str());
					msg_rsp.ori_additem("bussopid", s_bussopid.c_str());
					msg_rsp.ori_additem("result", 1);
					msg_rsp.ori_additem("result_des", "获取配置文件失败");
					msg_rsp.ori_end();
				}
				
				nsocket_sendlen(g_sock, msg_rsp.packet().c_str(), msg_rsp.packet().length(), 100);
				LOG(LOG6, "thread_ftp_procfile() send msg '%s'\n", msg_rsp.packet().c_str());
			}
			
			else if ( s_msg == "CA_A2F" )
			{
				LOG(LOG6, "thread_ftp_procfile() ignore msg\n\n");
			}
			
			//处理log文件
			else if ( s_msg == "55510" )
			{
				g_ftpscan.m_cbusstype = BUSSOPTYPE_REALTIME;
				g_ftpscan.m_cbuss = BUSS_VAS;
				
				i_ret = _func_ftpprocfile_realtime(s_fname.c_str(), s_bussopid.c_str(), s_terminal.c_str());
				
				if(0 == i_ret) 
				{
					msg_rsp.ori_init(s_msg.c_str());
					msg_rsp.ori_additem("terminal", s_terminal.c_str());
					msg_rsp.ori_additem("bussopid", s_bussopid.c_str());
					msg_rsp.ori_additem("result", 0);
					msg_rsp.ori_end();
				}
				else 
				{
					msg_rsp.ori_init(s_msg.c_str());
					msg_rsp.ori_additem("terminal", s_terminal.c_str());
					msg_rsp.ori_additem("bussopid", s_bussopid.c_str());
					msg_rsp.ori_additem("result", 1);
					msg_rsp.ori_additem("result_des", "文件处理失败");
					msg_rsp.ori_end();
				}
				
				nsocket_sendlen(g_sock, msg_rsp.packet().c_str(), msg_rsp.packet().length(), 100);
				LOG(LOG6, "thread_ftp_procfile() send msg '%s'\n", msg_rsp.packet().c_str());
			}
			
			else
			{
				LOG(LOG6, "thread_ftp_procfile() unknown msg, ignore\n\n");
			}
		}
		
		if( (tm_cur - tm_lstscan) > g_ftpscan.m_itmdelay_scan) 
		{
			tm_lstscan = tm_cur;
			
			if (    ToInt(myconfig["FTP_SCAN_TIME"][1]) == 0 
				|| (ToInt(myconfig["FTP_SCAN_TIME"][1]) > 0 && seconds_from_day_0hour0min0sec(time(NULL)) >= ftpscan_start_seconds && seconds_from_day_0hour0min0sec(time(NULL)) < ftpscan_stop_seconds) )
			{
				LOG(LOG6, "thread_ftp_procfile() scan begin\n");
				
				g_ftpscan.m_cbusstype = BUSSOPTYPE_ROUTINE;
				
				g_ftpscan.m_cbuss = BUSS_CQT;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
				
				g_ftpscan.m_cbuss = BUSS_CDMA;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
				
				g_ftpscan.m_cbuss = BUSS_FTP;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
				
				g_ftpscan.m_cbuss = BUSS_HTTP;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
				
				g_ftpscan.m_cbuss = BUSS_PING;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
				
				g_ftpscan.m_cbuss = BUSS_PESQ;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
				
				g_ftpscan.m_cbuss = BUSS_FETION;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
				
				g_ftpscan.m_cbuss = BUSS_VAS;
				_ftpprocfile_initialdir();
				n_dir_findfile(g_ftpscan.m_sfullpath.c_str(), _func_ftpprocfile_routine, NULL);
			}
			else
			{
				LOG(LOG6, "thread_ftp_procfile() NOT scan by config FTP_SCAN_TIME setting\n");
			}
		}
	}
	
	return NULL;
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
	
	LOG(LOG6, "server: recv msg, sockfd=%d, sockname='%s', ip='%s', port=%d, msg='%s'\n", sockfd, sockname.c_str(), ip.c_str(), port, buf);
	
	if ( xml["msg"] == "calculate_result" )
	{
		string flag_terminal;
		
		//更新数据库
		CNString sql;
		if ( strncmp(xml["RecordFile"].c_str(), "R_", 2) == 0 || strncmp(xml["RecordFile"].c_str(), "T_", 2) == 0 )
		{
			//排队机的录音文件
			flag_terminal = "no";
			sql.Format("update BUSSOP_RESULT_PESQ set pDSTFNAME = '%s', pRESULT1 = %s, pRESULT2 = %s, pRESULT3 = %s, pDone = 1 \
where BUSSOP_ID = '%s';", 
					xml["RecordFile"].c_str(), xml["Result1"].c_str(), xml["Result2"].c_str(), xml["Result3"].c_str(), 
					xml["CallID"].c_str()
					);
		}
		else
		{
			//终端的录音文件
			flag_terminal = "yes";
			sql.Format("update BUSSOP_RESULT_PESQ set tDSTFNAME = '%s', tRESULT1 = %s, tRESULT2 = %s, tRESULT3 = %s, tDone = 1 \
where BUSSOP_ID = '%s';", 
					xml["RecordFile"].c_str(), xml["Result1"].c_str(), xml["Result2"].c_str(), xml["Result3"].c_str(), 
					xml["CallID"].c_str()
					);
		}
		DB1.db_execute(sql.c_str(), LOG6);
		
		//异常数据需要插入BUSSOP_EXCEPTION_RESULT表
		{
			string sql;
			if ( flag_terminal == "no" )
			{
				sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'PESQ_UP' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
			}
			else
			{
				sql = "select * from CONFIG_BUSSOP_EXCEPTION where BUSSOPEXEC_TYPE = 'PESQ_DOWN' and DISABLE = 0 order by BUSSOPEXEC_LEVEL;";
			}
			DATASET CONFIG_BUSSOP_EXCEPTION_SPEED;
			DB1.db_select(sql.c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED, LOG6);
			
			for ( int jj=0; jj<CONFIG_BUSSOP_EXCEPTION_SPEED.size(); jj++ )
			{
				if (   ToFloat(xml["Result1"].c_str()) >= ToFloat(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MINVALUE"]) 
					&& ToFloat(xml["Result1"].c_str()) <= ToFloat(CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["MAXVALUE"]) 
					)
				{
					//插入BUSSOP_EXCEPTION_RESULT表
					insert_bussop_exception_result(xml["CallID"].c_str(), CONFIG_BUSSOP_EXCEPTION_SPEED[jj]["BUSSOPEXEC_ID"].c_str());
					break;
				}
			}
		}
		
		_ftpprocfile_movefile((xml["RecordFile"].substr(0, xml["RecordFile"].length()-4)).c_str());
		
		//只有排队机和终端的录音文件都计算过以后，测试状态才能是结束
		sql.Format("select * from BUSSOP_RESULT_PESQ where BUSSOP_ID = '%s' and pDone = 1 and tDone = 1;", xml["CallID"].c_str());
		DATASET dbresult;
		if ( DB1.db_select(sql.c_str(), dbresult, LOG6) > 0 )
		{
			appserver_dbupdate_bussop_success(xml["CallID"].c_str());
		}
	}
	
	else if ( xml["msg"] == "login" )
	{
		//login消息表明身份，如运算单元
		string type;
		xml_get_para_string(buf, "<type>", "</type>", type);
		
		if ( type == "calculate_unit" )
		{
			LOG(LOG6, "service: calculate_unit login\n");
		}
		
		//答复
		string outmsg;
		xml_put_para(outmsg, "head", "00000");
		xml_put_para(outmsg, "msg",  xml["msg"]);
		xml_put_para(outmsg, "result", 0);
		tcp_server1.Send(sockfd, outmsg.c_str(), outmsg.size(), 6);
	}
	
	else if ( xml["msg"] == "reload_config" )
	{
		if ( myconfig.readfile(CONFIG_FILE_PATH) == 0 )
		{
			LOG(LOG1, "ftpscan: read config file '%s' error\n", CONFIG_FILE_PATH);
		}
		
		//读配置文件后要根据配置文件内容做处理
		set_ftpscan_time(myconfig["FTP_SCAN_TIME"][2], myconfig["FTP_SCAN_TIME"][3]);
		
		CCLogFile.SetLoglevel(ToInt(myconfig["LogLevel"][1]));
		
		LOG(LOG1, "ftpscan: FTP_SCAN_TIME from '%02d:%02d' to '%02d:%02d'\n", ftpscan_start_seconds/3600, (ftpscan_start_seconds%3600)/60, ftpscan_stop_seconds/3600, (ftpscan_stop_seconds%3600)/60);
		
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

