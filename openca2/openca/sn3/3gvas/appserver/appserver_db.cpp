#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "StdAfx.h"
#include "appserver_db.h"
#include "../common/n_common.h"
#include "../common/n_string.h"

#include <string>
using namespace std;

#define LOG CCLogFile.logwrite
extern LogFile CCLogFile;

char g_s20_sqlstr[4096];
char g_s20_sqltablename[256];

MYSQLDB DB1;

CSQLAPIDBConnect	g_dbconnect;

int write_file(const char * configfile, const void * buf, int buflen)
{
	int fd, re;
	if ( (fd = open(configfile, O_RDWR|O_CREAT|O_TRUNC, (S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH))) < 0 )
	{
		LOG(LOG1, "write_file(): open() error\n");
		return 0;
	}
	
	if ( (re = write(fd, buf, buflen)) != buflen )
	{
		LOG(LOG1, "write_file(): write() error\n");
		close(fd);
		return 0;
	}
	
	fchmod(fd, 0644);
	
	close(fd);
	return 1;
}

// * ================================================================================ *
// @ _ori_bussopid
// * ================================================================================ *
t_void _ori_bussopid(string& s_bussopid, t_char c_type, t_pcstr s_terminal, t_pcstr s_bussoptype, t_pcstr s_starttime)
{
	t_char s_type[2];
	s_type[0] = c_type;
	s_type[1] = 0;
	s_bussopid = ((string)s_type + s_terminal + s_bussoptype + s_starttime);
	{
		static t_int i_bussseq = 1;
		static char s_temp[6];
		sprintf(s_temp, "%05d", i_bussseq++);
		s_bussopid += s_temp;
		if(i_bussseq >= 99999)	i_bussseq = 1;
	}
}

#ifdef WIN32
#else
// mysql -h 127.0.0.1 -uspider -pspider -D3GTEST
// mysql -h 127.0.0.1 -uspider -pspider < 3gtest.sql

// * ============================================================== *
// @ db_open
// * ============================================================== *
t_int CSQLAPIDBConnect::connect_db(t_pcstr dbname, t_pcstr servername, t_pcstr usrname, t_pcstr usrpwd)
{
	mysql_init(&m_pConnection);

	//	char value=1;
	//	mysql_options(&pConnection, MYSQL_OPT_RECONNECT, (char *)&value);
	if(mysql_real_connect(&m_pConnection, !servername || !servername[0] ? "127.0.0.1" : servername, 
		!usrname || !usrname[0] ? "spider" : usrname, 
		!usrpwd || !usrpwd[0] ? "spider" : usrpwd, 
		!dbname || !dbname[0] ? "3GTEST" : dbname, 
		0, NULL, 0))
	{
		LOG(LOG4, "Database:%s@%s is connected successfully as user %s\n",dbname, servername, usrname);
	}
	else
	{
		if(mysql_errno(&m_pConnection))
		{
			LOG(LOG1, "Database error=%d: '%s'\n", mysql_errno(&m_pConnection),mysql_error(&m_pConnection));
		}
		//return -1;
	}
	
	DB1.db_set_character_set("gbk");
	DB1.db_log(&CCLogFile);
	if ( DB1.db_open(dbname, servername, usrname, usrpwd) < 0 )
	{
		//error
		LOG(LOG1, "connect_db(): open database error\n");
		return -1;
	}
	
	return 0;
}

// * ============================================================== *
// @ db_close
// * ============================================================== *
int CSQLAPIDBConnect::close_db()
{
	mysql_close(&m_pConnection);
	LOG(LOG4, "Database is disconnected successfully.\n\n");
	
	DB1.db_close();
	
	return 1;
}

// * ============================================================== *
// @ db_execute
// * ============================================================== *
t_int CSQLAPIDBConnect::db_execute(t_pcstr s_sql)
{
	int i_ret = mysql_query(&m_pConnection, s_sql);
	if(i_ret)
	{
		LOG(LOG1, "Database error=%d: '%s' db_execute[%s]\n", mysql_errno(&m_pConnection),mysql_error(&m_pConnection), s_sql);
		if(mysql_query(&m_pConnection, s_sql))	return -1;
	}
	LOG(LOG6, "db_execute() sql = '%s'\n", s_sql);
	return 0;
}

// * ============================================================== *
// @ db_select_start
// * ============================================================== *
t_int CSQLAPIDBConnect::db_select_start(t_pcstr s_sql, CSQLAPIDBSelectResult& sqlresult)
{
	if(mysql_query(&m_pConnection, s_sql)) 
	{
		LOG(LOG1, "Database error=%d: '%s' db_select[%s]\n", mysql_errno(&m_pConnection),mysql_error(&m_pConnection), s_sql);
		if(mysql_query(&m_pConnection, s_sql))	return -1;
	}
	if ( !(sqlresult.p_res = mysql_store_result(&m_pConnection))) 
	{
		LOG(LOG1, "Database error=%d: '%s' db_select[%s]\n", mysql_errno(&m_pConnection),mysql_error(&m_pConnection), s_sql);
		return -3;
	}
	LOG(LOG6, "db_select() sql = '%s'\n", s_sql);
	return 0;
}

// * ============================================================== *
// @ db_select_fetch
// * ============================================================== *
char** CSQLAPIDBConnect::db_select_fetch(CSQLAPIDBSelectResult& sqlresult)
{
	char** s_result = mysql_fetch_row(sqlresult.p_res);
	if(s_result) 
	{
		static string s_outprint;
		s_outprint.clear();
		int rows = db_select_getrows(sqlresult);
		for(int i = 0; i < rows; i++) 
		{
			s_outprint += "[";
			s_outprint += s_result[i];
			s_outprint += "]  ";
		}
		LOG(LOG1, "db_fetch %s\n", s_outprint.c_str());
	}
	return s_result;
}

#endif

nt_mutex g_mutex_dbop;
// * ============================================================== *
// @ appserver_opendb
// * ============================================================== *
int appserver_opendb(t_pcstr s_dbname, t_pcstr s_ip, t_pcstr s_user, t_pcstr s_pwd)
{
	nmutex_initial(&g_mutex_dbop);
	return g_dbconnect.connect_db(s_dbname, s_ip, s_user, s_pwd);
}

// * ============================================================== *
// @ appserver_closedb
// * ============================================================== *
int appserver_closedb()
{
	nmutex_destroy(&g_mutex_dbop);
	return g_dbconnect.close_db();
}

// * ============================================================== *
// @ appserver_execute
// * ============================================================== *
int appserver_execute(t_pcstr s_sql)
{
	return DB1.db_execute(s_sql, LOG6);
	
	/*
	t_int i_ret;
	nmutex_lock(&g_mutex_dbop);
	i_ret = g_dbconnect.db_execute(s_sql);
	nmutex_unlock(&g_mutex_dbop);
	return i_ret;
	*/
}

// * ============================================================== *
// @ appserver_select_start
// * ============================================================== *
int appserver_select_start(t_pcstr s_sql, CSQLAPIDBSelectResult& result)
{
	t_int i_ret;
	nmutex_lock(&g_mutex_dbop);
	i_ret = g_dbconnect.db_select_start(s_sql, result);
	nmutex_unlock(&g_mutex_dbop);
	return i_ret;
}
// * ============================================================== *
// @ appserver_select_fetch
// * ============================================================== *
char** appserver_select_fetch(CSQLAPIDBSelectResult& result)
{
	return g_dbconnect.db_select_fetch(result);
}
// * ============================================================== *
// @ appserver_select_end
// * ============================================================== *
int appserver_select_end(CSQLAPIDBSelectResult& result)
{
	return g_dbconnect.db_select_end(result);
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_content_cqt
// * ============================================================== *
t_int appserver_dbinsert_bussop_content_cqt(t_pcstr s_bussopid, t_char c_type, t_pcstr s_called, t_pcstr s_tminter, t_pcstr s_times, t_pcstr s_test_tminter, t_pcstr s_test_times, t_pcstr s_tm_callhold)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_CONTENT_CQT (BUSSOP_ID, TYPE, CALLED, TM_INTER, TIMES, TEST_TMINTER, TEST_TIMES, TM_CALLHOLD)\
 VALUES('%s', '%c', '%s', '%s', '%s', '%s', '%s', '%s');",
				 s_bussopid, c_type, s_called, s_tminter, s_times, s_test_tminter, s_test_times, s_tm_callhold);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_content_ping
// * ============================================================== *
t_int appserver_dbinsert_bussop_content_ping(t_pcstr s_bussopid, t_pcstr s_times, t_pcstr s_packsize, t_pcstr s_tminter, t_pcstr s_tmout, t_pcstr s_url)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_CONTENT_PING (BUSSOP_ID, TIMES, PACKSIZE, TM_INTER, TMOUT, URL)\
 VALUES('%s', '%s', '%s', '%s', '%s', '%s');",
				 s_bussopid, s_times, s_packsize, s_tminter, s_tmout, s_url);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_content_ftp
// * ============================================================== *
t_int appserver_dbinsert_bussop_content_ftp(t_pcstr s_bussopid, t_pcstr s_url, t_pcstr s_times, t_pcstr s_tmout, t_pcstr s_direction, t_pcstr s_numprocess)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_CONTENT_FTP (BUSSOP_ID, TIMES, TMOUT, URL, DIRECTION, NUMBER_PROCESS)\
 VALUES('%s', '%s', '%s', '%s', '%s', '%s');",
				 s_bussopid, s_times, s_tmout, s_url, s_direction, s_numprocess);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_content_http
