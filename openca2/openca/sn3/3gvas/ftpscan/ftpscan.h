
#ifndef FTPSCAN_HEAD_DEFINE_
#define FTPSCAN_HEAD_DEFINE_

#include "../common/n_common.h"
#include "../common/StreamBuffer.h"
#include "../appserver/client_msg.h"
#include "../common/n_domxmlansy.h"

#define CONFIG_FILE_PATH	"/home/spider/config/service17.ini"

struct rundata_ftpscan
{
	t_int			m_bruning;
	CBasicBuffer*	p_buf_sql;
	CBasicBuffer*	p_buf_command;

	string			m_asip;
	t_ushort		m_asport;
	
	string			m_sdb_ip;
	string			m_sdb_name;
	string			m_sdb_uname;
	string			m_sdb_upwd;

	string			m_spath;
	string			m_sfullpath;
	string			m_smovepath;
	string			m_sfname;
	t_char			m_cbuss;
	t_char			m_cbusstype;
	t_int			m_itmdelay_scan;
};

extern rundata_ftpscan g_ftpscan;

extern t_pvoid thread_ftpsock(t_pvoid p_param);
extern t_pvoid thread_ftp_procfile(t_pvoid p_param);

#endif
