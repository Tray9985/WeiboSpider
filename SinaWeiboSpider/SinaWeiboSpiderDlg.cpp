
// SinaWeiboSpiderDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "SinaWeiboSpider.h"
#include "SinaWeiboSpiderDlg.h"
#include "afxdialogex.h"
#include "NetWork.h"
#include <vector>
#include "Tools.h"
#include <fstream>
#include "HTMLParser.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//全局对象DlgData,方便在其他线程中访问
DlgData Dlgdata;
NetWork nt;
BOOL IsLogin = false;
bool IsLoginThreadRun = false;
bool IsMissionThreadRun = false;

//定义一些代码
#define InitFail -1
#define LoginFail -2
#define ConnectionFail -3
#define TraversalFollowFail -4
#define TraversalWeiboFail -5
#define TraversalFansFail -6


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
	//	afx_msg LRESULT OnUpDateText(WPARAM wParam, LPARAM lParam);
public:
	CStatic m_Pic;
	virtual BOOL OnInitDialog();
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CC, m_Pic);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
	//	ON_MESSAGE(WM_UPDATETEXT, &CAboutDlg::OnUpDateText)
END_MESSAGE_MAP()


// CSinaWeiboSpiderDlg 对话框




CSinaWeiboSpiderDlg::CSinaWeiboSpiderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSinaWeiboSpiderDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_UserName = _T("");
	m_Password = _T("");
	m_FansUrl = _T("");
	m_FollowUrl = _T("");
	m_WeiboUrl = _T("");
	Dlgdata.MissionChoose.IsTraversalFans = false;
	Dlgdata.MissionChoose.IsTraversalFollow = false;
	Dlgdata.MissionChoose.IsTraversalFollow = false;
}

void CSinaWeiboSpiderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USER_NAME, m_UserName);
	DDV_MaxChars(pDX, m_UserName, 256);
	DDX_Text(pDX, IDC_USER_PASSWORD, m_Password);
	DDV_MaxChars(pDX, m_Password, 512);
	DDX_Check(pDX, IDC_TRAVEFANS, m_IsTreaveFans);
	DDX_Check(pDX, IDC_TRAVEFOLLOW, m_IsTreaveFollow);
	DDX_Check(pDX, IDC_TRAVEWEIBO, m_IsTreaveWeibo);
	DDX_Text(pDX, IDC_FANSURL, m_FansUrl);
	DDV_MaxChars(pDX, m_FansUrl, 512);
	DDX_Text(pDX, IDC_FOLLOWURL, m_FollowUrl);
	DDV_MaxChars(pDX, m_FollowUrl, 512);
	DDX_Text(pDX, IDC_WEIBOURL, m_WeiboUrl);
	DDV_MaxChars(pDX, m_WeiboUrl, 512);
	DDX_Control(pDX, IDC_ISLOGIN, m_LoginStatus);
	DDX_Check(pDX, IDC_AUTOSLEEP, m_AutoSleep);
	DDX_Control(pDX, IDC_STATUS, m_ChooseStatus);
}

BEGIN_MESSAGE_MAP(CSinaWeiboSpiderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_START, &CSinaWeiboSpiderDlg::OnBnClickedStart)
	ON_BN_CLICKED(IDC_LOGIN, &CSinaWeiboSpiderDlg::OnBnClickedLogin)
	ON_STN_CLICKED(IDC_PATH, &CSinaWeiboSpiderDlg::OnStnClickedPath)
	//	ON_MESSAGE(WM_UPDATETEXT, &CSinaWeiboSpiderDlg::OnUpDateText)
	ON_MESSAGE(WM_UPDATEMISSIONINFO, &CSinaWeiboSpiderDlg::OnUpDateMissionInfo)
	ON_BN_CLICKED(IDC_CLEAREDIT, &CSinaWeiboSpiderDlg::OnBnClickedClearedit)
END_MESSAGE_MAP()


// CSinaWeiboSpiderDlg 消息处理程序

BOOL CSinaWeiboSpiderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	IsLogin = false;
	//将对话框的句柄复制给NetWork类，方便更新
	nt.SetDlgHwnd(m_hWnd);
	//将自动睡眠设置为false
	m_AutoSleep = false;
	Tools::IsAutoSleep = false;
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CSinaWeiboSpiderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CSinaWeiboSpiderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CSinaWeiboSpiderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//登录线程
DWORD WINAPI LoginThread(LPVOID lpParameter);
//执行任务线程
DWORD WINAPI MissionThread(LPVOID lpParameter);

