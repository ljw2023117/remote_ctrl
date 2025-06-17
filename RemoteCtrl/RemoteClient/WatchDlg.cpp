// WatchDlg.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "afxdialogex.h"
#include "WatchDlg.h"
#include "RemoteClientDlg.h"


// CWatchDlg 对话框

IMPLEMENT_DYNAMIC(CWatchDlg, CDialog)

CWatchDlg::CWatchDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_nObjWidth = -1;
	m_nObjHeight = -1;
}

CWatchDlg::~CWatchDlg()
{
}

void CWatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDlg, CDialog)
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDlg::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDlg::OnBnClickedBtnUnlock)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


// CWatchDlg 消息处理程序

// 左键双击
void CWatchDlg::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;	// 左键
		event.nAction = 1;	// 双击
		(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnLButtonDblClk(nFlags, point);
}

// 左键按下
void CWatchDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;	// 左键
		event.nAction = 2;	// 按下
		(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnLButtonDown(nFlags, point);
}

// 左键抬起
void CWatchDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;	// 左键
		event.nAction = 4;	// 抬起
		(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnLButtonUp(nFlags, point);
}

// 鼠标移动
void CWatchDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 4;	// 没有按键
		event.nAction = 8;
		(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnMouseMove(nFlags, point);
}

// 右键双击
void CWatchDlg::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;	// 右键
		event.nAction = 1;	// 双击
		(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnRButtonDblClk(nFlags, point);
}

// 右键按下
void CWatchDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;	// 右键
		event.nAction = 2;	// 按下
		(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnRButtonDown(nFlags, point);
}

// 右键抬起
void CWatchDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	if (m_nObjWidth != -1 && m_nObjHeight != -1)
	{
		CPoint remote = UserPoint2RemoteScreenPoint(point);
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;	// 右键
		event.nAction = 4;	// 抬起
		(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 5 << 1 | 1, (WPARAM)&event);
	}
	CDialog::OnRButtonUp(nFlags, point);
}

// 定时器 将缓冲区的图片映射到图片控件上
void CWatchDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0)
	{
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();
		if (pParent->isFull())
		{
			CRect rect;
			m_picture.GetWindowRect(rect);
			if (m_nObjWidth == -1) m_nObjWidth = pParent->GetImage().GetWidth();
			if (m_nObjHeight == -1) m_nObjHeight = pParent->GetImage().GetHeight();
			// 图像缩放绘制，将源屏幕大小的图片绘制在控件上
			pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->SetImageIsFull(false);
		}
	}

	CDialog::OnTimer(nIDEvent);
}

// 锁机
void CWatchDlg::OnBnClickedBtnLock()
{
	(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 7 << 1 | 1);
}

// 解锁
void CWatchDlg::OnBnClickedBtnUnlock()
{
	(CRemoteClientDlg*)GetParent()->SendMessage(WM_SEND_PACKET, 8 << 1 | 1);
}

// 将图片控件上的坐标转换为远程主机的坐标，图片控件大小是800 * 450
// isScreen代表传入的坐标是否是全局坐标
CPoint CWatchDlg::UserPoint2RemoteScreenPoint(CPoint& point, bool isScreen)
{
	CRect clientRect;
	if (isScreen) ScreenToClient(&point);	// 传入的是全局坐标，先转换成客户区坐标
	point.y -= 30;
	TRACE("x=%d y=%d\r\n", point.x, point.y);
	m_picture.GetWindowRect(clientRect);	// 得到远程桌面控件的大小
	TRACE("x=%d y=%d\r\n", clientRect.Width(), clientRect.Height());
	return CPoint(point.x * m_nObjWidth / clientRect.Width(), point.y * m_nObjHeight / clientRect.Height());
}

BOOL CWatchDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0, 45, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


