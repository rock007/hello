
#include "StdAfx.h"
#include "appserver.h"
#include "appserver_bussop.h"
#include <list>
#include <set>
using namespace std;

#define LOG CCLogFile.logwrite
extern LogFile CCLogFile;

extern MYSQLDB DB1;

list<sms_66610_t> sms66610;

t_pvoid thread_procdata_sendsmsg(t_pvoid p_param);


//return 1 -- 多条消息收全了，可以处理了
//return 0 -- //消息不全，等待
int check_66610_complete(const string & phone, int total_count)
{
	set<int> set1;
	for ( int i=1; i<=total_count; i++ ) set1.insert(i);
	
	set<int> set2;
	for ( list<sms_66610_t>::iterator plist = sms66610.begin(); plist != sms66610.end(); ++plist )
	{
		if ( plist->phone == phone )
		{
			ConfigLine line1;
			line1.readline(plist->content, "|");
			
			set2.insert( ToInt(line1[1]) );
		}
	}
	
	if ( !(set1 == set2) )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}


int deal_sms_66610(const string & s_terminal)
{
	string s_bussopid;
	string s_starttime = time_format_str(time(NULL), 1);
	_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "GA", s_starttime.c_str());
	
	CNString sql;
	sql.Format("INSERT INTO BUSSOP_RESULT_CDMAPARAM (BUSSOP_ID, SEQ, STATUS) VALUES ('%s', %d, '%s');", s_bussopid.c_str(), 1, "");
	DB1.db_execute(sql.c_str(), LOG6);
	
	for ( list<sms_66610_t>::iterator plist = sms66610.begin(); plist != sms66610.end(); )
	{
		LOG(LOG6, "list [%s] [%s]\n", plist->phone.c_str(), plist->content.c_str());
		
		if ( plist->phone == s_terminal )
		{
			ConfigLine line1;
			line1.readline(plist->content, "|");
			
			string sql = "UPDATE BUSSOP_RESULT_CDMAPARAM SET ";
			for ( int jj=0; jj<(line1.size()-3)/2; jj++ )
			{
				if ( jj != 0 )
				{
					sql += "', ";
				}
				string fieldname = "CDMAPARA_" + line1[3+jj*2];
				sql += fieldname.c_str();
				sql += "='";
				sql += line1[3+jj*2+1].c_str();
			}
			sql += "' WHERE BUSSOP_ID = '";
			sql += s_bussopid.c_str();
			sql += "';";
			DB1.db_execute(sql.c_str(), LOG6);
			
			plist = sms66610.erase(plist);
			continue;
		}
		else
		{
			++plist;
		}
	}
	
	appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "GA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
	
	insert_bussop_exception_result(s_bussopid.c_str(), "66610_report_cdma");
	
	LOG(LOG6, "sms66610.size() = %d\n", sms66610.size());
	
	return 1;
}


// * ================================================================================ *
// @ _shortmsg_create_inter
// * ================================================================================ *
const CNString& _shortmsg_create_inter(CNString& s_shortmsg, t_pcstr s_terminal, t_pcstr s_content, t_pcstr s_bussopid)
{
	s_shortmsg.Format("<num>%s</num><msg>%s</msg><bussopid>%s</bussopid>", s_terminal, s_content, s_bussopid);
	return s_shortmsg;
}

// * ================================================================================ *
// @ _appserver_createbussopid
// * ================================================================================ *
t_void _appserver_createbussopid(string& s_bussopid, const char s_type, t_pcstr s_terminalid, t_pcstr s_bussoptype, t_pcstr s_starttime)
{
	s_bussopid = s_type;
	s_bussopid += s_terminalid;
	s_bussopid += s_bussoptype;
	s_bussopid += time_format_str(time(NULL), 1);
	{
		static t_int i_bussseq = 1;
		static char s_temp[6];
		sprintf(s_temp, "%05d", i_bussseq++);
		s_bussopid += s_temp;
		if(i_bussseq >= 99999)	i_bussseq = 1;
	}
}

