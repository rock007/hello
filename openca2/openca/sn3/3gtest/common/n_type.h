/* -*- head file -*-*/

#ifndef N_BASIC_TYPE_HEAD_DEFINE_
#define N_BASIC_TYPE_HEAD_DEFINE_

// #define N_BIGENDIAN

#include <time.h>
#include <string.h>
#include <stdlib.h>

typedef char				t_char, *t_pchar;
typedef short				t_short, *t_pshort;
typedef int					t_int, *t_pint;
#ifdef WIN32
typedef	__int64				t_int64, *t_pint64;
#else
typedef long long			t_int64, *t_pint64;	
#endif
typedef float				t_float, *t_pfloat;
typedef double				t_double, *p_tdouble;

typedef unsigned char		t_uchar, *t_puchar;
typedef unsigned short		t_ushort, *t_pushort;
typedef unsigned int		t_uint, *t_puint;
#ifdef WIN32
typedef	unsigned __int64	t_uint64, *t_puint64;
#else
typedef unsigned long long	t_uint64, *t_puint64;	
#endif

typedef const t_char		*t_pcstr;
typedef const t_uchar		*t_pucstr;

typedef	void				t_void, *t_pvoid;

typedef time_t				t_time;

typedef t_char				t_bool;
#define NTRUE				1
#define NFALSE				0


#define n_max(a,b)		(((a) > (b)) ? (a) : (b))
#define n_min(a,b)		(((a) < (b)) ? (a) : (b))

#define n_cmalign(a)	(0 == (a)%4) ? (a) : ((a)+4-(a)%4)


#endif // N_BASIC_TYPE_HEAD_DEFINE_
