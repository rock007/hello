#ifndef _CLASS_POOL_H_
#define _CLASS_POOL_H_

using namespace std;

#include <list>


template <class T>
class ClassPool 
{
public:
	ClassPool() {} 
	~ClassPool() 
	{ 
		//for_each(res.begin(), res.end(), destroy); 
		typename std::list<T*>::iterator it;
		for ( it = res.begin(); it != res.end(); ++it ) destroy(*it);
		res.clear();
	} 
	
	T* alloc() 
	{
		if ( res.empty() ) { return new T; }
		//return first element and remove it from list
		T* p = *(res.begin());
		res.erase(res.begin());
		return p;
	} 
	void free(T* p) { res.push_back(p); } 
	
private:
	list<T*> res;
	
	ClassPool(const ClassPool&);
	ClassPool& operator=(const ClassPool&);
	
	void destroy(T* p) { delete p; } 
};


#endif

