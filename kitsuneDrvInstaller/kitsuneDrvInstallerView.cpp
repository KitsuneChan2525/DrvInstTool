#include "pch.h"
#include "framework.h"
#include "kitsuneDrvInstaller.h"
#include "kitsuneDrvInstallerDoc.h"
#include "kitsuneDrvInstallerView.h"
#include "Localization.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	constexpr COLORREF ViewBackgroundColor = RGB(245, 247, 250);

	enum ControlId
	{
		IDC_SCAN = 2003,
		IDC_SELECT_RECOMMENDED = 2004,
		IDC_INSTALL = 2005,
		IDC_DEVICE_LIST = 2006,
		IDC_PROGRESS = 2007,
		IDC_LOG = 2008
		, IDC_LANGUAGE = 2009
	};

	bool FileExists(const std::wstring& path)
	{
		return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	std::wstring JoinPath(const std::wstring& left, const std::wstring& right)
	{
		return left.empty() || left.back() == L'\\' ? left + right : left + L"\\" + right;
	}

	std::wstring ParentDirectory(const std::wstring& path)
	{
		const size_t slash = path.find_last_of(L"\\/");
		return slash == std::wstring::npos ? std::wstring() : path.substr(0, slash);
	}

	std::wstring ProgramDataRoot()
	{
		wchar_t module[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, module, _countof(module));
		return JoinPath(ParentDirectory(module), L"Data");
	}

	void PumpMessages()
	{
		MSG message;
		while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
	}

	void ShowSystemCompatibilityError(HWND owner, const std::wstring& message)
	{
		MessageBoxW(owner, message.c_str(), Tr(TextId::UnsupportedSystemTitle), MB_OK | MB_ICONERROR);
	}
}

IMPLEMENT_DYNCREATE(CkitsuneDrvInstallerView, CView)

BEGIN_MESSAGE_MAP(CkitsuneDrvInstallerView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_BN_CLICKED(IDC_SCAN, &CkitsuneDrvInstallerView::OnScan)
	ON_BN_CLICKED(IDC_SELECT_RECOMMENDED, &CkitsuneDrvInstallerView::OnSelectRecommended)
	ON_BN_CLICKED(IDC_INSTALL, &CkitsuneDrvInstallerView::OnInstall)
	ON_CBN_SELCHANGE(IDC_LANGUAGE, &CkitsuneDrvInstallerView::OnLanguageChanged)
END_MESSAGE_MAP()

CkitsuneDrvInstallerView::CkitsuneDrvInstallerView() noexcept {}
CkitsuneDrvInstallerView::~CkitsuneDrvInstallerView() = default;

BOOL CkitsuneDrvInstallerView::PreCreateWindow(CREATESTRUCT& cs)
{
	cs.style |= WS_CLIPCHILDREN;
	return CView::PreCreateWindow(cs);
}

int CkitsuneDrvInstallerView::OnCreate(LPCREATESTRUCT createStruct)
{
	if (CView::OnCreate(createStruct) == -1) return -1;
	if (!m_backgroundBrush.CreateSolidBrush(ViewBackgroundColor)) return -1;

	m_titleFont.CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
	Localization::SetLanguage(Localization::DetectSystemLanguage());
	m_title.Create(L"", WS_CHILD | WS_VISIBLE, CRect(), this);
	m_title.SetFont(&m_titleFont);
	m_languageLabel.Create(L"", WS_CHILD | WS_VISIBLE | SS_RIGHT, CRect(), this);
	if (!m_language.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST |
		CBS_HASSTRINGS | CBS_NOINTEGRALHEIGHT | WS_VSCROLL, CRect(0, 0, 180, 240), this, IDC_LANGUAGE))
		return -1;
	m_language.SetFont(CFont::FromHandle(static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT))), FALSE);
	if (!InitializeLanguageSelector()) return -1;
	m_scan.Create(L"", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, CRect(), this, IDC_SCAN);
	m_selectRecommended.Create(L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_SELECT_RECOMMENDED);
	m_install.Create(L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_INSTALL);
	m_devices.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS, CRect(), this, IDC_DEVICE_LIST);
	m_devices.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
	m_devices.InsertColumn(0, L"", LVCFMT_LEFT, 335);
	m_devices.InsertColumn(1, L"", LVCFMT_LEFT, 210);
	m_devices.InsertColumn(2, L"", LVCFMT_LEFT, 165);
	m_devices.InsertColumn(3, L"", LVCFMT_LEFT, 155);
	m_devices.InsertColumn(4, L"", LVCFMT_LEFT, 135);
	m_progress.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH, CRect(), this, IDC_PROGRESS);
	m_logLabel.Create(L"", WS_CHILD | WS_VISIBLE, CRect(), this);
	m_log.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
		CRect(), this, IDC_LOG);
	ApplyLanguage();
	CRect clientRect;
	GetClientRect(&clientRect);
	LayoutControls(clientRect.Width(), clientRect.Height());
	return 0;
}

