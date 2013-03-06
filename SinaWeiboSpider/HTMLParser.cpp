// HTMLParser.cpp : 实现文件
//

#include "stdafx.h"
#include "SinaWeiboSpider.h"
#include "HTMLParser.h"


// CHTMLParser

CHTMLParser::CHTMLParser()
{

}

CHTMLParser::~CHTMLParser()
{
}


bool CHTMLParser::ReadHTML(CString &Source)
{
	if(Source.IsEmpty())
	{
		return false;
	}
	if(Source.Find("html"))
	{
		//这并不是一个HTML文件
		return false;
	}
	//复制到成员变量中
	m_HTMLCode = Source;
	return true;
}

bool CHTMLParser::Destory()
{
	m_HTMLCode.Empty();
	return true;
}

//取得头节点
CString CHTMLParser::GetHeadNode()
{
	CString Result;
	//取得头节点
	m_Tools.FindFirstString(m_HTMLCode,Result,"<head>.+?<//head.>");
	if(Result.IsEmpty())
	{
		//没有找到符合的字符串
		return;
	}
	else
	{
		return Result;
	}
}

//取得body节点
CString CHTMLParser::GetBodyNode()
{
	CString Result;
	//取得body节点
	//注意，body的内容可能会很长，对于内存开销是个问题
	m_Tools.FindFirstString(m_HTMLCode,Result,"<body>.+?<//body.>");
	if(Result.IsEmpty())
	{
		//没有找到符合的字符串
		return;
	}
	else
	{
		return Result;
	}
}

std::vector<CString> CHTMLParser::GetTag(CString TagName)
{
	std::vector<CString> _Res;
	//符合标签名的节点可能不止一个，返回包含所有符合结果
	if(TagName.IsEmpty())
	{
		//标签名称为空
		return;
	}
	CString Rule;
	//将标签名转换成正则表达式语法
	Rule.Format(_T("<%s>.+?<//%s>"),TagName,TagName);
	_Res = m_Tools.ParseString(m_HTMLCode,Rule);
	if(_Res.empty())
	{
		//容器为空
		return _Res;
	}
	else
	{
		return _Res;
	}
}

CString CHTMLParser::GetText(CString TagName)
{
	CString _Res;
	//符合标签名的节点可能不止一个，返回包含所有符合结果
	if(TagName.IsEmpty())
	{
		//标签名称为空
		return;
	}
	CString Rule;
	//将标签名转换成正则表达式语法
	Rule.Format(_T("<%s>.+?<//%s>"),TagName,TagName);
	if(_Res.empty())
	{
		//容器为空
		return _Res;
	}
	else
	{
		return _Res;
	}
}