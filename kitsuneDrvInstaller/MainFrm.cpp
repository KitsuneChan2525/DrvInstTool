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
	ON_WM_SYSCOMMAND()
	ON_WM_CLOSE()
	ON_WM_QUERYENDSESSION()
	ON_COMMAND(ID_APP_EXIT, &CMainFrame::OnAppExit)
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()

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
	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
	SetWindowTextW(Tr(TextId::AppTitle));
	if (CMenu* systemMenu = GetSystemMenu(FALSE))
	{
		systemMenu->DeleteMenu(SC_SIZE, MF_BYCOMMAND);
		systemMenu->DeleteMenu(SC_MINIMIZE, MF_BYCOMMAND);
		systemMenu->DeleteMenu(SC_MAXIMIZE, MF_BYCOMMAND);
	}
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
	cs.style &= ~(FWS_ADDTOTITLE | WS_THICKFRAME | WS_MINIMIZEBOX |
		WS_MAXIMIZEBOX | WS_MINIMIZE | WS_MAXIMIZE);
	cs.cx = 1180;
	cs.cy = 780;
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
		CRect rect;
		GetWindowRect(&rect);
		m_fixedWindowSize = rect.Size();
		m_fixedSizeActive = true;
		m_firstShowCentered = true;
	}
	CMDIFrameWndEx::OnShowWindow(show, status);
}

void CMainFrame::OnSysCommand(UINT id, LPARAM parameter)
{
	const UINT command = id & 0xFFF0;
	if (command == SC_SIZE || command == SC_MINIMIZE || command == SC_MAXIMIZE ||
		(m_installationActive && command == SC_CLOSE)) return;
	CMDIFrameWndEx::OnSysCommand(id, parameter);
}

void CMainFrame::SetInstallationActive(bool active)
{
	m_installationActive = active;
	if (CMenu* systemMenu = GetSystemMenu(FALSE))
		systemMenu->EnableMenuItem(SC_CLOSE, MF_BYCOMMAND |
			(active ? MF_GRAYED : MF_ENABLED));
	DrawMenuBar();
}

void CMainFrame::OnClose()
{
	if (m_installationActive) return;
	CMDIFrameWndEx::OnClose();
}

BOOL CMainFrame::OnQueryEndSession()
{
	if (m_installationActive) return FALSE;
	return CMDIFrameWndEx::OnQueryEndSession();
}

void CMainFrame::OnAppExit()
{
	if (m_installationActive) return;
	SendMessageW(WM_CLOSE);
}

void CMainFrame::OnWindowPosChanging(WINDOWPOS* position)
{
	if (m_fixedSizeActive && (position->flags & SWP_NOSIZE) == 0)
	{
		position->cx = m_fixedWindowSize.cx;
		position->cy = m_fixedWindowSize.cy;
	}
	CMDIFrameWndEx::OnWindowPosChanging(position);
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const { CMDIFrameWndEx::AssertValid(); }
void CMainFrame::Dump(CDumpContext& dc) const { CMDIFrameWndEx::Dump(dc); }
#endif
