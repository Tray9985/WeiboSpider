#include "StdAfx.h"
#include "NetWork.h"
#include "Tools.h"
#include <fstream>

//缓冲区最大大小
#define MAX 65535

NetWork::NetWork(void)
{
	IsSaveFilePathSet = false;
}

//取得对话框句柄，用于发送消息给控件
void NetWork::SetDlgHwnd(HWND m_hwnd)
{
	hwnd = m_hwnd;
	//将对话框句柄赋值给tools
	tools.hwnd = m_hwnd;
}

//设置存放结果的目录
void NetWork::SetSaveFilePath(CString Path)
{
	//首先判断是否第一次调用
	if(!IsSaveFilePathSet)
	{
		//如果Path为空，表示用户没有指定存储目录，采用默认目录存放
		if(Path.IsEmpty())
		{
			if(!DebugOut.Open(_T("D:\\debugout.txt"),CFile::modeCreate | CFile::modeReadWrite))
			{
				AfxMessageBox(_T("NetWork Constructor Function\n错误：无法创建Debug输出目录"),MB_ICONWARNING);
			}
			if(!m_Result.Open(_T("D:\\爬虫结果.txt"),CFile::modeReadWrite))
			{
				m_Result.Open(_T("D:\\爬虫结果.txt"),CFile::modeCreate|CFile::modeReadWrite);
			}
			m_Result.SeekToEnd();
		}
		//用户指定了存储目录
		else
		{
			if(!m_Result.Open(Path,CFile::modeReadWrite))
			{
				m_Result.Open(Path,CFile::modeCreate|CFile::modeReadWrite);
			}
			m_Result.SeekToEnd();
		}
		IsSaveFilePathSet = true;
	}
	//第2+次调用，得先关闭文件，再打开否则会报错
	//如果当前已打开文件，则关闭
	else
	{
		m_Result.Close();
		if(!m_Result.Open(Path,CFile::modeReadWrite))
		{
			m_Result.Open(Path,CFile::modeCreate|CFile::modeReadWrite);
		}
		m_Result.SeekToEnd();
		IsSaveFilePathSet = true;
	}
}

NetWork::~NetWork(void)
{
}

//初始化socket
BOOL NetWork::InitSocket()
{
	if(0 != WSAStartup(MAKEWORD(2,2),&data))
	{
		AfxMessageBox(_T("错误：初始化WSA结构体失败"),MB_ICONWARNING);
		return false;
	}
	Client = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(Client == INVALID_SOCKET)
	{
		AfxMessageBox(_T("错误：创建套接字失败"),MB_ICONWARNING);
		return false;
	}
	IsInit = true;
	return true;
}

//连接服务器
BOOL NetWork::ConnectionServer(CString IPAddr)
{
	if(IPAddr.IsEmpty())
	{
		AfxMessageBox(_T("ConnectionServer Function Say:\n错误：IP地址为空"),MB_ICONERROR);
		return false;
	}
	//将指定的IP赋值给成员变量IP
	IP = IPAddr;
	if(!IsInit)
	{
		AfxMessageBox(_T("错误：必须先初始化套接字"),MB_ICONWARNING);
		return false;
	}
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(IPAddr);

	ServerAddr.sin_port = htons(80);
	memset(ServerAddr.sin_zero,0,sizeof(ServerAddr));
	Result = connect(Client,(sockaddr*)&ServerAddr,sizeof(ServerAddr));
	if(SOCKET_ERROR == Result)
	{
		AfxMessageBox(_T("错误：连接服务器失败"),MB_ICONWARNING);
		return false;
	}
	IsConnection = true;
	return true;
}

//连接服务器重载版本，连接服务器IP为最后一次连接的IP
BOOL NetWork::ConnectionServer()
{
	if(!IsInit)
	{
		AfxMessageBox(_T("错误：必须先初始化套接字"),MB_ICONWARNING);
		return false;
	}

	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_addr.S_un.S_addr = inet_addr(IP);

	ServerAddr.sin_port = htons(80);
	memset(ServerAddr.sin_zero,0,sizeof(ServerAddr));
	Result = connect(Client,(sockaddr*)&ServerAddr,sizeof(ServerAddr));
	if(SOCKET_ERROR == Result)
	{
		AfxMessageBox(_T("错误：连接服务器失败"),MB_ICONWARNING);
		return false;
	}
	IsConnection = true;
	return true;
}

//发送数据，返回发送字节数
int NetWork::SendMsg(CString &Msg)
{
	if(!IsConnection)
	{
		AfxMessageBox(_T("错误：必须先连接服务器才能发送数据"),MB_ICONWARNING);
		return false;
	}

	Result = send(Client,Msg.GetBuffer(),Msg.GetLength(),0);
	if(SOCKET_ERROR == Result)
	{
		AfxMessageBox(_T("错误：发送数据失败"),MB_ICONWARNING);
		return false;
	}
	return Result;
}

//接受数据
BOOL NetWork::RecvMsg(CString &Exchange)
{
	std::ofstream fp;
	//初始化交换区
	Exchange = "";
	int Ret = 0;
	char *RecvBuffer = new char[MAX];
	while(true)
	{ 
		memset(RecvBuffer,0,MAX);
		Ret = 0;
		//最大值-1
		Ret = recv(Client,RecvBuffer,MAX-1,0);
		if(0 == Ret || SOCKET_ERROR == Ret)
		{
			//数据收完了
			break;
		}
		//因为服务器会以chunked编码的方法返回网页，这样的话，每次接受会多接收一个长度字符（不带0x的十六进制数）
		//为了避免指示长度的字符影响后来的解析，在每次接收时将该长度标记去除
		CString Temp(RecvBuffer);
		tools.RemoveString(Temp,"\\b[0-9a-fA-F]{2,8}?\\r");
		Exchange+=Temp;
	}
	if(Exchange.IsEmpty())
	{
		AfxMessageBox(_T("RecvMsg Function\n错误：服务器无返回，请稍候重试"),MB_ICONWARNING);
		delete []RecvBuffer;
		return false;
	}
	delete []RecvBuffer;
	return true;
}

//此函数重置socket连接
bool  NetWork::ResetSocket()
{
	//首先判断scket状态
	if(IsInit)
	{
		if(IsConnection)
		{
			//如果当前已连接，则关闭socket
			CloseSocket();
			//重新连接
			if(!InitSocket())
			{
				return false;
			}
			if(!ConnectionServer())
			{
				return false;
			}
			return true;
		}
		//已初始化但未连接，直接连接服务器即可
		else
		{
			if(!ConnectionServer())
			{
				return false;
			}
		}
	}
	//未初始化，直接初始化后连接
	else
	{
		if(!InitSocket())
		{
			return false;
		}
		if(!ConnectionServer())
		{
			return false;
		}
	}
	return true;
}

//此函数关闭socket
void NetWork::CloseSocket()
{
	try
	{
		closesocket(Client);
		WSACleanup();
		IsInit = false;
		IsConnection = false;
	}
	catch(...)
	{
		AfxMessageBox(_T("关闭套接字失败"),MB_ICONERROR);
		exit(-1);
	}
}

