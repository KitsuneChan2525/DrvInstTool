#include "pch.h"
#include "DriverEngine.h"
#include "Localization.h"

#include <SetupAPI.h>
#include <newdev.h>
#include <ShlObj.h>
#include <algorithm>
#include <fstream>
#include <sstream>

#pragma comment(lib, "Setupapi.lib")
#pragma comment(lib, "Shell32.lib")

namespace
{
	std::wstring Utf8ToWide(const std::string& value)
	{
		if (value.empty())
			return {};
		const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
		if (count <= 0)
			return {};
		std::wstring result(static_cast<size_t>(count), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), &result[0], count);
		return result;
	}

	void AppendUtf8(std::string& output, unsigned int codePoint)
	{
		if (codePoint <= 0x7f)
			output.push_back(static_cast<char>(codePoint));
		else if (codePoint <= 0x7ff)
		{
			output.push_back(static_cast<char>(0xc0 | (codePoint >> 6)));
			output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
		}
		else if (codePoint <= 0xffff)
		{
			output.push_back(static_cast<char>(0xe0 | (codePoint >> 12)));
			output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
			output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
		}
		else
		{
			output.push_back(static_cast<char>(0xf0 | (codePoint >> 18)));
			output.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3f)));
			output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3f)));
			output.push_back(static_cast<char>(0x80 | (codePoint & 0x3f)));
		}
	}

	class JsonReader
	{
	public:
		explicit JsonReader(const std::string& text) : m_begin(text.data()), m_current(text.data()), m_end(text.data() + text.size()) {}

		void SkipSpace()
		{
			while (m_current < m_end && (*m_current == ' ' || *m_current == '\t' || *m_current == '\r' || *m_current == '\n'))
				++m_current;
		}

		bool Consume(char ch)
		{
			SkipSpace();
			if (m_current >= m_end || *m_current != ch)
				return false;
			++m_current;
			return true;
		}

		bool ParseString(std::string& output)
		{
			SkipSpace();
			if (m_current >= m_end || *m_current != '"')
				return false;
			++m_current;
			output.clear();
			while (m_current < m_end)
			{
				const unsigned char ch = static_cast<unsigned char>(*m_current++);
				if (ch == '"')
					return true;
				if (ch != '\\')
				{
					output.push_back(static_cast<char>(ch));
					continue;
				}
				if (m_current >= m_end)
					return false;
				switch (*m_current++)
				{
				case '"': output.push_back('"'); break;
				case '\\': output.push_back('\\'); break;
				case '/': output.push_back('/'); break;
				case 'b': output.push_back('\b'); break;
				case 'f': output.push_back('\f'); break;
				case 'n': output.push_back('\n'); break;
				case 'r': output.push_back('\r'); break;
				case 't': output.push_back('\t'); break;
				case 'u':
				{
					unsigned int codePoint = 0;
					if (!ParseHex4(codePoint))
						return false;
					if (codePoint >= 0xd800 && codePoint <= 0xdbff && m_end - m_current >= 6 && m_current[0] == '\\' && m_current[1] == 'u')
					{
						m_current += 2;
						unsigned int low = 0;
						if (!ParseHex4(low) || low < 0xdc00 || low > 0xdfff)
							return false;
						codePoint = 0x10000 + ((codePoint - 0xd800) << 10) + (low - 0xdc00);
					}
					AppendUtf8(output, codePoint);
					break;
				}
				default: return false;
				}
			}
			return false;
		}

		bool SkipValue()
		{
			SkipSpace();
			if (m_current >= m_end)
				return false;
			if (*m_current == '"')
			{
				std::string ignored;
				return ParseString(ignored);
			}
			if (*m_current == '{')
			{
				++m_current;
				SkipSpace();
				if (Consume('}')) return true;
				for (;;)
				{
					std::string key;
					if (!ParseString(key) || !Consume(':') || !SkipValue()) return false;
					if (Consume('}')) return true;
					if (!Consume(',')) return false;
				}
			}
			if (*m_current == '[')
			{
				++m_current;
				SkipSpace();
				if (Consume(']')) return true;
				for (;;)
				{
					if (!SkipValue()) return false;
					if (Consume(']')) return true;
					if (!Consume(',')) return false;
				}
			}
			while (m_current < m_end && *m_current != ',' && *m_current != '}' && *m_current != ']')
				++m_current;
			return true;
		}

	private:
		bool ParseHex4(unsigned int& value)
		{
			if (m_end - m_current < 4) return false;
			value = 0;
			for (int i = 0; i < 4; ++i)
			{
				const char ch = *m_current++;
				value <<= 4;
				if (ch >= '0' && ch <= '9') value += ch - '0';
				else if (ch >= 'a' && ch <= 'f') value += ch - 'a' + 10;
				else if (ch >= 'A' && ch <= 'F') value += ch - 'A' + 10;
				else return false;
			}
			return true;
		}

		const char* m_begin;
		const char* m_current;
		const char* m_end;
	};

	bool ReadAllBytes(const std::wstring& path, std::string& data)
	{
		std::ifstream stream(path, std::ios::binary);
		if (!stream) return false;
		stream.seekg(0, std::ios::end);
		const std::streamoff length = stream.tellg();
		if (length < 0) return false;
		stream.seekg(0, std::ios::beg);
		data.resize(static_cast<size_t>(length));
		if (length > 0) stream.read(&data[0], length);
		return stream.good() || stream.eof();
	}

	std::wstring JoinPath(const std::wstring& left, const std::wstring& right)
	{
		if (left.empty()) return right;
		if (right.empty()) return left;
		if (left.back() == L'\\' || left.back() == L'/') return left + right;
		return left + L"\\" + right;
	}

	std::wstring Upper(const std::wstring& value)
	{
		std::wstring result = value;
		if (!result.empty()) CharUpperBuffW(&result[0], static_cast<DWORD>(result.size()));
		return result;
	}

	struct PackageConfig
	{
		std::wstring category;
		std::wstring directory;
		std::wstring index;
	};

	struct DriverMediaConfig
	{
		std::wstring os;
		std::wstring osArchitecture;
		std::vector<PackageConfig> packages;
	};

	bool ParseStringField(JsonReader& reader, std::wstring& output)
	{
		std::string value;
		if (!reader.ParseString(value)) return false;
		output = Utf8ToWide(value);
		return true;
	}

	bool ParseConfigObject(JsonReader& reader, PackageConfig& config)
	{
		if (!reader.Consume('{')) return false;
		if (reader.Consume('}')) return true;
		for (;;)
		{
			std::string key;
			if (!reader.ParseString(key) || !reader.Consume(':')) return false;
			if (key == "Category") { if (!ParseStringField(reader, config.category)) return false; }
			else if (key == "DrvPkgDir") { if (!ParseStringField(reader, config.directory)) return false; }
			else if (key == "DrvIndex") { if (!ParseStringField(reader, config.index)) return false; }
			else if (!reader.SkipValue()) return false;
			if (reader.Consume('}')) return true;
			if (!reader.Consume(',')) return false;
		}
	}

	bool LoadConfig(const std::wstring& path, DriverMediaConfig& config)
	{
		std::string data;
		if (!ReadAllBytes(path, data)) return false;
		JsonReader reader(data);
		if (!reader.Consume('{')) return false;
		if (reader.Consume('}')) return true;
		for (;;)
		{
			std::string key;
			if (!reader.ParseString(key) || !reader.Consume(':')) return false;
			if (key == "OS")
			{
				if (!ParseStringField(reader, config.os)) return false;
			}
			else if (key == "OSArch")
			{
				if (!ParseStringField(reader, config.osArchitecture)) return false;
			}
			else if (key == "DriverPackages")
			{
				if (!reader.Consume('[')) return false;
				if (!reader.Consume(']'))
				{
					for (;;)
					{
						PackageConfig package;
						if (!ParseConfigObject(reader, package)) return false;
						if (!package.category.empty() && !package.directory.empty() && !package.index.empty())
							config.packages.push_back(package);
						if (reader.Consume(']')) break;
						if (!reader.Consume(',')) return false;
					}
				}
			}
			else if (!reader.SkipValue()) return false;
			if (reader.Consume('}')) return true;
			if (!reader.Consume(',')) return false;
		}
	}

	bool ParseArchive(JsonReader& reader, DriverPackage& package)
	{
		if (!reader.Consume('{')) return false;
		if (reader.Consume('}')) return true;
		for (;;)
		{
			std::string key;
			if (!reader.ParseString(key) || !reader.Consume(':')) return false;
			if (key == "file") { if (!ParseStringField(reader, package.archiveFile)) return false; }
			else if (key == "subdirectory") { if (!ParseStringField(reader, package.archiveSubdirectory)) return false; }
			else if (!reader.SkipValue()) return false;
			if (reader.Consume('}')) return true;
			if (!reader.Consume(',')) return false;
		}
	}

	bool ParseDriver(JsonReader& reader, DriverPackage& package)
	{
		if (!reader.Consume('{')) return false;
		if (reader.Consume('}')) return true;
		for (;;)
		{
			std::string key;
			if (!reader.ParseString(key) || !reader.Consume(':')) return false;
			if (key == "driver_id") { if (!ParseStringField(reader, package.id)) return false; }
			else if (key == "category") { if (!ParseStringField(reader, package.category)) return false; }
			else if (key == "inf_path") { if (!ParseStringField(reader, package.infPath)) return false; }
			else if (key == "driver_date") { if (!ParseStringField(reader, package.driverDate)) return false; }
			else if (key == "driver_version") { if (!ParseStringField(reader, package.driverVersion)) return false; }
			else if (key == "provider") { if (!ParseStringField(reader, package.provider)) return false; }
			else if (key == "device_class") { if (!ParseStringField(reader, package.deviceClass)) return false; }
			else if (key == "archive") { if (!ParseArchive(reader, package)) return false; }
			else if (!reader.SkipValue()) return false;
			if (reader.Consume('}')) return true;
			if (!reader.Consume(',')) return false;
		}
	}

	bool ParseHardwareRoute(JsonReader& reader, std::wstring& driverId, std::wstring& deviceName)
	{
		if (!reader.Consume('{')) return false;
		if (reader.Consume('}')) return true;
		for (;;)
		{
			std::string key;
			if (!reader.ParseString(key) || !reader.Consume(':')) return false;
			if (key == "selected_driver_id")
			{
				if (!ParseStringField(reader, driverId)) return false;
			}
			else if (key == "device_names")
			{
				if (!reader.Consume('[')) return false;
				if (!reader.Consume(']'))
				{
					if (!ParseStringField(reader, deviceName)) return false;
					while (!reader.Consume(']'))
					{
						if (!reader.Consume(',') || !reader.SkipValue()) return false;
					}
				}
			}
			else if (!reader.SkipValue()) return false;
			if (reader.Consume('}')) return true;
			if (!reader.Consume(',')) return false;
		}
	}

	bool ParseIndex(const std::wstring& path, const std::wstring& packageDirectory,
		std::unordered_map<std::wstring, DriverPackage>& drivers,
		std::unordered_map<std::wstring, DriverCatalog::HardwareRoute>& routes)
	{
		std::string data;
		if (!ReadAllBytes(path, data)) return false;
		JsonReader reader(data);
		if (!reader.Consume('{')) return false;
		if (reader.Consume('}')) return true;
		for (;;)
		{
			std::string key;
			if (!reader.ParseString(key) || !reader.Consume(':')) return false;
			if (key == "drivers")
			{
				if (!reader.Consume('{')) return false;
				if (!reader.Consume('}'))
				{
					for (;;)
					{
						std::string objectKey;
						DriverPackage package;
						if (!reader.ParseString(objectKey) || !reader.Consume(':') || !ParseDriver(reader, package)) return false;
						if (package.id.empty()) package.id = Utf8ToWide(objectKey);
						package.packageDirectory = packageDirectory;
						drivers[package.id] = package;
						if (reader.Consume('}')) break;
						if (!reader.Consume(',')) return false;
					}
				}
			}
			else if (key == "hardware_id_map")
			{
				if (!reader.Consume('{')) return false;
				if (!reader.Consume('}'))
				{
					for (;;)
					{
						std::string hardwareId;
						std::wstring driverId, deviceName;
						if (!reader.ParseString(hardwareId) || !reader.Consume(':') || !ParseHardwareRoute(reader, driverId, deviceName)) return false;
						if (!driverId.empty()) routes[Upper(Utf8ToWide(hardwareId))] = { driverId, deviceName };
						if (reader.Consume('}')) break;
						if (!reader.Consume(',')) return false;
					}
				}
			}
			else if (!reader.SkipValue()) return false;
			if (reader.Consume('}')) return true;
			if (!reader.Consume(',')) return false;
		}
	}

	std::wstring FormatWin32Error(DWORD code)
	{
		wchar_t* buffer = nullptr;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, code, 0, reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
		std::wstring message = buffer ? buffer : Tr(TextId::UnknownError);
		if (buffer) LocalFree(buffer);
		while (!message.empty() && (message.back() == L'\r' || message.back() == L'\n')) message.pop_back();
		return message;
	}

	bool ReadRegistryProperty(HDEVINFO deviceInfoSet, SP_DEVINFO_DATA& deviceInfo, DWORD property, std::vector<BYTE>& buffer)
	{
		DWORD required = 0, type = 0;
		SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &deviceInfo, property, &type, nullptr, 0, &required);
		if (required == 0) return false;
		buffer.resize(required + sizeof(wchar_t));
		return SetupDiGetDeviceRegistryPropertyW(deviceInfoSet, &deviceInfo, property, &type, buffer.data(), static_cast<DWORD>(buffer.size()), &required) != FALSE;
	}

	std::wstring GetDeviceText(HDEVINFO set, SP_DEVINFO_DATA& device, DWORD property)
	{
		std::vector<BYTE> data;
		if (!ReadRegistryProperty(set, device, property, data)) return {};
		return reinterpret_cast<const wchar_t*>(data.data());
	}

	std::vector<std::wstring> GetMultiString(HDEVINFO set, SP_DEVINFO_DATA& device, DWORD property)
	{
		std::vector<BYTE> data;
		std::vector<std::wstring> result;
		if (!ReadRegistryProperty(set, device, property, data)) return result;
		const wchar_t* current = reinterpret_cast<const wchar_t*>(data.data());
		while (*current)
		{
			result.emplace_back(current);
			current += wcslen(current) + 1;
		}
		return result;
	}

	std::wstring Quote(const std::wstring& value)
	{
		return L"\"" + value + L"\"";
	}

	bool EnsureDirectory(const std::wstring& path)
	{
		const int result = SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
		return result == ERROR_SUCCESS || result == ERROR_ALREADY_EXISTS || GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
	}

	bool RunProcess(const std::wstring& commandLine, DWORD& exitCode, std::wstring& error)
	{
		std::vector<wchar_t> command(commandLine.begin(), commandLine.end());
		command.push_back(L'\0');
		STARTUPINFOW startup = {};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION process = {};
		if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
		{
			error = FormatWin32Error(GetLastError());
			return false;
		}
		WaitForSingleObject(process.hProcess, INFINITE);
		GetExitCodeProcess(process.hProcess, &exitCode);
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
		return true;
	}

	std::wstring ModuleDirectory()
	{
		wchar_t path[MAX_PATH] = {};
		GetModuleFileNameW(nullptr, path, _countof(path));
		wchar_t* slash = wcsrchr(path, L'\\');
		if (slash) *slash = L'\0';
		return path;
	}

	std::wstring ParentDirectory(const std::wstring& path)
	{
		const size_t slash = path.find_last_of(L"\\/");
		return slash == std::wstring::npos ? std::wstring() : path.substr(0, slash);
	}

	struct DetectedWindowsVersion
	{
		unsigned long majorVersion = 0;
		unsigned long minorVersion = 0;
		unsigned long buildNumber = 0;
		std::wstring architecture;
	};

	std::wstring NormalizeArchitecture(const std::wstring& architecture)
	{
		const std::wstring value = Upper(architecture);
		if (value == L"X64" || value == L"AMD64") return L"x64";
		if (value == L"X86" || value == L"I386" || value == L"WIN32") return L"x86";
		return {};
	}

	bool DetectWindowsVersion(DetectedWindowsVersion& version)
	{
		struct RtlVersionInfo
		{
			ULONG size;
			ULONG majorVersion;
			ULONG minorVersion;
			ULONG buildNumber;
			ULONG platformId;
			WCHAR servicePack[128];
		};
		using RtlGetVersionFunction = LONG(WINAPI*)(RtlVersionInfo*);
		HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
		RtlGetVersionFunction rtlGetVersion = ntdll == nullptr ? nullptr :
			reinterpret_cast<RtlGetVersionFunction>(GetProcAddress(ntdll, "RtlGetVersion"));
		if (rtlGetVersion == nullptr) return false;
		RtlVersionInfo info = {};
		info.size = sizeof(info);
		if (rtlGetVersion(&info) < 0) return false;
		version.majorVersion = info.majorVersion;
		version.minorVersion = info.minorVersion;
		version.buildNumber = info.buildNumber;

		SYSTEM_INFO systemInfo = {};
		GetNativeSystemInfo(&systemInfo);
		switch (systemInfo.wProcessorArchitecture)
		{
		case PROCESSOR_ARCHITECTURE_AMD64: version.architecture = L"x64"; break;
		case PROCESSOR_ARCHITECTURE_INTEL: version.architecture = L"x86"; break;
		case 12: version.architecture = L"ARM64"; break;
		default: version.architecture = L"unknown"; break;
		}
		return true;
	}

	std::wstring NumericVersion(unsigned long majorVersion, unsigned long minorVersion, unsigned long buildNumber)
	{
		return std::to_wstring(majorVersion) + L"." + std::to_wstring(minorVersion) + L"." + std::to_wstring(buildNumber);
	}

	std::wstring CurrentVersionDescription(const DetectedWindowsVersion& version)
	{
		std::wstring name = L"Windows";
		if (version.majorVersion == 6 && version.minorVersion == 0) name = L"Windows Vista";
		else if (version.majorVersion == 6 && version.minorVersion == 1) name = L"Windows 7";
		else if (version.majorVersion == 6 && version.minorVersion == 2) name = L"Windows 8";
		else if (version.majorVersion == 6 && version.minorVersion == 3) name = L"Windows 8.1";
		else if (version.majorVersion >= 10) name = version.buildNumber >= 22000 ? L"Windows 11" : L"Windows 10";
		return name + L" " + NumericVersion(version.majorVersion, version.minorVersion, version.buildNumber);
	}
}

