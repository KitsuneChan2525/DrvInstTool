#pragma once

class CMainFrame : public CMDIFrameWndEx
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame() noexcept;
	virtual ~CMainFrame();
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdateFrameMenu(HMENU hMenuAlt);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CMFCStatusBar m_statusBar;
	afx_msg int OnCreate(LPCREATESTRUCT createStruct);
	DECLARE_MESSAGE_MAP()
};
