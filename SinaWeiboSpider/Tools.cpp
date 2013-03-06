/*
*这个类是工具类，封装了一些常用的，小型的函数
*包括正则表达式解析，字符串转换等
*原则上应该将各种辅助功能添加到此类当中
*构建于2013-2-20
*/



#include "StdAfx.h"
#include "Tools.h"
#include "boost\regex.hpp"
#include <Windows.h>
#include "SinaWeiboSpiderDlg.h"

#define Time 5000

//声明静态类成员后，必须在类外初始化，否则会报lnk 2001 错误
bool Tools::IsAutoSleep = false;

Tools::Tools(void)
{
}


Tools::~Tools(void)
{
}

//*************************正则表达式相关的函数***************************/
bool Tools::FormatString(CString &Origin)
{
	/*
	*Bulid Data 2013/2/20 16:30
	*此函数用于删除多余的空格，回车
	*/
	/*
	2013-2-28 有BUG！！！字符串带中文，删除空格会导致乱码
	*/
	if(Origin.IsEmpty())
	{
		AfxMessageBox(_T("FormatString\n错误：不能传递空参数"),MB_ICONWARNING);
		return false;
	}
	//找到数量在1个以上的空格
	CString Rule_Space("\\s{2,}\\s");
	//找到数量在1个以上的回车
	//CString Rule_Enter("\\r{2,}\\r");

	boost::regex reg_space(Rule_Space);
	//boost::regex reg_enter(Rule_Enter);

	std::string Result;
	std::string origin(Origin);
	//将多个空格及回车替换成一个
	Result = boost::regex_replace(origin,reg_space,"");
	//Result = boost::regex_replace(Result,reg_enter,"\\n");
	//将数据写入到实参中(引用)
	Origin = Result.c_str();
	return true;
}

bool Tools::ReplaceString(CString &Origin,CString String,CString Rule)
{
	/*
	*Bulid 2013-2-20 17:31
	*此函数用于替换字符串
	*以参数3指定的替换规则用参数2替换参数1里的字符串
	*/
	if(Origin.IsEmpty() || Rule.IsEmpty())
	{
		AfxMessageBox(_T("ReplaceString\n错误：不能传递空参数"),MB_ICONWARNING);
		return false;
	}
	boost::regex reg(Rule);
	std::string origin(Origin);
	std::string Result;
	Result = boost::regex_replace(origin,reg,String.GetBuffer());
	Origin = Result.c_str();
	return true;
}

bool Tools::ParseString(CString &Origin,CString &Result,CString Rule)
{
	/*
	*Bulid 2013-2-20 17:47
	*此函数用于解析字符串
	*在参数1中查找字符串，并写入到参数2中，查找规则由参数3指定
	*/
	if(Origin.IsEmpty() || Rule.IsEmpty())
	{
		AfxMessageBox(_T("ParseString Function\n错误：不能传递空参数"),MB_ICONWARNING);
		return false;
	}
	//初始化交换区
	Result = "";
	std::string origin(Origin);
	boost::regex reg(Rule);
	boost::smatch what;
	std::string::const_iterator beg,end;
	beg = origin.begin();
	end = origin.end();
	//不一定会找到，所以先判断是否找到，如果找不到，返回false
	if(!boost::regex_search(beg,end,what,reg))
	{
		Result = "";
		return false;
	}
	//规则匹配成立，循环打印出规则
	while(boost::regex_search(beg,end,what,reg))
	{
		std::string msg(what[0]);
		Result += msg.c_str();
		beg = what[0].second;
	}
	return true;
}

bool Tools::ParseString(CString &Origin,CString &Result,CString Rule,int)
{
	/*
	*Bulid 2013-2-20 17:49
	*此函数用于解析字符串，并在每个结果后面添加换行
	*在参数1中查找字符串，并写入到参数2中，查找规则由参数3指定
	*/
	if(Origin.IsEmpty() || Rule.IsEmpty())
	{
		AfxMessageBox(_T("ParseString Function with Add Enter Version\n错误：不能传递空参数"),MB_ICONWARNING);
		return false;
	}
	//初始化交换区
	Result = "";
	std::string origin(Origin);
	boost::regex reg(Rule);
	boost::smatch what;
	std::string::const_iterator beg,end;
	beg = origin.begin();
	end = origin.end();
	//不一定会找到，所以先判断是否找到，如果找不到，返回false
	if(!boost::regex_search(beg,end,what,reg))
	{
		Result = "";
		return false;
	}
	//规则匹配成立，循环打印出规则
	while(boost::regex_search(beg,end,what,reg))
	{
		std::string msg(what[0]);
		//在每个结果后面添加一个回车
		msg = msg + '\n';
		Result += msg.c_str();
		beg = what[0].second;
	}
	return true;
}


