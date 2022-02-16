#include "util.hpp"
#include "bitextractor.hpp"
#include "bitexception.hpp"

#include <WinInet.h>
#include <TlHelp32.h>

std::recursive_mutex subclass::mutex_;
std::map<HWND, subclass*> subclass::table_;

std::string util::format_int(unsigned i)
{
	std::stringstream ss;
	ss.imbue(std::locale {ss.getloc(), new Sep<char>{}});
	ss << i;
	return ss.str();
}

std::string util::format_float(float f, unsigned precision)
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(precision) << f;
	return ss.str();
}

std::string util::int_to_filesize(unsigned i, bool with_bytes)
{
	float f {static_cast<float>(i)};
	std::string s, bytes {" (" + format_int(i) + ")"};
	if(i < 1024)
		s = format_int(i);
	else if(i < 1024*1024)
		s = std::to_string(i/1024) + " KB" + (with_bytes ? bytes : "");
	else if(i < 1024*1024*1024)
		s = format_float(f/(1024*1024)) + " MB" + (with_bytes ? bytes : "");
	else s = format_float(f/(1024*1024*1024)) + " GB" + (with_bytes ? bytes : "");
	return s;
}

std::string util::GetLastErrorStr(bool inet)
{
	std::string str(4096, '\0');
	if(inet) str.resize(FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE, 
									   GetModuleHandleA("Wininet.dll"), GetLastError(), 0, &str.front(), 4096, 0));
	else str.resize(FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, &str.front(), 4096, 0));
	size_t pos {str.find_first_of("\r\n")};
	if(pos != -1)
		str.erase(pos);
	return str;
}

HWND util::hwnd_from_pid(DWORD pid)
{
	struct lparam_t { HWND hwnd {nullptr}; DWORD pid {0}; } lparam;

	WNDENUMPROC enumfn = [](HWND hwnd, LPARAM lparam) -> BOOL
	{
		auto &target {*reinterpret_cast<lparam_t*>(lparam)};
		DWORD pid {0};
		GetWindowThreadProcessId(hwnd, &pid);
		if(pid == target.pid)
		{
			target.hwnd = hwnd;
			return FALSE;
		}
		return TRUE;
	};

	lparam.pid = pid;
	EnumWindows(enumfn, reinterpret_cast<LPARAM>(&lparam));
	return lparam.hwnd;
}