//此函数封装了登录过程
BOOL NetWork::Login(CString username,CString password)
{
	/*  微博登陆过程
	* 1) 采用GET方法请求3g.sina.com.cn/prog/wapsite/sso/login.php?ns=1&revalid=2&backURL=http%3A%2F%2Fweibo.cn%2F&backTitle=%D0%C2%C0%CB%CE%A2%B2%A9&vt=这个网页
	*     得到源代码，提取出form action=后面的URL，password的随机值，vk的随机值   
	* 2) 构建POST数据包，发送至3g.sina.com.cn//prog/wapsite/sso/这里加上第一步获取的form action后面的URL
	*    POST数据：mobile=" + 用户名 + "&" + 第一步获取的password随机值 +"=" + 用户密码 + "&remember=on&backURL=http%253A%252F%252Fweibo.cn%252F%253Fs2w%"
	*    "253Dlogin&backTitle=%E6%96%B0%E6%B5%AA%E5%BE%AE%E5%8D%9A&vk=" + 第一步获取的vk随机值 + "&submit=%E7%99%BB%E5%BD%95"
	* 3)服务器应该会返回HTTP 302 Found重定向,提取Location字段与Set-Cookie字段待用。如果服务器返回HTTP 200 OK，说明用户名或密码有错误
	*
	* 4)访问第二步返回的location进行跳转
	*
	* 5)接下来就登陆成功了，可以访问登陆后的网页，记得带上第二步返回的cookie
	*
	*
	*!!!!!!!!!!!!!GET方法最后面一定要有2个回车，不然会一直接受不到消息！！！！！！！！！！！！！！
	*/

	CString Request;
	Request =  "GET /prog/wapsite/sso/login.php?ns=1&revalid=2&backURL=http%3A%2F%2Fweibo.cn%2F&backTitle=%D0%C2%C0%CB%CE%A2%B2%A9&vt= HTTP/1.1\r\n"
		"Host: 3g.sina.com.cn\r\n"
		"User-Agent: Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
		"Referer: http://weibo.cn/pub/\r\n"
		//cookie开始
		"Cookie: ad_user=pos4ddc723cbedd25084b05a30e9d; ad_time=1350873178; U_TRS1=00000078.264e6b92.5097a058.a5dd7659; UOR=lcx.cc,tech.sina.com.cn,;"
		" SINAGLOBAL=5870298401953.488.1352264698102; user_survey=-1; FSINAGLOBAL=5870298401953.488.1352264698102; ULV=1352264699688:1:1:1:5870298401"
		"953.488.1352264698102:; vjuids=-3cf85321e.13ad941edfa.0.6c31fd762931c; vjlast=1352264708; ALF=1353307827; SUR=uid%3D1917215763%26user%3D6174"
		"78860%2540qq.com%26nick%3D617478860%26email%3D%26dob%3D%26ag%3D4%26sex%3D%26ssl%3D0; SUS=SID-1917215763-1352769650-XD-qyhz1-d69fa09fd946ce2b"
		"4097573d62722734; SUE=es%3D608da6a1a04d1ccfda4c66466e200cfe%26ev%3Dv1%26es2%3Dde3c0087d87c14526b3e0165adfd2750%26rs0%3DkYpezbkv4Afm1AUYgxH1o"
		"P8m5i3E5xpcpbKSHp8iyd6Uio2n60sWvy1Rg9EON8P759GDg4rA3Zk3WOBdRdar%252FOLJA0dfvZCNr4Wft0kwVAkBIZEdILoXz3WhmNIAE1WPylcZF66KS7Bdn7lMuWwYZxtkOWOK9"
		"0O%252Bp0w7k9U7g2s%253D%26rv%3D0; SUP=cv%3D1%26bt%3D1352769650%26et%3D1352856050%26d%3D40c3%26i%3D2734%26us%3D1%26vf%3D0%26vt%3D0%26ac%3D0%2"
		"6lt%3D7%26uid%3D1917215763%26user%3D617478860%2540qq.com%26ag%3D4%26name%3D617478860%2540qq.com%26nick%3D617478860%26sex%3D%26ps%3D0%26email"
		"%3D%26dob%3D%26ln%3D%26os%3D%26fmp%3D%26lcp%3D2011-06-15%252020%253A00%253A51\r\n\r\n";

	DebugInfo.Format(_T("\n第一次请求：获取随机值\n"));
	DebugOut.Write(DebugInfo,DebugInfo.GetLength());
	DebugOut.Flush();

	//发送消息给窗口更新控件
	UpdateInfo.Format(_T("获得网页随机值中......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(!SendMsg(Request))
	{
		UpdateInfo.Format(_T("发送请求失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}

	CString ServerReturn_UTF8,ServerReturn_ANSI;
	if(!RecvMsg(ServerReturn_UTF8))
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	/*
	*对服务器返回的字符串进行转码
	*新浪微博的编码为UTF8,正则表达式貌似并不支持unicode编码，所以必须转换到ANSI编码
	*/
	ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);

	if(ServerReturn_ANSI.IsEmpty())
	{
		AfxMessageBox(_T("Login Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONWARNING);
		UpdateInfo.Format(_T("UTF8ToANSI函数返回空字符串"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	/*
	*取得微博页面URL的随机值，等下待用
	*/
	CString RandomURL,m_temp;
	if(!tools.ParseString(ServerReturn_ANSI,RandomURL,"<form action=\".*ns=1\""))
	{
		UpdateInfo.Format(_T("获得URL随机值失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	m_temp = RandomURL;

	if(!tools.ParseString(m_temp,RandomURL,"login.*ns=1"))
	{
		UpdateInfo.Format(_T("获得URL随机值失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	m_temp.Empty();

	DebugInfo.Format(_T("\n\nURL的随机值是 :%s\n\n"),RandomURL);
	DebugOut.Write(DebugInfo,DebugInfo.GetLength());
	DebugOut.Flush();

	//取得微博页面password后面的随机值
	if(!tools.ParseString(ServerReturn_ANSI,m_RandomPassword,"password.*"))
	{
		UpdateInfo.Format(_T("获得Password随机值失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	m_temp = m_RandomPassword;
	if(!tools.ParseString(m_temp,m_RandomPassword,"password_...."))
	{
		return false;
	}
	DebugInfo.Format(_T("\n\nPassword随机值是:%s\n\n"),m_RandomPassword);
	DebugOut.Write(DebugInfo,DebugInfo.GetLength());
	DebugOut.Flush();

	//取得微博页面VK后面的随机值
	if(!tools.ParseString(ServerReturn_ANSI,m_vk,"vk..value..\\w{0,50}"))
	{
		UpdateInfo.Format(_T("获得VK随机值失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	m_temp = m_vk;

	if(!tools.ParseString(m_temp,m_vk,"=\"\\w{0,100}"))
	{
		UpdateInfo.Format(_T("获得VK随机值失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	//去掉vk多余的字符
	m_vk.Replace("\"",NULL);
	m_vk.Replace("=",NULL);


	UpdateInfo.Format(_T("成功"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	CString PostDataLenght;
	CString PostData;
	CString LoginRequest;

	//重置socket状态
	CloseSocket();
	if(!InitSocket())
	{
		return false;
	}
	//指定登录服务器的IP地址
	if(!ConnectionServer("221.179.175.244"))
	{
		return false;
	}

	//构建数据包
	PostData = "mobile=" + username + "&" + m_RandomPassword +"=" + password  + "&remember=on&backURL=http%253A%252F%252Fweibo.cn%252F%253Fs2w%"
		"253Dlogin&backTitle=%E6%96%B0%E6%B5%AA%E5%BE%AE%E5%8D%9A&vk=" + m_vk + "&submit=%E7%99%BB%E5%BD%95";
	//计算数据包的长度
	PostDataLenght.Format("Content-Length: %d\r\n\r\n",PostData.GetLength());

	//构建请求包
	LoginRequest = "POST /prog/wapsite/sso/"+RandomURL+" HTTP/1.1\r\n"
		"Host: 3g.sina.com.cn\r\n"
		"User-Agent: Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
		"Referer: http://3g.sina.com.cn/prog/wapsite/sso/login.php?ns=1&revalid=2&backURL=http%3A%2F%2Fweibo.cn%2F%3Fs2w%3Dlogin&backTitle=%D0%C2%C0%CB%CE%A2%B2%A9&vt=\r\n"
		//cookie 行开始
		"Cookie: ad_user=pos4ddc723cbedd25084b05a30e9d; ad_time=1350873178; U_TRS1=00000078.264e6b92.5097a058.a5dd7659; UOR=lcx.cc,tech.sina.com.cn,; SINAGLOBAL=5870298401953.488."
		"1352264698102; user_survey=-1; FSINAGLOBAL=5870298401953.488.1352264698102; ULV=1352264699688:1:1:1:5870298401953.488.1352264698102:; vjuids=-3cf85321e.13ad941edfa.0."
		"6c31fd762931c; vjlast=1352264708; SUS=SID-1917215763-1352703027-XD-vfgtd-bd3b55a5094b6402fd79fadefe693f97; SUE=es%3Dc5e170f1abd289a95794e07e9458f4d8%26ev%3Dv1%26es2%3D1815e128"
		"e69a7575d60b9aa17db3a3ab%26rs0%3DwTZJDGkATabYkGHu8%252FTMgtvVZLN8d%252FkA3Ihu7jDtTAZ1SkeS0FhqLJQAlP4XLA%252FtEbM5G79mWYnJ95nptVOlSSlGwe9c9uROq1zBZMiE6%252FuLRL9xiPTNBsJ2hJjQkn"
		"r0cpreeDv%252BGMpkmaeejcKvHaEq7ktvB1SYY3J1vzTsfDE%253D%26rv%3D0; "
		"SUP=cv%3D1%26bt%3D1352703027%26et%3D1352789427%26d%3D40c3%26i%3D3f97%26us%3D1%26vf%3D0%26vt%3D0%26ac%3D0%26lt%3D1%26uid%3D1917215763%26user%3D617478860%2540qq."
		"com%26ag%3D4%26name%3D617478860%2540qq.com%26nick%3D617478860%26sex%3D%26ps%3D0%26email%3D%26dob%3D%26ln%3D617478860%2540qq.com%26os%3D%26fmp%3D%26lcp%3D2011-06-"
		"15%252020%253A00%253A51; ALF=1353307827; SUR=uid%3D1917215763%26user%3D617478860%2540qq.com%26nick%3D617478860%26email%3D%26dob%3D%26ag%3D4%26sex%3D%26ssl%3D0\r\n"
		//cookie 行结束
		"Content-Type: application/x-www-form-urlencoded\r\n"+
		PostDataLenght+PostData;

	UpdateInfo.Format(_T("发送登录请求中......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(!SendMsg(LoginRequest))
	{
		UpdateInfo.Format(_T("失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}

	if(!RecvMsg(ServerReturn_UTF8))
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(ServerReturn_UTF8.IsEmpty())
	{
		AfxMessageBox(_T("Login Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
		UpdateInfo.Format(_T("服务器无返回，请稍候重试"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}

	ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
	if(ServerReturn_ANSI.IsEmpty())
	{
		AfxMessageBox(_T("Login Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
		UpdateInfo.Format(_T("UTF8ToANSI函数返回空字符串"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	//如果登录失败，服务器会返回HTTP 200 OK ，成功则返回HTTP 302 Found
	if(tools.SearchString(ServerReturn_ANSI,"HTTP/1.1 200 OK"))
	{
		//寻找密码错误的标记
		if(tools.SearchString(ServerReturn_ANSI,"<div class=\"msgErr\">"))
		{
			UpdateInfo.Format(_T("用户名或密码错误，请重新输入"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("Login Function\n用户名或密码输入错误，请重新输入"),MB_ICONERROR);
			return false;
		}
		//2013-2-21
		//如果没有找到密码错误的标记，那么有可能是要输入验证码，现在不打算解决此问题
		else
		{
			int Res;
			Res = AfxMessageBox(_T("Login Function Say:\n服务器返回异常，是否显示返回信息？"),MB_YESNO | MB_ICONERROR);
			if(IDYES == Res)
			{
				AfxMessageBox(ServerReturn_ANSI);
				return false;
			}
			else
			{
				AfxMessageBox(_T("Login Function Say:\n程序将会退出......"));
				exit(-1);
			}
		}
	}

	if(tools.SearchString(ServerReturn_ANSI,"HTTP/1.1 302 Found"))
	{
		//寻找Location字段
		if(!tools.ParseString(ServerReturn_ANSI,m_location,"Location:.http.*"))
		{
			AfxMessageBox(_T("Login Function Say:\n错误：未找到Location字段"),MB_ICONERROR);
			return false;
		}
		else
		{
			//寻找Set-Cooike字段
			if(!tools.ParseString(ServerReturn_ANSI,m_cookie,"Set-Cookie.*?;"))
			{
				AfxMessageBox(_T("Login Function\n错误:未找到Set-Cookie字段"),MB_ICONWARNING);
				return false;
			}

			//删除Location字段的信息，只保留网址
			m_location.Replace("Location:",NULL);
			m_location.Replace("http://login.sina.cn",NULL);
			//删除Vary及后面所有内容
			int cout = m_location.Find("Vary");
			m_location.Delete(cout,10000);
			m_location.Find("\r\n",NULL);
			//删除Set-Cookie字段信息，只保留cookie
			m_cookie.Replace("Set-Cookie:",NULL);
			m_cookie.Replace(";",NULL);
			m_cookie = "_T_WL=1;"+ m_cookie;
			m_cookie.Replace("\r\n",NULL);
		}
	}
	//服务器既不返回HTTP 200,也不返回HTTP 302
	//目前还没遇到这样的情况
	else
	{
		int Res;
		Res = AfxMessageBox(_T("Login Function Say:\n服务器返回异常，是否显示返回结果？(返回结果已输出至Debug文件)"),MB_YESNO | MB_ICONERROR);
		DebugInfo.Format(_T("\n微博登录失败，服务器返回结果异常，无法继续处理。结果如下:\n"));
		DebugOut.Write(DebugInfo,DebugInfo.GetLength());
		DebugOut.Write(ServerReturn_ANSI,ServerReturn_ANSI.GetLength());
		if(IDYES == Res)
		{
			AfxMessageBox(ServerReturn_ANSI);
			return false;
		}
		else
		{
			AfxMessageBox(_T("程序将会退出"));
			exit(-1);
		}
	}

	DebugInfo.Format(_T("\n\nCookie字段值：%s\nLocation字段值：%s\n\n"),m_cookie,m_location);
	DebugOut.Write(DebugInfo,DebugInfo.GetLength());
	DebugOut.Flush();

	UpdateInfo.Format(_T("成功"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	//重置Socket
	CloseSocket();
	if(!InitSocket())
	{
		return false;
	}
	if(!ConnectionServer("221.179.175.249"))
	{
		return false;
	}

	CString JumpRequest;
	JumpRequest =  "GET " + m_location + "HTTP/1.1\r\n"
		"Host: login.sina.cn\r\n"
		"User-Agent: Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language: zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
		"Connection: keep-alive\r\n"
		"Referer: http://3g.sina.com.cn/prog/wapsite/sso/login.php?ns=1&revalid=2&backURL=http%3A%2F%2Fweibo.cn%2F%3Fs2w%3Dlogin&backTitle=%D0%C2%C0%CB%CE%A2%B2%A9&vt=\r\n"
		"Cookie: ad_user=pos4ddc723cbedd25094866b177da; ad_time=1351911019; ustat=4c0077fd3103524f2f1e0748204b897b; vt=4\r\n\r\n";

	UpdateInfo.Format(_T("发送跳转请求中......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(!SendMsg(JumpRequest))
	{
		UpdateInfo.Format(_T("失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(!RecvMsg(ServerReturn_UTF8))
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(ServerReturn_UTF8.IsEmpty())
	{
		AfxMessageBox(_T("Login Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
		UpdateInfo.Format(_T("服务器无返回，请稍候重试"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}

	UpdateInfo.Format(_T("成功"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	CloseSocket();
	AfxMessageBox(_T("Login Function Say:\n亲，登录成功~!"),MB_ICONINFORMATION);

	UpdateInfo.Format(_T("登录成功."));

	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	return true;
}

//遍历关注
bool NetWork::TraversalFollow(CString URL)
{
	IP = "180.149.139.248";
	ResetSocket();
	DebugInfo.Format(_T("\n\n---------开始遍历关注-------\n\n"));
	DebugOut.Write(DebugInfo,DebugInfo.GetLength());
	DebugOut.Flush();

	UpdateInfo.Format(_T("开始遍历关注......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(!tools.SearchString(URL,"follow"))
	{
		UpdateInfo.Format(_T("错误：地址无效"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TraversalFollow Function Say:\n错误，这个地址貌似不是关注列表的地址，请传递关注列表的地址"),MB_ICONWARNING);
		return false;
	}

	CString Request,ServerReturn_UTF8,ServerReturn_ANSI;
	//去除前缀
	URL.Replace("http://weibo.cn",NULL);

	Request = "GET " + URL + " HTTP/1.1\r\n"
		"Host:weibo.cn\r\n"
		"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
		"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
		"Referer:http://weibo.cn/\r\n"
		"Cookie:"+m_cookie+";\r\n\r\n";

	UpdateInfo.Format(_T("发送请求获取网页中......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(!SendMsg(Request))
	{
		UpdateInfo.Format(_T("发送请求失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(!RecvMsg(ServerReturn_UTF8))
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}

	if(ServerReturn_UTF8.IsEmpty())
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TraversalFollow Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONWARNING);
		return false;
	}

	UpdateInfo.Format(_T("获取网页成功\n编码转换中......."));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);

	if(ServerReturn_ANSI.IsEmpty())
	{
		UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TraversalFollow Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
		return false;
	}

	UpdateInfo.Format(_T("成功"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	//重试计数
	int count = 0;
	while(!tools.SearchString(ServerReturn_ANSI,"HTTP/1.1 200 OK"))
	{
		++count;
		if(5 > count)
		{
			UpdateInfo.Format(_T("获取结果失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("TraversalFollow Function Say:\n重试次数过多，请稍候重试，或者换个帐号"));
			return false;
		}
		int Res;
		Res = AfxMessageBox(_T("TraversalFollow Function Say:\n服务器返回结果出乎意料了，是否稍后重新发送试试？"),MB_YESNO);
		if(IDYES == Res)
		{
			//出现这种情况的话，一般都是速度太快引起的，休眠时间应该长一点
			tools.Sleep();
			tools.Sleep();
			tools.Sleep();

			ResetSocket();
			if(!SendMsg(Request))
			{
				return false;
			}
			if(!RecvMsg(ServerReturn_UTF8))
			{
				return false;
			}
			if(ServerReturn_UTF8.IsEmpty())
			{
				UpdateInfo.Format(_T("接收服务器返回失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalFollow Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
				return false;
			}
			ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
			if(ServerReturn_ANSI.IsEmpty())
			{
				UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalFollow Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
				return false;
			}
		}
		else
		{
			return false;
		}
	}


	DebugOut.Write(ServerReturn_ANSI,ServerReturn_ANSI.GetLength());

	UpdateInfo.Format(_T("获得页数中......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	//获得页数
	CString All_Page,Page_Temp;
	if(!tools.ParseString(ServerReturn_ANSI,Page_Temp,";\\d{1,}.\\d{1,}.</div>"))
	{
		UpdateInfo.Format(_T("获取页数失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(!tools.ParseString(Page_Temp,All_Page,"/\\d{1,}页"))
	{
		UpdateInfo.Format(_T("获取页数失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	All_Page.Replace("/",NULL);
	All_Page.Replace("页",NULL);
	Follow_Page = atoi(All_Page.GetBuffer());

	CString Reg_res;
	CString Follow_URL,Follow_UID;

	//存放分割>字符的索引
	unsigned int unt= 0;
	unsigned int Now_Page = 1;

	UpdateInfo.Format(_T("获得页数成功！页数为:%d"),Follow_Page);
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	//存放分割后的字符串
	std::vector<CString> SplitStringReturn;
	std::vector<CString>::iterator vec_iter;

	//获得系统时间
	COleDateTime;
	COleDateTime datetime;  
	datetime=COleDateTime::GetCurrentTime();                                                                               
	SystemTime=datetime.Format("%Y年%m月%d日%H时%M分   "); 
	DebugInfo.Format(_T("\n-------开始遍历关注，当前时间：%s----------\n"),SystemTime);
	m_Result.Write(DebugInfo,DebugInfo.GetLength());

	while(Now_Page < Follow_Page)
	{
		//清空m_FollowList，每遍历一页写一次文件，节省内存
		m_FollowList.clear();

		while(true)
		{
			//睡眠一下
			tools.Sleep();

			//删除多余的字符
			ServerReturn_ANSI.Replace("?vt=4&amp;gsid=3_5afe4404b77a34da8b085446a50e8af47308&amp;st=5e0f",NULL);
			tools.RemoveString(ServerReturn_ANSI,".st=\\w{1,}");

			//获得关注对象的数据
			//格式weibo.cn\username>昵称<
			//或者是weibo.cn\u\userid>昵称<
			if(!tools.ParseString(ServerReturn_ANSI,Reg_res,"weibo.cn...\\w{1,30}\">[^<].+?<",0))
			{
				//有时候，即使是HTTP 200 OK的返回值，页面也有可能是空的，并没有内容
				//这样的话，就得跳到下一页，避免遇到这样的情况导致后面的页面不会被获取
				DebugInfo.Format(_T("TraversalFollw Function：第%d页获取不到内容，跳转下一页"),Now_Page);
				DebugOut.Write(DebugInfo,DebugInfo.GetLength());
				DebugOut.Flush();

				CString JumpNextPage;
				JumpNextPage.Format("?page=%d",++Now_Page);

				if(Now_Page > Follow_Page)
				{
					break;
				}

				UpdateInfo.Format(_T("当前遍历第%d页，共%d页"),Now_Page,Follow_Page);
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

				JumpNextPage = URL+JumpNextPage;
				Request = "GET " + JumpNextPage + " HTTP/1.1\r\n"
					"Host:weibo.cn\r\n"
					"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
					"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
					"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
					"Connection:keep-alive\r\n"
					//从哪个页面跳转的，虽然觉得随便填没什么关系，但是为了防止出莫名其妙的错误，按规律填上为妙
					"Referer:http://weibo.cn/" +JumpNextPage +"\r\n"
					"Cookie:"+m_cookie+";\r\n\r\n";


				UpdateInfo.Format(_T("发送请求中......"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				ResetSocket();
				if(!SendMsg(Request))
				{
					UpdateInfo.Format(_T("发送请求失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}
				if(!RecvMsg(ServerReturn_UTF8))
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}

				if(ServerReturn_UTF8.IsEmpty())
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFollow Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONWARNING);
					return false;
				}

				UpdateInfo.Format(_T("获取网页成功\n开始编码转换......"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

				ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
				if(ServerReturn_ANSI.IsEmpty())
				{
					UpdateInfo.Format(_T("编码转换失败,UTF8ToANSI函数返回空字符串"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFollow Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
					return false;
				}
			}
			else
			{
				break;
			}
		}

		Reg_res.Replace("\"",NULL);


		UpdateInfo.Format(_T("开始解析网页内容"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

		//分割字符串
		SplitStringReturn = tools.SplitString(Reg_res);


		if(SplitStringReturn.empty())
		{
			UpdateInfo.Format(_T("网页内容解析失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			return false;
		}
		vec_iter = SplitStringReturn.begin();
		while(vec_iter != SplitStringReturn.end())
		{
			//找到分割符
			unt = vec_iter->Find(">");
			//从左到分隔符之间的内容为用户url
			Follow_URL = vec_iter->Left(unt);
			//从分隔符到结束为用户昵称
			Follow_UID = vec_iter->Mid(unt);
			m_FollowList.insert(std::make_pair(Follow_URL,Follow_UID));
			++vec_iter;
		}

		m_iter = m_FollowList.begin();
		CString Out;
		while(m_iter != m_FollowList.end())
		{
			Out.Format("\n用户昵称：%s ----  用户URL：%s",m_iter->second,m_iter->first);
			m_Result.Write(Out,Out.GetLength());
			++m_iter;
		}

		CString JumpNextPage;
		//先判断下页数是否合法，之前的while循环有可能将页数遍历完毕了
		if(Now_Page > Follow_Page)
		{
			break;
		}


		JumpNextPage.Format("?page=%d",++Now_Page);
		JumpNextPage = URL+JumpNextPage;
		Request = "GET " + JumpNextPage + " HTTP/1.1\r\n"
			"Host:weibo.cn\r\n"
			"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
			"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
			"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
			"Connection:keep-alive\r\n"
			//从哪个页面跳转的，虽然觉得随便填没什么关系，但是为了防止出莫名其妙的错误，按规律填上为妙
			"Referer:http://weibo.cn/" +JumpNextPage +"\r\n"
			"Cookie:"+m_cookie+";\r\n\r\n";


		UpdateInfo.Format(_T("当前遍历第%d页，共%d页"),Now_Page,Follow_Page);
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

		ResetSocket();
		if(!SendMsg(Request))
		{
			UpdateInfo.Format(_T("发送请求失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			return false;
		}
		if(!RecvMsg(ServerReturn_UTF8))
		{
			UpdateInfo.Format(_T("接收服务器返回失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			return false;
		}

		if(ServerReturn_UTF8.IsEmpty())
		{
			UpdateInfo.Format(_T("接收服务器返回失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("TraversalFollow Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONWARNING);
			return false;
		}

		UpdateInfo.Format(_T("获取网页成功\n开始编码转换"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

		ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);

		if(ServerReturn_ANSI.IsEmpty())
		{
			UpdateInfo.Format(_T("编码转换失败,UTF8ToANSI函数返回空字符串"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("TraversalFollow Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
			return false;
		}

		//重置重试计数
		count = 0;
		while(!tools.SearchString(ServerReturn_ANSI,"HTTP/1.1 200 OK"))
		{
			++count;
			if(5 > count)
			{
				UpdateInfo.Format(_T("重试次数过多，请稍候重试，或者换个帐号"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalFollow Function Say:\n重试次数过多，请稍候重试，或者换个帐号"));
				return false;
			}
			int Res;
			Res = AfxMessageBox(_T("TraversalFollow Function Say:\n服务器返回结果出乎意料了，是否稍后重新发送试试？"),MB_YESNO);
			if(IDYES == Res)
			{
				//一般出现这个问题都是访问过快，睡眠久点
				tools.Sleep();
				tools.Sleep();
				tools.Sleep();
				ResetSocket();
				UpdateInfo.Format(_T("发送请求中......"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				if(!SendMsg(Request))
				{
					UpdateInfo.Format(_T("发送请求失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}
				if(!RecvMsg(ServerReturn_UTF8))
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}
				if(ServerReturn_UTF8.IsEmpty())
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFollow Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
					return false;
				}

				UpdateInfo.Format(_T("获取网页成功\n开始编码转换......"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

				ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
				if(ServerReturn_ANSI.IsEmpty())
				{
					UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFollow Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	CloseSocket();
	UpdateInfo.Format(_T("遍历关注完成"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	return true;
}

//遍历粉丝
bool NetWork::TraversalFans(CString URL)
{
	IP = "180.149.139.248";
	DebugInfo.Format(_T("\n-----------开始遍历粉丝--------\n"));
	DebugOut.Write(DebugInfo,DebugInfo.GetLength());

	UpdateInfo.Format(_T("开始遍历粉丝"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	ResetSocket();

	if(!tools.SearchString(URL,"fans"))
	{
		UpdateInfo.Format(_T("错误：地址无效"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TreaversalFans Function Say:\n错误：这个地址貌似不是粉丝列表地址，请传递粉丝列表地址"),MB_ICONERROR);
		return false;
	}

	CString ServerReturn_UTF8,ServerReturn_ANSI,Request;
	//删除http://weibo.cn的前缀
	URL.Replace("http://weibo.cn",NULL);

	Request  = "GET " + URL + " HTTP/1.1\r\n"
		"Host:weibo.cn\r\n"
		"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
		"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
		"Referer:http://weibo.cn/\r\n"
		"Cookie:"+m_cookie+";\r\n\r\n";

	UpdateInfo.Format(_T("发送请求中......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(!SendMsg(Request))
	{
		UpdateInfo.Format(_T("发送请求失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(!RecvMsg(ServerReturn_UTF8))
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}

	if(ServerReturn_UTF8.IsEmpty())
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TreaversalFans Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
		return false;
	}

	UpdateInfo.Format(_T("获取网页成功，开始编码转换"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
	if(ServerReturn_ANSI.IsEmpty())
	{
		UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TreaversalFans Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
		return false;
	}
	//设置重置计数器
	unsigned int count = 0;
	while(!tools.SearchString(ServerReturn_ANSI,"HTTP/1.1 200 OK"))
	{
		++count;
		if(5 > count)
		{
			UpdateInfo.Format(_T("获取结果失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("TreaversalFans Function Say:\n错误：重试次数过多，请稍候重试，或者换个帐号"));
			return false;
		}
		int Res;
		Res = AfxMessageBox(_T("TreaversalFans Function Say:\n服务器的返回结果出乎意料了，是否稍后重新发送试试?"),MB_YESNO);
		if(IDYES == Res)
		{
			//睡久点
			tools.Sleep();
			tools.Sleep();
			tools.Sleep();

			ResetSocket();
			UpdateInfo.Format(_T("发送请求中......"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

			if(!SendMsg(Request))
			{
				UpdateInfo.Format(_T("发送请求失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			if(!RecvMsg(ServerReturn_UTF8))
			{
				UpdateInfo.Format(_T("接收服务器返回失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			if(ServerReturn_UTF8.IsEmpty())
			{
				UpdateInfo.Format(_T("接收服务器返回失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalFans Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
				return false;
			}
			ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
			if(ServerReturn_ANSI.IsEmpty())
			{
				UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalFans Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	DebugOut.Write(ServerReturn_ANSI,ServerReturn_ANSI.GetLength());

	UpdateInfo.Format(_T("开始获取粉丝页数......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	//获取页数
	CString All_Page,Page_Temp;
	if(!tools.ParseString(ServerReturn_ANSI,Page_Temp,"\\w{1,}/\\w{1,}页"))
	{
		UpdateInfo.Format(_T("获取页数失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TreaversalFans Function Say:\n错误：获取页面数失败"),MB_ICONERROR);
		return false;
	}
	if(!tools.ParseString(Page_Temp,All_Page,"/\\d{1,}页"))
	{
		UpdateInfo.Format(_T("获取页数失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	All_Page.Replace("/",NULL);
	All_Page.Replace("页",NULL);
	Fans_Page = atoi(All_Page.GetBuffer());
	//清空字符串
	All_Page.Empty();

	UpdateInfo.Format(_T("获取页数成功，共%d页"),Fans_Page);
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(Fans_Page > 300)
	{
		CString Out;
		int Res;
		UpdateInfo.Format(_T("粉丝页数过多：%d，可能会影响效率"),Fans_Page);
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		Out.Format(_T("粉丝页数过多：%d  遍历时间可能会很长，是否继续遍历？"),Fans_Page);
		Res = AfxMessageBox(Out,MB_YESNO);
		if(IDNO == Res)
		{
			UpdateInfo.Format(_T("遍历粉丝已取消"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			DebugInfo.Format(_T("用户自行取消遍历粉丝"));
			DebugOut.Write(DebugInfo,DebugInfo.GetLength());
			return true;
		}
	}

	CString Reg_res;
	CString Fans_URL,Fans_UID;

	//存放分割>字符的索引
	unsigned int unt= 0;
	unsigned int Now_Page = 1;

	//存放分割后的字符串
	std::vector<CString> SplitStringReturn;
	std::vector<CString>::iterator vec_iter;

	//获得系统时间
	COleDateTime;
	COleDateTime datetime;  
	datetime=COleDateTime::GetCurrentTime();                                                                               
	SystemTime=datetime.Format("%Y年%m月%d日%H时%M分   "); 
	DebugInfo.Format(_T("\n-------开始遍历粉丝，当前时间：%s----------\n"),SystemTime);
	m_Result.Write(DebugInfo,DebugInfo.GetLength());

	while(Now_Page < Fans_Page)
	{
		//清空m_FansList，每遍历一页写一次文件，节省内存
		m_FansList.clear();

		while(true)
		{
			tools.Sleep();
			//删除多余的字符
			tools.RemoveString(ServerReturn_ANSI,".st=\\w{1,}");

			UpdateInfo.Format(_T("开始解析网页"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			//解析粉丝内容，格式与关注一样
			if(!tools.ParseString(ServerReturn_ANSI,Reg_res,"weibo.cn...\\w{1,30}\">[^<].+?<",0))
			{
				//有时候，即使是HTTP 200 OK的返回值，页面也有可能是空的，并没有内容
				//这样的话，就得跳到下一页，避免遇到这样的情况导致后面的页面不会被获取
				DebugInfo.Format(_T("TraversalFans Function：第%d页获取不到内容，跳转下一页"),Now_Page);
				DebugOut.Write(DebugInfo,DebugInfo.GetLength());
				DebugOut.Flush();

				CString JumpNextPage;
				JumpNextPage.Format("?page=%d",++Now_Page);

				if(Now_Page > Fans_Page)
				{
					break;
				}

				UpdateInfo.Format(_T("当前页%d解析失败，跳转到下一页"),Now_Page);
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

				JumpNextPage = URL+JumpNextPage;
				Request = "GET " + JumpNextPage + " HTTP/1.1\r\n"
					"Host:weibo.cn\r\n"
					"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
					"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
					"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
					//从哪个页面跳转的，虽然觉得随便填没什么关系，但是为了防止出莫名其妙的错误，按规律填上为妙
					"Referer:http://weibo.cn/" +JumpNextPage +"\r\n"
					"Cookie:"+m_cookie+";\r\n\r\n";

				UpdateInfo.Format(_T("发送请求中......"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

				ResetSocket();
				if(!SendMsg(Request))
				{
					UpdateInfo.Format(_T("发送请求失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}
				if(!RecvMsg(ServerReturn_UTF8))
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}

				if(ServerReturn_UTF8.IsEmpty())
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFans Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONWARNING);
					return false;
				}

				UpdateInfo.Format(_T("获取网页成功，开始编码转换"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
				if(ServerReturn_ANSI.IsEmpty())
				{
					UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFansFunction Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
					return false;
				}
			}
			else
			{
				break;
			}
		}

		UpdateInfo.Format(_T("开始解析网页"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		//分割字符串
		SplitStringReturn = tools.SplitString(Reg_res);

		if(SplitStringReturn.empty())
		{
			UpdateInfo.Format(_T("解析网页失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			return false;
		}

		vec_iter = SplitStringReturn.begin();
		while(vec_iter != SplitStringReturn.end())
		{
			//分离内容的方法与关注相同
			unt = vec_iter->Find(">");
			Fans_URL = vec_iter->Left(unt);
			Fans_UID = vec_iter->Mid(unt);
			m_FansList.insert(std::make_pair(Fans_URL,Fans_UID));
			++vec_iter;
		}

		m_iter = m_FansList.begin();
		CString Out;
		while(m_iter != m_FansList.end())
		{
			Out.Format("\n用户昵称：%s ----  用户URL：%s\n",m_iter->second,m_iter->first);
			m_Result.Write(Out,Out.GetLength());
			++m_iter;
		}


		CString JumpNextPage;
		//判断页数是否合法
		if(Now_Page > Fans_Page)
		{
			break;
		}

		UpdateInfo.Format(_T("正在遍历粉丝，当前页%d，共%d"),Now_Page,Fans_Page);
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

		JumpNextPage.Format("?page=%d",++Now_Page);
		JumpNextPage = URL+JumpNextPage;
		Request = "GET " + JumpNextPage + " HTTP/1.1\r\n"
			"Host:weibo.cn\r\n"
			"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
			"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
			"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
			//从哪个页面跳转的，虽然觉得随便填没什么关系，但是为了防止出莫名其妙的错误，按规律填上为妙
			"Referer:http://weibo.cn/" +JumpNextPage +"\r\n"
			"Cookie:"+m_cookie+";\r\n\r\n";


		UpdateInfo.Format(_T("发送请求中......"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);


		ResetSocket();
		if(!SendMsg(Request))
		{
			UpdateInfo.Format(_T("发送请求失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			return false;
		}
		if(!RecvMsg(ServerReturn_UTF8))
		{
			UpdateInfo.Format(_T("接收服务器返回失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			return false;
		}

		if(ServerReturn_UTF8.IsEmpty())
		{
			UpdateInfo.Format(_T("接收服务器返回失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("TraversalFans Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONWARNING);
			return false;
		}


		UpdateInfo.Format(_T("获取网页成功，开始编码转换"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

		ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);

		if(ServerReturn_ANSI.IsEmpty())
		{
			UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("TraversalFans Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
			return false;
		}

		//重置重试计数
		count = 0;
		while(!tools.SearchString(ServerReturn_ANSI,"HTTP/1.1 200 OK"))
		{
			++count;
			if(5 > count)
			{
				UpdateInfo.Format(_T("重试次数过多，请稍候重试，或者换个帐号"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalFoans Function Say:\n重试次数过多，请稍候重试，或者换个帐号"));
				return false;
			}
			int Res;
			Res = AfxMessageBox(_T("TraversalFans Function Say:\n服务器返回结果出乎意料了，是否稍后重新发送试试？"),MB_YESNO);
			if(IDYES == Res)
			{
				//睡久点
				tools.Sleep();
				tools.Sleep();
				tools.Sleep();
				ResetSocket();

				UpdateInfo.Format(_T("发送请求中......"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

				if(!SendMsg(Request))
				{
					UpdateInfo.Format(_T("发送请求失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}

				if(!RecvMsg(ServerReturn_UTF8))
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}
				if(ServerReturn_UTF8.IsEmpty())
				{
					UpdateInfo.Format(_T("接收服务器返回失败"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFans Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
					return false;
				}

				UpdateInfo.Format(_T("获取网页成功，开始编码转换"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

				ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
				if(ServerReturn_ANSI.IsEmpty())
				{
					UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					AfxMessageBox(_T("TraversalFans Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}
	CloseSocket();
	UpdateInfo.Format(_T("遍历粉丝完成"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	return true;
}

//遍历微博
bool NetWork::TraversalWeibo(CString URL)
{
	IP = "180.149.153.216";
	//重置socket
	ResetSocket();

	UpdateInfo.Format(_T("开始遍历微博"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	URL.Replace("http://weibo.cn",NULL);

	CString ServerReturn_ANSI,ServerReturn_UTF8,Request;

	Request = "GET " + URL + " HTTP/1.1\r\n"
		"Host:weibo.cn\r\n"
		"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
		"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
		"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
		"Referer:http://weibo.cn/\r\n"
		"Cookie:"+m_cookie+";\r\n\r\n";

	UpdateInfo.Format(_T("发送请求中....."));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	if(!SendMsg(Request))
	{
		UpdateInfo.Format(_T("发送请求失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(!RecvMsg(ServerReturn_UTF8))
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(ServerReturn_UTF8.IsEmpty())
	{
		UpdateInfo.Format(_T("接收服务器返回失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TreaversalWeibo Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
		return false;
	}

	UpdateInfo.Format(_T("获取网页成功，开始编码转换....."));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	//转换
	ServerReturn_ANSI  = tools.UTF8ToANSI(ServerReturn_UTF8);
	if(ServerReturn_ANSI.IsEmpty())
	{
		UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TreaversalWeibo Function Say:\nUTF8ToANSI函数返回空字符串"),MB_ICONWARNING);
		return false;
	}
	//重试计数器，避免一直重试
	unsigned int count = 0;
	//判断服务器是否成功返回
	while(!tools.SearchString(ServerReturn_ANSI,"HTTP/1.1 200 OK"))
	{
		UpdateInfo.Format(_T("获取网页内容失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		++count;
		if(5 > count)
		{
			UpdateInfo.Format(_T("重试次数过多，请稍候重试，或者换个帐号"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			AfxMessageBox(_T("TreaversalWeibo Function Say:\n重试次数过多，请稍候重试，或者换个帐号"));
			return false;
		}
		int Res;
		Res = AfxMessageBox(_T("TreaversalWeibo Function Say:\n服务器返回结果出乎意料了，是否稍后重新发送试试？"),MB_YESNO);
		if(IDYES == Res)
		{

			//睡久点
			tools.Sleep();
			tools.Sleep();
			tools.Sleep();
			ResetSocket();
			UpdateInfo.Format(_T("发送请求中......"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

			if(!SendMsg(Request))
			{
				UpdateInfo.Format(_T("发送请求失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}

			if(!RecvMsg(ServerReturn_UTF8))
			{
				UpdateInfo.Format(_T("接收服务器返回失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			if(ServerReturn_UTF8.IsEmpty())
			{
				UpdateInfo.Format(_T("接收服务返回失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TreaversalWeibo Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONERROR);
				return false;
			}

			UpdateInfo.Format(_T("获取网页成功，开始编码转换"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

			ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
			if(ServerReturn_ANSI.IsEmpty())
			{
				UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TreaversalWeibo Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	//微博的网页带有<span class=.ctt.>的字样，以此来判断
	if(!tools.SearchString(ServerReturn_ANSI,"<span class=.ctt.>"))
	{
		UpdateInfo.Format(_T("错误：地址无效"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		AfxMessageBox(_T("TreaversalWeibo Function Say:\n错误：这个地址貌似不是微博列表，请传递微博列表地址"),MB_ICONWARNING);
		return false;
	}

	DebugOut.Write(ServerReturn_ANSI,ServerReturn_ANSI.GetLength());

	UpdateInfo.Format(_T("开始获取微博页数......"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	//获得微博页数
	CString All_Page,Page_Temp;
	if(!tools.ParseString(ServerReturn_ANSI,Page_Temp,";\\d{1,}.\\d{1,}.</div>"))
	{
		UpdateInfo.Format(_T("获取微博页数失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	if(!tools.ParseString(Page_Temp,All_Page,"/\\d{1,}页"))
	{
		UpdateInfo.Format(_T("获取微博页数失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	All_Page.Replace("/",NULL);
	All_Page.Replace("页",NULL);
	Weibo_Page = atoi(All_Page.GetBuffer());

	UpdateInfo.Format(_T("获取微博页数成功，微博共%d页"),Weibo_Page);
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	UpdateInfo.Format(_T("开始获取微博数"));
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	//获取微博数
	CString WeiboNum_String;
	if(!tools.ParseString(ServerReturn_ANSI,WeiboNum_String,"<span class=.tc.>.*?<"))
	{
		UpdateInfo.Format(_T("获取微博数失败"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		return false;
	}
	WeiboNum_String.Replace("<span class=\"tc\">微博",NULL);
	WeiboNum_String.Replace("[",NULL);
	WeiboNum_String.Replace("]<",NULL);
	WeiboID = atoi(WeiboNum_String.GetBuffer());
	WeiboNum_String.ReleaseBuffer();
	WeiboNum_String.Empty();

	UpdateInfo.Format(_T("获取微博数成功，微博共%d条"),WeiboID);
	SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

	//存放分割后的微博，每条微博占一个CString
	std::vector<CString> SplitStringReturn;
	std::vector<CString>::iterator vec_iter;


	//获得系统时间
	COleDateTime;
	COleDateTime datetime;  
	datetime=COleDateTime::GetCurrentTime();                                                                               
	SystemTime=datetime.Format("%Y年%m月%d日%H时%M分   "); 
	DebugInfo.Format(_T("\n-------开始遍历微博，当前时间：%s----------\n"),SystemTime);
	m_Result.Write(DebugInfo,DebugInfo.GetLength());

	CString Reg_res;
	unsigned int Now_Page = 0;

	//因为10/10这样的页数是合法的，所以为了避免少获取一页，使用<=
	while(Now_Page <= Weibo_Page)
	{
		//每遍历一页写一次，并清空map，节省内存
		m_WeiboList.clear();
		tools.Sleep();
		while(true)
		{
			CString JumpNextPage;
			JumpNextPage.Format("?page=%d",++Now_Page);

			if(Now_Page > Weibo_Page)
			{
				break;
			}
			UpdateInfo.Format(_T("正在遍历微博，当前页%d，共%d页"),Now_Page,Weibo_Page);
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

			JumpNextPage = URL+JumpNextPage;
			Request = "GET " + JumpNextPage + " HTTP/1.1\r\n"
				"Host:weibo.cn\r\n"
				"User-Agent:Mozilla/5.0 (Windows NT 6.1; rv:16.0) Gecko/20100101 Firefox/16.0\r\n"
				"Accept:text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
				"Accept-Language:zh-cn,zh;q=0.8,en-us;q=0.5,en;q=0.3\r\n"
				"Connection:keep-alive\r\n"
				//从哪个页面跳转的，虽然觉得随便填没什么关系，但是为了防止出莫名其妙的错误，按规律填上为妙
				"Referer:http://weibo.cn/" +JumpNextPage +"\r\n"
				"Cookie:"+m_cookie+";\r\n\r\n";

			UpdateInfo.Format(_T("发送请求中......"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

			ResetSocket();
			if(!SendMsg(Request))
			{			
				UpdateInfo.Format(_T("发送请求失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			if(!RecvMsg(ServerReturn_UTF8))
			{
				UpdateInfo.Format(_T("接收服务器返回失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}

			if(ServerReturn_UTF8.IsEmpty())
			{
				UpdateInfo.Format(_T("接收服务器返回失败"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalWeibo Function Say:\n错误：服务器无返回，请稍候重试"),MB_ICONWARNING);
				return false;
			}

			UpdateInfo.Format(_T("获取页面成功，开始编码转换"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);

			ServerReturn_ANSI = tools.UTF8ToANSI(ServerReturn_UTF8);
			if(ServerReturn_ANSI.IsEmpty())
			{
				UpdateInfo.Format(_T("编码转换失败，UTF8ToANSI函数返回空字符串"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				AfxMessageBox(_T("TraversalWeibo Function Say:\n错误：UTF8ToANSI函数返回空字符串"),MB_ICONERROR);
				return false;
			}

			//判断是否能够成功获取到微博内容
			if(!tools.ParseString(ServerReturn_ANSI,Reg_res,"<div class=\"c\" id=\".*?\"><div><span class=\".*?\">.+?</span></div></div>",0))
			{
				//有时候，即使是HTTP 200 OK的返回值，页面也有可能是空的，并没有内容
				//这样的话，就得跳到下一页，避免遇到这样的情况导致后面的页面不会被获取
				DebugInfo.Format(_T("TraversalWeibo Function：第%d页获取不到内容，跳转下一页"),Now_Page);
				DebugOut.Write(DebugInfo,DebugInfo.GetLength());
				DebugOut.Flush();
				UpdateInfo.Format(_T("第%d页获取不到内容，跳转到下一页"),Now_Page);
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			}
			else
			{
				break;
			}
		}//while循环的结束花括号


		UpdateInfo.Format(_T("开始解析微博"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
		//删除掉不要的，方便稍后提取
		//移除置顶微博的信息
		//2013-2-27 BUG
		//删除后置顶微博将不会被统计
		tools.RemoveString(Reg_res,"<div><img alt=\"M\" src=\"http://.+?\" />&nbsp;<a href=\"/member/pay.+?>开通会员"
			"</a><br/>\\[<span class=\"kt\">置顶</span>\\]<span class=\"cmt\">会员特权</span>&nbsp;<span class=\"ctt\">");
		//删除『赞』的链接
		tools.RemoveString(Reg_res,"&nbsp;<a href=\"http://weibo.cn/attitude/.+?>");
		//删除『转发』的链接
		tools.RemoveString(Reg_res,"&nbsp;<a href=\"http://weibo.cn/repost/.+?>");
		//删除『评论』的链接
		tools.RemoveString(Reg_res,"&nbsp;<a href=\"http://weibo.cn/comment/.+?>");
		//删除『收藏』的链接
		tools.RemoveString(Reg_res,"&nbsp;<a href=\"http://weibo.cn/fav/addFav/.+?>");
		//删除『话题』的链接
		tools.RemoveString(Reg_res,"<a href=\"http://huati.weibo.cn/.+?>");
		//删除『小图』的链接
		tools.RemoveString(Reg_res,"<a href=\"http://weibo.cn/mblog/pic/.+?</a>&nbsp;");
		//删除来自非网页客户端的链接
		tools.RemoveString(Reg_res,"<a href=\"http://weibo.cn/sinaurl.+?>");
		//删除&nbsp
		tools.RemoveString(Reg_res,"&nbsp");
		//删除id
		tools.RemoveString(Reg_res,"<div class=\"c\" id=\".*?\"><div>");

		//这样就把精简后当前页的微博存放到Reg_res这个字符串中
		//然后调用SplitString分割字符串，以</span></div></div>为标志分割每条微博
		SplitStringReturn = tools.SplitString(Reg_res,"<span class=.+?</span></div></div>");

		if(SplitStringReturn.empty())
		{
			UpdateInfo.Format(_T("微博解析失败"));
			SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
			return false;
		}

		//分割后字符串容器的迭代器
		vec_iter = SplitStringReturn.begin();

		while(vec_iter != SplitStringReturn.end())
		{
			CString Temp;
			//处理转发的微博
			if(tools.SearchString(*vec_iter,"<span class=\"cmt\">"))
			{
				//如果微博有@到别人，在源代码会显示一大串URL，为了好看，将该URL替换成@
				if(!tools.ReplaceString(*vec_iter,"@",";<a href=\"http://weibo.cn/.+?\">"))
				{
					UpdateInfo.Format(_T("错误！！搜索不到被@用户的URL！！"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					DebugInfo.Format(_T("错误！！搜索不到被@用户的URL！！"));
					DebugOut.Write(DebugInfo,DebugInfo.GetLength());
				}
				//删除用户后面的标识（如v认证）
				if(!tools.ReplaceString(*vec_iter,"","</a><img src=\".+?/>;"))
				{
					UpdateInfo.Format(_T("错误！！搜索不到用户标识！！"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					DebugInfo.Format(_T("错误！！搜索不到用户标识！！"));
					DebugOut.Write(DebugInfo,DebugInfo.GetLength());
				}
				//删除微博正文前的标识
				if(!vec_iter->Replace("</span><span class=.+?>",NULL))
				{
					UpdateInfo.Format(_T("错误！！搜索不到</span><span class=\"ctt\">！！"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					DebugInfo.Format(_T("\n错误！！搜索不到</span><span class=\"ctt\">！！\n"));
					DebugOut.Write(DebugInfo,DebugInfo.GetLength());
					//return false;
				}

				if(!tools.ParseString(*vec_iter,Temp,"<span class=\"cmt\">.+?<span class=\"cmt\">"))
				{
					UpdateInfo.Format(_T("错误！！提取不到微博正文，请重试！！"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}
				//删除HTML标记
				if(!tools.ReplaceString(Temp," ","<.+?>"))
				{
					UpdateInfo.Format(_T("错误！！搜索不到HTML标记！！"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					DebugInfo.Format(_T("错误！！搜索不到HTML标记！！"));
					DebugOut.Write(DebugInfo,DebugInfo.GetLength());
					return false;
				}
				//将整理后的微博填入到WeiboInfo的TEXT字段
				WeiboInfo.Text = Temp;

				//如果带图片则处理
				if(tools.ParseString(*vec_iter,Temp,"<a href=\"http://weibo.cn/mblog/.+?\">"))
				{
					Temp.Replace("<a href=\"",NULL);
					Temp.Replace("\"",NULL);
					WeiboInfo.PicUrl = Temp;
					WeiboInfo.HasPic = true;
				}
				//将转发标记设置为true
				WeiboInfo.IsResport = true;
			}

			//处理非转发的微博
			else
			{
				//整理微博正文格式
				if(!tools.ParseString(*vec_iter,Temp,"<span class=\"ctt\">.+?赞"))
				{
					return false;
				}
				Temp.Replace("<span class=\"ctt\">",NULL);
				Temp.Replace("赞",NULL);

				//删除HTML标记
				if(!tools.ReplaceString(Temp," ","<.+?>"))
				{
					UpdateInfo.Format(_T("错误！！搜索不到HTML标记！！"));
					SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
					return false;
				}
				//将整理后的微博填入到WeiboInfo的TEXT字段
				WeiboInfo.Text = Temp;

				//处理带图片的情况
				if(tools.ParseString(*vec_iter,Temp,"<a href=\"http://weibo.cn/mblog/.+?\">"))
				{
					Temp.Replace("<a href=\"",NULL);
					Temp.Replace("\"",NULL);
					WeiboInfo.PicUrl = Temp;
					WeiboInfo.HasPic = true;
				}

				WeiboInfo.IsResport = false;
			}

			//删除原图的提示
			WeiboInfo.Text.Replace("原图",NULL);

			//获取转发数
			if(!tools.ParseString(*vec_iter,Temp,"[^原文]转发\\[\\d{1,}\\]"))
			{
				UpdateInfo.Format(_T("错误！！获取不到转发数！！"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			Temp.Replace(">转发[",NULL);
			Temp.Replace("]",NULL);
			WeiboInfo.Resport = atoi(Temp.GetBuffer());
			Temp.ReleaseBuffer();

			//获取评论数
			if(!tools.ParseString(*vec_iter,Temp,"[^原文]评论\\[\\d{1,}\\]"))
			{
				UpdateInfo.Format(_T("错误！！获取不到评论数！！"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			Temp.Replace(">评论[",NULL);
			Temp.Replace("]",NULL);
			WeiboInfo.Comment = atoi(Temp.GetBuffer());
			Temp.ReleaseBuffer();

			//获取来源
			if(!tools.ParseString(*vec_iter,Temp,"来自.*?<"))
			{
				UpdateInfo.Format(_T("错误！！获取不到来源！！"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			Temp.Replace("来自",NULL);
			Temp.Replace("<",NULL);
			WeiboInfo.From = Temp;

			//获取时间
			//2013-2-27 BUG
			//目前只能以网页源代码中的时间显示，以相对时间
			//如获取网页是12点，则11:50发布的微博会显示10分钟前
			//下一步打算以系统时间来进行计算，显示完整时间
			if(!tools.ParseString(*vec_iter,Temp,"<span class=\"ct\">.+?;"))
			{
				UpdateInfo.Format(_T("错误！！获取不到发布时间！！"));
				SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
				return false;
			}
			Temp.Replace("<span class=\"ct\">",NULL);
			Temp.Replace(";",NULL);
			WeiboInfo.Time = Temp;

			//将整理好的微博插入map中
			//以微博ID索引
			m_WeiboList.insert(std::make_pair(WeiboID,WeiboInfo));
			//WeiboID为微博数，倒序存放
			--WeiboID;
			++vec_iter;


		}
		std::map<unsigned int,DataStruct,std::greater<unsigned int> >::iterator  map_iter;
		map_iter = m_WeiboList.begin();
		CString out;
		while(map_iter != m_WeiboList.end())
		{
			if(map_iter->second.HasPic)
			{
				if(map_iter->second.IsResport)
				{
					out.Format("ID:%d\n微博正文:%s\n是否转发:是\n评论数:%d\n转发数:%d\n图片URL:%s\n发布时间:%s\n来源:%s\n\n",map_iter->first,map_iter->second.Text,
						map_iter->second.Comment,map_iter->second.Resport,map_iter->second.PicUrl,map_iter->second.Time,
						map_iter->second.From);
				}
				else
				{
					out.Format("ID:%d\n微博正文:%s\n是否转发:否\n评论数:%d\n转发数:%d\n图片URL:%s\n发布时间:%s\n来源:%s\n\n",map_iter->first,map_iter->second.Text,
						map_iter->second.Comment,map_iter->second.Resport,map_iter->second.PicUrl,map_iter->second.Time,
						map_iter->second.From);
				}
			}
			else
			{
				if(map_iter->second.IsResport)
				{
					out.Format("ID:%d\n微博正文:%s\n是否转发:是\n评论数:%d\n转发数:%d\n\n发布时间:%s\n来源:%s\n\n",map_iter->first,map_iter->second.Text,
						map_iter->second.Comment,map_iter->second.Resport,map_iter->second.Time,
						map_iter->second.From);
				}
				else
				{
					out.Format("ID:%d\n微博正文:%s\n是否转发:否\n评论数:%d\n转发数:%d\n\n发布时间:%s\n来源:%s\n\n",map_iter->first,map_iter->second.Text,
						map_iter->second.Comment,map_iter->second.Resport,map_iter->second.Time,
						map_iter->second.From);
				}
			}
			m_Result.Write(out,out.GetLength());
			++map_iter;
		}
		UpdateInfo.Format(_T("微博解析成功"));
		SendMessage(hwnd,WM_UPDATEMISSIONINFO,(WPARAM)(LPCTSTR)UpdateInfo,0);
	}
	return true;
}