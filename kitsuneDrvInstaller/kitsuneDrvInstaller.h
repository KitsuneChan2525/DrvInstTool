
// kitsuneDrvInstaller.h: kitsuneDrvInstaller 应用程序的主头文件
//
#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含 'pch.h' 以生成 PCH"
#endif

#include "resource.h"       // 主符号


// CkitsuneDrvInstallerApp:
// 有关此类的实现，请参阅 kitsuneDrvInstaller.cpp
//

class CkitsuneDrvInstallerApp : public CWinAppEx
{
public:
	CkitsuneDrvInstallerApp() noexcept;


// 重写
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	virtual BOOL PreTranslateMessage(MSG* message);
	bool IsAiMode() const { return m_aiMode; }
	void SetInstallerView(CWnd* view) { m_installerView = view; }
	void NotifyAiUserActivity();

// 实现
	BOOL  m_bHiColorIcons;
	bool m_aiMode = false;
	CWnd* m_installerView = nullptr;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern CkitsuneDrvInstallerApp theApp;
