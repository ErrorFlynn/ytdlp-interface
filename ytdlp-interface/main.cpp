#include "gui.hpp"
#pragma warning (disable: 4244)


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

	OleInitialize(0);
	

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
			(mbox << confpath.string() << "\n\nAn exception occured when trying to load the settings file:\n\n" << e.what())();
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
			GUI::conf.outpath = jconf["outpath"].get<std::string>();
			GUI::conf.fmt1 = to_wstring(jconf["fmt1"].get<std::string>());
			GUI::conf.fmt2 = to_wstring(jconf["fmt2"].get<std::string>());
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
				if(!jconf.contains("json_hide_null")) // v2.2 added two options at the top
					GUI::conf.pref_res = jconf["pref_res"] + 2;
				else GUI::conf.pref_res = jconf["pref_res"];
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
				for(auto &el : jconf["argsets"])
					GUI::conf.argsets.push_back(el);
				if(jconf.contains("vidinfo"))
					jconf.erase("vidinfo"); // no longer used since v1.6
			}
			if(jconf.contains("output_template")) // v1.6
			{
				GUI::conf.output_template = to_wstring(jconf["output_template"].get<std::string>());
				GUI::conf.max_concurrent_downloads = jconf["max_concurrent_downloads"];
				GUI::conf.cb_lengthyproc = jconf["cb_lengthyproc"];
				GUI::conf.max_proc_dur = std::chrono::milliseconds {jconf["max_proc_dur"].get<int>()};
				for(auto &el : jconf["unfinished_queue_items"])
					GUI::conf.unfinished_queue_items.push_back(el);
				for(auto &el : jconf["outpaths"])
					GUI::conf.outpaths.insert(to_wstring(el.get<std::string>()));
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
			if(jconf.contains("open_dialog_origin")) // v1.7
			{
				GUI::conf.gpopt_hidden = jconf["gpopt_hidden"];
				GUI::conf.open_dialog_origin = jconf["open_dialog_origin"];
				GUI::conf.playlist_indexing = to_wstring(jconf["playlist_indexing"].get<std::string>());
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
			if(jconf.contains("get_releases_at_startup")) // v2.0
			{
				GUI::conf.get_releases_at_startup = jconf["get_releases_at_startup"];
				GUI::conf.col_format = jconf["queue_columns"]["format"];
				GUI::conf.col_format_note = jconf["queue_columns"]["format_note"];
				GUI::conf.col_ext = jconf["queue_columns"]["ext"];
				GUI::conf.col_fsize = jconf["queue_columns"]["fsize"];
			}
			if(jconf.contains("output_template_bandcamp")) // v2.1
			{
				GUI::conf.output_template_bandcamp = to_wstring(jconf["output_template_bandcamp"].get<std::string>());
			}
			if(jconf.contains("json_hide_null")) // v2.2
			{
				GUI::conf.json_hide_null = jconf["json_hide_null"];
			}
			if(jconf.contains("ytdlp_nightly")) // v2.3
			{
				GUI::conf.col_site_icon = jconf["queue_columns"]["website_icon"];
				GUI::conf.col_site_text = jconf["queue_columns"]["website_text"];
				GUI::conf.ytdlp_nightly = jconf["ytdlp_nightly"];
				GUI::conf.audio_multistreams = jconf["audio_multistreams"];
			}
			if(jconf.contains("sblock")) // v2.4
			{
				for(auto &el : jconf["sblock"]["mark"])
					GUI::conf.sblock_mark.push_back(el.get<int>());
				for(auto &el : jconf["sblock"]["remove"])
					GUI::conf.sblock_remove.push_back(el.get<int>());
				GUI::conf.cb_sblock_mark = jconf["sblock"]["cb_mark"];
				GUI::conf.cb_sblock_remove = jconf["sblock"]["cb_remove"];
				GUI::conf.cb_proxy = jconf["proxy"]["enabled"];
				GUI::conf.proxy = to_wstring(jconf["proxy"]["URL"].get<std::string>());
			}
			if(jconf.contains("cbsnap")) // v2.5
			{
				GUI::conf.cbsnap = jconf["cbsnap"];
			}
			if(jconf.contains("limit_output_buffer")) // v2.6
			{
				GUI::conf.limit_output_buffer = jconf["limit_output_buffer"];
				GUI::conf.output_buffer_size = jconf["output_buffer_size"];
			}
			if(jconf.contains("update_self_only")) // v2.7
			{
				GUI::conf.update_self_only = jconf["update_self_only"];
				GUI::conf.pref_vcodec = jconf["pref_vcodec"];
				GUI::conf.pref_acodec = jconf["pref_acodec"];
				GUI::conf.argset = jconf["argset"];
			}
			if(jconf.contains("cb_premium")) // v2.8
			{
				GUI::conf.cb_premium = jconf["cb_premium"];
				GUI::conf.cbminw = jconf["cbminw"];
				GUI::conf.cb_save_errors = jconf["cb_save_errors"];
				GUI::conf.cb_ffplay = jconf["cb_ffplay"];
			}
			if(jconf.contains("ffmpeg_path")) // v2.9
			{
				GUI::conf.ffmpeg_path = jconf["ffmpeg_path"].get<std::string>();
			}
			if(jconf.contains("cb_clear_done")) // v2.11
			{
				GUI::conf.cb_clear_done = jconf["cb_clear_done"];
				GUI::conf.cb_formats_fsize_bytes = jconf["cb_formats_fsize_bytes"];
				GUI::conf.cb_add_on_focus = jconf["cb_add_on_focus"];
			}
			if(jconf.contains("theme")) // v2.13
			{
				GUI::conf.cb_custom_dark_theme = jconf["cb_custom_dark_theme"];
				GUI::conf.cb_custom_light_theme = jconf["cb_custom_light_theme"];
				GUI::conf.theme_dark.from_json(jconf["theme"]["dark"]);
				GUI::conf.theme_dark.contrast(GUI::conf.contrast, true);
				GUI::conf.theme_light.from_json(jconf["theme"]["light"]);
				GUI::conf.theme_light.contrast(GUI::conf.contrast, false);
			}
			if(jconf.contains("cb_android")) // v2.14
			{
				GUI::conf.cb_android = jconf["cb_android"];
			}
		}
	}
	else GUI::conf.outpath = util::get_sys_folder(FOLDERID_Downloads);

	GUI gui;
	gui.confpath = confpath;

	if(jconf.contains("playsel_strings"))
	{
		for(auto &el : jconf["unfinished_queue_items"])
		{
			const std::string url {el};
			const auto wurl {to_wstring(url)};
			auto &bot {gui.botref().at(wurl)};
			if(jconf["playsel_strings"].contains(url))
				bot.playsel_string = to_wstring(jconf["playsel_strings"][url].get<std::string>());
		}
	}

	gui.fn_write_conf = [&]
	{
		static bool busy {false};
		if(busy) return true;

		busy = true;
		jconf["cb_custom_dark_theme"] = GUI::conf.cb_custom_dark_theme;
		jconf["cb_custom_light_theme"] = GUI::conf.cb_custom_light_theme;
		GUI::conf.theme_dark.to_json(jconf["theme"]["dark"]);
		GUI::conf.theme_light.to_json(jconf["theme"]["light"]);

		jconf["cb_android"] = GUI::conf.cb_android;
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
		jconf["get_releases_at_startup"] = GUI::conf.get_releases_at_startup;
		jconf["queue_columns"]["format"] = GUI::conf.col_format;
		jconf["queue_columns"]["format_note"] = GUI::conf.col_format_note;
		jconf["queue_columns"]["ext"] = GUI::conf.col_ext;
		jconf["queue_columns"]["fsize"] = GUI::conf.col_fsize;
		jconf["queue_columns"]["website_icon"] = GUI::conf.col_site_icon;
		jconf["queue_columns"]["website_text"] = GUI::conf.col_site_text;
		jconf["output_template_bandcamp"] = to_utf8(GUI::conf.output_template_bandcamp);
		jconf["json_hide_null"] = GUI::conf.json_hide_null;
		jconf["ytdlp_nightly"] = GUI::conf.ytdlp_nightly;
		jconf["audio_multistreams"] = GUI::conf.audio_multistreams;
		jconf["cbsnap"] = GUI::conf.cbsnap;
		jconf["limit_output_buffer"] = GUI::conf.limit_output_buffer;
		jconf["output_buffer_size"] = GUI::conf.output_buffer_size;
		jconf["update_self_only"] = GUI::conf.update_self_only;
		jconf["pref_vcodec"] = GUI::conf.pref_vcodec;
		jconf["pref_acodec"] = GUI::conf.pref_acodec;
		jconf["argset"] = GUI::conf.argset;
		jconf["cb_premium"] = GUI::conf.cb_premium;
		jconf["cbminw"] = GUI::conf.cbminw;
		jconf["cb_save_errors"] = GUI::conf.cb_save_errors;
		jconf["cb_ffplay"] = GUI::conf.cb_ffplay;
		jconf["ffmpeg_path"] = GUI::conf.ffmpeg_path;
		jconf["cb_clear_done"] = GUI::conf.cb_clear_done;
		jconf["cb_formats_fsize_bytes"] = GUI::conf.cb_formats_fsize_bytes;
		jconf["cb_add_on_focus"] = GUI::conf.cb_add_on_focus;

		if(jconf.contains("sblock"))
		{
			if(jconf["sblock"].contains("mark"))
				jconf["sblock"]["mark"].clear();
			if(jconf["sblock"].contains("remove"))
				jconf["sblock"]["remove"].clear();
		}

		for(auto i : GUI::conf.sblock_mark)
			jconf["sblock"]["mark"].push_back(i);
		for(auto i : GUI::conf.sblock_remove)
			jconf["sblock"]["remove"].push_back(i);

		jconf["sblock"]["cb_mark"] = GUI::conf.cb_sblock_mark;
		jconf["sblock"]["cb_remove"] = GUI::conf.cb_sblock_remove;
		jconf["proxy"]["enabled"] = GUI::conf.cb_proxy;
		jconf["proxy"]["URL"] = to_utf8(GUI::conf.proxy);

		if(jconf.contains("playsel_strings"))
			jconf.erase("playsel_strings");
		for(auto &el : jconf["unfinished_queue_items"])
		{
			const std::string url {el};
			auto &bot {gui.botref().at(url)};
			if(!bot.playsel_string.empty() && !bot.playlist_info["entries"].empty())
				jconf["playsel_strings"][url] = bot.playlist_info["entries"][0]["id"].get<std::string>() + "|" + to_utf8(bot.playsel_string);
		}

		busy = false;
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