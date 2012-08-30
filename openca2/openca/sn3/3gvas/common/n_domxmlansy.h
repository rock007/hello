/* -*- head file -*-*/

#ifndef DOMXML_ANSY_HEAD_DEFINE
#define DOMXML_ANSY_HEAD_DEFINE

#include "n_string.h"
#include "n_link.h"

/*========================================================================*/
/**
*  @ ö����   : EDOMXmlNodeB
*  @ ��Ҫ���� : ö�ٻ����Ľڵ�����
*  @ �޸ļ�¼   
*/
/*========================================================================*/
enum EDOMXmlNodeType
{
	EDOMXML_NODETYPE_UNKNOWN=0,					// *- δ֪ *-
	EDOMXML_NODETYPE_ATTRIBUTE,					// *- ���� *-
	EDOMXML_NODETYPE_CDATA_SECTION,				// *- CDATA���� *-
	EDOMXML_NODETYPE_MESSAGE,					// *- ע�� *-
	EDOMXML_NODETYPE_DOCUMENT_FRAGMENT_NODE,	// *- �ĵ�Ƭ�� *-
	EDOMXML_NODETYPE_DOCUMENT,					// *- �ĵ� *-
	EDOMXML_NODETYPE_DOCUMENT_TYPE,				// *- �ĵ����� *-
	EDOMXML_NODETYPE_ELEMENT,					// *- Ԫ�� *-
	EDOMXML_NODETYPE_ENTITY,					// *- ʵ�� *-
	EDOMXML_NODETYPE_ENTITY_REFERENCE,			// *- ʵ������ *-
	EDOMXML_NODETYPE_NOTATION,					// *- Notation *-
	EDOMXML_NODETYPE_PROCESSING_INSTRUCTION,	// *- ָ��� *-
	EDOMXML_NODETYPE_TEXT						// *- Text *-
};


class CDOMXmlNode_Document;
class CDOMXmlNode;
typedef CNLink<const CDOMXmlNode *> CDomXmlNodeLink;

/*========================================================================*/
/**
*  @ ����     : CDOMXmlNode
*  @ ��Ҫ���� : �����ڵ㶨��
*  @ �޸ļ�¼   
*/
/*========================================================================*/
class CDOMXmlNode
{
	// *- friend class declare *-
	friend class CDOMXmlNode_Document;

	// * ============================================================== *
	// @ ���캯�� 
	// * ============================================================== *
public:
	CDOMXmlNode(const CDOMXmlNode_Document* pdocument=NULL);
	virtual ~CDOMXmlNode();

	// *- ���xml�ļ���ʽ�� *-
	virtual bool oriXmlData(CNString& sresult) const;

	// * ============================================================== *
	// @ �ڵ����ݶ��� 
	// * ============================================================== *
protected:
	// -* node value *- 
	CNString m_svalue;

	// -* node type *- 
	EDOMXmlNodeType m_type;

	// -* parent or child node *- 
	CDOMXmlNode *m_pparent, *m_pchild;

	// -* sibling node *- 
	CDOMXmlNode *m_pprev_sibling, *m_pnext_sibling;

	// -* owner document node *-
	CDOMXmlNode_Document* m_powner;

public:
	// -* �ڵ����� *-
	virtual CNString& nodeValue(const char* svalue, const int nlen=-1);
	virtual const CNString& nodeValue() const {	return m_svalue; }

	// -* ��ȡ�ڵ����� *-
	virtual EDOMXmlNodeType nodeType(const EDOMXmlNodeType type) { m_type = type; return m_type; }
	virtual const EDOMXmlNodeType nodeType() const { return m_type;	}

	// -* �����ĵ� *-
	virtual CDOMXmlNode_Document* ownerDocument(const CDOMXmlNode_Document* pdocument) { m_powner = (CDOMXmlNode_Document*)pdocument; return m_powner; }
	virtual const CDOMXmlNode_Document* ownerDocument() const { return m_powner; }

	// -* ���ڵ� *-
	virtual CDOMXmlNode* parentNode(const CDOMXmlNode* pparent) { m_pparent=(CDOMXmlNode *)pparent; return m_pparent; }
	virtual const CDOMXmlNode* parentNode() const{ return m_pparent; }

	// -* ���ֵܽڵ� *-
	virtual CDOMXmlNode* prevSiblingNode(const CDOMXmlNode* prevSibling) { m_pprev_sibling=(CDOMXmlNode *)prevSibling; return m_pprev_sibling; }
	virtual const CDOMXmlNode* prevSiblingNode() const { return m_pprev_sibling; }

