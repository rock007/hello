/* -*- source file -*-*/ 

#include "StdAfx.h"
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
#include "n_string.h"

CNString::CNString()
{
	m_sdata = NULL;
	m_nlen = m_capacity = 0;
	set_capacity_size(DEF_STRING_ALLOCADD);
}

CNString::CNString(const char sdata)
{
	m_sdata = NULL;
	m_nlen = m_capacity = 0;
	insert(0, &sdata, 1);
}

CNString::CNString(const char* sdata)
{
	m_sdata = NULL;
	m_nlen = m_capacity = 0;
	set_capacity_size(DEF_STRING_ALLOCADD);
	insert(0, sdata, (int)strlen(sdata));
}

CNString::CNString(const CNString& sdata)
{
	m_sdata = NULL;
	m_nlen = m_capacity = 0;
	set_capacity_size(DEF_STRING_ALLOCADD);
	insert(0, sdata.c_str(), sdata.get_length());
}

CNString::~CNString()
{
	if(m_sdata) {
		delete [] m_sdata;
		m_sdata = NULL;
	}
}

// * ============================================================== *
// @ 功能描述: 设定字符缓冲区容量
// * ============================================================== *
int CNString::set_capacity_size(const int capacity)
{
	if( (m_nlen > (capacity+1)) || capacity <= 0)
		return -1;
	
	char* stmpdata = new char[capacity];
	if(!stmpdata)		return -2;
	
	m_capacity = capacity;
	if(m_sdata) 
	{
		memcpy(stmpdata, m_sdata, m_nlen);
		delete [] m_sdata;
	}
	
	m_sdata = stmpdata;
	m_sdata[m_nlen] = 0;
	
	return 0;
}

// * ============================================================== *
// @ 功能描述: 获取字符串的容量
// * ============================================================== *
const int CNString::get_capacity_size() const
{
	return m_capacity;
}

// * ============================================================== *
// @ 功能描述: 字符串置空
// * ============================================================== *
int CNString::empty()
{
	m_nlen = 0;
	m_sdata[0] = 0;
	return 0;
}

// * ============================================================== *
// @ 功能描述: 获取字符串的长度
// * ============================================================== *
const int CNString::get_length() const
{
	return m_nlen;
}

// * ============================================================== *
// @ 功能描述: 设定字符串的长度
// * ============================================================== *
const int CNString::set_length(const int nlen)
{
	if(nlen >= m_capacity) set_capacity_size(nlen+DEF_STRING_ALLOCADD);
	m_nlen = nlen;
	m_sdata[m_nlen] = 0;
	return 0;
}

// * ============================================================== *
// @ 功能描述: 释放多余的数据空间
// * ============================================================== *
int CNString::release_extra()
{
	if(m_capacity - m_nlen > 3) {
		return set_capacity_size(m_nlen+1);
	}
	return 0;
}

// * ============================================================== *
// @ 功能描述: 获取串
// * ============================================================== *
char* CNString::c_str()
{
	return m_sdata;
}

// * ============================================================== *
// @ 功能描述: 获取串
// * ============================================================== *
const char* CNString::c_str() const
{
	return m_sdata;
}
// * ============================================================== *
// @ 功能描述: 获取字符
// * ============================================================== *
char& CNString::operator [](const int nIndex)
{
	if(nIndex < 0 || nIndex > m_nlen)
		return m_sdata[m_nlen];
	return m_sdata[nIndex];
}

// * ============================================================== *
// @ 功能描述: 获取字符
// * ============================================================== *
const char& CNString::operator[](const int nIndex) const
{
	if(nIndex < 0 || nIndex > m_nlen)
		return m_sdata[m_nlen];
	return m_sdata[nIndex];
}

const CNString& CNString::operator=(const char cdata)
{
	empty();
	insert(0, cdata);
	return *this;
}
const CNString& CNString::operator=(const char* str)
{
	empty();
	insert(0, str, (int)(strlen(str)));
	return *this;
}

const CNString& CNString::operator=(const CNString& strSrc)
{
	empty();
	insert(0, strSrc);
	return *this;
}

const CNString& CNString::operator+=(const char ch)
{
	insert(m_nlen, ch);
	return *this;
}

const CNString& CNString::operator+=(const char* lpstr)
{
	if(lpstr) {
		insert(m_nlen, lpstr);
	}
	return *this;
}

const CNString& CNString::operator+=(const CNString& strSrc)
{
	insert(m_nlen, strSrc.m_sdata, strSrc.m_nlen);
	return *this;
}

