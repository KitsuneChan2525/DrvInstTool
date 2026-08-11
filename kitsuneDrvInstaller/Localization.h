#pragma once

enum class UiLanguage
{
	English = 0,
	ChineseSimplified = 1,
	ChineseTraditional = 2
};

enum class TextId
{
	AppTitle,
	ViewTitle,
	Subtitle,
	Language,
	English,
	ChineseSimplified,
	ChineseTraditional,
	DataDirectory,
	Browse,
	ScanHardware,
	MatchedDevices,
	SelectMissing,
	InstallSelected,
	Ready,
	ColumnDevice,
	ColumnCategory,
	ColumnProvider,
	ColumnVersionDate,
	ColumnPackage,
	ColumnStatus,
	OperationLog,
	BrowseTitle,
	MissingDriver,
	InstalledUpdate,
	InstalledSuccess,
	InstalledReboot,
	InstallFailedStatus,
	LoadingIndex,
	EnumeratingDevices,
	IndexLoadFailed,
	DeviceScanFailed,
	InstallingDrivers,
	DataConfigMissing,
	SelectDriverFirst,
	SevenZipMediaMissing,
	UsingExtractor,
	StartInstall,
	InstallSuccessLog,
	InstallFailureLog,
	RebootSuffix,
	ScanSummaryFormat,
	InstallSummaryFormat,
	ConfigInvalid,
	IndexReadFailed,
	EnumerateFailed,
	ReadDeviceListFailed,
	UnknownError,
	SevenZipNotFound,
	ArchiveMissing,
	ProgramDataFailed,
	CacheCreateFailed,
	Extracting,
	SevenZipLaunchFailed,
	SevenZipExtractFailed,
	InfMissing,
	InstallingInf,
	NewDevLoadFailed,
	UpdateApiUnsupported,
	DriverInstallFailed,
	SystemMismatchFormat,
	VistaSp2Required,
	Win7Sp1Required,
	UnsupportedSystemConfig,
	VersionDetectionFailed,
	UnsupportedSystemTitle,
	UseCompatibleDriverPackage,
	Count
};

class Localization
{
public:
	static UiLanguage GetLanguage();
	static void SetLanguage(UiLanguage language);
	static UiLanguage DetectSystemLanguage();
	static const wchar_t* Text(TextId id);
};

inline const wchar_t* Tr(TextId id) { return Localization::Text(id); }