	// -* ���ֵܽڵ� *-
	virtual CDOMXmlNode* nextSiblingNode(const CDOMXmlNode* nextSibling) { m_pnext_sibling=(CDOMXmlNode *)nextSibling; return m_pnext_sibling; }
	virtual const CDOMXmlNode* nextSiblingNode() const { return m_pnext_sibling; }

	// -* ��һ�ӽڵ� *-
	virtual CDOMXmlNode* firstChildNode(const CDOMXmlNode* pchild) { m_pchild=(CDOMXmlNode *)pchild; return m_pchild; }
	virtual const CDOMXmlNode* firstChildNode() const	{ return m_pchild; }

	// -* ���һ�ӽڵ� *-
	virtual const CDOMXmlNode* lastChildNode() const;
public:
	// -* �����ӽڵ� *-
	virtual bool appendChild(CDOMXmlNode* padd);

	// -* ���Ӻ�����һ�ڵ� *-
	virtual bool appendPrevChild(CDOMXmlNode* pchild, CDOMXmlNode* padd);

	// -* ���Ӻ�����һ�ڵ� *-
	virtual bool appendNextChild(CDOMXmlNode* pnode, CDOMXmlNode* padd);

	// -* ɾ����ǰ�ڵ� *-
	virtual bool moveThis(bool delnextsibling);

	// -* ���ƽڵ� *-
	virtual CDOMXmlNode* cloneNode(CDOMXmlNode* pparent, CDOMXmlNode_Document* pdocument, bool bnextsibling=false) const;

	// *- ��ȡ��� *-
	virtual int getLevel() const
	{
		int level=0;

		const CDOMXmlNode* tmpnode=parentNode();
		while(tmpnode) {
			level++;
			tmpnode=tmpnode->parentNode();
		}
		return level;
	}
};

/*========================================================================*/
/**
*  @ ����     : CDOMXmlNode_Attribute
*  @ ��Ҫ���� : Attribute �ڵ�
*  @ �޸ļ�¼   
*/
/*========================================================================*/
class CDOMXmlNode_Attribute : public CDOMXmlNode
{
public:
	CDOMXmlNode_Attribute(const CDOMXmlNode_Document* pdocument, const char* sattrname, const char cinter='\"');

public:
	// -* ��֯xml�ڵ����� *-
	virtual bool oriXmlData(CNString& sresult) const;

	// -* ���÷��� *-
	char attrInter(const char cinter) { m_cinter = cinter; return m_cinter; }
	const char attrInter() const { return m_cinter; }

	// -* �������� *-
	CNString& attrName(const char* sattrname) { return nodeValue(sattrname); }
	const CNString& attrName() const { return nodeValue(); }

	// -* ����ֵ *-
	CNString& attrValue(const char* sattrvalue, const int nlen=-1) { 
		m_sattrValue.empty();
		if(nlen == -1)	m_sattrValue.insert(0, sattrvalue);
		else			m_sattrValue.insert(0, sattrvalue, nlen);
		return m_sattrValue;
	}
	const CNString& attrValue() const { return m_sattrValue; }

	// -* ���ش��麯�� *-
	virtual bool appendChild(CDOMXmlNode* padd)	{ return false;}
	virtual bool appendPrevChild(CDOMXmlNode* pchild, CDOMXmlNode* padd) { return false; }
	virtual bool appendNextChild(CDOMXmlNode* pnode, CDOMXmlNode* padd) { return false;}

private:
	char m_cinter;
	CNString m_sattrValue;
};

/*========================================================================*/
/**
*  @ ����     : CDOMXmlNode_Message
*  @ ��Ҫ���� : Message �ڵ�
*  @ �޸ļ�¼   
*/
/*========================================================================*/
class CDOMXmlNode_Message : public CDOMXmlNode
{
public:
	CDOMXmlNode_Message(const CDOMXmlNode_Document* pdocument);

	// -* ���ش��麯�� *-
	virtual bool appendChild(CDOMXmlNode* padd)	{ return false;}
	virtual bool appendPrevChild(CDOMXmlNode* pchild, CDOMXmlNode* padd) { return false; }
	virtual bool appendNextChild(CDOMXmlNode* pnode, CDOMXmlNode* padd) { return false;}

public:
	// -* ��֯xml�ڵ����� *-
	virtual bool oriXmlData(CNString& sresult) const
	{
		int m_nlevel=getLevel();
		int i=0;
		sresult += "\r\n";
		for(; i< m_nlevel; i++) {
			sresult += "	";
		}
		sresult += "<!--";
		sresult += m_svalue;
		sresult += "-->";
		return true;
	}
};

