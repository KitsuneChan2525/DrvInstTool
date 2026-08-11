
// ChildFrm.cpp: CChildFrame 类的实现
//

#include "pch.h"
#include "framework.h"
#include "kitsuneDrvInstaller.h"

#include "ChildFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWndEx)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWndEx)
END_MESSAGE_MAP()

// CChildFrame 构造/析构

CChildFrame::CChildFrame() noexcept
{
	// TODO: 在此添加成员初始化代码
}

CChildFrame::~CChildFrame()
{
}


BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIChildWndEx::PreCreateWindow(cs) )
		return FALSE;

	// 安装器只有一个工作页：去掉 MDI 子窗口标题、边框和系统按钮。
	cs.style &= ~(WS_CAPTION | WS_BORDER | WS_THICKFRAME | WS_SYSMENU |
		WS_MINIMIZEBOX | WS_MAXIMIZEBOX | FWS_ADDTOTITLE);
	cs.style |= WS_CHILD | WS_MAXIMIZE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	cs.dwExStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE | WS_EX_DLGMODALFRAME);

	return TRUE;
}

// CChildFrame 诊断

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWndEx::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWndEx::Dump(dc);
}
#endif //_DEBUG

// CChildFrame 消息处理程序