bool SystemCompatibility::IsVersionSupported(const std::wstring& targetOs, unsigned long majorVersion,
	unsigned long minorVersion, unsigned long buildNumber, std::wstring& requiredVersion,
	std::wstring& updateHint)
{
	requiredVersion.clear();
	updateHint.clear();
	const std::wstring os = Upper(targetOs);
	if (os == L"WINVISTA")
	{
		requiredVersion = L"Windows Vista 6.0.6002";
		if (majorVersion == 6 && minorVersion == 0 && (buildNumber == 6000 || buildNumber == 6001))
			updateHint = Tr(TextId::VistaSp2Required);
		return majorVersion == 6 && minorVersion == 0 && buildNumber == 6002;
	}
	if (os == L"WIN7")
	{
		requiredVersion = L"Windows 7 6.1.7601";
		if (majorVersion == 6 && minorVersion == 1 && buildNumber == 7600)
			updateHint = Tr(TextId::Win7Sp1Required);
		return majorVersion == 6 && minorVersion == 1 && buildNumber == 7601;
	}
	if (os == L"WIN8")
	{
		requiredVersion = L"Windows 8 6.2.9200 / Windows 8.1 6.3.9600";
		return (majorVersion == 6 && minorVersion == 2 && buildNumber == 9200) ||
			(majorVersion == 6 && minorVersion == 3 && buildNumber == 9600);
	}
	if (os == L"WIN10")
	{
		switch (Localization::GetLanguage())
		{
		case UiLanguage::ChineseSimplified: requiredVersion = L"Windows 10 10.0.10240 或更高版本"; break;
		case UiLanguage::ChineseTraditional: requiredVersion = L"Windows 10 10.0.10240 或更新版本"; break;
		default: requiredVersion = L"Windows 10 10.0.10240 or later"; break;
		}
		return majorVersion > 10 || (majorVersion == 10 &&
			(minorVersion > 0 || (minorVersion == 0 && buildNumber >= 10240)));
	}
	return false;
}

