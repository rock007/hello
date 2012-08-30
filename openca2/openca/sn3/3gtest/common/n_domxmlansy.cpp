/* -*- source file -*-*/

#include "n_domxmlansy.h"

// * ============================================================== *
// @ 功能描述: 删除空格
// * ============================================================== *
int CDomXmlAnsy::fetch_space(CNString& ssource, int& nstart)
{
	for(;nstart < ssource.get_length(); nstart++) {
		if(!CDOMXmlExamine::ISSpace(ssource[nstart]))
			return 0;
	}
	return -1;
}

// * ============================================================== *
// @ 功能描述: 
// * ============================================================== *
int CDomXmlAnsy::fetch_name(CNString& ssource, int& nstart, CNString& svalue)
{
	for(;nstart < ssource.get_length(); nstart++) {
		if(!CDOMXmlExamine::IsNameData(ssource[nstart]))
			return 0;
		svalue += ssource[nstart];
	}
	return -1;
}

// * ============================================================== *
// @ 功能描述: 
// * ============================================================== *
int CDomXmlAnsy::fetch_attribute(CNString& ssource, int& nstart, CNString& sname, CNString& svalue, char& cinter)
{
	fetch_space(ssource, nstart);

	if(ssource[nstart] == '>')		return 1;
	if(ssource[nstart] == '?')		return 1;
	if(ssource[nstart] == '/')		return 1;
	if(nstart >= ssource.get_length())			return 1;

	fetch_name(ssource, nstart, sname);

	fetch_space(ssource, nstart);

	if(ssource[nstart] != '=')		return -111;
	nstart++;
	if(nstart > ssource.get_length())			return -112;
	fetch_space(ssource, nstart);

	cinter = ssource[nstart];
	nstart++;
	if(nstart > ssource.get_length())			return -113;

	if(!CDOMXmlExamine::IsAttributeInter(cinter))		return -114;
	fetch_value(ssource, nstart, cinter, &svalue);

	return 0;
}

int CDomXmlAnsy::fetch_value(CNString& ssource, int& nstart, const char* sfind, CNString* sfetch)
{
	int nend = ssource.find(nstart, sfind);
	if(nend > 0)
	{
		if(sfetch)
		{
			sfetch->empty();
			sfetch->insert(0, ssource.c_str()+nstart, nend-nstart);
		}
		nstart = nend;
		nstart += (int)(strlen(sfind));
		return 0;
	}
	return -1;
}

int CDomXmlAnsy::fetch_value(CNString& ssource, int& nstart, const char sfind, CNString* sfetch)
{
	int nend = ssource.find(nstart, sfind);
	if(nend > 0)
	{
		if(sfetch)
		{
			sfetch->empty();
			sfetch->insert(0, ssource.c_str()+nstart, nend-nstart);
		}
		nstart = nend;
		nstart += 1;
		return 0;
	}
	return -1;
}

int CDomXmlAnsy::ansyxmlattr(CDOMXmlNode* pparent, CNString& ssource, int& nstart)
{
	CDOMXmlNode_Attribute* pnode = NULL;
	CNString sname, svalue;
	char cinter;
	int ret;
	while(1) {
		sname.empty();
		svalue.empty();
		ret = fetch_attribute(ssource, nstart, sname, svalue, cinter);
		if(ret < 0)		return ret;
		if(ret == 1)	return 0;
		if((pnode=m_documentnode.createAttribute(sname.c_str(), svalue.c_str(),svalue.get_length(), cinter)) == NULL)
			return -210;
		if(!pparent->appendChild(pnode))
			return -211;
	}
	return 0;
}

