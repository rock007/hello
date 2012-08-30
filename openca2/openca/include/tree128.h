/*****************************************************
 * support 0x00 ~ 0x7F
*****************************************************/
#if !defined __TREE128__
#define __TREE128__

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
            struct __Node128  //�ڵ�struct
        {
            typedef __Node128<VALUE>* POINTER;
            
            VALUE * data; //����
            POINTER ptr[128]; //�ӽڵ�
            
            __Node128():data(0)
            {
                memset(ptr,0,sizeof(POINTER)*128);
            }
        };
#ifdef __WIN32__
    }
#endif
    
    template<typename VALUE>
        class CTree128
    {
#ifdef __WIN32__
        typedef  Private::__Node128<VALUE> NODE;
#else
        typedef  __Node128<VALUE> NODE;
#endif
        
    private:
        long                    m_lcount;          //�������������
        long                    m_lnodecount;      //�ڵ���
        NODE*                   m_proot;           //�����ָ��
        
    public:
        CTree128():m_lcount(0),m_lnodecount(0),m_proot(CreateNode()) //notice:Keep order with their declare
        {
        }
        
        ~CTree128()
        {
            DestroyAll();//�ͷ������ӽڵ�
            
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
            for(short i =0;i<128;++i)
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
            if ( c >= 0x00 && c <= 0x7F )
            {
            	return c;
            }
            
            return -1;
        }
        
        bool Insert(const char*pKey,const VALUE &data) //����ڵ�
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
            for( short i = 0 ; i < 128 ; i ++ )
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