// * ================================================================================ *
// @ appserver_proc_realtimebussop
// * ================================================================================ *
t_int appserver_proc_realtimebussop(t_pcstr s_pack_rmbussop, t_pcstr s_bussoptype, t_int& i_ret, string& s_resultdes, string & s_bussopid)
{
	LOG(LOG6, "appserver_proc_realtimebussop(): s_bussoptype='%s'\n", s_bussoptype);
	
	static string s_terminal, s_starttime, s_user, s_resfname;
	static CNString s_shormsg_submit, s_shortmsg_content;

	i_ret = 0;
	s_resultdes = "";
	rundata_realtest* p_runtest = NULL;
	
	//重新上传log文件
	//<head>xxxxx</head><msg>PA_W2A</msg><user>xxx</user><bussopid></bussopid>
	if ( strcmp(s_bussoptype, "PA") == 0 )
	{
		CClientMsg::ansy_getitem(s_pack_rmbussop, "bussopid", s_bussopid);
		
		DATASET dbresult;
		if ( select_bussop_by_bussopid(s_bussopid.c_str(), dbresult) <= 0 )
		{
			i_ret = 1;
			s_resultdes = "bussopid不存在";
			return 1;
		}
		
		//TANG 2010-6-27 15:06
		//有可能数据库中已经是X状态，而web看到的以前的Y状态所以才发出PA消息
		if ( dbresult[0]["STATUS"] == "X" )
		{
			LOG(LOG6, "appserver_proc_realtimebussop(): success state in db, not need re-ftp-update again\n");
			i_ret = 0;
			s_resultdes = "";
			return 0;
		}
		
		char s_bussoptype_new[8] = {0};
		
		s_terminal = dbresult[0]["TERMINAL_ID"];
		strcpy(s_bussoptype_new, dbresult[0]["BUSSOP_TYPE"].c_str());
		s_resfname = dbresult[0]["RES_FNAM"];
		s_starttime = dbresult[0]["STARTTIME"];
		
		//TANG 2010-6-14 14:58
		//给终端的文件名不能是zip，要换成txt或者wav
		s_resfname.erase(s_resfname.length()-4, 4);
		if ( s_bussoptype_new[0] == 'C' ) s_resfname += ".wav";
		else s_resfname += ".txt";
		
		p_runtest = realtest_insert(s_terminal.c_str(), s_bussoptype_new[0], s_bussoptype_new);
		p_runtest->c_status = 'W';
		p_runtest->s_testdata = ToString(s_pack_rmbussop) + "<terminal>" + s_terminal + "</terminal><starttime>" + s_starttime + "</starttime>";
		p_runtest->s_bussopid = s_bussopid;
		
		appserver_dbupdate_bussop_status(s_bussopid.c_str(), 'W', 5001);
		
		s_shortmsg_content.Format("55512|%s", s_resfname.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), "");
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		
		return 0;
	}
	
	if( ("" == CClientMsg::ansy_getitem(s_pack_rmbussop, "terminal", s_terminal)) ||
		("" == CClientMsg::ansy_getitem(s_pack_rmbussop, "starttime", s_starttime)) ||
		("" == CClientMsg::ansy_getitem(s_pack_rmbussop, "user", s_user))) 
	{
		i_ret = 1;
		s_resultdes = "参数错误";
		return 1;
	}
	
	//蜂鸣短信
	if ( strcmp(s_bussoptype, "KA") == 0 )
	{
		s_shortmsg_content.Format("88805");
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), "");
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		
		i_ret = 0;
		s_resultdes = "";
		s_bussopid = "";
		return 0;
	}
	
	if('X' == s_bussoptype[0]) 
	{
		p_runtest = realtest_find(s_terminal.c_str(), s_bussoptype[1]);
		if(p_runtest) 
		{
			s_bussopid = p_runtest->s_bussopid;
			appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 101);
			realtest_delete(s_terminal.c_str(), s_bussoptype[1]);
			LOG(LOG3, "测试取消 %s %c\n", s_terminal.c_str(),s_bussoptype[1]);
			return 0;
		}
	}

	p_runtest = realtest_find(s_terminal.c_str(), s_bussoptype[0]); 
	if(p_runtest) 
	{
		i_ret = 2;
		s_resultdes = "测试进行中...";
		s_bussopid = p_runtest->s_bussopid;
		return 1;
	}
	
	_appserver_createbussopid(s_bussopid, 'T', s_terminal.c_str(), s_bussoptype, s_starttime.c_str());
	p_runtest = realtest_insert(s_terminal.c_str(), s_bussoptype[0], s_bussoptype);
	p_runtest->c_status = 'B';
	p_runtest->s_testdata = s_pack_rmbussop;
	p_runtest->s_bussopid = s_bussopid;

	appserver_dbinsert_bussop(s_bussopid.c_str(), 'T', s_bussoptype, s_terminal.c_str(), s_starttime.c_str(), s_user.c_str(), p_runtest->c_status);

	// PESQ TEST
	if(!strcmp(s_bussoptype, "CA")) 
	{
		string s_srcfname;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "srcfname", s_srcfname);
		s_shortmsg_content.Format("33380|%s", s_srcfname.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		p_runtest->c_status = 'H';
	}
	// 
	else if(!strcmp(s_bussoptype, "AA")) 
	{
		s_shortmsg_content.Format("22210");
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "AB")) 
	{
		string s_authpwd, s_url;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "auth_pwd", s_authpwd);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "url", s_url);

		s_shortmsg_content.Format("%s|%s|%s", "00110", s_authpwd.c_str(), s_url.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';

	}
	else if(!strcmp(s_bussoptype, "AC")) 
	{
		s_shortmsg_content.Format("00120");
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "DA") || !strcmp(s_bussoptype, "MA")) 
	{
		string s_url, s_times, s_packsize, s_tminter, s_tmout;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "url", s_url);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "times", s_times);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "packsize", s_packsize);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tm_inter", s_tminter);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tmout", s_tmout);

		appserver_dbinsert_bussop_content_ping(s_bussopid.c_str(), s_times.c_str(), s_packsize.c_str(), s_tminter.c_str(), s_tmout.c_str(), s_url.c_str());

		if(!strcmp(s_bussoptype, "DA")) s_shortmsg_content.Format("%s|%s|%s|%s|%s|%s", "33310", s_times.c_str(), s_packsize.c_str(), s_tminter.c_str(), s_tmout.c_str(), s_url.c_str());
		if(!strcmp(s_bussoptype, "MA")) s_shortmsg_content.Format("%s|%s|%s|%s|%s|%s", "W33310", s_times.c_str(), s_packsize.c_str(), s_tminter.c_str(), s_tmout.c_str(), s_url.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "EA") || !strcmp(s_bussoptype, "NA")) 
	{
		string s_url, s_times, s_tmout, s_direction, s_numprocess;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "url", s_url);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "times", s_times);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tmout", s_tmout);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "direction", s_direction);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "numprocess", s_numprocess);

		appserver_dbinsert_bussop_content_ftp(s_bussopid.c_str(), s_url.c_str(), s_times.c_str(), s_tmout.c_str(), s_direction.c_str(), s_numprocess.c_str());

		if(!strcmp(s_bussoptype, "EA")) s_shortmsg_content.Format("%s|%s|%s|%s|%s|%s", "33320", s_times.c_str(), s_tmout.c_str(), s_url.c_str(), s_direction.c_str(), s_numprocess.c_str());
		if(!strcmp(s_bussoptype, "NA")) s_shortmsg_content.Format("%s|%s|%s|%s|%s|%s", "W33320", s_times.c_str(), s_tmout.c_str(), s_url.c_str(), s_direction.c_str(), s_numprocess.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "FA") || !strcmp(s_bussoptype, "OA")) 
	{
		string s_url, s_times, s_tmout;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "url", s_url);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "times", s_times);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tmout", s_tmout);

		appserver_dbinsert_bussop_content_http(s_bussopid.c_str(), s_url.c_str(), s_times.c_str(), s_tmout.c_str());

		if(!strcmp(s_bussoptype, "FA")) s_shortmsg_content.Format("%s|%s|%s|%s", "33330", s_times.c_str(), s_tmout.c_str(), s_url.c_str());
		if(!strcmp(s_bussoptype, "OA")) s_shortmsg_content.Format("%s|%s|%s|%s", "W33330", s_times.c_str(), s_tmout.c_str(), s_url.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "BA")) 
	{
		string s_tminter, s_times, s_test_tminter, s_test_times, s_tm_callhold;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tm_inter", s_tminter);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "times", s_times);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "test_tminter", s_test_tminter);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "test_times", s_test_times);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tm_callhold", s_tm_callhold);

		appserver_dbinsert_bussop_content_cqt(s_bussopid.c_str(), '1', "", s_tminter.c_str(), s_times.c_str(), s_test_tminter.c_str(), s_test_times.c_str(), s_tm_callhold.c_str());

		s_shortmsg_content.Format("%s|%s|%s|%s|%s|%s|%s", "33360", "1", s_tminter.c_str(), s_times.c_str(), s_test_tminter.c_str(), s_test_times.c_str(), s_tm_callhold.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "BB")) 
	{
		string s_terminal2, s_tminter, s_times, s_test_tminter, s_test_times, s_tm_callhold;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "terminal2", s_terminal2);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tm_inter", s_tminter);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "times", s_times);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "test_tminter", s_test_tminter);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "test_times", s_test_times);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "tm_callhold", s_tm_callhold);

		appserver_dbinsert_bussop_content_cqt(s_bussopid.c_str(), '1', s_terminal2.c_str(),	s_tminter.c_str(), s_times.c_str(), s_test_tminter.c_str(), s_test_times.c_str(), s_tm_callhold.c_str());

		s_shortmsg_content.Format("%s|%s|%s|%s", "33362", "2", s_tminter.c_str(), s_times.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "GA")) 
	{
		s_shortmsg_content.Format("44480");
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "HA")) 
	{
		string s_pwdori, s_pwdnew;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "pwd_ori", s_pwdori);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "pwd_new", s_pwdnew);

		s_shortmsg_content.Format("%s|%s|%s", s_bussoptype, "00200", s_pwdori.c_str(), s_pwdnew.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "HB")) 
	{
		string s_authpwd;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "authpwd", s_authpwd);
		CClientMsg msg_toftp;
		msg_toftp.ori_init("HB_1_A2F");
		msg_toftp.ori_additem("terminal", s_terminal.c_str());
		msg_toftp.ori_additem("authpwd", s_authpwd.c_str());
		msg_toftp.ori_end();
		
		if(0 != ftpsock_send(msg_toftp.packet().c_str(), msg_toftp.packet().length())) 
		{
			LOG(LOG1, "ftpsock_send() error, ftp扫描单元未启动 terminal[%s] bussoptype:%c\n", s_terminal.c_str(), s_bussoptype[0]);
			appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 7);
			realtest_delete(s_terminal.c_str(), s_bussoptype[0]);
			return 0;
		}
		
		p_runtest->c_status = 'H';
	}
	else if(!strcmp(s_bussoptype, "HC")) 
	{
		string s_authpwd, s_url;
		CClientMsg::ansy_getitem(s_pack_rmbussop, "auth_pwd", s_authpwd);
		CClientMsg::ansy_getitem(s_pack_rmbussop, "url", s_url);

		s_shortmsg_content.Format("%s|%s|%s", "00112", s_authpwd.c_str(), s_url.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());

		p_runtest->c_status = 'H';
	}
	
	//终端重启
	else if(!strcmp(s_bussoptype, "LA")) 
	{
		s_shortmsg_content.Format("88888|restart");
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		
		p_runtest->c_status = 'H';
	}
	
	return 0;
}

