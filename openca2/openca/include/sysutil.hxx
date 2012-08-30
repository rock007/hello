#ifndef _SYSUTIL_HXX_
#define _SYSTUIL_HXX_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <kstat.h>
#include <math.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#define CPUSTATES     4

/*
#ifdef SYSUTIL//{

//}
#else//{
*/
extern kstat_ctl_t *kc;
extern kstat_t **cpu_ksp_list;
extern int ncpus;
extern long cp_time[];
extern long last[];

//#endif//}

void cpu_stats_init();
int get_cpu_stats();
void getLoad(int Maximum, int *usr, int *nice, int *sys, int *free);
int get_cpu_percent(int *usr, int *sys, int *free);

#endif //_SYSUTIL_HXX_
