#include "gui.hpp"
#include "icons.hpp"

GUI::GUI() : form()
{
	using namespace nana;

	if(WM_TASKBAR_BUTTON_CREATED = RegisterWindowMessageW(L"TaskbarButtonCreated"))
	{
		sc.make_after(WM_TASKBAR_BUTTON_CREATED, [&](UINT, WPARAM, LPARAM, LRESULT*)
		{
			if(FAILED(i_taskbar.CoCreateInstance(CLSID_TaskbarList)))
				return false;

			if(i_taskbar->HrInit() != S_OK)
			{
				i_taskbar.Release();
				return false;
			}
			sc.clear();
			return true;
		});
	}

	std::wstring modpath(4096, '\0');
	modpath.resize(GetModuleFileNameW(0, &modpath.front(), modpath.size()));
	modpath.erase(modpath.rfind('\\'));
	appdir = modpath;

	if(std::filesystem::exists(appdir / "ffmpeg.exe"))
		ffmpeg_loc = appdir / "ffmpeg.exe";
	else if(!conf.ytdlp_path.empty())
	{
		auto tmp {std::filesystem::path {conf.ytdlp_path}.replace_filename("ffmpeg.exe")};
		if(std::filesystem::exists(tmp))
			ffmpeg_loc = tmp;
	}

	hwnd = reinterpret_cast<HWND>(native_handle());

	center(630, 193);
	caption("ytdlp-interface v1.2");
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

	events().unload([&]
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


void GUI::on_btn_next()
{
	using namespace nana;
	auto id {tblink.caption()};
	if(id.find("youtu.be") != -1)
		id = id.substr(id.rfind('/')+1);
	else id = id.substr(id.rfind("?v=")+3);
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

			auto it {std::find_if(vidinfo["thumbnails"].begin(), vidinfo["thumbnails"].end(), [](const auto &el)
			{
				return std::string{el["url"]}.rfind("mqdefault.jpg") != -1;
			})};
			std::string thumb_url {(*it)["url"]};

			l_wait.cursor(cursor::arrow);
			cursor(cursor::arrow);
			std::string title {vidinfo["title"]};
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

			auto thumb_jpg {get_inet_res(thumb_url)};
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
			}
			if(is_zoomed(true)) restore();
			if(API::screen_dpi(true) > 96)
				maximize_v(1000);
			else center(1000, 384.0 + lbformats.item_count()*20.4);
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
		else center(1000, 384.0 + lbformats.item_count()*20.4);
		get_place().field_display("page2", true);
		collocate();
	}
}


void GUI::on_btn_dl()
{
	if(btndl.caption().find("Stop") == -1)
	{
		btndl.caption("Stop download");
		btndl.fgcolor(nana::color {"#966"});
		btndl.scheme().activated = nana::color {"#a77"};

		std::wstring cmd {L'\"' + conf.ytdlp_path.wstring() + L'\"'};
		if(link_yt && conf.vidinfo) cmd += L" -f " + strfmt;
		else
		{
			cmd += L" -S \"res:" + com_res_options[conf.pref_res];
			if(conf.pref_video)
				cmd += L",vcodec:" + com_video_options[conf.pref_video];
			if(conf.pref_audio)
				cmd += L",acodec:" + com_audio_options[conf.pref_audio];
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
			nana::API::refresh_window(tbpipe);
			unsigned fmtnum {0};
			auto sel {lbformats.selected()};
			ULONGLONG prev_val {0};
			auto cb_progress = [&, this](ULONGLONG completed, ULONGLONG total, std::string text)
			{
				auto fmt {nana::to_utf8(fmtnum ? conf.fmt2 : conf.fmt1)};
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
						if(link_yt && conf.vidinfo)
							prog.caption(text + " (format " + fmt + ")");
						else prog.caption(text);
					}
					else if(link_yt && conf.vidinfo)
						prog.caption("Format " + fmt + " has already been downloaded!");
					else prog.caption("File has already been downloaded!");
					fmtnum++;
				}
				prog.value(completed);
				prev_val = completed;
			};
			prog.value(0);
			prog.caption("");
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
			nana::API::refresh_window(tbpipe);
			btndl.caption("Begin download");
			btndl.fgcolor(btnbackto2.fgcolor());
			btndl.scheme().activated = btnbackto2.scheme().activated;
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
		tbpipe.append("\n[GUI] yt-dlp.exe process was ended\n", false);
		auto ca {tbpipe.colored_area_access()};
		auto p {ca->get(tbpipe.text_line_count()-2)};
		p->count = 1;
		p->fgcolor = nana::color {"#832"};
		nana::API::refresh_window(tbpipe);
		btnbackto2.enabled(true);
		btndl.enabled(true);
		btndl.caption("Begin download");
		btndl.fgcolor(btnbackto2.fgcolor());
		btndl.scheme().activated = btnbackto2.scheme().activated;
	}
}


