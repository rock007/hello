#ifndef _PROCESS_H_
#define _PROCESS_H_

#ifdef __WIN32__
	#include <stdio.h>
	#include <process.h>
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <sys/types.h>
	#include <signal.h>
	#include <errno.h>
	#include <fstream.h>
	#include <strstream>
	//#include <fstream>
	//#include <strstream>
	#include <string.h>
	#include <strings.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#ifdef __SunOS__
		#include <procfs.h>
	#endif
	#ifdef __Linux__
		#include <sys/procfs.h>
	#endif
	
	int kill_process_by_name(char * processname, int delayms = 1000);
	int kill_process_by_name(pid_t & pid, char * processname, int delayms = 1000);
	
	int get_process_memory(pid_t pid);
	
	#ifdef __SunOS__
	int get_process_cputime(pid_t pid, psinfo_t * ps);
	#endif
#endif

#endif