// * ============================================================== *
t_int appserver_dbinsert_bussop_content_http(t_pcstr s_bussopid, t_pcstr s_url, t_pcstr s_times, t_pcstr s_tmout)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_CONTENT_HTTP (BUSSOP_ID, TIMES, TMOUT, URL)\
 VALUES('%s', '%s', '%s', '%s');",
				 s_bussopid, s_times, s_tmout, s_url);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop
// * ============================================================== *
t_int appserver_dbinsert_bussop(t_pcstr s_bussopid, t_char c_inivatetype, t_pcstr s_bussoptype, t_pcstr s_terminal, t_pcstr s_starttime, t_pcstr s_user, t_char c_status)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP (BUSSOP_ID, INIVATE_TYPE, BUSSOP_TYPE, TERMINAL_ID, STARTTIME, USER_NAME, STATUS) \
VALUES ('%s', '%c', '%s', '%s', '%s', '%s', '%c');",
			s_bussopid, c_inivatetype, s_bussoptype, s_terminal, s_starttime, s_user, c_status);
	return DB1.db_execute(s_sql.c_str(), LOG6);
}

t_int appserver_dbinsert_bussop(t_pcstr s_bussopid, t_pcstr s_tid, t_char c_inivatetype, t_pcstr s_kqi_type, t_pcstr s_terminal, t_pcstr s_from_terminal, t_pcstr s_starttime, t_pcstr s_timeout, t_pcstr s_user, t_char c_status)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP (BUSSOP_ID, TID, INIVATE_TYPE, NETWORK_TYPE, KQI_TYPE, TERMINAL_ID, FROM_TERMINAL_ID, \
STARTTIME, TIMEOUT, USER_NAME, STATUS) \
VALUES ('%s', '%s', '%c', %d, '%s', '%s', '%s', '%s', '%s', '%s', '%c');",
			s_bussopid, s_tid, c_inivatetype, 0, s_kqi_type, s_terminal, s_from_terminal, 
			s_starttime, s_timeout, s_user, c_status);
	return DB1.db_execute(s_sql.c_str(), LOG6);
}

t_int appserver_dbinsert_bussop(t_pcstr s_bussopid, t_char r_or_t, int network_type, int kqi_type, 
	t_pcstr s_terminal, t_pcstr s_terminal_from, t_pcstr s_starttime, t_pcstr s_endtime, t_pcstr s_user, 
	t_char c_status, int rescode, t_pcstr resfname)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP (BUSSOP_ID, INIVATE_TYPE, NETWORK_TYPE, KQI_TYPE, TERMINAL_ID, FROM_TERMINAL_ID, \
STARTTIME, ENDTIME, USER_NAME, STATUS, RES_CODE, RES_FNAM) \
VALUES ('%s', '%c', %d, %d, '%s', '%s', '%s', '%s', '%s', '%c', %d, '%s');", 
			s_bussopid, r_or_t, network_type, kqi_type, s_terminal, s_terminal_from, 
			s_starttime, s_endtime, s_user, c_status, rescode, resfname);
	return DB1.db_execute(s_sql.c_str(), LOG6);
}

t_int appserver_dbupdate_bussop_resfname(t_pcstr s_bussopid, t_pcstr s_resfname)
{
	CNString s_sql;
	s_sql.Format("UPDATE BUSSOP SET RES_FNAM='%s' WHERE BUSSOP_ID='%s';", s_resfname, s_bussopid);
	return appserver_execute(s_sql.c_str());
}

t_int appserver_dbupdate_bussop_status(t_pcstr s_bussopid, char c_status, t_int i_rescode)
{
	CNString s_sql;
	s_sql.Format("UPDATE BUSSOP SET STATUS='%c', RES_CODE=%d WHERE BUSSOP_ID='%s';",
		c_status, i_rescode, s_bussopid);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbupdate_bussop
// * ============================================================== *
t_int appserver_dbupdate_bussop_error(t_pcstr s_bussopid, t_int i_err)
{
	CNString s_sql;
	s_sql.Format("UPDATE BUSSOP SET STATUS='Y', RES_CODE=%d WHERE BUSSOP_ID='%s';",
		i_err, s_bussopid);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbupdate_bussop_success
// * ============================================================== *
t_int appserver_dbupdate_bussop_success(t_pcstr s_bussopid)
{
	CNString s_sql;
	s_sql.Format("UPDATE BUSSOP SET STATUS='X' WHERE BUSSOP_ID='%s';",s_bussopid);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_queryver
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_queryver(t_pcstr s_bussopid, t_pcstr s_curver, t_pcstr s_bkver)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_QUERYVERSION (BUSSOP_ID, CUR_VERSION, BACK_VERSION)\
 VALUES('%s', '%s', '%s');",
				 s_bussopid, s_curver, s_bkver);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_cqt
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_cqt(t_pcstr s_bussopid, t_int i_conn_frequency, t_int i_lost_frequency, t_int i_time_avg)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_CQT (BUSSOP_ID, FREQUENCY_CONNECT, FREQUENCY_LOST, TMDELAY_CALL)\
 VALUES('%s', '%d', '%d', '%d');",
				 s_bussopid, i_conn_frequency, i_lost_frequency, i_time_avg);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_cqt
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_cqt(t_pcstr s_bussopid, t_pcstr s_freconn, t_pcstr s_fredis, t_pcstr s_calltime)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_CQT (BUSSOP_ID, FREQUENCY_CONNECT, FREQUENCY_LOST, TMDELAY_CALL)\
 VALUES('%s', '%s', '%s', '%s');",
				 s_bussopid, s_freconn, s_fredis, s_calltime);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_upver
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_upver(t_pcstr s_bussopid, t_pcstr s_curver, t_pcstr s_bkver, t_char c_flag)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_UPDATEVERSION (BUSSOP_ID, CUR_VERSION, BACK_VERSION, SUCC_FLAG)\
 VALUES('%s', '%s', '%s', '%c');",
				 s_bussopid, s_curver, s_bkver, c_flag);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_rollbkver
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_rollbkver(t_pcstr s_bussopid, t_pcstr s_curver, t_char c_flag)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_ROLLBACKVERSION (BUSSOP_ID, CUR_VERSION, SUCC_FLAG)\
 VALUES('%s', '%s', '%c');",
				 s_bussopid, s_curver, c_flag);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_ftp
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_ftp(t_pcstr s_bussopid, t_int i_seq, t_pcstr s_url, double s_usetime, int s_filesize, t_char c_flag)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_FTP (BUSSOP_ID, SEQ, URL, USETIME, FILESIZE, SUCC_FLAG)\
 VALUES('%s', '%d', '%s', '%f', '%d', '%c');",
				 s_bussopid, i_seq, s_url, s_usetime, s_filesize, c_flag);
	return appserver_execute(s_sql.c_str());
}

t_int appserver_dbinsert_bussop_result_fetion(t_pcstr tablename, t_pcstr s_bussopid, t_int i_seq, t_char c_flag, t_pcstr starttime, float delay, t_pcstr value)
{
	string s_sql = FormatString("INSERT INTO %s (BUSSOP_ID, SEQ, STARTTIME, USETIME, SUCC_FLAG, VALUE1) \
VALUES ('%s', '%d', '%s', '%f', '%c', '%s');",
			tablename, 
			s_bussopid, i_seq, starttime, delay, c_flag, value);
	return DB1.db_execute(s_sql, LOG6);
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_ping
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_ping(t_pcstr s_bussopid, t_int i_seq, t_char c_succflag, t_pcstr s_ip, t_pcstr s_packsize, t_pcstr s_tmdelay, t_pcstr s_ttl)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_PING (BUSSOP_ID, SEQ, IP, PACKSIZE, DELAY, TTL, SUCC_FLAG)\
 VALUES('%s', '%d', '%s', '%s', '%s', '%s', '%c');",
				 s_bussopid, i_seq, s_ip, s_packsize, s_tmdelay, s_ttl, c_succflag);
	return appserver_execute(s_sql.c_str());
}

// * ============================================================== *
// @ appserver_dbinsert_bussop_result_http
// * ============================================================== *
t_int appserver_dbinsert_bussop_result_http(t_pcstr s_bussopid, t_int i_seq, t_char c_flag,  t_pcstr s_url, t_pcstr s_usetime, t_pcstr s_filesize, t_char log_type)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_RESULT_HTTP (BUSSOP_ID, SEQ, URL, USETIME, FILESIZE, SUCC_FLAG, LOG_TYPE)\
 VALUES('%s', '%d', '%s', '%s', '%s', '%c', '%c');",
				 s_bussopid, i_seq, s_url, s_usetime, s_filesize, c_flag, log_type);
	return appserver_execute(s_sql.c_str());
}


int get_realtime_bussopid(string & s_bussopid, t_pcstr s_terminal, t_pcstr s_bussoptype, time_t t1)
{
	CNString sql;
	sql.Format("SELECT * from BUSSOP where TERMINAL_ID = '%s' and BUSSOP_TYPE = '%s' and INIVATE_TYPE = 'T' \
and ( STATUS != 'X' and STATUS != 'Y' ) order by STARTTIME desc;", 
		s_terminal, s_bussoptype);
	
	DATASET dbresult;
	int ret = DB1.db_select(sql.c_str(), dbresult, LOG6);
	
	if ( ret > 0 )
	{
		s_bussopid = dbresult[0]["BUSSOP_ID"];
	}
	else if ( ret == 0 )
	{
		LOG(LOG1, "get_realtime_bussopid() found 0 result, s_terminal='%s', s_bussoptype='%s'\n", s_terminal, s_bussoptype);
		
		string s_starttime = time_format_str(t1, 1);
		_ori_bussopid(s_bussopid, 'T', s_terminal, s_bussoptype, s_starttime.c_str());
		
		appserver_dbinsert_bussop(s_bussopid.c_str(), 'T', s_bussoptype, s_terminal, s_starttime.c_str(), "program", 'B');
	}
	
	return 1;
}