// * ================================================================================ *
// @ appserver_proc_webpack_cancel
// * ================================================================================ *
t_int appserver_proc_webpack_cancel(t_pcstr s_pack_cancelbussop, t_pcstr s_msgtype, t_int& i_ret, string& s_resultdes)
{
	LOG(LOG6, "appserver_proc_webpack_cancel()\n");
	
	i_ret = 0;
	s_resultdes = "";
	return 0;
}

// * ================================================================================ *
// @ appserver_proc_pesqpack
// * ================================================================================ *
t_int appserver_proc_pesqpack(t_pcstr s_rpack)
{
	LOG(LOG6, "appserver_proc_pesqpack()\n");
	
	string s_testtype;
	CClientMsg::ansy_getitem(s_rpack, "test_type", s_testtype);
	if('R' == s_testtype[0])
		return 0;

	if('T' == s_testtype[0]) {
		string s_terminal;
		t_int i_ret;
		rundata_realtest* p_runtest = NULL;

		CClientMsg::ansy_getitem(s_rpack, "terminal", s_terminal);
		p_runtest = realtest_find(s_terminal.c_str(), BUSS_PESQ);
		if(!p_runtest) {
			LOG(LOG1, "appserver_proc_pesqpack 测试查询失败 %s_%c\n", s_terminal.c_str(), BUSS_PESQ);
			return 1;
		}
		i_ret = CClientMsg::ansy_getitem(s_rpack, "result");
		if(0 != i_ret) {	// 失败的PESQ操作
			appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 1000+i_ret);
			realtest_delete(s_terminal.c_str(), BUSS_PESQ);
			return 0;
		}

		/*
		string s_starttime, s_srcfname, s_recordfname;
		CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "starttime", s_starttime);
		CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "srcfname", s_srcfname);
		CClientMsg::ansy_getitem(s_rpack, "recordfname", s_recordfname);
		CClientMsg climsg_2sum;
		climsg_2sum.ori_init("CA_A2S");
		climsg_2sum.ori_additem("terminal", s_terminal.c_str());
		climsg_2sum.ori_additem("starttime", s_starttime.c_str());
		climsg_2sum.ori_additem("srcfname", s_srcfname.c_str());
		climsg_2sum.ori_additem("testid", p_runtest->s_bussopid.c_str());
		climsg_2sum.ori_additem("test_type", 'T');
		climsg_2sum.ori_additem("recordfname", s_recordfname.c_str());
		climsg_2sum.ori_end();
		if(0 != sumsock_send(climsg_2sum.packet().c_str(), climsg_2sum.packet().length())) {
			appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 9);
			realtest_delete(s_terminal.c_str(), BUSS_PESQ);
			LOG(LOG1, "appserver_proc_pesqpack 发送中间结果运算失败 %s_%c\n", s_terminal.c_str(), BUSS_PESQ);
			return 1;
		}
		*/
		
		return 0;
	}
	return 1;
}

