
#include "StdAfx.h"
#include <stdio.h>
#include <string.h>
#include "StreamBuffer.h"

// * ============================================================== *
// @ CBasicBuffer
// * ============================================================== *
CBasicBuffer::CBasicBuffer(t_int i_cap)
{
	m_icap = i_cap;
	m_istart = m_ilen = 0;
	m_sdata = new char[m_icap];
}

// * ============================================================== *
// @ ~CBasicBuffer
// * ============================================================== *
CBasicBuffer::~CBasicBuffer()
{
	delete [] m_sdata;
}

// * ============================================================== *
// @ CreateInstance
// * ============================================================== *
CBasicBuffer* CBasicBuffer::CreateInstance(t_int i_cap)
{
	if(i_cap <= 0)	i_cap = 1024*64;
	return new CBasicBuffer(i_cap);
}

// * ============================================================== *
// @ inter_pushdata
// * ============================================================== *
t_int CBasicBuffer::inter_pushdata(t_pcstr s_data, const t_int i_len)
{
	t_int i_insertlen, i_insert;

	m_mutex.lock();
	if( (m_icap-m_ilen) < i_len)
	{
		m_mutex.unlock();
		return -1;
	}
	// calc insert position
	i_insert = m_istart+m_ilen;
	if(i_insert >= m_icap)	i_insert -= m_icap;
	// insert data
	i_insertlen = n_min((m_icap-i_insert), i_len);
	if(i_insertlen > 0)	memcpy(m_sdata+i_insert, s_data, i_insertlen);

	// insert data len2
	if(i_insertlen != i_len)
	{
		i_insertlen = (i_len-i_insertlen);
		memcpy(m_sdata, s_data+i_len-i_insertlen, i_insertlen);
	}
	m_ilen += i_len;
	m_mutex.unlock();
	return 0;
}

// * ============================================================== *
// @ inter_popdata
// * ============================================================== *
t_int CBasicBuffer::inter_popdata(t_pchar s_data, const t_int i_len)
{
	t_int i_dellen;
	m_mutex.lock();
	if(m_ilen < i_len) {
		m_mutex.unlock();
		return -1;
	}

	i_dellen = n_min((m_icap-m_istart), i_len);
	if(i_dellen > 0)	memcpy(s_data, m_sdata+m_istart, i_dellen);
	m_istart += i_dellen;
	if(m_istart >= m_icap )	m_istart = 0;

	if(i_len != i_dellen)
	{
		memcpy(s_data+i_dellen, m_sdata+m_istart, i_len-i_dellen);
		m_istart += (i_len-i_dellen);
	}
	m_ilen -= i_len;
	m_mutex.unlock();
	return 0;
}

// * ============================================================== *
// @ pack_push
// * ============================================================== *
t_int CBasicBuffer::pack_push(t_pcstr s_data, const t_int i_len)
{
	if( (t_int)(sizeof(t_int)+i_len) > (m_icap - m_ilen))	return -1;
	inter_pushdata((t_pcstr)(&i_len), sizeof(t_int));
	inter_pushdata(s_data, i_len);
	return 0;
}

// * ============================================================== *
// @ pack_pop
// * ============================================================== *
t_int CBasicBuffer::pack_pop(t_pchar s_data, t_int& i_len)
{
	i_len = 0;
	if(0 == inter_popdata((t_pchar)(&i_len), sizeof(t_int)))
	{
		inter_popdata(s_data, i_len);
		return 0;
	}
	return 1;
}

// * ============================================================== *
// @ pack_push_ex
// * ============================================================== *
t_int CBasicBuffer::pack_push_ex(t_pcstr s_data, const t_int i_len0, const struct timeval& tm_rcv)
{
	t_int i_len = i_len0+sizeof(tm_rcv);
	if( (t_int)(sizeof(t_int)+i_len) > (m_icap - m_ilen))	return -1;
	inter_pushdata((t_pcstr)(&i_len), sizeof(t_int));
	inter_pushdata((t_pcstr)(&tm_rcv), sizeof(tm_rcv));
	inter_pushdata(s_data, i_len0);
	return 0;
}

// * ============================================================== *
// @ pack_pop_ex
// * ============================================================== *
t_int CBasicBuffer::pack_pop_ex(t_pchar s_data, t_int& i_len, struct timeval& tm_rcv)
{
	i_len = 0;
	if(0 == inter_popdata((t_pchar)(&i_len), sizeof(t_int)))
	{
		i_len -= sizeof(tm_rcv);
		inter_popdata((t_pchar)&tm_rcv, sizeof(tm_rcv));
		inter_popdata(s_data, i_len);
		return 0;
	}
	return 1;
}

t_int test_CBasicBuffer()
{
	t_int i_rr;
	char s_result[20];
	t_int i_ret;
	char s_data[11] = "1234567890";
	CBasicBuffer* pbuf = CBasicBuffer::CreateInstance(13);
	memset(s_result, 0, sizeof(s_result));

	i_rr = pbuf->pack_push(s_data, 1);
	i_rr = pbuf->pack_pop(s_result, i_ret);
	i_rr = pbuf->pack_push(s_data, 5);
	i_rr = pbuf->pack_push(s_data, 0);

	memset(s_result, 0, sizeof(s_result));
	i_rr = pbuf->pack_pop(s_result, i_ret);
	i_rr = pbuf->pack_pop(s_result, i_ret);
	i_rr = pbuf->pack_pop(s_result, i_ret);
	pbuf->Destroy();
	return 0;
}
