#ifndef _DATABUF_H_
#define _DATABUF_H_


template <class T>
struct databuf_t
{
	//data
	T	data;
	
	struct databuf_t<T> * next;
};


template <class T>
class databuf
{
private:
	struct databuf_t<T> * phead;	//dynamic malloc
	struct databuf_t<T> * ptail;
	int count;
	int maxcount;

public:
	databuf(int max = 0)
	{
		phead = NULL;
		ptail = NULL;
		count = 0;
		maxcount = max;
	}
	
	~databuf()
	{
		free_databuf_list();
		
		phead = NULL;
		ptail = NULL;
		count = 0;
		maxcount = 0;
	}
	
	int add(T & data)
	{
		struct databuf_t<T> * pmsg = NULL;
		
		if ( maxcount && count >= maxcount )
		{
			//don't malloc()
			pmsg = phead;
			phead = phead->next;
		}
		else
		{
			//malloc()
			pmsg = (struct databuf_t<T> *) malloc(sizeof(struct databuf_t<T>));
			if ( pmsg == NULL ) return 0;
			//printf("databuf(): dynamic malloc() databuf_t %08x\n", pmsg);
			
			count++;
		}
		
		memcpy(&pmsg->data, &data, sizeof(T));
		pmsg->next = NULL;
		
		//add to list
		if ( phead == NULL )
		{
			//first msg
			phead = pmsg;
			ptail = phead;
		}
		else
		{
			ptail->next = pmsg;
			ptail = ptail->next;
		}
		
		return 1;
	}
	
	int free_databuf_list()
	{
		if ( phead == NULL ) return 1;
		
		struct databuf_t<T> * p1 = phead;
		struct databuf_t<T> * p2 = NULL;
		
		while ( p1 )
		{
			p2 = p1->next;
			
			free(p1);
			
			p1 = p2;
		}
		
		phead = NULL;
		ptail = NULL;
		count = 0;
		
		return 1;
	}
	
	int getcurrentcount()
	{
		return count; 
	}
	
	struct databuf_t<T> * getheader()
	{
		return phead; 
	}
	
	int isnull() 
	{ 
		return (phead == NULL); 
	}
	
	int setmaxcount(int max) 
	{ 
		maxcount = max; 
		return 1;
	}

private:
	databuf(const databuf& rhs);
	databuf& operator=(const databuf& rhs);
};


#endif