bool SystemCompatibility::ValidateDriverMedia(const std::wstring& dataRoot, std::wstring& error)
{
	const std::wstring configPath = JoinPath(dataRoot, L"config.json");
	DriverMediaConfig config;
	if (!LoadConfig(configPath, config))
	{
		error = std::wstring(Tr(TextId::ConfigInvalid)) + configPath;
		return false;
	}
	const std::wstring requiredArchitecture = NormalizeArchitecture(config.osArchitecture);
	std::wstring requiredVersion;
	std::wstring updateHint;
	if (requiredArchitecture.empty() ||
		!IsVersionSupported(config.os, 0, 0, 0, requiredVersion, updateHint) && requiredVersion.empty())
	{
		error = std::wstring(Tr(TextId::UnsupportedSystemConfig)) + config.os + L" / " + config.osArchitecture;
		return false;
	}

	DetectedWindowsVersion current;
	if (!DetectWindowsVersion(current))
	{
		error = Tr(TextId::VersionDetectionFailed);
		return false;
	}
	const bool versionMatches = IsVersionSupported(config.os, current.majorVersion, current.minorVersion,
		current.buildNumber, requiredVersion, updateHint);
	const bool architectureMatches = current.architecture == requiredArchitecture;
	if (versionMatches && architectureMatches) return true;

	CString message;
	message.Format(Tr(TextId::SystemMismatchFormat), CurrentVersionDescription(current).c_str(),
		current.architecture.c_str(), requiredVersion.c_str(), requiredArchitecture.c_str());
	error = message.GetString();
	if (!updateHint.empty()) error += L"\n" + updateHint;
	return false;
}

