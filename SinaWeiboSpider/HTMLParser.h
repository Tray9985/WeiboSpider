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
	//TODO:获得head节点 失败返回NULL
	CString GetHeadNode();
	//获得body节点 失败返回NULL
	CString GetBodyNode();
	//TODO:获得整个标签内容  参数1指定要的标签
	std::vector<CString> GetTag(CString);
	//TODO:获得标签下的内容
	CString GetText(CString);
private:
	Tools m_Tools;
	CString m_HTMLCode;
};


