
#ifndef CLIENT_MSG_HEAD_DEFINE_
#define CLIENT_MSG_HEAD_DEFINE_


#include "../common/n_type.h"
#include <string>
using namespace std;

class CClientMsg
{
public:
	static string& ansy_getitem(t_pcstr s_pack, t_pcstr s_item, string& s_value);
	static int ansy_getitem(t_pcstr s_pack, t_pcstr s_item);

	t_void ori_init(t_pcstr s_msgtype);
	t_void ori_additem(t_pcstr s_item, t_pcstr s_itemdata);
	t_void ori_additem(t_pcstr s_item, t_int i_itemdata);
	t_void ori_end();

	string& packet() { return m_spack; }

protected:
	string m_spack;
	char   m_stmp[1024];
};


#endif