bool SystemCompatibility::RunRuleTests(std::wstring& error)
{
	struct TestCase
	{
		const wchar_t* os;
		unsigned long majorVersion;
		unsigned long minorVersion;
		unsigned long buildNumber;
		bool expected;
		bool expectsUpdateHint;
	};
	const TestCase cases[] =
	{
		{ L"WinVista", 6, 0, 6002, true, false },
		{ L"WinVista", 6, 0, 6000, false, true },
		{ L"WinVista", 6, 0, 6001, false, true },
		{ L"Win7", 6, 1, 7601, true, false },
		{ L"Win7", 6, 1, 7600, false, true },
		{ L"Win8", 6, 2, 9200, true, false },
		{ L"Win8", 6, 3, 9600, true, false },
		// Negative case: reject an invalid Win8/8.1 version-and-build pairing.
		{ L"Win8", 6, 3, 9200, false, false },
		{ L"Win10", 10, 0, 10240, true, false },
		// Negative boundary: reject the build immediately below the Win10 RTM minimum.
		{ L"Win10", 10, 0, 10239, false, false },
		{ L"Win10", 10, 0, 26100, true, false }
	};
	for (const auto& item : cases)
	{
		std::wstring requiredVersion;
		std::wstring updateHint;
		const bool actual = IsVersionSupported(item.os, item.majorVersion, item.minorVersion,
			item.buildNumber, requiredVersion, updateHint);
		if (actual != item.expected || (!updateHint.empty()) != item.expectsUpdateHint)
		{
			error = std::wstring(L"Compatibility rule failed: ") + item.os + L" " +
				NumericVersion(item.majorVersion, item.minorVersion, item.buildNumber);
			return false;
		}
	}
	if (NormalizeArchitecture(L"x64") != L"x64" || NormalizeArchitecture(L"AMD64") != L"x64" ||
		NormalizeArchitecture(L"x86") != L"x86" || NormalizeArchitecture(L"Win32") != L"x86")
	{
		error = L"Architecture normalization failed";
		return false;
	}
	error.clear();
	return true;
}

