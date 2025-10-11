#pragma once

#include "json.hpp"
#include "log.hpp"

#include <Windows.h>
#include <Shlobj.h>

#include <string>
#include <fstream>
#include <sstream>
#include <codecvt>

#include <nana/gui.hpp>

#pragma warning(disable : 4267)

extern logger g_log;
namespace fs = std::filesystem;

namespace util
{
	template<typename CharT>
	struct Sep : public std::numpunct<CharT>
	{
		virtual std::string do_grouping() const { return "\003"; }
	};

	enum class INTERNET_STATUS
	{
		CONNECTED,
		DISCONNECTED,
		CONNECTED_TO_LOCAL,
		CONNECTION_ERROR
	};

	using progress_callback = std::function<void(ULONGLONG, ULONGLONG, std::string, int, int)>;
	using append_callback = std::function<void(std::string, bool)>;

	std::string format_int(std::uint64_t i);
	std::string format_double(double f, unsigned precision = 2);
	std::string int_to_filesize(std::uint64_t i, bool with_bytes = true);
	std::string get_last_error_str(bool inet = false);
	HWND hwnd_from_pid(DWORD pid);
	std::vector<HWND> hwnds_from_pid(DWORD pid);
	std::string run_piped_process(std::wstring cmd, std::atomic_bool *working = nullptr, append_callback cbappend = nullptr,
								  progress_callback cbprog = nullptr, std::atomic_bool *graceful_exit = nullptr, std::string suppress = "");
	DWORD other_instance(std::wstring path = L"");
	unsigned close_children(bool report_only = false);
	std::wstring get_sys_folder(REFKNOWNFOLDERID rfid);
	std::string get_inet_res(std::string res, std::string *error = nullptr, bool truncate = false);
	std::string dl_inet_res(std::string res, fs::path fname, bool *working = nullptr, std::function<void(unsigned)> cb = nullptr);
	std::string extract_7z(fs::path arc_path, fs::path out_path, unsigned ffmpeg = 0, bool ytdlp_interface = false);
	std::wstring get_clipboard_text();
	void set_clipboard_text(HWND hwnd, std::wstring text);
	bool is_dir_writable(fs::path dir);
	bool is_win7();
	int scale(int val); // DPI scaling
	unsigned scale_uint(unsigned val); // DPI scaling
	INTERNET_STATUS check_inet_connection();
	BOOL pwr_shutdown();
	BOOL pwr_enable_shutdown_privilege();
	BOOL pwr_sleep();
	BOOL pwr_hibernate();
	bool pwr_can_hibernate();
	fs::path to_relative_path(const fs::path &abs);
}


class chronometer
{
	long long start, elapsed_;
	bool stopped;
	struct time { long long h, m, s; };

public:
	chronometer() { reset(); }
	void stop() { stopped = true; elapsed_ = (std::chrono::system_clock::now().time_since_epoch().count() - start) / 10000; }
	void reset() { stopped = false; elapsed_ = 0; start = std::chrono::system_clock::now().time_since_epoch().count(); }

	long long elapsed_ms()
	{
		if(stopped) return elapsed_;
		else return (std::chrono::system_clock::now().time_since_epoch().count() - start) / 10000;
	}

	long long elapsed_s() { return elapsed_ms() / 1000; }

	time elapsed()
	{
		auto es = elapsed_s();
		return {es / 3600, (es % 3600) / 60, (es % 3600) % 60};
	}
};


class regkey
{
	using string = std::string;
	using wstring = std::wstring;

	string error;
	HKEY hkey {nullptr}, hparent {nullptr};

public:

	regkey() = default;

	regkey(HKEY parent, HKEY key) noexcept : hparent {parent}, hkey {key} {}

	regkey(HKEY parent, const string &name, bool read_only = true, bool create = false) noexcept
	{
		LSTATUS res {0};
		if(create)
		{
			DWORD disposition {0};
			res = RegCreateKeyExA(parent, name.data(), 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, &disposition);
		}
		else res = RegOpenKeyExA(parent, name.data(), 0, read_only ? KEY_QUERY_VALUE : KEY_ALL_ACCESS, &hkey);

		if(res != ERROR_SUCCESS)
			error = __FUNCTION__ + string(" - ") + lstatus_to_str(res);
	}

	~regkey() { if(hkey) RegCloseKey(hkey); }

	operator bool() const { return hkey != nullptr; }
	operator HKEY() const { return hkey; }

	regkey create_subkey(const string &name)
	{
		HKEY hnewkey {nullptr};
		DWORD disposition {0};
		LSTATUS res = RegCreateKeyExA(hkey, name.data(), 0, 0, 0, KEY_ALL_ACCESS, 0, &hnewkey, &disposition);
		if(res == ERROR_SUCCESS)
			return regkey {hkey, hnewkey};
		error = __FUNCTION__ + string(" - ") + lstatus_to_str(res);
		return regkey {};
	}

	DWORD get_dword(const string &value_name)
	{
		DWORD data {DWORD(-1)}, cbdata {sizeof DWORD};
		LSTATUS res {RegQueryValueExA(hkey, value_name.data(), 0, 0, reinterpret_cast<LPBYTE>(&data), &cbdata)};
		if(res == ERROR_SUCCESS)
			error.clear();
		else error = __FUNCTION__ + string(" - ") + lstatus_to_str(res);
		return data;
	}

	wstring get_wstring(const wstring &value_name)
	{
		if(hkey == nullptr) return L"";
		DWORD bufsize {8192}; // buffer size in bytes
		wstring strval(bufsize / 2, '\0');
		LSTATUS res {RegQueryValueExW(hkey, value_name.data(), 0, 0, reinterpret_cast<LPBYTE>(&strval.front()), &bufsize)};
		if(res == ERROR_SUCCESS)
		{
			auto pathsize {bufsize / 2};
			if(strval[pathsize - 1] == '\0') pathsize--;
			strval.resize(pathsize);
		}
		else
		{
			strval.clear();
			error = __FUNCTION__ + string(" - ") + lstatus_to_str(res);
		}
		return strval;
	}

	string get_string(const string &value_name)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
		return cvt.to_bytes(get_wstring(cvt.from_bytes(value_name)));
	}

	bool set_string(string name, string value)
	{
		LSTATUS res = RegSetValueExA(hkey, name.data(), 0, REG_SZ, reinterpret_cast<const BYTE *>(value.data()), value.size() + 1);
		if(res == ERROR_SUCCESS) return true;
		error = __FUNCTION__ + string(" - ") + lstatus_to_str(res);
		return false;
	}

	string last_error() { return error; }

	static bool exists(HKEY parent, string name)
	{
		HKEY reskey {nullptr};
		LSTATUS res = RegOpenKeyExA(parent, name.c_str(), 0, KEY_READ, &reskey);
		if(reskey) RegCloseKey(reskey);
		return res != ERROR_FILE_NOT_FOUND;
	}

	// returns error string (string is empty when successful)
	static string delete_tree(HKEY parent, string name)
	{
		LSTATUS res = RegDeleteTreeA(parent, name.data());
		if(res == ERROR_SUCCESS) return "";
		return lstatus_to_str(res);
	}

private:

	static string lstatus_to_str(DWORD code)
	{
		char err[4096];
		FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, code, 0, err, 4096, 0);
		string str(err);
		size_t pos = str.find_first_of("\r\n");
		if(pos != string::npos) str.erase(pos);
		return str;
	}
};