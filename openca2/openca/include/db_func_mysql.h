/*
MySQL数据库使用utf8时，客户端程序需要执行SET NAMES utf8
select得到的数据，经过
iconv_convert("UTF-8", "GB2312", CONFIG_BUSSOP_EXCEPTION[0]["DES"])
后，得到正常数据值
*/

#ifndef __DB_FUNC_MYSQL_H__
#define __DB_FUNC_MYSQL_H__

#include <string>
#include <mysql.h>

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

class MYSQLDB
{
private:
	MYSQL dbConnection;
	string host;
	string databasename;
	string username;
	string password;
	int opened;
	LogFile * plog;
	string character_set;	//"latin1", "gbk", "utf8"
	
	boost::recursive_mutex mutex;
	
public:
	MYSQLDB()
	{
		opened = 0;
		plog = NULL;
		character_set = "latin1";
	}
	
	
	MYSQL * db()
	{
		return &dbConnection;
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
		
		printf("%s: Database: try to connect mysql '%s@%s(%s)'\n", pid2name(getpid()).c_str(), username.c_str(), databasename.c_str(), host.c_str());
		dblog(LOG1, "%s: Database: try to connect mysql '%s@%s(%s)'\n", pid2name(getpid()).c_str(), username.c_str(), databasename.c_str(), host.c_str());
		
		mysql_init(&dbConnection); 
		if ( mysql_real_connect(&dbConnection, host.c_str(), username.c_str(), password.c_str(), databasename.c_str(), 0, NULL, 0) )
		{
			opened = 1;
			printf("%s: Database: %s@%s connected successfully as user %s\n", pid2name(getpid()).c_str(), databasename.c_str(), host.c_str(), username.c_str());
			dblog(LOG1, "%s: Database: %s@%s connected successfully as user %s\n", pid2name(getpid()).c_str(), databasename.c_str(), host.c_str(), username.c_str());
			
			//TANG 2010-3-26 9:40
			//需要设置字符集
			if ( character_set != "latin1" )
			{
				string sql = "SET NAMES " + character_set;
				db_execute(sql, LOG1);
			}
			
			return 1;
		}
		else
		{
			opened = 0;
			if ( mysql_errno(&dbConnection) )
			{
				printf("%s: Database '%s@%s(%s)' open error=%d: '%s'\n", 
					pid2name(getpid()).c_str(), 
					username.c_str(), databasename.c_str(), host.c_str(), 
					mysql_errno(&dbConnection), mysql_error(&dbConnection)
					);
				dblog(LOG1, "%s: Database '%s@%s(%s)' open error=%d: '%s'\n", 
					pid2name(getpid()).c_str(), 
					username.c_str(), databasename.c_str(), host.c_str(), 
					mysql_errno(&dbConnection), mysql_error(&dbConnection)
					);
			}
			return -1;
		}
	}
	
	
	int db_close()
	{
		if ( opened )
		{
			mysql_close(&dbConnection);
			printf("Database: %s@%s disconnected\n", databasename.c_str(), host.c_str());
			dblog(LOG1, "Database: %s@%s disconnected\n", databasename.c_str(), host.c_str());
		}
		opened = 0;
		return 1;
	}
	
	
	int db_select(const string & sql, DATASET & dbresult, int loglevel = 0, int logline = 0, char * logsourcefile = NULL)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		dbresult.clear();
		
		int flag_reconnect = 0;
		
		if ( opened == 0 )
		{
			if ( db_open(databasename, host, username, password) < 0 )
			{
				//re-connect failed
				dblog(LOG1, "SQL \"%s\" error: '%s'\n", sql.c_str(), "Can't connect to MySQL server");
				return -1;
			}
			else
			{
				//re-connect ok
				flag_reconnect = 1;
			}
		}
		
		MYSQL_RES *res_ptr;
		MYSQL_ROW sqlrow;
		