// * ================================================================================ *
// @ appserver_proc_ftppack
// * ================================================================================ *
t_int appserver_proc_ftppack(t_pcstr s_rpack)
{
	LOG(LOG6, "appserver_proc_ftppack()\n");
	
	string s_msg, s_terminal, s_bussopid;
	t_int i_result;

	CClientMsg::ansy_getitem(s_rpack, "msg", s_msg);
	CClientMsg::ansy_getitem(s_rpack, "terminal", s_terminal);
	CClientMsg::ansy_getitem(s_rpack, "bussopid", s_bussopid);
	i_result = CClientMsg::ansy_getitem(s_rpack, "result");
	
	rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), s_msg.c_str()[0]);
	if(!p_runtest) 
	{
		LOG(LOG1, "appserver_proc_ftppack 测试查询失败 %s_%c\n", s_terminal.c_str(), s_msg.c_str()[0]);
		return 1;
	}
	
	if("HB_1_F2A" == s_msg) //升级配置文件
	{
		if(0 != i_result) 
		{
			LOG(LOG1, "appserver_proc_ftppack 返回失败 %s_%c\n", s_terminal.c_str(), s_msg.c_str()[0]);
			appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), i_result+3000);
			realtest_delete(s_terminal.c_str(), s_msg.c_str()[0]);
			return 0;
		}
		
		string s_authpwd, s_fileurl;
		CNString s_shortmsg_content, s_shormsg_submit;
		CClientMsg::ansy_getitem(s_rpack, "authpwd", s_authpwd);
		CClientMsg::ansy_getitem(s_rpack, "url", s_fileurl);

		s_shortmsg_content.Format("%s|%s|%s", "00112", (( s_authpwd.length() > 0 ) ? s_authpwd.c_str() : "123456"), s_fileurl.c_str());
		_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), s_bussopid.c_str());
		g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		
		return 0;
	}
	
	if(0 != i_result) 
	{
		LOG(LOG1, "appserver_proc_ftppack 返回失败 %s_%c\n", s_terminal.c_str(), s_msg.c_str()[0]);
		appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), i_result+3000);
	}
	else 
	{
		appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
	}
	realtest_delete(s_terminal.c_str(), s_msg.c_str()[0]);
	
	return 0;
}

