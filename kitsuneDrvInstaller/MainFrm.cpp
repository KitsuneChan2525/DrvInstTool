#include "pch.h"
#include "framework.h"
#include "kitsuneDrvInstaller.h"
#include "MainFrm.h"
#include "Localization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWndEx)
	ON_WM_CREATE()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()

static UINT indicators[] = { ID_SEPARATOR };

CMainFrame::CMainFrame() noexcept {}
CMainFrame::~CMainFrame() = default;

int CMainFrame::OnCreate(LPCREATESTRUCT createStruct)
{
	if (CMDIFrameWndEx::OnCreate(createStruct) == -1) return -1;
	// Keep the resource menu owned by MFC, but detach it while the frame is
	// still hidden. This avoids both a first-frame flash and an invalid menu
	// handle during later MDI updates/shutdown.
	SetMenu(nullptr);
	CMDITabInfo tabs;
	EnableMDITabbedGroups(FALSE, tabs);
	EnableMDITabs(FALSE);
	if (!m_statusBar.Create(this)) return -1;
	m_statusBar.SetIndicators(indicators, _countof(indicators));
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	SetWindowTextW(Tr(TextId::AppTitle));
	return 0;
}

void CMainFrame::OnUpdateFrameMenu(HMENU /*hMenuAlt*/)
{
	// MDI 会在子窗口激活时重新合并文档菜单；在框架更新点持续清空它。
	if (m_hWndMDIClient != nullptr)
		::SendMessageW(m_hWndMDIClient, WM_MDISETMENU, 0, 0);
	SetMenu(nullptr);
	DrawMenuBar();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CMDIFrameWndEx::PreCreateWindow(cs)) return FALSE;
	// The installer supplies its complete title. Do not let the MDI framework
	// append the active child/document title a second time.
	cs.style &= ~FWS_ADDTOTITLE;
	cs.cx = 1180;
	cs.cy = 780;
	cs.style &= ~WS_MAXIMIZE;
	// Set the final coordinates before CreateWindowEx. The frame is therefore
	// born at the centered position and never paints at a saved/old position.
	POINT cursor = {};
	GetCursorPos(&cursor);
	const HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO info = { sizeof(info) };
	if (GetMonitorInfoW(monitor, &info))
	{
		cs.x = info.rcWork.left + (info.rcWork.right - info.rcWork.left - cs.cx) / 2;
		cs.y = info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - cs.cy) / 2;
	}
	return TRUE;
}

void CMainFrame::CenterBeforeFirstShow()
{
	POINT cursor = {};
	GetCursorPos(&cursor);
	const HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO info = { sizeof(info) };
	if (!GetMonitorInfoW(monitor, &info)) return;

	CRect windowRect;
	GetWindowRect(&windowRect);
	const int x = info.rcWork.left + (info.rcWork.right - info.rcWork.left - windowRect.Width()) / 2;
	const int y = info.rcWork.top + (info.rcWork.bottom - info.rcWork.top - windowRect.Height()) / 2;
	SetWindowPos(nullptr, x, y, 0, 0,
		SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW);
}

void CMainFrame::OnShowWindow(BOOL show, UINT status)
{
	// WM_SHOWWINDOW arrives before the first visible frame is composed. Apply
	// the final position here so even a late MFC placement restore stays hidden.
	if (show && !m_firstShowCentered)
	{
		CenterBeforeFirstShow();
		m_firstShowCentered = true;
	}
	CMDIFrameWndEx::OnShowWindow(show, status);
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const { CMDIFrameWndEx::AssertValid(); }
void CMainFrame::Dump(CDumpContext& dc) const { CMDIFrameWndEx::Dump(dc); }
#endif
