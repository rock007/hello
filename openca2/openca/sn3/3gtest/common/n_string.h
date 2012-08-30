/* -*- head file -*-*/

#ifndef COMMON_CNSTRING_HEAD_DEFINE_
#define COMMON_CNSTRING_HEAD_DEFINE_

#include <string.h>
#include <stdlib.h>

#define DEF_STRING_ALLOCADD		16
/*========================================================================*/
/**
*  @ 类名       CNString
*  @ 主要功能 : 字符串类的定义
*  @ 修改纪录   
*/
/*========================================================================*/
class CNString
{
	// *- construction and deconstruction *-
public:
	CNString();
	CNString(const char data);
	CNString(const char* sdata);
	CNString(const CNString& sdata);
	virtual ~CNString();


public:
	const char* fetch_haveendchar(char* s_result, const int i_maxlen, const char c_end = ',');

	// *- basic operation -- space operation *-
public:
	// *- string length operation *-
	const int set_length(const int len);
	const int get_length() const;

	// *- capacity operation *-
	int set_capacity_size(const int capacity);
	const int get_capacity_size() const;

	// *- empty string *-
	int empty();
	int release_extra();

	// *- string or char operation *-
	char* c_str();
	const char* c_str() const;

	char& operator[](const int nIndex);
	const char& operator[](const int nIndex) const;

	// set the buffer value
	const CNString& operator=(const char sdata);
	const CNString& operator=(const char* sdata);
	const CNString& operator=(const CNString& sdata);

	// add buffer value
	const CNString& operator+=(const char sdata);
	const CNString& operator+=(const char* sdata);
	const CNString& operator+=(const CNString& sdata);

	// string format
	int Format(const char* sformat, ...);

	// *- basic operator --(string operation) *-
public:
	int insert(const int nindex, const char* lpstr, const int nlen);
	int dellen(const int nindex, const int nlen);
	int find(const int nstartpos, const char* strFind, const int nlen) const;
	int replace(const char* lpstrOld, const int nOldlen, const char* lpstrNew, const int nNewlen);

public:
	// find overlapped function
	int find(const int nstartpos, const char sdata) const;
	int find(const int nstartpos, const char* sdata) const;
	int find(const int nstartpos, const CNString& sdata) const;

	// insert overlapped function
	int insert(const int nindex, const char sdata);
	int insert(const int nindex, const char* sdata);
	int insert(const int nindex, const CNString& sdata);

	// replace overplapped function
	int replace(const char sori_data, const char srep_data);
	int replace(const char* sori_data, const char* srep_data);
	int replace(const char* sori_data, const char srep_data);
	int replace(const char sori_data, const char* srep_data);
	int replace(const CNString& sori_data, const CNString& srep_data);

	// append overlapped function
	int append(const char sdata);
	int append(const char* sdata);
	int append(const CNString& sdata);
	int append(const char* sdata, const int ndatalen);

	// upper all string
	void make_upper();
	// lower all string
	void make_lower();
	// convert network sequence
	void make_convert();

	// compare string
	int compare(const char* sdata, const int ndatalen=-1) const;
	int compare(const CNString& sdata) const;
	int icompare(const char* sdata, const int ndatalen=-1) const;
	int icompare(const CNString& sdata) const;

	// file operation 
	int readfile(const char *sfilmame, const int nperread=256);
	int writefile(const char* sfilename) const;
	int writefile_append(const char* sfilename) const;

	// read line from position
	int read_line(CNString &sline, int& i_start) const;
	int read_para(CNString& s_para, int& i_start, const char c_inter) const;

	// trim operation
	int trim_left(const char *sinclude, const int nlen = 0);
	int trim_left(CNString& fetchstr, const char* sinclude, const int nlen=0);
	int trim_right(const char* sinclude, const int nlen = 0);
	int trim_right(CNString& fetchstr, const char* sinclude, const int nlen=0);

	int to_uint32() const { return strtoul(m_sdata, NULL, 10); }
	int to_int32() const { return strtol(m_sdata, NULL, 10); }
	double to_float() const { return strtod(m_sdata, NULL); }
protected:
	void kmp_getnextval(const char* ssubstr, const int nlen, int* pnext) const;
	char* m_sdata;		// the buffer point
	int m_nlen;			// len of string
	int m_capacity;		// capacity
};

inline bool operator==(const CNString& s1, const char* s2)
{
	if(!strcmp(s1.c_str(), s2))		return true;
	return false;
}

inline bool operator==(const char* s1, const CNString& s2)
{
	if(!strcmp(s1, s2.c_str()))		return true;
	return false;
}

inline bool operator==(const CNString& s1, const CNString& s2)
{
	if(s1.get_length() != s2.get_length())	return false;
	for(int i=0; i < s1.get_length(); i++)
	{
		if((s1.c_str())[i] != (s2.c_str())[i])
			return false;
	}
	return true;
}

inline bool operator!=(const CNString& s1, const char* s2)
{
	if(!strcmp(s1.c_str(), s2))		return false;
	return true;
}

inline bool operator!=(const char* s1, const CNString& s2)
{
	if(!strcmp(s1, s2.c_str()))		return false;
	return true;
}

inline bool operator!=(const CNString& s1, const CNString& s2)
{
	return !(s1==s2);
}

#endif