// * ============================================================== *
// @ 功能描述: 字符串格式化 按照sprintf格式进行格式化
// * ============================================================== *
int CNString::Format(const char* sformat, ...)
{
	va_list ap;
	va_start (ap, sformat);
	int taille=1024;
	
	while (1)
	{
		set_capacity_size(taille);

		if (m_sdata == NULL)
		{
			return -1;
		}

		int n = vsnprintf (m_sdata, taille, sformat, ap);

		if (n >= 0 && n < taille)
		{
			break;
		}

		taille *= 2;
	}

	va_end (ap);

	m_nlen=(int)strlen(m_sdata);

	return 0;
}

// * ============================================================== *
// @ 功能描述: 声称kmp算法next节点数据
// * ============================================================== *
void CNString::kmp_getnextval(const char* sdata, const int nlen, int* pnext) const
{
	int i = 1, j = 0;
	pnext[1] = 0;
	while(i < nlen) {
		if(j == 0 || sdata[i] == sdata[j]) {
			++i; ++j;
			if(sdata[i] != sdata[j])	pnext[i] = j;
			else	pnext[i] = pnext[j];
		}
		else	j = pnext[j];
	}
}

// * ============================================================== *
// @ 功能描述: 数据查询(kmp算法)
// * ============================================================== *
int CNString::find(const int nIndex, const char* sFind, const int nlen) const
{
	if(nIndex < 0 || nIndex >= m_nlen)	return -1;
	if(nlen <= 0)						return -2;
	int* pnext = new int[nlen+1];
	kmp_getnextval(sFind, nlen, pnext);

	int i = nIndex, j=1;
	while(i <= m_nlen && j <= nlen) {
		if(j == 0 || m_sdata[i] == sFind[j-1]) { ++i; ++j; }
		else		j = pnext[j];
	}
	if(i <= m_nlen)	return i-nlen;
	return -1;
}

// * ============================================================== *
// @ 功能描述: 插入数据
// * ============================================================== *
int CNString::insert(const int nindex, const char* lpsz, const int nlen)
{
	assert( (nindex >= 0) && (nindex <= m_nlen) );
	if(nindex < 0 || nindex > m_nlen)	return -1;
	if(lpsz == NULL)					return -2;
	if(nlen <= 0)						return -3;
	int ntlen = m_nlen+nlen;
	// if the alloc len is not enough
	if(ntlen >= m_capacity) {
		if(set_capacity_size(ntlen+DEF_STRING_ALLOCADD))	return -1;
	}

	// if insert buffer position in the string middle
	if(m_nlen-nindex > 0) {
		memmove(m_sdata+nindex+nlen, m_sdata+nindex, m_nlen-nindex);
	}
	if(nlen > 0) {
		memcpy(m_sdata+nindex, lpsz, nlen);
	}
	m_nlen+=nlen;
	m_sdata[m_nlen] = 0;
	return 0;
}

// * ============================================================== *
// @ 功能描述: 删除长度
// * ============================================================== *
int CNString::dellen(const int nIndex, const int nlen)
{
	if(nIndex < 0 || nIndex >= m_nlen)	return 255;
	if(nlen <= 0)	return 255;
	if(nIndex+nlen > m_nlen)	return 255;
	if(m_nlen-nIndex-nlen > 0)
		memmove(m_sdata+nIndex, m_sdata+nIndex+nlen, m_nlen-nIndex-nlen);
	m_nlen -= nlen;
	m_sdata[m_nlen] = 0;
	return 0;
}

// * ============================================================== *
// @ 功能描述: 替换数据
// * ============================================================== *
int CNString::replace(const char* lpstrOld, const int nOldlen, const char* lpstrNew, const int nNewlen)
{
	int nstart = 0;
	while(1)
	{
		nstart = find(nstart, lpstrOld, nOldlen);
		if(nstart < 0)	break;
		dellen(nstart, nOldlen);
		insert(nstart, lpstrNew, nNewlen);
		nstart += nNewlen;
	}
	return 0;
}

int CNString::find(const int nstartpos, const char sdata) const
{
	return find(nstartpos, &sdata, 1);
}

int CNString::find(const int nstartpos, const char* sdata) const
{
	if(!sdata)	return -1;
	return find(nstartpos, sdata, (int)strlen(sdata));
}

int CNString::find(const int nstartpos, const CNString& sdata) const
{
	return find(nstartpos, sdata.c_str(), sdata.get_length());
}

int CNString::insert(const int nindex, const char sdata)
{
	return insert(nindex, &sdata, 1);
}

