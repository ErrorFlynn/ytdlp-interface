#include "gui.hpp"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
#ifdef _DEBUG
	AllocConsole();
	freopen("conout$", "w", stdout);
	SetConsoleOutputCP(CP_UTF8);
#endif

	using namespace nana;
	std::setlocale(LC_ALL, "en_US.UTF-8");
	int argc;
	LPWSTR *argv {CommandLineToArgvW(GetCommandLineW(), &argc)};
	fs::path modpath {argv[0]}, appdir {modpath.parent_path()};
	const std::string ytdlp_fname {INTPTR_MAX == INT64_MAX ? "yt-dlp.exe" : "yt-dlp_x86.exe"};
	std::error_code ec;
	if(fs::exists(appdir / "7zxa.dll"))
		fs::remove(appdir / "7zxa.dll", ec);

	if(argc > 1)
	{
		const std::wstring first_arg {argv[1]};
		if(first_arg == L"update")
		{
			fs::path arc_path {argv[2]}, target_dir {argv[3]};
			util::end_processes(modpath.filename());
			auto res {util::extract_7z(arc_path, target_dir)};
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
	}
	LocalFree(argv);
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	paint::image img {modpath};
	API::window_icon_default(img, img);

	if(fs::exists(appdir / "ytdl-patched-red.exe"))
		GUI::conf.ytdlp_path = appdir / "ytdl-patched-red.exe";
	else if(fs::exists(appdir / ytdlp_fname))
		GUI::conf.ytdlp_path = appdir / ytdlp_fname;

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
			(mbox << "an exception occured when trying to load the settings file: " << e.what())();
		}
		if(!jconf.empty())
		{
			GUI::conf.ytdlp_path = std::string {jconf["ytdlp_path"]};
			if(GUI::conf.ytdlp_path.empty() || !fs::exists(GUI::conf.ytdlp_path))
			{
				if(fs::exists(appdir / "ytdl-patched-red.exe"))
					GUI::conf.ytdlp_path = appdir / "ytdl-patched-red.exe";
				else if(fs::exists(appdir / ytdlp_fname))
					GUI::conf.ytdlp_path = appdir / ytdlp_fname;
			}
			GUI::conf.outpath = std::string {jconf["outpath"]};
			GUI::conf.fmt1 = to_wstring(std::string {jconf["fmt1"]});
			GUI::conf.fmt2 = to_wstring(std::string {jconf["fmt2"]});
			GUI::conf.ratelim = jconf["ratelim"];
			GUI::conf.ratelim_unit = jconf["ratelim_unit"];
			if(jconf.contains("cbsplit")) // v1.1
			{
				GUI::conf.cbsplit = jconf["cbsplit"];
				GUI::conf.cbchaps = jconf["cbchaps"];
				GUI::conf.cbsubs = jconf["cbsubs"];
				GUI::conf.cbthumb = jconf["cbthumb"];
				GUI::conf.cbtime = jconf["cbtime"];
				GUI::conf.cbkeyframes = jconf["cbkeyframes"];
				GUI::conf.cbmp3 = jconf["cbmp3"];
			}
			if(jconf.contains("pref_res")) // v1.2
			{
				GUI::conf.pref_res = jconf["pref_res"];
				GUI::conf.pref_video = jconf["pref_video"];
				GUI::conf.pref_audio = jconf["pref_audio"];
				GUI::conf.pref_fps = jconf["pref_fps"];
			}
			if(jconf.contains("cbtheme")) // v1.3
				GUI::conf.cbtheme = jconf["cbtheme"];
			if(jconf.contains("contrast")) // v1.4
			{
				GUI::conf.contrast = jconf["contrast"];
				GUI::conf.cbargs = jconf["cbargs"];
				if(jconf.contains("args"))
				{
					GUI::conf.argsets.push_back(jconf["args"]);
					jconf.erase("args"); // no longer used since v1.5
				}
			}
			if(jconf.contains("kwhilite")) // v1.5
			{
				GUI::conf.kwhilite = jconf["kwhilite"];
				GUI::conf.com_args = jconf["com_args"];
				for(auto &el : jconf["argsets"])
					GUI::conf.argsets.push_back(el);
				if(jconf.contains("vidinfo"))
					jconf.erase("vidinfo"); // no longer used since v1.6
			}
			if(jconf.contains("output_template")) // v1.6
			{
				GUI::conf.output_template = to_wstring(std::string {jconf["output_template"]});
				GUI::conf.max_concurrent_downloads = jconf["max_concurrent_downloads"];
				GUI::conf.cb_lengthyproc = jconf["cb_lengthyproc"];
				GUI::conf.max_proc_dur = std::chrono::milliseconds {static_cast<int>(jconf["max_proc_dur"])};
				for(auto &el : jconf["unfinished_queue_items"])
					GUI::conf.unfinished_queue_items.push_back(el);
				for(auto &el : jconf["outpaths"])
					GUI::conf.outpaths.insert(to_wstring(std::string {el}));
				GUI::conf.common_dl_options = jconf["common_dl_options"];
				GUI::conf.cb_autostart = jconf["cb_autostart"];
			}
			else
			{
				GUI::conf.output_template = GUI::conf.output_template_default;
				GUI::conf.outpaths.insert(GUI::conf.outpath);
			}
			if(jconf.contains("cb_queue_autostart")) // v1.6.1
			{
				GUI::conf.cb_queue_autostart = jconf["cb_queue_autostart"];
			}
			if(jconf.contains("gpopt_hidden")) // v1.7
			{
				GUI::conf.gpopt_hidden = jconf["gpopt_hidden"];
				GUI::conf.open_dialog_origin = jconf["open_dialog_origin"];
				GUI::conf.playlist_indexing = to_wstring(std::string {jconf["playlist_indexing"]});
			}
			if(jconf.contains("cb_zeropadding")) // v1.8
			{
				GUI::conf.cb_zeropadding = jconf["cb_zeropadding"];
				GUI::conf.cb_playlist_folder = jconf["cb_playlist_folder"];
				GUI::conf.winrect.x = jconf["window"]["x"];
				GUI::conf.winrect.y = jconf["window"]["y"];
				GUI::conf.winrect.width = jconf["window"]["w"];
				GUI::conf.winrect.height = jconf["window"]["h"];
				GUI::conf.zoomed = jconf["window"]["zoomed"];
				if(jconf["window"].contains("dpi"))
					GUI::conf.dpi = jconf["window"]["dpi"];
				else GUI::conf.dpi = API::screen_dpi(true);
			}
		}
	}
	else
	{
		GUI::conf.outpath = util::get_sys_folder(FOLDERID_Downloads);
	}

	GUI gui;
	gui.events().unload([&]
	{
		jconf["ytdlp_path"] = GUI::conf.ytdlp_path;
		jconf["outpath"] = GUI::conf.outpath;
		jconf["fmt1"] = to_utf8(GUI::conf.fmt1);
		jconf["fmt2"] = to_utf8(GUI::conf.fmt2);
		jconf["ratelim"] = GUI::conf.ratelim;
		jconf["ratelim_unit"] = GUI::conf.ratelim_unit;
		jconf["cbsplit"] = GUI::conf.cbsplit;
		jconf["cbchaps"] = GUI::conf.cbchaps;
		jconf["cbsubs"] = GUI::conf.cbsubs;
		jconf["cbthumb"] = GUI::conf.cbthumb;
		jconf["cbtime"] = GUI::conf.cbtime;
		jconf["cbkeyframes"] = GUI::conf.cbkeyframes;
		jconf["cbmp3"] = GUI::conf.cbmp3;
		jconf["pref_res"] = GUI::conf.pref_res;
		jconf["pref_video"] = GUI::conf.pref_video;
		jconf["pref_audio"] = GUI::conf.pref_audio;
		jconf["pref_fps"] = GUI::conf.pref_fps;
		jconf["cbtheme"] = GUI::conf.cbtheme;
		jconf["contrast"] = GUI::conf.contrast;
		jconf["cbargs"] = GUI::conf.cbargs;
		jconf["kwhilite"] = GUI::conf.kwhilite;
		jconf["argsets"] = GUI::conf.argsets;
		jconf["com_args"] = GUI::conf.com_args;
		jconf["output_template"] = to_utf8(GUI::conf.output_template);
		jconf["max_concurrent_downloads"] = GUI::conf.max_concurrent_downloads;
		jconf["cb_lengthyproc"] = GUI::conf.cb_lengthyproc;
		jconf["max_proc_dur"] = GUI::conf.max_proc_dur.count();
		jconf["unfinished_queue_items"] = GUI::conf.unfinished_queue_items;
		jconf["common_dl_options"] = GUI::conf.common_dl_options;
		jconf["cb_autostart"] = GUI::conf.cb_autostart;
		jconf["cb_queue_autostart"] = GUI::conf.cb_queue_autostart;
		jconf["gpopt_hidden"] = GUI::conf.gpopt_hidden;
		jconf["open_dialog_origin"] = GUI::conf.open_dialog_origin;
		jconf["playlist_indexing"] = to_utf8(GUI::conf.playlist_indexing);
		jconf["cb_zeropadding"] = GUI::conf.cb_zeropadding;
		jconf["cb_playlist_folder"] = GUI::conf.cb_playlist_folder;
		jconf["window"]["x"] = GUI::conf.winrect.x;
		jconf["window"]["y"] = GUI::conf.winrect.y;
		jconf["window"]["w"] = GUI::conf.winrect.width;
		jconf["window"]["h"] = GUI::conf.winrect.height;
		jconf["window"]["zoomed"] = GUI::conf.zoomed;
		jconf["window"]["dpi"] = API::screen_dpi(true);
		if(jconf.contains("outpaths"))
			jconf["outpaths"].clear();
		for(auto &path : GUI::conf.outpaths)
			jconf["outpaths"].push_back(to_utf8(path));
		std::ofstream {confpath} << std::setw(4) << jconf;
	});
	nana::exec();
	CoUninitialize();
}