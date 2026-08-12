#pragma once

class CMainFrame : public CMDIFrameWndEx
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame() noexcept;
	virtual ~CMainFrame();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdateFrameMenu(HMENU hMenuAlt);
	void CenterBeforeFirstShow();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CMFCStatusBar m_statusBar;
	bool m_firstShowCentered = false;
	bool m_fixedSizeActive = false;
	CSize m_fixedWindowSize;
	afx_msg int OnCreate(LPCREATESTRUCT createStruct);
	afx_msg void OnShowWindow(BOOL show, UINT status);
	afx_msg void OnSysCommand(UINT id, LPARAM parameter);
	afx_msg void OnWindowPosChanging(WINDOWPOS* position);
	DECLARE_MESSAGE_MAP()
};
