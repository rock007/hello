/*
//功能：
  1，读取配置文件（每一行中以' '或'\t'分隔参数）
  2，数据库SELECT返回结果集
  3，读取xml格式消息
  4，拆分字符串

//ConfigFile功能：
  1，以数组方式访问数据
  2，以行名称访问数据（区分大小写）（允许重复名称）

//ConfigLine功能：
  1，以数组方式访问数据
  2，以列名称访问数据（不区分大小写）（不允许重复名称）
  3，以>>号访问数据
*/

#ifndef __CONFIG_FILE_T_H__
#define __CONFIG_FILE_T_H__

#include <stdarg.h>
#include <string>
#include <iostream>
#include <fstream>
#include <strstream>
#include <vector>
#include <map>

#ifdef __WIN32__
#include <time.h>
#else
#include <iconv.h>
#endif

#include <errno.h>

using namespace std;

#include <sstream>
using std::cout;
using std::endl;
using std::string;
using std::ostringstream;


inline float ToFloat(const string & intstr)
{
	float f1 = 0;
	sscanf(intstr.c_str(), "%f", &f1);
	return f1;
}

inline int ToInt(const string & intstr)
{
	return atoi(intstr.c_str());
}

inline string FormatString(const char *fmt, ...)
{
	int n, size = 1024;
	va_list ap;
	char *p;
	if ((p = (char*)malloc (size)) == NULL) return "";
	
	while (1)
	{
		//Try to print in the allocated space
		va_start(ap, fmt);
		n = vsnprintf (p, size, fmt, ap);
		va_end(ap);
		
		//If that worked, return the string
		if (n > -1 && n < size)
		{
			string str(p);
			if(p) free(p);
			return str;
		}
		
		//Else try again with more space
		if (n > -1)     //glibc 2.1
			size = n+1; //precisely what is needed
		else            //glibc 2.0
			size *= 2;  //twice the old size
		
		//1. realloc失败的时候，返回NULL
		//2. realloc失败的时候，原来的内存不改变，也就是不free或不move，(这个地方很容易出错)
		//3. 假如原来的内存后面还有足够多剩余内存的话，realloc的内存=原来的内存+剩余内存,realloc还是返回原来内存的地址; 
		//   假如原来的内存后面没有足够多剩余内存的话，realloc将申请新的内存，然后把原来的内存数据拷贝到新内存里，
		//   原来的内存将被free掉,realloc返回新内存的地址
		//4. 如果size为0，效果等同于free()
		//5. 传递给realloc的指针必须是先前通过malloc(), calloc(), 或realloc()分配的
		//6. 传递给realloc的指针可以为空，等同于malloc
		char * newp = (char *) realloc (p, size);
		if ( newp == NULL )
		{
			if(p) free(p);
			return "";
		}
		else
		{
			p = newp;
		}
	}
}

template < class T >
inline string ToString(const T &arg)
{
	ostringstream out;
	out << arg;
	return (out.str());
}

//返回某天凌晨零点对应的time_t时间
inline time_t day_0hour0min0sec_time_t(time_t t1)
{
	struct tm tm1;
	memcpy(&tm1, localtime(&t1), sizeof(struct tm));
	
	tm1.tm_hour = 0;
	tm1.tm_min = 0;
	tm1.tm_sec = 0;
	
	return mktime(&tm1);
}

//返回某个时间点从当天凌晨零点以来的秒数
inline int seconds_from_day_0hour0min0sec(time_t t1)
{
	struct tm tm1;
	memcpy(&tm1, localtime(&t1), sizeof(struct tm));
	return ( tm1.tm_hour*3600 + tm1.tm_min*60 + tm1.tm_sec );
}

inline time_t time_string_to_time_t(const string & timestr)
{
	//20100104152755
	if ( timestr.length() == 14 )
	{
		struct tm tm1 = {0};
		tm1.tm_year = ToInt( timestr.substr(0,4) ) - 1900;
		tm1.tm_mon  = ToInt( timestr.substr(4,2) ) - 1;
		tm1.tm_mday = ToInt( timestr.substr(6,2) );
		tm1.tm_hour = ToInt( timestr.substr(8,2) );
		tm1.tm_min  = ToInt( timestr.substr(10,2) );
		tm1.tm_sec  = ToInt( timestr.substr(12,2) );
		
		time_t t1 = mktime(&tm1);
		return t1;
	}
	
	//2010-04-18 22:22:22
	if ( timestr.length() == 19 )
	{
		struct tm tm1 = {0};
		tm1.tm_year = ToInt( timestr.substr(0,4) ) - 1900;
		tm1.tm_mon  = ToInt( timestr.substr(5,2) ) - 1;
		tm1.tm_mday = ToInt( timestr.substr(8,2) );
		tm1.tm_hour = ToInt( timestr.substr(11,2) );
		tm1.tm_min  = ToInt( timestr.substr(14,2) );
		tm1.tm_sec  = ToInt( timestr.substr(17,2) );
		
		time_t t1 = mktime(&tm1);
		return t1;
	}
	
	return -1;
}

