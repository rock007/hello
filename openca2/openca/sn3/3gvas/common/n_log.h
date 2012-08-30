
#ifndef N_LOG_HEADDEFINE_
#define N_LOG_HEADDEFINE_

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include "n_io.h"

#define NLOG_MAXFILE_LEN	2*1024*1024
#ifdef WIN32
#define NLOG_DEFAULT_FILENAME	"c:/nzq_working/new.log"
#else
#define NLOG_DEFAULT_FILENAME	"new"
#endif

#define NLOGNUL	0, 0, ""
#define NLOG0	0, __LINE__, __FILE__
#define NLOG1	1, __LINE__, __FILE__
#define NLOG2	2, __LINE__, __FILE__
#define NLOG3	3, __LINE__, __FILE__
#define NLOG4	4, __LINE__, __FILE__
#define NLOG5	5, __LINE__, __FILE__
#define NLOG6	6, __LINE__, __FILE__
#define NLOG7	7, __LINE__, __FILE__
#define NLOG8	8, __LINE__, __FILE__
#define NLOG9	9, __LINE__, __FILE__

typedef struct log_datastr
{
	int   m_iflag;
	char  m_sfname[1024];
	FILE* m_pfile;
	int   m_ilevel;

	int   m_icursize;

} log_datastr;

extern int nlog_initial(const char* s_logfname, const int i_level);
extern int nlog_logit(const int loglevel, const int i_line, const char *s_fname, const char *sformat, ...);

#define N_LOGIT	nlog_logit
#endif
