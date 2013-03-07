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
	//复制到成员变量中
	m_HTMLCode = Source;
	return true;
}


bool CHTMLParser::ParserHTML()
{
	//临时存放div节点的vector
	std::vector<CString> _TempSaveDivNode;

	//删除HTML中的CSS样式
	if(!m_Tools.RemoveString(m_HTMLCode,"<style .+?>.+?</style>"))
	{
		//删除css格式失败
		return false;
	}
	//寻找头节点
	if(!m_Tools.FindFirstString(m_HTMLCode,m_HeadNode,"<head>.+?</head>"))
	{
		//没有找到head节点
		return false;
	}

	//寻找body节点
	if(!m_Tools.ParseString(m_HTMLCode,m_BodyNode,"<body>.+?</body>"))
	{
		//没有找到body节点
		return false;
	}
	//找到body节点
	else
	{
		if(m_Tools.ParseString(m_BodyNode,m_UserInfoNode,"<div class=\"u\">[^<div>].+?</div></div>"))
		{
			//找到用户信息节点
			//说明这个是微博页面
			//将body节点里用户信息的节点移除
			m_BodyNode.Replace(m_UserInfoNode,NULL);
		}

		
		//删除首部的信息
		CString _TempRemove;
		if(m_Tools.ParseString(m_BodyNode,_TempRemove,"<div class=\"n\".+?>.+?</a></div><div class=\"c\".+?>"))
		{
			//如果找到
			//将div class="c" 删除，避免获取不到第一条微博
			m_Tools.RemoveString(_TempRemove,"<div class=\"c\".+?>");
			m_BodyNode.Replace(_TempRemove,NULL);
		}
		//获取节点内容的正则表达式 <.+?>.+?</.+?><div class="s"></div>
		_TempSaveDivNode = m_Tools.ParseString(m_BodyNode,"<.+?>.+?</.+?><div class=\"s\"></div>");
		if(_TempSaveDivNode.empty())
		{
			//容器为空，报错
			return false;
		}
		std::vector<CString>::iterator _TempIter;
		_TempIter = _TempSaveDivNode.begin();
		while(_TempIter != _TempSaveDivNode.end())
		{
			CString _TagName;
			//获取标签名
			if(!m_Tools.FindFirstString(*_TempIter,_TagName,"<div class=.+?>"))
			{
				//获取不到标签名
				return false;
			}
			m_DivNode.insert(std::make_pair(_TagName,*_TempIter));
			++_TempIter;
		}
	}

	CFile fp;
	fp.Open(_T("D:\\ParserOut.txt"),CFile::modeCreate | CFile::modeWrite);
	std::multimap<CString,CString>::iterator  _iter;
	_iter = m_DivNode.begin();
	CString out;
	while(_iter != m_DivNode.end())
	{
		out.Format(_T("\n\nDiv Id is :%s--------\nDiv Content is :%s \n\n\n"),_iter->first,_iter->second);
		fp.Write(out,out.GetLength());
		++_iter;
	}
	return true;
}

bool CHTMLParser::Destory()
{
	m_HTMLCode.Empty();
	return true;
}

std::vector<CString> CHTMLParser::GetTag(CString TagName)
{
	std::vector<CString> _Res;
	//符合标签名的节点可能不止一个，返回包含所有符合结果
	if(TagName.IsEmpty())
	{
		//标签名称为空
		return _Res;
	}
	CString Rule;
	//将标签名转换成正则表达式语法
	Rule.Format(_T("<%s>.+?</%s>"),TagName,TagName);
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
		return NULL;
	}
	CString Rule;
	//将标签名转换成正则表达式语法
	Rule.Format(_T("<%s>.+?<//%s>"),TagName,TagName);
	if(_Res.IsEmpty())
	{
		return _Res;
	}
	else
	{
		return _Res;
	}
}