inline string time_format_str(time_t t1, int flag_format = 0)
{
	char buf[32] = {0};
	
	if ( t1 )
	{
		switch ( flag_format )
		{
			case 0:
				strftime(buf, 32, "%Y-%m-%d %H:%M:%S", localtime(&t1));
				break;
			case 1:
				strftime(buf, 32, "%Y%m%d%H%M%S", localtime(&t1));
				break;
			case 2:
				strftime(buf, 32, "%Y_%m_%d_%H_%M_%S", localtime(&t1));
				break;
			case 3:
				strftime(buf, 32, "%Y-%m-%d-%H-%M-%S", localtime(&t1));
				break;
			case 4:
				strftime(buf, 32, "%Y/%m/%d %H:%M:%S", localtime(&t1));
				break;
			case 5:
				strftime(buf, 32, "%Y%m%d %H%M%S", localtime(&t1));
				break;
			
			case 10:
				strftime(buf, 32, "%Y-%m-%d", localtime(&t1));
				break;
			case 11:
				strftime(buf, 32, "%Y%m%d", localtime(&t1));
				break;
			case 12:
				strftime(buf, 32, "%Y_%m_%d", localtime(&t1));
				break;
			case 13:
				strftime(buf, 32, "%Y-%m-%d", localtime(&t1));
				break;
			case 14:
				strftime(buf, 32, "%Y/%m/%d", localtime(&t1));
				break;
			
			case 20:
				strftime(buf, 32, "%Y-%m", localtime(&t1));
				break;
			case 21:
				strftime(buf, 32, "%Y%m", localtime(&t1));
				break;
			case 22:
				strftime(buf, 32, "%Y_%m", localtime(&t1));
				break;
			case 23:
				strftime(buf, 32, "%Y-%m", localtime(&t1));
				break;
			case 24:
				strftime(buf, 32, "%Y/%m", localtime(&t1));
				break;
			
			case 30:
				strftime(buf, 32, "%H:%M:%S", localtime(&t1));
				break;
			case 31:
				strftime(buf, 32, "%H%M%S", localtime(&t1));
				break;
			case 32:
				strftime(buf, 32, "%H_%M_%S", localtime(&t1));
				break;
			case 33:
				strftime(buf, 32, "%H-%M-%S", localtime(&t1));
				break;
			case 34:
				strftime(buf, 32, "%H:%M:%S", localtime(&t1));
				break;
			
			case 40:
				strftime(buf, 32, "%H:%M", localtime(&t1));
				break;
			case 41:
				strftime(buf, 32, "%H%M", localtime(&t1));
				break;
			case 42:
				strftime(buf, 32, "%H_%M", localtime(&t1));
				break;
			case 43:
				strftime(buf, 32, "%H-%M", localtime(&t1));
				break;
			
			default:
				break;
		}
	}
	
	string s = buf;
	return s;
}

inline string string_tolower( const string & src )
{
	string dest = src;
	use_facet< ctype<char> >( locale() ).tolower( &(*dest.begin()), &(*dest.end()) );
	return dest;
}

inline string string_toupper( const string & src )
{
	string dest = src;
	use_facet< ctype<char> >( locale() ).toupper( &(*dest.begin()), &(*dest.end()) );
	return dest;
}

inline int string_replace(string & strBig, const string & strsrc, const string & strdst)
{
	if ( strsrc.size() == 0 ) return 0;
	
	int count = 0;
	string::size_type pos=0;
	string::size_type srclen = strsrc.size();
	string::size_type dstlen = strdst.size();
	
	while ( (pos = strBig.find(strsrc, pos)) != string::npos )
	{
		strBig.replace(pos, srclen, strdst);
		pos += dstlen;
		count++;
	}
	
	return count;
}