int check_bussopid_complete(t_pcstr s_bussopid)
{
	CNString sql;
	sql.Format("SELECT * from BUSSOP where BUSSOP_ID = '%s' and ( STATUS = 'X' or STATUS = 'Y' );", s_bussopid);
	
	DATASET dbresult;
	return DB1.db_select(sql.c_str(), dbresult, LOG6);
}


int select_bussop_by_bussopid(t_pcstr s_bussopid, DATASET & dbresult)
{
	CNString sql;
	sql.Format("SELECT * from BUSSOP where BUSSOP_ID = '%s';", s_bussopid);
	
	return DB1.db_select(sql.c_str(), dbresult, LOG6);
}


int make_terminal_config_ini(t_pcstr s_terminal, t_pcstr s_file)
{
	CNString sql;
	sql.Format("SELECT * from CONFIG_TERMINAL where TERMINAL_ID = '%s';", s_terminal);
	
	DATASET dbresult;
	int ret = DB1.db_select(sql.c_str(), dbresult, LOG6);
	
	if ( ret > 0 )
	{
		string buf;
		buf += "[COMMON]\r\n";
		buf += "PROGRAM=BayardSMS.exe|BayardRoutine.exe|BayardData.exe|HttpTest.exe|BayardConfig.exe|BayardCDMA.exe|BayardPhone.exe\r\n";
		buf += "FILE=BaseTech.dll\r\n";
		buf += "MAINPHONE1=" + dbresult[0]["MAINPHONE1"] + "\r\n";
		buf += "MAINPHONE2=" + dbresult[0]["MAINPHONE2"] + "\r\n";
		buf += "MAINPHONE3=" + dbresult[0]["MAINPHONE3"] + "\r\n";
		buf += "REMSGTIMES=" + dbresult[0]["REMSGTIMES"] + "\r\n";
		buf += "REMSGINTERVAL=" + dbresult[0]["REMSGINTERVAL"] + "\r\n";
		buf += "DTMF=" + dbresult[0]["DTMF"] + "\r\n";
		buf += "DTMF_GAP=" + dbresult[0]["DTMF_GAP"] + "\r\n";
		buf += "DTMF_WAIT1=" + dbresult[0]["DTMF_WAIT1"] + "\r\n";
		buf += "REC_TIME=" + dbresult[0]["REC_TIME"] + "\r\n";
		buf += "Playfile=" + dbresult[0]["PLAYFILE"] + "\r\n";
		buf += "DTMF_WAIT2=" + dbresult[0]["DTMF_WAIT2"] + "\r\n";
		buf += "DTMF_WAIT3=" + dbresult[0]["DTMF_WAIT3"] + "\r\n";
		buf += "DTMF_WAIT4=" + dbresult[0]["DTMF_WAIT4"] + "\r\n";
		buf += "CQT_PERSIST=" + dbresult[0]["CQT_PERSIST"] + "\r\n";
		buf += "REPORTNUM=" + dbresult[0]["PEPORTNUM"] + "\r\n";
		buf += "PESQ_CENTER_ACTIVE=" + dbresult[0]["PESQ_CENTER_ACTIVE"] + "\r\n";
		buf += "PESQ_TERM_ACTIVE=" + dbresult[0]["PESQ_TERM_ACTIVE"] + "\r\n";
		buf += "PESQ_CENTER_PASSIVE=" + dbresult[0]["PESQ_CENTER_PASSIVE"] + "\r\n";
		buf += "PESQ_TERM_PASSIVE=" + dbresult[0]["PESQ_TERM_PASSIVE"] + "\r\n";
		//buf += "CONNPOINT=" + dbresult[0]["CONNPOINT"] + "\r\n";
		buf += "CONNPOINT=Internet 设置\r\n";
		buf += "SMSPrefix=" + dbresult[0]["SMSPrefix"] + "\r\n";
		buf += "BuzzingDuration=" + dbresult[0]["BuzzingDuration"] + "\r\n";
		buf += "N=" + dbresult[0]["CDMA_N"] + "\r\n";
		buf += "NV=" + dbresult[0]["CDMA_NV"] + "\r\n";
		buf += "NUV=" + dbresult[0]["CDMA_NVT"] + "\r\n";
		buf += "NetFailSMSReport=" + dbresult[0]["NetFailSMSReport"] + "\r\n";
		buf += "[REPORT]\r\n";
		buf += "FTPIP=" + dbresult[0]["FTPIP"] + "\r\n";
		buf += "FTPUSER=" + dbresult[0]["FTPUSER"] + "\r\n";
		buf += "FTPPASS=" + dbresult[0]["FTPPASS"] + "\r\n";
		buf += "FTPDIR=" + dbresult[0]["FTPDIR"] + "\r\n";
		buf += "HEARTPHONE=" + dbresult[0]["HEARTPHONE"] + "\r\n";
		buf += "\r\n";
		buf += "\r\n";
		
		write_file(s_file, buf.c_str(), buf.length());
		
		LOG(LOG6, "config file '%s' is:\n%s\n", s_file, buf.c_str());
	}
	
	return ret;
}


int make_terminal_routine_ini(t_pcstr s_terminal, t_pcstr s_file)
{
	CNString sql;
	sql.Format("SELECT * from CONFIG_TERMINAL_ROUTINE where TERMINAL_ID = '%s' order by STARTTIME;", s_terminal);
	
	DATASET dbresult;
	int ret = DB1.db_select(sql.c_str(), dbresult, LOG6);
	
	if ( ret > 0 )
	{
		int linecount = 0;
		
		string buf;
		buf += "[ROUTINE]\r\n";
		buf += "NUM=" + ToString(ret) + "\r\n";
		for ( int i=0; i<dbresult.size(); i++ )
		{
			if ( string_toupper(dbresult[i]["TYPE"]) == "PING" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",PING,33310|" + dbresult[i]["PARA1"] + "|" + dbresult[i]["PARA2"] + "|" + dbresult[i]["PARA3"] + "|" + dbresult[i]["PARA4"] + "|" + dbresult[i]["URL"] + "\r\n";
				if ( dbresult[i]["FLAG_CDMA"] == "T" ) buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",CDMA,44480\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "HTTP" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",HTTP,33330|" + dbresult[i]["PARA1"] + "|" + dbresult[i]["PARA2"] + "|" + dbresult[i]["URL"] + "\r\n";
				if ( dbresult[i]["FLAG_CDMA"] == "T" ) buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",CDMA,44480\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "FTP" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",FTP,33320|" + dbresult[i]["PARA1"] + "|" + dbresult[i]["PARA2"] + "|" + dbresult[i]["URL"] + "|" + dbresult[i]["PARA3"] + "|" + dbresult[i]["PARA4"] + "\r\n";
				if ( dbresult[i]["FLAG_CDMA"] == "T" ) buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",CDMA,44480\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "PESQ" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",PESQ\r\n";
				if ( dbresult[i]["FLAG_CDMA"] == "T" ) buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",CDMA,44480\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "CQT" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",CQT,33360|" + dbresult[i]["PARA1"] + "|" + dbresult[i]["PARA2"] + "|" + dbresult[i]["PARA3"] + "|" + dbresult[i]["PARA4"] + "|" + dbresult[i]["PARA5"] + "|" + dbresult[i]["PARA6"] + "\r\n";
				if ( dbresult[i]["FLAG_CDMA"] == "T" ) buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",CDMA,44480\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "CDMA" || string_toupper(dbresult[i]["TYPE"]) == "CDMAPARA" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",CDMA,44480\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "HEARTBEAT" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",HEARTBEAT\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "RESTART" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",RESTART\r\n";
			}
			else if ( string_toupper(dbresult[i]["TYPE"]) == "UPLOAD" )
			{
				buf += "TEST" + ToString(++linecount) + "=" + dbresult[i]["STARTTIME"] + ",upload\r\n";
			}
			else
			{
				LOG(LOG1, "unknown routine.ini type = '%s'\n", dbresult[i]["TYPE"].c_str());
			}
		}
		
		buf += "TEST" + ToString(++linecount) + "=23:59,RESTART\r\n";
		buf += "TEST" + ToString(++linecount) + "=00:00,RESTART\r\n";
		buf += "TEST" + ToString(++linecount) + "=11:59,RESTART\r\n";
		buf += "TEST" + ToString(++linecount) + "=12:00,RESTART\r\n";
		
		buf += "\r\n";
		buf += "\r\n";
		
		if ( ret != linecount )
		{
			string str1 = "NUM=" + ToString(ret);
			string str2 = "NUM=" + ToString(linecount);
			string_replace(buf, str1, str2);
		}
		
		write_file(s_file, buf.c_str(), buf.length());
		
		LOG(LOG6, "config file '%s' is:\n%s\n", s_file, buf.c_str());
	}
	
	return ret;
}