void GUI::make_page1()
{
	using namespace nana;

	plc1.div(R"(
			vert <weight=25 <l_ytdlp weight=164> <weight=15> <l_path> <weight=15> <btn_path weight=25> >
				<weight=20>
				<weight=25 < l_ytlink weight=164> <weight=15> <tblink> >
				<weight=20> <separator_a weight=1> <weight=1> <separator_b weight=1> <weight=20>
				<weight=35 <> <btn_settings weight=152> <weight=20> <btn_next weight=180> <> >
		)");

	plc1["l_ytdlp"] << l_ytdlp;
	plc1["l_path"] << l_path;
	plc1["btn_path"] << btn_path;
	plc1["l_ytlink"] << l_ytlink;
	plc1["tblink"] << tblink;
	plc1["btn_settings"] << btn_settings;
	plc1["btn_next"] << btn_next;
	plc1["separator_a"] << separator_a;
	plc1["separator_b"] << separator_b;

	l_path.bgcolor(color {"#eef6ee"});
	l_path.fgcolor(color {"#354"});
	l_path.typeface(paint::font_info {"Tahoma", 10});
	l_path.text_align(align::center, align_v::center);
	l_path.events().resized([this]
	{
		label_path_caption(l_path, conf.ytdlp_path);
	});

	paint::image img;
	img.open(arr_settings_ico, sizeof arr_settings_ico);
	btn_settings.icon(img);
	btn_path.bgcolor(colors::white);
	separator_a.bgcolor(color {"#d6d6d6"});
	separator_b.bgcolor(color {"#d6d6d6"});

	tblink.typeface(paint::font_info {"", 11, {800}});
	tblink.fgcolor(color {"#789"});
	tblink.text_align(align::center);
	tblink.editable(false);
	tblink.line_wrapped(false);
	tblink.multi_lines(false);
	tblink.caption("Click to paste video link");
	API::effects_edge_nimbus(tblink, effects::edge_nimbus::none);

	tblink.events().mouse_up([this]
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
							tblink.caption(ptext);
						GlobalUnlock(h);
					}
				}
			}
			CloseClipboard();
		}
	});

	tblink.events().text_changed([this]
	{
		btn_next.enabled(!l_path.caption().empty());
		link_yt = is_ytlink(tblink.text());
		btn_next_morph();
	});

	btn_next.enabled(false);

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
				btn_next.enabled(!tblink.empty());
				auto tmp {std::filesystem::path {conf.ytdlp_path}.replace_filename("ffmpeg.exe")};
				if(std::filesystem::exists(tmp))
					ffmpeg_loc = tmp;
			}
		}
	};

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
	btn_path.events().click(find_ytdlp);
	l_path.events().click(find_ytdlp);
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
	lbformats.append_header("acodec", dpi_transform(90).width);
	lbformats.append_header("vcodec", dpi_transform(90).width);
	lbformats.append_header("ext", dpi_transform(50).width);
	lbformats.append_header("fps", dpi_transform(32).width);
	lbformats.append_header("vbr", dpi_transform(40).width);
	lbformats.append_header("abr", dpi_transform(40).width);
	lbformats.append_header("asr", dpi_transform(50).width);
	lbformats.append_header("filesize", dpi_transform(160).width);
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