bool DriverCatalog::Load(const std::wstring& dataRoot, std::wstring& error)
{
	m_drivers.clear();
	m_hardwareIds.clear();
	DriverMediaConfig config;
	const std::wstring configPath = JoinPath(dataRoot, L"config.json");
	if (!LoadConfig(configPath, config) || config.packages.empty())
	{
		error = std::wstring(Tr(TextId::ConfigInvalid)) + configPath;
		return false;
	}
	for (const auto& package : config.packages)
	{
		const std::wstring path = JoinPath(JoinPath(dataRoot, package.directory), package.index);
		if (!ParseIndex(path, package.directory, m_drivers, m_hardwareIds))
		{
			error = std::wstring(Tr(TextId::IndexReadFailed)) + path;
			m_drivers.clear();
			m_hardwareIds.clear();
			return false;
		}
	}
	return true;
}

bool DriverCatalog::Match(const std::vector<std::wstring>& hardwareIds, DriverPackage& driver,
	std::wstring& matchedHardwareId, std::wstring& indexedDeviceName) const
{
	for (const auto& hardwareId : hardwareIds)
	{
		auto route = m_hardwareIds.find(Upper(hardwareId));
		if (route == m_hardwareIds.end()) continue;
		auto package = m_drivers.find(route->second.driverId);
		if (package == m_drivers.end()) continue;
		driver = package->second;
		matchedHardwareId = hardwareId;
		indexedDeviceName = route->second.deviceName;
		return true;
	}
	return false;
}

