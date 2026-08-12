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
	afx_msg int OnCreate(LPCREATESTRUCT createStruct);
	afx_msg void OnShowWindow(BOOL show, UINT status);
	DECLARE_MESSAGE_MAP()
};
