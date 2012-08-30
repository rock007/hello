#ifndef APPSERVER_DB_HEAD_DEFINE_
#define APPSERVER_DB_HEAD_DEFINE_

#include "../common/n_type.h"
#include "../common/n_string.h"

#include "logfile.h"
#include "db_func_mysql.h"

#ifdef WIN32
struct CSQLAPIDBSelectResult
{
	int i_lines, i_rows;
	char **p_res;
};


#include "../common/n_string.h"

#define WIN32_DB_WRITEFNAME	"c:/nzq_working/db.log"
class CSQLAPIDBConnect
{
public:
	t_int connect_db(t_pcstr dbname, t_pcstr servername, t_pcstr usrname, t_pcstr usrpwd)
	{
		CNString s_tr;
		time_t curtm = time(NULL);
		struct tm* p_tm = localtime(&curtm);
		s_tr.Format("%04d-%02d-%02d %02d:%02d:%02d opendb() dbname=%s servername=%s usrname=%s usrpwd=%s\r\n",
			p_tm->tm_year+1900, p_tm->tm_mon+1, p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec,
			dbname, servername, usrname, usrpwd);
		s_tr.writefile_append(WIN32_DB_WRITEFNAME);
		return 0;
	}
	t_int close_db()
	{
		CNString s_tr;
		time_t curtm = time(NULL);
		struct tm* p_tm = localtime(&curtm);
		s_tr.Format("%04d-%02d-%02d %02d:%02d:%02d close db\r\n",
			p_tm->tm_year+1900, p_tm->tm_mon+1, p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
		s_tr.writefile_append(WIN32_DB_WRITEFNAME);
		return 0;
	}

	t_int db_execute(t_pcstr s_sql)
	{
		CNString s_tr;
		time_t curtm = time(NULL);
		struct tm* p_tm = localtime(&curtm);
		s_tr.Format("%s\r\n", s_sql);
		s_tr.writefile_append(WIN32_DB_WRITEFNAME);
		return 0;
	}

public:
	t_int db_select_start(t_pcstr s_sql, CSQLAPIDBSelectResult& sqlresult) { return 0; }
	char** db_select_fetch(CSQLAPIDBSelectResult& sqlresult) { return NULL;}
	t_int db_select_end(CSQLAPIDBSelectResult& sqlresult)		{ return 0;}

	t_int db_select_getlines(CSQLAPIDBSelectResult& sqlresult)	{ return 0; }
	t_int db_select_getrows(CSQLAPIDBSelectResult& sqlresult)	{ return 0; }

};
#else
#include "mysql.h"
struct CSQLAPIDBSelectResult
{
	MYSQL_RES	*p_res;
};
class CSQLAPIDBConnect
{
public:
	t_int connect_db(t_pcstr dbname, t_pcstr servername, t_pcstr usrname, t_pcstr usrpwd);
	t_int close_db();

	t_int db_execute(t_pcstr s_sql);

public:
	t_int db_select_start(t_pcstr s_sql, CSQLAPIDBSelectResult& sqlresult);
	char** db_select_fetch(CSQLAPIDBSelectResult& sqlresult);
	t_int db_select_end(CSQLAPIDBSelectResult& sqlresult)		{ mysql_free_result(sqlresult.p_res); return 0;}

