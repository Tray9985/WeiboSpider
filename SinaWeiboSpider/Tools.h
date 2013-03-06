#pragma once
#include <vector>

//自定义消息，用于更新显示任务消息的控件
#define WM_UPDATEMISSIONINFO (WM_USER + 100)

class Tools
{
public:
	Tools(void);
	~Tools(void);
	//解析字符串  参数1:待解析的字符串  参数2:存放解析后的字符串  参数3:解析规则
	bool ParseString(CString&,CString&,CString);
	//解析字符串2 参数1:待解析的字符串 参数2:存放解析后的字符串 参数3:解析规则 参数4:表示在每个结果
	//后面添加回车
	bool ParseString(CString&,CString&,CString,int);
	//解析字符串，返回第一个符合要求的字符串
	//参数1:待解析的字符串，参数2:存放解析后的字符串，参数3:解析规则
	bool FindFirstString(CString&,CString&,CString);
	//参数1:待解析的字符串，参数2:解析规则 返回符合结果的vector容器
	std::vector<CString> ParseString(CString&,CString);
	//格式化字符串，用于删除多余的回车，空格等
	//参数1:待解析的字符串    将格式化后的字符串写入到参数1中
	bool FormatString(CString&);
	//寻找字符串，找到返回ture，否则返回false
	//参数1:待查找的字符串 参数2:查找规则
	bool SearchString(CString&,CString);
	//替换字符串 将将替换后的字符串写入到参数1中
	//参数1:待替换的字符串 参数2 替换字符串 参数3 替换规则
	bool ReplaceString(CString&,CString,CString);
	//删除字符串  参数1：待删除的字符串 参数2：删除规则
	//将删除后的字符串写入到参数1中
	bool RemoveString(CString&,CString);
	//接收宽字节字符串，返回多字节字符串
	CString UTF8ToANSI(CString);
	//字符串分割函数  接受CString字符串进行分割，返回vector，默认以空格分隔
	std::vector<CString> SplitString(CString);
	//字符串分割函数重载版本  接受CString字符串进行分割，返回vector，可以自己指定分割规则
	std::vector<CString> SplitString(CString,CString);
	void Sleep();
	HWND hwnd;
	static bool IsAutoSleep;
private:
	CString UpdateInfo;
	unsigned int SleepTime;
};