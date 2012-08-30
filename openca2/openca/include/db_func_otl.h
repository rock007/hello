/*
OTL supports UTF-8 and UTF-16 for Oracle, and UTF-16 for the rest of the database types, 
when the underlying database API / ODBC driver supports it.

DB2数据库使用UTF-8时，linux的shell必须设置成：
LANG="en_US.UTF-8"
SUPPORTED="zh_CN.UTF-8:zh_CN:zh:en_US.UTF-8:en_US:en"
这样select出来的字符串是乱码，经过code_convert()函数后，转换为正常的字符串

如果shell中设置为LANG=C，这样select出来的字符串中的中文都没有了
*/

/*
DB2数据库需要在insert/update/delete后面做commit，否则X锁会导致其他程序不能读表
*/

#ifndef __DB_FUNC_OTL_H__
#define __DB_FUNC_OTL_H__

#define OTL_STL
#include <otlv4.h> // include the OTL 4 header file

#ifdef __WIN32__
#include "mutex_win.h"
#else
#include <boost/thread/recursive_mutex.hpp>
#endif

#include "config_file_t.h"
#include "logfile.h"

#define dblog if ( plog ) plog->logwrite

//return > 1, select return multi records, or insert/update/delete affect multi records
//return = 1, select return 1 record, or insert/update/delete affect 1 record
//return = 0, select return 0 record, or insert/update/delete affect 0 record
//return < 0, fail

class OTLDB
{
private:
	otl_connect db; // connect object
	string host;
	string databasename;
	string username;
	string password;
	int opened;
	LogFile * plog;
	string character_set;	//"latin1", "gbk", "utf8"
	
	boost::recursive_mutex mutex;
	
public:
	OTLDB()
	{
		opened = 0;
		plog = NULL;
		character_set = "latin1";
	}
	
	
	void db_log(LogFile * plog1)
	{
		plog = plog1;
	}
	
	
	int db_set_character_set(const string & c1)
	{
		character_set = c1;
		return 1;
	}
	
	
	int db_open(const string & dbname, const string & ip, const string & uname, const string & pass)
	{
		host = ip;
		databasename = dbname;
		username = uname;
		password = pass;
		
		otl_connect::otl_initialize(); // initialize DB2-CLI environment
		try
		{
			#ifdef _USE_DB2_
			printf("Pid=%d: OTL: try to connect db2 '%s'\n", getpid(), databasename.c_str());
			dblog(LOG1, "Pid=%d: OTL: try to connect db2 '%s'\n", getpid(), databasename.c_str());
			string l = "UID=" + username + ";PWD=" + password + ";DSN=" + databasename;
			db.rlogon(l.c_str()); // connect to DB2
			#endif
			
			#ifdef _USE_ORACLE_
			printf("Pid=%d: OTL: try to connect oracle '%s'\n", getpid(), databasename.c_str());
			dblog(LOG1, "Pid=%d: OTL: try to connect oracle '%s'\n", getpid(), databasename.c_str());
			string l = username + "/" + password + "@" + databasename;
			db.rlogon(l.c_str()); // connect to ORACLE
			#endif
		}
		catch(otl_exception& p) // intercept OTL exceptions
		{
			//cerr<<p.msg<<endl; // print out error message
			//cerr<<p.stm_text<<endl; // print out SQL that caused the error
			//cerr<<p.sqlstate<<endl; // print out SQLSTATE message
			//cerr<<p.var_info<<endl; // print out the variable that caused the error
			printf("Pid=%d: OTL: connect to db '%s' error: %s\n", getpid(), databasename.c_str(), p.msg);
			dblog(LOG1, "Pid=%d: OTL: connect to db '%s' error: %s\n", getpid(), databasename.c_str(), p.msg);
			opened = 0;
			return -1;
		}
		
		printf("Pid=%d: OTL: connect to db '%s' ok\n", getpid(), databasename.c_str());
		dblog(LOG1, "Pid=%d: OTL: connect to db '%s' ok\n", getpid(), databasename.c_str());
		opened = 1;
		return 1;
	}
	
	
	int db_close()
	{
		if ( opened )
		{
			db.logoff(); // disconnect from DB2
			printf("OTL: disconnect from db '%s' done\n", databasename.c_str());
			dblog(LOG1, "OTL: disconnect from db '%s' done\n", databasename.c_str());
		}
		opened = 0;
		return 1;
	}
	
	
	int db_select(const string & sql, DATASET & dbresult, int loglevel = 0, int logline = 0, char * logsourcefile = NULL, unsigned int column_types_mask = otl_all_num2str|otl_all_date2str)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		dbresult.clear();
		