inline int xml_get_para_string(const string & buf, const string & str1, const string & str2, string & para)
{
	string::size_type first = buf.find(str1, 0);
	if ( first == string::npos ) return 0;
	
	first += str1.size();
	
	string::size_type last = buf.find(str2, first);
	if ( last == string::npos ) return 0;
	
	para = buf.substr(first, last - first);
	return 1;
}


inline string xml_get_para(const string & buf, const string & str1, const string & str2)
{
	string::size_type first = buf.find(str1, 0);
	if ( first == string::npos ) return "";
	
	first += str1.size();
	
	string::size_type last = buf.find(str2, first);
	if ( last == string::npos ) return "";
	
	string para = buf.substr(first, last - first);
	return para;
}


inline int xml_put_para(string & buf, const string & key, int para)
{
	char parabuf[32] = {0};
	sprintf(parabuf, "%d", para);
	
	buf += "<" + key + ">";
	buf += parabuf;
	buf += "</" + key + ">";
	
	//修改消息头部的长度参数
	char lenbuf[16] = {0};
	sprintf(lenbuf, "%05d", buf.length()-18);
	buf.replace(6, 5, lenbuf);
	
	return 1;
}

inline int xml_put_para(string & buf, const string & key, double para)
{
	char parabuf[32] = {0};
	sprintf(parabuf, "%.3f", para);
	
	buf += "<" + key + ">";
	buf += parabuf;
	buf += "</" + key + ">";
	
	//修改消息头部的长度参数
	char lenbuf[16] = {0};
	sprintf(lenbuf, "%05d", buf.length()-18);
	buf.replace(6, 5, lenbuf);
	
	return 1;
}

inline int xml_put_para(string & buf, const string & key, const string & para)
{
	buf += "<" + key + ">" + para + "</" + key + ">";
	
	//修改消息头部的长度参数
	char lenbuf[16] = {0};
	sprintf(lenbuf, "%05d", buf.length()-18);
	buf.replace(6, 5, lenbuf);
	
	return 1;
}

inline int xml_put_para(string & buf, const string & para)
{
	buf += para;
	
	//修改消息头部的长度参数
	char lenbuf[16] = {0};
	sprintf(lenbuf, "%05d", buf.length()-18);
	buf.replace(6, 5, lenbuf);
	
	return 1;
}


#ifdef __WIN32__

#else
inline string iconv_convert(const string & fromcs, const string & tocs, const string & atext)
{
	iconv_t cd = iconv_open(tocs.c_str(), fromcs.c_str());
	if ( cd == (iconv_t)(-1) )
	{
		perror("iconv_open() error : ");
		return atext;
	}
	
	string r, text(atext);
	size_t inleft, outleft, soutleft;
	char *inbuf, *outbuf, *sinbuf, *soutbuf;
	
	//from iconv.c (libiconv)
	iconv(cd,NULL,NULL,NULL,NULL);
	
	while ( !text.empty() )
	{
		sinbuf = inbuf = strdup(text.c_str());
		inleft = strlen(inbuf);
		
		soutleft = outleft = inleft*4;
		soutbuf = outbuf = new char[outleft];
		
		#ifdef __SunOS__
		size_t res = iconv(cd, &((const char *)inbuf), &inleft, &outbuf, &outleft);
		#else
		size_t res = iconv(cd, &inbuf, &inleft, &outbuf, &outleft);
		#endif
		
		soutbuf[soutleft-outleft] = 0;
		r += soutbuf;
		text.erase(0, text.size()-inleft);
		
		delete soutbuf;
		delete sinbuf;
		
		if ( res == (size_t)-1 && errno != EILSEQ ) break;
		
		if ( !text.empty() )
		{
			text.erase(0, 1);
			r += " ";
		}
	}
	
	iconv_close(cd);
	return r;
}
#endif


class ConfigFile;


class ConfigLine
{
private:
	vector<string> Columns;		//以动态数组方式存放value，可通过下标来访问
	map<string,int> ColumnsName;//以map方式存放key-index的对应关系，先通过key得到下标，再通过下标可以得到value
	string null;
	string nullstring;
	int curloc;
	ConfigFile * father;
	
public:
	ConfigLine(ConfigFile * pf = NULL)
	{
		curloc = 0;
		father = pf;
	};
	
	~ConfigLine()
	{
		clear();
	};
	