int make_terminal_cdmapara_txt(t_pcstr s_terminal, t_pcstr s_file)
{
	CNString sql;
	sql.Format("select a.TERMINAL_ID, a.CDMAPARAM_CODE, a.IDLE_HIGH, a.IDLE_LOW, a.ACTIVE_HIGH, a.ACTIVE_LOW, \
a.STATUS, a.IMPORTANT, b.TERMCONF_NAME, b.BUSSOP_RESULT_NAME, b.BUSSOP_RESULT_DBNAME, b.FLAG_IN_LOG \
from CONFIG_TERMINAL_CDMAPARAM a left join CONFIG_CDMAPARAM b \
on a.CDMAPARAM_CODE = b.CDMAPARAM_CODE \
where a.TERMINAL_ID='%s' order by a.CDMAPARAM_CODE;", s_terminal);
	
	DATASET dbresult;
	int ret = DB1.db_select(sql.c_str(), dbresult, LOG6);
	
	if ( ret == 0 )
	{
		sql.Format("select a.TERMINAL_ID, a.CDMAPARAM_CODE, a.IDLE_HIGH, a.IDLE_LOW, a.ACTIVE_HIGH, a.ACTIVE_LOW, \
a.STATUS, a.IMPORTANT, b.TERMCONF_NAME, b.BUSSOP_RESULT_NAME, b.BUSSOP_RESULT_DBNAME, b.FLAG_IN_LOG \
from CONFIG_TERMINAL_CDMAPARAM a left join CONFIG_CDMAPARAM b \
on a.CDMAPARAM_CODE = b.CDMAPARAM_CODE \
where a.TERMINAL_ID='%s' order by a.CDMAPARAM_CODE;", "00001");
		
		ret = DB1.db_select(sql.c_str(), dbresult, LOG6);
	}
	
	if ( ret > 0 )
	{
		string buf;
		buf += "名称			编号		IDLE上阈值	IDLE下阈值	ACTIVE上阈值	ACTIVE下阈值     状态	重要度\r\n";
		for ( int i=0; i<dbresult.size(); i++ )
		{
			if ( dbresult[i]["FLAG_IN_LOG"] == "T" )
			{
				buf += dbresult[i]["TERMCONF_NAME"] + "|			" + dbresult[i]["CDMAPARAM_CODE"] + "|		" + dbresult[i]["IDLE_HIGH"] + "|		" + dbresult[i]["IDLE_LOW"] + "|		" + dbresult[i]["ACTIVE_HIGH"] + "|		" + dbresult[i]["ACTIVE_LOW"] + "|		" + dbresult[i]["STATUS"] + "|	" + dbresult[i]["IMPORTANT"] + "\r\n";
			}
			else
			{
				buf += dbresult[i]["TERMCONF_NAME"] + "			" + dbresult[i]["CDMAPARAM_CODE"] + "		" + dbresult[i]["IDLE_HIGH"] + "		" + dbresult[i]["IDLE_LOW"] + "		" + dbresult[i]["ACTIVE_HIGH"] + "		" + dbresult[i]["ACTIVE_LOW"] + "		" + dbresult[i]["STATUS"] + "	" + dbresult[i]["IMPORTANT"] + "\r\n";
			}
		}
		buf += "\r\n";
		buf += "\r\n";
		
		write_file(s_file, buf.c_str(), buf.length());
		
		LOG(LOG6, "config file '%s' is:\n%s\n", s_file, buf.c_str());
	}
	
	return ret;
}


int update_terminal_heartbeat(t_pcstr s_terminal)
{
	CNString sql;
	sql.Format("update CONFIG_TERMINAL set T_RECV_HEARTBEAT = %u, HEARTBEAT_STATE = 0 where TERMINAL_ID = '%s';", time(NULL), s_terminal);
	
	return DB1.db_execute(sql.c_str(), LOG6);
}


int update_terminal_heartbeat_timeout(t_pcstr s_terminal)
{
	CNString sql;
	sql.Format("update CONFIG_TERMINAL set HEARTBEAT_STATE = 1 where TERMINAL_ID = '%s';", s_terminal);
	
	return DB1.db_execute(sql.c_str(), LOG6);
}


int insert_bussop_exception_result(t_pcstr s_bussopid, t_pcstr s_bussopexecid)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_EXCEPTION_RESULT (BUSSOP_ID, BUSSOPEXEC_ID) VALUES ('%s', '%s');", s_bussopid, s_bussopexecid);
	
	return DB1.db_execute(s_sql.c_str(), LOG6);
}

// * ================================================================================ *
// @ _cdmapara_db_convert
// * ================================================================================ *
/*
CHN|PN|SYSID|NETID|SLOT|RSSI|TotalECIO|MaxECIO|TX_Pwr|TxAdj|FFER|RX_AGC|ECIO|SRCH_WIN_A|SRCH_WIN_N|SRCH_WIN_R|Active|Neighboring|PILOT_INC|T_ADD|T_DROP|T_COMP|T_TDROP|T_COPM_HARD|BSID|Walsh|MACindex|EV_Frequency|Band|EV_RxAGC0|EV_RxAGC1|EV_TxAGC|ServingSectorPN|EV_RxPER|TxOpenLoopPower|TxClosedLoopAdjust|TxPilotPower|TotalSINR|FRAB|DRC_Value|SessionState|ATState|ALMPState|InitState|IdleState|OverheadState|ConnectedState|RouteUpdateState|HDRHybirdModeState|BestSINR|EV_RxRLPtHRPUTiNSTANT|EV_TxRLPtHRPUTINSTANT|Totalc_i|DRC_Index|DRC_Cover|ActiveCount|UATI|EV_Duplicate_Rate|EV_RLP_Error_Rate_Rx|EV_RLP_Error_Rate_Tx|EV_RLP_Rtx_Rate_Rx|EV_RLP_Rtx_Rate_Tx|Ev_Rx_Packet_Throughput|EV_TxPacketThroughput|EV_Tx_PER|Rx_Su_Packet_Throughput|Rx_Mu_Packet_Throughput|Rx_Su_PER|Rx_Mu_PER|
*/
t_pcstr _cdmapara_db_convert(CNString& s_cdma)
{
	if("CHN" == s_cdma)					s_cdma = "CDMAPARA_001";
	else if("PN" == s_cdma)				s_cdma = "CDMAPARA_002";
	else if("SYSID" == s_cdma)			s_cdma = "CDMAPARA_003";
	else if("NETID" == s_cdma)			s_cdma = "CDMAPARA_004";
	else if("SLOT" == s_cdma)			s_cdma = "CDMAPARA_005";
	else if("RSSI" == s_cdma)			s_cdma = "CDMAPARA_006";
	else if("TotalECIO" == s_cdma)		s_cdma = "CDMAPARA_007";
	else if("MaxECIO" == s_cdma)		s_cdma = "CDMAPARA_008";
	else if("TX_Pwr" == s_cdma)			s_cdma = "CDMAPARA_009";

	else if("TxAdj" == s_cdma)			s_cdma = "CDMAPARA_010";
	else if("FFER" == s_cdma)			s_cdma = "CDMAPARA_011";
	else if("RX_AGC" == s_cdma)			s_cdma = "CDMAPARA_012";
	else if("ECIO" == s_cdma)			s_cdma = "CDMAPARA_013";
	else if("SRCH_WIN_A" == s_cdma)		s_cdma = "CDMAPARA_014";
	else if("SRCH_WIN_N" == s_cdma)		s_cdma = "CDMAPARA_015";
	else if("SRCH_WIN_R" == s_cdma)		s_cdma = "CDMAPARA_016";
	else if("Active" == s_cdma)			s_cdma = "CDMAPARA_017";
	else if("Neighboring" == s_cdma)	s_cdma = "CDMAPARA_018";
	else if("PILOT_INC" == s_cdma)		s_cdma = "CDMAPARA_019";

	else if("T_ADD" == s_cdma)				s_cdma = "CDMAPARA_020";
	else if("T_DROP" == s_cdma)				s_cdma = "CDMAPARA_021";
	else if("T_COMP" == s_cdma)				s_cdma = "CDMAPARA_022";
	else if("T_TDROP" == s_cdma)			s_cdma = "CDMAPARA_023";
	else if("T_COPM_HARD" == s_cdma)		s_cdma = "CDMAPARA_024";
	else if("BSID" == s_cdma)				s_cdma = "CDMAPARA_025";
	else if("Walsh" == s_cdma)				s_cdma = "CDMAPARA_026";
	else if("MACindex" == s_cdma)			s_cdma = "CDMAPARA_027";
	else if("EV_Frequency" == s_cdma)		s_cdma = "CDMAPARA_028";
	else if("Band" == s_cdma)				s_cdma = "CDMAPARA_029";

	else if("EV_RxAGC0" == s_cdma)			s_cdma = "CDMAPARA_030";
	else if("EV_RxAGC1" == s_cdma)			s_cdma = "CDMAPARA_031";
	else if("EV_TxAGC" == s_cdma)			s_cdma = "CDMAPARA_032";
	else if("ServingSectorPN" == s_cdma)	s_cdma = "CDMAPARA_033";
	else if("EV_RxPER" == s_cdma)			s_cdma = "CDMAPARA_034";
	else if("TxOpenLoopPower" == s_cdma)	s_cdma = "CDMAPARA_035";
	else if("TxClosedLoopAdjust" == s_cdma)	s_cdma = "CDMAPARA_036";
	else if("TxPilotPower" == s_cdma)		s_cdma = "CDMAPARA_037";
	else if("TotalSINR" == s_cdma)			s_cdma = "CDMAPARA_038";
	else if("FRAB" == s_cdma)				s_cdma = "CDMAPARA_039";

	else if("DRC_Value" == s_cdma)				s_cdma = "CDMAPARA_040";
	else if("SessionState" == s_cdma)			s_cdma = "CDMAPARA_041";
	else if("ATState" == s_cdma)				s_cdma = "CDMAPARA_042";
	else if("ALMPState" == s_cdma)				s_cdma = "CDMAPARA_043";
	else if("InitState" == s_cdma)				s_cdma = "CDMAPARA_044";
	else if("IdleState" == s_cdma)		        s_cdma = "CDMAPARA_045";
	else if("OverheadState" == s_cdma)			s_cdma = "CDMAPARA_046";
	else if("ConnectedState" == s_cdma)			s_cdma = "CDMAPARA_047";
	else if("RouteUpdateState" == s_cdma)		s_cdma = "CDMAPARA_048";
	else if("HDRHybirdModeState" == s_cdma)		s_cdma = "CDMAPARA_049";

	else if("BestSINR" == s_cdma)				s_cdma = "CDMAPARA_050";
	else if("EV_RxRLPtHRPUTiNSTANT" == s_cdma)	s_cdma = "CDMAPARA_051";
	else if("EV_TxRLPtHRPUTINSTANT" == s_cdma)	s_cdma = "CDMAPARA_052";
	else if("Totalc_i" == s_cdma)				s_cdma = "CDMAPARA_053";
	else if("DRC_Index" == s_cdma)				s_cdma = "CDMAPARA_054";
	else if("DRC_Cover" == s_cdma)				s_cdma = "CDMAPARA_055";
	else if("ActiveCount" == s_cdma)			s_cdma = "CDMAPARA_056";
	else if("UATI" == s_cdma)					s_cdma = "CDMAPARA_057";
	else if("EV_Duplicate_Rate" == s_cdma)		s_cdma = "CDMAPARA_058";
	else if("EV_RLP_Error_Rate_Rx" == s_cdma)	s_cdma = "CDMAPARA_059";

	else if("EV_RLP_Error_Rate_Tx" == s_cdma)		s_cdma = "CDMAPARA_060";
	else if("EV_RLP_Rtx_Rate_Rx" == s_cdma)			s_cdma = "CDMAPARA_061";
	else if("EV_RLP_Rtx_Rate_Tx" == s_cdma)			s_cdma = "CDMAPARA_062";
	else if("Ev_Rx_Packet_Throughput" == s_cdma)	s_cdma = "CDMAPARA_063";
	else if("EV_TxPacketThroughput" == s_cdma)		s_cdma = "CDMAPARA_064";
	else if("EV_Tx_PER" == s_cdma)					s_cdma = "CDMAPARA_065";
	else if("Rx_Su_Packet_Throughput" == s_cdma)	s_cdma = "CDMAPARA_066";
	else if("Rx_Mu_Packet_Throughput" == s_cdma)	s_cdma = "CDMAPARA_067";
	else if("Rx_Su_PER" == s_cdma)					s_cdma = "CDMAPARA_068";
	else if("Rx_Mu_PER" == s_cdma)					s_cdma = "CDMAPARA_069";
	
	else
	{
		LOG(LOG1, "_cdmapara_db_convert(): s_cdma='%s' not found\n", s_cdma.c_str());
		return NULL;
	}
	
	return s_cdma.c_str();
}


