#include "gui.hpp"
#include "icons.hpp"

#include <nana/gui/timer.hpp>
#include <nana/gui/filebox.hpp>

#pragma warning(disable : 4267)

GUI::GUI() : themed_form {std::bind(&GUI::apply_theme, this, std::placeholders::_1)}
{
	using namespace nana;

	thr_releases = std::thread {[this]
	{
		auto jtext {util::get_inet_res("https://api.github.com/repos/ErrorFlynn/ytdlp-interface/releases", &inet_error)};
		if(!jtext.empty())
		{
			fs::path tmp_fname {std::tmpnam(nullptr)};
			std::ofstream {tmp_fname} << jtext;
			std::ifstream {tmp_fname} >> releases;
			fs::remove(tmp_fname);
			std::string tag_name {releases[0]["tag_name"]};
			if(is_tag_a_new_version(tag_name))
				caption(title + "   (" + tag_name + " is available)");
		}
		thr_releases.detach();
	}};

	if(WM_TASKBAR_BUTTON_CREATED = RegisterWindowMessageW(L"TaskbarButtonCreated"))
	{
		msg.make_after(WM_TASKBAR_BUTTON_CREATED, [&](UINT, WPARAM, LPARAM, LRESULT*)
		{
			if(FAILED(i_taskbar.CoCreateInstance(CLSID_TaskbarList)))
				return false;

			if(i_taskbar->HrInit() != S_OK)
			{
				i_taskbar.Release();
				return false;
			}
			msg.umake_after(WM_TASKBAR_BUTTON_CREATED);
			return true;
		});
	}

	std::wstring modpath(4096, '\0');
	modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));
	self_path = modpath;
	modpath.erase(modpath.rfind('\\'));
	appdir = modpath;

	if(fs::exists(appdir / "ffmpeg.exe"))
		ffmpeg_loc = appdir / "ffmpeg.exe";
	else if(!conf.ytdlp_path.empty())
	{
		auto tmp {fs::path {conf.ytdlp_path}.replace_filename("ffmpeg.exe")};
		if(fs::exists(tmp))
			ffmpeg_loc = tmp;
	}

	center(630, 193);
	caption(title);

	make_page1();
	make_page2();
	make_page3();

	div("margin=20 <switchable <page1> <page1a> <page2> <page3>>");
	get_place()["page1"] << page1;
	get_place()["page1a"] << page1a;
	get_place()["page2"] << page2;
	get_place()["page3"] << page3;
	collocate();

	theme.contrast(conf.contrast);

	auto clr {btn_settings.scheme().activated};
	if(conf.cbtheme == 2)
	{
		if(system_supports_darkmode())
			system_theme(true);
		else conf.cbtheme = 1;
	}
	else if(conf.cbtheme == 0) dark_theme(true);
	if(conf.cbtheme == 1) dark_theme(false);

	events().unload([&]
	{
		working = false;
		if(thr.joinable())
			thr.join();
		if(thr_releases.joinable())
			thr_releases.detach();
		if(i_taskbar)
			i_taskbar.Release();
	});
}