bool Tools::FindFirstString(CString &Origin,CString &Exchange,CString Rule)
{
	/*
	*Bulid 2013-3-5 15:03
	*此函数用于解析字符串，返回第一个符合要求的字符串
	*在参数1中查找字符串，并写入到参数2中，查找规则由参数3指定
	*/
	if(Origin.IsEmpty() || Rule.IsEmpty())
	{
		AfxMessageBox(_T("FindFirstString Function\n错误：不能传递空参数"),MB_ICONWARNING);
		return false;
	}
	//初始化交换区
	Exchange = "";
	std::string origin(Origin);
	boost::regex reg(Rule);
	boost::smatch what;
	std::string::const_iterator beg,end;
	beg = origin.begin();
	end = origin.end();
	//不一定会找到，所以先判断是否找到，如果找不到，返回false
	if(!boost::regex_search(beg,end,what,reg))
	{
		Exchange = "";
		return false;
	}
	//规则匹配成立，返回第一个结果
	boost::regex_search(beg,end,what,reg);
	std::string msg(what[0]);
	Exchange = msg.c_str();
	return true;
}

std::vector<CString> Tools::ParseString(CString &Source,CString Rule)
{
	/*
	*Bulid 2013-3-6 17:52
	*此函数用于解析字符串，将符合的结果压入vector中
	*在参数1中查找字符串，查找规则由参数2指定
	*/
	std::vector<CString> Res;
	if(Source.IsEmpty() || Rule.IsEmpty())
	{
		AfxMessageBox(_T("ParseString Function\n错误：不能传递空参数"),MB_ICONWARNING);
		return Res;
	}
	std::string origin(Source);
	boost::regex reg(Rule);
	boost::smatch what;
	std::string::const_iterator beg,end;
	beg = origin.begin();
	end = origin.end();
	//不一定会找到，所以先判断是否找到，如果找不到，返回false
	if(!boost::regex_search(beg,end,what,reg))
	{
		return Res;
	}
	//规则匹配成立，循环打印出规则
	while(boost::regex_search(beg,end,what,reg))
	{
		std::string msg(what[0]);
		//将结果压入vector中
		Res.push_back(msg.c_str());
		beg = what[0].second;
	}
	return Res;
}

