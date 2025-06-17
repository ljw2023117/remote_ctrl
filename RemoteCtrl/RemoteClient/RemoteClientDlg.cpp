﻿
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "WatchDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_port(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_port);
	DDX_Control(pDX, IDC_TREE_DIR, m_Tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_MESSAGE(WM_SEND_PACKET, &CRemoteClientDlg::OnSendPacket)	// 自定义消息第三步：在消息列表中注册消息
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

void CRemoteClientDlg::threadEntryForWatchData(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CRemoteClientDlg::threadWatchData()
{
	Sleep(50);
	CClientSocket* pClient = CClientSocket::getInstance();
	while (!m_isClosed)
	{
		if (m_isFull == false)
		{
			int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 1);
			if (ret == 6)
			{
				BYTE* pData = (BYTE*)pClient->GetPacket().strData.c_str();
				// GlobalAlloc用于分配全局内存块的函数调用全局堆（Global Heap）,
				// 全局堆（Global Heap）是 Windows 特有的内存分配机制，用于系统级交互（如 COM、剪贴板）；
				HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
				if (hMem == NULL)
				{
					TRACE("内存不足了\r\n");
					Sleep(1);
					continue;
				}
				IStream* pStream = NULL;
				HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
				if (hRet == S_OK)
				{
					ULONG length = 0;
					pStream->Write(pData, pClient->GetPacket().strData.size(), &length);
					LARGE_INTEGER bg = { 0 };	// 大整数
					pStream->Seek(bg, STREAM_SEEK_SET, NULL);
					// CImage 内部维护了一个 Windows 的位图对象（HBITMAP）
					// 通过强制类型转换(HBITMAP)，你可以获取这个底层的位图句柄。
					if ((HBITMAP)m_image != NULL) m_image.Destroy();
					m_image.Load(pStream);
					m_isFull = true;
				}
			}
			else Sleep(1);
		}
		else Sleep(1);
	}
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_Tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_Tree.HitTest(ptMouse, 0);
	if (hTreeSelected == NULL)
	{
		TRACE("没选中节点\n");
		return;
	}
	//if (m_Tree.GetChildItem(hTreeSelected) == NULL) return; // 没有子节点说明是文件
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CStringW strPath = GetPath(hTreeSelected);
	TRACE("client path - %ls\r\n", strPath);
	int nCmd = SendCommandPacket(2, false, (BYTE*)strPath.GetString(), strPath.GetLength() * 2 + 2);
	CClientSocket* pClient = CClientSocket::getInstance();
	PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	TRACE(L"recieve file: name: %s, HasNext :%d, isDirectory: %d\r\n", pInfo->szFileName, pInfo->HasNext, pInfo->IsDirectory);
	int count = 0;	
	while (pInfo->HasNext)
	{
		if (pInfo->IsDirectory)
		{
			if (CString(pInfo->szFileName) != L"." && CString(pInfo->szFileName) != L"..")
			{
				TRACE(L"recieve file: name: %s, HasNext :%d, isDirectory: %d\r\n", pInfo->szFileName, pInfo->HasNext, pInfo->IsDirectory);
				HTREEITEM hTmp = m_Tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
				TRACE(L"new node name %s\r\n", m_Tree.GetItemText(hTmp));
				m_Tree.InsertItem(_T(""), hTmp, TVI_LAST);
			}
		}
		else
		{
			TRACE(L"recieve file: name: %s, HasNext :%d, isDirectory: %d\r\n", pInfo->szFileName, pInfo->HasNext, pInfo->IsDirectory);
			m_List.InsertItem(0, pInfo->szFileName);
		}
		count++;
		int cmd = pClient->DealCommand();
		TRACE(L"next DealCommand cmd = %d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
		TRACE(L"recieve file: name: %s, HasNext :%d, isDirectory: %d\r\n", pInfo->szFileName, pInfo->HasNext, pInfo->IsDirectory);
	}
	pClient->CloseSocket();
	TRACE("clostsocket client file count = %d\r\n", count);
}

// 在删除文件后调用的，更新当前文件夹中的文件
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength() * 2 + 2);
	CClientSocket* pClient = CClientSocket::getInstance();
	PFILEINFO pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	while (pInfo->HasNext)
	{
		TRACE("[%ls] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory)
			m_List.InsertItem(0, pInfo->szFileName);
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)pClient->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}

void CRemoteClientDlg::threadEntryForDownFile(void* arg)
{
	CRemoteClientDlg* thiz = (CRemoteClientDlg*)arg;
	thiz->threadDownFile();
	_endthread();
}

void CRemoteClientDlg::threadDownFile()
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	CFileDialog dlg(FALSE, NULL, strFile, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, this);
	if (dlg.DoModal() == IDOK)
	{
		FILE* pFile;
		_wfopen_s(&pFile, dlg.GetPathName(), L"wb+");
		if (pFile == NULL)
		{
			AfxMessageBox(_T("本地没有权限保存该文件，或者文件无法创建！！！"));
			m_dlgStatus.ShowWindow(SW_HIDE);
			EndWaitCursor();
			return;
		}
		HTREEITEM hSelected = m_Tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		CClientSocket* pClient = CClientSocket::getInstance();
		do
		{
			int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)strFile.GetString());
			if (ret < 0)
			{
				AfxMessageBox(_T("执行下载命令失败！！"));
				TRACE("执行下载命令失败：ret = %d\r\n", ret);
				break;
			}
			long long nLength = *(long long*)pClient->GetPacket().strData.c_str();
			TRACE("nLength = %lld\r\n", nLength);
			if (nLength == 0)
			{
				AfxMessageBox(_T("文件长度为零或者无法读取文件！！"));
				break;
			}
			long long nCount = 0;
			while (nCount < nLength)
			{
				ret = pClient->DealCommand();
				if (ret < 0)
				{
					AfxMessageBox(_T("传输失败！！"));
					TRACE("传输失败：ret = %d\r\n", ret);
					break;
				}
				fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pFile);
				nCount += pClient->GetPacket().strData.size();
			}
			TRACE("nCount = %d\r\n", nCount);
		} while (false);
		fseek(pFile, 0, SEEK_END);
		long long data = _ftelli64(pFile);
		TRACE("dataLength = %lld\r\n", data);
		fclose(pFile);
		pClient->CloseSocket();
		MessageBox(_T("下载完成！！"), _T("完成"));
	}
	else
	{
		MessageBox(_T("取消下载！！"), _T("取消下载"));
	}
	m_dlgStatus.ShowWindow(SW_HIDE);
	EndWaitCursor();
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree)
{
	while (m_Tree.GetChildItem(hTree))
		m_Tree.DeleteItem(m_Tree.GetChildItem(hTree));
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString path = m_Tree.GetItemText(hTree);
	while (m_Tree.GetParentItem(hTree))
	{
		hTree = m_Tree.GetParentItem(hTree);
		path = m_Tree.GetItemText(hTree) + '\\' + path;
	}
	path += '\\';
	return path;
}

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, m_port);
	if (!ret)
	{
		AfxMessageBox(_T("网络初始化失败！"));
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret %d\r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("ack:%d\r\n", cmd);
	if (bAutoClose) pClient->CloseSocket();
	return cmd;
}

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	//m_server_address = 0x7F000001; // 127.0.0.1
	m_server_address = 0xC0A8FE81;	// 192.168.254.129s
	m_port = 9527;
	UpdateData(false);
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	m_isFull = false;
	
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
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
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRemoteClientDlg::OnBnClickedBtnTest()
{
	int ret = SendCommandPacket(1981);
	if (ret) AfxMessageBox(_T("连接成功"));
	else AfxMessageBox(_T("连接失败"));
	return;
}