void CSinaWeiboSpiderDlg::OnBnClickedStart()
{
	//判断任务线程是否启动
	if(IsMissionThreadRun)
	{
		MessageBox(_T("线程已启动，请等待线程完成"));
		return;
	}
	if(!IsLogin)
	{
		MessageBox(_T("错误：必须登录后才能进行下一步，请先登录"));
		return;
	}

	//从输入框获取数据并校验合法性
	UpdateData(true);
	if(m_IsTreaveWeibo)
	{

		if(m_WeiboUrl.IsEmpty())
		{
			MessageBox(_T("如果勾选遍历微博，必须填写微博地址，请检查"));
			return;
		}
		Dlgdata.MissionChoose.IsTraversalWeibo = true;
		Dlgdata.m_WeiboUrl = m_WeiboUrl;
	}
	if(m_IsTreaveFans)
	{

		if(m_FansUrl.IsEmpty())
		{
			MessageBox(_T("如果勾选遍历粉丝，必须填写粉丝地址，请检查"));
			return;
		}
		Dlgdata.MissionChoose.IsTraversalFans = true;
		Dlgdata.m_FansUrl = m_FansUrl;
	}
	if(m_IsTreaveFollow)
	{

		if(m_FollowUrl.IsEmpty())
		{
			MessageBox(_T("如果勾选遍历关注，必须填写关注地址，请检查"));
			return;
		}
		Dlgdata.MissionChoose.IsTraversalFollow = true;
		Dlgdata.m_FollowUrl = m_FollowUrl;
	}
	if(!(m_IsTreaveWeibo || m_IsTreaveFans || m_IsTreaveFollow))
	{
		MessageBox(_T("错误：必须选择至少一个任务，请检查"));
		return;
	}
	if(!m_AutoSleep)
	{
		BOOL m_Res;
		m_Res = MessageBox(_T("警告:如果不勾选此选项，有可能因为遍历速度过快而导致帐号被限制甚至封禁！\n建议勾选此选项，是否勾选？"),0,MB_YESNO| MB_ICONWARNING);
		if(IDYES == m_Res)
		{
			m_AutoSleep = true;
			Tools::IsAutoSleep = true;
			UpdateData(false);
		}
		else
		{
			Tools::IsAutoSleep = false;
		}
	}
	Dlgdata.m_ChooseStatus = &m_ChooseStatus;
	//创建任务线程
	HANDLE hd;
	hd = CreateThread(NULL,0,MissionThread,NULL,0,NULL);
	CloseHandle(hd);
}

void CSinaWeiboSpiderDlg::OnBnClickedLogin()
{
	//设置目录
	nt.SetSaveFilePath(SaveFilePath);
	if(IsLoginThreadRun)
	{
		MessageBox(_T("登录线程已启动，请等待线程完成"));
		return;
	}
	if(IsLogin)
	{
		int Res;
		Res = MessageBox(_T("当前已经登录，确定要重新登录？"),0,MB_YESNO);
		if(IDNO == Res)
		{
			return;
		}
	}
	//获得输入框内容并校验合法性
	UpdateData(true);
	if(m_UserName.IsEmpty())
	{
		MessageBox(_T("错误：用户名不能为空"),0,MB_ICONWARNING);
		return;
	}
	if(m_Password.IsEmpty())
	{
		MessageBox(_T("错误：密码不能为空"),0,MB_ICONWARNING);
		return;
	}
	//将用户名，密码与静态文本控件赋值给全局结构体Dlgdata,以便在线程中访问
	Dlgdata.m_Password = m_Password;
	Dlgdata.m_Username = m_UserName;
	Dlgdata.m_LoginStatus = &m_LoginStatus;

	m_LoginStatus.SetWindowTextA(_T("正在登录，请稍等"));
	//创建登录线程
	HANDLE hd;
	hd = CreateThread(NULL,0,LoginThread,NULL,0,NULL);
	CloseHandle(hd);
}


void CSinaWeiboSpiderDlg::OnStnClickedPath()
{
#if 0
	// TODO: 在此添加控件通知处理程序代码}
	CFileDialog SaveFileDlg(false,".txt","爬虫结果",6UL,"文本格式|*.txt||");
	if(SaveFileDlg.DoModal()==IDOK)
	{
		//取回完整路径
		SaveFilePath = SaveFileDlg.GetPathName();
	}
	CString Out;
	Out.Format(_T("设置存放的目录为:%s"),SaveFilePath);
	GetDlgItem(IDC_PATH)->SetWindowTextA(Out);
	nt.SetSaveFilePath(SaveFilePath);
#endif
	CFile fp;
	fp.Open(_T("D:\\xmll.txt"),CFile::modeReadWrite);
	char *pStr = new char[fp.GetLength()+1];
	fp.Read(pStr,fp.GetLength());
	CString szStr(pStr);
	delete []pStr;
	CHTMLParser ps;
	ps.ReadHTML(szStr);
	ps.ParserHTML();
	Sleep(100);
}