bool Tools::SearchString(CString &Origin,CString Rule)
{
	/*
	*Bulid 2013-2-20 17:50
	*此函数用于查找字符串
	*以参数2指定的规则在参数1中查找，找到返回true，否则返回false
	*/
	if(Origin.IsEmpty() || Rule.IsEmpty())
	{
		AfxMessageBox(_T("SearchString Function\n错误：不能传递空参数"),MB_ICONWARNING);
		return false;
	}
	std::string origin(Origin);
	boost::regex reg(Rule);
	boost::smatch what;
	std::string::const_iterator beg,end;
	beg = origin.begin();
	end = origin.end();
	if(!boost::regex_search(beg,end,what,reg))
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool Tools::RemoveString(CString &Origin,CString Rule)
{
	/*
	*Bulid 2013-2-20 18:10
	*此函数用于删除字符串
	*参数2指定删除规则，在参数1中执行操作
	*/
	if(Origin.IsEmpty() || Rule.IsEmpty())
	{
		AfxMessageBox(_T("RemoveString Function\n错误：不能传递空参数"),MB_ICONWARNING);
		return false;
	}
	boost::regex reg(Rule);
	std::string origin(Origin);
	std::string res;
	res = boost::regex_replace(origin,reg,"");
	if(0 == res.length())
	{
		AfxMessageBox(_T("RemoveString Function\n错误：Boost::Regex_replace返回空字符串"));
		return false;
	}
	Origin = res.c_str();
	return true;
}

//*********************正则表达式相关的函数****************************

CString Tools::UTF8ToANSI(CString utf8)
{
	/*
	*此函数参考网上代码
	*Thanks   bbs.sciencenet.cn/thread-107913-1-1.html
	*/
	if(utf8.IsEmpty())
	{
		AfxMessageBox(_T("UTF8ToANSI\n错误：不能传递空参数"),MB_ICONERROR);
		return "";
	}
	int srcCodeLen=0;   
	srcCodeLen=MultiByteToWideChar(CP_UTF8,NULL,utf8,utf8.GetLength(),NULL,0);   
	wchar_t* result_t=new wchar_t[srcCodeLen+1];   
	MultiByteToWideChar(CP_UTF8,NULL,utf8,utf8.GetLength(),result_t,srcCodeLen); //utf-8转换为Unicode  
	result_t[srcCodeLen]='\0';   
	srcCodeLen=WideCharToMultiByte(CP_ACP,NULL,result_t,wcslen(result_t),NULL,0,NULL,NULL);   
	char* result=new char[srcCodeLen+1];   
	WideCharToMultiByte(CP_ACP,NULL,result_t,wcslen(result_t),result,srcCodeLen,NULL,NULL);//Unicode转换为ANSI   
	result[srcCodeLen]='\0';   
	delete result_t;   
	return result; 
}

std::vector<CString> Tools::SplitString(CString Origin)
{
	std::vector<CString> Res;
	if(Origin.IsEmpty())
	{
		AfxMessageBox(_T("SplitString Function Say：\n错误：不能传递空参数"),MB_ICONERROR);
		return Res;
	}
	CString str;
	while(true)
	{
		char *szBuffer = new char[Origin.GetLength()];
		if(!szBuffer)
		{
			AfxMessageBox(_T("SplitString Function Say:\n申请内存失败！"),MB_ICONERROR);
			return Res;
		}
		//分割字符
		if(!sscanf(Origin.GetBuffer(),"%s",szBuffer))
		{
			if(!szBuffer)
			{
				delete []szBuffer;
			}
			szBuffer = NULL;
			AfxMessageBox(_T("SplitString Function Say:\n sscanf 函数发生错误"));
			return Res;
		}
		Origin.ReleaseBuffer();
		str = szBuffer;
		if(!Origin.Replace(str,NULL))
		{
			if(!szBuffer)
			{
				delete []szBuffer;
			}
			szBuffer = NULL;
			break;
		}
		Res.push_back(str);
		if(!szBuffer)
		{
			delete []szBuffer;
		}
		szBuffer = NULL;
	}
	return Res;
}


std::vector<CString> Tools::SplitString(CString Origin,CString Rule)
{
	std::vector<CString> Res;
	if(Origin.IsEmpty())
	{
		AfxMessageBox(_T("SplitString Function Say：\n错误：不能传递空参数"),MB_ICONERROR);
		return Res;
	}
	std::string origin(Origin);
	boost::regex reg(Rule);
	boost::smatch what;
	std::string::const_iterator beg,end;
	beg = origin.begin();
	end = origin.end();
	//不一定会找到，所以先判断是否找到，如果找不到，返回false
	if(!boost::regex_search(beg,end,what,reg))
	{
		return Res;
	}
	//规则匹配成立，循环打印出规则
	while(boost::regex_search(beg,end,what,reg))
	{
		std::string msg(what[0]);
		Res.push_back(msg.c_str());
		beg = what[0].second;
	}
	return Res;
}

void Tools::Sleep()
{
	//此函数用于休眠线程，避免访问过快被限制
	//如果用户选择自动休眠，则使用随机生成的数值进行休眠
	//如果没有，则使用固定的数值进行休眠
	if(IsAutoSleep)
	{
		SleepTime = rand() % 900 * 100;
		UpdateInfo.Format(_T("线程睡眠中......睡眠时间为%d毫秒，请耐心等待"),SleepTime);
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		//睡眠，模拟无规律的操作，避免被限制
		::Sleep(SleepTime);
	}
	//未选择自动休眠，默认5秒
	else
	{
		UpdateInfo.Format(_T("线程睡眠中......睡眠时间为%d毫秒，请耐心等待"),Time);
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		//睡眠
		::Sleep(Time);
	}
}