		int flag_reconnect = 0;
		
		if ( opened == 0 )
		{
			if ( db_open(databasename, host, username, password) < 0 )
			{
				//re-connect failed
				dblog(LOG1, "OTL: SQL \"%s\" error: '%s'\n", sql.c_str(), "Can't connect to DB server");
				return -1;
			}
			else
			{
				//re-connect ok
				flag_reconnect = 1;
			}
		}
		
		try
		{
			//otl_stream i(50, sql.c_str(), db);
			
			otl_stream i;
			i.set_all_column_types(column_types_mask);
			i.open(50, sql.c_str(), db);
			
			int desc_len = 0;
			otl_column_desc * des2 = i.describe_select(desc_len);
			
			int recordcount = 0;
			
			while ( !i.eof() ) // while not end-of-data
			{
				ConfigLine line(&dbresult);
				
				for ( int kk=0; kk<desc_len; kk++ )
				{
					string value;
					
					if ( des2 == NULL ) break;
					
					switch ( des2[kk].otl_var_dbtype )
					{
						case 1: //char[]
						{
							i >> value;
							
							if ( character_set == "utf8" )
							{
								value = iconv_convert("UTF-8", "GB2312", value);
							}
							
							break;
						}
						case 2: //double
						{
							double f1 = 0;
							i >> f1;
							value = ToString(f1);
							break;
						}
						case 3: //float
						{
							float f1 = 0;
							i >> f1;
							value = ToString(f1);
							break;
						}
						case 4: //int
						{
							int f1 = 0;
							i >> f1;
							value = ToString(f1);
							break;
						}
						case 5: //unsigned int
						{
							unsigned int f1 = 0;
							i >> f1;
							value = ToString(f1);
							break;
						}
						case 6: //short
						{
							short f1 = 0;
							i >> f1;
							value = ToString(f1);
							break;
						}
						case 7: //otl_var_long_int
						{
							int f1 = 0;
							i >> f1;
							value = ToString(f1);
							break;
						}
						case 9:  //otl_var_varchar_long
						case 10: //otl_var_raw_long
						{
							long f1 = 0;
							i >> f1;
							char temp[64] = {0};
							sprintf(temp, "%ld", f1);
							value = temp;
							break;
						}
						case 8:  //otl_var_timestamp	//Oracle date/timestamp, DB2 timestamp, MS SQL datetime/datetime2/time/date, Sybase timestamp, etc.
						case 16: //otl_var_db2time		//DB2 TIME
						case 17: //otl_var_db2date		//DB2 DATE
						case 18: //otl_var_tz_timestamp	//Oracle timestamp with timezone type
						case 19: //otl_var_ltz_timestamp//Oracle 9i/10g/11g TIMESTAMP WITH LOCAL TIME ZONE type
						{
							otl_datetime f1;
							i >> f1;
							char temp[64] = {0};
							sprintf(temp, "%04d-%02d-%02d %02d:%02d:%02d", f1.year, f1.month, f1.day, f1.hour, f1.minute, f1.second);
							value = temp;
							break;
						}
						default:
						{
							dblog(LOG1, "OTL: otl_var_dbtype = %d, unknown, error\n", des2[kk].otl_var_dbtype);
							i >> value;
							break;
						}
					}
					
					//cout << "field name is " << des2[kk].name << ", field type is " << des2[kk].otl_var_dbtype << ", column length is " << des2[kk].dbsize << ", value is " << value << endl;
					
					if ( recordcount == 0 ) dbresult.push_field_name(des2[kk].name);
					line.push_field_value(value);
					
				}//列
				
				if ( !line[0].empty() ) dbresult.push_line_name(line[0], dbresult.size(), dbresult.get_repeat_key_count(line[0]));
				dbresult.push_back(line);
				recordcount++;
				
			}//行
		}
		catch(otl_exception& p) // intercept OTL exceptions
		{
			//cerr<<p.msg<<endl; // print out error message
			//cerr<<p.stm_text<<endl; // print out SQL that caused the error
			//cerr<<p.sqlstate<<endl; // print out SQLSTATE message
			//cerr<<p.var_info<<endl; // print out the variable that caused the error
			dblog(LOG1, "OTL: SQL \"%s\" error=%d: %s\n", p.stm_text, p.code, p.msg);
			
			//-30081: [IBM][CLI Driver] SQL30081N  A communication error has been detected.  Communication protocol being used: "TCP/IP".  Communication API being used: "SOCKETS".  Location where the error was detected: "192.168.1.159".  Communication function detecting the error: "recv".  Protocol specific error code(s): "*", "*", "0".  SQLSTATE=08001
			if ( p.code == -30081 )
			{
				db_close();
				
				//数据库刚刚重连成功时，sql失败，不能递归调用，否则引起死循环
				if ( flag_reconnect == 0 )
				{
					return db_select(sql, dbresult, loglevel, logline, logsourcefile, column_types_mask);
				}
			}
			
			return -1;
		}
		