int CNString::insert(const int nindex, const char* sdata)
{
	if(sdata == NULL)	return -1;
	return insert(nindex, sdata, (int)strlen(sdata));
}

int CNString::insert(const int nindex, const CNString& sdata)
{
	return insert(nindex, sdata.c_str(), sdata.get_length());
}

int CNString::replace(const char sori_data, const char srep_data)
{
	return replace(&sori_data, 1, &srep_data, 1);
}

int CNString::replace(const char* sori_data, const char* srep_data)
{
	return replace(sori_data, (int) (strlen(sori_data)), srep_data, (int)(strlen(srep_data)) );
}

int CNString::replace(const char* sori_data, const char srep_data)
{
	return replace(sori_data, (int) (strlen(sori_data)), &srep_data, 1);
}

int CNString::replace(const char sori_data, const char* srep_data)
{
	return replace(&sori_data, 1, srep_data, (int)(strlen(srep_data)) );
}

int CNString::replace(const CNString& sori_data, const CNString& srep_data)
{
	return replace(sori_data.c_str(), sori_data.get_length(), srep_data.c_str(), srep_data.get_length());
}

int CNString::append(const char lpch)
{
	return insert(m_nlen, lpch);
}

int CNString::append(const char* lpstr)
{
	return insert(m_nlen, lpstr);
}

int CNString::append(const CNString& strSrc)
{
	return insert(m_nlen, strSrc);
}

int CNString::append(const char* lpstr, const int nlen)
{
	return insert(m_nlen, lpstr, nlen);
}

// * ============================================================== *
// @ 功能描述: 转换为大写字符
// * ============================================================== *
void CNString::make_upper()
{
	int i;
	for(i = 0; i < m_nlen; i++) {
		if(m_sdata[i] >= 'a' && m_sdata[i] <= 'z') {
			m_sdata[i] -= 'a';
			m_sdata[i] += 'A';
		}
	}
}

// * ============================================================== *
// @ 功能描述: 转换为小写字符
// * ============================================================== *
void CNString::make_lower()
{
	int i;
	for(i = 0; i < m_nlen; i++) {
		if(m_sdata[i] >= 'A' && m_sdata[i] <= 'Z') {
			m_sdata[i] -= 'A';
			m_sdata[i] += 'a';
		}
	}
}

// * ============================================================== *
// @ 功能描述: 转换字节顺序
// * ============================================================== *
void CNString::make_convert()
{
	int i=0;
	for(; i < m_nlen/2; i++)
	{
		char ctemp=m_sdata[i];
		m_sdata[i]=m_sdata[m_nlen-i-1];
		m_sdata[m_nlen-i-1]=ctemp;
	}
}

// * ============================================================== *
// @ 功能描述: 字符串比较
// * ============================================================== *
int CNString::compare(const char* sdata, const int ndatalen) const
{
	int ntmp = ndatalen;
	if(ntmp < 0)	ntmp = (int)(strlen(sdata));
	if(m_nlen > ntmp)		return 1;
	else if(m_nlen < ntmp)	return -1;
	int nsize = (((m_nlen) > (ntmp)) ? (m_nlen) : (ntmp));
	return memcmp(m_sdata, sdata, nsize);
}

int CNString::compare(const CNString& sdata) const
{
	int ntmp = sdata.get_length();
	if(m_nlen > ntmp)		return 1;
	else if(m_nlen < ntmp)	return -1;
	int nsize = (((m_nlen) > (ntmp)) ? (m_nlen) : (ntmp));
	return memcmp(m_sdata, sdata.c_str(), nsize);
}

// * ============================================================== *
// @ 功能描述: 字符串比较 -- 不区分大小写
// * ============================================================== *
int CNString::icompare(const char* sdata, const int ndatalen) const
{
	CNString stemp1, stemp2;
	stemp1.insert(0, m_sdata, m_nlen);
	if(ndatalen > 0)	stemp2.insert(0, sdata, ndatalen);
	else				stemp2.insert(0, sdata, (int)(strlen(sdata)) );
	stemp1.make_lower();
	stemp2.make_lower();
	return stemp1.compare(stemp2);
}

int CNString::icompare(const CNString& sdata) const
{
	CNString stemp1, stemp2;
	stemp1.insert(0, m_sdata, m_nlen);
	stemp2.insert(0, sdata.c_str(), sdata.get_length());
	stemp1.make_lower();
	stemp2.make_lower();
	return stemp1.compare(stemp2);
}