void GUI::on_btn_next()
{
	using namespace nana;
	auto id {to_utf8(url)};
	if(id.find("youtu.be") != -1)
		id = id.substr(id.rfind('/')+1, 11);
	else id = id.substr(id.rfind("?v=")+3, 11);
	if(vidinfo.empty() || vidinfo["id"] != id)
	{
		if(!vidinfo.empty())
		{
			tbpipe.select(true);
			tbpipe.del();
			prog.value(0);
			prog.caption("");
		}
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

			auto jtext {util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + L" -j " + url)};
			if(!working) return;
			auto pos {jtext.find("{\"id\":")};
			if(pos == -1)
			{
				themed_form fm {nullptr, *this, API::make_center(*this, 1000, 600)};
				fm.caption("yt-dlp.exe error");
				fm.bgcolor(theme.fmbg);
				fm.div("vert margin=20 <label weight=30><weight=20><textbox>");
				fm.theme_callback([&, this](bool dark)
				{
					if(dark) theme.make_dark();
					else theme.make_light();
					fm.bgcolor(theme.fmbg);
				});
				::widgets::Label lbl {fm, &theme, "Received unexpected output from yt-dlp.exe after requesting video info!"};
				fm["label"] << lbl;
				lbl.text_align(align::center, align_v::center);
				lbl.typeface(nana::paint::font_info {"", 13, {800}});
				lbl.fgcolor(color {"#BA4B3E"});
				::widgets::Textbox tb {fm, &theme};
				tb.caption(jtext);
				fm["textbox"] << tb;
				tb.line_wrapped(true);
				fm.collocate();
				fm.modality();
				get_place().field_display("page1", true);
				collocate();
				bring_top(true);
				working = false;
				return;
			}
			if(pos) jtext.erase(0, pos);
			fs::path tmp_fname {std::tmpnam(nullptr)};
			vidinfo.clear();
			std::ofstream {tmp_fname} << jtext;
			std::ifstream {tmp_fname} >> vidinfo;
			fs::remove(tmp_fname);

			auto it {std::find_if(vidinfo["thumbnails"].begin(), vidinfo["thumbnails"].end(), [](const auto &el)
			{
				return std::string{el["url"]}.rfind("mqdefault.jpg") != -1;
			})};
			std::string thumb_url {(*it)["url"]};

			l_wait.cursor(cursor::arrow);
			cursor(cursor::arrow);
			std::string title {vidinfo["title"]};
			l_title.caption(title);

			int dur {0};//{vidinfo["duration"]};
			if(vidinfo["is_live"])
			{
				if(lbformats.size_categ() == 3)
				{
					lbformats.erase(1);
					lbformats.erase(1);
				}
			}
			else
			{
				dur = vidinfo["duration"];
				if(lbformats.size_categ() == 1)
					lbformats.append({"Audio only", "Video only"});
			}
			int hr {(dur/60)/60}, min {(dur/60)%60}, sec {dur%60};
			if(dur < 60) sec = dur;
			std::string durstr {"live"};
			if(dur)
			{
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
			}
			l_durtext.caption(durstr);

			auto strchap {std::to_string(vidinfo["chapters"].size())};
			if(strchap == "0") l_chaptext.caption("none");
			else l_chaptext.caption(strchap);

			l_upltext.caption(std::string {vidinfo["uploader"]});

			auto strdate {std::string {vidinfo["upload_date"]}};
			l_datetext.caption(strdate.substr(0, 4) + '-' + strdate.substr(4, 2) + '-' + strdate.substr(6, 2));

			auto thumb_jpg {util::get_inet_res(thumb_url)};
			if(!thumb_jpg.empty())
			{
				paint::image img;
				img.open(thumb_jpg.data(), thumb_jpg.size());
				thumb.load(img);
			}
			if(!working) return;

			lbformats.clear();
			for(auto &fmt : vidinfo["formats"])
			{
				std::string format {fmt["format"]}, acodec {fmt["acodec"]}, vcodec {fmt["vcodec"]}, ext {fmt["ext"]},
					fps {"null"}, vbr {"none"}, abr {"none"}, asr {"null"}, filesize {"null"};
				if(format.find("storyboard") == -1)
				{
					if(fmt.contains("abr"))
						abr = std::to_string(unsigned(fmt["abr"]));
					if(fmt.contains("vbr"))
						vbr = std::to_string(unsigned(fmt["vbr"]));
					if(fmt["asr"] != nullptr)
						asr = std::to_string(unsigned(fmt["asr"]));
					if(fmt["fps"] != nullptr)
						fps = std::to_string(unsigned(fmt["fps"]));
					if(fmt["filesize"] != nullptr)
						filesize = util::int_to_filesize(fmt["filesize"]);
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
			}
			if(is_zoomed(true)) restore();
			if(API::screen_dpi(true) > 96)
				maximize_v(1000);
			else center(1000, max(384.0 + lbformats.item_count()*20.4, 800));
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
		else center(1000, max(384.0 + lbformats.item_count() * 20.4, 800));
		get_place().field_display("page2", true);
		collocate();
	}
}