// * ============================================================== *
// @ 功能描述: 解析xml文档类型
// * ============================================================== *
int CDomXmlAnsy::ansyxmlhead(CDOMXmlNode* pparent, CNString& ssource, int& nstart)
{
	int ret, nlen = 0;
	CDOMXmlNode_DocumentType* phead = NULL;
	if( (nlen=CDOMXmlExamine::Is_Head_Start(ssource.c_str()+nstart, ssource.get_length())) < 0) {
		return -200;
	}
	nstart += nlen;
	if(nstart >= ssource.get_length())			return -201;
	if( (phead = ownerDocument()->createDocumentType()) == NULL) {
		return -202;
	}

	if(!pparent->appendChild(phead)) {
		return -203;
	}

	if( (ret=ansyxmlattr(phead, ssource, nstart)) != 0)
		return ret;

	fetch_space(ssource, nstart);

	if(ssource[nstart] != '?' || ssource[nstart+1] != '>') {
		return -204;
	}
	nstart += 2;
	fetch_space(ssource, nstart);
	return 0;
}

int CDomXmlAnsy::ansyxmlelement(CDOMXmlNode* pparent, CNString& ssource, int& nstart)
{
	int ret;
	CNString stemp, sname;
	CDOMXmlNode_Element* pelement;
	fetch_space(ssource, nstart);

	if(ssource[nstart] != '<')				return -220;
	nstart++;
	if(fetch_name(ssource, nstart, sname))
		return -221;
	if(sname.get_length() <= 0)				return -222;

	pelement = ownerDocument()->createElement(sname.c_str());
	if(!pelement)							return -223;
	if(!pparent->appendChild(pelement))		return -224;

	fetch_space(ssource, nstart);
	// ansy element attribute
	ret = ansyxmlattr(pelement, ssource, nstart);
	if(ret)		return ret;

	if(ssource[nstart] == '/') {
		if(ssource[nstart+1] == '>') {
			nstart += 2;
			fetch_space(ssource, nstart);
			return 0;
		}
		else
			return -225;
	}
	if(ssource[nstart] != '>')		return -226;
	nstart++;

	fetch_space(ssource, nstart);

	if(ssource[nstart] != '<')
	{
		CNString sdata;
		if(fetch_value(ssource, nstart, "</", &sdata))
			return -227;
		nstart -= 2;
		if(ssource.get_length() < nstart)	return -228;
		if(sdata.get_length() > 0) {
			pelement->elementValue(sdata.c_str(), sdata.get_length());
		}
	}

	while(ssource[nstart] == '<' && ssource[nstart+1] != '/')
	{
		if(CDOMXmlExamine::Is_Message_Start(ssource.c_str()+nstart, ssource.get_length()) > 0)
		{
			ret = ansyxmlmessage(pelement, ssource, nstart);
			if(ret)		return ret;
		}
		// ansy element child data
		else
		{
			if((ret=ansyxmlelement(pelement, ssource, nstart)))
				return ret;
		}
	}

	// examine element end
	sname.insert(0, "</");
	sname += ">";
	if(memcmp(sname.c_str(), ssource.c_str()+nstart, sname.get_length()))	return -229;
	nstart += sname.get_length();

	fetch_space(ssource, nstart);

	return 0;
}

// * ============================================================== *
// @ 功能描述: 解析xml解析信息  <!-- --->
// * ============================================================== *
int CDomXmlAnsy::ansyxmlmessage(CDOMXmlNode* pparent, CNString& ssource, int& nstart)
{
	int ret;
	CNString sresword;

	ret = CDOMXmlExamine::Is_Message_Start(ssource.c_str()+nstart, ssource.get_length());
	if(ret < 0)					return -240;
	nstart += ret;

	int nend = ssource.find(nstart, "-->");
	CDOMXmlNode_Message* pnodemsg=ownerDocument()->createMessage(ssource.c_str()+nstart, nend-nstart);
	if(!pparent->appendChild(pnodemsg))
	{
		return -241;
	}

	nstart = (int)(nend+strlen("-->"));
	fetch_space(ssource, nstart);

	return 0;
}