/*========================================================================*/
/**
*  @ ����     : CDOMXmlNode_Element
*  @ ��Ҫ���� : Element �ڵ�
*  @ �޸ļ�¼   
*/
/*========================================================================*/
class CDOMXmlNode_Element : public CDOMXmlNode
{
public:
	CDOMXmlNode_Element(const CDOMXmlNode_Document* pdocument);

public:
	// -* �趨 element ���� *-
	CNString& elementName(const char* sname) { return nodeValue(sname);}
	const CNString& elementName() const { return nodeValue(); }

	// -* �趨 element ���� *-
	CNString& elementValue(const char* savlue, const int nlen=-1)
	{
		m_elementValue.empty();
		if(nlen==-1)	m_elementValue.insert(0, savlue);
		else			m_elementValue.insert(0, savlue, nlen);
		return m_elementValue;
	}
	const CNString& elementValue() const { return m_elementValue; }

	// -* ���Խڵ� *-
	bool setAttrValue(const char* sname, const char* svalue, const int nvallen=-1, const char cinter='\"');
	const CNString* getAttrValue(const char* sname) const;

	// -* ��֯xml�ڵ����� *-
	virtual bool oriXmlData(CNString& sresult) const;

	// *- ��ѯ�ڵ� *-
	void findElementNode(CDomXmlNodeLink& nodelink, 
		const char* sname=NULL, 
		const char* sattrname=NULL, const char* sattrval=NULL, const int attrvallen=-1, 
		bool bdepth=true, bool bnextsibling=false) const;

	// *- ��ѯ��ĳ�ڵ�λ�� *-
	const CDOMXmlNode_Element* findElementNode(const CDOMXmlNode_Element* pcurnode=NULL, const char* sname=NULL, 
		const char* sattrname=NULL, const char* sattrval=NULL, const int sattrvallen=-1) const
	{
		const CDOMXmlNode* pstartnode;
		if(!pcurnode)
		{
			pstartnode = firstChildNode();
		}
		else
		{
			pstartnode = pcurnode->nextSiblingNode();
		}
		while(pstartnode)
		{
			if( pstartnode->nodeType() == EDOMXML_NODETYPE_ELEMENT)
			{
				const CDOMXmlNode_Element* pelementnode=(const CDOMXmlNode_Element*)pstartnode;
				if(pelementnode->examineNode(sname, sattrname, sattrval, sattrvallen))
					return pelementnode;
			}
			pstartnode = pstartnode->nextSiblingNode();
		}
		return NULL;
	}

private:
	bool examineNode(const char* sname=NULL, const char* sattrname=NULL, const char* sattrval=NULL, const int attrvallen=-1) const
	{
		if(sname)
		{
			if(sname != m_svalue) {
				return false;
			}
		}
		if(sattrname)
		{
			const CNString* inter_pattrval;
			CNString inter_strattrval;
			if(attrvallen > 0)		inter_strattrval.insert(0, sattrval, attrvallen);
			else					inter_strattrval=sattrval;

			if( ( (inter_pattrval=getAttrValue(sattrname)) == NULL ) ) {
				return false;
			}
			else if(*inter_pattrval != inter_strattrval) {
				return false;
			}

		}
		return true;
	}
	CNString m_elementValue;

	// -* ��ѯ���Խڵ�Ԫ�� *-
	const CDOMXmlNode_Attribute* findAttrNode(const char* sname) const;
};

/*========================================================================*/
/**
*  @ ����     : CDOMXmlNode_DocumentType
*  @ ��Ҫ���� : DocumentType �ڵ�
*  @ �޸ļ�¼   
*/
/*========================================================================*/
class CDOMXmlNode_DocumentType : public CDOMXmlNode
{
public:
	CDOMXmlNode_DocumentType(const CDOMXmlNode_Document* pdocument);

public:
	// -* �������� *-
	bool setAttrValue(const char* sname, const char* svalue, const int nvallen=-1, const char cinter='\"');

	// -* ��ȡ����ֵ *-
	CNString* getAttrValue(const char* sname);

	// -* ��֯xml�ڵ����� *-
	virtual bool oriXmlData(CNString& sresult) const;

private:
	// -* ��ѯ���Խڵ�Ԫ�� *-
	CDOMXmlNode_Attribute* findAttrNode(const char* sname) const;
};

