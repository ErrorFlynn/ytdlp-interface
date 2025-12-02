#include "gui.hpp"
#pragma warning (disable: 4244)


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
#ifdef _DEBUG
	g_enable_log = true;
#endif
	/*AllocConsole();
	freopen("conout$", "w", stdout);
	SetConsoleOutputCP(CP_UTF8);*/


	using namespace nana;
	std::setlocale(LC_ALL, "en_US.UTF-8");
	int argc;
	LPWSTR *argv {CommandLineToArgvW(GetCommandLineW(), &argc)};
	fs::path modpath {argv[0]}, appdir {modpath.parent_path()};
	fs::current_path(appdir);
	std::error_code ec;
	if(fs::exists(appdir / "7zxa.dll"))
		fs::remove(appdir / "7zxa.dll", ec);

	if(argc > 1)
	{
		const std::wstring first_arg {argv[1]};
		if(first_arg == L"debug_log")
		{
			g_enable_log = true;
		}
		else if(first_arg == L"update")
		{
			const fs::path arc_path {argv[2]}, target_dir {argv[3]};
			const bool self_only {argc == 5 && std::wstring {argv[4]} == L"self_only"};
			const auto pid {util::other_instance(target_dir / modpath.filename())};
			if(pid)
			{
				auto hwnds {util::hwnds_from_pid(pid)};
				for(auto hwnd : hwnds)
					if(IsWindow(hwnd))
						SendMessage(hwnd, WM_CLOSE, 0, 0);
			}
			auto res {util::extract_7z(arc_path, target_dir, 0, self_only)};
			fs::remove(arc_path, ec);
			fs::remove(fs::temp_directory_path() / "7z.dll", ec);
			if(!res.empty())
			{
				msgbox mbox {"ytdlp-interface - updating failed"};
				mbox.icon(msgbox::icon_error);
				(mbox << "failed to extract the downloaded 7z archive: " << res)();
			}
			else
			{
				auto newself {target_dir / "ytdlp-interface.exe"};
				ShellExecuteW(NULL, L"open", newself.wstring().data(), L"cleanup", NULL, SW_SHOW);
			}
			LocalFree(argv);
			return 0;
		}
		else if(first_arg == L"cleanup")
		{
			fs::remove(fs::temp_directory_path() / "ytdlp-interface.exe", ec);
		}
		else if(first_arg == L"ytdlp_status")
		{
			if(argc > 3)
			{
				auto hwnd {reinterpret_cast<HWND>(std::stoull(argv[3], 0, 16))};
				std::wstring url {argv[4]};
				COPYDATASTRUCT cds;
				cds.dwData = std::stoi(argv[2]);
				cds.cbData = url.size()*2;
				cds.lpData = url.data();
				SendMessageW(hwnd, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
			}
			return 0;
		}
		else GUI::conf.url_passed_as_arg = argv[1];
	}
	LocalFree(argv);

	auto pid {util::other_instance()};
	if(pid)
	{
		auto hwnds {util::hwnds_from_pid(pid)};
		for(auto hwnd : hwnds)
		{
			if(IsIconic(hwnd))
				ShowWindow(hwnd, SW_RESTORE);
			SetForegroundWindow(hwnd);
			BringWindowToTop(hwnd);
			if(!GUI::conf.url_passed_as_arg.empty())
			{
				COPYDATASTRUCT cds;
				cds.dwData = ADD_URL;
				cds.cbData = GUI::conf.url_passed_as_arg.size() * 2;
				cds.lpData = GUI::conf.url_passed_as_arg.data();
				SendMessageW(hwnd, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
			}
			break;
		}
		return 0;
	}

	void(OleInitialize(0));
	g_exiting = false;
	if(g_enable_log)
	{
		g_log.use_window = true;
		g_logfile.open(appdir / "debug_log.txt", std::ofstream::trunc);
		if(!g_logfile.good())
		{
			msgbox mbox {"failed to open debug_log.txt for writing"};
			mbox.icon(msgbox::icon_error);
			(mbox << std::strerror(errno))();
		}
		else g_logfile << "log file open\n";
	}
	

	paint::image img {modpath};
	API::window_icon_default(img, img);

	if(fs::exists(appdir / "ytdl-patched-red.exe"))
		GUI::conf.ytdlp_path = ".\\ytdl-patched-red.exe";
	else if(fs::exists(appdir / ytdlp_fname))
		GUI::conf.ytdlp_path = ".\\" + ytdlp_fname;

	fs::path confpath {util::get_sys_folder(FOLDERID_RoamingAppData) + L"\\ytdlp-interface.json"};
	auto appdir_confpath {appdir / "ytdlp-interface.json"};
	if(util::is_dir_writable(appdir))
	{
		if(fs::exists(confpath) && !fs::exists(appdir_confpath))
		{
			fs::rename(confpath, appdir_confpath, ec);
			if(ec) fs::copy_file(confpath, appdir_confpath, ec);
		}
		confpath = appdir_confpath;
	}
	else if(fs::exists(appdir_confpath) && !fs::exists(confpath))
		fs::copy_file(appdir_confpath, confpath, ec);

	nlohmann::json jconf;
	if(fs::exists(confpath))
	{
		std::ifstream f {confpath};
		try { f >> jconf; }
		catch(nlohmann::detail::exception e)
		{
			msgbox mbox {"ytdlp-interface JSON error"};
			mbox.icon(msgbox::icon_error);
			(mbox << confpath.string() << "\n\nAn exception occured when trying to load the settings file:\n\n" << e.what())();
		}
		if(!jconf.empty())
			GUI::conf.from_json(jconf);
	}
	else GUI::conf.outpath = util::get_sys_folder(FOLDERID_Downloads);

	jconf.clear();

	GUI gui;
	gui.confpath = confpath;
	gui.infopath = fs::path{confpath}.replace_filename("unfinished_qitems_data.json");

	gui.fn_write_conf = [&confpath]
	{
		nlohmann::json jconf;
		GUI::conf.to_json(jconf);
		return (std::ofstream {confpath} << std::setw(4) << jconf).good();
	};

	gui.events().unload([&]
	{
		if(!gui.fn_write_conf() && errno)
		{
			std::string error {std::strerror(errno)};
			msgbox mbox {"ytdlp-interface - error writing settings file"};
			mbox.icon(msgbox::icon_error);
			(mbox << confpath.string() << "\n\nAn error occured when trying to save the settings file:\n\n" << error)();
		}
	});
	nana::exec();
	CoUninitialize();
}