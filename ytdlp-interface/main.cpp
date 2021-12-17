#include "gui.hpp"


int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
//int main()
{
	using namespace nana;

	nlohmann::json jconf;

	std::wstring modpath(4096, '\0');
	modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));
	auto appdir {modpath.substr(0, modpath.rfind('\\'))};
	if(std::filesystem::exists(appdir + L"\\yt-dlp.exe"))
		conf.ytdlp_path = appdir + L"\\yt-dlp.exe";

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
			if(conf.ytdlp_path.empty()) 
				conf.ytdlp_path = std::string {jconf["ytdlp_path"]};
			conf.outpath = std::string {jconf["outpath"]};
			conf.fmt1 = to_wstring(std::string {jconf["fmt1"]});
			conf.fmt2 = to_wstring(std::string {jconf["fmt2"]});
			conf.ratelim = jconf["ratelim"];
			conf.ratelim_unit = jconf["ratelim_unit"];
			if(jconf.contains("cbsplit"))
			{
				conf.cbsplit = jconf["cbsplit"];
				conf.cbchaps = jconf["cbchaps"];
				conf.cbsubs = jconf["cbsubs"];
				conf.cbthumb = jconf["cbthumb"];
				conf.cbtime = jconf["cbtime"];
				conf.cbkeyframes = jconf["cbkeyframes"];
				conf.cbmp3 = jconf["cbmp3"];
			}
		}
	}
	else // make defaults
	{
		if(conf.ytdlp_path.empty() && std::filesystem::exists(LR"(C:\Program Files\yt-dlp\yt-dlp.exe)"))
			conf.ytdlp_path = LR"(C:\Program Files\yt-dlp\yt-dlp.exe)";
		conf.outpath = get_sys_folder(FOLDERID_Downloads);
	}

	GUI gui;
	gui.icon(paint::image {modpath});
	gui.show();
	gui.events().unload([&]
	{
		jconf["ytdlp_path"] = conf.ytdlp_path;
		jconf["outpath"] = conf.outpath;
		jconf["fmt1"] = to_utf8(conf.fmt1);
		jconf["fmt2"] = to_utf8(conf.fmt2);
		jconf["ratelim"] = conf.ratelim;
		jconf["ratelim_unit"] = conf.ratelim_unit;
		jconf["cbsplit"] = conf.cbsplit;
		jconf["cbchaps"] = conf.cbchaps;
		jconf["cbsubs"] = conf.cbsubs;
		jconf["cbthumb"] = conf.cbthumb;
		jconf["cbtime"] = conf.cbtime;
		jconf["cbkeyframes"] = conf.cbkeyframes;
		jconf["cbmp3"] = conf.cbmp3;
		std::ofstream f {confpath};
		f << std::setw(4) << jconf;
	});
	nana::exec();
}