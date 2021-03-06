#include "gui.hpp"


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
	//AllocConsole();
	//freopen("conout$", "w", stdout);
	using namespace nana;

	int argc;
	LPWSTR *argv {CommandLineToArgvW(GetCommandLineW(), &argc)};
	fs::path modpath {argv[0]}, appdir {modpath.parent_path()};
	std::error_code ec;
	if(fs::exists(appdir / "7zxa.dll"))
		fs::remove(appdir / "7zxa.dll", ec);

	if(argc > 1)
	{
		if(std::wstring {argv[1]} == L"update")
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
		else if(std::wstring {argv[1]} == L"cleanup")
		{
			fs::remove(fs::temp_directory_path() / "ytdlp-interface.exe", ec);
		}
	}
	LocalFree(argv);

	nlohmann::json jconf;

	paint::image img {modpath};
	API::window_icon_default(img, img);

	if(fs::exists(appdir / "yt-dlp.exe"))
		GUI::conf.ytdlp_path = appdir / "yt-dlp.exe";

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

	if(fs::exists(confpath))
	{
		std::ifstream f {confpath};
		try { f >> jconf; }
		catch(nlohmann::detail::exception e)
		{
			msgbox mbox {"ytdlp-interface"};
			mbox.icon(msgbox::icon_error);
			(mbox << "an exception occured when trying to load the settings file: " << e.what())();
		}
		if(!jconf.empty())
		{
			if(GUI::conf.ytdlp_path.empty())
				GUI::conf.ytdlp_path = std::string {jconf["ytdlp_path"]};
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
				GUI::conf.vidinfo = jconf["vidinfo"];
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
			}
		}
	}
	else
	{
		if(GUI::conf.ytdlp_path.empty() && fs::exists(LR"(C:\Program Files\yt-dlp\yt-dlp.exe)"))
			GUI::conf.ytdlp_path = LR"(C:\Program Files\yt-dlp\yt-dlp.exe)";
		GUI::conf.outpath = util::get_sys_folder(FOLDERID_Downloads);
	}

	GUI gui;
	gui.show();
	gui.bring_top(true);
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
		jconf["vidinfo"] = GUI::conf.vidinfo;
		jconf["cbtheme"] = GUI::conf.cbtheme;
		jconf["contrast"] = GUI::conf.contrast;
		jconf["cbargs"] = GUI::conf.cbargs;
		jconf["kwhilite"] = GUI::conf.kwhilite;
		jconf["argsets"] = GUI::conf.argsets;
		jconf["com_args"] = GUI::conf.com_args;
		std::ofstream {confpath} << std::setw(4) << jconf;
	});
	nana::exec();
}