bool DeviceScanner::Scan(const DriverCatalog& catalog, std::vector<DeviceMatch>& matches,
	int& scannedDeviceCount, std::wstring& error)
{
	matches.clear();
	scannedDeviceCount = 0;
	HDEVINFO set = SetupDiGetClassDevsW(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);
	if (set == INVALID_HANDLE_VALUE)
	{
		error = std::wstring(Tr(TextId::EnumerateFailed)) + FormatWin32Error(GetLastError());
		return false;
	}
	for (DWORD index = 0;; ++index)
	{
		SP_DEVINFO_DATA device = {};
		device.cbSize = sizeof(device);
		if (!SetupDiEnumDeviceInfo(set, index, &device))
		{
			if (GetLastError() != ERROR_NO_MORE_ITEMS) error = std::wstring(Tr(TextId::ReadDeviceListFailed)) + FormatWin32Error(GetLastError());
			break;
		}
		++scannedDeviceCount;
		std::vector<std::wstring> ids = GetMultiString(set, device, SPDRP_HARDWAREID);
		if (ids.empty()) continue;
		DriverPackage package;
		std::wstring matchedId, indexedName;
		if (!catalog.Match(ids, package, matchedId, indexedName))
		{
			std::vector<std::wstring> compatible = GetMultiString(set, device, SPDRP_COMPATIBLEIDS);
			if (!catalog.Match(compatible, package, matchedId, indexedName)) continue;
		}
		DeviceMatch match;
		wchar_t instanceId[4096] = {};
		if (SetupDiGetDeviceInstanceIdW(set, &device, instanceId, _countof(instanceId), nullptr)) match.instanceId = instanceId;
		match.displayName = GetDeviceText(set, device, SPDRP_FRIENDLYNAME);
		if (match.displayName.empty()) match.displayName = GetDeviceText(set, device, SPDRP_DEVICEDESC);
		if (match.displayName.empty()) match.displayName = indexedName.empty() ? matchedId : indexedName;
		match.hardwareId = matchedId;
		match.indexedDeviceName = indexedName;
		match.driver = package;
		std::vector<BYTE> installedDriver;
		match.needsDriver = !ReadRegistryProperty(set, device, SPDRP_DRIVER, installedDriver);
		matches.push_back(match);
	}
	SetupDiDestroyDeviceInfoList(set);
	return error.empty();
}