void CRemoteClientDlg::OnBnClickedBtnFileinfo()
{
	int ret = SendCommandPacket(1);
	if (ret == -1)
	{
		AfxMessageBox(_T("命令处理失败!!!"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;
	std::string dr;
	m_Tree.DeleteAllItems();
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] != ',')
		{	
			CString buf = CString(drivers[i]) + ':';
			TVINSERTSTRUCT tvInsert;
			tvInsert.hParent = NULL;
			tvInsert.hInsertAfter = NULL;
			tvInsert.item.mask = TVIF_TEXT;
			tvInsert.item.pszText = buf.GetBuffer();
			HTREEITEM hTemp = m_Tree.InsertItem(&tvInsert);
			//m_Tree.InsertItem(NULL, hTemp, TVI_LAST);
		}
	}
}

// 左键单击文件树控件
void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}

// 右键单击文件树控件
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}

void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse, ptList;
	GetCursorPos(&ptMouse);
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);	// 传入传出函数，将屏幕坐标转换为客户区坐标
	int ListSelected = m_List.HitTest(ptList);
	if (ListSelected < 0)
	{
		TRACE("未点到列表控件\r\n");
		return;
	}
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);
	CMenu* pPopup = menu.GetSubMenu(0);	// 选中菜单中第一个子菜单
	if (pPopup != NULL)
	{
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, ptMouse.x, ptMouse.y, this); // 弹出
	}
}

void CRemoteClientDlg::OnDownloadFile()
{
	// TODO: 在此添加命令处理程序代码
	_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	BeginWaitCursor();
	m_dlgStatus.m_info.SetWindowText(_T("命令正在执行中！"));
	m_dlgStatus.ShowWindow(SW_SHOW);
	m_dlgStatus.CenterWindow(this);
	m_dlgStatus.SetActiveWindow();
}

void CRemoteClientDlg::OnDeleteFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength() * 2 + 2);
	if (ret < 0)
	{
		AfxMessageBox(_T("删除文件命令执行失败！"));
	}
	LoadFileCurrent();
}

void CRemoteClientDlg::OnRunFile()
{
	// TODO: 在此添加命令处理程序代码
	HTREEITEM hSelected = m_Tree.GetSelectedItem();
	CString strPath = GetPath(hSelected);
	int nSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nSelected, 0);
	strFile = strPath + strFile;
	int ret = SendCommandPacket(3, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength() * 2 + 2);
	if (ret < 0)
	{
		AfxMessageBox(_T("打开文件命令执行失败！"));
	}
}

LRESULT CRemoteClientDlg::OnSendPacket(WPARAM wParam, LPARAM lParam)
{
	int ret = 0;
	int cmd = wParam >> 1;
	switch (cmd)
	{
	case 4:
	{
		CString strFile = (LPCTSTR)lParam;
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)strFile.GetString(), strFile.GetLength() * 2 + 2);
	}
		break;
	case 5:
		ret = SendCommandPacket(cmd, wParam & 1, (BYTE*)lParam, sizeof(MOUSEEV));
		break;
	case 6:
	case 7:
	case 8:
	{
		ret = SendCommandPacket(cmd, wParam & 1);
	}
		break;
	default:
		ret = -1;
	}

	return ret;
}

void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	m_isClosed = false;
	CWatchDlg dlg(this);
	HANDLE hThread = (HANDLE)_beginthread(CRemoteClientDlg::threadEntryForWatchData, 0, this);
	dlg.DoModal();
	m_isClosed = true;
	WaitForSingleObject(hThread, 500);	// 等待子线程结束
}
