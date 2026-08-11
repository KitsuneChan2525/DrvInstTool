#include "pch.h"
#include "framework.h"
#include "kitsuneDrvInstaller.h"
#include "kitsuneDrvInstallerDoc.h"
#include "kitsuneDrvInstallerView.h"
#include "Localization.h"

#include <ShlObj.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	enum ControlId
	{
		IDC_DATA_ROOT = 2001,
		IDC_BROWSE = 2002,
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

	std::wstring DetectDataRoot()
	{
		wchar_t module[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, module, _countof(module));
		const std::wstring exeDirectory = ParentDirectory(module);
		const std::wstring candidates[] =
		{
			JoinPath(exeDirectory, L"Data"),
			JoinPath(ParentDirectory(exeDirectory), L"Data"),
			L"D:\\Project\\DrvInst\\Win7\\x64\\Data"
		};
		for (const auto& candidate : candidates)
			if (FileExists(JoinPath(candidate, L"config.json"))) return candidate;
		return candidates[0];
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
}

IMPLEMENT_DYNCREATE(CkitsuneDrvInstallerView, CView)

BEGIN_MESSAGE_MAP(CkitsuneDrvInstallerView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_BROWSE, &CkitsuneDrvInstallerView::OnBrowse)
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

	m_titleFont.CreateFontW(-28, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");
	m_subtitleFont.CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei UI");

	Localization::SetLanguage(Localization::DetectSystemLanguage());
	m_title.Create(L"", WS_CHILD | WS_VISIBLE, CRect(), this);
	m_title.SetFont(&m_titleFont);
	m_subtitle.Create(L"", WS_CHILD | WS_VISIBLE, CRect(), this);
	m_subtitle.SetFont(&m_subtitleFont);
	m_languageLabel.Create(L"", WS_CHILD | WS_VISIBLE | SS_RIGHT, CRect(), this);
	m_language.Create(WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, CRect(), this, IDC_LANGUAGE);
	m_language.AddString(L"English");
	m_language.AddString(L"简体中文");
	m_language.AddString(L"繁體中文");
	m_language.SetCurSel(static_cast<int>(Localization::GetLanguage()));
	m_dataLabel.Create(L"", WS_CHILD | WS_VISIBLE, CRect(), this);
	m_dataRoot.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect(), this, IDC_DATA_ROOT);
	m_browse.Create(L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_BROWSE);
	m_scan.Create(L"", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, CRect(), this, IDC_SCAN);
	m_listLabel.Create(L"", WS_CHILD | WS_VISIBLE, CRect(), this);
	m_selectRecommended.Create(L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_SELECT_RECOMMENDED);
	m_install.Create(L"", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, CRect(), this, IDC_INSTALL);
	m_statusLabel.Create(L"", WS_CHILD | WS_VISIBLE | SS_RIGHT, CRect(), this);
	m_devices.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS, CRect(), this, IDC_DEVICE_LIST);
	m_devices.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES);
	m_devices.InsertColumn(0, L"", LVCFMT_LEFT, 265);
	m_devices.InsertColumn(1, L"", LVCFMT_LEFT, 80);
	m_devices.InsertColumn(2, L"", LVCFMT_LEFT, 175);
	m_devices.InsertColumn(3, L"", LVCFMT_LEFT, 155);
	m_devices.InsertColumn(4, L"", LVCFMT_LEFT, 150);
	m_devices.InsertColumn(5, L"", LVCFMT_LEFT, 125);
	m_progress.Create(WS_CHILD | WS_VISIBLE | PBS_SMOOTH, CRect(), this, IDC_PROGRESS);
	m_logLabel.Create(L"", WS_CHILD | WS_VISIBLE, CRect(), this);
	m_log.Create(WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL,
		CRect(), this, IDC_LOG);
	ApplyLanguage();
	return 0;
}

void CkitsuneDrvInstallerView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
	m_dataRoot.SetWindowTextW(DetectDataRoot().c_str());
	if (GetParentFrame()) GetParentFrame()->SetWindowTextW(Tr(TextId::ViewTitle));
	if (AfxGetMainWnd()) AfxGetMainWnd()->SetWindowTextW(Tr(TextId::AppTitle));
	AppendLog(Tr(TextId::InitialLog));
}

void CkitsuneDrvInstallerView::OnDraw(CDC*) {}

BOOL CkitsuneDrvInstallerView::OnEraseBkgnd(CDC* dc)
{
	CRect rect;
	GetClientRect(&rect);
	dc->FillSolidRect(rect, RGB(245, 247, 250));
	return TRUE;
}

void CkitsuneDrvInstallerView::OnSize(UINT type, int cx, int cy)
{
	CView::OnSize(type, cx, cy);
	if (m_title.GetSafeHwnd()) LayoutControls(cx, cy);
}

