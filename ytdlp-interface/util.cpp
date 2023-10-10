#include "util.hpp"
#include "bitextractor.hpp"
#include "bitexception.hpp"

#include <WinInet.h>
#include <TlHelp32.h>
#include <iostream>
#include <codecvt>

#pragma warning (disable: 4244)

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

std::vector<HWND> util::hwnds_from_pid(DWORD pid)
{
	struct lparam_t { std::vector<HWND> hwnds; DWORD pid {0}; } lparam;

	WNDENUMPROC enumfn = [](HWND hwnd, LPARAM lparam) -> BOOL
	{
		auto &target {*reinterpret_cast<lparam_t*>(lparam)};
		DWORD pid {0};
		GetWindowThreadProcessId(hwnd, &pid);
		if(pid == target.pid && IsWindowVisible(hwnd))
			target.hwnds.push_back(hwnd);
		return TRUE;
	};

	lparam.pid = pid;
	EnumWindows(enumfn, reinterpret_cast<LPARAM>(&lparam));
	return lparam.hwnds;
}

std::string util::run_piped_process(std::wstring cmd, bool *working, append_callback cbappend, progress_callback cbprog, bool *graceful_exit, std::string suppress)
{
	std::wstring modpath(4096, '\0');
	modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));

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

	BOOL res {CreateProcessW(NULL, &cmd.front(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE|CREATE_NEW_PROCESS_GROUP, NULL, NULL, &si, &pi)};
	if(!res)
	{
		CloseHandle(hPipeWrite);
		CloseHandle(hPipeRead);
		return ret;
	}

	auto killproc = [&]
	{
		if(AttachConsole(pi.dwProcessId))
		{
			auto res {GenerateConsoleCtrlEvent(CTRL_C_EVENT, pi.dwProcessId)};
			FreeConsole();
			if(res && WaitForSingleObject(pi.hProcess, 6000) == WAIT_OBJECT_0)
			{
				if(graceful_exit)
					*graceful_exit = true;
				return;
			}
		}
		auto hwnd {hwnd_from_pid(pi.dwProcessId)};
		if(hwnd) SendMessageA(hwnd, WM_CLOSE, 0, 0);
		if(graceful_exit)
			*graceful_exit = false;
	};
	std::string playlist_line;
	bool procexit {false}, subs {false};
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
			std::string s;
			if(cbprog)
			{
				if(std::string(buf).find("Writing video subtitles") != -1)
					subs = true;
				std::string line, strbuf {buf};
				if(!nana::is_utf8(strbuf))
				{
					std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> u16conv;
					auto u16str {u16conv.from_bytes(strbuf)};
					std::wstring wstr(u16str.size(), L'\0');
					memcpy(&wstr.front(), &u16str.front(), wstr.size() * 2);
					std::wstring_convert<std::codecvt_utf8<wchar_t>> u8conv;
					strbuf = u8conv.to_bytes(wstr);
				}
				size_t lnstart {0}, lnend {0};
				do
				{
					line.clear();
					if(lnend != -1)
					{
						lnend = strbuf.find_first_of("\r\n", lnstart);
						if(lnend != 1)
							line = strbuf.substr(lnstart, lnend - lnstart);
						else line = strbuf.substr(lnstart);
						if(line.size()) 
							line += '\n';
						if(lnend + 1 == strbuf.size())
							lnend = -1;
						else lnstart = lnend + 1;
					}
					if(!line.empty() && (suppress.empty() || line.find(suppress) == -1))
					{
						if(cbappend && *working && line[0] == '[')
						{
							auto pos {line.find(']')};
							if(pos != -1)
							{
								auto text {line.substr(0, pos + 1)};
								if(text != "[download]" && text[1] != '#')
									if(*working)
										cbappend(text, true);
								if(text == "[Exec]" && text.find("ytdlp_status") != 1)
									continue;
								if(text == "[ExtractAudio]" || text.starts_with("[Fixup") || text == "[Merger]")
									cbprog(-1, -1, text, 0, 0);
								else if(!suppress.empty())
									if(text == "[download]" && (line.find("Destination:") == 11 || line.find("has already been downloaded") != -1))
										cbprog(-1, -1, line, 0, 0);
							}
						}
						const bool line_starts_with_download {line.starts_with("[download]")},
						           playlist_progress {line_starts_with_download && line.find(" Downloading item ") == 10};
						if(playlist_progress)
							playlist_line = line;
						auto pos {line.find('%')};
						if(pos != -1 && line_starts_with_download)
						{
							if(subs)
							{
								s += line;
								if(line.find("100%") != -1)
									subs = false;
							}
							else
							{
								auto pos2 {line.rfind(' ', pos) + 1};
								if(pos2 != -1)
								{
									try {
										auto percent {std::stod(line.substr(pos2, pos - pos2))};
										line.pop_back();
										if(*working)
										{
											int playlist_complete {1}, playlist_total {0};
											pos = playlist_line.find(" of ", 28);
											if(pos != -1)
											{
												playlist_complete = std::stod(playlist_line.substr(28, pos - 28));
												playlist_total = std::stod(playlist_line.substr(pos + 4, playlist_line.size() - pos - 1));
											}
											if(percent != 100 || line.find(" in ") != -1)
												cbprog(static_cast<ULONGLONG>(percent * 10), 1000, line.substr(pos2), playlist_complete-1, playlist_total);
										}
									} catch(...) {}
								}
							}
						}
						else if(pos != -1 && line.starts_with("[#") && line[line.size()-2] == ']')
						{
							auto pos2 {line.rfind('(', pos) + 1};
							auto strpct {line.substr(pos2, pos - pos2)};
							auto pos3 {line.find("ETA:")};
							std::string eta;
							if(pos3 != -1)
								eta = line.substr(pos3, line.size() - 2 - pos3);
							std::string text {strpct + '%'};
							if(!eta.empty()) text += " " + eta;
							if(*working)
								cbprog(static_cast<ULONGLONG>(std::stod(strpct) * 10), 1000, text, 0, 0);
							s += line;
						}
						else s += line;
					}
				} while(lnend != -1);
			}
			else ret += buf;
			if(cbappend && *working)
				cbappend(s, false);
		}
		if(working && !*working)
		{
			DWORD exit_code {0};
			GetExitCodeProcess(pi.hProcess, &exit_code);
			if(exit_code == STILL_ACTIVE)
				killproc();
			break;
		}
	}

	DWORD exit_code {0};
	GetExitCodeProcess(pi.hProcess, &exit_code);
	if(exit_code == STILL_ACTIVE)
	{
		killproc();
	}

	CloseHandle(hPipeWrite);
	CloseHandle(hPipeRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return ret;
}

