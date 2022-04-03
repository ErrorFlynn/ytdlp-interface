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
		std::wstring fmt1, fmt2, args;
		double ratelim {0}, contrast {.1};
		unsigned ratelim_unit {1}, pref_res {2}, pref_video {1}, pref_audio {1}, cbtheme {2};
		bool cbsplit {false}, cbchaps {false}, cbsubs {false}, cbthumb {false}, cbtime {true}, cbkeyframes {false}, cbmp3 {false},
			cbargs {true};
		bool pref_fps {true}, vidinfo {false};
	}
	conf;

private:

	widgets::theme_t theme;
	nlohmann::json vidinfo, releases;
	fs::path self_path, appdir, ffmpeg_loc;
	std::wstring strfmt, url;
	std::string inet_error;
	bool working {false}, link_yt {false};
	std::thread thr, thr_releases;
	CComPtr<ITaskbarList3> i_taskbar;
	UINT WM_TASKBAR_BUTTON_CREATED {0};
	const std::string ver_tag {"v1.4.1"}, title {"ytdlp-interface " + ver_tag/*.substr(0, 4)*/};

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
	widgets::Textbox tbpipe {page3, &theme}, tbrate {gpopt, &theme}, tbargs {gpopt, &theme};
	widgets::Button btnbackto2 {page3, &theme, "<<< Back "}, btndl {page3, &theme, "Begin download"};
	widgets::Label l_out {gpopt, &theme, "Download folder:"}, l_rate {gpopt, &theme, "Download rate limit:"};
	widgets::path_label l_outpath {gpopt, &theme, &conf.outpath};
	widgets::Combox com_rate {gpopt, &theme};
	widgets::cbox cbsplit {gpopt, &theme, "Split chapters"}, cbkeyframes {gpopt, &theme, "Force keyframes at cuts"},
		cbmp3 {gpopt, &theme, "Convert audio to MP3"}, cbchaps {gpopt, &theme, "Embed chapters"}, 
		cbsubs {gpopt, &theme, "Embed subtitles"}, cbthumb {gpopt, &theme, "Embed thumbnail"},
		cbtime {gpopt, &theme, "File modification time = time of writing"}, cbargs {gpopt, &theme, "Custom arguments:"};
	nana::label tbpipe_overlay {page3, "output from yt-dlp.exe appears here\n\nclick to copy to clipboard"};

	const std::vector<std::wstring>
		com_res_options {L"2160", L"1440", L"1080", L"720", L"480", L"360"},
		com_audio_options {L"none", L"m4a", L"mp3", L"ogg", L"webm"},
		com_video_options {L"none", L"mp4", L"webm"};

	void on_btn_next();
	void on_btn_dl();
	void settings_dlg();
	void changes_dlg(nana::window parent);
	void make_page1();
	void make_page2();
	void make_page3();
	void apply_theme(bool dark);

	nana::size dpi_transform(double w, double h = 0)
	{
		double dpi_horz {static_cast<double>(nana::API::screen_dpi(true))},
			dpi_vert {static_cast<double>(nana::API::screen_dpi(false))};

		if(dpi_horz > 96)
		{
			w = round(w * dpi_horz/96.0);
			h = round(h * dpi_vert/96.0);
		}
		return nana::size {static_cast<unsigned>(w), static_cast<unsigned>(h)};
	}


	void maximize_v(unsigned w = 0)
	{
		auto sz {nana::screen {}.from_window(*this).area().dimension()};
		sz.width = w ? dpi_transform(w).width : size().width;
		move(nana::API::make_center(sz).position().x, 0);
		nana::API::window_outline_size(*this, sz);
	}


	void center(double w, double h)
	{
		auto r {nana::API::make_center(dpi_transform(w, h))};
		if(r.y >= 20) r.y -= 20;
		move(r);
		auto sz {nana::API::window_outline_size(*this)};
		auto maxh {nana::screen {}.from_window(*this).area().dimension().height};
		if(sz.height > maxh)
		{
			nana::API::window_outline_size(*this, {sz.width, maxh});
			move(r.x, 0);
		}
	}


	bool is_ytlink(std::string_view text)
	{
		if(text.find(R"(https://www.youtube.com/watch?v=)") == 0)
			if(text.size() == 43)
				return true;
			else if(text.size() > 43 && text[43] == '&')
				return true;
		if(text.find(R"(https://youtu.be/)") == 0)
			if(text.size() == 28)
				return true;
			else if(text.size() > 28 && text[28] == '?')
				return true;
		return false;
	}


	template<class T>
	void change_field_weight(T &obj, std::string field, unsigned new_weight)
	{
		std::string divtext {obj.div()};
		auto pos {divtext.find(field)};
		if(pos != -1)
		{
			pos = divtext.find("weight=", pos);
			if(pos != -1)
			{
				pos += 7;
				auto cut_pos {pos};
				while(isdigit(divtext[pos]))
					pos++;
				obj.div(divtext.substr(0, cut_pos) + std::to_string(new_weight) + divtext.substr(pos));
				obj.collocate();
			}
		}
	}


	void btn_next_morph()
	{
		if(link_yt && conf.vidinfo)
		{
			btn_next.caption(" Format selection >>>");
			change_field_weight(plc1, "btn_next", 240);
		}
		else
		{
			btn_next.caption(" Download >>>");
			change_field_weight(plc1, "btn_next", 180);
		}
	}


	bool is_tag_a_new_version(std::string tag_name)
	{
		if(tag_name[1] > ver_tag[1]) return true;
		if(tag_name[1] == ver_tag[1] && tag_name[3] > ver_tag[3]) return true;
		if(tag_name[1] == ver_tag[1] && tag_name[3] == ver_tag[3] && tag_name[5] > ver_tag[5]) return true;
		return false;
	};
};