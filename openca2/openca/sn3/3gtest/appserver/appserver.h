
#ifndef APPSERVER_HEAD_DEFINE_
#define APPSERVER_HEAD_DEFINE_

#include "../common/n_common.h"
#include "../common/n_domxmlansy.h"
#include "../common/StreamBuffer.h"
#include "../common/n_index.h"
#include "client_msg.h"
#include "appserver_db.h"
#include "appserver_bussop.h"

#ifdef WIN32
#define APPSERVER_CONFIGFNAME	"C:/NZQ_Working/Current Work/Working/N3GTEST/etc/appserver.xml"
#else
#define APPSERVER_CONFIGFNAME	"../etc/appserver.xml"
#endif

struct sms_66610_t
{
	time_t tinsert;
	string phone;
	string content;
};

struct rundata_realtest
{
	t_int   index_terminal;
	t_uchar u_buss;

	string  s_bussopid;

	t_char  c_status;

	string  s_testdata;

	time_t  tm_start;
	time_t  tm_end;
};

struct terminal_routine_pesq
{
	string	s_terminal;
	string	s_srcfname;
	string	s_starttime;
	string  s_starttime_ori;
	string	s_type;
	time_t	tm_starttm;

};

#define MAXNUM_REALTEST	4093
struct rundata_appserver
{
	t_int			m_runing;

	string			m_suser_shortmsg;
	string			m_spwd_shortmsg;
	string			m_pesq_number;

	string			m_sdb_ip;
	string			m_sdb_name;
	string			m_sdb_uname;
	string			m_sdb_upwd;


	CNIndex<rundata_realtest>		p_index_terminal;
	CBasicBuffer*	p_buf_smsg_snd;
	CBasicBuffer*	p_buf_smsg_rcv;

	t_int			m_iport_shortmsg;
	string			m_sip_shortmsg;
	t_int			m_imaxsendpersec_shortmsg;
	t_int			m_iresendtmout_shortmsg;
	t_int			m_iresendtimes_shortmsg;

	CNLink<terminal_routine_pesq> m_termrout_pesq;
	CNString					  m_scurdate;
};

extern rundata_appserver g_appserver;

extern t_int ftpsock_send(t_pcstr s_pack, const int i_packlen);
extern t_int pesqsock_send(t_pcstr s_pack, const int i_packlen);
extern t_int sumsock_send(t_pcstr s_pack, const int i_packlen);
extern rundata_realtest* realtest_find(const char* s_terminalid, const char c_buss);
extern rundata_realtest* realtest_insert(const char* s_terminalid, const char c_buss, const char* s_bussoptype);
extern void realtest_delete(const char* s_terminalid, const char c_buss);



#endif
