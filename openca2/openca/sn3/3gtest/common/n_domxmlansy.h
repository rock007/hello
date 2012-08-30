/* -*- head file -*-*/

#ifndef DOMXML_ANSY_HEAD_DEFINE
#define DOMXML_ANSY_HEAD_DEFINE

#include "n_string.h"
#include "n_link.h"

/*========================================================================*/
/**
*  @ 枚举名   : EDOMXmlNodeB
*  @ 主要功能 : 枚举基本的节点类型
*  @ 修改纪录   
*/
/*========================================================================*/
enum EDOMXmlNodeType
{
	EDOMXML_NODETYPE_UNKNOWN=0,					// *- 未知 *-
	EDOMXML_NODETYPE_ATTRIBUTE,					// *- 属性 *-
	EDOMXML_NODETYPE_CDATA_SECTION,				// *- CDATA属性 *-
	EDOMXML_NODETYPE_MESSAGE,					// *- 注释 *-
	EDOMXML_NODETYPE_DOCUMENT_FRAGMENT_NODE,	// *- 文档片断 *-
	EDOMXML_NODETYPE_DOCUMENT,					// *- 文档 *-
	EDOMXML_NODETYPE_DOCUMENT_TYPE,				// *- 文档类型 *-
	EDOMXML_NODETYPE_ELEMENT,					// *- 元素 *-
	EDOMXML_NODETYPE_ENTITY,					// *- 实体 *-
	EDOMXML_NODETYPE_ENTITY_REFERENCE,			// *- 实体引用 *-
	EDOMXML_NODETYPE_NOTATION,					// *- Notation *-
	EDOMXML_NODETYPE_PROCESSING_INSTRUCTION,	// *- 指令处理 *-
	EDOMXML_NODETYPE_TEXT						// *- Text *-
};


class CDOMXmlNode_Document;
class CDOMXmlNode;
typedef CNLink<const CDOMXmlNode *> CDomXmlNodeLink;

/*========================================================================*/
/**
*  @ 类名     : CDOMXmlNode
*  @ 主要功能 : 基本节点定义
*  @ 修改纪录   
*/
/*========================================================================*/
class CDOMXmlNode
{
	// *- friend class declare *-
	friend class CDOMXmlNode_Document;

	// * ============================================================== *
	// @ 构造函数 
	// * ============================================================== *
public:
	CDOMXmlNode(const CDOMXmlNode_Document* pdocument=NULL);
	virtual ~CDOMXmlNode();

	// *- 组合xml文件格式串 *-
	virtual bool oriXmlData(CNString& sresult) const;

	// * ============================================================== *
	// @ 节点数据定义 
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
	// -* 节点数据 *-
	virtual CNString& nodeValue(const char* svalue, const int nlen=-1);
	virtual const CNString& nodeValue() const {	return m_svalue; }

	// -* 获取节点类型 *-
	virtual EDOMXmlNodeType nodeType(const EDOMXmlNodeType type) { m_type = type; return m_type; }
	virtual const EDOMXmlNodeType nodeType() const { return m_type;	}

	// -* 归属文档 *-
	virtual CDOMXmlNode_Document* ownerDocument(const CDOMXmlNode_Document* pdocument) { m_powner = (CDOMXmlNode_Document*)pdocument; return m_powner; }
	virtual const CDOMXmlNode_Document* ownerDocument() const { return m_powner; }

	// -* 父节点 *-
	virtual CDOMXmlNode* parentNode(const CDOMXmlNode* pparent) { m_pparent=(CDOMXmlNode *)pparent; return m_pparent; }
	virtual const CDOMXmlNode* parentNode() const{ return m_pparent; }

	// -* 左兄弟节点 *-
	virtual CDOMXmlNode* prevSiblingNode(const CDOMXmlNode* prevSibling) { m_pprev_sibling=(CDOMXmlNode *)prevSibling; return m_pprev_sibling; }
	virtual const CDOMXmlNode* prevSiblingNode() const { return m_pprev_sibling; }

	// -* 右兄弟节点 *-
	virtual CDOMXmlNode* nextSiblingNode(const CDOMXmlNode* nextSibling) { m_pnext_sibling=(CDOMXmlNode *)nextSibling; return m_pnext_sibling; }
	virtual const CDOMXmlNode* nextSiblingNode() const { return m_pnext_sibling; }

	// -* 第一子节点 *-
	virtual CDOMXmlNode* firstChildNode(const CDOMXmlNode* pchild) { m_pchild=(CDOMXmlNode *)pchild; return m_pchild; }
	virtual const CDOMXmlNode* firstChildNode() const	{ return m_pchild; }

	// -* 最后一子节点 *-
	virtual const CDOMXmlNode* lastChildNode() const;
public:
	// -* 添加子节点 *-
	virtual bool appendChild(CDOMXmlNode* padd);

	// -* 添加孩子上一节点 *-
	virtual bool appendPrevChild(CDOMXmlNode* pchild, CDOMXmlNode* padd);

	// -* 添加孩子下一节点 *-
	virtual bool appendNextChild(CDOMXmlNode* pnode, CDOMXmlNode* padd);

	// -* 删除当前节点 *-
	virtual bool moveThis(bool delnextsibling);

	// -* 复制节点 *-
	virtual CDOMXmlNode* cloneNode(CDOMXmlNode* pparent, CDOMXmlNode_Document* pdocument, bool bnextsibling=false) const;