std::string util::run_piped_process(std::wstring cmd, bool *working, nana::textbox *tb, callback cb)
{
	std::string ret;
	HANDLE hPipeRead, hPipeWrite;

	SECURITY_ATTRIBUTES sa {sizeof(SECURITY_ATTRIBUTES)};
	sa.bInheritHandle = TRUE;
	sa.lpSecurityDescriptor = NULL;

	if(!CreatePipe(&hPipeRead, &hPipeWrite, &sa, 0))
		return ret;

	STARTUPINFOW si {sizeof(STARTUPINFOW)};
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.hStdOutput = hPipeWrite;
	si.hStdError = hPipeWrite;
	si.wShowWindow = SW_HIDE;

	PROCESS_INFORMATION pi {0};

	BOOL res {CreateProcessW(NULL, &cmd.front(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)};
	if(!res)
	{
		CloseHandle(hPipeWrite);
		CloseHandle(hPipeRead);
		return ret;
	}

	auto killproc = [&pi]
	{
		auto hwnd {hwnd_from_pid(pi.dwProcessId)};
		if(hwnd) SendMessageA(hwnd, WM_CLOSE, 0, 0);
	};

	bool procexit {false};
	while(!procexit)
	{
		procexit = WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0;

		for(;;)
		{
			char buf[1024];
			DWORD dwRead = 0;
			DWORD dwAvail = 0;

			if(!::PeekNamedPipe(hPipeRead, NULL, 0, NULL, &dwAvail, NULL))
				break;

			if(!dwAvail)
				break;

			if(!::ReadFile(hPipeRead, buf, min(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
				break;

			if(working && !(*working))
			{
				killproc();
				break;
			}

			buf[dwRead] = 0;
			std::string s {buf};
			if(cb)
			{
				static bool subs {false};
				if(s.find("Writing video subtitles") != -1)
					subs = true;
				auto pos1 {s.find('%')};
				if(pos1 != -1)
				{
					if(!subs)
					{
						auto pos2 {s.rfind('%')}, pos {s.rfind(' ', pos2)+1};
						auto pct {s.substr(pos, pos2-pos)};
						auto pos1a {s.rfind('\n', pos1)+1}, pos2a {s.find('\n', pos2)};
						pos1 = s.rfind(' ', pos2)+1;
						auto text {s.substr(pos1, pos2a-pos1)};
						pos1 = text.rfind("  ");
						if(pos1 != -1) text.erase(pos1, 1);
						cb(static_cast<ULONGLONG>(stod(pct)*10), 1000, text);
						s.erase(pos1a, pos2a == -1 ? pos2a : pos2a-pos1a);
						if(s == "\n") s.clear();
						if(!s.empty() && s.back() == '\n' && s[s.size()-2] == '\n')
							s.pop_back();
					}
					else
					{
						if(s.find("100%") != -1)
							subs = false;
					}
				}
			}
			else ret += buf;
			if(tb) tb->append(s, false);
		}
		if(working && !*working)
		{
			killproc();
			break;
		}
	}

	CloseHandle(hPipeWrite);
	CloseHandle(hPipeRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return ret;
}

void util::end_processes(std::wstring img_name)
{
	HANDLE hSnapshot {CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)};
	if(hSnapshot)
	{
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);
		if(Process32First(hSnapshot, &pe32))
		{
			DWORD curpid {GetCurrentProcessId()};
			do
			{
				if(img_name == pe32.szExeFile)
				{
					if(pe32.th32ProcessID != curpid)
					{
						auto hwnd {hwnd_from_pid(pe32.th32ProcessID)};
						std::string wndtext(1024, '\0');
						wndtext.resize(GetWindowTextA(hwnd, &wndtext.front(), wndtext.size()-1));
						if(hwnd && wndtext.find("settings") == -1)
							SendMessageA(hwnd, WM_CLOSE, 0, 0);
						else
						{
							HANDLE hproc {OpenProcess(PROCESS_TERMINATE, 0, pe32.th32ProcessID)};
							if(hproc)
							{
								TerminateProcess(hproc, 0xDeadBeef);
								WaitForSingleObject(hproc, INFINITE);
								CloseHandle(hproc);
							}
						}
					}
				}
			} while(Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}

}

std::wstring util::get_sys_folder(REFKNOWNFOLDERID rfid)
{
	wchar_t *res {nullptr};
	std::wstring sres;
	if(SHGetKnownFolderPath(rfid, 0, 0, &res) == S_OK)
		sres = res;
	CoTaskMemFree(res);
	return sres;
}

std::string util::get_inet_res(std::string res, std::string *error)
{
	std::string ret;
	if(error) error->clear();

	/*auto get_inet_error = []
	{
		std::string str(4096, '\0');
		DWORD dwerr {0}, dwsize {4096};
		if(InternetGetLastResponseInfoA(&dwerr, &str.front(), &dwsize))
			str.resize(dwsize);
		else str.clear();
		return str;
	};*/

	auto hinet {InternetOpenA("Smith", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)};
	if(hinet)
	{
		auto hfile {InternetOpenUrlA(hinet, res.data(), NULL, 0, 0, 0)};
		if(hfile)
		{
			DWORD read {1};
			while(read)
			{
				std::string buf(4096, '\0');
				if(InternetReadFile(hfile, &buf.front(), buf.size(), &read))
				{
					if(read < buf.size())
						buf.resize(read);
					ret += buf;
				}
				else break;
			}
			InternetCloseHandle(hfile);
		}
		else if(error) *error = GetLastErrorStr(true);
		InternetCloseHandle(hinet);
	}
	else if(error) *error = GetLastErrorStr(true);
	return ret;
}

std::string util::dl_inet_res(std::string res, fs::path fname, bool *working, std::function<void(unsigned)> cb)
{
	std::string ret;
	std::ofstream f {fname, std::ios::binary};
	if(!f.good())
		return "Failed to open file for writing: " + fname.u8string();

	auto hinet {InternetOpenA("Smith", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)};
	auto hfile {InternetOpenUrlA(hinet, res.data(), NULL, 0, 0, 0)};
	DWORD read {1};

	while(read)
	{
		if(working && !*working) break;
		std::string buf(65535, '\0');
		if(InternetReadFile(hfile, &buf.front(), buf.size(), &read))
		{
			if(read < buf.size())
				buf.resize(read);
			f.write(buf.data(), buf.size());
			if(!f.good())
			{
				ret = "Failed writing to file: " + fname.u8string();
				break;
			}
			if(cb) cb(buf.size());
		}
		else break;
	}

	InternetCloseHandle(hinet);
	InternetCloseHandle(hfile);
	return ret;
}

std::string util::extract_7z(std::filesystem::path arc_path, std::filesystem::path out_path)
{
	using namespace bit7z;

	std::wstring modpath(4096, '\0');
	modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));
	std::filesystem::path lib_path {modpath};
	lib_path.replace_filename("7zxa.dll");

	try
	{
		Bit7zLibrary lib {lib_path};
		BitExtractor extractor {lib, BitFormat::SevenZip};
		extractor.extract(arc_path, out_path);
		return "";
	}
	catch(const BitException& ex)
	{
		return ex.what();
	}
}