		int res = mysql_query(&dbConnection, sql.c_str());
		if ( res )
		{
			int errorid = mysql_errno(&dbConnection);
			dblog(LOG1, "SQL \"%s\" error=%d: '%s'\n", sql.c_str(), errorid, mysql_error(&dbConnection));
			
			//2006 - MySQL server has gone away
			//2013 - Lost connection to MySQL server during query
			if ( errorid == 2006 || errorid == 2013 )
			{
				db_close();
				
				//数据库刚刚重连成功时，sql失败，不能递归调用，否则引起死循环
				if ( flag_reconnect == 0 )
				{
					return db_select(sql, dbresult, loglevel, logline, logsourcefile);
				}
			}
			
			return -1;
		}
		
		res_ptr = mysql_store_result(&dbConnection);
		if ( res_ptr )
		{
			int row_num = 0;
			if ( (row_num = (int)mysql_num_rows(res_ptr)) == 0 )
			{
				dblog(loglevel, logline, logsourcefile, "DB: SQL \"%s\" return 0 records\n", sql.c_str());
				mysql_free_result(res_ptr);
				return 0;
			}
			
			int col_num = mysql_num_fields(res_ptr);
			
			MYSQL_FIELD * fields = mysql_fetch_fields(res_ptr);
			
			int recordcount = 0;
			while ( (sqlrow = mysql_fetch_row(res_ptr)) )
			{
				ConfigLine line(&dbresult);
				for ( int i=0; i<col_num; i++ )
				{
					if ( recordcount == 0 ) dbresult.push_field_name(fields[i].name);
					
					string value = (sqlrow[i]) ? sqlrow[i] : "";
					
					switch ( fields[i].type )
					{
						case MYSQL_TYPE_TINY: 		//TINYINT字段
						case MYSQL_TYPE_SHORT: 		//SMALLINT字段
						case MYSQL_TYPE_LONG: 		//INTEGER字段
						case MYSQL_TYPE_INT24: 		//MEDIUMINT字段
						case MYSQL_TYPE_LONGLONG: 	//BIGINT字段
						case MYSQL_TYPE_DECIMAL: 	//DECIMAL或NUMERIC字段
						//case MYSQL_TYPE_NEWDECIMAL: //精度数学DECIMAL或NUMERIC
						case MYSQL_TYPE_FLOAT: 		//FLOAT字段
						case MYSQL_TYPE_DOUBLE: 	//DOUBLE或REAL字段
							break;
						//case MYSQL_TYPE_BIT: 		//BIT字段
						//	break;
						case MYSQL_TYPE_TIMESTAMP: 	//TIMESTAMP字段
						case MYSQL_TYPE_DATE: 		//DATE字段
						case MYSQL_TYPE_TIME: 		//TIME字段
						case MYSQL_TYPE_DATETIME: 	//DATETIME字段
						case MYSQL_TYPE_YEAR: 		//YEAR字段
							break;
						case MYSQL_TYPE_STRING: 	//CHAR字段
						case MYSQL_TYPE_VAR_STRING: //VARCHAR字段
							if ( character_set == "utf8" )
							{
								value = iconv_convert("UTF-8", "GB2312", value);
							}
							break;
						case MYSQL_TYPE_BLOB: 		//BLOB或TEXT字段（使用max_length来确定最大长度）
							break;
						case MYSQL_TYPE_SET: 		//SET字段
						case MYSQL_TYPE_ENUM: 		//ENUM字段
						case MYSQL_TYPE_GEOMETRY: 	//Spatial字段
						case MYSQL_TYPE_NULL: 		//NULL-type字段
						//case MYSQL_TYPE_CHAR: 	//不再重视，用MYSQL_TYPE_TINY取代
							break;
						default:
							printf("DB FIELD name='%s', type=%d, value='%s', type unknown, error\n", fields[i].name, fields[i].type, value.c_str());
							break;
					}
					
					line.push_field_value(value);
				}
				
				if ( !line[0].empty() ) dbresult.push_line_name(line[0], dbresult.size(), dbresult.get_repeat_key_count(line[0]));
				dbresult.push_back(line);
				recordcount++;
			}
			
			if ( logsourcefile )
			{
				dblog(loglevel, logline, logsourcefile, "DB: SQL \"%s\" return %d records\n", sql.c_str(), dbresult.size());
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
			
			mysql_free_result(res_ptr);
			return row_num;
		}
		
		mysql_free_result(res_ptr);
		return -1;
	}
	
	
	//paralist like "string,p1,in,abc;int,p2,out,0;int,p3,inout,5"
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
				dblog(LOG1, "DB: db_stored_procedure() name='%s', paralist='%s', error: '%s'\n", procedure_name.c_str(), paralist.c_str(), "Can't connect to MySQL server");
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
		