DWORD util::other_instance(std::wstring path)
{
	std::wstring modpath(4096, '\0');
	if(path.empty())
		modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));
	else modpath = path;
	auto img_name {fs::path {modpath}.filename().wstring()};

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
					auto hproc {OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pe32.th32ProcessID)};
					if(hproc)
					{
						DWORD bufsize {8192};
						std::wstring buf(bufsize, '\0');
						if(QueryFullProcessImageName(hproc, 0, &buf.front(), &bufsize))
						{
							buf.resize(bufsize);
							if(modpath == buf && pe32.th32ProcessID != curpid)
							{
								CloseHandle(hproc);
								CloseHandle(hSnapshot);
								return pe32.th32ProcessID;
							}
						}
						CloseHandle(hproc);
					}
				}
			} while(Process32Next(hSnapshot, &pe32));
		}
		CloseHandle(hSnapshot);
	}
	return 0;
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

	const char *agent {res.find("github.com") == -1 ? "Mozilla/5.0 (Windows NT 10.0; Win64; x64)" : "ytdlp-interface"};
	auto hinet {InternetOpenA(agent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)};
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
				else
				{
					if(error) *error = GetLastErrorStr(true);
					break;
				}
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
		return "Failed to open file for writing: " + fname.string();

	auto hinet {InternetOpenA("ytdlp-interface", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0)};
	if(hinet)
	{
		auto hfile {InternetOpenUrlA(hinet, res.data(), NULL, 0, 0, 0)};
		if(hfile)
		{
			DWORD read {1};

			while(read)
			{
				if(working && !*working)
				{
					f.close();
					fs::remove(fname);
					break;
				}
				std::string buf(65535, '\0');
				if(InternetReadFile(hfile, &buf.front(), buf.size(), &read))
				{
					if(read < buf.size())
						buf.resize(read);
					f.write(buf.data(), buf.size());
					if(!f.good())
					{
						ret = "Failed writing to file: " + fname.string();
						break;
					}
					if(cb)
					{
						if(working)
						{
							if(*working)
								cb(buf.size());
						}
						else cb(buf.size());
					}
				}
				else
				{
					ret = GetLastErrorStr(true);
					break;
				}
			}
			InternetCloseHandle(hfile);
		}
		else ret = GetLastErrorStr(true);
		InternetCloseHandle(hinet);
	}
	else ret = GetLastErrorStr(true);

	return ret;
}