void CkitsuneDrvInstallerView::LayoutControls(int width, int height)
{
	const int margin = 22;
	const int contentWidth = max(300, width - margin * 2);
	m_title.MoveWindow(margin, 16, contentWidth, 34);
	m_subtitle.MoveWindow(margin, 49, contentWidth, 22);
	m_languageLabel.MoveWindow(width - margin - 270, 20, 90, 24);
	m_language.MoveWindow(width - margin - 172, 17, 172, 220);
	m_dataLabel.MoveWindow(margin, 82, 95, 24);
	const int buttonWidth = 95;
	m_scan.MoveWindow(width - margin - 105, 78, 105, 30);
	m_browse.MoveWindow(width - margin - 105 - buttonWidth - 8, 78, buttonWidth, 30);
	m_dataRoot.MoveWindow(margin + 100, 79, max(80, width - margin * 2 - 100 - 105 - buttonWidth - 16), 28);

	m_listLabel.MoveWindow(margin, 122, 190, 24);
	m_install.MoveWindow(width - margin - 125, 116, 125, 31);
	m_selectRecommended.MoveWindow(width - margin - 125 - 132, 116, 124, 31);
	m_statusLabel.MoveWindow(margin + 200, 122, max(50, width - margin * 2 - 470), 23);

	const int logHeight = 98;
	const int bottomArea = 32 + 22 + logHeight + margin;
	const int listHeight = max(100, height - 152 - bottomArea);
	m_devices.MoveWindow(margin, 151, contentWidth, listHeight);
	m_progress.MoveWindow(margin, 158 + listHeight, contentWidth, 17);
	m_logLabel.MoveWindow(margin, 181 + listHeight, 100, 22);
	m_log.MoveWindow(margin, 203 + listHeight, contentWidth, max(50, height - (203 + listHeight) - margin));
}

