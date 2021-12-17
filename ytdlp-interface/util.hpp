#pragma once

#include "json.hpp"
#include <Windows.h>
#include <Shlobj.h>

#include <string>
#include <fstream>
#include <sstream>

#include <nana/gui/widgets/textbox.hpp>


struct settings_t
{
	std::filesystem::path ytdlp_path, outpath;
	std::wstring fmt1, fmt2;
	double ratelim {0};
	unsigned ratelim_unit {1};
	bool cbsplit {false}, cbchaps {false}, cbsubs {false}, cbthumb {false}, cbtime {true}, cbkeyframes {false}, cbmp3 {false};
}
conf;

template<typename CharT>
struct Sep : public std::numpunct<CharT>
{
	virtual std::string do_grouping() const { return "\003"; }
};

auto format_int(unsigned i)
{
	std::stringstream ss;
	ss.imbue(std::locale {ss.getloc(), new Sep<char>{}});
	ss << i;
	return ss.str();
}

auto format_float(float f, unsigned precision = 2)
{
	std::stringstream ss;
	ss << std::fixed << std::setprecision(precision) << f;
	return ss.str();
}

auto GetLastErrorStr()
{
	std::string str(4096, '\0');
	str.resize(FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, &str.front(), 4096, 0));
	size_t pos {str.find_first_of("\r\n")};
	if(pos != -1)
		str.erase(pos);
	return str;
}

HWND hwnd_from_pid(DWORD pid)
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

auto run_piped_process(std::wstring cmd, bool *working = nullptr, nana::textbox *tb = nullptr, 
	std::function<void(ULONGLONG, ULONGLONG, std::string)> cb = nullptr)
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

auto get_sys_folder(REFKNOWNFOLDERID rfid)
{
	wchar_t *res {nullptr};
	std::wstring sres;
	if(SHGetKnownFolderPath(rfid, 0, 0, &res) == S_OK)
		sres = res;
	CoTaskMemFree(res);
	return sres;
}