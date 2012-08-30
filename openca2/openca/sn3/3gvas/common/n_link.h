/* -*- head file -*-*/

#ifndef CLINK_HEAD_DEFINE_
#define CLINK_HEAD_DEFINE_

#ifndef NPOSITION_DEFINE_
#define NPOSITION_DEFINE_
struct __NPOSITION { };
typedef __NPOSITION* NPOSITION;
#endif

#include <stdio.h>

template <class T>
class CNLink
{
protected:
	struct LinkNode
	{
		T data;
		struct LinkNode* pprev;
		struct LinkNode* pnext;
	};

public:
	CNLink() { m_phead = m_ptail = NULL; m_nNode = 0;}
	virtual ~CNLink(){ 
		MoveAll();
	}

	int GetLength() const {
		return m_nNode;
	}

	NPOSITION InsertHead() {
		LinkNode* pNewNode = MakeNode();
		pNewNode->pnext = m_phead;
		if(m_phead)	m_phead->pprev = pNewNode;
		else		m_ptail = pNewNode;
		m_phead = pNewNode;
		return (NPOSITION)pNewNode;
	}

	NPOSITION InsertTail() {
		LinkNode* pNewNode = MakeNode();
		pNewNode->pprev = m_ptail;
		if(m_ptail)	m_ptail->pnext = pNewNode;
		else		m_phead = pNewNode;
		m_ptail = pNewNode;
		return (NPOSITION)pNewNode;
	}

	NPOSITION InsertBefore(NPOSITION pos) {
		if(!pos)	return NULL;
		if(pos == (NPOSITION)m_phead)	return InsertHead();
		LinkNode* pNode = (LinkNode *)pos;
		LinkNode* pNewNode = MakeNode();
		pNewNode->pprev = pNode;
		pNewNode->pnext = pNode->pnext;
		pNode->pprev = pNewNode;
		return (NPOSITION)pNewNode;
	}

	NPOSITION InsertAfter(NPOSITION pos) {
		if(!pos)		return NULL;
		if(pos == (NPOSITION)m_ptail)
			return InsertTail();
		LinkNode* pNode = (LinkNode *)pos;
		LinkNode* pNewNode = MakeNode();
		pNewNode->pnext = pNode;
		pNewNode->pprev = pNode->pprev;
		pNode->pnext = pNewNode;
		return (NPOSITION)pNewNode;
	}

	// delete data
	int MovePosition(NPOSITION pos) {
		if(pos == NULL)	return -1;
		// first we must delete link operation
		LinkNode* pNode = (LinkNode *)pos;
		if(pNode == m_phead)
		{
			m_phead = m_phead->pnext;
			if(m_phead)	m_phead->pprev = NULL;
			else		m_ptail = NULL;
		}
		else if(pNode == m_ptail)
		{
			m_ptail = m_ptail->pprev;
			if(m_ptail)	m_ptail->pnext = NULL;
			else		m_ptail = NULL;
		}
		else
		{
			pNode->pprev->pnext = pNode->pnext;
			pNode->pnext->pprev = pNode->pprev;
		}
		// set return data value
		ReleaseNode(pNode);
		return 0;
	}

	void MoveAll() {
		while(m_phead) {
			MovePosition(GetHeadPosition());
		}
	}

	// used for view data
	NPOSITION GetHeadPosition() const {
		return (NPOSITION)m_phead;
	}
	NPOSITION GetTailPosition() const {
		return (NPOSITION)m_ptail;
	}

	NPOSITION GetPrevPos(NPOSITION pos) const {
		if(!pos)	return NULL;
		return (NPOSITION)(((LinkNode *)pos)->pprev);
	}
	NPOSITION GetNextPos(NPOSITION pos) const {
		if(!pos)	return NULL;
		return (NPOSITION)(((LinkNode *)pos)->pnext);
	}

	// used for get data from position
	T& operator[](NPOSITION pos) {
		LinkNode* pNode = (LinkNode *)pos;
		return pNode->data;
	}
	const T& operator[](NPOSITION pos) const {
		LinkNode* pNode = (LinkNode *)pos;
		return pNode->data;
	}

	// push data
	NPOSITION Push() {	return InsertTail();}
	int Pop() { return MovePosition((NPOSITION)m_ptail); }

	// add queue data
	NPOSITION AddQueue() { return InsertTail(); }
	int DelQueue() {	return MovePosition((NPOSITION)m_phead);	}

protected:
	LinkNode* m_phead;	// head data
	LinkNode* m_ptail;	// tail data
	int m_nNode;

	LinkNode* MakeNode() {
		LinkNode* pNewNode = new LinkNode; 
		pNewNode->pnext = pNewNode->pprev = NULL;
		m_nNode++;
		return pNewNode;
	}
	void ReleaseNode(LinkNode* pnode) {
		m_nNode--;
		delete pnode;
	}
};

#endif
