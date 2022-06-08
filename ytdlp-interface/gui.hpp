#include <thread>
#include <sstream>
#include <iostream>
#include <atlbase.h> // CComPtr
#include <Shobjidl.h> // ITaskbarList3

#include "widgets.hpp"
#include "themed_form.hpp"

namespace fs = std::filesystem;


class GUI : public themed_form
{	
public:

	GUI();

	static struct settings_t
	{
		fs::path ytdlp_path, outpath;
		std::wstring fmt1, fmt2;
		std::vector<std::string> argsets;
		double ratelim {0}, contrast {.1};
		unsigned ratelim_unit {1}, pref_res {2}, pref_video {1}, pref_audio {1}, cbtheme {2}, max_argsets {10};
		size_t com_args {0};
		bool cbsplit {false}, cbchaps {false}, cbsubs {false}, cbthumb {false}, cbtime {true}, cbkeyframes {false}, cbmp3 {false},
			cbargs {true}, kwhilite {true};
		bool pref_fps {true}, vidinfo {false};
	}
	conf;

private:

	widgets::theme_t theme;
	nlohmann::json vidinfo, releases;
	fs::path self_path, appdir, ffmpeg_loc;
	std::wstring strfmt, url;
	std::string inet_error, url_latest_ffmpeg, url_latest_ytdlp, url_latest_ytdlp_relnotes;
	unsigned size_latest_ffmpeg {0}, size_latest_ytdlp {0};
	bool working {false}, link_yt {false};
	std::thread thr, thr_releases, thr_releases_misc, thr_versions;
	CComPtr<ITaskbarList3> i_taskbar;
	UINT WM_TASKBAR_BUTTON_CREATED {0};
	const std::string ver_tag {"v1.5.1"}, title {"ytdlp-interface " + ver_tag/*.substr(0, 4)*/};

	nana::panel<false> page1 {*this}, page1a {*this}, page2 {*this}, page3 {*this};
	nana::place plc1 {page1}, plc1a {page1a}, plc2 {page2}, plc3 {page3}, plcopt;

	/*** page1 widgets ***/
	widgets::Label l_ytdlp {page1, &theme, "Location of yt-dlp.exe:"}, l_ytlink {page1, &theme, "URL of media webpage:"};
	widgets::Button btn_next {page1, &theme, " Download >>>"}, btn_settings {page1, &theme, " Settings"};
	widgets::separator separator_a {page1, &theme}, separator_b {page1, &theme};
	widgets::path_label l_path {page1, &theme, &conf.ytdlp_path}, l_url {page1, &theme, &url};
	nana::label l_wait {page1a};

	/*** page2 widgets ***/
	widgets::Title l_title {page2, &theme};
	widgets::Label l_dur {page2, &theme, "Duration:", true}, l_chap {page2, &theme, "Chapters:", true},
		l_upl {page2, &theme, "Uploader:", true}, l_date {page2, &theme, "Upload date:", true};
	widgets::Text l_durtext {page2, &theme}, l_chaptext {page2, &theme}, l_upltext {page2, &theme}, l_datetext {page2, &theme};
	nana::picture thumb {page2};
	widgets::separator separator_1a {page2, &theme}, separator_1b {page2, &theme}, separator_2a {page2, &theme}, separator_2b {page2, &theme};
	widgets::Listbox lbformats {page2, &theme};
	widgets::Button btntodl {page2, &theme, " Download >>>"}, btnbackto1 {page2, &theme, "<<< Back "};

	/*** page3 widgets ***/
	widgets::Group gpopt {page3, "Options", &theme};
	widgets::Progress prog {page3, &theme};
	widgets::Textbox tbpipe {page3, &theme}, tbrate {gpopt, &theme};
	widgets::Button btnbackto2 {page3, &theme, "<<< Back "}, btndl {page3, &theme, "Begin download"}, 
		btnerase {gpopt, &theme};
	widgets::Label l_out {gpopt, &theme, "Download folder:"}, l_rate {gpopt, &theme, "Download rate limit:"};
	widgets::path_label l_outpath {gpopt, &theme, &conf.outpath};
	widgets::Combox com_rate {gpopt, &theme}, com_args {gpopt, &theme};
	widgets::cbox cbsplit {gpopt, &theme, "Split chapters"}, cbkeyframes {gpopt, &theme, "Force keyframes at cuts"},
		cbmp3 {gpopt, &theme, "Convert audio to MP3"}, cbchaps {gpopt, &theme, "Embed chapters"}, 
		cbsubs {gpopt, &theme, "Embed subtitles"}, cbthumb {gpopt, &theme, "Embed thumbnail"},
		cbtime {gpopt, &theme, "File modification time = time of writing"}, cbargs {gpopt, &theme, "Custom arguments:"};
	nana::label tbpipe_overlay {page3, "output from yt-dlp.exe appears here\n\nclick to copy to clipboard"
		"\n\nright-click to toggle keyword highlighting"};

	const std::vector<std::wstring>
		com_res_options {L"2160", L"1440", L"1080", L"720", L"480", L"360"},
		com_audio_options {L"none", L"m4a", L"mp3", L"ogg", L"webm"},
		com_video_options {L"none", L"mp4", L"webm"};

	struct version_t
	{
		int year {0}, month {0}, day {0};
		bool empty() { return year == 0; }
		auto string()
		{
			auto fmt = [](int i)
			{
				auto str {std::to_string(i)};
				return str.size() == 1 ? '0' + str : str;
			};
			return std::to_string(year) + '.' + fmt(month) + '.' + fmt(day);
		}
		bool operator > (const version_t &o)
		{
			auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
			auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
			return self > other;
		}
	}
	ver_ffmpeg, ver_ffmpeg_latest, ver_ytdlp, ver_ytdlp_latest;

	void on_btn_next();
	void on_btn_dl();
	void settings_dlg();
	void changes_dlg(nana::window parent);
	void make_page1();
	void make_page2();
	void make_page3();
	void apply_theme(bool dark);
	void get_releases();
	void get_releases_misc();
	void get_versions();
	nana::size dpi_transform(double w, double h = 0);
	void maximize_v(unsigned w = 0);
	void center(double w, double h);
	bool is_ytlink(std::string_view text);
	template<class T> void change_field_weight(T &obj, std::string field, unsigned new_weight);
	void btn_next_morph();
	bool is_tag_a_new_version(std::string tag_name);
	void updater_dlg(nana::window parent);
};