void CkitsuneDrvInstallerView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	if (m_language.GetCount() != 3 || m_language.GetCurSel() == CB_ERR)
		InitializeLanguageSelector();
	UpdateTitle();
}

void CkitsuneDrvInstallerView::OnDraw(CDC*) {}

BOOL CkitsuneDrvInstallerView::OnEraseBkgnd(CDC* dc)
{
	CRect rect;
	GetClientRect(&rect);
	dc->FillSolidRect(rect, ViewBackgroundColor);
	return TRUE;
}

HBRUSH CkitsuneDrvInstallerView::OnCtlColor(CDC* dc, CWnd* window, UINT controlColor)
{
	HBRUSH brush = CView::OnCtlColor(dc, window, controlColor);
	if (controlColor == CTLCOLOR_STATIC)
	{
		dc->SetBkMode(TRANSPARENT);
		return static_cast<HBRUSH>(m_backgroundBrush.GetSafeHandle());
	}
	return brush;
}

void CkitsuneDrvInstallerView::OnSize(UINT type, int cx, int cy)
{
	CView::OnSize(type, cx, cy);
	if (m_title.GetSafeHwnd()) LayoutControls(cx, cy);
}

void CkitsuneDrvInstallerView::LayoutControls(int width, int height)
{
	const int margin = 22;
	const int gap = 8;
	const int contentWidth = max(300, width - margin * 2);

	// Header: keep the language selector at the upper-right and reserve the
	// remaining width for the package title so the controls never overlap.
	const int languageWidth = 172;
	const int languageLabelWidth = 74;
	const int languageX = width - margin - languageWidth;
	const int languageLabelX = languageX - gap - languageLabelWidth;
	m_title.MoveWindow(margin, 16, max(120, languageLabelX - margin - 14), 36);
	m_languageLabel.MoveWindow(languageLabelX, 21, languageLabelWidth, 24);
	m_language.SetWindowPos(nullptr, languageX, 17, languageWidth, 240,
		SWP_NOZORDER | SWP_SHOWWINDOW);

	// All three actions share the row below the title.
	const int scanWidth = 112;
	const int selectWidth = 140;
	const int installWidth = 140;
	const int installX = width - margin - installWidth;
	const int selectX = installX - gap - selectWidth;
	const int scanX = selectX - gap - scanWidth;
	m_scan.MoveWindow(scanX, 62, scanWidth, 32);
	m_selectRecommended.MoveWindow(selectX, 62, selectWidth, 32);
	m_install.MoveWindow(installX, 62, installWidth, 32);

	// The device list receives all flexible vertical space. Progress and log
	// areas stay anchored to the bottom for a stable layout at every size.
	const int devicesTop = 105;
	const int logHeight = max(96, min(150, height / 5));
	const int logTop = height - margin - logHeight;
	const int logLabelY = logTop - 25;
	const int progressY = logLabelY - 23;
	const int devicesBottom = progressY - gap;
	const int listHeight = max(100, devicesBottom - devicesTop);
	m_devices.MoveWindow(margin, devicesTop, contentWidth, listHeight);
	m_progress.MoveWindow(margin, devicesTop + listHeight + gap, contentWidth, 17);
	m_logLabel.MoveWindow(margin, logLabelY, 160, 22);
	m_log.MoveWindow(margin, logTop, contentWidth, max(50, height - logTop - margin));
}

bool CkitsuneDrvInstallerView::InitializeLanguageSelector()
{
	if (!m_language.GetSafeHwnd()) return false;
	m_language.SetRedraw(FALSE);
	m_language.ResetContent();
	const wchar_t* const languages[] = { L"English", L"简体中文", L"繁體中文" };
	for (const wchar_t* language : languages)
	{
		if (m_language.AddString(language) != CB_ERR) continue;
		m_language.SetRedraw(TRUE);
		return false;
	}
	int selection = static_cast<int>(Localization::GetLanguage());
	if (selection < 0 || selection >= _countof(languages)) selection = 0;
	if (m_language.SetCurSel(selection) == CB_ERR) m_language.SetCurSel(0);
	m_language.SetRedraw(TRUE);
	m_language.ShowWindow(SW_SHOW);
	m_language.Invalidate(FALSE);
	return m_language.GetCount() == _countof(languages) && m_language.GetCurSel() != CB_ERR;
}

