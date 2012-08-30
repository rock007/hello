/*********************************************************************************
This class must be used in the scope of a process(½ø³Ì), threads are OK
*********************************************************************************/
#ifndef _MSGQUEUE_T__SIP_
#define _MSGQUEUE_T__SIP_

#include <stdio.h>
#include <pthread.h>

#define MAX_BUFFER_4_MUTEX_MSGQUEUE_T__SIP_ 64

template<class T>
class msgqueue_t
{
private:
	int		i1;
	int 	i2;
	int * 	ifirst;
	int * 	ilast;
	T*		buffer;
	int 	size;
	
	static 	pthread_mutex_t access_mutex[MAX_BUFFER_4_MUTEX_MSGQUEUE_T__SIP_];
	static 	int totalindex;
	int		index;	// for access_mutex[index]
	
public:
	msgqueue_t(){}
	msgqueue_t(T* queuebuf, int bufsize) : i1(0), i2(0), 
				buffer(queuebuf), size(bufsize) 
	{
		ifirst = &i1;
		ilast  = &i2;
		index = totalindex;
		totalindex = ( totalindex + 1 ) % MAX_BUFFER_4_MUTEX_MSGQUEUE_T__SIP_;
	}
	msgqueue_t(T* queuebuf, int bufsize, int* ihead, int* itail) : 
				ifirst(ihead), ilast(itail), 
				buffer(queuebuf), size(bufsize) 
	{
		index = totalindex;
		totalindex = ( totalindex + 1 ) % MAX_BUFFER_4_MUTEX_MSGQUEUE_T__SIP_;
	}
	
	~msgqueue_t(){}
	
	int init(T* queuebuf, int bufsize)
	{
		if ( queuebuf==NULL || bufsize<=0 ) return 0;
		i1	=	0;
		i2	=	0;
		ifirst = &i1;
		ilast  = &i2;
		buffer	=	queuebuf;
		size	=	bufsize;
		index = totalindex;
		totalindex = ( totalindex + 1 ) % MAX_BUFFER_4_MUTEX_MSGQUEUE_T__SIP_;
		return 1;
	}
	int init(T* queuebuf, int bufsize, int* ihead, int* itail)
	{
		if ( queuebuf==NULL || bufsize<=0 ) return 0;
		if ( ihead==NULL || itail==NULL ) return 0;
		i1	=	0;
		i2	=	0;
		ifirst = ihead;
		ilast  = itail;
		buffer	=	queuebuf;
		size	=	bufsize;
		index = totalindex;
		totalindex = ( totalindex + 1 ) % MAX_BUFFER_4_MUTEX_MSGQUEUE_T__SIP_;
		return 1;
	}
		
	T* 	get();
	int put(T* data);
	int isempty();
	int isfull();
	
	int getcount();
	int clearall();
};


template<class T>
int msgqueue_t<T>::totalindex = 0;

template<class T>
pthread_mutex_t msgqueue_t<T>::access_mutex[MAX_BUFFER_4_MUTEX_MSGQUEUE_T__SIP_] = {
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,
	PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER,PTHREAD_MUTEX_INITIALIZER
	};
	

template<class T>
T* msgqueue_t<T>::get()		// return NULL means no msg
{
	int tempint;

	if ( isempty() == 1 ){	//empty
		return NULL;
	}
	
	pthread_mutex_lock(&access_mutex[index]);
	
	if ( isempty() == 1 ){	//empty
		pthread_mutex_unlock(&access_mutex[index]);
		return NULL;
	}
	*ifirst = ( *ifirst + 1 ) % size;
	tempint = *ifirst;

	pthread_mutex_unlock(&access_mutex[index]);
	return &(buffer[tempint]);
}

template<class T>
int msgqueue_t<T>::put(T* data)
{
	pthread_mutex_lock(&access_mutex[index]);
	
	if ( isfull() == 1 ){	//full
		pthread_mutex_unlock(&access_mutex[index]);
		return 0;
	}
	*ilast = ( *ilast + 1 ) % size;
	memcpy( &(buffer[*ilast]), data, sizeof(T) );
	
	pthread_mutex_unlock(&access_mutex[index]);
	return 1;
}

template<class T>
int msgqueue_t<T>::isempty()
{
	if ( *ilast == *ifirst )
		return 1;
	else 
		return 0;	
}

template<class T>
int msgqueue_t<T>::isfull()
{
	if ( ((*ilast + 1) % size) == *ifirst )
		return 1;
	else 
		return 0;	
}


template<class T>
int msgqueue_t<T>::getcount()
{
	int tempint;
	
	//pthread_mutex_lock(&access_mutex[index]);
	
	if ( *ilast >= *ifirst ){
		tempint = *ilast - *ifirst;
	}else{
		tempint = size - (*ifirst - *ilast);
	}

	//pthread_mutex_unlock(&access_mutex[index]);
	return tempint;
}


template<class T>
int msgqueue_t<T>::clearall()
{
	pthread_mutex_lock(&access_mutex[index]);
	
	*ifirst = *ilast = 0;

	pthread_mutex_unlock(&access_mutex[index]);
	return 1;
}


#endif

