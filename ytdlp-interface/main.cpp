#include "gui.hpp"


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
//int main()
{
	using namespace nana;

	nlohmann::json jconf;

	std::wstring modpath(4096, '\0');
	modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));

	paint::image img {modpath};
	API::window_icon_default(img, img);

	auto appdir {modpath.substr(0, modpath.rfind('\\'))};
	if(std::filesystem::exists(appdir + L"\\yt-dlp.exe"))
		GUI::conf.ytdlp_path = appdir + L"\\yt-dlp.exe";

	std::filesystem::path confpath {get_sys_folder(FOLDERID_RoamingAppData) + L"\\ytdlp-interface.json"};
	if(std::filesystem::exists(confpath))
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
		}
	}
	else
	{
		if(GUI::conf.ytdlp_path.empty() && std::filesystem::exists(LR"(C:\Program Files\yt-dlp\yt-dlp.exe)"))
			GUI::conf.ytdlp_path = LR"(C:\Program Files\yt-dlp\yt-dlp.exe)";
		GUI::conf.outpath = get_sys_folder(FOLDERID_Downloads);
	}

	GUI gui;
	gui.show();
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
		std::ofstream f {confpath};
		f << std::setw(4) << jconf;
	});
	nana::exec();
}