/*****************************************************
 * support 0~9
 * support #
 * support *
 * support a~z or A~Z
*****************************************************/
#if !defined __TREE38__
#define __TREE38__

#ifdef __WIN32__
#pragma warning(disable:4786)
#endif

#ifdef __WIN32__
namespace sandy
{
    namespace Private  
    {
#endif
        template<class VALUE>
            struct __Node38  //节点struct
        {
            typedef __Node38<VALUE>* POINTER;
            
            VALUE * data; //数据
            POINTER ptr[38]; //子节点
            
            __Node38():data(0)
            {
                memset(ptr,0,sizeof(POINTER)*38);
            }
        };
#ifdef __WIN32__
    }
#endif
    
    template<typename VALUE>
        class CTree38
    {
#ifdef __WIN32__
        typedef  Private::__Node38<VALUE> NODE;
#else
        typedef  __Node38<VALUE> NODE;
#endif
        
    private:
        long                    m_lcount;          //插入的数据数量
        long                    m_lnodecount;      //节点数
        NODE*                   m_proot;           //根结点指针
        
    public:
        CTree38():m_lcount(0),m_lnodecount(0),m_proot(CreateNode()) //notice:Keep order with their declare
        {
        }
        
        ~CTree38()
        {
            DestroyAll();//释放所有子节点
            
            //TANG 20061211
            if ( m_proot )
            {
            	--m_lnodecount;
            	delete m_proot;
            }
        }
        
        long get_data_count()
        {
        	return m_lcount;
        }
        
        long get_node_count()
        {
        	return m_lnodecount;
        }
        
        long get_data_size()
        {
        	return (m_lcount * sizeof(VALUE));
        }
        
        long get_node_size()
        {
        	return (m_lnodecount * sizeof(NODE));
        }
        
        long get_all_size()
        {
        	return ( get_data_size() + get_node_size() );
        }
        
        void DestroyAll()
        {
            for(short i =0;i<38;++i)
            {
                if(m_proot->ptr[i]!=0)
                {
                    Remove(m_proot->ptr[i]);
                    m_proot->ptr[i] = 0;
                }
            }
        }
        
        int get_char_map_iptr(char c)
        {
            if ( c >= '0' && c <= '9' )
            {
            	return ( c - '0' );
            }
            else if ( c == '#' )
            {
            	return 10;
            }
            else if ( c == '*' )
            {
            	return 11;
            }
            else if ( c >= 'a' && c <= 'z' )
            {
            	return ( c - 'a' + 12 );
            }
            else if ( c >= 'A' && c <= 'Z' )
            {
            	return ( c - 'A' + 12 );
            }
            
            return -1;
        }
        
        bool Insert(const char*pKey,const VALUE &data) //插入节点
        {
#ifdef __WIN32__
            assert(pKey!=0);
#else
            if ( pKey == 0 ) return false;
#endif
            
            NODE * pNode = m_proot;
            NODE * pChildNode =0;
            char c = 0;
            int ichild = 0;
            
            for(unsigned int i=0;i<strlen(pKey);++i)
            {
                c = pKey[i];    
                ichild = get_char_map_iptr(c);
                if ( ichild < 0 ) return false;
                
                pChildNode = pNode->ptr[ichild];
                if(pChildNode == 0) //not build
                {
                    pChildNode = pNode->ptr[ichild] = CreateNode();//create a new child
                }
                pNode = pChildNode ;//change node to child node
            }
            
            if(pNode->data == 0) //empty 
            {
                pNode->data = new VALUE(data);
                ++m_lcount;
                return true;
            }
            else//already inserted
            {
                return false;
            }
        }
        
        bool Lookup(const char*pKey,VALUE &data,bool strick = true)
        {
#ifdef __WIN32__
            assert(pKey!=0);
#else
            if ( pKey == 0 ) return false;
#endif
            
            NODE * pNode = m_proot;
            NODE * pChildNode =0;
            char c = 0;
            int ichild = 0;
            
            bool retval = false;
            
            for(unsigned int i=0;i<strlen(pKey);++i)
            {
                c = pKey[i]; 
                ichild = get_char_map_iptr(c);
                if ( ichild < 0 ) return false;
                
                pChildNode = pNode->ptr[ichild];
                if(pChildNode!=0)
                {
                    pNode = pChildNode;
	                if ( pNode->data != 0 ) //already inserted
	                {
	                	data = *(pNode->data);
	                	retval = true;
	                }
                }
                else // can't find
                {
                    if(!strick)
                    {
                        break;
                    }
                    return false;
                }
            }
            
            return retval;
        }
        
    private:
        NODE *CreateNode()
        {
            NODE *pNewNode = new NODE();
#ifdef __WIN32__
            assert(pNewNode!= 0);
#else
            if ( pNewNode == 0 ) return NULL;
#endif
            ++m_lnodecount;
            return pNewNode;
        }
        
        void Remove(NODE *pNode)
        {
#ifdef __WIN32__
            assert(pNode!=0);
#else
            if ( pNode == 0 ) return;
#endif
            for( short i = 0 ; i < 38 ; i ++ )
            {
                if( pNode -> ptr[ i ] )
                    Remove( pNode -> ptr[ i ] );
            }
            if(pNode->data!=0)
            {
                delete pNode->data;
                --m_lcount;
            }
            --m_lnodecount;
            delete pNode;
        }
    };
#ifdef __WIN32__
}
#endif

#endif