		for ( int i=0; i<para.size(); i++ )
		{
			if ( para[i][2] == "inout" )
			{
				sql.clear();
				sql += "SET @" + para[i][1] + " = " + para[i][3] + ";";
				
				db_execute(sql, loglevel, logline, logsourcefile);
			}
		}
		
		
		//call procedure
		sql.clear();
		sql = "call " + procedure_name + "(";
		for ( int i=0; i<para.size(); i++ )
		{
			if ( i ) sql += ", ";
			
			if ( para[i][2] == "in" )
			{
				sql += para[i][3];
			}
			else if ( para[i][2] == "out" || para[i][2] == "inout" )
			{
				sql += "@" + para[i][1];
			}
			else
			{
				return -1;
			}
		}
		sql += ");";
		
		if ( db_execute_2(sql, loglevel, logline, logsourcefile) < 0 )
		{
			return -1;
		}
		
		
		//got parameter
		sql.clear();
		for ( int i=0; i<para.size(); i++ )
		{
			if ( para[i][2] == "out" || para[i][2] == "inout" )
			{
				if ( sql == "" )
				{
					sql += "SELECT @" + para[i][1];
				}
				else
				{
					sql += ", @" + para[i][1];
				}
			}
		}
		
		if ( sql.size() == 0 ) return 0;	//存储过程没有输出参数
		