// * ============================================================== *
// @ 功能描述: 解析xml字符串
// * ============================================================== *
int CDomXmlAnsy::ansyxmlsource(CNString& ssource)
{
	m_documentnode.moveThis(false);
	int nstart = 0, ret;
	CNString stemp;
	while(1)
	{
		stemp.empty();
		if(nstart >= ssource.get_length())	break;
		if(CDOMXmlExamine::Is_Head_Start(ssource.c_str()+nstart, ssource.get_length()) > 0)
		{
			ret = ansyxmlhead(&m_documentnode, ssource, nstart);
			if(ret)		return ret;
		}
		else if(CDOMXmlExamine::Is_Message_Start(ssource.c_str()+nstart, ssource.get_length()) > 0)
		{
			ret = ansyxmlmessage(&m_documentnode, ssource, nstart);
			if(ret)		return ret;
		}
		else
		{
			ret = ansyxmlelement(&m_documentnode, ssource, nstart);
			if(ret)		return ret;
		}
	}
	return 0;
}

// * ============================================================== *
// @ 功能描述: 解析xml 文件
// * ============================================================== *
int CDomXmlAnsy::ansyxmlfile(const char* sfname, const int npreread)
{
	CNString sansyxml;
	if(sansyxml.readfile(sfname, npreread))		return -1;
	return ansyxmlsource(sansyxml);
}

// * ============================================================== *
// @ 功能描述: 写入xml 文件
// * ============================================================== *
int CDomXmlAnsy::writexmlfile(const char* sfname)
{
	CNString sresult;
	if( (m_documentnode.oriXmlData(sresult)) )
		return sresult.writefile(sfname);
	return -1;
}

// * ============================================================== *
// @ 功能描述: 判断xml属性字符
// * ============================================================== *
bool CDOMXmlExamine::IsAttributeInter(const char chinter)
{
	if(chinter == '\"' || chinter == '\'')
		return true;
	return false;
}

// * ============================================================== *
// @ 功能描述: IsDigit
// * ============================================================== *
bool CDOMXmlExamine::IsDigit(const char ch)
{
	if(ch >= '0' && ch <= '9')
		return true;
	return false;
}

// * ============================================================== *
// @ 功能描述: IsLetter
// * ============================================================== *
bool CDOMXmlExamine::IsLetter(const char ch)
{
	if( (ch >= 'a' && ch <= 'z') ||
		(ch >= 'A' && ch <= 'Z') )
		return true;
	return false;
}

// * ============================================================== *
// @ 功能描述: IsSigns
// * ============================================================== *
bool CDOMXmlExamine::IsSigns(const char ch)
{
	static const char* s_sSigns = "[~!@#%^&*()?:;\"\',./-_+=|\\";
	int i;
	int nlen = (int)(strlen(s_sSigns));
	for(i = 0; i < nlen; i++)
	{
		if(ch == s_sSigns[i])
			return true;
	}
	return false;
}

// * ============================================================== *
// @ 功能描述: IsAscii2
// * ============================================================== *
bool CDOMXmlExamine::IsAscii2(const char ch)
{
	if( ((unsigned char)ch >= 0x80) && ( (unsigned char)ch < 0xFF) )
		return true;
	return false;
}

// * ============================================================== *
// @ 功能描述: ISSpace
// * ============================================================== *
bool CDOMXmlExamine::ISSpace(const char ch)
{
	if(ch == ' ' || ch == '	' || ch == '\n' || ch == '\r')
		return true;
	return false;
}

// * ============================================================== *
// @ 功能描述: IsReserver
// * ============================================================== *
bool CDOMXmlExamine::IsReserver(const char ch)
{
	if(ch == '<' || ch == '>')
		return true;
	return false;
}

// * ============================================================== *
// @ 功能描述: IsNameData
// * ============================================================== *
bool CDOMXmlExamine::IsNameData(const char ch)
{
	if(CDOMXmlExamine::ISSpace(ch))
		return false;
	if(CDOMXmlExamine::IsReserver(ch))
		return false;
	if(CDOMXmlExamine::IsSigns(ch))
	{
		if(ch != '_' && ch != '-' && ch != ':' && ch != '.')
			return false;
	}
	return true;
}

// * ============================================================== *
// @ 功能描述: Is_Str_Value
// * ============================================================== *
int CDOMXmlExamine::Is_Str_Value(const char* lpsource, const int nmaxlen, const char* sexam)
{
	int nlen = (int)(strlen(sexam));
	if(!memcmp(lpsource, sexam, nlen))
		return nlen;
	return -1;
}