void GUI::on_btn_dl()
{
	bool graceful_exit {false};
	if(btndl.caption().find("Stop") == -1)
	{
		btndl.caption("Stop download");
		btndl.cancel_mode(true);
		tbpipe_overlay.hide();

		std::wstring cmd {L'\"' + conf.ytdlp_path.wstring() + L'\"'};
		if(link_yt && conf.vidinfo) cmd += L" -f " + strfmt;
		else
		{
			cmd += L" -S \"res:" + com_res_options[conf.pref_res];
			if(conf.pref_video)
				cmd += L",vext:" + com_video_options[conf.pref_video];
			if(conf.pref_audio)
				cmd += L",aext:" + com_audio_options[conf.pref_audio];
			if(conf.pref_fps)
				cmd += L",fps";
			cmd += '\"';
			btnbackto2.enabled(false);
		}
		cmd += L" --windows-filenames ";

		if(conf.cbchaps) cmd += L"--embed-chapters ";
		if(conf.cbsubs) cmd += L"--embed-subs ";
		if(conf.cbthumb) cmd += L"--embed-thumbnail ";
		if(conf.cbtime) cmd += L"--no-mtime ";
		if(conf.cbkeyframes) cmd += L"--force-keyframes-at-cuts ";
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
					std::wstring txt {nana::charset{lbformats.at(sel).text(6)}};
					cmd += L"--audio-quality " + txt + L"K ";
				}
		}
		if(cbsplit.checked())
			cmd += L"--split-chapters -o chapter:\"" + conf.outpath.wstring() + L"\\%(title)s - %(section_number)s -%(section_title)s.%(ext)s\" ";
		if(cbkeyframes.checked())
			cmd += L"--force-keyframes-at-cuts ";
		if(conf.cbargs && !conf.args.empty())
			cmd += conf.args + L" ";
		cmd += L"-o \"" + conf.outpath.wstring() + L"\\%(title)s.%(ext)s\" ";
		cmd += url;
		btnbackto1.enabled(false);
		tbpipe.select(true);
		tbpipe.del();
		if(thr.joinable()) thr.join();
		thr = std::thread([this, cmd, &graceful_exit]
		{
			working = true;
			auto ca {tbpipe.colored_area_access()};
			ca->clear();
			if(fs::exists(conf.ytdlp_path))
				tbpipe.append(L"[GUI] executing command line: " + cmd + L"\n\n", false);
			else tbpipe.append(L"ytdlp.exe not found: " + conf.ytdlp_path.wstring(), false);
			auto p {ca->get(0)};
			p->count = 1;
			if(!fs::exists(conf.ytdlp_path))
			{
				p->fgcolor = theme.is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
				nana::API::refresh_window(tbpipe);
				btndl.caption("Begin download");
				btndl.cancel_mode(false);
				btnbackto2.enabled(true);
				working = false;
				return;
			}
			p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
			nana::API::refresh_window(tbpipe);
			auto sel {lbformats.selected()};
			ULONGLONG prev_val {0};
			auto cb_progress = [&, this](ULONGLONG completed, ULONGLONG total, std::string text)
			{
				if(i_taskbar) i_taskbar->SetProgressValue(hwnd, completed, total);
				if(completed < 1000)
					prog.caption(text);
				else
				{
					if(prev_val)
					{
						auto pos {text.find_last_not_of(" \t")};
						if(text.size() > pos)
							text.erase(pos+1);
						prog.caption(text);
					}
				}
				prog.value(completed);
				prev_val = completed;
			};
			prog.value(0);
			prog.caption("");
			if(i_taskbar) i_taskbar->SetProgressState(hwnd, TBPF_NORMAL);
			util::run_piped_process(cmd, &working, &tbpipe, cb_progress, &graceful_exit);
			if(i_taskbar) i_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
			if(!working) return;
			btnbackto1.enabled(true);
			btndl.enabled(true);
			tbpipe.append("\n[GUI] yt-dlp.exe process has exited\n", false);
			p = ca->get(tbpipe.text_line_count()-2);
			p->count = 1;
			p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
			nana::API::refresh_window(tbpipe);
			btndl.caption("Begin download");
			btndl.cancel_mode(false);
			btnbackto2.enabled(true);
			working = false;
		});
	}
	else
	{
		btndl.enabled(false);
		working = false;
		if(thr.joinable())
			thr.join();
		if(graceful_exit)
			tbpipe.append("\n[GUI] yt-dlp.exe process was ended gracefully via Ctrl+C signal\n", false);
		else tbpipe.append("\n[GUI] yt-dlp.exe process was ended forcefully via WM_CLOSE message\n", false);
		auto ca {tbpipe.colored_area_access()};
		auto p {ca->get(tbpipe.text_line_count()-2)};
		p->count = 1;
		if(graceful_exit)
			p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
		else p->fgcolor = theme.is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
		nana::API::refresh_window(tbpipe);
		btnbackto1.enabled(true);
		btnbackto2.enabled(true);
		btndl.enabled(true);
		btndl.caption("Begin download");
		btndl.cancel_mode(false);
	}
}


