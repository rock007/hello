/* -*- head file -*-*/

#ifndef STREAM_BUFFER_HEAD_DEFINE_
#define STREAM_BUFFER_HEAD_DEFINE_

#include "../common/n_common.h"

class CNMutex
{
public:
	CNMutex() { nmutex_initial(&m_mutex); }
	~CNMutex() { nmutex_destroy(&m_mutex);	}

public:
	t_void lock() { nmutex_lock(&m_mutex); }

	t_void unlock() { nmutex_unlock(&m_mutex); }
private:
	nt_mutex m_mutex;
};

class CBasicBuffer
{
protected:
	t_pchar		m_sdata;
	t_int		m_istart, m_icap, m_ilen;
protected:
	CBasicBuffer(t_int i_cap);
	virtual ~CBasicBuffer();

public:
	static CBasicBuffer* CreateInstance(t_int i_cap=1024*1024);
	void Destroy() { delete this; }

public:
	t_int inter_pushdata(t_pcstr s_data, const t_int i_len);
	t_int inter_popdata(t_pchar s_data, const t_int i_len);

public:
	t_int pack_push(t_pcstr s_data, const t_int i_len);
	t_int pack_pop(t_pchar s_data, t_int& i_len);

	t_int pack_push_ex(t_pcstr s_data, const t_int i_len, const struct timeval& tm_rcv);
	t_int pack_pop_ex(t_pchar s_data, t_int& i_len, struct timeval& tm_rcv);

protected:
	CNMutex m_mutex;
};

extern t_int test_CBasicBuffer();
#endif