// * ================================================================================ *
// @ appserver_proc_sumpack
// * ================================================================================ *
t_int appserver_proc_sumpack(t_pcstr s_rpack)
{
	LOG(LOG6, "appserver_proc_sumpack()\n");
	
	string s_terminal;
	t_int i_ret;
	rundata_realtest* p_runtest = NULL;

	CClientMsg::ansy_getitem(s_rpack, "terminal", s_terminal);
	p_runtest = realtest_find(s_terminal.c_str(), BUSS_PESQ);
	if(!p_runtest) {
		LOG(LOG1, "appserver_proc_sumpack 测试查询失败 %s_%c\n", s_terminal.c_str(), BUSS_PESQ);
		return 1;
	}
	if(0 != i_ret) {	// 失败的SUM操作
		i_ret += 4000;
		appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 4000+i_ret);
	}
	else
		appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
	realtest_delete(s_terminal.c_str(), BUSS_PESQ);
	return 0;
}

// * ================================================================================ *
// @ _appserver_shortmsg_send_rsp
// * ================================================================================ *
/*
t_void _appserver_shortmsg_send_rsp(t_pcstr s_terminal, t_int i_cmd)
{
	static CClientMsg clientmsg;
	static CNString s_cmdid;
	s_cmdid.Format("%d", i_cmd+1);
	clientmsg.ori_init(s_cmdid.c_str());
	clientmsg.ori_additem("num", s_terminal);
	clientmsg.ori_end();

	LOG(LOG4, "_appserver_shortmsg_send_rsp [%s] [%s]\n", s_terminal, clientmsg.packet().c_str());

	g_appserver.p_buf_smsg_snd->pack_push(clientmsg.packet().c_str(), clientmsg.packet().length());
}
*/