void GUI::make_page1()
{
	using namespace nana;

	plc1.div(R"(
			vert <weight=25 <l_ytdlp weight=164> <weight=15> <l_path> >
				<weight=20>
				<weight=25 < l_ytlink weight=164> <weight=15> <l_url> >
				<weight=20> <separator_a weight=1> <weight=1> <separator_b weight=1> <weight=20>
				<weight=35 <> <btn_settings weight=152> <weight=20> <btn_next weight=180> <> >
		)");

	plc1["l_ytdlp"] << l_ytdlp;
	plc1["l_path"] << l_path;
	plc1["l_ytlink"] << l_ytlink;
	plc1["l_url"] << l_url;
	plc1["btn_settings"] << btn_settings;
	plc1["btn_next"] << btn_next;
	plc1["separator_a"] << separator_a;
	plc1["separator_b"] << separator_b;

	paint::image img;
	img.open(arr_settings_ico, sizeof arr_settings_ico);
	btn_settings.icon(img);

	l_url.events().mouse_up([this]
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
						if(isalpha(*ptext))
						{
							url = to_wstring(ptext);
							l_url.update_caption();
							btn_next.enabled(!l_path.caption().empty());
							link_yt = is_ytlink(ptext);
							btn_next_morph();
						}
						GlobalUnlock(h);
					}
				}
			}
			CloseClipboard();
		}
	});

	btn_next.enabled(false);

	plc1a.div("<l_wait>");
	plc1a["l_wait"] << l_wait;
	l_wait.format(true);
	l_wait.text_align(align::center, align_v::center);
	nana::API::effects_bground(l_wait, nana::effects::bground_transparent(0), 0);

	l_path.events().click([this]
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
				btn_next.enabled(!url.empty());
				auto tmp {fs::path {conf.ytdlp_path}.replace_filename("ffmpeg.exe")};
				if(fs::exists(tmp))
					ffmpeg_loc = tmp;
			}
		}
	});

	btn_next.events().click([this]
	{
		if(link_yt && conf.vidinfo) on_btn_next();
		else
		{
			if(is_zoomed(true)) restore();
			center(1000, 800);
			get_place().field_display("page3", true);
			collocate();
		}
	});
	btn_settings.events().click([this]
	{
		settings_dlg();
		btn_next_morph();
	});
}


