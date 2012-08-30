/* -*- head file -*-*/

#ifndef N_OPERATION_HEAD_DEFINE_
#define N_OPERATION_HEAD_DEFINE_

#include "n_type.h"

#ifdef WIN32
	#include <windows.h>

	typedef CRITICAL_SECTION nt_mutex;
	typedef HANDLE nt_thread;

#else
	#include <unistd.h>
	#include <pthread.h>

	typedef pthread_mutex_t nt_mutex;
	typedef pthread_t nt_thread;
#endif

#define N_THREAD_STATUS_INITIAL		1
#define N_THREAD_STATUS_RUNING		2
#define N_THREAD_STATUS_WAITEXIST	3
#define N_THREAD_STATUS_EXIT		4

typedef void* (* func_thread)(void*);

// * ================================================== *
// @ mutex operation
// * ================================================== *
#ifdef WIN32
	#define nmutex_initial(pmutex)	InitializeCriticalSection(pmutex)
	#define nmutex_destroy(pmutex)	DeleteCriticalSection(pmutex)
	#define nmutex_lock(pmutex)		EnterCriticalSection(pmutex)
	#define nmutex_unlock(pmutex)	LeaveCriticalSection(pmutex)
#else
	#define nmutex_initial(pmutex)	pthread_mutex_init(pmutex, 0)
	#define nmutex_destroy(pmutex)	pthread_mutex_destroy(pmutex)
	#define nmutex_lock(pmutex)		pthread_mutex_lock(pmutex)
	#define nmutex_unlock(pmutex)	pthread_mutex_unlock(pmutex)
#endif

// * ================================================== *
// @ thread operation
// * ================================================== *
extern nt_thread nthread_create(func_thread func, void* param, t_uint ui_size);
extern t_int nthread_waitexit(nt_thread thr_hand);
extern t_void nthread_destroy(nt_thread thr_hand);

// * ================================================== *
// @ sleep
// * ================================================== *
#ifdef WIN32
	#define n_sleep(misec)	Sleep(misec)
#else
	#define n_sleep(misec)	usleep(misec*1000);
#endif

#endif // N_OPERATION_HEAD_DEFINE_