// * ============================================================== *
// @ 功能描述: Is_Reserver_Word
// * ============================================================== *
int CDOMXmlExamine::Is_Reserver_Word(const char* lpsource, const int nmaxlen)
{
	static const char* sRes_Word[] ={ "DOCTYPE", "ELEMENT", "NOTATION" };
	for(int i = 0; i < 3; i++)
	{
		int nlen = Is_Str_Value(lpsource, nmaxlen, sRes_Word[i]);
		if(nlen > 0)
			return nlen;
	}
	return -1;
}

// * ============================================================== *
// @ 功能描述: Is_Element_End
// * ============================================================== *
int CDOMXmlExamine::Is_Element_End(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "</");
}

// * ============================================================== *
// @ 功能描述: Is_CDATA_Start
// * ============================================================== *
int CDOMXmlExamine::Is_CDATA_Start(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "<![CDATA[");
}

// * ============================================================== *
// @ 功能描述: Is_CDATA_End
// * ============================================================== *
int CDOMXmlExamine::Is_CDATA_End(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "]]>");
}

// * ============================================================== *
// @ 功能描述: Is_Head_Start
// * ============================================================== *
int CDOMXmlExamine::Is_Head_Start(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "<?xml");
}

// * ============================================================== *
// @ 功能描述: Is_Head_End
// * ============================================================== *
int CDOMXmlExamine::Is_Head_End(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "?>");
}

// * ============================================================== *
// @ 功能描述: Is_Instruct_Start
// * ============================================================== *
int CDOMXmlExamine::Is_Instruct_Start(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "<?");
}

// * ============================================================== *
// @ 功能描述: Is_Instruct_End
// * ============================================================== *
int CDOMXmlExamine::Is_Instruct_End(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "?>");
}

// * ============================================================== *
// @ 功能描述: Is_Declare_Start
// * ============================================================== *
int CDOMXmlExamine::Is_Declare_Start(const char* lpsource, const int nmaxlen, CNString& sresword)
{
	int nlen, ret;
	nlen = Is_Str_Value(lpsource, nmaxlen, "<!");
	if(nlen < 0)	return nlen;
	ret = Is_Reserver_Word(lpsource+nlen, nmaxlen);
	if(ret < 0)	return ret;
	sresword.empty();
	sresword.insert(0, lpsource+2, ret);
	return nlen+ret;
}

// * ============================================================== *
// @ 功能描述: Is_Declare_End
// * ============================================================== *
int CDOMXmlExamine::Is_Declare_End(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, ">");
}

// * ============================================================== *
// @ 功能描述: Is_Message_Start
// * ============================================================== *
int CDOMXmlExamine::Is_Message_Start(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "<!--");
}

// * ============================================================== *
// @ 功能描述: Is_Message_End
// * ============================================================== *
int CDOMXmlExamine::Is_Message_End(const char* lpsource, const int nmaxlen)
{
	return Is_Str_Value(lpsource, nmaxlen, "-->");
}


// * ============================================================== *
// @ 功能描述: CDOMmlNode初始化
// * ============================================================== *
CDOMXmlNode::CDOMXmlNode(const CDOMXmlNode_Document* pdocument)
{
	m_type = EDOMXML_NODETYPE_UNKNOWN;
	m_svalue.empty();
	m_pparent = m_pchild = NULL;
	m_pprev_sibling = m_pnext_sibling = NULL;
	m_powner = (CDOMXmlNode_Document *)pdocument;
}

// * ============================================================== *
// @ 功能描述: CDOMmlNode摧毁 
// * ============================================================== *
CDOMXmlNode::~CDOMXmlNode()
{
	moveThis(false);
}

// * ============================================================== *
// @ 功能描述: 获取节点名称
// * ============================================================== *
CNString& CDOMXmlNode::nodeValue(const char* svalue, const int nlen)
{
	m_svalue.empty();
	if(nlen == -1) {
		m_svalue.insert(0, svalue);
	}
	else {
		m_svalue.insert(0, svalue, nlen);
	}
	return m_svalue;
}