std::string util::extract_7z(fs::path arc_path, fs::path out_path, bool ffmpeg, bool ytdlp_interface)
{
	using namespace bit7z;

	std::wstring modpath(4096, '\0');
	modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));
	fs::path lib_path {modpath};
	lib_path.replace_filename("7z.dll");

	try
	{
		Bit7zLibrary lib {lib_path};
		BitExtractor extractor {lib, ffmpeg ? BitFormat::Zip : BitFormat::SevenZip};
		if(ffmpeg)
		{
			extractor.extractMatching(arc_path, L"*\\bin\\ffmpeg.exe", out_path);
			extractor.extractMatching(arc_path, L"*\\bin\\ffprobe.exe", out_path);
		}
		else if(ytdlp_interface)
		{
			extractor.extractMatching(arc_path, L"ytdlp-interface.exe", out_path);
		}
		else extractor.extract(arc_path, out_path);
		return "";
	}
	catch(const BitException& ex)
	{
		return ex.what();
	}
}

std::wstring util::get_clipboard_text()
{
	std::wstring text;
	if(OpenClipboard(NULL))
	{
		if(IsClipboardFormatAvailable(CF_UNICODETEXT))
		{
			auto h {GetClipboardData(CF_UNICODETEXT)};
			if(h)
			{
				auto ptext {static_cast<wchar_t*>(GlobalLock(h))};
				if(ptext)
				{
					text = ptext;
					GlobalUnlock(h);
				}
			}
		}
		CloseClipboard();
	}
	return text;
}

void util::set_clipboard_text(HWND hwnd, std::wstring text)
{
	if(!text.empty() && OpenClipboard(hwnd))
	{
		if(EmptyClipboard())
		{
			HGLOBAL hmem {GlobalAlloc(GMEM_MOVEABLE, text.size()*2+2)};
			if(hmem)
			{
				auto ptext {static_cast<wchar_t*>(GlobalLock(hmem))};
				if(ptext)
				{
					wmemcpy(ptext, text.data(), text.size());
					GlobalUnlock(hmem);
					if(!SetClipboardData(CF_UNICODETEXT, hmem))
						GlobalFree(hmem);
				}
				else GlobalFree(hmem);
			}
		}
		CloseClipboard();
	}
}

bool util::is_dir_writable(fs::path dir)
{
	if(std::ofstream {dir / "write_test.out"}.good())
	{
		fs::remove(dir / "write_test.out");
		return true;
	}
	return false;
}

int util::scale(int val)
{
	const static double dpi {static_cast<double>(nana::API::screen_dpi(true))};
	if(dpi != 96)
		val = round(val * dpi / 96);
	return val;
};

unsigned util::scale_uint(unsigned val)
{
	const static double dpi {static_cast<double>(nana::API::screen_dpi(true))};
	if(dpi != 96)
		val = round(val * dpi / 96);
	return val;
}