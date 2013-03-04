#pragma once
#include "Tools.h"
#include <map>
#include "DataStruct.h"



class NetWork
{
public:
	NetWork(void);
	~NetWork(void);
	//获得对话框句柄，用于PostMessage，
	void SetDlgHwnd(HWND);
	BOOL InitSocket();
	BOOL ConnectionServer(CString);
	BOOL ConnectionServer();
	BOOL Login(CString,CString);
	int SendMsg(CString&);
	BOOL RecvMsg(CString&);
	void CloseSocket();
	bool ResetSocket();
	void SetSaveFilePath(CString);
	//TODO: 参数1:要遍历的微博关注列表地址
	bool TraversalFollow(CString);
	bool TraversalFans(CString);
	bool TraversalWeibo(CString);


	unsigned int Follow_Page;
	unsigned int Weibo_Page;
	unsigned int Fans_Page;
	unsigned int WeiboID;
private:
	HWND hwnd;
	CFile DebugOut;
	CFile m_Result;
	Tools tools;
	sockaddr_in ServerAddr;
	WSADATA data;
	SOCKET Client;
	std::map<CString,CString> m_FollowList;
	std::map<CString,CString>::iterator m_iter;
	std::map<CString,CString> m_FansList;

	std::map<unsigned int,DataStruct,std::greater<unsigned int> > m_WeiboList;
	DataStruct WeiboInfo;
	//用于更新UI
	CString UpdateInfo;
	//存放password字段的随机值
	CString m_RandomPassword;
	//存放VK的随机值
	CString m_vk;
	//存放跳转的URL
	CString m_location;
	//存放登陆后的cookie
	CString m_cookie;
	//存放最后一次调用connectionServer的IP,被ResetSocket函数所使用
	CString IP;
	CString DebugInfo;
	CString SystemTime;
	int Result;
	bool IsInit;
	bool IsConnection;
	bool IsLogin;
	bool IsSaveFilePathSet;
};