// * ============================================================== *
// @ 功能描述: 复制节点
// * ============================================================== *
CDOMXmlNode* CDOMXmlNode::cloneNode(CDOMXmlNode* pparent, CDOMXmlNode_Document* pdocument, bool bnextsibling) const
{
	CDOMXmlNode* pnew=NULL;
	switch(m_type)
	{
	case EDOMXML_NODETYPE_ATTRIBUTE:
		{
			CDOMXmlNode_Attribute* pcurattr = (CDOMXmlNode_Attribute *)this;
			CDOMXmlNode_Attribute* pattr = new CDOMXmlNode_Attribute(pdocument, pcurattr->attrName().c_str(), pcurattr->attrInter());
			pattr->attrValue(pcurattr->attrValue().c_str(), pcurattr->attrValue().get_length());
			pnew = pattr;
		}
		break;
	case EDOMXML_NODETYPE_ELEMENT:
		{
			CDOMXmlNode_Element* pelement = new CDOMXmlNode_Element(pdocument);
			CDOMXmlNode_Element* pcurelement = (CDOMXmlNode_Element *)this;
			pcurelement->elementValue(pcurelement->elementValue().c_str(), pcurelement->elementValue().get_length());
			pnew = pelement;
		}
		break;
	case EDOMXML_NODETYPE_DOCUMENT_FRAGMENT_NODE:
		{
			pnew = (new CDOMXmlNode_DocumentType(pdocument));
		}
		break;
	case EDOMXML_NODETYPE_MESSAGE:
		{
			pnew = (new CDOMXmlNode_Message(pdocument));
		}
		break;
	default:
		return NULL;
	}
	pnew->m_svalue=m_svalue;
	pnew->m_type=m_type;
	pnew->m_powner=pdocument;
	pnew->m_pparent=pparent;
	// -* clone child *-
	if(m_pchild)
	{
		pnew->m_pchild=m_pchild->cloneNode(pnew, pdocument, true);
	}
	// -* clone next sibling *-
	if(bnextsibling && m_pnext_sibling)
	{
		pnew->m_pnext_sibling=m_pnext_sibling->cloneNode(pparent, pdocument, true);
		pnew->m_pnext_sibling->m_pprev_sibling = pnew;
	}
	return pnew;
}

// * ============================================================== *
// @ 功能描述: 添加元素 -- child
// * ============================================================== *
bool CDOMXmlNode::appendChild(CDOMXmlNode* padd)
{
	if(!padd)
	{
		return true;
	}

	CDOMXmlNode* pchild=(CDOMXmlNode *)lastChildNode();
	if(!m_pchild)
	{
		m_pchild=padd;
		padd->m_pprev_sibling=NULL;
	}
	else
	{
		pchild->m_pnext_sibling=padd;
		padd->m_pprev_sibling=pchild;
	}
	padd->m_pnext_sibling=NULL;
	padd->m_pparent = this;
	return true;
}

// * ============================================================== *
// @ 功能描述: 添加元素 -- prev sibling
// * ============================================================== *
bool CDOMXmlNode::appendPrevChild(CDOMXmlNode* pchild, CDOMXmlNode* padd)
{
	if(!padd || !pchild || pchild->m_pparent != this)
	{
		return true;
	}

	// *- add node *-
	padd->m_pparent=this;
	padd->m_powner=m_powner;

	padd->m_pnext_sibling = pchild;
	padd->m_pprev_sibling = pchild->m_pprev_sibling;
	if(pchild->m_pprev_sibling)
	{
		pchild->m_pprev_sibling->m_pnext_sibling = padd;
	}
	pchild->m_pprev_sibling = padd;
	if(m_pchild == pchild)
	{
		m_pchild = padd;
	}
	return true;
}

// * ============================================================== *
// @ 功能描述: 添加元素 -- next sibling
// * ============================================================== *
bool CDOMXmlNode::appendNextChild(CDOMXmlNode* pchild, CDOMXmlNode* padd)
{
	if(!padd || !pchild || pchild->m_pparent != this)
	{
		return true;
	}

	// *- add node *-
	padd->m_pparent=this;
	padd->m_powner=m_powner;

	padd->m_pprev_sibling=pchild;
	padd->m_pnext_sibling=pchild->m_pnext_sibling;
	if(pchild->m_pnext_sibling)
	{
		pchild->m_pnext_sibling->m_pprev_sibling=padd;
	}
	pchild->m_pnext_sibling = padd;
	return true;
}

