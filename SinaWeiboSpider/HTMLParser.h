#pragma once
#include <map>
#include <vector>
#include "Tools.h"

// CHTMLParser
//HTML解析类
class CHTMLParser
{
public:
	CHTMLParser();
	virtual ~CHTMLParser();
	//TODO:从CString中获得HTML源代码，并做一些初始化工作
	bool ReadHTML(CString&);
	//TODO:销毁资源，释放内存
	bool Destory();
	//TODO:解析HTML
	bool ParserHTML();
	//TODO:获得整个标签内容  参数1指定要的标签
	std::vector<CString> GetTag(CString);
	//TODO:获得标签下的内容
	CString GetText(CString);
private:
	Tools m_Tools;
	//存放HTML
	CString m_HTMLCode;
	//存放head节点
	CString m_HeadNode;
	//存放body节点
	CString m_BodyNode;
	//存放用户信息的节点
	CString m_UserInfoNode;
	//存放div节点
	std::multimap<CString,CString> m_DivNode;
};