// * ================================================================================ *
// @ appserver_proc_rcvshortmsg
// * ================================================================================ *
t_int appserver_proc_rcvshortmsg(string& s_terminal, string& s_content)
{
	LOG(LOG6, "appserver_proc_rcvshortmsg() [%s] [%s]\n", s_terminal.c_str(), s_content.c_str());
	
	ConfigLine line1;
	line1.readline(s_content, "|");
	
	//心跳短信
	if ( line1[0] == "heartbeat" )
	{
		insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		
		CNString sql;
		sql.Format("select * from CONFIG_TERMINAL where TERMINAL_ID = '%s' and HEARTBEAT_STATE = 1;", s_terminal.c_str());
		
		DATASET dbresult;
		if ( DB1.db_select(sql.c_str(), dbresult, LOG6) > 0 )
		{
			//终端当前是心跳短信超时状态
			string s_bussopid;
			string s_starttime = time_format_str(time(NULL), 1);
			_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "IB", s_starttime.c_str());
			appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "IB", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
			insert_bussop_exception_result(s_bussopid.c_str(), "HEARTBEAT_OK");
		}
		
		update_terminal_heartbeat(s_terminal.c_str());
		return 0;
	}
	
	//掉电告警 YR1|77760|年月日时分秒
	else if ( line1[0] == "77760" )
	{
		insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		
		string s_bussopid;
		string s_starttime = line1[1];
		_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "JA", s_starttime.c_str());
		appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "JA", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
		insert_bussop_exception_result(s_bussopid.c_str(), "220V_STOP");
		return 0;
	}
	
	//220V来电 YR1|77762|年月日时分秒
	else if ( line1[0] == "77762" )
	{
		insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		
		string s_bussopid;
		string s_starttime = line1[1];
		_ori_bussopid(s_bussopid, BUSSOPTYPE_ROUTINE, s_terminal.c_str(), "JB", s_starttime.c_str());
		appserver_dbinsert_bussop(s_bussopid.c_str(), BUSSOPTYPE_ROUTINE, "JB", s_terminal.c_str(), s_starttime.c_str(), "", 'X');
		insert_bussop_exception_result(s_bussopid.c_str(), "220V_START");
		return 0;
	}
	
	// 系统配置更新之更新配置文件
	else if ( line1[0] == "00113" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_CONFTERM);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			//YR1|00113|2|1
			//确认状态：0：主叫号码不符失败；1：认证密码不符失败；2：确认指令正确可以执行
			if ( ToInt(line1[1]) != 2 )
			{
				//失败
				appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 6000 + ToInt(line1[1]));
				realtest_delete(s_terminal.c_str(), BUSS_CONFTERM);
				LOG(LOG6, "升级配置文件失败：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			}
			else
			{
				//等待配置文件升级结果反馈
				LOG(LOG6, "等待配置文件升级结果反馈：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			}
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// 在线升级命令确认
	else if ( line1[0] == "00111" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_UPVER);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "在线升级命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// PING测试命令确认
	else if ( line1[0] == "33311" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_PING);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "PING测试命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// PING测试命令确认
	else if ( line1[0] == "W33311" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_PING_WIFI);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "PING测试命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// FTP测试命令确认
	else if ( line1[0] == "33321" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_FTP);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "FTP测试命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// FTP测试命令确认
	else if ( line1[0] == "W33321" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_FTP_WIFI);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "FTP测试命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// HTTP测试命令确认
	else if ( line1[0] == "33331" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_HTTP);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "HTTP测试命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// HTTP测试命令确认
	else if ( line1[0] == "W33331" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_HTTP_WIFI);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "HTTP测试命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// CDMA参数实时测试短信命令确认
	else if ( line1[0] == "44481" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_CDMA);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "CDMA参数实时测试短信命令确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// 版本回滚确认
	else if ( line1[0] == "00121" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_UPVER);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "版本回滚确认：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// 配置文件升级结果反馈
	else if ( line1[0] == "77732" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_CONFTERM);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			//配置文件升级结果反馈确认
			//77733|重发控制位
			static CNString s_shormsg_submit, s_shortmsg_content;
			s_shortmsg_content.Format("77733|%s", line1[2].c_str());
			_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), p_runtest->s_bussopid.c_str());
			g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
			
			//YR1|77732|1|1
			//升级状态：0，表示失败；1，表示成功
			if ( ToInt(line1[1]) == 0 )
			{
				//失败
				appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 6010);
				realtest_delete(s_terminal.c_str(), BUSS_CONFTERM);
				LOG(LOG6, "升级配置文件失败：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			}
			else
			{
				//成功
				appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
				realtest_delete(s_terminal.c_str(), BUSS_CONFTERM);
				LOG(LOG6, "升级配置文件成功：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			}
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
			
			//配置文件升级结果反馈确认
			//77733|重发控制位
			static CNString s_shormsg_submit, s_shortmsg_content;
			s_shortmsg_content.Format("77733|%s", line1[2].c_str());
			_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), "");
			g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		}
		
		return 0;
	}
	
	// CDMA短信上报命令
	else if ( line1[0] == "66610" )
	{
		insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		
		sms_66610_t s1;
		s1.tinsert = time(NULL);
		s1.phone = s_terminal;
		s1.content = s_content;
		sms66610.push_back(s1);
		LOG(LOG6, "sms66610.size() = %d\n", sms66610.size());
		
		if ( check_66610_complete(s_terminal, ToInt(line1[2])) )
		{
			//多条消息收全了，可以处理了
			LOG(LOG6, "66610 complete\n");
			deal_sms_66610(s_terminal);
		}
		else
		{
			//消息不全，等待
			LOG(LOG6, "66610 not complete\n\n");
		}
		
		return 0;
		
		/*
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_CDMA);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			CNString sql;
			sql.Format("select * from BUSSOP_RESULT_CDMAPARAM where BUSSOP_ID = '%s';", p_runtest->s_bussopid.c_str());
			DATASET dbresult;
			if ( DB1.db_select(sql.c_str(), dbresult, LOG6) == 0 )
			{
				sql.Format("INSERT INTO BUSSOP_RESULT_CDMAPARAM (BUSSOP_ID, SEQ, STATUS) VALUES ('%s', %d, '%s');", p_runtest->s_bussopid.c_str(), 1, "");
				DB1.db_execute(sql.c_str(), LOG6);
			}
			
			sql = "UPDATE BUSSOP_RESULT_CDMAPARAM SET ";
			for ( int jj=0; jj<(line1.size()-3)/2; jj++ )
			{
				if ( jj != 0 )
				{
					sql += "', ";
				}
				CNString fieldname = line1[3+jj*2].c_str();
				if(NULL == _cdmapara_db_convert(fieldname))
				{
					LOG(LOG1, "CDMA参数 %s 解析失败\n", line1[3+jj*2].c_str());
					return 0;
				}
				sql += fieldname.c_str();
				sql += "='";
				sql += line1[3+jj*2+1].c_str();
			}
			sql += "' WHERE BUSSOP_ID = '";
			sql += p_runtest->s_bussopid.c_str();
			sql += "';";
			DB1.db_execute(sql.c_str(), LOG6);
			
			if ( ToInt(line1[1]) == ToInt(line1[2]) )
			{
				//最后一条短信
				appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
				realtest_delete(s_terminal.c_str(), BUSS_CDMA);
				LOG(LOG6, "CDMA短信上报成功：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			}
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
		*/
	}
	
	// 认证密码配置反馈
	else if ( line1[0] == "00201" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_CONFTERM);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
			realtest_delete(s_terminal.c_str(), BUSS_CONFTERM);
			LOG(LOG6, "[%s] 认证密码配置反馈 %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// CQT
	else if ( line1[0] == "33362" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_CQT);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			string s_called, s_tminter, s_times, s_test_tminter, s_test_times, s_tm_callhold;
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "called", s_called);
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "tm_inter", s_tminter);
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "times", s_times);
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "test_tminter", s_test_tminter);
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "test_times", s_test_times);
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "tm_callhold", s_tm_callhold);
			
			static CNString s_shormsg_submit, s_shortmsg_content;
			s_shortmsg_content.Format("%s|%s|%s|%s|%s|%s|%s",
					"33360", "1", s_tminter.c_str(), s_times.c_str(), s_test_tminter.c_str(), s_test_times.c_str(), s_tm_callhold.c_str());
			_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), p_runtest->s_bussopid.c_str());
			g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
			
			/*
			CClientMsg clientmsg;
			clientmsg.ori_init("");
			clientmsg.ori_additem("number", s_terminal.c_str());
			{
				CNString s_cmd;
				s_cmd.Format("%s|%s|%s|%s|%s|%s|%s",
					"33360", "1", s_tminter.c_str(), s_times.c_str(), s_test_tminter.c_str(), s_test_times.c_str(), s_tm_callhold.c_str());
				clientmsg.ori_additem("content", s_cmd.c_str());
			}
			clientmsg.ori_end();
			g_appserver.p_buf_smsg_snd->pack_push(clientmsg.packet().c_str(), clientmsg.packet().length());
			*/
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// PESQ测试确认命令
	else if ( line1[0] == "33381" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_PESQ);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			LOG(LOG6, "PESQ测试确认命令：[%s] %s\n", s_terminal.c_str(), s_content.c_str());
			
			string s_srcfname, s_starttime;
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "srcfname", s_srcfname);
			CClientMsg::ansy_getitem(p_runtest->s_testdata.c_str(), "starttime", s_starttime);
				
			CClientMsg msg_pesq_a2p;
			msg_pesq_a2p.ori_init("CA_A2P");
			msg_pesq_a2p.ori_additem("terminal", s_terminal.c_str());
			msg_pesq_a2p.ori_additem("starttime", s_starttime.c_str());
			msg_pesq_a2p.ori_additem("srcfname", s_srcfname.c_str());
			msg_pesq_a2p.ori_additem("test_type", "T");
			msg_pesq_a2p.ori_end();
			
			if(0 != pesqsock_send(msg_pesq_a2p.packet().c_str(), msg_pesq_a2p.packet().length())) 
			{
				appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 6);
				realtest_delete(s_terminal.c_str(), BUSS_PESQ);
				LOG(LOG6, "排队机未启动,测试取消 terminal[%s] bussoptype:%c\n", s_terminal.c_str(), BUSS_PESQ);
				return 0;
			}
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// 在线升级版本查询反馈
	else if ( line1[0] == "22211" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_UPVER);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			CNString s_ncontent, s_ncurver, s_nbkver;
			t_int i_start = 0;
			s_ncontent = s_content.c_str();
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_nbkver, i_start, '|');
			appserver_dbinsert_bussop_result_queryver(p_runtest->s_bussopid.c_str(), s_ncurver.c_str(), s_nbkver.c_str());
			appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
			realtest_delete(s_terminal.c_str(), BUSS_UPVER);
			LOG(LOG6, "[%s]版本查询结果 %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// 终端重启
	else if ( line1[0] == "88889" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_RESTART);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
			realtest_delete(s_terminal.c_str(), BUSS_RESTART);
			LOG(LOG6, "[%s]终端重启反馈 %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// 在线升级成功
	else if ( line1[0] == "77730" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_UPVER);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			//版本升级结果反馈确认
			//77731|重发控制位
			static CNString s_shormsg_submit, s_shortmsg_content;
			s_shortmsg_content.Format("77731|%s", line1[3].c_str());
			_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), p_runtest->s_bussopid.c_str());
			g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
			
			CNString s_ncontent, s_ncurver, s_nbkver;
			t_int i_start = 0;
			s_ncontent = s_content.c_str();
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_nbkver, i_start, '|');
			appserver_dbinsert_bussop_result_upver(p_runtest->s_bussopid.c_str(), s_ncurver.c_str(), s_nbkver.c_str(), 'S');
			appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
			realtest_delete(s_terminal.c_str(), BUSS_UPVER);
			LOG(LOG6, "[%s]版本升级成功 %s\n", s_terminal.c_str(), s_content.c_str());
			
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
			
			//版本升级结果反馈确认
			//77731|重发控制位
			static CNString s_shormsg_submit, s_shortmsg_content;
			s_shortmsg_content.Format("77731|%s", line1[3].c_str());
			_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), "");
			g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		}
		
		return 0;
	}
	
	// 在线升级失败
	else if ( line1[0] == "77740" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_UPVER);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			//版本升级结果反馈确认
			//77741|重发控制位
			static CNString s_shormsg_submit, s_shortmsg_content;
			s_shortmsg_content.Format("77741|%s", line1[3].c_str());
			_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), p_runtest->s_bussopid.c_str());
			g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
			
			CNString s_ncontent, s_ncurver, s_nbkver;
			t_int i_start = 0;
			s_ncontent = s_content.c_str();
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_nbkver, i_start, '|');
			appserver_dbinsert_bussop_result_upver(p_runtest->s_bussopid.c_str(), s_ncurver.c_str(), s_nbkver.c_str(), 'F');
			appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 7000);
			realtest_delete(s_terminal.c_str(), BUSS_UPVER);
			LOG(LOG6, "[%s]版本升级失败 %s\n", s_terminal.c_str(), s_content.c_str());
			
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
			
			//版本升级结果反馈确认
			//77741|重发控制位
			static CNString s_shormsg_submit, s_shortmsg_content;
			s_shortmsg_content.Format("77741|%s", line1[3].c_str());
			_shortmsg_create_inter(s_shormsg_submit, s_terminal.c_str(), s_shortmsg_content.c_str(), "");
			g_appserver.p_buf_smsg_snd->pack_push(s_shormsg_submit.c_str(), s_shormsg_submit.get_length());
		}
		
		return 0;
	}
	
	//当有同类命令已经在进行时，反馈冲突指令给主控端
	//77750|0|02|0|02
	//77750＋当前进行的测试类型＋当前进行的测试种类＋抛弃掉的测试类型＋抛弃掉的测试种类
	//测试类型：0－实时，1－例行
	//测试种类：同55510测试命令。00：CQT；01：PESQ；02：PING；03：CDMA参数收集； 04：FTP：05：HTTP
	else if ( line1[0] == "77750" )
	{
		LOG(LOG6, "同类命令已经在进行，反馈冲突指令\n\n");
		return 0;
	}
	
	// 版本回滚报告
	else if ( line1[0] == "77770" )
	{
		rundata_realtest* p_runtest = realtest_find(s_terminal.c_str(), BUSS_UPVER);
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			CNString s_ncontent, s_ncurver, s_flag;
			t_int i_start = 0;
			s_ncontent = s_content.c_str();
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_ncurver, i_start, '|');
			s_ncontent.read_para(s_flag, i_start, '|');
			
			appserver_dbinsert_bussop_result_rollbkver(p_runtest->s_bussopid.c_str(), s_ncurver.c_str(), '1'==s_flag.c_str()[0] ? 'S' : 'F');
			if(1 == s_flag.to_int32()) 
			{
				appserver_dbupdate_bussop_success(p_runtest->s_bussopid.c_str());
			}
			else
			{
				appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 8000);
			}
			realtest_delete(s_terminal.c_str(), BUSS_UPVER);
			LOG(LOG6, "[%s] 版本回滚报告 %s\n", s_terminal.c_str(), s_content.c_str());
			return 0;
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	// 文件结果
	else if ( line1[0] == "55510" )
	{
		CNString s_ncontent, s_nbussoptype, s_nsuccflag, s_nftpaddr;
		t_int i_start = 0;
		s_ncontent = s_content.c_str();
		s_ncontent.read_para(s_nbussoptype, i_start, '|');
		s_ncontent.read_para(s_nbussoptype, i_start, '|');
		s_ncontent.read_para(s_nsuccflag, i_start, '|');
		s_ncontent.read_para(s_nftpaddr, i_start, '|');
		
		//不知道为什么，收到短信中的文件名的.zip实际是.zi
		string filename = s_nftpaddr.c_str();
		string postfix = filename.substr(filename.find_last_of(".") + 1, string::npos);
		if ( postfix == "zi" )
		{
			s_nftpaddr += "p";
		}
	
		rundata_realtest* p_runtest = NULL;
		CClientMsg msg_toftp;
		t_char c_busstype;
		switch(s_nbussoptype.to_int32())
		{
		case 0:	//CQT
			c_busstype = BUSS_CQT;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("BA_A2F");
			break;
		case 1:	//PESQ
			c_busstype = BUSS_PESQ;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("CA_A2F");
			break;
		case 2:	//PING
			c_busstype = BUSS_PING;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("DA_A2F");
			break;
		case 3:	//CDMA
			c_busstype = BUSS_CDMA;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("GA_A2F");
			break;
		case 4:	//FTP
			c_busstype = BUSS_FTP;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("EA_A2F");
			break;
		case 5:	//HTTP
			c_busstype = BUSS_HTTP;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("FA_A2F");
			break;
		case 7:	//PING-WIFI
			c_busstype = BUSS_PING_WIFI;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("MA_A2F");
			break;
		case 8:	//FTP-WIFI
			c_busstype = BUSS_FTP_WIFI;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("NA_A2F");
			break;
		case 9:	//HTTP-WIFI
			c_busstype = BUSS_HTTP_WIFI;
			p_runtest = realtest_find(s_terminal.c_str(), c_busstype);
			msg_toftp.ori_init("OA_A2F");
			break;
		default:
			LOG(LOG6, "未处理FTP上传类型 %s\n", s_nbussoptype.c_str());
			break;
		}
		
		if(p_runtest) 
		{
			insert_db_bussop_sms(p_runtest->s_bussopid.c_str(), s_terminal.c_str(), s_content.c_str(), 'R');
			
			appserver_dbupdate_bussop_resfname(p_runtest->s_bussopid.c_str(), s_nftpaddr.c_str());
			
			if(0 == s_nsuccflag.to_int32()) 
			{
				LOG(LOG6, "55510 终端上传文件失败 bussopid=%s\n", p_runtest->s_bussopid.c_str());
				appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 5001);
				realtest_delete(s_terminal.c_str(), c_busstype);
				return 0;
			}
			
			msg_toftp.ori_additem("terminal", s_terminal.c_str());
			msg_toftp.ori_additem("bussopid", p_runtest->s_bussopid.c_str());
			msg_toftp.ori_additem("filename", s_nftpaddr.c_str());
			msg_toftp.ori_end();
			
			if(0 != ftpsock_send(msg_toftp.packet().c_str(), msg_toftp.packet().length())) 
			{
				LOG(LOG1, "ftpsock_send() error, ftp扫描单元未启动, bussopid=%s\n", p_runtest->s_bussopid.c_str());
				appserver_dbupdate_bussop_error(p_runtest->s_bussopid.c_str(), 7);
				realtest_delete(s_terminal.c_str(), c_busstype);
			}
			
			//FTP扫描单元处理完文件后会设置数据库的状态，这里删除以避免超时，而且也避免下一次测试不能进行
			if ( c_busstype == BUSS_PESQ )
			{
				LOG(LOG6, "55510 PESQ complete, delete real test info\n");
				realtest_delete(s_terminal.c_str(), c_busstype);
				return 0;
			}
		}
		else
		{
			insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		}
		
		return 0;
	}
	
	else
	{
		insert_db_bussop_sms("", s_terminal.c_str(), s_content.c_str(), 'R');
		LOG(LOG1, "error: [%s] 不能识别的短信 %s\n\n", s_terminal.c_str(), s_content.c_str());
		return 0;
	}
	
	return 0;
}

