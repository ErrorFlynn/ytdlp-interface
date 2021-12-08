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
#include <iostream> // cout debugging
#include <sstream>

#include <atlbase.h> // CComPtr
#include <Shobjidl.h> // ITaskbarList3

#include "util.hpp"
#include "nana_subclassing.hpp"

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
	Text(nana::window parent) : nana::label(parent)
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

class GUI : public nana::form
{
public:

	GUI() : form()
	{
		using namespace nana;

		if(WM_TASKBAR_BUTTON_CREATED = RegisterWindowMessageW(L"TaskbarButtonCreated"))
		{
			sc.make_before(WM_TASKBAR_BUTTON_CREATED, [&](UINT, WPARAM, LPARAM, LRESULT*)
			{
				if(FAILED(i_taskbar.CoCreateInstance(CLSID_TaskbarList)))
					return false;

				if(i_taskbar->HrInit() != S_OK)
				{
					i_taskbar.Release();
					return false;
				}

				return true;
			});
		}

		std::wstring modpath(4096, '\0');
		modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));
		modpath.erase(modpath.rfind('\\'));
		appdir = modpath;

		if(std::filesystem::exists(appdir.wstring() + L"\\ffmpeg.exe"))
			ffmpeg_loc = appdir.wstring() + L"\\ffmpeg.exe";
		else if(!conf.ytdlp_path.empty())
		{
			auto tmp {std::filesystem::path {conf.ytdlp_path}.replace_filename("ffmpeg.exe")};
			if(std::filesystem::exists(tmp))
				ffmpeg_loc = tmp;
		}

		hwnd = reinterpret_cast<HWND>(native_handle());

		center(630, 193);
		caption("ytdlp-interface v1.0");
		bgcolor(colors::white);

		make_page1();
		make_page2();
		make_page3();

		div("margin=20 <switchable <page1> <page1a> <page2> <page3>>");
		get_place()["page1"] << page1;
		get_place()["page1a"] << page1a;
		get_place()["page2"] << page2;
		get_place()["page3"] << page3;
		collocate();

		events().unload([&](const nana::arg_unload &arg)
		{
			working = false;
			if(thr.joinable())
				thr.join();
			if(thr.joinable())
				thr.detach();
			if(i_taskbar)
				i_taskbar.Release();
		});
	}