void GUI::make_page2()
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

	lbformats.enable_single(true, true);
	lbformats.append_header("format", dpi_transform(240).width);
	lbformats.append_header("acodec", dpi_transform(90).width);
	lbformats.append_header("vcodec", dpi_transform(90).width);
	lbformats.append_header("ext", dpi_transform(50).width);
	lbformats.append_header("fps", dpi_transform(32).width);
	lbformats.append_header("vbr", dpi_transform(40).width);
	lbformats.append_header("abr", dpi_transform(40).width);
	lbformats.append_header("asr", dpi_transform(50).width);
	lbformats.append_header("filesize", dpi_transform(160).width);

	lbformats.events().selected([this](const arg_listbox &arg)
	{
		if(arg.item.selected())
		{
			btntodl.enabled(true);
			if(arg.item.pos().cat == 0)
			{
				if(lbformats.size_categ() == 3)
				{
					lbformats.at(1).select(false);
					lbformats.at(2).select(false);
				}
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


void GUI::make_page3()
{
	using namespace nana;

	plc3.div(R"(
			vert <tbpipe> <weight=20>
			<prog weight=30> <weight=20>
			<gpopt weight=220> <weight=20>
			<weight=35 <> <btnbackto2 weight=160> <weight=20> <btndl weight=200> <>>
		)");
	plc3["tbpipe"].fasten(tbpipe).fasten(tbpipe_overlay);
	plc3["prog"] << prog;
	plc3["gpopt"] << gpopt;
	plc3["btnbackto2"] << btnbackto2;
	plc3["btndl"] << btndl;

	prog.amount(1000);
	prog.caption("");

	tbpipe.editable(false);
	tbpipe.line_wrapped(true);
	tbpipe.scheme().mouse_wheel.lines = 3;
	tbpipe.typeface(paint::font_info {"Arial", 10});
	API::effects_edge_nimbus(tbpipe, effects::edge_nimbus::none);
	tbpipe.events().click([this]
	{
		tbpipe.select(true);
		tbpipe.copy();
		tbpipe.select(false);
	});

	API::effects_bground(tbpipe_overlay, effects::bground_transparent {0}, 0.0);
	tbpipe_overlay.text_align(align::center, align_v::center);
	tbpipe_overlay.typeface(nana::paint::font_info {"", 14, {700}});

	plcopt.bind(gpopt);
	page3.bgcolor(theme.fmbg);
	gpopt.size({10, 10}); // workaround for weird caption display bug

	gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=122> <weight=15> <l_outpath> > <weight=20>
			<weight=25 
				<l_rate weight=144> <weight=15> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbchaps weight=133> <> <cbsplit weight=116> <> <cbkeyframes weight=186>
			> <weight=20>
			<weight=25 <cbtime weight=296> <> <cbthumb weight=144> <> <cbsubs weight=132> <> <cbmp3 weight=173>>
			<weight=20> <weight=25 <cbargs weight=156> <weight=15> <tbargs>>
		)");
	gpopt["l_out"] << l_out;
	gpopt["l_outpath"] << l_outpath;
	gpopt["l_rate"] << l_rate;
	gpopt["tbrate"] << tbrate;
	gpopt["com_rate"] << com_rate;
	gpopt["cbsplit"] << cbsplit;
	gpopt["cbkeyframes"] << cbkeyframes;
	gpopt["cbmp3"] << cbmp3;
	gpopt["cbchaps"] << cbchaps;
	gpopt["cbsubs"] << cbsubs;
	gpopt["cbthumb"] << cbthumb;
	gpopt["cbtime"] << cbtime;
	gpopt["cbargs"] << cbargs;
	gpopt["tbargs"] << tbargs;

	tbrate.multi_lines(false);
	tbrate.padding(0, 5, 0, 5);
	tbargs.multi_lines(false);
	tbargs.padding(0, 5, 0, 5);
	tbargs.caption(conf.args);

	if(conf.ratelim)
		tbrate.caption(util::format_float(conf.ratelim, 1));
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

	tbargs.events().text_changed([this]
	{
		conf.args = tbargs.caption_wstring();
	});

	com_rate.editable(false);
	com_rate.push_back(" KB/s");
	com_rate.push_back(" MB/s");
	com_rate.option(conf.ratelim_unit);
	com_rate.events().selected([&]
	{
		conf.ratelim_unit = com_rate.option();
	});

	cbsplit.tooltip("Split into multiple files based on internal chapters. (--split-chapters)");
	cbkeyframes.tooltip("Force keyframes around the chapters before\nremoving/splitting them. Requires a\n"
		"reencode and thus is very slow, but the\nresulting video may have fewer artifacts\naround the cuts. (--force-keyframes-at-cuts)");
	cbtime.tooltip("Do not use the Last-modified header to set the file modification time (--no-mtime)");
	cbsubs.tooltip("Embed subtitles in the video (only for mp4, webm and mkv videos) (--embed-subs)");
	cbchaps.tooltip("Add chapter markers to the video file (--embed-chapters)");
	cbthumb.tooltip("Embed thumbnail in the video as cover art (--embed-thumbnail)");

	btndl.events().click([this] { on_btn_dl(); });

	btnbackto2.events().click([this]
	{
		if(link_yt && conf.vidinfo)
		{
			if(is_zoomed(true)) restore();
			get_place().field_display("page2", true);
			collocate();
		}
		else btnbackto1.events().click.emit({}, btnbackto1);
	});

	l_outpath.events().click([this]
	{
		std::wstring init {l_outpath.caption_wstring()};
		if(init.empty())
			init = util::get_sys_folder(FOLDERID_Downloads);
		nana::folderbox fb {*this, init};
		fb.allow_multi_select(false);
		fb.title("Locate and select the desired download folder");
		auto res {fb()};
		if(res.size())
		{
			conf.outpath = res.front();
			l_outpath.caption(conf.outpath.u8string());
		}
	});

	cbsplit.check(conf.cbsplit);
	cbchaps.check(conf.cbchaps);
	cbsubs.check(conf.cbsubs);
	cbthumb.check(conf.cbthumb);
	cbtime.check(conf.cbtime);
	cbkeyframes.check(conf.cbkeyframes);
	cbmp3.check(conf.cbmp3);
	cbargs.check(conf.cbargs);

	cbsplit.radio(true);
	cbchaps.radio(true);

	cbsplit.events().checked([this]
	{
		if(cbsplit.checked() && cbchaps.checked())
			cbchaps.check(false);
		conf.cbsplit = cbsplit.checked();
	});

	cbchaps.events().checked([this]
	{
		if(cbchaps.checked() && cbsplit.checked())
			cbsplit.check(false);
		conf.cbchaps = cbchaps.checked();
	});

	cbtime.events().checked([this] { conf.cbtime = cbtime.checked(); });
	cbthumb.events().checked([this] { conf.cbthumb = cbthumb.checked(); });
	cbsubs.events().checked([this] { conf.cbsubs = cbsubs.checked(); });
	cbkeyframes.events().checked([this] { conf.cbkeyframes = cbkeyframes.checked(); });
	cbmp3.events().checked([this] { conf.cbmp3 = cbmp3.checked(); });
	cbargs.events().checked([this] { conf.cbargs = cbargs.checked(); });

	plc3.collocate();
}


void GUI::settings_dlg()
{
	themed_form fm {nullptr, *this, nana::API::make_center(*this, dpi_transform(567, 500)), appear::decorate<appear::minimize>{}};
	fm.caption(title + " - settings");
	fm.bgcolor(theme.fmbg);
	fm.div(R"(vert margin=20
		<weight=25 <l_res weight=145> <weight=10> <com_res weight=52> <> <cbfps weight=196> <weight=54> > <weight=20>
		<weight=25
			<l_video weight=182> <weight=10> <com_video weight=60> <>
			<l_audio weight=182> <weight=10> <com_audio weight=60>
		>
		<weight=20> <weight=25 <cbinfo weight=380> > <weight=20>
		<separator_a weight=1> <weight=1> <separator_b weight=1> <weight=20>
		<weight=25 <l_theme weight=92> <weight=20> <cbtheme_dark weight=57> <weight=20> 
			<cbtheme_light weight=58> <weight=20> <cbtheme_system weight=152> > <weight=20>
		<weight=25 <l_contrast weight=64> <weight=20> <slider> > <weight=20>
		<separator_c weight=1> <weight=1> <separator_d weight=1>
		<weight=20> <gpupdate weight=133> <weight=20>
		<weight=35 <> <btn_whatever weight=170> <>>
	)");

	widgets::Label l_res {fm, &theme, "Preferred resolution:"}, l_latest {fm, &theme, "Latest version:"},
		l_video {fm, &theme, "Preferred video container:"}, l_audio {fm, &theme, "Preferred audio container:"},
		l_theme {fm, &theme, "Color theme:"}, l_contrast {fm, &theme, "Contrast:"};
	widgets::Combox com_res {fm, &theme}, com_video {fm, &theme}, com_audio {fm, &theme};
	widgets::cbox cbfps {fm, &theme, "Prefer a higher framerate"}, cbinfo {fm, &theme, "YouTube: show video info and list available formats"},
		cbtheme_dark {fm, &theme, "Dark"}, cbtheme_light {fm, &theme, "Light"}, cbtheme_system {fm, &theme, "System preference"};
	widgets::separator separator_a {fm, &theme}, separator_b {fm, &theme}, separator_c {fm, &theme}, separator_d {fm, &theme};
	widgets::Button btn_whatever {fm, &theme, " Whatever"};
	nana::paint::image img;
	img.open(arr_whatever_ico, sizeof arr_whatever_ico);
	btn_whatever.icon(img);
	btn_whatever.events().click([&] {fm.close(); });
	widgets::Slider slider {fm, &theme};
	{
		slider.maximum(30);
		slider.value(conf.contrast*100);
		slider.events().value_changed([&]
		{
			conf.contrast = static_cast<double>(slider.value()) / 100;
			theme.contrast(conf.contrast);
			fm.bgcolor(theme.fmbg);
			fm.refresh_widgets();
			gpopt.bgcolor(theme.gpbg);
			apply_theme(theme.is_dark());
			refresh_widgets();
		});
	}
	nana::radio_group rg;
	rg.add(cbtheme_dark);
	rg.add(cbtheme_light);
	if(system_supports_darkmode())
		rg.add(cbtheme_system);
	rg.on_checked([&, this](const nana::arg_checkbox &arg)
	{
		if(arg.widget->checked())
		{
			conf.cbtheme = rg.checked();
			if(conf.cbtheme == 2)
			{
				system_theme(true);
				fm.system_theme(true);
			}
			else
			{
				dark_theme(conf.cbtheme == 0);
				fm.dark_theme(conf.cbtheme == 0);
			}
		}
	});
	if(conf.cbtheme == 2)
	{
		if(system_supports_darkmode())
			cbtheme_system.check(true);
		else conf.cbtheme = 1;
	}
	if(conf.cbtheme == 0) cbtheme_dark.check(true);
	if(conf.cbtheme == 1)  cbtheme_light.check(true);

	widgets::Group gpupdate {fm, "Updater", &theme};

	fm["l_res"] << l_res;
	fm["com_res"] << com_res;
	fm["cbfps"] << cbfps;
	fm["cbinfo"] << cbinfo;
	fm["l_video"] << l_video;
	fm["com_video"] << com_video;
	fm["l_audio"] << l_audio;
	fm["com_audio"] << com_audio;
	fm["separator_a"] << separator_a;
	fm["separator_b"] << separator_b;
	fm["l_theme"] << l_theme;
	fm["cbtheme_dark"] << cbtheme_dark;
	fm["cbtheme_light"] << cbtheme_light;
	if(system_supports_darkmode())
		fm["cbtheme_system"] << cbtheme_system;
	fm["l_contrast"] << l_contrast;
	fm["slider"] << slider;
	fm["gpupdate"] << gpupdate;
	fm["separator_c"] << separator_c;
	fm["separator_d"] << separator_d;
	fm["btn_whatever"] << btn_whatever;

	cbinfo.tooltip("This allows manual selection of formats for YouTube videos.\n"
		"If unchecked, the formats are selected automatically, using\n"
		"the preferences defined above.");
	cbfps.tooltip("If several different formats have the same resolution,\ndownload the one with the highest framerate.");

	auto res_tip {"Download the best video with the largest resolution available that is\n"
		"not higher than the selected value. Resolution is determined using the\n"
		"smallest dimension, so this works correctly for vertical videos as well."};

	for(auto &opt : com_video_options)
		com_video.push_back(" " + nana::to_utf8(opt));
	com_video.option(conf.vidinfo);

	for(auto &opt : com_audio_options)
		com_audio.push_back(" " + nana::to_utf8(opt));
	com_audio.option(conf.pref_audio);

	for(auto &opt : com_res_options)
		com_res.push_back(" " + nana::to_utf8(opt));

	com_res.option(conf.pref_res);
	com_res.tooltip(res_tip);
	l_res.tooltip(res_tip);

	gpupdate.div(R"(vert margin=[15,20,20,20]
		<weight=30 <l_ver weight=104> <weight=10> <l_vertext> <weight=20> <btn_changes weight=150> >
		<weight=20>
		<weight=30 <btn_update weight=100> <weight=20> <prog> >
	)");

	widgets::Label l_ver {gpupdate, &theme, "Latest version:"};
	widgets::Text l_vertext {gpupdate, &theme, "checking..."};
	widgets::Button btn_changes {gpupdate, &theme, "Release notes"};
	btn_changes.typeface(nana::paint::font_info {"", 12, {800}});
	btn_changes.enabled(false);
	btn_changes.events().click([&] { changes_dlg(fm); });
	widgets::Button btn_update {gpupdate, &theme, "Update"};
	btn_update.typeface(nana::paint::font_info {"", 12, {800}});
	btn_update.enabled(false);
	widgets::Progress prog {gpupdate, &theme};

	gpupdate["l_ver"] << l_ver;
	gpupdate["l_vertext"] << l_vertext;
	gpupdate["btn_changes"] << btn_changes;
	gpupdate["btn_update"] << btn_update;
	gpupdate["prog"] << prog;

	com_res.option(conf.pref_res);
	com_video.option(conf.pref_video);
	com_audio.option(conf.pref_audio);
	cbfps.check(conf.pref_fps);
	cbinfo.check(conf.vidinfo);

	bool working {false};
	std::thread thr;

	btn_update.events().click([&, this]
	{
		static fs::path arc_path;
		arc_path = fs::temp_directory_path() / "ytdlp-interface.7z";
		if(btn_update.caption() == "Update")
		{
			btn_update.caption("Cancel");
			btn_update.cancel_mode(true);
			thr = std::thread {[&, this]
			{
				working = true;
				unsigned arc_size {releases[0]["assets"][0]["size"]}, progval {0};
				std::string arc_url {releases[0]["assets"][0]["browser_download_url"]};
				prog.amount(arc_size);
				prog.value(0);
				auto cb_progress = [&, this](unsigned prog_chunk)
				{
					progval += prog_chunk;
					auto total {util::int_to_filesize(arc_size, false)},
						pct {std::to_string(progval/(arc_size/100)) + '%'};
					prog.caption("downloading archive: " + pct + " of " + total);
					prog.value(progval);
				};
				util::dl_inet_res(arc_url, arc_path, &working, cb_progress);
				if(working)
				{
					btn_update.caption("Update");
					btn_update.fgcolor(btnbackto2.fgcolor());
					btn_update.scheme().activated = btnbackto2.scheme().activated;
					auto tempself {fs::temp_directory_path() / self_path.filename()};
					if(fs::exists(tempself))
						fs::remove(tempself);
					try
					{
						fs::copy_file(self_path, tempself);
						auto temp_7zlib {fs::temp_directory_path() / "7zxa.dll"};
						fs::remove(temp_7zlib);
						fs::copy_file(appdir / "7zxa.dll", temp_7zlib);
					}
					catch(fs::filesystem_error const &e) {
						nana::msgbox mbox {fm, "File copy error"};
						mbox.icon(nana::msgbox::icon_error);
						(mbox << e.what())();
						working = false;
						thr.detach();
						return;
					}
					std::wstring params {L"update \"" + arc_path.wstring() + L"\" \"" + appdir.wstring() + L"\""};
					ShellExecuteW(NULL, L"runas", tempself.wstring().data(), params.data(), NULL, SW_SHOW);
					working = false;
					fm.close();
					close();
				}
			}};
		}
		else
		{
			if(fs::exists(arc_path))
				fs::remove(arc_path);
			btn_update.caption("Update");
			btn_update.cancel_mode(false);
			prog.caption("");
			prog.value(0);
			working = false;
			thr.detach();
		}
	});

	fm.events().unload([&, this]
	{
		conf.pref_res = com_res.option();
		conf.pref_video = com_video.option();
		conf.pref_audio = com_audio.option();
		conf.pref_fps = cbfps.checked();
		conf.vidinfo = cbinfo.checked();
		working = false;
		if(thr.joinable())
			thr.detach();
	});

	auto display_version = [&, this]
	{
		if(releases.empty())
		{
			l_vertext.error_mode(true);
			l_vertext.caption("failed to connect to GitHub!");
			if(!inet_error.empty())
				l_vertext.tooltip(inet_error);
		}
		else
		{
			std::string tag_name {releases[0]["tag_name"]};
			std::string vertext;
			if(is_tag_a_new_version(tag_name))
			{
				vertext = tag_name + " (new version)";
				btn_update.enabled(true);
			}
			else vertext = tag_name + " (current)";
			l_vertext.caption(vertext);
			btn_changes.enabled(true);
		}
	};

	nana::timer t;
	t.interval(std::chrono::milliseconds {100});
	t.elapse([&, this]
	{
		if(!thr_releases.joinable())
		{
			display_version();
			t.stop();
		}
	});
	if(thr_releases.joinable())
		t.start();
	else display_version();

	fm.theme_callback([&, this](bool dark)
	{
		if(dark) theme.make_dark();
		else theme.make_light();
		fm.bgcolor(theme.fmbg);
	});

	fm.collocate();
	fm.show();
	hide();
	fm.modality();
	show();
}


void GUI::changes_dlg(nana::window parent)
{
	themed_form fm {nullptr, parent, nana::API::make_center(parent, dpi_transform(900, 445)), appear::decorate<appear::sizable>{}};
	fm.theme_callback([&, this](bool dark)
	{
		if(dark) theme.make_dark();
		else theme.make_light();
		fm.bgcolor(theme.fmbg);
	});
	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.caption("ytdlp-interface - release notes");
	fm.bgcolor(theme.fmbg);
	fm.div(R"(vert margin=20 <tb> <weight=20> <weight=25 <> <l_history weight=164> <weight=10> <com_history weight=55> <> >)");

	widgets::Textbox tb {fm, &theme};
	{
		fm["tb"] << tb;
		nana::API::effects_edge_nimbus(tb, nana::effects::edge_nimbus::none);
		tb.typeface(nana::paint::font_info {"Courier New", 12});
		tb.line_wrapped(true);
		tb.editable(false);
	}

	auto display_release_notes = [&](const unsigned ver)
	{
		std::string tag_name {releases[ver]["tag_name"]};
		std::string body {releases[ver]["body"]};
		body.erase(0, body.find('-'));
		std::string date {releases[ver]["published_at"]};
		date.erase(date.find('T'));
		auto title {tag_name + " (" + date + ")\n"};
		title += std::string(title.size()-1, '=') + "\n\n";
		tb.caption(title + body);
	};

	display_release_notes(0);

	widgets::Label l_history {fm, &theme, "Show release notes for:"};
	fm["l_history"] << l_history;

	widgets::Combox com_history {fm, &theme};
	{
		fm["com_history"] << com_history;
		com_history.editable(false);
		for(auto &release : releases)
		{
			std::string text {release["tag_name"]};
			com_history.push_back(" " + text);
		}
		com_history.option(0);
		com_history.events().selected([&]
		{
			display_release_notes(com_history.option());
		});
	}

	tb.events().mouse_wheel([&](const nana::arg_wheel &arg)
	{
		if(!tb.scroll_operation()->visible(true) && arg.which == nana::arg_wheel::wheel::vertical)
		{
			const auto idx_sel {com_history.option()}, idx_last {com_history.the_number_of_options() - 1};
			if(arg.upwards)
			{
				if(idx_sel == 0)
					com_history.option(idx_last);
				else com_history.option(idx_sel - 1);
			}
			else
			{
				if(idx_sel == idx_last)
					com_history.option(0);
				else com_history.option(idx_sel + 1);
			}
		}
	});

	fm.collocate();
	fm.show();
	fm.modality();
}


void GUI::apply_theme(bool dark)
{
	if(dark)
	{
		theme.make_dark();
		l_wait.caption("<bold color=0xcA9B8E size=20>DON'T MOVE\n\n</><bold color=0xddeeff size=14>getting video info...\n</>");
		tbpipe_overlay.fgcolor(nana::color {"#666"});
	}
	else
	{
		theme.make_light();
		l_wait.caption("<bold color=0xaa8888 size=20>DON'T MOVE\n\n</><bold color=0x556677 size=14>getting video info...\n</>");
		tbpipe_overlay.fgcolor(nana::color {"#ccd0d3"});
	}

	if(tbpipe.text_line_count())
	{
		auto ca {tbpipe.colored_area_access()};
		ca->clear();
		auto p {ca->get(0)};
		p->count = 1;
		p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
		if(!working)
		{
			p = ca->get(tbpipe.text_line_count() - 2);
			p->count = 1;
			p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
		}
		nana::API::refresh_window(tbpipe);
	}

	bgcolor(theme.fmbg);
	page1.bgcolor(theme.fmbg);
	page1a.bgcolor(theme.fmbg);
	page2.bgcolor(theme.fmbg);
	page3.bgcolor(theme.fmbg);
}


GUI::settings_t GUI::conf;