		return db_select(sql, dbresult, loglevel, logline, logsourcefile);
	}
	
	
	//SET @p3 = 7; SET @p2 = 4;
	//call qq(6, @p2, @p3);
	//SELECT @p2, @p3;
	int db_stored_procedure(const string & sql_set, const string & sql_call, const string & sql_select, DATASET & dbresult, int loglevel = 0, int logline = 0, char * logsourcefile = NULL)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		dbresult.clear();
		
		int flag_reconnect = 0;
		
		if ( opened == 0 )
		{
			if ( db_open(databasename, host, username, password) < 0 )
			{
				//re-connect failed
				dblog(LOG1, "DB: db_stored_procedure() sql_set='%s', sql_call='%s', sql_select='%s', error: '%s'\n", sql_set.c_str(), sql_call.c_str(), sql_select.c_str(), "Can't connect to MySQL server");
				return -1;
			}
			else
			{
				//re-connect ok
				flag_reconnect = 1;
			}
		}
		
		if ( logsourcefile ) dblog(loglevel, logline, logsourcefile, "DB: db_stored_procedure() sql_set='%s', sql_call='%s', sql_select='%s'\n", sql_set.c_str(), sql_call.c_str(), sql_select.c_str());
		
		if ( sql_set.size() > 0 )
		{
			ConfigLine line;
			line.readline(sql_set, ";");
			
			for ( int i=0; i<line.size(); i++ )
			{
				db_execute(line[i], loglevel, logline, logsourcefile);
			}
		}
		
		//call procedure
		if ( db_execute_2(sql_call, loglevel, logline, logsourcefile) < 0 )
		{
			return -1;
		}
		
		if ( sql_select.size() == 0 ) return 0;	//存储过程没有输出参数
		
		//got parameter
		return db_select(sql_select, dbresult, loglevel, logline, logsourcefile);
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
				dblog(LOG1, "SQL \"%s\" error: '%s'\n", sql.c_str(), "Can't connect to MySQL server");
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
		
		int res = mysql_query(&dbConnection, newsql.c_str());
		if ( res )
		{
			int errorid = mysql_errno(&dbConnection);
			dblog(LOG1, "SQL \"%s\" error=%d: '%s'\n", sql.c_str(), errorid, mysql_error(&dbConnection));
			
			//2006 - MySQL server has gone away
			//2013 - Lost connection to MySQL server during query
			if ( errorid == 2006 || errorid == 2013 )
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
		
		int affected_rows = mysql_affected_rows(&dbConnection);
		
		if ( logsourcefile ) dblog(loglevel, logline, logsourcefile, "DB: SQL \"%s\" done, %d records affected\n", sql.c_str(), affected_rows);
		
		return affected_rows;
	}
	
	
	int db_execute_2(const string & sql, int loglevel = 0, int logline = 0, char * logsourcefile = NULL)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		int flag_reconnect = 0;
		
		if ( opened == 0 )
		{
			if ( db_open(databasename, host, username, password) < 0 )
			{
				//re-connect failed
				dblog(LOG1, "SQL \"%s\" error: '%s'\n", sql.c_str(), "Can't connect to MySQL server");
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
		
		int res = mysql_real_query(&dbConnection, newsql.c_str(), newsql.size());
		if ( res )
		{
			int errorid = mysql_errno(&dbConnection);
			dblog(LOG1, "SQL \"%s\" error=%d: '%s'\n", sql.c_str(), errorid, mysql_error(&dbConnection));
			
			//2006 - MySQL server has gone away
			//2013 - Lost connection to MySQL server during query
			if ( errorid == 2006 || errorid == 2013 )
			{
				db_close();
				
				//数据库刚刚重连成功时，sql失败，不能递归调用，否则引起死循环
				if ( flag_reconnect == 0 )
				{
					return db_execute_2(sql, loglevel, logline, logsourcefile);
				}
			}
			
			return -1;
		}
		
		int affected_rows = mysql_affected_rows(&dbConnection);
		
		if ( logsourcefile ) dblog(loglevel, logline, logsourcefile, "DB: SQL \"%s\" done, %d records affected\n", sql.c_str(), affected_rows);
		
		return affected_rows;
	}
	
	
	//mysql_insert_id() 返回给定的 connection 中上一步 INSERT 产生的 AUTO_INCREMENT 的 ID 号
	unsigned long db_insert_id()
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		return mysql_insert_id(&dbConnection);
	}
	
	
	int db_create(const string & table_define_p)
	{
		boost::recursive_mutex::scoped_lock lock(mutex);
		
		string table_define = table_define_p;
		
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
			dblog(LOG1, "db_create(): SQL \"%s\" error\n", sql.c_str());
			return -1;
		}
		
		string db_define = dbresult[0][1];
		
		unsigned int f1 = db_define.find("AUTO_INCREMENT=");
		if ( f1 != string::npos )
		{
			unsigned int f2 = db_define.find(" ", f1);
			if ( f2 != string::npos )
			{
				db_define.erase(f1, f2-f1+1);
			}
		}
		
		//TANG 2010-7-2 16:32
		//数据库版本不同，有时候大小写有区别
		table_define = string_toupper(table_define);
		db_define    = string_toupper(db_define);
		
		//TANG 2010-6-29 11:47
		//数据库版本不同，有时候是"PRIMARY KEY  ("，有时候是"PRIMARY KEY ("
		string_replace(table_define, "PRIMARY KEY  (", "PRIMARY KEY (");
		string_replace(db_define,    "PRIMARY KEY  (", "PRIMARY KEY (");
		
		if ( db_define == table_define )
		{
			//same
			dblog(LOG1, "db_create(): table '%s' ok\n", table_name.c_str());
			return 1;
		}
		else
		{
			//not same
			printf("db_create(): table '%s' error, db define is:\n%s\nprogram define is:\n%s\n", 
				dbresult[0][0].c_str(), db_define.c_str(), table_define.c_str());
			dblog(LOG1, "db_create(): table '%s' error, db define is:\n%s\nprogram define is:\n%s\n", 
				dbresult[0][0].c_str(), db_define.c_str(), table_define.c_str());
			return -1;
		}
	}
};

#endif