std::wstring CkitsuneDrvInstallerView::CurrentDataRoot() const
{
	CString text;
	const_cast<CEdit&>(m_dataRoot).GetWindowTextW(text);
	std::wstring result = text.GetString();
	while (!result.empty() && (result.back() == L'\\' || result.back() == L'/')) result.pop_back();
	return result;
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

void CkitsuneDrvInstallerView::SetBusy(bool busy, const wchar_t* status)
{
	m_busy = busy;
	m_browse.EnableWindow(!busy);
	m_scan.EnableWindow(!busy);
	m_selectRecommended.EnableWindow(!busy && !m_matches.empty());
	m_install.EnableWindow(!busy && !m_matches.empty());
	m_statusLabel.SetWindowTextW(status);
	UpdateWindow();
}

void CkitsuneDrvInstallerView::OnBrowse()
{
	BROWSEINFOW info = {};
	info.hwndOwner = GetSafeHwnd();
	info.lpszTitle = Tr(TextId::BrowseTitle);
	info.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	PIDLIST_ABSOLUTE item = SHBrowseForFolderW(&info);
	if (!item) return;
	wchar_t path[MAX_PATH] = {};
	if (SHGetPathFromIDListW(item, path)) m_dataRoot.SetWindowTextW(path);
	CoTaskMemFree(item);
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
	SetBusy(true, Tr(TextId::LoadingIndex));
	m_progress.SetRange32(0, 100);
	m_progress.SetPos(12);
	AppendLog(std::wstring(Tr(TextId::LoadingIndex)) + L" " + root);
	std::wstring error;
	if (!m_catalog.Load(root, error))
	{
		AppendLog(std::wstring(Tr(TextId::InstallFailureLog)) + error);
		SetBusy(false, Tr(TextId::IndexLoadFailed));
		AfxMessageBox(error.c_str(), MB_ICONERROR);
		return;
	}
	m_progress.SetPos(58);
	m_statusLabel.SetWindowTextW(Tr(TextId::EnumeratingDevices));
	int scanned = 0;
	if (!DeviceScanner::Scan(m_catalog, m_matches, scanned, error))
	{
		AppendLog(std::wstring(Tr(TextId::InstallFailureLog)) + error);
		SetBusy(false, Tr(TextId::DeviceScanFailed));
		AfxMessageBox(error.c_str(), MB_ICONERROR);
		return;
	}
	m_progress.SetPos(100);
	RefreshDeviceList();
	CString summary;
	summary.Format(Tr(TextId::ScanSummaryFormat),
		scanned, static_cast<unsigned>(m_matches.size()), static_cast<unsigned>(m_catalog.DriverCount()), static_cast<unsigned>(m_catalog.HardwareIdCount()));
	AppendLog(summary.GetString());
	SetBusy(false, summary);
}

void CkitsuneDrvInstallerView::RefreshDeviceList()
{
	m_devices.DeleteAllItems();
	for (size_t i = 0; i < m_matches.size(); ++i)
	{
		const DeviceMatch& match = m_matches[i];
		const int row = m_devices.InsertItem(static_cast<int>(i), match.displayName.c_str());
		m_devices.SetItemText(row, 1, match.driver.category.c_str());
		m_devices.SetItemText(row, 2, match.driver.provider.c_str());
		const std::wstring version = match.driver.driverVersion + L" / " + match.driver.driverDate;
		m_devices.SetItemText(row, 3, version.c_str());
		m_devices.SetItemText(row, 4, match.driver.archiveFile.c_str());
		m_devices.SetItemText(row, 5, match.needsDriver ? Tr(TextId::MissingDriver) : Tr(TextId::InstalledUpdate));
		m_devices.SetCheck(row, match.needsDriver ? TRUE : FALSE);
	}
}

void CkitsuneDrvInstallerView::OnSelectRecommended()
{
	for (int row = 0; row < m_devices.GetItemCount(); ++row)
		m_devices.SetCheck(row, m_matches[static_cast<size_t>(row)].needsDriver ? TRUE : FALSE);
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
	const std::wstring sevenZip = DriverInstaller::Find7Zip(root);
	if (sevenZip.empty())
	{
		AfxMessageBox(Tr(TextId::SevenZipMediaMissing), MB_ICONERROR);
		return;
	}
	AppendLog(std::wstring(Tr(TextId::UsingExtractor)) + sevenZip);
	SetBusy(true, Tr(TextId::InstallingDrivers));
	m_progress.SetRange32(0, static_cast<int>(selected.size()));
	m_progress.SetPos(0);
	int success = 0;
	bool anyReboot = false;
	for (size_t position = 0; position < selected.size(); ++position)
	{
		const int row = selected[position];
		DeviceMatch& match = m_matches[static_cast<size_t>(row)];
		m_statusLabel.SetWindowTextW((std::wstring(Tr(TextId::StartInstall)) + match.displayName).c_str());
		AppendLog(std::wstring(Tr(TextId::StartInstall)) + match.displayName + L" [" + match.driver.id + L"]");
		bool reboot = false;
		std::wstring error;
		if (DriverInstaller::Install(root, match, reboot, error, [this](const std::wstring& line) { AppendLog(line); }))
		{
			++success;
			anyReboot = anyReboot || reboot;
			m_devices.SetItemText(row, 5, reboot ? Tr(TextId::InstalledReboot) : Tr(TextId::InstalledSuccess));
			m_devices.SetCheck(row, FALSE);
			AppendLog(std::wstring(Tr(TextId::InstallSuccessLog)) + match.displayName);
		}
		else
		{
			m_devices.SetItemText(row, 5, Tr(TextId::InstallFailedStatus));
			AppendLog(std::wstring(Tr(TextId::InstallFailureLog)) + error);
		}
		m_progress.SetPos(static_cast<int>(position + 1));
		PumpMessages();
	}
	CString summary;
	summary.Format(Tr(TextId::InstallSummaryFormat), success, static_cast<unsigned>(selected.size()), anyReboot ? Tr(TextId::RebootSuffix) : L"");
	AppendLog(summary.GetString());
	SetBusy(false, summary);
	AfxMessageBox(summary, success == static_cast<int>(selected.size()) ? MB_ICONINFORMATION : MB_ICONWARNING);
}

void CkitsuneDrvInstallerView::ApplyLanguage()
{
	m_title.SetWindowTextW(Tr(TextId::AppTitle));
	m_subtitle.SetWindowTextW(Tr(TextId::Subtitle));
	m_languageLabel.SetWindowTextW(Tr(TextId::Language));
	m_dataLabel.SetWindowTextW(Tr(TextId::DataDirectory));
	m_browse.SetWindowTextW(Tr(TextId::Browse));
	m_scan.SetWindowTextW(Tr(TextId::ScanHardware));
	m_listLabel.SetWindowTextW(Tr(TextId::MatchedDevices));
	m_selectRecommended.SetWindowTextW(Tr(TextId::SelectMissing));
	m_install.SetWindowTextW(Tr(TextId::InstallSelected));
	m_logLabel.SetWindowTextW(Tr(TextId::OperationLog));
	if (!m_busy) m_statusLabel.SetWindowTextW(Tr(TextId::Ready));
	const TextId columns[] = { TextId::ColumnDevice, TextId::ColumnCategory, TextId::ColumnProvider,
		TextId::ColumnVersionDate, TextId::ColumnPackage, TextId::ColumnStatus };
	for (int index = 0; index < _countof(columns); ++index)
	{
		LVCOLUMNW column = {};
		column.mask = LVCF_TEXT;
		column.pszText = const_cast<LPWSTR>(Tr(columns[index]));
		m_devices.SetColumn(index, &column);
	}
	if (GetParentFrame()) GetParentFrame()->SetWindowTextW(Tr(TextId::ViewTitle));
	if (AfxGetMainWnd()) AfxGetMainWnd()->SetWindowTextW(Tr(TextId::AppTitle));
	if (!m_matches.empty()) RefreshDeviceList();
	Invalidate();
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