// * ============================================================== *
// @ 功能描述: 删除当前节点
// * ============================================================== *
bool CDOMXmlNode::moveThis(bool delnextsibling)
{
	if(m_pchild)
	{
		CDOMXmlNode* pdelnode=m_pchild;
		pdelnode->moveThis(true);
		delete pdelnode;
	}
	if(delnextsibling && m_pnext_sibling)	// -* delete sibling node *-
	{
		CDOMXmlNode* pdelnode=m_pnext_sibling;
		pdelnode->moveThis(true);
		delete pdelnode;
	}

	// -* move this from the DOMXML tree *-
	if(m_pparent && (m_pparent->m_pchild == this) )
	{
		m_pparent->m_pchild = m_pnext_sibling;
	}
	else
	{
		if(m_pprev_sibling)
		{
			m_pprev_sibling->m_pnext_sibling=m_pnext_sibling;
		}
		if(m_pnext_sibling)
		{
			m_pnext_sibling->m_pprev_sibling=m_pprev_sibling;
		}
	}
	return true;
}

// * ============================================================== *
// @ 功能描述: 组织xml节点数据
// * ============================================================== *
bool CDOMXmlNode::oriXmlData(CNString& sresult) const
{
	if(m_pchild)
	{
		m_pchild->oriXmlData(sresult);
	}
	if(m_pnext_sibling)
	{
		m_pnext_sibling->oriXmlData(sresult);
	}
	return true;
}

// * ============================================================== *
// @ 功能描述: 获取最后一子节点
// * ============================================================== *
const CDOMXmlNode* CDOMXmlNode::lastChildNode() const
{
	CDOMXmlNode* pnode=m_pchild;
	if(!pnode)	return pnode;
	while(pnode->m_pnext_sibling)
	{
		pnode = pnode->m_pnext_sibling;
	}
	return pnode;
}

// * ============================================================== *
// @ 功能描述: CDOMXmlNode_Attribute
// * ============================================================== *
CDOMXmlNode_Attribute::CDOMXmlNode_Attribute(const CDOMXmlNode_Document* pdocument, const char* sattrname, const char cinter)
{
	m_powner=(CDOMXmlNode_Document *)pdocument;
	m_type=EDOMXML_NODETYPE_ATTRIBUTE;
	m_svalue = sattrname;
	m_cinter=cinter;
}

// * ============================================================== *
// @ 功能描述: 组织xml数据
// * ============================================================== *
bool CDOMXmlNode_Attribute::oriXmlData(CNString& sresult) const
{
	sresult += " ";
	sresult += attrName();
	sresult += "=";
	sresult += m_cinter;
	sresult += attrValue();
	sresult += m_cinter;
	return true;
}

// * ============================================================== *
// @ 功能描述: CDOMXmlNode_Message
// * ============================================================== *
CDOMXmlNode_Message::CDOMXmlNode_Message(const CDOMXmlNode_Document* pdocument)
{
	m_powner=(CDOMXmlNode_Document *)pdocument;
	m_type=EDOMXML_NODETYPE_MESSAGE;
}

// * ============================================================== *
// @ 功能描述: CDOMXmlNode_Element
// * ============================================================== *
CDOMXmlNode_Element::CDOMXmlNode_Element(const CDOMXmlNode_Document* pdocument)
{
	m_type=EDOMXML_NODETYPE_ELEMENT;
	m_powner=(CDOMXmlNode_Document *)pdocument;
}

// * ============================================================== *
// @ 功能描述: 查询属性节点位置
// * ============================================================== *
const CDOMXmlNode_Attribute* CDOMXmlNode_Element::findAttrNode(const char* sname) const
{
	const CDOMXmlNode* pxmlnode=m_pchild;
	while(pxmlnode)
	{
		if( (pxmlnode->nodeType() == EDOMXML_NODETYPE_ATTRIBUTE) &&
			(pxmlnode->nodeValue() == sname) )
		{
			break;
		}
		pxmlnode=pxmlnode->nextSiblingNode();
	}
	return (const CDOMXmlNode_Attribute *)pxmlnode;
}