//需要修正某些CDMA参数的值
//BestSINR-050 Walsh-026 EV_RxRLPtHRPUTiNSTANT-051 EV_TxRLPtHRPUTINSTANT-052
//TotalSINR-038 MACindex-027
int correct_cdma_para(const char * s_bussopid)
{
	CNString sql;
	sql.Format("update BUSSOP_RESULT_CDMAPARAM set CDMAPARA_050 = CDMAPARA_038 \
where BUSSOP_ID = '%s' and CDMAPARA_050 > CDMAPARA_038;", s_bussopid);
	DB1.db_execute(sql.c_str(), LOG6);
	
	
	sql.Format("update BUSSOP_RESULT_CDMAPARAM set CDMAPARA_026 = CDMAPARA_027 * 2 + 1 where BUSSOP_ID = '%s';", 
		s_bussopid);
	DB1.db_execute(sql.c_str(), LOG6);
	
	
	sql.Format("select BUSSOP_ID,SEQ,STATUS,CDMAPARA_051,CDMAPARA_052 \
from BUSSOP_RESULT_CDMAPARAM where BUSSOP_ID = '%s' order by SEQ;", s_bussopid);
	DATASET dbresult;
	int ret = DB1.db_select(sql.c_str(), dbresult, LOG6);
	
	for ( int i=1; i<dbresult.size(); i++ )
	{
		int newvalue051 = ToInt(dbresult[i]["CDMAPARA_051"]) - ToInt(dbresult[i-1]["CDMAPARA_051"]);
		int newvalue052 = ToInt(dbresult[i]["CDMAPARA_052"]) - ToInt(dbresult[i-1]["CDMAPARA_052"]);
		
		if ( newvalue051 < 0 ) newvalue051 = 0;
		if ( newvalue052 < 0 ) newvalue052 = 0;
		
		sql.Format("update BUSSOP_RESULT_CDMAPARAM set CDMAPARA_051 = %d, CDMAPARA_052 = %d \
where BUSSOP_ID = '%s' and SEQ = %d;", newvalue051, newvalue052, s_bussopid, ToInt(dbresult[i]["SEQ"]));
		DB1.db_execute(sql.c_str(), LOG6);
	}
	
	return ret;
}


int insert_db_bussop_sms(const char * s_bussopid, const char * s_terminalid, const char * sms, char flag)
{
	CNString s_sql;
	s_sql.Format("INSERT INTO BUSSOP_SMS (BUSSOP_ID, TERMINAL_ID, SMS, FLAG, CURTIME) \
VALUES ('%s', '%s', '%s', '%c', '%s');", s_bussopid, s_terminalid, sms, flag, time_format_str(time(NULL), 1).c_str());
	
	return DB1.db_execute(s_sql.c_str(), LOG6);
}


int create_db_tables()
{
	/*
	DB1.db_create("CREATE TABLE `BUSSOP` (\n\
  `BUSSOP_ID` varchar(64) NOT NULL DEFAULT '',\n\
  `INIVATE_TYPE` varchar(1) NOT NULL DEFAULT '',\n\
  `BUSSOP_TYPE` varchar(2) NOT NULL DEFAULT '',\n\
  `TERMINAL_ID` varchar(20) NOT NULL DEFAULT '',\n\
  `STARTTIME` varchar(14) NOT NULL DEFAULT '',\n\
  `ENDTIME` varchar(14) NOT NULL DEFAULT '',\n\
  `USER_NAME` varchar(24) NOT NULL DEFAULT '',\n\
  `STATUS` varchar(1) NOT NULL DEFAULT 'A',\n\
  `RES_CODE` int(11) NOT NULL DEFAULT '0',\n\
  `RES_FNAM` varchar(100) NOT NULL,\n\
  PRIMARY KEY (`BUSSOP_ID`)\n\
) ENGINE=MyISAM DEFAULT CHARSET=utf8");
	*/
	
	return 1;
}