/*========================================================================*/
/**
*  @ ����     : CDOMXmlNode_Document
*  @ ��Ҫ���� : Document �ڵ�
*  @ �޸ļ�¼   
*/
/*========================================================================*/
class CDOMXmlNode_Document : public CDOMXmlNode
{
public:
	CDOMXmlNode_Document() { m_type = EDOMXML_NODETYPE_DOCUMENT; m_proot=NULL; }

public:
	// -* ����Message *-
	CDOMXmlNode_Message* createMessage(const char* svalue, const int vallen);

	// -* ����Attribute *-
	CDOMXmlNode_Attribute* createAttribute(const char* sname, const char* svalue,
		const int vallen=-1, const char cinter='\"');
	// -* ����Element *-
	CDOMXmlNode_Element* createElement(const char* sname);

	// -* ����Element *-
	CDOMXmlNode_DocumentType* createDocumentType();

	// -* ��֯xml�ṹ������ *-
	virtual bool oriXmlData(CNString& sresult) const;

	virtual bool appendChild(CDOMXmlNode* padd)
	{
		if(padd->nodeType() == EDOMXML_NODETYPE_ELEMENT)
		{
			m_proot = (CDOMXmlNode_Element*) padd;
		}
		return CDOMXmlNode::appendChild(padd);
	}

	CDOMXmlNode_Element* rootElement(CDOMXmlNode_Element& elementroot)
	{
		m_proot = &elementroot;
		return m_proot;
	}

	const CDOMXmlNode_Element* rootElement() const
	{
		return m_proot;
	}

private:
	CDOMXmlNode_Element* m_proot;
};

class CDOMXmlExamine
{
public:
	// attribute data examine
	static bool IsAttributeInter(const char chinter);

	static bool IsDigit(const char ch);
	static bool IsLetter(const char ch);
	static bool IsSigns(const char ch);
	static bool IsAscii2(const char ch);
	static bool ISSpace(const char ch);
	static bool IsReserver(const char ch);

	static bool IsNameData(const char ch);

	static int Is_CDATA_Start(const char* lpsource, const int nmaxlen);
	static int Is_CDATA_End(const char* lpsource, const int nmaxlen);

	static int Is_Head_Start(const char* lpsource, const int nmaxlen);
	static int Is_Head_End(const char* lpsource, const int nmaxlen);

	static int Is_Instruct_Start(const char* lpsource, const int nmaxlen);
	static int Is_Instruct_End(const char* lpsource, const int nmaxlen);

	static int Is_Message_Start(const char* lpsource, const int nmaxlen);
	static int Is_Message_End(const char* lpsource, const int nmaxlen);

	static int Is_Declare_Start(const char* lpsource, const int nmaxlen, CNString& sresword);
	static int Is_Declare_End(const char* lpsource, const int nmaxlen);

	static int Is_Element_End(const char* lpsource, const int nmaxlen);

	static int Is_Reserver_Word(const char* lpsource, const int nmaxlen);

protected:
	static int Is_Str_Value(const char* lpsource, const int nmaxlen, const char* sexamine);
};

class CDomXmlAnsy
{
public:
	const CDOMXmlNode_Document* ownerDocument() const { return &m_documentnode;	}
	CDOMXmlNode_Document* ownerDocument() { return &m_documentnode;	}
	int ansyxmlsource(CNString& source);
	int ansyxmlfile(const char* sfname, const int npreread = 1024);
	int writexmlfile(const char* sfname);

protected:
	int ansyxmlhead(CDOMXmlNode* pparent, CNString& source, int& nstart);
	int ansyxmlattr(CDOMXmlNode* pparent, CNString& source, int& nstart);
	int ansyxmlelement(CDOMXmlNode* pparent, CNString& source, int& nstart);
	int ansyxmlmessage(CDOMXmlNode* pparent, CNString& source, int& nstart);

protected:
	int fetch_attribute(CNString& lpsource, int& nstart, CNString& sname, CNString& svalue, char& cinter);
	int fetch_name(CNString& lpsource, int& nstart, CNString& svalue);
	int fetch_space(CNString& lpsource, int& nstart);

	int fetch_value(CNString& ssource, int& nstart, const char* sfind, CNString* sfetch);
	int fetch_value(CNString& ssource, int& nstart, const char sfind, CNString* sfetch);

protected:
	CDOMXmlNode_Document m_documentnode;
};

#endif