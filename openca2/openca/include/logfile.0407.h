/*********************************************************************************
 1.The member function startlog() is the new version of function enable() and 
 * disable(), it combines the two functions together. It is strongly recommended
 * to use startlog() rather than enable() and disable(). Pass different parameter
 * to startlog() to set prtflag and logflag, as discriped below:
 * Parameter flag = 0:  both prtflag and logflag are set to 0 
 *                = 1:  prtflag is set to 0, logflag is set to 1
 *                = 2:  prtflag is set to 1, logflag is set to 0
 *                = 3:  both prtflag and logflag are set to 1
 *
 2.The function logreopen() will first close the opened file, and then open a new
 * file as specified with the parameter (char* path, char* name).
 * 
 3.The function loglen() will return the file size.
 * 
 4.TANG 0827: logfile can automatically write log into another file if size is large,
 * don't use logreopen()
 *
 5.TANG 0827: you must call logopen() first, then call startlog(int).
 * But SetLogLevel(int) and SetLogSize(int) can be called whenever
 *
 6.TANG 0827: logopen() has a default parameter LOG_OPEN_LAST_FILE. 
 * If you want to open a new log file when the whole program first start, you can call
 * logopen() with the parameter LOG_OPEN_NEW_FILE in the monitor process, and 
 * other process call logopen() without parameter. 
 * If you want to continue the last log file, just call logopen(void) in every process
 *
 *********************************************************************************/

#ifndef LOGFILE_H_CC
#define LOGFILE_H_CC

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>

#define LOG_PATH_LEN 256
#define LOG_NAME_LEN 64

#define LOG_OPEN_NEW_FILE	1
#define LOG_OPEN_LAST_FILE	0

class LogFile
{
private:
	int writing_file(char * printbuf);
	int mkdirectory(char * path_name);
	
public:
	LogFile(const char* path, const char* name);
	~LogFile();
	
	LogFile();													//TANG 20021014
	int SetLogPathName(const char* path, const char* name);		//TANG 20021014
	
	int GetLogPathName(char * pathname);		//TANG 2002.11.22
	
	int logopen(int nextfile=LOG_OPEN_LAST_FILE);
	int logreopen(const char* path, const char* name);
	
	int logwrite(const char* format, ...);
	int logwrite(int level, const char* format, ...);
	int logwrite(int level, int msglen, unsigned char * msgbuf);
	
	int enable(int flag);
	int disable(int flag);
	int startlog(int flag);
	void logclose(int flag = 9);
	long loglen();
	int SetLoglevel(int level);
	int GetLoglevel() const { return loglevel; }
	
	int SetLogSize(int size);		//TANG 0827
	
	int SetLogLine(int lines);		//TANG 2003.03.31
	
	int IsDateChanged();
	
	int SetLogTimeFlag(int flag);	//TANG 0307
	int logtime();
	
	int SetSubdirMonth(int flag);	//TANG 2003.04.01
	
	int logChangeFile();			//TANG 2003.03.31
	
	int SetMultiThreadFlag(int flag);
	
	int isSameDay(time_t t1, time_t t2);
	int rename_old_file_before_first_open(char * current_filename, time_t tfile_modify);
	
	int get_file_lines(char * file);
	
	int isFileInodeChanged(int fd, char * filename);
	
protected:
	int file0;
	int file1;
	int plogfile;
	int	whichfile;
	
	char currentLogPathName[LOG_PATH_LEN];		//TANG 2002.11.22
	
	char logpath[LOG_PATH_LEN];
	char logname[LOG_NAME_LEN];
	unsigned long logshare;
	int logflag;
	int fileopenflag;
	int prtflag;
	int timeflag;
	int loglevel;
	
	int logsize;					//TANG 0827
	
	int loglinemax;					//TANG 2003.03.31
	int logline;
	
	struct tm tm4file;
	int	filefix;
	
	int checklog;					//TANG 0905
	
	int logtransfer;
	
	int logtimeflag;				//TANG 0307
	
	int flag_subdir_month;
	
	int flag_multi_thread;
	
	int flag_file_inode_changed;	//TANG 2003.04.14
	
	static pthread_key_t dkey;				//TANG 0402
	static pthread_once_t once_control;
	static void buf_free(void * buffer);
	static void create_key(void);
	
	static pthread_mutex_t access_mutex;
};

#endif

