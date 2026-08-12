#include "pch.h"
#include "Localization.h"

namespace
{
	UiLanguage g_language = UiLanguage::English;

	const wchar_t* const kText[static_cast<size_t>(TextId::Count)][3] =
	{
		{ L"kitsune Driver Installer", L"kitsune Driver Installer", L"kitsune Driver Installer" },
		{ L"Driver installation", L"驱动安装", L"驅動程式安裝" },
		{ L"Driver package target system", L"驱动程序包目标系统", L"驅動程式套件目標系統" },
		{ L"Language", L"语言", L"語言" },
		{ L"English", L"英语", L"英文" },
		{ L"Simplified Chinese", L"简体中文", L"簡體中文" },
		{ L"Traditional Chinese", L"繁体中文", L"繁體中文" },
		{ L"Driver data", L"驱动数据目录", L"驅動程式資料目錄" },
		{ L"Browse...", L"浏览…", L"瀏覽…" },
		{ L"Scan hardware", L"扫描硬件", L"掃描硬體" },
		{ L"Select missing", L"选择缺失驱动", L"選擇缺少的驅動程式" },
		{ L"Install selected", L"安装所选驱动", L"安裝選取的驅動程式" },
		{ L"Device name", L"设备名称", L"裝置名稱" },
		{ L"Provider", L"提供商", L"提供者" },
		{ L"Version", L"版本", L"版本" },
		{ L"Date", L"日期", L"日期" },
		{ L"Status", L"状态", L"狀態" },
		{ L"Log", L"日志", L"記錄" },
		{ L"Select the Data directory containing config.json", L"选择包含 config.json 的 Data 目录", L"選擇包含 config.json 的 Data 目錄" },
		{ L"Missing", L"缺失", L"缺少" },
		{ L"Update available", L"可更新", L"可更新" },
		{ L"Installed", L"已安装", L"已安裝" },
		{ L"Installed - restart required", L"成功，需重启", L"成功，需重新啟動" },
		{ L"Failed", L"安装失败", L"安裝失敗" },
		{ L"Loading driver indexes...", L"正在加载驱动索引…", L"正在載入驅動程式索引…" },
		{ L"Enumerating Plug and Play devices...", L"正在枚举即插即用设备…", L"正在列舉隨插即用裝置…" },
		{ L"Index loading failed", L"索引加载失败", L"索引載入失敗" },
		{ L"Device scan failed", L"设备扫描失败", L"裝置掃描失敗" },
		{ L"Installing drivers...", L"正在安装驱动…", L"正在安裝驅動程式…" },
		{ L"config.json was not found. Select the Data directory on the driver media.", L"所选目录中没有 config.json。请选择驱动介质的 Data 目录。", L"所選目錄中沒有 config.json。請選擇驅動程式媒體的 Data 目錄。" },
		{ L"Select at least one driver first.", L"请先勾选要安装的驱动。", L"請先勾選要安裝的驅動程式。" },
		{ L"7z.exe was not found in the bin directory next to Data.", L"未找到与 Data 同级 bin 目录内的 7z.exe。", L"找不到與 Data 同層 bin 目錄內的 7z.exe。" },
		{ L"Extractor: ", L"使用解压程序：", L"使用解壓縮程式：" },
		{ L"Installing ", L"开始安装 ", L"開始安裝 " },
		{ L"Installed: ", L"安装成功：", L"安裝成功：" },
		{ L"Failed: ", L"安装失败：", L"安裝失敗：" },
		{ L", restart Windows", L"，请重启系统", L"，請重新啟動系統" },
		{ L"Scanned %d devices; matched %u drivers (index: %u drivers / %u hardware IDs)", L"扫描 %d 个设备，匹配 %u 个驱动（索引：%u 个驱动 / %u 个硬件 ID）", L"掃描 %d 個裝置，比對 %u 個驅動程式（索引：%u 個驅動程式 / %u 個硬體 ID）" },
		{ L"Installation finished: %d of %u succeeded%s", L"安装结束：成功 %d / %u%s", L"安裝結束：成功 %d / %u%s" },
		{ L"Invalid configuration or no driver packages: ", L"配置文件格式无效或没有驱动包：", L"設定檔格式無效或沒有驅動程式套件：" },
		{ L"Failed to read driver index: ", L"驱动索引读取失败：", L"驅動程式索引讀取失敗：" },
		{ L"Failed to enumerate devices: ", L"枚举设备失败：", L"列舉裝置失敗：" },
		{ L"Failed to read the device list: ", L"读取设备列表失败：", L"讀取裝置清單失敗：" },
		{ L"Unknown error", L"未知错误", L"未知錯誤" },
		{ L"7z.exe was not found. Place it in the bin directory next to Data.", L"未找到 7z.exe。请将 7z.exe 放到驱动介质的 bin 子目录（与 Data 同级）。", L"找不到 7z.exe。請將 7z.exe 放到驅動程式媒體的 bin 子目錄（與 Data 同層）。" },
		{ L"Driver archive does not exist: ", L"驱动压缩包不存在：", L"驅動程式壓縮檔不存在：" },
		{ L"Failed to locate ProgramData.", L"获取 ProgramData 目录失败。", L"取得 ProgramData 目錄失敗。" },
		{ L"Failed to create driver cache: ", L"创建驱动缓存目录失败：", L"建立驅動程式快取目錄失敗：" },
		{ L"Extracting ", L"解压 ", L"解壓縮 " },
		{ L"Failed to start 7-Zip: ", L"启动 7-Zip 失败：", L"啟動 7-Zip 失敗：" },
		{ L"7-Zip extraction failed; exit code ", L"7-Zip 解压失败，退出码 ", L"7-Zip 解壓縮失敗，結束代碼 " },
		{ L"INF not found after extraction: ", L"解压后未找到 INF：", L"解壓縮後找不到 INF：" },
		{ L"Installing INF ", L"安装 ", L"安裝 " },
		{ L"Failed to load newdev.dll: ", L"加载 newdev.dll 失败：", L"載入 newdev.dll 失敗：" },
		{ L"UpdateDriverForPlugAndPlayDevicesW is unavailable.", L"系统不支持 UpdateDriverForPlugAndPlayDevicesW。", L"系統不支援 UpdateDriverForPlugAndPlayDevicesW。" },
		{ L"Driver installation failed: ", L"驱动安装失败：", L"驅動程式安裝失敗：" },
		{ L"Current system version: %s %s (%s)\nDriver-required system version: %s %s (%s)", L"当前系统版本：%s %s (%s)\n驱动程序所需系统版本：%s %s (%s)", L"目前系統版本：%s %s (%s)\n驅動程式所需系統版本：%s %s (%s)" },
		{ L"Update the operating system to Service Pack 2 before continuing driver installation.", L"请更新操作系统至 Service Pack 2 后继续执行驱动程序安装", L"請將作業系統更新至 Service Pack 2 後再繼續執行驅動程式安裝" },
		{ L"Update the operating system to Service Pack 1 before continuing driver installation.", L"请更新操作系统至 Service Pack 1 后继续执行驱动程序安装", L"請將作業系統更新至 Service Pack 1 後再繼續執行驅動程式安裝" },
		{ L"Unsupported OS or OSArch in config.json: ", L"config.json 中的 OS 或 OSArch 不受支持：", L"config.json 中的 OS 或 OSArch 不受支援：" },
		{ L"Failed to detect the Windows version.", L"检测 Windows 版本失败。", L"偵測 Windows 版本失敗。" },
		{ L"Unsupported system version", L"不支持的系统版本", L"不支援的系統版本" },
		{ L"Use a compatible driver package before continuing driver installation.", L"请更换至兼容的驱动程序包后继续执行驱动程序安装", L"請更換為相容的驅動程式套件後再繼續執行驅動程式安裝" },
		{ L"Operation failed: an unsupported operating system version was detected.", L"操作失败：检测到不支持的操作系统版本", L"操作失敗：偵測到不支援的作業系統版本" }
	};
}

UiLanguage Localization::GetLanguage() { return g_language; }
void Localization::SetLanguage(UiLanguage language) { g_language = language; }

UiLanguage Localization::DetectSystemLanguage()
{
	const LANGID language = GetUserDefaultUILanguage();
	if (PRIMARYLANGID(language) != LANG_CHINESE) return UiLanguage::English;

	const WORD sub = SUBLANGID(language);
	if (sub == SUBLANG_CHINESE_SIMPLIFIED || sub == SUBLANG_CHINESE_SINGAPORE)
		return UiLanguage::ChineseSimplified;
	if (sub == SUBLANG_CHINESE_TRADITIONAL || sub == SUBLANG_CHINESE_HONGKONG ||
		sub == SUBLANG_CHINESE_MACAU)
		return UiLanguage::ChineseTraditional;

	// English is the fallback for every unrecognized or neutral UI language.
	return UiLanguage::English;
}

const wchar_t* Localization::Text(TextId id)
{
	const size_t row = static_cast<size_t>(id);
	const size_t column = static_cast<size_t>(g_language);
	if (row >= static_cast<size_t>(TextId::Count) || column >= 3) return L"";
	return kText[row][column];
}