	//读取xml消息如<head>00059</head><msg>outcallresp</msg><State>5</State><Caller>6102</Caller>
	int read_xml_msg(const string & msg)
	{
		string buf = msg;
		
		while ( buf.size() )
		{
			string var_name = "";
			if ( xml_get_para_string(buf, "<", ">", var_name) == 0 ) break;
			
			string begin = "<" + var_name + ">";
			string end = "</" + var_name + ">";
			
			string var_value = "";
			if ( xml_get_para_string(buf, begin, end, var_value) == 0 ) break;
			
			push_field_name(var_name);
			push_field_value(var_value);
			
			buf = buf.substr(buf.find(end) + end.length(), string::npos);
		}
		
		return 1;
	};
	
	//读配置文件，每行中有以' '和'\t'分隔的多个参数
	int readline(const string & buf, const string & separator)
	{
		unsigned int first = 0;
		unsigned int last = 0;
		
		while ( 1 )
		{
			first = buf.find_first_not_of(separator, first);
			if ( first == string::npos ) break;
			
			last = buf.find_first_of(separator, first);
			if ( last == string::npos )
			{
				Columns.push_back(buf.substr(first, string::npos));
				break;
			}
			
			Columns.push_back(buf.substr(first, last-first));
			
			first = last;
		}
		
		return 1;
	};
	
	int size()
	{
		return Columns.size();
	};
	
	int capacity()
	{
		return Columns.capacity();
	};
	
	void resize(int count)
	{
		return Columns.resize(count);
	};
	
	void clear()
	{
		Columns.clear();
		ColumnsName.clear();
		curloc = 0;
	};
	
	void push_back(const string & col)
	{
		return Columns.push_back(col);
	};
	
	const string & operator[](int index)
	{
		if ( index < 0 || index >= (int)Columns.size() ) return null;
		return Columns[index];
	};
	
	const string & operator[](const string & fieldname)
	{
		int index = search_field_name(fieldname);
		if ( index < 0 || index >= (int)Columns.size() ) return null;
		return Columns[index];
	};
	
	ConfigLine & operator>>(char * s)
	{
		if ( curloc < 0 || curloc >= (int)Columns.size() ) return *this;
		
		strcpy(s, Columns[curloc].c_str());
		
		curloc++;
		if ( curloc >= (int)Columns.size() ) curloc = 0;
		
		return *this;
	};
	
	ConfigLine & operator>>(unsigned char * s)
	{
		if ( curloc < 0 || curloc >= (int)Columns.size() ) return *this;
		
		memcpy(s, Columns[curloc].c_str(), Columns[curloc].size()+1);
		
		curloc++;
		if ( curloc >= (int)Columns.size() ) curloc = 0;
		
		return *this;
	};
	
	ConfigLine & operator>>(int & i)
	{
		if ( curloc < 0 || curloc >= (int)Columns.size() ) return *this;
		
		i = ToInt(Columns[curloc]);
		
		curloc++;
		if ( curloc >= (int)Columns.size() ) curloc = 0;
		
		return *this;
	};
	
	ConfigLine & operator>>(unsigned int & i)
	{
		if ( curloc < 0 || curloc >= (int)Columns.size() ) return *this;
		
		i = ToInt(Columns[curloc]);
		
		curloc++;
		if ( curloc >= (int)Columns.size() ) curloc = 0;
		
		return *this;
	};
	
	ConfigLine & operator>>(string & s)
	{
		if ( curloc < 0 || curloc >= (int)Columns.size() ) return *this;
		
		s = Columns[curloc];
		
		curloc++;
		if ( curloc >= (int)Columns.size() ) curloc = 0;
		
		return *this;
	};
	
	void push_field_value(const string & fieldvalue)
	{
		return Columns.push_back(fieldvalue);
	};
	
	void push_field_name(const string & fieldname)
	{
		int index = ColumnsName.size();
		ColumnsName.insert(map<string,int>::value_type(string_toupper(fieldname), index));
		return;
	};
	
	int search_field_name(const string & fieldname);
	
	const string & get_field_name(int index);
};


class ConfigFile
{
private:
	vector<ConfigLine> Lines;
	map<string,int> LinesName;	//行名，int表示行的下标
	map<string,int> ColumnsName;//列名，int表示列的下标
	ConfigLine null;
	string nullstring;
	
public:
	ConfigFile()
	{
		
	};
	
	ConfigFile(const char * pinipath, const char * separator = " \t")
	{
		readfile(pinipath, separator);
	};
	