std::wstring DriverInstaller::Find7Zip(const std::wstring& dataRoot)
{
	// 驱动介质的固定布局是 <介质根>\Data 和 <介质根>\bin。
	const std::wstring media7Zip = JoinPath(JoinPath(ParentDirectory(dataRoot), L"bin"), L"7z.exe");
	if (GetFileAttributesW(media7Zip.c_str()) != INVALID_FILE_ATTRIBUTES) return media7Zip;
	return {};
}

bool DriverInstaller::Install(const std::wstring& dataRoot, const DeviceMatch& match,
	bool& rebootRequired, std::wstring& error, const LogCallback& log)
{
	rebootRequired = false;
	const std::wstring sevenZip = Find7Zip(dataRoot);
	if (sevenZip.empty())
	{
		error = Tr(TextId::SevenZipNotFound);
		return false;
	}
	const std::wstring archive = JoinPath(JoinPath(dataRoot, match.driver.packageDirectory), match.driver.archiveFile);
	if (GetFileAttributesW(archive.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		error = std::wstring(Tr(TextId::ArchiveMissing)) + archive;
		return false;
	}
	wchar_t commonData[MAX_PATH] = {};
	if (FAILED(SHGetFolderPathW(nullptr, CSIDL_COMMON_APPDATA, nullptr, SHGFP_TYPE_CURRENT, commonData)))
	{
		error = Tr(TextId::ProgramDataFailed);
		return false;
	}
	std::wstring archiveBase = match.driver.archiveFile;
	const size_t extension = archiveBase.find_last_of(L'.');
	if (extension != std::wstring::npos) archiveBase.resize(extension);
	const std::wstring cache = JoinPath(JoinPath(JoinPath(commonData, L"kitsuneDriverInstaller"), L"Cache"), match.driver.packageDirectory + L"\\" + archiveBase);
	if (!EnsureDirectory(cache))
	{
		error = std::wstring(Tr(TextId::CacheCreateFailed)) + cache;
		return false;
	}
	const std::wstring inf = JoinPath(cache, match.driver.infPath);
	if (GetFileAttributesW(inf.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		const size_t slash = match.driver.infPath.find_last_of(L"\\/");
		const std::wstring pattern = slash == std::wstring::npos ? L"*" : match.driver.infPath.substr(0, slash) + L"\\*";
		if (log) log(std::wstring(Tr(TextId::Extracting)) + match.driver.archiveFile + L" / " + pattern);
		const std::wstring command = Quote(sevenZip) + L" x -y -aoa -bd " + Quote(L"-o" + cache) + L" " + Quote(archive) + L" " + Quote(pattern);
		DWORD exitCode = 0;
		if (!RunProcess(command, exitCode, error))
		{
			error = std::wstring(Tr(TextId::SevenZipLaunchFailed)) + error;
			return false;
		}
		if (exitCode != 0)
		{
			error = std::wstring(Tr(TextId::SevenZipExtractFailed)) + std::to_wstring(exitCode) + L".";
			return false;
		}
	}
	if (GetFileAttributesW(inf.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		error = std::wstring(Tr(TextId::InfMissing)) + inf;
		return false;
	}
	if (log) log(std::wstring(Tr(TextId::InstallingInf)) + match.driver.id + L": " + inf);
	using UpdateDriverFn = BOOL(WINAPI*)(HWND, LPCWSTR, LPCWSTR, DWORD, PBOOL);
	HMODULE newDev = LoadLibraryW(L"newdev.dll");
	if (!newDev)
	{
		error = std::wstring(Tr(TextId::NewDevLoadFailed)) + FormatWin32Error(GetLastError());
		return false;
	}
	auto updateDriver = reinterpret_cast<UpdateDriverFn>(GetProcAddress(newDev, "UpdateDriverForPlugAndPlayDevicesW"));
	if (!updateDriver)
	{
		FreeLibrary(newDev);
		error = Tr(TextId::UpdateApiUnsupported);
		return false;
	}
	BOOL reboot = FALSE;
	const BOOL installed = updateDriver(nullptr, match.hardwareId.c_str(), inf.c_str(), INSTALLFLAG_FORCE, &reboot);
	const DWORD installError = installed ? ERROR_SUCCESS : GetLastError();
	FreeLibrary(newDev);
	if (!installed)
	{
		error = std::wstring(Tr(TextId::DriverInstallFailed)) + FormatWin32Error(installError) + L" (" + std::to_wstring(installError) + L")";
		return false;
	}
	rebootRequired = reboot != FALSE;
	return true;
}
