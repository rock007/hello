#ifndef _SAFEBUFFER_H_
#define _SAFEBUFFER_H_


class safebuffer
{
private:
	size_t size;
	
public:
	char * buf;
	
public:
	safebuffer();
	safebuffer(size_t len);
	~safebuffer();
	
	int safemalloc(size_t len);
	int getsize() { return (int)size; };
};


#endif

