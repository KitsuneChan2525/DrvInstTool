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
END_MESSAGE_MAP()

static UINT indicators[] = { ID_SEPARATOR };

CMainFrame::CMainFrame() noexcept {}
CMainFrame::~CMainFrame() = default;

int CMainFrame::OnCreate(LPCREATESTRUCT createStruct)
{
	if (CMDIFrameWndEx::OnCreate(createStruct) == -1) return -1;
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
	cs.cx = 1180;
	cs.cy = 780;
	cs.style |= WS_MAXIMIZE;
	return TRUE;
}

#ifdef _DEBUG
void CMainFrame::AssertValid() const { CMDIFrameWndEx::AssertValid(); }
void CMainFrame::Dump(CDumpContext& dc) const { CMDIFrameWndEx::Dump(dc); }
#endif