std::wstring CkitsuneDrvInstallerView::CurrentDataRoot() const
{
	return ProgramDataRoot();
}

void CkitsuneDrvInstallerView::AppendLog(const std::wstring& text)
{
	CString existing;
	m_log.GetWindowTextW(existing);
	SYSTEMTIME now;
	GetLocalTime(&now);
	CString line;
	line.Format(L"[%02u:%02u:%02u] %s\r\n", now.wHour, now.wMinute, now.wSecond, text.c_str());
	existing += line;
	m_log.SetWindowTextW(existing);
	m_log.SetSel(-1, -1);
	m_log.SendMessageW(EM_SCROLLCARET, 0, 0);
	PumpMessages();
}

void CkitsuneDrvInstallerView::SetBusy(bool busy)
{
	m_busy = busy;
	m_scan.EnableWindow(!busy);
	m_selectRecommended.EnableWindow(!busy && !m_matches.empty());
	m_install.EnableWindow(!busy && !m_matches.empty());
	UpdateWindow();
}

void CkitsuneDrvInstallerView::OnScan()
{
	if (m_busy) return;
	const std::wstring root = CurrentDataRoot();
	if (!FileExists(JoinPath(root, L"config.json")))
	{
		AfxMessageBox(Tr(TextId::DataConfigMissing), MB_ICONWARNING);
		return;
	}
	std::wstring error;
	if (!SystemCompatibility::ValidateDriverMedia(root, error))
	{
		AppendLog(Tr(TextId::UnsupportedSystemLog));
		ShowSystemCompatibilityError(GetSafeHwnd(), error);
		return;
	}
	SetBusy(true);
	m_progress.SetRange32(0, 100);
	m_progress.SetPos(12);
	AppendLog(std::wstring(Tr(TextId::LoadingIndex)) + L" " + root);
	if (!m_catalog.Load(root, error))
	{
		AppendLog(std::wstring(Tr(TextId::InstallFailureLog)) + error);
		SetBusy(false);
		AfxMessageBox(error.c_str(), MB_ICONERROR);
		return;
	}
	m_progress.SetPos(58);
	int scanned = 0;
	if (!DeviceScanner::Scan(m_catalog, m_matches, scanned, error))
	{
		AppendLog(std::wstring(Tr(TextId::InstallFailureLog)) + error);
		SetBusy(false);
		AfxMessageBox(error.c_str(), MB_ICONERROR);
		return;
	}
	m_progress.SetPos(100);
	RefreshDeviceList();
	CString summary;
	summary.Format(Tr(TextId::ScanSummaryFormat),
		scanned, static_cast<unsigned>(m_matches.size()), static_cast<unsigned>(m_catalog.DriverCount()), static_cast<unsigned>(m_catalog.HardwareIdCount()));
	AppendLog(summary.GetString());
	SetBusy(false);
}

void CkitsuneDrvInstallerView::RefreshDeviceList()
{
	m_devices.DeleteAllItems();
	for (size_t i = 0; i < m_matches.size(); ++i)
	{
		const DeviceMatch& match = m_matches[i];
		const int row = m_devices.InsertItem(static_cast<int>(i), match.displayName.c_str());
		m_devices.SetItemText(row, 1, match.driver.provider.c_str());
		m_devices.SetItemText(row, 2, match.driver.driverVersion.c_str());
		m_devices.SetItemText(row, 3, match.driver.driverDate.c_str());
		const TextId status = match.needsDriver ? TextId::MissingDriver :
			(match.updateAvailable ? TextId::InstalledUpdate : TextId::InstalledSuccess);
		m_devices.SetItemText(row, 4, Tr(status));
		m_devices.SetCheck(row, (match.needsDriver || match.updateAvailable) ? TRUE : FALSE);
	}
}

void CkitsuneDrvInstallerView::OnSelectRecommended()
{
	for (int row = 0; row < m_devices.GetItemCount(); ++row)
	{
		const DeviceMatch& match = m_matches[static_cast<size_t>(row)];
		m_devices.SetCheck(row, (match.needsDriver || match.updateAvailable) ? TRUE : FALSE);
	}
}