// * ============================================================== *
// @ 功能描述: 读取节点属性值
// * ============================================================== *
const CNString* CDOMXmlNode_Element::getAttrValue(const char* sname) const
{
	// find attribute node position
	const CDOMXmlNode_Attribute* pattr=(const CDOMXmlNode_Attribute *)findAttrNode(sname);
	if(pattr)
	{
		return &pattr->attrValue();
	}
	return NULL;
}

// * ============================================================== *
// @ 功能描述: 设置属性节点
// * ============================================================== *
bool CDOMXmlNode_Element::setAttrValue(const char* sname, const char* svalue, const int nvallen, 
									   const char cinter)
{
	// find attribute node position
	CDOMXmlNode_Attribute* pattr=(CDOMXmlNode_Attribute *)findAttrNode(sname);
	if(!pattr) {
		if((pattr = m_powner->createAttribute(sname, svalue, nvallen, cinter)) == NULL) {
			return false;
		}
		if(!appendChild(pattr)) {
			delete pattr;
			return false;
		}
	}
	pattr->attrValue(svalue, nvallen);
	pattr->attrInter(cinter);
	return true;
}

// * ============================================================== *
// @ 功能描述: 组织节点数据
// * ============================================================== *
bool CDOMXmlNode_Element::oriXmlData(CNString& sresult) const
{
	// 处理level
	int m_nlevel = getLevel();

	int i;
	sresult+="\r\n";
	for(i=1; i < m_nlevel; i++)
	{
		sresult+="	";
	}
	sresult += "<";
	sresult += m_svalue;
	const CDOMXmlNode* pcurnode=m_pchild;
	bool bhave=false;
	while(pcurnode)
	{
		if(pcurnode->nodeType() == EDOMXML_NODETYPE_ATTRIBUTE)
		{
			CDOMXmlNode_Attribute* pexecnode = (CDOMXmlNode_Attribute *)pcurnode;
			pexecnode->oriXmlData(sresult);
		}
		else
		{
			bhave=true;
		}
		pcurnode = pcurnode->nextSiblingNode();
	}
	if(!bhave)
	{
		sresult += "/>";
		return true;
	}
	sresult += elementValue();

	sresult += ">";
	pcurnode=m_pchild;
	while(pcurnode)
	{
		if(pcurnode->nodeType() == EDOMXML_NODETYPE_ELEMENT || pcurnode->nodeType() == EDOMXML_NODETYPE_MESSAGE)
		{
			pcurnode->oriXmlData(sresult);
		}
		pcurnode = pcurnode->nextSiblingNode();
	}
	sresult+="\r\n";
	for(i=1; i < m_nlevel; i++)
	{
		sresult+="	";
	}
	sresult += "</";
	sresult += m_svalue;
	sresult += ">";
	return true;
}

// * ============================================================== *
// @ 功能描述: 查询节点
// * ============================================================== *
void CDOMXmlNode_Element::findElementNode(CDomXmlNodeLink& nodelink, 
										  const char* sname, 
										  const char* sattrname, const char* sattrval, const int attrvallen, 
										  bool bdepth, bool bnextsibling) const
{
	if(this->examineNode(sname, sattrname, sattrval, attrvallen))
	{
		// add node to the Link
		NPOSITION pos = nodelink.InsertTail();
		nodelink[pos] = this;
	}

	if(bdepth) {
		if(m_pchild) {
			((CDOMXmlNode_Element *)m_pchild)->findElementNode(nodelink, sname, sattrname, sattrval, attrvallen, bdepth, true);
		}
	}
	if(bnextsibling) {
		if(m_pnext_sibling)
		{
			((CDOMXmlNode_Element *)m_pnext_sibling)->findElementNode(nodelink, sname, sattrname, sattrval, attrvallen, bdepth, true);
		}
	}
}