	// *- 获取层次 *-
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
*  @ 类名     : CDOMXmlNode_Attribute
*  @ 主要功能 : Attribute 节点
*  @ 修改纪录   
*/
/*========================================================================*/
class CDOMXmlNode_Attribute : public CDOMXmlNode
{
public:
	CDOMXmlNode_Attribute(const CDOMXmlNode_Document* pdocument, const char* sattrname, const char cinter='\"');

public:
	// -* 组织xml节点数据 *-
	virtual bool oriXmlData(CNString& sresult) const;

	// -* 引用符号 *-
	char attrInter(const char cinter) { m_cinter = cinter; return m_cinter; }
	const char attrInter() const { return m_cinter; }

	// -* 属性名称 *-
	CNString& attrName(const char* sattrname) { return nodeValue(sattrname); }
	const CNString& attrName() const { return nodeValue(); }

	// -* 属性值 *-
	CNString& attrValue(const char* sattrvalue, const int nlen=-1) { 
		m_sattrValue.empty();
		if(nlen == -1)	m_sattrValue.insert(0, sattrvalue);
		else			m_sattrValue.insert(0, sattrvalue, nlen);
		return m_sattrValue;
	}
	const CNString& attrValue() const { return m_sattrValue; }

	// -* 重载存虚函数 *-
	virtual bool appendChild(CDOMXmlNode* padd)	{ return false;}
	virtual bool appendPrevChild(CDOMXmlNode* pchild, CDOMXmlNode* padd) { return false; }
	virtual bool appendNextChild(CDOMXmlNode* pnode, CDOMXmlNode* padd) { return false;}

private:
	char m_cinter;
	CNString m_sattrValue;
};

/*========================================================================*/
/**
*  @ 类名     : CDOMXmlNode_Message
*  @ 主要功能 : Message 节点
*  @ 修改纪录   
*/
/*========================================================================*/
class CDOMXmlNode_Message : public CDOMXmlNode
{
public:
	CDOMXmlNode_Message(const CDOMXmlNode_Document* pdocument);

	// -* 重载存虚函数 *-
	virtual bool appendChild(CDOMXmlNode* padd)	{ return false;}
	virtual bool appendPrevChild(CDOMXmlNode* pchild, CDOMXmlNode* padd) { return false; }
	virtual bool appendNextChild(CDOMXmlNode* pnode, CDOMXmlNode* padd) { return false;}

public:
	// -* 组织xml节点数据 *-
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
*  @ 类名     : CDOMXmlNode_Element
*  @ 主要功能 : Element 节点
*  @ 修改纪录   
*/
/*========================================================================*/
class CDOMXmlNode_Element : public CDOMXmlNode
{
public:
	CDOMXmlNode_Element(const CDOMXmlNode_Document* pdocument);

public:
	// -* 设定 element 名称 *-
	CNString& elementName(const char* sname) { return nodeValue(sname);}
	const CNString& elementName() const { return nodeValue(); }

	// -* 设定 element 数据 *-
	CNString& elementValue(const char* savlue, const int nlen=-1)
	{
		m_elementValue.empty();
		if(nlen==-1)	m_elementValue.insert(0, savlue);
		else			m_elementValue.insert(0, savlue, nlen);
		return m_elementValue;
	}
	const CNString& elementValue() const { return m_elementValue; }

	// -* 属性节点 *-
	bool setAttrValue(const char* sname, const char* svalue, const int nvallen=-1, const char cinter='\"');
	const CNString* getAttrValue(const char* sname) const;

	// -* 组织xml节点数据 *-
	virtual bool oriXmlData(CNString& sresult) const;

	// *- 查询节点 *-
	void findElementNode(CDomXmlNodeLink& nodelink, 
		const char* sname=NULL, 
		const char* sattrname=NULL, const char* sattrval=NULL, const int attrvallen=-1, 
		bool bdepth=true, bool bnextsibling=false) const;

	// *- 查询到某节点位置 *-
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

	// -* 查询属性节点元素 *-
	const CDOMXmlNode_Attribute* findAttrNode(const char* sname) const;
};

/*========================================================================*/
/**
*  @ 类名     : CDOMXmlNode_DocumentType
*  @ 主要功能 : DocumentType 节点
*  @ 修改纪录   
*/
/*========================================================================*/
class CDOMXmlNode_DocumentType : public CDOMXmlNode
{
public:
	CDOMXmlNode_DocumentType(const CDOMXmlNode_Document* pdocument);

public:
	// -* 设置属性 *-
	bool setAttrValue(const char* sname, const char* svalue, const int nvallen=-1, const char cinter='\"');

	// -* 读取属性值 *-
	CNString* getAttrValue(const char* sname);

	// -* 组织xml节点数据 *-
	virtual bool oriXmlData(CNString& sresult) const;

private:
	// -* 查询属性节点元素 *-
	CDOMXmlNode_Attribute* findAttrNode(const char* sname) const;
};

/*========================================================================*/
/**
*  @ 类名     : CDOMXmlNode_Document
*  @ 主要功能 : Document 节点
*  @ 修改纪录   
*/
/*========================================================================*/
class CDOMXmlNode_Document : public CDOMXmlNode
{
public:
	CDOMXmlNode_Document() { m_type = EDOMXML_NODETYPE_DOCUMENT; m_proot=NULL; }

public:
	// -* 创建Message *-
	CDOMXmlNode_Message* createMessage(const char* svalue, const int vallen);

	// -* 创建Attribute *-
	CDOMXmlNode_Attribute* createAttribute(const char* sname, const char* svalue,
		const int vallen=-1, const char cinter='\"');
	// -* 创建Element *-
	CDOMXmlNode_Element* createElement(const char* sname);

	// -* 创建Element *-
	CDOMXmlNode_DocumentType* createDocumentType();

	// -* 组织xml结构体数据 *-
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
