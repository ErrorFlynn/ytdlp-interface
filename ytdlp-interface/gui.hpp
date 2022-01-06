#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/filebox.hpp>

#include <thread>
#include <sstream>

#include <iostream>
#include <atlbase.h> // CComPtr
#include <Shobjidl.h> // ITaskbarList3

#include "util.hpp"
#include "progress_ex.hpp"

class Label : public nana::label
{
public:
	Label(nana::window parent, std::string_view text, bool dpi_adjust = false) : nana::label(parent, text)
	{
		fgcolor(nana::color {"#458"});
		bgcolor(nana::colors::white);
		typeface(nana::paint::font_info {"", 12 - (double)(nana::API::screen_dpi(true) > 96)*2*dpi_adjust});
		text_align(nana::align::right, nana::align_v::center);
	}
};

class Text : public nana::label
{
public:
	Text(nana::window parent, std::string_view text = "") : nana::label(parent, text)
	{
		fgcolor(nana::color {"#575"});
		bgcolor(nana::colors::white);
		typeface(nana::paint::font_info {"", 12 - (double)(nana::API::screen_dpi(true) > 96)*2});
		text_align(nana::align::left, nana::align_v::center);
	}
};

class cbox : public nana::checkbox
{
public:
	cbox(nana::window parent, std::string_view text) : nana::checkbox(parent, text)
	{
		fgcolor(nana::color {"#458"});
		bgcolor(nana::colors::white);
		typeface(nana::paint::font_info {"", 12});
		scheme().square_border_color = fgcolor();
	}
};

class Button : public nana::button
{
public:
	Button(nana::window parent, std::string_view text = "") : nana::button(parent, text)
	{
		typeface(nana::paint::font_info {"", 14, {800}});
		fgcolor(nana::color {"#67a"});
		bgcolor(nana::colors::white);
		enable_focus_color(false);
	}
};

class Listbox : public nana::listbox
{
public:

	Listbox(nana::window parent) : listbox(parent) {}

	size_t item_count()
	{
		size_t count {0};
		for(size_t cat_idx {0}; cat_idx < size_categ(); cat_idx++)
			count += at(cat_idx).size();
		return count;
	}
};

class Progress : public nana::progress_ex
{
public:

	Progress(nana::window parent) : progress_ex(parent)
	{
		bgcolor(nana::colors::white);
		typeface(nana::paint::font_info {"", 11, {800}});
		text_contrast_colors(nana::colors::white, nana::color {"#678"});
		nana::paint::image img;
		img.open(arr_progbar_jpg, sizeof arr_progbar_jpg);
		image(img);
	}
};

class GUI : public nana::form
{
public:

	GUI();

	static struct settings_t
	{
		std::filesystem::path ytdlp_path, outpath;
		std::wstring fmt1, fmt2;
		double ratelim {0};
		unsigned ratelim_unit {1}, pref_res {2}, pref_video {1}, pref_audio {1};
		bool cbsplit {false}, cbchaps {false}, cbsubs {false}, cbthumb {false}, cbtime {true}, cbkeyframes {false}, cbmp3 {false};
		bool pref_fps {true}, vidinfo {false};
	}
	conf;

private:

	nlohmann::json vidinfo;
	std::filesystem::path appdir, ffmpeg_loc;
	std::wstring strfmt;
	bool working {false}, link_yt {false};
	std::thread thr;
	CComPtr<ITaskbarList3> i_taskbar;
	UINT WM_TASKBAR_BUTTON_CREATED {0};
	subclass sc {*this};
	HWND hwnd {nullptr};
	const std::string ver_tag {"v1.2.0"};

	nana::panel<false> page1 {*this}, page1a {*this}, page2 {*this}, page3 {*this};
	nana::place plc1 {page1}, plc1a {page1a}, plc2 {page2}, plc3 {page3}, plcopt;

	/*** page1 widgets ***/
	Label l_ytdlp {page1, "Location of yt-dlp.exe:"}, l_ytlink {page1, "URL of video webpage:"};
	Button btn_next {page1, " Download >>>"}, btn_settings {page1, " Settings"};
	nana::button btn_path {page1, "..."};
	nana::textbox tblink {page1};
	nana::label separator_a {page1}, separator_b {page1}, l_path {page1};
	nana::label l_wait {page1a, "<bold color=0xaa8888 size=20>DON'T MOVE\n\n</><bold color=0x556677 size=14>getting video info...\n</>"};

	/*** page2 widgets ***/
	nana::label l_title {page2};
	Label l_dur {page2, "Duration:", true}, l_chap {page2, "Chapters:", true}, l_upl {page2, "Uploader:", true}, l_date {page2, "Upload date:", true};
	Text l_durtext {page2}, l_chaptext {page2}, l_upltext {page2}, l_datetext {page2};
	nana::picture thumb {page2};
	nana::label separator_1a {page2}, separator_1b {page2}, separator_2a {page2}, separator_2b {page2};
	Listbox lbformats {page2};
	Button btntodl {page2, " Download >>>"}, btnbackto1 {page2, "<<< Back "};

	/*** page3 widgets ***/
	nana::group gpopt {page3, "Options"};
	Progress prog {page3};
	nana::textbox tbpipe {page3}, tbrate {gpopt};
	Button btnbackto2 {page3, "<<< Back "}, btndl {page3, "Begin download"};
	Label l_out {gpopt, "Download folder:"}, l_rate {gpopt, "Download rate limit:"};
	nana::label l_outpath {gpopt};
	nana::button btn_outpath {gpopt, "..."};
	nana::combox com_rate {gpopt};
	cbox cbsplit {gpopt, "Split chapters"}, cbkeyframes {gpopt, "Force keyframes at cuts"}, cbmp3 {gpopt, "Convert audio to MP3"}, 
		cbchaps {gpopt, "Embed chapters"}, cbsubs {gpopt, "Embed subtitles"}, cbthumb {gpopt, "Embed thumbnail"}, 
		cbtime {gpopt, "File modification time = time of writing"};

	const std::vector<std::wstring>
		com_res_options {L"2160", L"1440", L"1080", L"720", L"480", L"360"},
		com_audio_options {L"none", L"m4a", L"mp3", L"ogg", L"webm"},
		com_video_options {L"none", L"mp4", L"webm"};

	void on_btn_next();
	void on_btn_dl();
	void settings_dlg();
	void make_page1();
	void make_page2();
	void make_page3();


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
		if(text.find(R"(https://youtu.be/)") == 0)
			if(text.size() == 28)
				return true;
		return false;
	}


	void label_path_caption(nana::label &l, const std::filesystem::path &p)
	{
		if(!l.size().empty())
		{
			l.caption(p.wstring());
			int offset {0};
			while(l.measure(1234).width > l.size().width)
			{
				offset += 1;
				if(p.wstring().size() - offset < 4)
				{
					l.caption("");
					return;
				}
				l.caption(L"..." + std::wstring {p.wstring()}.substr(offset));
			}
		}
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
};