// * ============================================================== *
// @ 功能描述: 读取文件数据
// * ============================================================== *
int CNString::readfile(const char *sfilmame, const int nperread)
{
	m_nlen = 0;
	FILE* file = fopen(sfilmame, "rb+");
	if(!file)			return -2;
	if(file)
	{
		char* sread = new char[nperread];
		while(1)
		{
			int nread = ((int)(fread(sread, 1, nperread, file)));
			if(nread <= 0)	break;
			insert(m_nlen, sread, nread);
		}
		delete [] sread;
		fclose(file);
	}
	return 0;
}

// * ============================================================== *
// @ 功能描述: 写入文件数据
// * ============================================================== *
int CNString::writefile(const char* sfilename) const
{
	FILE* file = fopen(sfilename, "wb");
	if(!file)		return -1;
	fwrite(m_sdata, 1, m_nlen, file);
	fclose(file);
	return 0;
}

// * ============================================================== *
// @ 功能描述: 递增写入文件数据
// * ============================================================== *
int CNString::writefile_append(const char* sfilename) const
{
	FILE* file = fopen(sfilename, "ab");
	if(!file)	return -1;
	fwrite(m_sdata, 1, m_nlen, file);
	fclose(file);
	return 0;
}

// * ============================================================== *
// @ 功能描述: 读取一行
// * ============================================================== *
int CNString::read_line(CNString &sline, int& i_start) const 
{
	sline.m_nlen = 0;
	while(i_start < m_nlen && (m_sdata[i_start] == '\r' || m_sdata[i_start] == '\n'))
	{
		i_start++;
	}
	while(i_start < m_nlen && m_sdata[i_start] != '\r' && m_sdata[i_start] != '\n')
	{
		sline += m_sdata[i_start++];
	}
	return i_start;
}

// * ============================================================== *
// @ 功能描述: 读取参数
// * ============================================================== *
int CNString::read_para(CNString& s_para, int& i_start, const char c_inter) const
{
	s_para.m_nlen = 0;
	while(i_start < m_nlen && (m_sdata[i_start] == c_inter))
	{
		i_start++;
	}
	while(i_start < m_nlen && m_sdata[i_start] != c_inter)
	{
		s_para += m_sdata[i_start++];
	}
	return i_start;
}

// * ============================================================== *
// @ 功能描述: 去头操作 -- 左
// * ============================================================== *
int CNString::trim_left(CNString& fetchstr, const char* sinclude, const int slen)
{
	int nlen = slen;
	if(nlen <= 0)	nlen = (int)(strlen(sinclude));
	int i, j;
	for(i = 0; i < m_nlen; i++)
	{
		for(j = 0; j < nlen; j++)
		{
			if(m_sdata[i] == sinclude[j])
			{
				break;
			}
		}
		if(j == nlen)	break;
	}
	fetchstr.empty();
	fetchstr.insert(0, m_sdata, i);
	dellen(0, i);
	return 0;
}

int CNString::trim_left(const char *sinclude, const int slen)
{
	CNString fetchstr;
	trim_left(fetchstr, sinclude, slen);
	return 0;
}

// * ============================================================== *
// @ 功能描述: 去尾操作 -- 右
// * ============================================================== *
int CNString::trim_right(CNString& fetchstr, const char* sinclude, const int slen)
{
	int nlen = slen;
	if(nlen <= 0)	nlen = (int)(strlen(sinclude));
	int i, j;
	for(i = m_nlen-1; i > 0; i--)
	{
		for(j = 0; j < nlen; j++)
		{
			if(m_sdata[i] == sinclude[j])
			{
				break;
			}
		}
		if(j == nlen)	break;
	}
	fetchstr.empty();
	fetchstr.insert(0, m_sdata+i, m_nlen-i+1);
	dellen(i, m_nlen-i+1);
	return 0;
}

// * ============================================================== *
// @ 功能描述: 去尾操作 -- 右
// * ============================================================== *
int CNString::trim_right(const char* sinclude, const int slen)
{
	CNString result;
	trim_right(result, sinclude, slen);
	return 0;
}

// * ============================================================== *
// @ 功能描述: 带分割符的函数操作
// * ============================================================== *
const char* CNString::fetch_haveendchar(char* s_result, const int i_maxlen, const char c_end)
{
	s_result[0] = 0;
	if(m_nlen <= 0)	return s_result;

	int i_end = find(0, c_end);
	if(i_end <= 0)	i_end = m_nlen;

	if(i_end > i_maxlen)	i_end = (i_maxlen-1);
	memcpy(s_result, m_sdata, i_end);
	s_result[i_end] = 0;
	if(i_end == m_nlen)		m_nlen = 0;
	else					dellen(0, i_end+1);
	return s_result;
}