//登录线程
DWORD WINAPI LoginThread(LPVOID lpParameter)
{
	IsLoginThreadRun =  true;
	CString Msg;
	if(!nt.InitSocket())
	{
		Msg.Format(_T("初始化Socket失败"));
		Dlgdata.m_LoginStatus->SetWindowTextA(Msg);
		IsLoginThreadRun = false;
		return InitFail;
	}
	if(!nt.ConnectionServer(_T("221.179.175.244")))
	{
		Msg.Format(_T("连接服务器失败"));
		Dlgdata.m_LoginStatus->SetWindowTextA(Msg);
		IsLoginThreadRun = false;
		return ConnectionFail;
	}
	if(!nt.Login(Dlgdata.m_Username,Dlgdata.m_Password))
	{
		Msg.Format(_T("登录失败"));
		Dlgdata.m_LoginStatus->SetWindowTextA(Msg);
		IsLoginThreadRun = false;
		return LoginFail;
	}
	else
	{
		//设置登录状态
		IsLogin = true;
		Msg.Format(_T("当前已登录，登录帐号为: %s"),Dlgdata.m_Username);
		Dlgdata.m_LoginStatus->SetWindowTextA(Msg);
		Msg.Empty();
	}
	IsLoginThreadRun = false;
	return true;
}

//任务线程
DWORD WINAPI MissionThread(LPVOID lpParameter)
{
	bool IsMissionSuccess = true;
	IsMissionThreadRun = true;
	if(Dlgdata.MissionChoose.IsTraversalFans)
	{
		Dlgdata.m_ChooseStatus->SetWindowTextA(_T("正在遍历粉丝......"));
		if(!nt.TraversalFans(Dlgdata.m_FansUrl))
		{
			IsMissionSuccess = false;
			Dlgdata.m_ChooseStatus->SetWindowTextA(_T("遍历粉丝失败"));
		}
		else
		{
			Dlgdata.m_ChooseStatus->SetWindowTextA(_T("遍历粉丝完成"));
		}
	}
	if(Dlgdata.MissionChoose.IsTraversalFollow)
	{
		Dlgdata.m_ChooseStatus->SetWindowTextA(_T("正在遍历关注......"));
		if(!nt.TraversalFollow(Dlgdata.m_FollowUrl))
		{
			IsMissionSuccess = false;
			Dlgdata.m_ChooseStatus->SetWindowTextA(_T("遍历关注失败"));
		}
		else
		{
			Dlgdata.m_ChooseStatus->SetWindowTextA(_T("遍历关注完成"));
		}
	}
	if(Dlgdata.MissionChoose.IsTraversalWeibo)
	{
		Dlgdata.m_ChooseStatus->SetWindowTextA(_T("正在遍历微博......"));
		if(!nt.TraversalWeibo(Dlgdata.m_WeiboUrl))
		{
			IsMissionSuccess = false;
			Dlgdata.m_ChooseStatus->SetWindowTextA(_T("遍历微博失败"));
		}
		else
		{
			Dlgdata.m_ChooseStatus->SetWindowTextA(_T("遍历微博完成"));
		}
	}
	//判断任务状态
	if(IsMissionSuccess)
	{
		Dlgdata.m_ChooseStatus->SetWindowTextA(_T("所有任务已完成，等待新指令中......"));
	}
	else
	{
		Dlgdata.m_ChooseStatus->SetWindowTextA(_T("至少有一个任务失败，等待新指令中......"));
	}
	IsMissionThreadRun = false;
	return true;
}

afx_msg LRESULT CSinaWeiboSpiderDlg::OnUpDateMissionInfo(WPARAM wParam, LPARAM lParam)
{

	int index;
	CEdit *pEdit;
	pEdit = (CEdit *)GetDlgItem(IDC_MISSIONINFO);
	index = pEdit->GetWindowTextLengthA();
	pEdit->SetSel(index,index);
	CString Temp((LPCTSTR)wParam);
	pEdit->ReplaceSel(Temp + _T("\n"));
	return 0;
}


void CSinaWeiboSpiderDlg::OnBnClickedClearedit()
{
	// TODO: 在此添加控件通知处理程序代码
	CEdit *pEdit;
	pEdit = (CEdit*)GetDlgItem(IDC_MISSIONINFO);
	pEdit->SetWindowTextA(_T(""));
}

//载入关于对话框的CC协议图像
CImage image;
BOOL CAboutDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//先判断image是否已载入
	//如果没载入直接载入，如果已经载入了，销毁image后重新载入
	if(!image)
	{
		image.Load(_T("res\\by-nc-sa.png"));
		m_Pic.SetBitmap(image);
	}
	else
	{
		image.Destroy();
		image.Load(_T("res\\by-nc-sa.png"));
		m_Pic.SetBitmap(image);
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}
