#pragma once

#include "json.hpp"

#include <Windows.h>
#include <Shlobj.h>

#include <string>
#include <fstream>
#include <sstream>
#include <mutex>
#include <codecvt>

#include <nana/gui.hpp>

#pragma warning(disable : 4267)

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
	std::string run_piped_process(std::wstring cmd, bool *working = nullptr, append_callback cbappend = nullptr,
								  progress_callback cbprog = nullptr, bool *graceful_exit = nullptr, std::string suppress = "");
	DWORD other_instance(std::wstring path = L"");
	std::wstring get_sys_folder(REFKNOWNFOLDERID rfid);
	std::string get_inet_res(std::string res, std::string *error = nullptr, bool truncate = false);
	std::string dl_inet_res(std::string res, fs::path fname, bool *working = nullptr, std::function<void(unsigned)> cb = nullptr);
	std::string extract_7z(fs::path arc_path, fs::path out_path, unsigned ffmpeg = 0, bool ytdlp_interface = false);
	std::wstring get_clipboard_text();
	void set_clipboard_text(HWND hwnd, std::wstring text);
	bool is_dir_writable(fs::path dir);
	int scale(int val); // DPI scaling
	unsigned scale_uint(unsigned val); // DPI scaling
	INTERNET_STATUS check_inet_connection();
	BOOL pwr_shutdown();
	BOOL pwr_enable_shutdown_privilege();
	BOOL pwr_sleep();
	BOOL pwr_hibernate();
	bool pwr_can_hibernate();
}

// https://github.com/qPCR4vir/nana-demo/blob/master/Examples/windows-subclassing.cpp
class subclass
{
	struct msg_pro
	{
		std::function<bool(UINT, WPARAM, LPARAM, LRESULT*)> before;
		std::function<bool(UINT, WPARAM, LPARAM, LRESULT*)> after;
	};

	typedef std::lock_guard<std::recursive_mutex> lock_guard;
public:
	subclass(nana::window wd)
		: native_(reinterpret_cast<HWND>(nana::API::root(wd))),
		old_proc_(nullptr)
	{
	}

	~subclass()
	{
		clear();
	}

	void make_before(UINT msg, std::function<bool(UINT, WPARAM, LPARAM, LRESULT*)> fn)
	{
		lock_guard lock(mutex_);
		msg_table_[msg].before = std::move(fn);
		_m_subclass(true);
	}

	void make_after(UINT msg, std::function<bool(UINT, WPARAM, LPARAM, LRESULT*)> fn)
	{
		lock_guard lock(mutex_);
		msg_table_[msg].after = std::move(fn);
		_m_subclass(true);
	}

	void umake_before(UINT msg)
	{
		lock_guard lock(mutex_);
		auto i = msg_table_.find(msg);
		if(msg_table_.end() != i)
		{
			i->second.before = nullptr;
			if(nullptr == i->second.after)
			{
				msg_table_.erase(msg);
				if(msg_table_.empty())
					_m_subclass(false);
			}
		}
	}

	void umake_after(UINT msg)
	{
		lock_guard lock(mutex_);
		auto i = msg_table_.find(msg);
		if(msg_table_.end() != i)
		{
			i->second.after = nullptr;
			if(nullptr == i->second.before)
			{
				msg_table_.erase(msg);
				if(msg_table_.empty())
					_m_subclass(false);
			}
		}
	}

	void umake(UINT msg)
	{
		lock_guard lock(mutex_);
		msg_table_.erase(msg);

		if(msg_table_.empty())
			_m_subclass(false);
	}

	void clear()
	{
		lock_guard lock(mutex_);
		msg_table_.clear();
		_m_subclass(false);
	}
private:
	void _m_subclass(bool enable)
	{
		lock_guard lock(mutex_);

		if(enable)
		{
			if(native_ && (nullptr == old_proc_))
			{
				old_proc_ = (WNDPROC)::SetWindowLongPtr(native_, -4 /* GWL_WNDPROC*/, (LONG_PTR)_m_subclass_proc);
				if(old_proc_)
					table_[native_] = this;
			}
		}
		else
		{
			if(old_proc_)
			{
				table_.erase(native_);
				::SetWindowLongPtr(native_, -4 /* GWL_WNDPROC*/, (LONG_PTR)old_proc_);
				old_proc_ = nullptr;

			}
		}
	}

	static bool _m_call_before(msg_pro& pro, UINT msg, WPARAM wp, LPARAM lp, LRESULT* res)
	{
		return (pro.before ? pro.before(msg, wp, lp, res) : true);
	}

	static bool _m_call_after(msg_pro& pro, UINT msg, WPARAM wp, LPARAM lp, LRESULT* res)
	{
		return (pro.after ? pro.after(msg, wp, lp, res) : true);
	}
private:
	static LRESULT CALLBACK _m_subclass_proc(HWND wd, UINT msg, WPARAM wp, LPARAM lp)
	{
		lock_guard lock(mutex_);

		subclass * self = _m_find(wd);
		if(nullptr == self || nullptr == self->old_proc_)
			return 0;

		auto i = self->msg_table_.find(msg);
		if(self->msg_table_.end() == i)
			return ::CallWindowProc(self->old_proc_, wd, msg, wp, lp);

		LRESULT res = 0;
		if(self->_m_call_before(i->second, msg, wp, lp, &res))
		{
			res = ::CallWindowProc(self->old_proc_, wd, msg, wp, lp);
			self->_m_call_after(i->second, msg, wp, lp, &res);
		}

		if(WM_DESTROY == msg)
			self->clear();

		return res;
	}

	static subclass * _m_find(HWND wd)
	{
		lock_guard lock(mutex_);
		std::map<HWND, subclass*>::iterator i = table_.find(wd);
		if(i != table_.end())
			return i->second;

		return 0;
	}
private:
	HWND native_;
	WNDPROC old_proc_;
	std::map<UINT, msg_pro> msg_table_;

	static std::recursive_mutex mutex_;
	static std::map<HWND, subclass*> table_;
};


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