	~ConfigFile()
	{
		clear();
	};
	
	//读配置文件
	int readfile(const char * pinipath, const char * separator = " \t")
	{
		clear();
		
		ifstream fin(pinipath);
		if ( !fin.is_open() ) return 0;
		
		while ( !fin.eof() )
		{
			string inbuf;
			getline(fin, inbuf, '\n');
			
			if ( inbuf.length() >= 1 && inbuf[inbuf.length()-1] == '\r' ) inbuf.erase(inbuf.length()-1, 1);
			if ( inbuf.length() >= 1 && inbuf[inbuf.length()-1] == '\r' ) inbuf.erase(inbuf.length()-1, 1);
			
			ConfigLine line;
			line.readline(inbuf, separator);	//以空格或者\t分开的字符串
			
			if ( !line[0].empty() ) push_line_name(line[0], Lines.size(), get_repeat_key_count(line[0]));
			Lines.push_back(line);
		}
		
		fin.close();
		
		if ( Lines.empty() ) return 0;
		
		return 1;
	};
	
	//读类似SDP协议的多行文本
	int read_txt(const string & txt, const char * separator = " \t")
	{
		clear();
		
		string buf = txt;
		unsigned int first = 0;
		
		while ( (first = buf.find("\n")) != string::npos )
		{
			string inbuf = buf.substr(0, first);
			
			if ( inbuf.length() >= 1 && inbuf[inbuf.length()-1] == '\r' ) inbuf.erase(inbuf.length()-1, 1);
			if ( inbuf.length() >= 1 && inbuf[inbuf.length()-1] == '\r' ) inbuf.erase(inbuf.length()-1, 1);
			
			ConfigLine line;
			line.readline(inbuf, separator);	//以空格或者\t分开的字符串
			
			if ( !line[0].empty() ) push_line_name(line[0], Lines.size(), get_repeat_key_count(line[0]));
			Lines.push_back(line);
			
			buf.erase(0, first+1);
		}
		
		//buf中可能剩下最后一行没有以'\n'结尾的
		if ( buf.size() )
		{
			string inbuf = buf;
			
			if ( inbuf.length() >= 1 && inbuf[inbuf.length()-1] == '\r' ) inbuf.erase(inbuf.length()-1, 1);
			if ( inbuf.length() >= 1 && inbuf[inbuf.length()-1] == '\r' ) inbuf.erase(inbuf.length()-1, 1);
			
			ConfigLine line;
			line.readline(inbuf, separator);	//以空格或者\t分开的字符串
			
			if ( !line[0].empty() ) push_line_name(line[0], Lines.size(), get_repeat_key_count(line[0]));
			Lines.push_back(line);
		}
		
		if ( Lines.empty() ) return 0;
		
		return 1;
	};
	
	//buf like "string,p1,in,abc;int,p2,out,null;int,p3,inout,5"
	//分号表示行，逗号表示列
	int read_string(const string & buf, const string & separator_line, const string & separator_column)
	{
		clear();
		
		ConfigLine l1;
		l1.readline(buf, separator_line);
		
		for ( int i=0; i<l1.size(); i++ )
		{
			ConfigLine line;
			line.readline(l1[i], separator_column);
			
			Lines.push_back(line);
		}
		
		if ( Lines.empty() ) return 0;
		
		return 1;
	};
	
	void push_line_name(const string & linename, int index, int keycount = 0)
	{
		string key = linename + ":" + ToString(keycount);
		LinesName.insert(map<string,int>::value_type(key, index));
		return;
	};
	
	int search_line_name(const string & linename, int keycount = 0)
	{
		//ConfigFile类中是否有行名称的信息
		if ( !LinesName.empty() )
		{
			string key = linename + ":" + ToString(keycount);
			map<string,int>::iterator it = LinesName.find(key);
			if ( it != LinesName.end() )
			{
				return it->second;
			}
			else
			{
				return -1;
			}
		}
		
		return -1;
	};
	
	int get_repeat_key_count(const string & linename)
	{
		int keycount = 0;
		while ( 1 )
		{
			if ( search_line_name(linename, keycount) < 0 ) break;
			keycount++;
		}
		return keycount;
	};
	
	void push_field_name(const string & fieldname)
	{
		int index = ColumnsName.size();
		ColumnsName.insert(map<string,int>::value_type(string_toupper(fieldname), index));
		return;
	};
	
