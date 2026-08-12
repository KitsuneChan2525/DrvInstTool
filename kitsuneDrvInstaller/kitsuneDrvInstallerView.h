#pragma once

#include "DriverEngine.h"

class CkitsuneDrvInstallerView : public CView
{
protected:
	CkitsuneDrvInstallerView() noexcept;
	DECLARE_DYNCREATE(CkitsuneDrvInstallerView)

public:
	CkitsuneDrvInstallerDoc* GetDocument() const;
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	virtual ~CkitsuneDrvInstallerView();

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CStatic m_title;
	CStatic m_languageLabel;
	CComboBox m_language;
	CStatic m_logLabel;
	CEdit m_log;
	CButton m_scan;
	CButton m_selectRecommended;
	CButton m_install;
	CListCtrl m_devices;
	CProgressCtrl m_progress;
	CFont m_uiFont;
	CFont m_titleFont;
	CBrush m_backgroundBrush;
	DriverCatalog m_catalog;
	std::vector<DeviceMatch> m_matches;
	bool m_busy = false;

	void LayoutControls(int width, int height);
	void AppendLog(const std::wstring& text);
	void SetBusy(bool busy);
	bool HasCheckedDrivers();
	void UpdateInstallButtonState();
	void RefreshDeviceList();
	void UpdateTitle();
	void ApplyLanguage();
	bool InitializeLanguageSelector();
	std::wstring CurrentDataRoot() const;

	afx_msg int OnCreate(LPCREATESTRUCT createStruct);
	afx_msg void OnSize(UINT type, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* dc);
	afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* window, UINT controlColor);
	afx_msg void OnScan();
	afx_msg void OnSelectRecommended();
	afx_msg void OnInstall();
	afx_msg void OnLanguageChanged();
	afx_msg void OnDeviceItemChanged(NMHDR* notifyHeader, LRESULT* result);
	afx_msg LRESULT OnAutoScan(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CkitsuneDrvInstallerDoc* CkitsuneDrvInstallerView::GetDocument() const
{
	return reinterpret_cast<CkitsuneDrvInstallerDoc*>(m_pDocument);
}
#endif