private:

	nlohmann::json vidinfo;
	std::filesystem::path appdir, ffmpeg_loc;
	std::wstring strfmt;
	bool working {false};
	std::thread thr;
	CComPtr<ITaskbarList3> i_taskbar;
	UINT WM_TASKBAR_BUTTON_CREATED {0};
	subclass sc {*this};
	HWND hwnd {nullptr};

	nana::panel<false> page1 {*this}, page1a {*this}, page2 {*this}, page3 {*this};
	nana::place plc1 {page1}, plc1a {page1a}, plc2 {page2}, plc3 {page3}, plcopt;

	/*** page1 widgets ***/
	Label l_ytdlp {page1, "Location of yt-dlp.exe:"}, l_ytlink {page1, "Link to YouTube video:"};
	Button btn_info {page1, " Format selection >>>"};
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
	nana::listbox lbformats {page2};
	Button btntodl {page2, " Download >>>"}, btnbackto1 {page2, "<<< Back "};

	/*** page3 widgets ***/
	nana::group gpopt {page3, "Options"};
	nana::textbox tbpipe {page3}, tbrate {gpopt};
	Button btnbackto2 {page3, "<<< Back "}, btndl {page3, "Begin download"};
	Label l_out {gpopt, "Download folder:"}, l_rate {gpopt, "Download rate limit:"};
	nana::label l_outpath {gpopt};
	nana::button btn_outpath {gpopt, "..."};
	nana::combox com_rate {gpopt};
	cbox cbsplit {gpopt, "Split chapters"}, cbkeyframes {gpopt, "Force keyframes at cuts"}, cbmp3 {gpopt, "Convert audio to MP3"};

	void on_btn_info()
	{
		using namespace nana;
		auto id {tblink.caption()};
		if(id.find("youtu.be") != -1)
			id = id.substr(id.rfind('/')+1);
		else id = id.substr(id.rfind("?v=")+3);
		if(vidinfo.empty() || vidinfo["id"] != id)
		{
			if(thr.joinable()) thr.join();
			thr = std::thread([this]
			{
				working = true;
				btntodl.enabled(false);
				get_place().field_display("page1a", true);
				collocate();
				thumb.focus();
				cursor(cursor::wait);
				l_wait.cursor(cursor::wait);

				auto jtext {run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + L" -j " + tblink.caption_wstring())};
				if(!working) return;
				if(jtext.find("{\"id\":") != 0)
				{
					form fm {*this, API::make_center(*this, 1000, 600)};
					fm.caption("yt-dlp.exe error");
					fm.bgcolor(colors::white);
					fm.div("vert margin=20 <label weight=30><weight=20><textbox>");
					nana::label lbl {fm, "Received unexpected output from yt-dlp.exe after requesting video info!"};
					fm["label"] << lbl;
					lbl.text_align(align::center, align_v::center);
					lbl.typeface(nana::paint::font_info {"", 13, {800}});
					lbl.fgcolor(color {"#BA4B3E"});
					nana::textbox tb {fm, jtext};
					fm["textbox"] << tb;
					tb.line_wrapped(true);
					fm.collocate();
					fm.modality();
					get_place().field_display("page1", true);
					collocate();
					working = false;
					return;
				}
				std::filesystem::path tmp_fname {std::tmpnam(nullptr)};
				vidinfo.clear();
				std::ofstream {tmp_fname} << jtext;
				std::ifstream {tmp_fname} >> vidinfo;
				std::filesystem::remove(tmp_fname);

				std::wstring thumb_url;
				for(auto &el : vidinfo["thumbnails"])
				{
					std::string url {el["url"]};
					std::wstring wrl {charset{url}};
					if(wrl.substr(wrl.rfind('/')+1) == L"mqdefault.jpg")
					{
						thumb_url = wrl;
						thumb_url.erase(4, 1);
						break;
					}
				}
				l_wait.cursor(cursor::arrow);
				cursor(cursor::arrow);
				std::string title(vidinfo["title"]);
				l_title.caption(title);

				int dur {vidinfo["duration"]};
				int hr {(dur/60)/60}, min {(dur/60)%60}, sec {dur%60};
				if(dur < 60) sec = dur;
				std::string durstr;
				std::stringstream ss;
				ss.width(2);
				ss.fill('0');
				if(hr)
				{
					ss << min;
					durstr = std::to_string(hr) + ':' + ss.str();
					ss.str("");
				}
				else durstr = std::to_string(min);
				ss << sec;
				durstr += ':' + ss.str();
				l_durtext.caption(durstr);

				auto strchap {std::to_string(vidinfo["chapters"].size())};
				if(strchap == "0") l_chaptext.caption("none");
				else l_chaptext.caption(strchap);

				l_upltext.caption(std::string {vidinfo["uploader"]});

				auto strdate {std::string {vidinfo["upload_date"]}};
				l_datetext.caption(strdate.substr(0, 4) + '-' + strdate.substr(4, 2) + '-' + strdate.substr(6, 2));

				tmp_fname = std::tmpnam(nullptr);
				run_piped_process(L'\"' + appdir.wstring() + L"\\curl.exe\" -o " + tmp_fname.wstring() + L' ' + thumb_url, &working);
				if(!working) return;
				thumb.load(paint::image {tmp_fname});
				std::filesystem::remove(tmp_fname);

				lbformats.clear();
				for(auto &fmt : vidinfo["formats"])
				{
					std::string format {fmt["format"]}, acodec {fmt["acodec"]}, vcodec {fmt["vcodec"]}, ext {fmt["ext"]},
						fps {"null"}, vbr {"none"}, abr {"none"}, asr {"null"}, filesize {"null"};
					if(fmt.contains("abr"))
						abr = std::to_string(unsigned(fmt["abr"]));
					if(fmt.contains("vbr"))
						vbr = std::to_string(unsigned(fmt["vbr"]));
					if(fmt["asr"] != nullptr)
						asr = std::to_string(unsigned(fmt["asr"]));
					if(fmt["fps"] != nullptr)
						fps = std::to_string(unsigned(fmt["fps"]));
					if(fmt["filesize"] != nullptr)
					{
						unsigned i {fmt["filesize"]};
						float f {static_cast<float>(i)};
						std::string s, bytes {" (" + format_int(i) + ")"};
						if(i < 1024)
							s = format_int(i);
						else if(i < 1024*1024)
							s = std::to_string(i/1024) + " KB" + bytes;
						else if(i < 1024*1024*1024)
							s = format_float(f/(1024*1024)) + " MB" + bytes;
						else s = format_float(f/(1024*1024*1024)) + " GB" + bytes;
						filesize = s;
					}
					unsigned catidx {0};
					if(acodec == "none")
						catidx = 2; // video only
					else if(vcodec == "none")
						catidx = 1; // audio only
					lbformats.at(catidx).append({format, acodec, vcodec, ext, fps, vbr, abr, asr, filesize});
					auto idstr {to_wstring(std::string {fmt["format_id"]})};
					lbformats.at(catidx).back().value(idstr);
					if(idstr == conf.fmt1 || idstr == conf.fmt2)
						lbformats.at(catidx).back().select(true);
				}
				if(is_zoomed(true)) restore();
				if(API::screen_dpi(true) > 96)
					maximize_v(1000);
				else center(1000, 384.0 + vidinfo["formats"].size()*20.4);
				get_place().field_display("page2", true);
				collocate();
				working = false;
			});
		}
		else
		{
			if(is_zoomed(true)) restore();
			if(API::screen_dpi(true) > 96)
				maximize_v(1000);
			else center(1000, 384.0 + vidinfo["formats"].size()*20.4);
			get_place().field_display("page2", true);
			collocate();
		}
	}

	void make_page1()
	{
		using namespace nana;
		
		plc1.div(R"(
			vert <weight=25 <l_ytdlp weight=164> <weight=15> <l_path> <weight=15> <btn_path weight=25> >
				<weight=20>
				<weight=25 < l_ytlink weight=164> <weight=15> <tblink> >
				<weight=20> <separator_a weight=1> <weight=1> <separator_b weight=1> <weight=20>
				<weight=40 <> <btn_info weight=260> <> >
		)");

		plc1["l_ytdlp"] << l_ytdlp;
		plc1["l_path"] << l_path;
		plc1["btn_path"] << btn_path;
		plc1["l_ytlink"] << l_ytlink;
		plc1["tblink"] << tblink;
		plc1["btn_info"] << btn_info;
		plc1["separator_a"] << separator_a;
		plc1["separator_b"] << separator_b;

		l_path.bgcolor(color {"#eef6ee"});
		l_path.fgcolor(color {"#354"});
		l_path.typeface(paint::font_info {"Tahoma", 10});
		l_path.text_align(align::center, align_v::center);
		l_path.caption(conf.ytdlp_path.wstring());

		btn_path.bgcolor(colors::white);
		separator_a.bgcolor(color {"#d6d6d6"});
		separator_b.bgcolor(color {"#d6d6d6"});

		tblink.typeface(paint::font_info {"", 11, {800}});
		tblink.fgcolor(color {"#789"});
		tblink.text_align(align::center);
		tblink.editable(false);
		tblink.caption("Click to paste YT video link");
		API::effects_edge_nimbus(tblink, effects::edge_nimbus::none);

		tblink.set_accept([this](wchar_t c)
		{
			if(tblink.text().size() >= 43 && c != keyboard::backspace && c != keyboard::del)
				return false;
			return true;
		});

		tblink.events().mouse_up([this](const arg_mouse &arg)
		{
			if(OpenClipboard(NULL))
			{
				if(IsClipboardFormatAvailable(CF_TEXT))
				{
					auto h {GetClipboardData(CF_TEXT)};
					if(h)
					{
						auto ptext {static_cast<char*>(GlobalLock(h))};
						if(ptext)
						{
							std::string str {ptext};
							if(str.size() > 43)
								str.resize(43);
							if(is_ytlink(str))
								tblink.caption(str);
							else
							{
								tblink.caption("Clipboard content is not a valid link!");
								if(!working) std::thread {[this]
								{
									working = true;
									tblink.bgcolor(colors::dark_red);
									auto clr {tblink.fgcolor()};
									tblink.fgcolor(colors::white);
									std::this_thread::sleep_for(std::chrono::seconds {2});
									tblink.bgcolor(colors::white);
									tblink.fgcolor(clr);
									working = false;
								}}.detach();
							}
							GlobalUnlock(h);
						}
					}
				}
				CloseClipboard();
			}
		});

		tblink.events().text_changed([this](const arg_textbox &arg)
		{
			btn_info.enabled(is_ytlink(tblink.caption()) && !l_path.caption().empty());
		});

		btn_info.typeface(paint::font_info {"", 15, {800}});
		btn_info.fgcolor(color {"#67a"});
		btn_info.bgcolor(colors::white);
		btn_info.enable_focus_color(false);
		btn_info.enabled(false);

		plc1a.div("<l_wait>");
		plc1a["l_wait"] << l_wait;
		l_wait.format(true);
		l_wait.text_align(align::center, align_v::center);
		l_wait.bgcolor(colors::white);

		auto find_ytdlp = [this]
		{
			nana::filebox fb {*this, true};
			if(!conf.ytdlp_path.empty())
				fb.init_file(conf.ytdlp_path);
			fb.allow_multi_select(false);
			fb.add_filter("", "yt-dlp.exe");
			fb.title("Locate and select yt-dlp.exe");
			auto res {fb()};
			if(res.size())
			{
				auto path {res.front()};
				if(path.u8string().find("yt-dlp.exe") != -1)
				{
					conf.ytdlp_path = res.front();
					l_path.caption(conf.ytdlp_path.wstring());
					btn_info.enabled(is_ytlink(tblink.caption()));
					auto tmp {std::filesystem::path {conf.ytdlp_path}.replace_filename("ffmpeg.exe")};
					if(std::filesystem::exists(tmp))
						ffmpeg_loc = tmp;
				}
			}
		};

		btn_info.events().click([this] { on_btn_info(); });
		btn_path.events().click(find_ytdlp);
		l_path.events().click(find_ytdlp);
	}

	void make_page2()
	{
		using namespace nana;

		plc2.div(R"(
			vert <weight=180px 
					<thumb weight=320px> <weight=20px>
					<vert
						<l_title weight=27px> <weight=10px>
						<separator_1a weight=1px> <weight=1px> <separator_1b weight=1px> <weight=10px>
						<weight=28px <l_dur weight=45%> <weight=10> <l_durtext> >
						<weight=28px <l_chap weight=45%> <weight=10> <l_chaptext> >
						<weight=28px <l_upl weight=45%> <weight=10> <l_upltext> >
						<weight=28px <l_date weight=45%> <weight=10> <l_datetext> > <weight=15px>
						<separator_2a weight=1px> <weight=1px> <separator_2b weight=1px>
					>
				 >
				<weight=20> <lbformats> <weight=20>
				<weight=35 <> <btnback weight=160> <weight=20> <btntodl weight=200> <>>
		)");
		plc2["l_title"] << l_title;
		plc2["l_dur"] << l_dur;
		plc2["l_durtext"] << l_durtext;
		plc2["l_chap"] << l_chap;
		plc2["l_chaptext"] << l_chaptext;
		plc2["l_upl"] << l_upl;
		plc2["l_upltext"] << l_upltext;
		plc2["l_date"] << l_date;
		plc2["l_datetext"] << l_datetext;
		plc2["thumb"] << thumb;
		plc2["separator_1a"] << separator_1a;
		plc2["separator_1b"] << separator_1b;
		plc2["separator_2a"] << separator_2a;
		plc2["separator_2b"] << separator_2b;
		plc2["lbformats"] << lbformats;
		plc2["btnback"] << btnbackto1;
		plc2["btntodl"] << btntodl;
		
		l_title.typeface(paint::font_info {"Arial", 15 - (double)(nana::API::screen_dpi(true) > 96)*3, {800}});
		l_title.text_align(align::center, align_v::top);
		l_title.fgcolor(color {"#789"});
		l_title.bgcolor(colors::white);
		
		separator_1a.bgcolor(color {"#d6d6d6"});
		separator_1b.bgcolor(color {"#d6d6d6"});
		separator_2a.bgcolor(color {"#d6d6d6"});
		separator_2b.bgcolor(color {"#d6d6d6"});
		
		lbformats.enable_single(true, true);
		lbformats.scheme().item_selected = color {"#D7EEF4"};
		lbformats.scheme().item_highlighted = color {"#ECFCFF"};
		lbformats.scheme().item_height_ex;
		lbformats.append_header("format", dpi_transform(240).width);
		lbformats.append_header("acodec", dpi_transform(90).width );
		lbformats.append_header("vcodec", dpi_transform(90).width );
		lbformats.append_header("ext", dpi_transform(50).width );
		lbformats.append_header("fps", dpi_transform(32).width );
		lbformats.append_header("vbr", dpi_transform(40).width );
		lbformats.append_header("abr", dpi_transform(40).width );
		lbformats.append_header("asr", dpi_transform(50).width );
		lbformats.append_header("filesize", dpi_transform(160).width );
		lbformats.append({"Audio only", "Video only"});

		lbformats.events().selected([this](const arg_listbox &arg)
		{
			if(arg.item.selected())
			{
				btntodl.enabled(true);
				if(arg.item.pos().cat == 0)
				{
					lbformats.at(1).select(false);
					lbformats.at(2).select(false);
				}
				else
				{
					lbformats.at(0).select(false);
				}
			}
			else if(lbformats.selected().empty())
				btntodl.enabled(false);
		});

		btnbackto1.events().click([this]
		{
			if(is_zoomed(true)) restore();
			get_place().field_display("page1", true);
			center(630, 193);
		});

		btntodl.events().click([this]
		{
			auto sel {lbformats.selected()};
			strfmt = lbformats.at(sel.front()).value<std::wstring>();
			conf.fmt1 = strfmt;
			if(sel.size() > 1)
			{
				conf.fmt2 = lbformats.at(sel.back()).value<std::wstring>();
				strfmt += L'+' + conf.fmt2;
			}
			else conf.fmt2.clear();
			get_place().field_display("page3", true);
			collocate();
		});
	}

	void make_page3()
	{
		using namespace nana;

		plc3.div(R"(
			vert <tbpipe> <weight=20>
			<gpopt weight=132> <weight=20>
			<weight=35 <> <btnbackto2 weight=160> <weight=20> <btndl weight=200> <>>
		)");
		plc3["tbpipe"] << tbpipe;
		plc3["gpopt"] << gpopt;
		plc3["btnbackto2"] << btnbackto2;
		plc3["btndl"] << btndl;

		tbpipe.editable(false);
		tbpipe.scheme().mouse_wheel.lines = 3;
		API::effects_edge_nimbus(tbpipe, effects::edge_nimbus::none);

		plcopt.bind(gpopt);
		page3.bgcolor(colors::white);
		gpopt.bgcolor(colors::white);
		gpopt.enable_format_caption(true);
		gpopt.caption("<bold color=0x61241F size=11 font=\"Tahoma\"> Options </>");
		gpopt.caption_align(align::center);
		gpopt.size({10, 10}); // workaround for weird caption display bug
		gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=122> <weight=15> <l_outpath> <weight=15> <btn_outpath weight=25> > <weight=20>
			<weight=25 
				<l_rate weight=144> <weight=15> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbmp3 weight=173> <> <cbsplit weight=116> <> <cbkeyframes weight=186>
			>
		)");
		gpopt["l_out"] << l_out;
		gpopt["l_outpath"] << l_outpath;
		gpopt["btn_outpath"] << btn_outpath;
		gpopt["l_rate"] << l_rate;
		gpopt["tbrate"] << tbrate;
		gpopt["com_rate"] << com_rate;
		gpopt["cbsplit"] << cbsplit;
		gpopt["cbkeyframes"] << cbkeyframes;
		gpopt["cbmp3"] << cbmp3;

		tbpipe.line_wrapped(true);

		tbrate.multi_lines(false);
		tbrate.padding(0, 5, 0, 5);
		if(conf.ratelim)
			tbrate.caption(format_float(conf.ratelim, 1));
		tbrate.set_accept([this](wchar_t wc)->bool
		{
			return wc == keyboard::backspace || wc == keyboard::del || ((isdigit(wc) || wc == '.') && tbrate.text().size() < 5);
		});

		tbrate.events().focus([this](const arg_focus &arg)
		{
			if(!arg.getting)
			{
				auto str {tbrate.caption()};
				if(str.back() == '.')
				{
					str.pop_back();
					tbrate.caption(str);
				}
				conf.ratelim = tbrate.to_double();
			}
		});

		com_rate.editable(false);
		com_rate.push_back(" KB/s");
		com_rate.push_back(" MB/s");
		com_rate.option(conf.ratelim_unit);
		com_rate.events().selected([&]
		{
			conf.ratelim_unit = com_rate.option();
		});

		l_outpath.bgcolor(color {"#eef6ee"});
		l_outpath.fgcolor(color {"#354"});
		l_outpath.typeface(paint::font_info {"Tahoma", 10});
		l_outpath.text_align(align::center, align_v::center);
		l_outpath.caption(conf.outpath.u8string());

		btn_outpath.bgcolor(colors::white);
		cbsplit.tooltip("Split into multiple files based on internal chapters.");
		cbkeyframes.tooltip("Force keyframes around the chapters before\nremoving/splitting them. Requires a\n"
			"reencode and thus is very slow, but the\nresulting video may have fewer artifacts\naround the cuts.");

		btndl.events().click([this]
		{
			if(btndl.caption().find("Stop") == -1)
			{
				btndl.caption("Stop download");
				btndl.fgcolor(color {"#966"});
				btndl.scheme().activated = color {"#a77"};

				std::wstring cmd {L'\"' + conf.ytdlp_path.wstring() + L"\" -f " + strfmt + L" --windows-filenames --newline "};
				if(!ffmpeg_loc.empty())
					cmd += L"--ffmpeg-location \"" + ffmpeg_loc.wstring() + L"\" ";
				if(tbrate.to_double())
					cmd += L"-r " + tbrate.caption_wstring() + (com_rate.option() ? L"M " : L"K ");
				if(cbmp3.checked())
				{
					cmd += L"-x --audio-format mp3 ";
					auto sel {lbformats.selected()};
					for(auto sel : lbformats.selected())
						if(sel.cat == 1)
						{
							std::wstring txt {charset{lbformats.at(sel).text(6)}};
							cmd += L"--audio-quality " + txt + L"K ";
						}
				}
				if(cbsplit.checked())
					cmd += L"--split-chapters -o chapter:\"" + conf.outpath.wstring() + L"\\%(title)s - %(section_number)s -%(section_title)s.%(ext)s\" ";
				if(cbkeyframes.checked())
					cmd += L"--force-keyframes-at-cuts ";
				cmd += L"-o \"" + conf.outpath.wstring() + L"\\%(title)s.%(ext)s\" ";
				cmd += tblink.caption_wstring();
				btnbackto1.enabled(false);
				tbpipe.select(true);
				tbpipe.del();
				if(thr.joinable()) thr.join();
				thr = std::thread([this, cmd]
				{
					working = true;
					auto ca {tbpipe.colored_area_access()};
					ca->clear();
					tbpipe.append(L"[GUI] executing command line: " + cmd + L"\n\n", false);
					auto p {ca->get(0)};
					p->count = 1;
					p->fgcolor = nana::color {"#569"};
					API::refresh_window(tbpipe);
					auto cb_progress = [this](ULONGLONG completed, ULONGLONG total)
					{
						if(i_taskbar) i_taskbar->SetProgressValue(hwnd, completed, total);
					};
					if(i_taskbar) i_taskbar->SetProgressState(hwnd, TBPF_NORMAL);
					run_piped_process(cmd, &working, &tbpipe, cb_progress);
					if(i_taskbar) i_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
					if(!working) return;
					btnbackto1.enabled(true);
					btndl.enabled(true);
					tbpipe.append("\n[GUI] yt-dlp.exe process has exited\n", false);
					p = ca->get(tbpipe.text_line_count()-2);
					p->count = 1;
					p->fgcolor = nana::color {"#569"};
					API::refresh_window(tbpipe);
					btndl.caption("Begin download");
					btndl.fgcolor(btnbackto2.fgcolor());
					btndl.scheme().activated = btnbackto2.scheme().activated;
					working = false;
				});
			}
			else
			{
				btndl.enabled(false);
				working = false;
				if(thr.joinable())
					thr.join();
				tbpipe.append("\n[GUI] yt-dlp.exe process was terminated\n", false);
				auto ca {tbpipe.colored_area_access()};
				auto p {ca->get(tbpipe.text_line_count()-2)};
				p->count = 1;
				p->fgcolor = nana::color {"#832"};
				API::refresh_window(tbpipe);
				btndl.enabled(true);
				btndl.caption("Begin download");
				btndl.fgcolor(btnbackto2.fgcolor());
				btndl.scheme().activated = btnbackto2.scheme().activated;
			}
		});

		btnbackto2.events().click([this]
		{
			if(is_zoomed(true)) restore();
			get_place().field_display("page2", true);
			collocate();
		});

		auto choose_outpath = [this]
		{
			std::wstring init {l_outpath.caption_wstring()};
			if(init.empty())
				init = get_sys_folder(FOLDERID_Downloads);
			nana::folderbox fb {*this, init};
			fb.allow_multi_select(false);
			fb.title("Locate and select the desired download folder");
			auto res {fb()};
			if(res.size())
			{
				conf.outpath = res.front();
				l_outpath.caption(conf.outpath.u8string());
			}
		};

		btn_outpath.events().click(choose_outpath);
		l_outpath.events().click(choose_outpath);

		plc3.collocate();
	}

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
};