int insert_db_default_data()
{
	DATASET dbresult;
	DB1.db_select("select count(*) from CONFIG_BUSSOP_EXCEPTION;", dbresult, LOG6);
	if ( ToInt(dbresult[0][0]) == 0 )
	{
		LOG(LOG1, "insert_db_default_data() for table CONFIG_BUSSOP_EXCEPTION\n");
		
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HEARTBEAT_TMOUT','IA','HEARTBEAT_TMOUT','A','心跳短信超时',0,0,1,'','','','0000-00-00 00:00:00',1,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HEARTBEAT_OK','IB','HEARTBEAT_OK','A','心跳短信恢复',0,0,1,'','','','0000-00-00 00:00:00',1,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('220V_STOP','JA','220V_STOP','A','220V断电',0,0,1,'','','','0000-00-00 00:00:00',1,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('220V_START','JB','220V_START','A','220V供电',0,0,1,'','','','0000-00-00 00:00:00',1,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_FAIL_RATE_EVDO_A','DA','PING_FAIL_RATE_EVDO','A','PING测试丢包率非常大',100,80,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_FAIL_RATE_EVDO_B','DA','PING_FAIL_RATE_EVDO','B','PING测试丢包率很大',79,50,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_FAIL_RATE_EVDO_C','DA','PING_FAIL_RATE_EVDO','C','PING测试丢包率大',49,30,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_SPEED_EVDO_A','DA','PING_SPEED_EVDO','A','PING测试时延非常大',100,80,1,'300','8000','毫秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_SPEED_EVDO_B','DA','PING_SPEED_EVDO','B','PING测试时延很大',100,50,1,'200','300','毫秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_SPEED_EVDO_C','DA','PING_SPEED_EVDO','C','PING测试时延大',100,30,1,'100','200','毫秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_FAIL_RATE_EVDO_A','EA','FTP_FAIL_RATE_EVDO','A','FTP测试失败率非常大',100,80,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_FAIL_RATE_EVDO_B','EA','FTP_FAIL_RATE_EVDO','B','FTP测试失败率很大',79,50,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_FAIL_RATE_EVDO_C','EA','FTP_FAIL_RATE_EVDO','C','FTP测试失败率大',49,30,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_SPEED_EVDO_A','EA','FTP_SPEED_EVDO','A','FTP测试速度非常慢',100,80,1,'0','100','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_SPEED_EVDO_B','EA','FTP_SPEED_EVDO','B','FTP测试速度很慢',100,50,1,'100','500','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_SPEED_EVDO_C','EA','FTP_SPEED_EVDO','C','FTP测试速度慢',100,30,1,'300','600','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_FAIL_RATE_EVDO_A','FA','HTTP_FAIL_RATE_EVDO','A','HTTP测试失败率非常大',100,80,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_FAIL_RATE_EVDO_B','FA','HTTP_FAIL_RATE_EVDO','B','HTTP测试失败率很大',79,50,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_FAIL_RATE_EVDO_C','FA','HTTP_FAIL_RATE_EVDO','C','HTTP测试失败率大',49,30,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_SPEED_EVDO_A','FA','HTTP_SPEED_EVDO','A','HTTP测试速度非常慢',100,80,1,'0','100','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_SPEED_EVDO_B','FA','HTTP_SPEED_EVDO','B','HTTP测试速度很慢',100,50,1,'100','500','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_SPEED_EVDO_C','FA','HTTP_SPEED_EVDO','C','HTTP测试速度慢',100,30,1,'300','600','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PESQ_UP_A','CA','PESQ_UP','A','PESQ测试上行结果非常差',0,0,1,'0.0','2.0','PESQ值','0000-00-00 00:00:00',1,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PESQ_UP_B','CA','PESQ_UP','B','PESQ测试上行结果很差',0,0,1,'2.0','2.5','PESQ值','0000-00-00 00:00:00',1,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PESQ_UP_C','CA','PESQ_UP','C','PESQ测试上行结果差',0,0,1,'2.5','3.0','PESQ值','0000-00-00 00:00:00',1,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PESQ_DOWN_A','CA','PESQ_DOWN','A','PESQ测试下行结果非常差',0,0,1,'0.0','2.0','PESQ值','0000-00-00 00:00:00',1,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PESQ_DOWN_B','CA','PESQ_DOWN','B','PESQ测试下行结果很差',0,0,1,'2.0','2.5','PESQ值','0000-00-00 00:00:00',1,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PESQ_DOWN_C','CA','PESQ_DOWN','C','PESQ测试下行结果差',0,0,1,'2.5','3.0','PESQ值','0000-00-00 00:00:00',1,0,0);"));
		
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_FAIL_RATE_WIFI_A','MA','PING_FAIL_RATE_WIFI','A','PING测试丢包率非常大',100,80,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_FAIL_RATE_WIFI_B','MA','PING_FAIL_RATE_WIFI','B','PING测试丢包率很大',79,50,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_FAIL_RATE_WIFI_C','MA','PING_FAIL_RATE_WIFI','C','PING测试丢包率大',49,30,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_SPEED_WIFI_A','MA','PING_SPEED_WIFI','A','PING测试时延非常大',100,80,1,'300','8000','毫秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_SPEED_WIFI_B','MA','PING_SPEED_WIFI','B','PING测试时延很大',100,50,1,'200','300','毫秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PING_SPEED_WIFI_C','MA','PING_SPEED_WIFI','C','PING测试时延大',100,30,1,'100','200','毫秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_FAIL_RATE_WIFI_A','NA','FTP_FAIL_RATE_WIFI','A','FTP测试失败率非常大',100,80,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_FAIL_RATE_WIFI_B','NA','FTP_FAIL_RATE_WIFI','B','FTP测试失败率很大',79,50,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_FAIL_RATE_WIFI_C','NA','FTP_FAIL_RATE_WIFI','C','FTP测试失败率大',49,30,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_SPEED_WIFI_A','NA','FTP_SPEED_WIFI','A','FTP测试速度非常慢',100,80,1,'0','100','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_SPEED_WIFI_B','NA','FTP_SPEED_WIFI','B','FTP测试速度很慢',100,50,1,'100','500','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FTP_SPEED_WIFI_C','NA','FTP_SPEED_WIFI','C','FTP测试速度慢',100,30,1,'300','600','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_FAIL_RATE_WIFI_A','OA','HTTP_FAIL_RATE_WIFI','A','HTTP测试失败率非常大',100,80,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_FAIL_RATE_WIFI_B','OA','HTTP_FAIL_RATE_WIFI','B','HTTP测试失败率很大',79,50,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_FAIL_RATE_WIFI_C','OA','HTTP_FAIL_RATE_WIFI','C','HTTP测试失败率大',49,30,1,'','','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_SPEED_WIFI_A','OA','HTTP_SPEED_WIFI','A','HTTP测试速度非常慢',100,80,1,'0','100','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_SPEED_WIFI_B','OA','HTTP_SPEED_WIFI','B','HTTP测试速度很慢',100,50,1,'100','500','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HTTP_SPEED_WIFI_C','OA','HTTP_SPEED_WIFI','C','HTTP测试速度慢',100,30,1,'300','600','Byte/秒','0000-00-00 00:00:00',0,0,0);"));
		
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('CHN_A','GA','CHN','A','CDMA参数异常CHN',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PN_A','GA','PN','A','CDMA参数异常PN',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SYSID_A','GA','SYSID','A','CDMA参数异常SYSID',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('NETID_A','GA','NETID','A','CDMA参数异常NETID',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SLOT_A','GA','SLOT','A','CDMA参数异常SLOT',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RSSI_A','GA','RSSI','A','CDMA参数异常RSSI',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TotalECIO_A','GA','TotalECIO','A','CDMA参数异常TotalECIO',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('MaxECIO_A','GA','MaxECIO','A','CDMA参数异常MaxECIO',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TX_Pwr_A','GA','TX_Pwr','A','CDMA参数异常TX_Pwr',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxAdj_A','GA','TxAdj','A','CDMA参数异常TxAdj',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FFER_A','GA','FFER','A','CDMA参数异常FFER',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RX_AGC_A','GA','RX_AGC','A','CDMA参数异常RX_AGC',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ECIO_A','GA','ECIO','A','CDMA参数异常ECIO',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_A_A','GA','SRCH_WIN_A','A','CDMA参数异常SRCH_WIN_A',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_N_A','GA','SRCH_WIN_N','A','CDMA参数异常SRCH_WIN_N',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_R_A','GA','SRCH_WIN_R','A','CDMA参数异常SRCH_WIN_R',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Active_A','GA','Active','A','CDMA参数异常Active',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Neighboring_A','GA','Neighboring','A','CDMA参数异常Neighboring',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PILOT_INC_A','GA','PILOT_INC','A','CDMA参数异常PILOT_INC',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_ADD_A','GA','T_ADD','A','CDMA参数异常T_ADD',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_DROP_A','GA','T_DROP','A','CDMA参数异常T_DROP',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_COMP_A','GA','T_COMP','A','CDMA参数异常T_COMP',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_TDROP_A','GA','T_TDROP','A','CDMA参数异常T_TDROP',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_COPM_HARD_A','GA','T_COPM_HARD','A','CDMA参数异常T_COPM_HARD',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('BSID_A','GA','BSID','A','CDMA参数异常BSID',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Walsh_A','GA','Walsh','A','CDMA参数异常Walsh',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('MACindex_A','GA','MACindex','A','CDMA参数异常MACindex',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Frequency_A','GA','EV_Frequency','A','CDMA参数异常EV_Frequency',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Band_A','GA','Band','A','CDMA参数异常Band',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxAGC0_A','GA','EV_RxAGC0','A','CDMA参数异常EV_RxAGC0',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxAGC1_A','GA','EV_RxAGC1','A','CDMA参数异常EV_RxAGC1',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxAGC_A','GA','EV_TxAGC','A','CDMA参数异常EV_TxAGC',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ServingSectorPN_A','GA','ServingSectorPN','A','CDMA参数异常ServingSectorPN',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxPER_A','GA','EV_RxPER','A','CDMA参数异常EV_RxPER',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxOpenLoopPower_A','GA','TxOpenLoopPower','A','CDMA参数异常TxOpenLoopPower',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxClosedLoopAdjust_A','GA','TxClosedLoopAdjust','A','CDMA参数异常TxClosedLoopAdjust',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxPilotPower_A','GA','TxPilotPower','A','CDMA参数异常TxPilotPower',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TotalSINR_A','GA','TotalSINR','A','CDMA参数异常TotalSINR',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FRAB_A','GA','FRAB','A','CDMA参数异常FRAB',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Value_A','GA','DRC_Value','A','CDMA参数异常DRC_Value',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SessionState_A','GA','SessionState','A','CDMA参数异常SessionState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ATState_A','GA','ATState','A','CDMA参数异常ATState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ALMPState_A','GA','ALMPState','A','CDMA参数异常ALMPState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('InitState_A','GA','InitState','A','CDMA参数异常InitState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('IdleState_A','GA','IdleState','A','CDMA参数异常IdleState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('OverheadState_A','GA','OverheadState','A','CDMA参数异常OverheadState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ConnectedState_A','GA','ConnectedState','A','CDMA参数异常ConnectedState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RouteUpdateState_A','GA','RouteUpdateState','A','CDMA参数异常RouteUpdateState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HDRHybirdModeState_A','GA','HDRHybirdModeState','A','CDMA参数异常HDRHybirdModeState',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('BestSINR_A','GA','BestSINR','A','CDMA参数异常BestSINR',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxRLPtHRPUTiNSTANT_A','GA','EV_RxRLPtHRPUTiNSTANT','A','CDMA参数异常EV_RxRLPtHRPUTiNSTANT',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxRLPtHRPUTINSTANT_A','GA','EV_TxRLPtHRPUTINSTANT','A','CDMA参数异常EV_TxRLPtHRPUTINSTANT',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Totalc_i_A','GA','Totalc_i','A','CDMA参数异常Totalc_i',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Index_A','GA','DRC_Index','A','CDMA参数异常DRC_Index',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Cover_A','GA','DRC_Cover','A','CDMA参数异常DRC_Cover',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ActiveCount_A','GA','ActiveCount','A','CDMA参数异常ActiveCount',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('UATI_A','GA','UATI','A','CDMA参数异常UATI',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Duplicate_Rate_A','GA','EV_Duplicate_Rate','A','CDMA参数异常EV_Duplicate_Rate',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Error_Rate_Rx_A','GA','EV_RLP_Error_Rate_Rx','A','CDMA参数异常EV_RLP_Error_Rate_Rx',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Error_Rate_Tx_A','GA','EV_RLP_Error_Rate_Tx','A','CDMA参数异常EV_RLP_Error_Rate_Tx',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Rtx_Rate_Rx_A','GA','EV_RLP_Rtx_Rate_Rx','A','CDMA参数异常EV_RLP_Rtx_Rate_Rx',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Rtx_Rate_Tx_A','GA','EV_RLP_Rtx_Rate_Tx','A','CDMA参数异常EV_RLP_Rtx_Rate_Tx',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Ev_Rx_Packet_Throughput_A','GA','Ev_Rx_Packet_Throughput','A','CDMA参数异常Ev_Rx_Packet_Throughput',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxPacketThroughput_A','GA','EV_TxPacketThroughput','A','CDMA参数异常EV_TxPacketThroughput',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Tx_PER_A','GA','EV_Tx_PER','A','CDMA参数异常EV_Tx_PER',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Su_Packet_Throughput_A','GA','Rx_Su_Packet_Throughput','A','CDMA参数异常Rx_Su_Packet_Throughput',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Mu_Packet_Throughput_A','GA','Rx_Mu_Packet_Throughput','A','CDMA参数异常Rx_Mu_Packet_Throughput',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Su_PER_A','GA','Rx_Su_PER','A','CDMA参数异常Rx_Su_PER',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Mu_PER_A','GA','Rx_Mu_PER','A','CDMA参数异常Rx_Mu_PER',50,100,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('CHN_B','GA','CHN','B','CDMA参数异常CHN',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PN_B','GA','PN','B','CDMA参数异常PN',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SYSID_B','GA','SYSID','B','CDMA参数异常SYSID',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('NETID_B','GA','NETID','B','CDMA参数异常NETID',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SLOT_B','GA','SLOT','B','CDMA参数异常SLOT',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RSSI_B','GA','RSSI','B','CDMA参数异常RSSI',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TotalECIO_B','GA','TotalECIO','B','CDMA参数异常TotalECIO',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('MaxECIO_B','GA','MaxECIO','B','CDMA参数异常MaxECIO',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TX_Pwr_B','GA','TX_Pwr','B','CDMA参数异常TX_Pwr',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxAdj_B','GA','TxAdj','B','CDMA参数异常TxAdj',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FFER_B','GA','FFER','B','CDMA参数异常FFER',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RX_AGC_B','GA','RX_AGC','B','CDMA参数异常RX_AGC',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ECIO_B','GA','ECIO','B','CDMA参数异常ECIO',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_A_B','GA','SRCH_WIN_A','B','CDMA参数异常SRCH_WIN_A',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_N_B','GA','SRCH_WIN_N','B','CDMA参数异常SRCH_WIN_N',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_R_B','GA','SRCH_WIN_R','B','CDMA参数异常SRCH_WIN_R',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Active_A_B','GA','Active','B','CDMA参数异常Active',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Neighboring_B','GA','Neighboring','B','CDMA参数异常Neighboring',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PILOT_INC_B','GA','PILOT_INC','B','CDMA参数异常PILOT_INC',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_ADD_B','GA','T_ADD','B','CDMA参数异常T_ADD',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_DROP_B','GA','T_DROP','B','CDMA参数异常T_DROP',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_COMP_B','GA','T_COMP','B','CDMA参数异常T_COMP',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_TDROP_B','GA','T_TDROP','B','CDMA参数异常T_TDROP',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_COPM_HARD_B','GA','T_COPM_HARD','B','CDMA参数异常T_COPM_HARD',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('BSID_B','GA','BSID','B','CDMA参数异常BSID',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Walsh_B','GA','Walsh','B','CDMA参数异常Walsh',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('MACindex_B','GA','MACindex','B','CDMA参数异常MACindex',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Frequency_B','GA','EV_Frequency','B','CDMA参数异常EV_Frequency',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Band_B','GA','Band','B','CDMA参数异常Band',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxAGC0_B','GA','EV_RxAGC0','B','CDMA参数异常EV_RxAGC0',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxAGC1_B','GA','EV_RxAGC1','B','CDMA参数异常EV_RxAGC1',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxAGC_B','GA','EV_TxAGC','B','CDMA参数异常EV_TxAGC',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ServingSectorPN_B','GA','ServingSectorPN','B','CDMA参数异常ServingSectorPN',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxPER_B','GA','EV_RxPER','B','CDMA参数异常EV_RxPER',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxOpenLoopPower_B','GA','TxOpenLoopPower','B','CDMA参数异常TxOpenLoopPower',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxClosedLoopAdjust_B','GA','TxClosedLoopAdjust','B','CDMA参数异常TxClosedLoopAdjust',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxPilotPower_B','GA','TxPilotPower','B','CDMA参数异常TxPilotPower',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TotalSINR_B','GA','TotalSINR','B','CDMA参数异常TotalSINR',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FRAB_B','GA','FRAB','B','CDMA参数异常FRAB',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Value_B','GA','DRC_Value','B','CDMA参数异常DRC_Value',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SessionState_B','GA','SessionState','B','CDMA参数异常SessionState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ATState_B','GA','ATState','B','CDMA参数异常ATState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ALMPState_B','GA','ALMPState','B','CDMA参数异常ALMPState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('InitState_B','GA','InitState','B','CDMA参数异常InitState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('IdleState_B','GA','IdleState','B','CDMA参数异常IdleState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('OverheadState_B','GA','OverheadState','B','CDMA参数异常OverheadState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ConnectedState_B','GA','ConnectedState','B','CDMA参数异常ConnectedState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RouteUpdateState_B','GA','RouteUpdateState','B','CDMA参数异常RouteUpdateState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HDRHybirdModeState_B','GA','HDRHybirdModeState','B','CDMA参数异常HDRHybirdModeState',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('BestSINR_B','GA','BestSINR','B','CDMA参数异常BestSINR',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxRLPtHRPUTiNSTANT_B','GA','EV_RxRLPtHRPUTiNSTANT','B','CDMA参数异常EV_RxRLPtHRPUTiNSTANT',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxRLPtHRPUTINSTANT_B','GA','EV_TxRLPtHRPUTINSTANT','B','CDMA参数异常EV_TxRLPtHRPUTINSTANT',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Totalc_i_B','GA','Totalc_i','B','CDMA参数异常Totalc_i',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Index_B','GA','DRC_Index','B','CDMA参数异常DRC_Index',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Cover_B','GA','DRC_Cover','B','CDMA参数异常DRC_Cover',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ActiveCount_B','GA','ActiveCount','B','CDMA参数异常ActiveCount',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('UATI_B','GA','UATI','B','CDMA参数异常UATI',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Duplicate_Rate_B','GA','EV_Duplicate_Rate','B','CDMA参数异常EV_Duplicate_Rate',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Error_Rate_Rx_B','GA','EV_RLP_Error_Rate_Rx','B','CDMA参数异常EV_RLP_Error_Rate_Rx',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Error_Rate_Tx_B','GA','EV_RLP_Error_Rate_Tx','B','CDMA参数异常EV_RLP_Error_Rate_Tx',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Rtx_Rate_Rx_B','GA','EV_RLP_Rtx_Rate_Rx','B','CDMA参数异常EV_RLP_Rtx_Rate_Rx',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Rtx_Rate_Tx_B','GA','EV_RLP_Rtx_Rate_Tx','B','CDMA参数异常EV_RLP_Rtx_Rate_Tx',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Ev_Rx_Packet_Throughput_B','GA','Ev_Rx_Packet_Throughput','B','CDMA参数异常Ev_Rx_Packet_Throughput',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxPacketThroughput_B','GA','EV_TxPacketThroughput','B','CDMA参数异常EV_TxPacketThroughput',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Tx_PER_B','GA','EV_Tx_PER','B','CDMA参数异常EV_Tx_PER',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Su_Packet_Throughput_B','GA','Rx_Su_Packet_Throughput','B','CDMA参数异常Rx_Su_Packet_Throughput',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Mu_Packet_Throughput_B','GA','Rx_Mu_Packet_Throughput','B','CDMA参数异常Rx_Mu_Packet_Throughput',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Su_PER_B','GA','Rx_Su_PER','B','CDMA参数异常Rx_Su_PER',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Mu_PER_B','GA','Rx_Mu_PER','B','CDMA参数异常Rx_Mu_PER',30,50,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('CHN_C','GA','CHN','C','CDMA参数异常CHN',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PN_C','GA','PN','C','CDMA参数异常PN',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SYSID_C','GA','SYSID','C','CDMA参数异常SYSID',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('NETID_C','GA','NETID','C','CDMA参数异常NETID',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SLOT_C','GA','SLOT','C','CDMA参数异常SLOT',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RSSI_C','GA','RSSI','C','CDMA参数异常RSSI',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TotalECIO_C','GA','TotalECIO','C','CDMA参数异常TotalECIO',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('MaxECIO_C','GA','MaxECIO','C','CDMA参数异常MaxECIO',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TX_Pwr_C','GA','TX_Pwr','C','CDMA参数异常TX_Pwr',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxAdj_C','GA','TxAdj','C','CDMA参数异常TxAdj',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FFER_C','GA','FFER','C','CDMA参数异常FFER',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RX_AGC_C','GA','RX_AGC','C','CDMA参数异常RX_AGC',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ECIO_C','GA','ECIO','C','CDMA参数异常ECIO',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_A_C','GA','SRCH_WIN_A','C','CDMA参数异常SRCH_WIN_A',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_N_C','GA','SRCH_WIN_N','C','CDMA参数异常SRCH_WIN_N',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SRCH_WIN_R_C','GA','SRCH_WIN_R','C','CDMA参数异常SRCH_WIN_R',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Active_A_C','GA','Active','C','CDMA参数异常Active',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Neighboring_C','GA','Neighboring','C','CDMA参数异常Neighboring',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('PILOT_INC_C','GA','PILOT_INC','C','CDMA参数异常PILOT_INC',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_ADD_C','GA','T_ADD','C','CDMA参数异常T_ADD',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_DROP_C','GA','T_DROP','C','CDMA参数异常T_DROP',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_COMP_C','GA','T_COMP','C','CDMA参数异常T_COMP',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_TDROP_C','GA','T_TDROP','C','CDMA参数异常T_TDROP',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('T_COPM_HARD_C','GA','T_COPM_HARD','C','CDMA参数异常T_COPM_HARD',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('BSID_C','GA','BSID','C','CDMA参数异常BSID',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Walsh_C','GA','Walsh','C','CDMA参数异常Walsh',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('MACindex_C','GA','MACindex','C','CDMA参数异常MACindex',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Frequency_C','GA','EV_Frequency','C','CDMA参数异常EV_Frequency',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Band_C','GA','Band','C','CDMA参数异常Band',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxAGC0_C','GA','EV_RxAGC0','C','CDMA参数异常EV_RxAGC0',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxAGC1_C','GA','EV_RxAGC1','C','CDMA参数异常EV_RxAGC1',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxAGC_C','GA','EV_TxAGC','C','CDMA参数异常EV_TxAGC',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ServingSectorPN_C','GA','ServingSectorPN','C','CDMA参数异常ServingSectorPN',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxPER_C','GA','EV_RxPER','C','CDMA参数异常EV_RxPER',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxOpenLoopPower_C','GA','TxOpenLoopPower','C','CDMA参数异常TxOpenLoopPower',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxClosedLoopAdjust_C','GA','TxClosedLoopAdjust','C','CDMA参数异常TxClosedLoopAdjust',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TxPilotPower_C','GA','TxPilotPower','C','CDMA参数异常TxPilotPower',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('TotalSINR_C','GA','TotalSINR','C','CDMA参数异常TotalSINR',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('FRAB_C','GA','FRAB','C','CDMA参数异常FRAB',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Value_C','GA','DRC_Value','C','CDMA参数异常DRC_Value',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('SessionState_C','GA','SessionState','C','CDMA参数异常SessionState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ATState_C','GA','ATState','C','CDMA参数异常ATState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ALMPState_C','GA','ALMPState','C','CDMA参数异常ALMPState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('InitState_C','GA','InitState','C','CDMA参数异常InitState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('IdleState_C','GA','IdleState','C','CDMA参数异常IdleState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('OverheadState_C','GA','OverheadState','C','CDMA参数异常OverheadState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ConnectedState_C','GA','ConnectedState','C','CDMA参数异常ConnectedState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('RouteUpdateState_C','GA','RouteUpdateState','C','CDMA参数异常RouteUpdateState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('HDRHybirdModeState_C','GA','HDRHybirdModeState','C','CDMA参数异常HDRHybirdModeState',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('BestSINR_C','GA','BestSINR','C','CDMA参数异常BestSINR',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RxRLPtHRPUTiNSTANT_C','GA','EV_RxRLPtHRPUTiNSTANT','C','CDMA参数异常EV_RxRLPtHRPUTiNSTANT',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxRLPtHRPUTINSTANT_C','GA','EV_TxRLPtHRPUTINSTANT','C','CDMA参数异常EV_TxRLPtHRPUTINSTANT',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Totalc_i_C','GA','Totalc_i','C','CDMA参数异常Totalc_i',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Index_C','GA','DRC_Index','C','CDMA参数异常DRC_Index',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('DRC_Cover_C','GA','DRC_Cover','C','CDMA参数异常DRC_Cover',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('ActiveCount_C','GA','ActiveCount','C','CDMA参数异常ActiveCount',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('UATI_C','GA','UATI','C','CDMA参数异常UATI',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Duplicate_Rate_C','GA','EV_Duplicate_Rate','C','CDMA参数异常EV_Duplicate_Rate',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Error_Rate_Rx_C','GA','EV_RLP_Error_Rate_Rx','C','CDMA参数异常EV_RLP_Error_Rate_Rx',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Error_Rate_Tx_C','GA','EV_RLP_Error_Rate_Tx','C','CDMA参数异常EV_RLP_Error_Rate_Tx',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Rtx_Rate_Rx_C','GA','EV_RLP_Rtx_Rate_Rx','C','CDMA参数异常EV_RLP_Rtx_Rate_Rx',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_RLP_Rtx_Rate_Tx_C','GA','EV_RLP_Rtx_Rate_Tx','C','CDMA参数异常EV_RLP_Rtx_Rate_Tx',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Ev_Rx_Packet_Throughput_C','GA','Ev_Rx_Packet_Throughput','C','CDMA参数异常Ev_Rx_Packet_Throughput',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_TxPacketThroughput_C','GA','EV_TxPacketThroughput','C','CDMA参数异常EV_TxPacketThroughput',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('EV_Tx_PER_C','GA','EV_Tx_PER','C','CDMA参数异常EV_Tx_PER',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Su_Packet_Throughput_C','GA','Rx_Su_Packet_Throughput','C','CDMA参数异常Rx_Su_Packet_Throughput',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Mu_Packet_Throughput_C','GA','Rx_Mu_Packet_Throughput','C','CDMA参数异常Rx_Mu_Packet_Throughput',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Su_PER_C','GA','Rx_Su_PER','C','CDMA参数异常Rx_Su_PER',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
		DB1.db_execute(("INSERT INTO `CONFIG_BUSSOP_EXCEPTION` (`BUSSOPEXEC_ID`, `BUSSOP_TYPE`, `BUSSOPEXEC_TYPE`, `BUSSOPEXEC_LEVEL`, `DES`, `TIMES_MAX`, `TIMES_MIN`, `TIMES`, `MINVALUE`, `MAXVALUE`, `DISPLAY`, `RECALC_TIME`, `READ_ONLY_1`, `READ_ONLY_2`, `DISABLE`) VALUES ('Rx_Mu_PER_C','GA','Rx_Mu_PER','C','CDMA参数异常Rx_Mu_PER',20,30,20,'NULL','NULL','','0000-00-00 00:00:00',0,1,0);"));
	}
	
	return 1;
}

t_int appserver_dbinsert_report_stat(t_int i_count, t_pcstr starttime, t_pcstr endtime, t_pcstr model, t_pcstr address, t_pcstr operater, t_pcstr terminalid, t_pcstr s_bussopid)
{
	string s_sql = FormatString("INSERT INTO REPORT_STAT (COUNT, STARTTIME, ENDTIME, MODEL, ADDRESS, OPERATER, TERMINAL_ID, BUSSOP_ID) \
VALUES ('%d', '%s', '%s', '%s', '%s', '%s', '%s', '%s');",
			i_count, starttime, endtime, model, address, operater, terminalid, s_bussopid);
	DB1.db_execute(s_sql, LOG6);
	
	return DB1.db_insert_id();
}


t_int appserver_dbinsert_report_stat_detail(t_int report_id, t_pcstr terminalid, t_pcstr kqi_id, t_pcstr kqi_name, int i_seq, t_pcstr starttime, t_pcstr endtime, t_pcstr value)
{
	string s_sql = FormatString("INSERT INTO REPORT_STAT_DETAIL (REPORT_ID, TERMINAL_ID, KQI_ID, KQI_NAME, SEQ, STARTTIME, ENDTIME, VALUE1) \
VALUES ('%d', '%s', '%s', '%s', '%d', '%s', '%s', '%s');",
			report_id, terminalid, kqi_id, kqi_name, i_seq, starttime, endtime, value);
	return DB1.db_execute(s_sql, LOG6);
}


t_int appserver_dbinsert_error_log(t_pcstr filename, t_pcstr reason)
{
	string s_sql = FormatString("INSERT INTO UPLOAD_LOGS (FILENAME, DESCRIPTION, UPLOAD_TIME) VALUES ('%s', '%s', '%s');",
			filename, reason, time_format_str(time(NULL)).c_str());
	return DB1.db_execute(s_sql, LOG6);
}


string db_search_bussopid_by_tid(const string & tid)
{
	CNString sql;
	sql.Format("SELECT * from BUSSOP where TID = '%s' and ( STATUS != 'X' and STATUS != 'Y' ) order by STARTTIME desc;", 
			tid.c_str());
	
	DATASET dbresult;
	int ret = DB1.db_select(sql.c_str(), dbresult, LOG6);
	
	if ( ret > 0 )
	{
		return dbresult[0]["BUSSOP_ID"];
	}
	
	return "";
}