// * ============================================================== *
// @ 功能描述:
// * ============================================================== *
CDOMXmlNode_DocumentType::CDOMXmlNode_DocumentType(const CDOMXmlNode_Document* pdocument)
{
	m_type=EDOMXML_NODETYPE_DOCUMENT_TYPE;
	m_powner=(CDOMXmlNode_Document *)pdocument;
}

// * ============================================================== *
// @ 功能描述: 设定属性值
// * ============================================================== *
bool CDOMXmlNode_DocumentType::setAttrValue(const char* sname, const char* svalue, const int nvallen, const char cinter)
{
	CDOMXmlNode_Attribute* pattrnode=findAttrNode(sname);
	if(pattrnode)
	{
		pattrnode->attrInter(cinter);
		pattrnode->attrValue(svalue, nvallen);
	}
	else
	{
		pattrnode = m_powner->createAttribute(sname, svalue, nvallen, cinter);
		if(!pattrnode)	return false;
		return appendChild(pattrnode);
	}
	return true;
}

// * ============================================================== *
// @ 功能描述: 查询属性节点位置
// * ============================================================== *
CDOMXmlNode_Attribute* CDOMXmlNode_DocumentType::findAttrNode(const char* sname) const
{
	const CDOMXmlNode* pxmlnode=m_pchild;
	while(pxmlnode)
	{
		if( (pxmlnode->nodeType() == EDOMXML_NODETYPE_ATTRIBUTE) &&
			(pxmlnode->nodeValue() == sname) )
		{
			break;
		}
		pxmlnode=pxmlnode->nextSiblingNode();
	}
	return (CDOMXmlNode_Attribute *)pxmlnode;
}

// * ============================================================== *
// @ 功能描述: 组织xml节点数据
// * ============================================================== *
bool CDOMXmlNode_DocumentType::oriXmlData(CNString& sresult) const
{
	sresult += "<?xml ";
	const CDOMXmlNode* pchild = m_pchild;
	while(pchild) {
		pchild->oriXmlData(sresult);
		pchild = pchild->nextSiblingNode();
	}
	sresult += "?>";
	return true;
}

// * ============================================================== *
// @ 功能描述: 创建Attribute节点
// * ============================================================== *
CDOMXmlNode_Attribute* CDOMXmlNode_Document::createAttribute(const char* sname,
															 const char* svalue, const int nvallen,
															 const char cinter)
{
	CDOMXmlNode_Attribute* pattr = new CDOMXmlNode_Attribute(this, sname, cinter);
	if(!pattr)
	{
		return NULL;
	}
	pattr->attrValue(svalue, nvallen);
	return pattr;
}

// * ============================================================== *
// @ 功能描述: 创建Message节点
// * ============================================================== *
CDOMXmlNode_Message* CDOMXmlNode_Document::createMessage(const char* svalue, const int vallen)
{
	CDOMXmlNode_Message* pmessaage=new CDOMXmlNode_Message(this);
	if(!pmessaage)	return pmessaage;
	pmessaage->nodeValue(svalue, vallen);
	return pmessaage;

}

// * ============================================================== *
// @ 功能描述: 创建Element节点
// * ============================================================== *
CDOMXmlNode_Element* CDOMXmlNode_Document::createElement(const char* sname)
{
	CDOMXmlNode_Element* pelement = new CDOMXmlNode_Element(this);
	if(!pelement)
	{
		return pelement;
	}
	pelement->elementName(sname);
	return pelement;
}

// * ============================================================== *
// @ 功能描述: 创建DocumentType节点
// * ============================================================== *
CDOMXmlNode_DocumentType* CDOMXmlNode_Document::createDocumentType()
{
	return new CDOMXmlNode_DocumentType(this);
}

// * ============================================================== *
// @ 功能描述: 组织xml结构体数据
// * ============================================================== *
bool CDOMXmlNode_Document::oriXmlData(CNString& sresult) const
{
	const CDOMXmlNode* pnode=m_pchild;
	while(pnode)
	{
		pnode->oriXmlData(sresult);
		pnode = pnode->nextSiblingNode();
	}
	return true;
}