	t_int db_select_getlines(CSQLAPIDBSelectResult& sqlresult)	{ return (int)mysql_num_rows(sqlresult.p_res); }
	t_int db_select_getrows(CSQLAPIDBSelectResult& sqlresult)	{ return (int)mysql_num_fields(sqlresult.p_res); }

protected:
	MYSQL 		m_pConnection;
};
#endif


#define BUSS_UPVER		'A'
#define BUSS_CQT		'B'
#define BUSS_PESQ		'C'
#define BUSS_PING		'D'
#define BUSS_FTP		'E'
#define BUSS_HTTP		'F'
#define BUSS_CDMA		'G'
#define BUSS_CONFTERM	'H'
#define BUSS_HEARTBEAT	'I'
#define BUSS_220V		'J'
#define BUSS_FENGMING	'K'
#define BUSS_RESTART	'L'
#define BUSS_PING_WIFI	'M'
#define BUSS_FTP_WIFI	'N'
#define BUSS_HTTP_WIFI	'O'
#define BUSS_FTP_AGAIN	'P'

#define BUSSOPTYPE_REALTIME	'T'
#define BUSSOPTYPE_ROUTINE	'R'

// ===========================================================
extern int appserver_opendb(t_pcstr, t_pcstr, t_pcstr, t_pcstr);
extern int appserver_closedb();
extern int appserver_execute(t_pcstr s_sql);
extern int appserver_select_start(t_pcstr s_sql, CSQLAPIDBSelectResult& result);
extern char** appserver_select_fetch(CSQLAPIDBSelectResult& result);
extern int appserver_select_end(CSQLAPIDBSelectResult& result);
// ===========================================================

// ===========================================================
// 插入业务操作记录
t_int appserver_dbinsert_bussop(t_pcstr s_bussopid, t_char c_inivatetype, t_pcstr s_bussoptype, t_pcstr s_terminal, t_pcstr s_starttime, t_pcstr s_user, t_char c_status);
extern t_int appserver_dbupdate_bussop_error(t_pcstr s_bussopid, t_int i_err);
extern t_int appserver_dbupdate_bussop_success(t_pcstr s_bussopid);
extern t_int appserver_dbupdate_bussop_status(t_pcstr s_bussopid, char c_status, t_int i_rescode);
extern t_int appserver_dbupdate_bussop_resfname(t_pcstr s_bussopid, t_pcstr s_resfname);
// ===========================================================

// ===========================================================
// 插入业务操作内容
extern t_int appserver_dbinsert_bussop_content_cqt(t_pcstr s_bussopid, t_char c_type, t_pcstr s_called, t_pcstr s_tminter, t_pcstr s_times, t_pcstr s_test_tminter, t_pcstr s_test_times, t_pcstr s_tm_callhold);
extern t_int appserver_dbinsert_bussop_content_ping(t_pcstr s_bussopid, t_pcstr s_times, t_pcstr s_packsize, t_pcstr s_tminter, t_pcstr s_tmout, t_pcstr s_url);
extern t_int appserver_dbinsert_bussop_content_ftp(t_pcstr s_bussopid, t_pcstr s_url, t_pcstr s_times, t_pcstr s_tmout, t_pcstr s_direction, t_pcstr s_numprocess);
extern t_int appserver_dbinsert_bussop_content_http(t_pcstr s_bussopid, t_pcstr s_url, t_pcstr s_times, t_pcstr s_tmout);
// ===========================================================

// ===========================================================
// 插入业务操作结果
extern t_int appserver_dbinsert_bussop_result_cqt(t_pcstr s_bussopid, t_pcstr s_freconn, t_pcstr s_fredis, t_pcstr s_calltime);
extern t_int appserver_dbinsert_bussop_result_cqt(t_pcstr s_bussopid, t_int i_conn_frequency, t_int i_lost_frequency, t_int i_time_avg);
extern t_int appserver_dbinsert_bussop_result_queryver(t_pcstr s_bussopid, t_pcstr s_curver, t_pcstr s_bkver);
extern t_int appserver_dbinsert_bussop_result_upver(t_pcstr s_bussopid, t_pcstr s_curver, t_pcstr s_bkver, t_char c_flag);
extern t_int appserver_dbinsert_bussop_result_rollbkver(t_pcstr s_bussopid, t_pcstr s_curver, t_char c_flag);
extern t_int appserver_dbinsert_bussop_result_ftp(t_pcstr s_bussopid, t_int i_seq, t_pcstr s_url, double s_usetime, int s_filesize, t_char c_flag);
extern t_int appserver_dbinsert_bussop_result_ping(t_pcstr s_bussopid, t_int i_seq, t_char c_succflag, t_pcstr s_ip, t_pcstr s_packsize, t_pcstr s_tmdelay, t_pcstr s_ttl);
extern t_int appserver_dbinsert_bussop_result_http(t_pcstr s_bussopid, t_int i_seq, t_char c_flag,  t_pcstr s_url, t_pcstr s_usetime, t_pcstr s_filesize, t_char log_type);
// ===========================================================

extern int get_realtime_bussopid(string & s_bussopid, t_pcstr s_terminal, t_pcstr s_bussoptype, time_t t1);

extern int check_bussopid_complete(t_pcstr s_bussopid);

extern t_void _ori_bussopid(string& s_bussopid, t_char c_type, t_pcstr s_terminal, t_pcstr s_bussoptype, t_pcstr s_starttime);

extern int make_terminal_config_ini(t_pcstr s_terminal, t_pcstr s_file);
extern int make_terminal_routine_ini(t_pcstr s_terminal, t_pcstr s_file);
extern int make_terminal_cdmapara_txt(t_pcstr s_terminal, t_pcstr s_file);

extern int update_terminal_heartbeat(t_pcstr s_terminal);
extern int update_terminal_heartbeat_timeout(t_pcstr s_terminal);

extern int insert_bussop_exception_result(t_pcstr s_bussopid, t_pcstr s_bussopexecid);

extern t_pcstr _cdmapara_db_convert(CNString& s_cdma);

extern int correct_cdma_para(const char * s_bussopid);

extern int insert_db_bussop_sms(const char * s_bussopid, const char * s_terminalid, const char * sms, char flag);

extern int select_bussop_by_bussopid(t_pcstr s_bussopid, DATASET & dbresult);

extern int create_db_tables();
extern int insert_db_default_data();

#endif
