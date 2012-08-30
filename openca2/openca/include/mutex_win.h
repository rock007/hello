#ifndef _MUTEX_WIN_H_
#define _MUTEX_WIN_H_

#include <windows.h>

namespace boost 
{

template <class T>
class scoped_lock;


class recursive_mutex
{
private:
	HANDLE hMutex;
	
public:
	typedef scoped_lock<recursive_mutex> scoped_lock;
	
	recursive_mutex()
	{
		hMutex = CreateMutex(NULL, FALSE, NULL);
	}
	
	~recursive_mutex()
	{
		CloseHandle(hMutex);
	}
	
	HANDLE getHandle()
	{
		return hMutex;
	}
};


template <class T>
class scoped_lock
{
private:
	HANDLE hMutex;
	
public:
	scoped_lock(T & mutex)
	{
		hMutex = mutex.getHandle();
		WaitForSingleObject(hMutex, INFINITE);
	}
	
	~scoped_lock()
	{
		ReleaseMutex(hMutex);
	}
};

} // namespace boost

#endif