void CkitsuneDrvInstallerView::OnInstall()
{
	if (m_busy) return;
	std::vector<int> selected;
	for (int row = 0; row < m_devices.GetItemCount(); ++row)
		if (m_devices.GetCheck(row)) selected.push_back(row);
	if (selected.empty())
	{
		AfxMessageBox(Tr(TextId::SelectDriverFirst), MB_ICONINFORMATION);
		return;
	}
	const std::wstring root = CurrentDataRoot();
	std::wstring compatibilityError;
	if (!SystemCompatibility::ValidateDriverMedia(root, compatibilityError))
	{
		AppendLog(Tr(TextId::UnsupportedSystemLog));
		ShowSystemCompatibilityError(GetSafeHwnd(), compatibilityError);
		return;
	}
	const std::wstring sevenZip = DriverInstaller::Find7Zip(root);
	if (sevenZip.empty())
	{
		AfxMessageBox(Tr(TextId::SevenZipMediaMissing), MB_ICONERROR);
		return;
	}
	AppendLog(std::wstring(Tr(TextId::UsingExtractor)) + sevenZip);
	SetBusy(true);
	m_progress.SetRange32(0, static_cast<int>(selected.size()));
	m_progress.SetPos(0);
	int success = 0;
	bool anyReboot = false;
	for (size_t position = 0; position < selected.size(); ++position)
	{
		const int row = selected[position];
		DeviceMatch& match = m_matches[static_cast<size_t>(row)];
		AppendLog(std::wstring(Tr(TextId::StartInstall)) + match.displayName + L" [" + match.driver.id + L"]");
		bool reboot = false;
		std::wstring error;
		if (DriverInstaller::Install(root, match, reboot, error, [this](const std::wstring& line) { AppendLog(line); }))
		{
			++success;
			anyReboot = anyReboot || reboot;
			m_devices.SetItemText(row, 4, reboot ? Tr(TextId::InstalledReboot) : Tr(TextId::InstalledSuccess));
			m_devices.SetCheck(row, FALSE);
			AppendLog(std::wstring(Tr(TextId::InstallSuccessLog)) + match.displayName);
		}
		else
		{
			m_devices.SetItemText(row, 4, Tr(TextId::InstallFailedStatus));
			AppendLog(std::wstring(Tr(TextId::InstallFailureLog)) + error);
		}
		m_progress.SetPos(static_cast<int>(position + 1));
		PumpMessages();
	}
	CString summary;
	summary.Format(Tr(TextId::InstallSummaryFormat), success, static_cast<unsigned>(selected.size()), anyReboot ? Tr(TextId::RebootSuffix) : L"");
	AppendLog(summary.GetString());
	SetBusy(false);
	AfxMessageBox(summary, success == static_cast<int>(selected.size()) ? MB_ICONINFORMATION : MB_ICONWARNING);
}

void CkitsuneDrvInstallerView::ApplyLanguage()
{
	UpdateTitle();
	m_languageLabel.SetWindowTextW(Tr(TextId::Language));
	m_scan.SetWindowTextW(Tr(TextId::ScanHardware));
	m_selectRecommended.SetWindowTextW(Tr(TextId::SelectMissing));
	m_install.SetWindowTextW(Tr(TextId::InstallSelected));
	m_logLabel.SetWindowTextW(Tr(TextId::OperationLog));
	const TextId columns[] = { TextId::ColumnDevice, TextId::ColumnProvider,
		TextId::ColumnVersion, TextId::ColumnDate, TextId::ColumnStatus };
	for (int index = 0; index < _countof(columns); ++index)
	{
		LVCOLUMNW column = {};
		column.mask = LVCF_TEXT;
		column.pszText = const_cast<LPWSTR>(Tr(columns[index]));
		m_devices.SetColumn(index, &column);
	}
	if (!m_matches.empty()) RefreshDeviceList();
	Invalidate();
}

void CkitsuneDrvInstallerView::UpdateTitle()
{
	const std::wstring title = L"kitsune Driver Installer";
	m_title.SetWindowTextW(title.c_str());
	if (AfxGetMainWnd()) AfxGetMainWnd()->SetWindowTextW(title.c_str());
}

void CkitsuneDrvInstallerView::OnLanguageChanged()
{
	const int selection = m_language.GetCurSel();
	if (selection < 0 || selection > 2) return;
	Localization::SetLanguage(static_cast<UiLanguage>(selection));
	ApplyLanguage();
}

#ifdef _DEBUG
void CkitsuneDrvInstallerView::AssertValid() const { CView::AssertValid(); }
void CkitsuneDrvInstallerView::Dump(CDumpContext& dc) const { CView::Dump(dc); }
CkitsuneDrvInstallerDoc* CkitsuneDrvInstallerView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CkitsuneDrvInstallerDoc)));
	return static_cast<CkitsuneDrvInstallerDoc*>(m_pDocument);
}
#endif
