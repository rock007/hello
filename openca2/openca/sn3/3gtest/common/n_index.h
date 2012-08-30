/* -*- head file -*-*/

#ifndef CNINDEX_HEAD_DEFINE_
#define CNINDEX_HEAD_DEFINE_
#include "n_link.h"

template <class T>
class CNIndex
{
	struct nindex_key
	{
		char*		s_key;
		int			i_keylen;
		NPOSITION	pos_data;
	};
protected:
	CNLink<T>			m_tdata;
	CNLink<nindex_key>	m_keyother;
	int					m_imaxlen;
	nindex_key			*m_key;

protected:
	int calc_hashcode(const char* s_data, const int i_datalen)
	{
		unsigned int ui_result = 0;
		int i;
		for(i = 0; i < i_datalen; i++) {
			ui_result += 37*ui_result+ *(s_data+i);
		}
		return (int)(ui_result%m_imaxlen);
	}

public:
	CNIndex(const int i_maxlen=1011) {
		m_key = new nindex_key[i_maxlen];
		for(m_imaxlen = 0; m_imaxlen < i_maxlen; m_imaxlen++) m_key[m_imaxlen].s_key = NULL;
		m_imaxlen = i_maxlen;
	}
	virtual ~CNIndex(){ if(m_key)	delete [] m_key; }

public:
	void Initial(const int i_maxlen) {
		if(m_key)	delete [] m_key;
		m_key = new nindex_key[i_maxlen];
		for(m_imaxlen = 0; m_imaxlen < i_maxlen; m_imaxlen++) m_key[m_imaxlen].s_key = NULL;
		m_imaxlen = i_maxlen;
	}
public:
	T* Insert(const char* s_key, const int i_keylen) {
		NPOSITION pos = m_tdata.Push();
		int i_index = calc_hashcode(s_key, i_keylen);
		if(NULL == m_key[i_index].s_key) {
			m_key[i_index].pos_data = pos;
			m_key[i_index].s_key = new char[i_keylen+1];
			memcpy(m_key[i_index].s_key, s_key, i_keylen);
			m_key[i_index].i_keylen = i_keylen;
			m_key[i_index].s_key[i_keylen] = 0;
		}
		else {
			NPOSITION pos_key = m_keyother.Push();
			m_keyother[pos_key].pos_data = pos;
			m_keyother[pos_key].s_key = new char[i_keylen+1];
			m_keyother[pos_key].i_keylen = i_keylen;
			memcpy(m_keyother[pos_key].s_key, s_key, i_keylen);
			m_keyother[pos_key].s_key[i_keylen] = 0;
		}
		return &(m_tdata[pos]);
	}
	T* Find(NPOSITION& pos) {
		if(NULL == pos) { pos = m_tdata.GetHeadPosition(); return &(m_tdata[pos]);}
		pos = m_tdata.GetNextPos(pos);
		if(pos)	return &(m_tdata[pos]);
		return NULL;
	}
	T* Find(const char* s_key, const int i_keylen) {
		int i_index = calc_hashcode(s_key, i_keylen);
		if(m_key[i_index].s_key && (m_key[i_index].i_keylen == i_keylen) && (!memcmp(m_key[i_index].s_key, s_key, i_keylen))) { NPOSITION pos = m_key[i_index].pos_data;	return &(m_tdata[pos]);	} 
		NPOSITION pos_key = m_keyother.GetHeadPosition();
		while(pos_key){ 
			if(m_keyother[pos_key].s_key && (m_keyother[pos_key].i_keylen == i_keylen) && (!memcmp(m_keyother[pos_key].s_key, s_key, i_keylen)))	break;
			pos_key = m_keyother.GetNextPos(pos_key);
		}
		if(!pos_key)	return NULL;
		NPOSITION pos = m_keyother[pos_key].pos_data;
		return &(m_tdata[pos]);
	}
	int Delete(const char* s_key, const int i_keylen) {
		int i_index = calc_hashcode(s_key, i_keylen);
		if(m_key[i_index].s_key && (m_key[i_index].i_keylen == i_keylen) && (!memcmp(m_key[i_index].s_key, s_key, i_keylen))) {
			NPOSITION pos = m_key[i_index].pos_data;
			m_tdata.MovePosition(pos);
			delete []m_key[i_index].s_key;
			m_key[i_index].s_key = NULL;
			return 0;
		}
		NPOSITION pos_key = m_keyother.GetHeadPosition();
		while(pos_key) {
			if(m_keyother[pos_key].s_key && (m_keyother[pos_key].i_keylen == i_keylen) && (!memcmp(m_keyother[pos_key].s_key, s_key, i_keylen))) {
				NPOSITION pos = m_keyother[pos_key].pos_data;
				m_tdata.MovePosition(pos);
				delete []m_keyother[pos_key].s_key;
				m_keyother.MovePosition(pos_key);
				return 0;
			}
			pos_key = m_keyother.GetNextPos(pos_key);
		}
		return 1;
	}
};

/*
static void nindex_test()
{
	CNIndex<int> index_test;
	index_test.Insert("12345", 5);
	index_test.Insert("23456", 5);
	index_test.Delete("12345", 5);
	index_test.Delete("23456", 5);

	index_test.Insert("12345", 5);
	index_test.Insert("12345", 5);

	index_test.Delete("12345", 5);
	index_test.Find("12345", 5);
	index_test.Delete("12345", 5);
	index_test.Delete("12345", 5);
	int* p_i = index_test.Find("12345", 5);
}
*/
#endif
