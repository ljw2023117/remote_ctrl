
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1)	// 自定义消息第一步：定义发送数据包的消息

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:
	bool isFull() const { return m_isFull; }
	CImage& GetImage() { return m_image; }
	void SetImageIsFull(bool isFull = false) { m_isFull = isFull; }
private:
	CImage m_image;	// 缓存
	bool m_isFull;	// 缓存是否有数据
	bool m_isClosed;// 监视是否关闭
private:
	static void threadEntryForWatchData(void* arg);	// 监视程序入口函数
	void threadWatchData();	// 监视程序线程函数
	void LoadFileInfo();	// 加载文件信息
	void LoadFileCurrent();	// 加载当前文件信息
	static void threadEntryForDownFile(void* arg);	// 下载文件线程入口函数
	void threadDownFile();	// 下载文件线程函数
	void DeleteTreeChildrenItem(HTREEITEM hTree);	// 将文件树上该节点的所有子节点删除
	CString GetPath(HTREEITEM hTree);	// 通过文件树的节点生成路径
	/*
	封装发送包
	1 查看磁盘分区
	2 查看指定目录下的文件
	3 打开文件
	4 下载文件
	9 删除文件
	5 鼠标操作
	6 发送屏幕内容
	7 锁机
	8 解锁
	1981 测试连接
	返回值：是命令号，如果小于0则是错误
	*/
	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);

// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	DWORD m_server_address;
	short m_port;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_Tree;
	CListCtrl m_List;
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);	// 自定义消息第二步：定义发包消息的响应函数
	afx_msg void OnBnClickedBtnStartWatch();
};