	int search_field_name(const string & fieldname)
	{
		//ConfigFile类中是否有列名称的信息
		if ( !ColumnsName.empty() )
		{
			map<string,int>::iterator it = ColumnsName.find(string_toupper(fieldname));
			if ( it != ColumnsName.end() )
			{
				return it->second;
			}
			else
			{
				return -1;
			}
		}
		
		return -1;
	};
	
	const string & get_field_name(int index)
	{
		//ConfigFile类中是否有列名称的信息
		if ( !ColumnsName.empty() )
		{
			if ( index < 0 || index >= (int)ColumnsName.size() ) return nullstring;
			
			for ( map<string,int>::iterator it = ColumnsName.begin(); it != ColumnsName.end(); it++ )
			{
				if ( it->second == index )
				{
					return it->first;
				}
			}
			
			return nullstring;
		}
		
		return nullstring;
	};
	
	int size()
	{
		return Lines.size();
	};
	
	int capacity()
	{
		return Lines.capacity();
	};
	
	void resize(int count)
	{
		return Lines.resize(count);
	};
	
	void push_back(const ConfigLine & line)
	{
		return Lines.push_back(line);
	};
	
	void clear()
	{
		Lines.clear();
		LinesName.clear();
		ColumnsName.clear();
	};
	
	vector<ConfigLine>::iterator begin()
	{
		return Lines.begin();
	};
	
	vector<ConfigLine>::iterator end()
	{
		return Lines.end();
	};
	
	ConfigLine & operator[](int index)
	{
		if ( index < 0 || index >= (int)Lines.size() ) return null;
		return Lines[index];
	};
	
	ConfigLine & operator[](const string & key)
	{
		int index = search_line_name(key);
		if ( index < 0 || index >= (int)Lines.size() ) return null;
		return Lines[index];
	};
	
	ConfigLine & find_repeat_key_line(const string & key, int keycount)
	{
		int index = search_line_name(key, keycount);
		if ( index < 0 || index >= (int)Lines.size() ) return null;
		return Lines[index];
	};
};


	inline int ConfigLine::search_field_name(const string & fieldname)
	{
		//ConfigLine类中是否有列名称的信息
		if ( !ColumnsName.empty() )
		{
			map<string,int>::iterator it = ColumnsName.find(string_toupper(fieldname));
			if ( it != ColumnsName.end() )
			{
				return it->second;
			}
			else
			{
				return -1;
			}
		}
		
		//ConfigFile类中是否有列名称的信息
		if ( father )
		{
			return father->search_field_name(fieldname);
		}
		
		return -1;
	};


	inline const string & ConfigLine::get_field_name(int index)
	{
		//ConfigLine类中是否有列名称的信息
		if ( !ColumnsName.empty() )
		{
			if ( index < 0 || index >= (int)ColumnsName.size() ) return nullstring;
			
			for ( map<string,int>::iterator it = ColumnsName.begin(); it != ColumnsName.end(); it++ )
			{
				if ( it->second == index )
				{
					return it->first;
				}
			}
			
			return nullstring;
		}
		
		//ConfigFile类中是否有列名称的信息
		if ( father )
		{
			return father->get_field_name(index);
		}
		
		return nullstring;
	};


typedef ConfigFile DATASET;


//2010-4-8 2:2:2 长度不固定
inline time_t time_string2_to_time_t(const string & timestr)
{
	if ( timestr.size() == 0 ) return -1;
	
	ConfigLine line;
	line.readline(timestr, "-: ");
	
	if ( line.size() != 6 ) return -1;
	
	struct tm tm1 = {0};
	tm1.tm_year = ToInt(line[0]) - 1900;
	tm1.tm_mon  = ToInt(line[1]) - 1;
	tm1.tm_mday = ToInt(line[2]);
	tm1.tm_hour = ToInt(line[3]);
	tm1.tm_min  = ToInt(line[4]);
	tm1.tm_sec  = ToInt(line[5]);
	
	time_t t1 = mktime(&tm1);
	return t1;
}

#ifdef __Linux__
inline string pid2name(pid_t pid)
{
	if ( (int)pid <= 0 ) return "PROCESS";
	
	string ff = "/proc/" + ToString(pid) + "/status";
	
	ConfigFile pidinfo;
	if ( pidinfo.readfile(ff.c_str()) == 0 ) return "PROCESS";
	
	return pidinfo["Name:"][1];
}
#endif

#endif