void GUI::make_page3()
{
	using namespace nana;

	plc3.div(R"(
			vert <tbpipe> <weight=20>
			<prog weight=30> <weight=20>
			<gpopt weight=175> <weight=20>
			<weight=35 <> <btnbackto2 weight=160> <weight=20> <btndl weight=200> <>>
		)");
	plc3["tbpipe"] << tbpipe;
	plc3["prog"] << prog;
	plc3["gpopt"] << gpopt;
	plc3["btnbackto2"] << btnbackto2;
	plc3["btndl"] << btndl;

	prog.amount(1000);
	prog.caption("");

	tbpipe.editable(false);
	tbpipe.line_wrapped(true);
	tbpipe.scheme().mouse_wheel.lines = 3;
	API::effects_edge_nimbus(tbpipe, effects::edge_nimbus::none);

	plcopt.bind(gpopt);
	page3.bgcolor(colors::white);
	gpopt.bgcolor(colors::white);
	gpopt.enable_format_caption(true);
	gpopt.caption("<bold color=0x81544F size=11 font=\"Tahoma\"> Options </>");
	gpopt.caption_align(align::center);
	gpopt.size({10, 10}); // workaround for weird caption display bug
	gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=122> <weight=15> <l_outpath> <weight=15> <btn_outpath weight=25> > <weight=20>
			<weight=25 
				<l_rate weight=144> <weight=15> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbchaps weight=133> <> <cbsplit weight=116> <> <cbkeyframes weight=186>
			> <weight=20>
			<weight=25 <cbtime weight=296> <> <cbthumb weight=144> <> <cbsubs weight=132> <> <cbmp3 weight=173>>
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
	gpopt["cbchaps"] << cbchaps;
	gpopt["cbsubs"] << cbsubs;
	gpopt["cbthumb"] << cbthumb;
	gpopt["cbtime"] << cbtime;

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
	l_outpath.events().resized([this]
	{
		label_path_caption(l_outpath, conf.outpath);
	});

	btn_outpath.bgcolor(colors::white);
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

	cbsplit.check(conf.cbsplit);
	cbchaps.check(conf.cbchaps);
	cbsubs.check(conf.cbsubs);
	cbthumb.check(conf.cbthumb);
	cbtime.check(conf.cbtime);
	cbkeyframes.check(conf.cbkeyframes);
	cbmp3.check(conf.cbmp3);

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

	plc3.collocate();
}


void GUI::settings_dlg()
{
	using namespace nana;

	form fm {*this, API::make_center(*this, dpi_transform(567, 233)), appear::decorate<appear::minimize>{}};
	fm.caption("ytdlp-interface settings");
	fm.bgcolor(colors::white);
	fm.div(R"(vert margin=20
		<weight=25 <l_res weight=145> <weight=10> <com_res weight=52> <> <cbfps weight=196> <weight=54> > <weight=20>
		<weight=25
			<l_video weight=182> <weight=10> <com_video weight=60> <>
			<l_audio weight=182> <weight=10> <com_audio weight=60>
		>
		<weight=20> <weight=25 <cbinfo weight=380> > <weight=20>
		<separator_a weight=1> <weight=1> <separator_b weight=1>
		<weight=20> <weight=35 <> <btn_whatever weight=170> <>>
	)");

	Label l_res {fm, "Preferred resolution:"}, l_latest {fm, "Latest version:"},
		l_video {fm, "Preferred video container:"}, l_audio {fm, "Preferred audio container:"};
	combox com_res {fm}, com_video {fm}, com_audio {fm};
	cbox cbfps {fm, "Prefer a higher framerate"}, cbinfo {fm, "YouTube: show video info and list available formats"};
	nana::label separator_a {fm}, separator_b {fm};
	Button btn_whatever {fm, " Whatever"};
	paint::image img;
	img.open(arr_whatever_ico, sizeof arr_whatever_ico);
	btn_whatever.icon(img);
	btn_whatever.events().click([&] {fm.close(); });

	separator_a.bgcolor(color {"#d6d6d6"});
	separator_b.bgcolor(color {"#d6d6d6"});

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
	fm["btn_whatever"] << btn_whatever;

	cbinfo.tooltip("This allows manual selection of formats for YouTube videos.\n"
		"If unchecked, the formats are selected automatically, using\n"
		"the preferences defined above.");
	cbfps.tooltip("If several different formats have the same resolution,\ndownload the one with the highest framerate.");

	auto res_tip {"Download the best video with the largest resolution available that is\n"
		"not higher than the selected value. Resolution is determined using the\n"
		"smallest dimension, so this works correctly for vertical videos as well."};

	for(auto &opt : com_video_options)
		com_video.push_back(" " + to_utf8(opt));
	com_video.option(conf.vidinfo);

	for(auto &opt : com_audio_options)
		com_audio.push_back(" " + to_utf8(opt));
	com_audio.option(conf.pref_audio);

	for(auto &opt : com_res_options)
		com_res.push_back(" " + to_utf8(opt));
	com_res.option(conf.pref_res);
	com_res.tooltip(res_tip);
	l_res.tooltip(res_tip);

	com_res.option(conf.pref_res);
	com_video.option(conf.pref_video);
	com_audio.option(conf.pref_audio);
	cbfps.check(conf.pref_fps);
	cbinfo.check(conf.vidinfo);

	fm.events().unload([&, this]
	{
		conf.pref_res = com_res.option();
		conf.pref_video = com_video.option();
		conf.pref_audio = com_audio.option();
		conf.pref_fps = cbfps.checked();
		conf.vidinfo = cbinfo.checked();
	});

	fm.collocate();
	fm.show();
	hide();
	fm.modality();
	show();
}


GUI::settings_t GUI::conf;