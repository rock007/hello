
#include "StdAfx.h"
#include "client_msg.h"
#include "logfile.h"

#define LOG CCLogFile.logwrite
extern LogFile CCLogFile;

// * ============================================================== *
// @ ansy_getitem
// * ============================================================== *
string& CClientMsg::ansy_getitem(t_pcstr s_pack, t_pcstr s_item, string& s_value)
{
	const char *s_start, *s_end;
	char s_curitem[100];

	s_value.clear();
	sprintf(s_curitem, "<%s>", s_item);
	s_start = strstr(s_pack, s_curitem);
	if(s_start)
	{
		s_start += strlen(s_curitem);
		sprintf(s_curitem, "</%s>", s_item);
		s_end = strstr(s_pack, s_curitem);
		if(s_end)
			s_value.append(s_start, (s_end-s_start));
	}
	return s_value;
}

// * ============================================================== *
// @ ansy_getitem
// * ============================================================== *
int CClientMsg::ansy_getitem(t_pcstr s_pack, t_pcstr s_item)
{
	const char *s_start;
	char s_curitem[100];

	sprintf(s_curitem, "<%s>", s_item);
	s_start = strstr(s_pack, s_curitem);
	if(s_start)
	{
		s_start += strlen(s_curitem);
		return atoi(s_start);
	}
	return -1;
}

// * ============================================================== *
// @ ori_additem
// * ============================================================== *
t_void CClientMsg::ori_additem(t_pcstr s_item, t_int i_itemdata)
{
	sprintf(m_stmp, "<%s>%d</%s>", s_item, i_itemdata, s_item);
	m_spack += m_stmp;
}

// * ============================================================== *
// @ ori_additem
// * ============================================================== *
t_void CClientMsg::ori_additem(t_pcstr s_item, t_pcstr s_itemdata)
{
	sprintf(m_stmp, "<%s>%s</%s>", s_item, s_itemdata, s_item);
	m_spack += m_stmp;
}
// * ============================================================== *
// @ ori_additem
// * ============================================================== *
t_void CClientMsg::ori_init(t_pcstr s_msgtype)
{
	m_spack = "<head>00000</head>";
	ori_additem("msg", s_msgtype);
}

// * ============================================================== *
// @ s20_clientmsg_ori_end
// * ============================================================== *
t_void CClientMsg::ori_end()
{
	sprintf(m_stmp, "%05d", m_spack.length()-strlen("<head>00000</head>"));
	int j = strlen("<head>");
	for(int i = 0; i < 5; i++)
		m_spack[j+i] = m_stmp[i];
}