		if ( logsourcefile )
		{
			dblog(loglevel, logline, logsourcefile, "OTL: SQL \"%s\" return %d records\n", sql.c_str(), dbresult.size());
			for ( int j=0; j<dbresult.size(); j++ )
			{
				string dis;
				for ( int k=0; k<dbresult[j].size(); k++ )
				{
					if ( k != 0 ) dis += ", ";
					dis += "[" + dbresult[j].get_field_name(k) + "]=" + dbresult[j][k];
				}
				dblog(loglevel, logline, logsourcefile, "  %d : %s\n", j, dis.c_str());
			}
		}
		
		return dbresult.size();
	}
	
	
	//paralist like "char[31],p1,in,abc;int,p2,out,0;int,p3,inout,5"
	int db_stored_procedure(const string & procedure_name, const string & paralist, DATASET & dbresult, int loglevel = 0, int logline = 0, char * logsourcefile = NULL)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		dbresult.clear();
		
		int flag_reconnect = 0;
		
		if ( opened == 0 )
		{
			if ( db_open(databasename, host, username, password) < 0 )
			{
				//re-connect failed
				dblog(LOG1, "OTL: db_stored_procedure() name='%s', paralist='%s', error: '%s'\n", procedure_name.c_str(), paralist.c_str(), "Can't connect to DB server");
				return -1;
			}
			else
			{
				//re-connect ok
				flag_reconnect = 1;
			}
		}
		
		string sql;
		
		if ( logsourcefile ) dblog(loglevel, logline, logsourcefile, "DB: db_stored_procedure() name='%s', paralist='%s'\n", procedure_name.c_str(), paralist.c_str());
		
		
		DATASET para;
		para.read_string(paralist, ";", ",");
		
		sql.clear();
		sql = "BEGIN " + procedure_name + "(";
		for ( int j=0; j<para.size(); j++ )
		{
			if ( j ) sql += ", ";
			
			if ( para[j][2] == "in" || para[j][2] == "out" || para[j][2] == "inout" )
			{
				sql += ":" + para[j][1] + "<" + para[j][0] + "," + para[j][2] + ">";
			}
			else
			{
				return -1;
			}
		}
		sql += ");END;";
		
		try
		{
			otl_stream i(1, sql.c_str(), db);
			
			for ( int j=0; j<para.size(); j++ )
			{
				if ( para[j][2] == "in" || para[j][2] == "inout" )
				{
					if ( strncmp(para[j][0].c_str(), "char[", strlen("char[")) == 0 )
					{
						i << para[j][3].c_str();
					}
					else if ( para[j][0] == "int" )
					{
						int f1 = atoi(para[j][3].c_str());
						i << f1;
					}
					else if ( para[j][0] == "unsigned int" )
					{
						unsigned int f1 = atoi(para[j][3].c_str());
						i << f1;
					}
					else if ( para[j][0] == "short" )
					{
						short f1 = atoi(para[j][3].c_str());
						i << f1;
					}
					else if ( para[j][0] == "float" )
					{
						float f1 = 0;
						sscanf(para[j][3].c_str(), "%f", &f1);
						i << f1;
					}
					else if ( para[j][0] == "double" )
					{
						double f1 = 0;
						sscanf(para[j][3].c_str(), "%lf", &f1);
						i << f1;
					}
					else
					{
						dblog(LOG1, "OTL: variable type = '%s', unknown, error\n", para[j][0].c_str());
						i << para[j][3].c_str();
					}
					
					if ( logsourcefile ) dblog(loglevel, logline, logsourcefile, "DB: db_stored_procedure() input para '%s' = '%s'\n", para[j][1].c_str(), para[j][3].c_str());
				}
			}
			
			ConfigLine line(&dbresult);
			
			for ( int j=0; j<para.size(); j++ )
			{
				if ( para[j][2] == "out" || para[j][2] == "inout" )
				{
					string value;
					
					if ( strncmp(para[j][0].c_str(), "char[", strlen("char[")) == 0 )
					{
						i >> value;
						
						if ( character_set == "utf8" )
						{
							value = iconv_convert("UTF-8", "GB2312", value);
						}
					}
					else if ( para[j][0] == "int" )
					{
						int f1 = 0;
						i >> f1;
						value = ToString(f1);
					}
					else if ( para[j][0] == "unsigned int" )
					{
						unsigned int f1 = 0;
						i >> f1;
						value = ToString(f1);
					}
					else if ( para[j][0] == "short" )
					{
						short f1 = 0;
						i >> f1;
						value = ToString(f1);
					}
					else if ( para[j][0] == "float" )
					{
						float f1 = 0;
						i >> f1;
						value = ToString(f1);
					}
					else if ( para[j][0] == "double" )
					{
						double f1 = 0;
						i >> f1;
						value = ToString(f1);
					}
					else
					{
						dblog(LOG1, "OTL: variable type = '%s', unknown, error\n", para[j][0].c_str());
						i >> value;
					}
					
					line.push_field_name(para[j][1]);
					line.push_field_value(value);
				}
			}
			
			dbresult.push_back(line);
			
		}
		catch(otl_exception& p) // intercept OTL exceptions
		{
			//cerr<<p.msg<<endl; // print out error message
			//cerr<<p.stm_text<<endl; // print out SQL that caused the error
			//cerr<<p.sqlstate<<endl; // print out SQLSTATE message
			//cerr<<p.var_info<<endl; // print out the variable that caused the error
			dblog(LOG1, "OTL: SQL \"%s\" error=%d: %s\n", p.stm_text, p.code, p.msg);
			
			//-30081: [IBM][CLI Driver] SQL30081N  A communication error has been detected.  Communication protocol being used: "TCP/IP".  Communication API being used: "SOCKETS".  Location where the error was detected: "192.168.1.159".  Communication function detecting the error: "recv".  Protocol specific error code(s): "*", "*", "0".  SQLSTATE=08001
			if ( p.code == -30081 )
			{
				db_close();
				
				//数据库刚刚重连成功时，sql失败，不能递归调用，否则引起死循环
				if ( flag_reconnect == 0 )
				{
					return db_stored_procedure(procedure_name, paralist, dbresult, loglevel, logline, logsourcefile);
				}
			}
			
			return -1;
		}
		
		if ( logsourcefile )
		{
			dblog(loglevel, logline, logsourcefile, "OTL: SQL \"%s\" return %d records\n", sql.c_str(), dbresult.size());
			for ( int j=0; j<dbresult.size(); j++ )
			{
				string dis;
				for ( int k=0; k<dbresult[j].size(); k++ )
				{
					if ( k != 0 ) dis += ", ";
					dis += "[" + dbresult[j].get_field_name(k) + "]=" + dbresult[j][k];
				}
				dblog(loglevel, logline, logsourcefile, "  %d : %s\n", j, dis.c_str());
			}
		}
		
		return dbresult.size();
	}
	
	
	int db_execute(const string & sql, int loglevel = 0, int logline = 0, char * logsourcefile = NULL)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		int flag_reconnect = 0;
		
		if ( opened == 0 )
		{
			if ( db_open(databasename, host, username, password) < 0 )
			{
				//re-connect failed
				dblog(LOG1, "OTL: SQL \"%s\" error: '%s'\n", sql.c_str(), "Can't connect to DB server");
				return -1;
			}
			else
			{
				//re-connect ok
				flag_reconnect = 1;
			}
		}
		
		string newsql = sql;
		
		if ( character_set == "utf8" )
		{
			newsql = iconv_convert("GB2312", "UTF-8", sql);
		}
		
		int affected_rows = 0;
		
		try
		{
			/*
			otl_stream i(50, newsql.c_str(), db);
			*/
			
			affected_rows = otl_cursor::direct_exec(db, newsql.c_str());
			
		}
		catch(otl_exception& p) // intercept OTL exceptions
		{
			//cerr<<p.msg<<endl; // print out error message
			//cerr<<p.stm_text<<endl; // print out SQL that caused the error
			//cerr<<p.sqlstate<<endl; // print out SQLSTATE message
			//cerr<<p.var_info<<endl; // print out the variable that caused the error
			dblog(LOG1, "OTL: SQL \"%s\" error=%d: %s\n", p.stm_text, p.code, p.msg);
			
			//-30081: [IBM][CLI Driver] SQL30081N  A communication error has been detected.  Communication protocol being used: "TCP/IP".  Communication API being used: "SOCKETS".  Location where the error was detected: "192.168.1.159".  Communication function detecting the error: "recv".  Protocol specific error code(s): "*", "*", "0".  SQLSTATE=08001
			if ( p.code == -30081 )
			{
				db_close();
				
				//数据库刚刚重连成功时，sql失败，不能递归调用，否则引起死循环
				if ( flag_reconnect == 0 )
				{
					return db_execute(sql, loglevel, logline, logsourcefile);
				}
			}
			
			return -1;
		}
		
		if ( logsourcefile ) dblog(loglevel, logline, logsourcefile, "DB: SQL \"%s\" done, %d records affected\n", sql.c_str(), affected_rows);
		
		return affected_rows;
	}
	
	
	int db_create(const string & table_define)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		//创建表
		string sql = "CREATE TABLE IF NOT EXISTS" + table_define.substr(strlen("CREATE TABLE"));
		int ret = db_execute(sql);
		if ( ret < 0 ) return ret;
		
		unsigned int first = table_define.find("`");
		if ( first == string::npos ) return -1;
		
		unsigned int last = table_define.find("`", first+1);
		if ( last == string::npos ) return -1;
		
		string table_name = table_define.substr(first+1, last-first-1);
		
		DATASET dbresult;
		
		//检查表
		sql = "show create table " + table_name;
		ret = db_select(sql, dbresult);
		if ( ret != 1 )
		{
			dblog(LOG1, "OTL: db_create(): SQL: '%s' error\n", sql.c_str());
			return -1;
		}
		
		if ( dbresult[0][1] == table_define )
		{
			//same
			dblog(LOG1, "OTL: db_create(): table '%s' ok\n", table_name.c_str());
			return 1;
		}
		else
		{
			//not same
			printf("OTL: db_create(): table '%s' error, db define is:\n%s\nprogram define is:\n%s\n", 
				dbresult[0][0].c_str(), dbresult[0][1].c_str(), table_define.c_str());
			dblog(LOG1, "OTL: db_create(): table '%s' error, db define is:\n%s\nprogram define is:\n%s\n", 
				dbresult[0][0].c_str(), dbresult[0][1].c_str(), table_define.c_str());
			return -1;
		}
	}
};

#endif

