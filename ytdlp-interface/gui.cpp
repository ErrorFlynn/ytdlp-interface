#include "gui.hpp"
#include "icons.hpp"

#include <nana/gui/filebox.hpp>
#include <nana/system/platform.hpp>


GUI::GUI() : themed_form {std::bind(&GUI::apply_theme, this, std::placeholders::_1)}
{
	using namespace nana;

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

	msg.make_after(WM_COPYDATA, [&](UINT, WPARAM, LPARAM lparam, LRESULT*)
	{
		auto pcds {reinterpret_cast<PCOPYDATASTRUCT>(lparam)};
		std::wstring url {reinterpret_cast<LPCWSTR>(pcds->lpData), pcds->cbData / 2};
		auto item {lbq.item_from_value(url)};
		if(!item.empty())
		{
			switch(pcds->dwData)
			{
			case YTDLP_POSTPROCESS:
				if(bottoms.at(url).started())
					item.text(3, "processing");
				else return true;
				if(conf.cb_lengthyproc && bottoms.contains(url))
				{
					auto &bottom {bottoms.at(url)};
					if(!bottom.received_procmsg)
					{
						bottom.received_procmsg = true;
						bottom.timer_proc.interval(conf.max_proc_dur);
						bottom.timer_proc.elapse([&, url, this]
						{
							if(bottoms.contains(url))
							{
								bottom.timer_proc.stop();
								auto next_url {next_startable_url(url)};
								if(!next_url.empty())
									on_btn_dl(next_url);
							}
						});
						bottom.timer_proc.start();
					}
				}
				break;

			case YTDLP_DOWNLOAD:
				item.text(3, "downloading");
				break;
			}
		}
		return true;
	});

	msg.make_after(WM_ENDSESSION, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam) close();
		return true;
	});

	msg.make_after(WM_POWERBROADCAST, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == PBT_APMSUSPEND) close();
		return true;
	});

	msg.make_after(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == 'V' && GetAsyncKeyState(VK_CONTROL) & 0xff00)
		{
			if(api::focus_window() != bottoms.current().com_args)
			{
				l_url.events().mouse_up.emit({}, l_url);
				btnadd.focus();
				if(!queue_panel.visible())
					show_queue(true);
			}
		}
		else if(wparam == VK_DELETE)
		{
			auto fwnd {api::focus_window()};
			auto &bottom {bottoms.current()};
			if(fwnd != bottom.com_args && fwnd != bottom.tbrate && !lbq.selected().empty())
			{
				outbox.clear();
				remove_queue_item(bottom.url);
			}
		}
		else if(wparam == VK_APPS)
		{
			auto curpos {api::cursor_position()};
			api::calc_window_point(*this, curpos); 
			auto pos {btn_qact.pos()};
			pop_queue_menu(pos.x, pos.y);
		}
		return true;
	});

	msg.make_before(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == VK_UP)
		{
			if(lbq.at(0).size() > 1)
			{
				lbq.move_select(true);
				return false;
			}
		}
		else if(wparam == VK_DOWN)
		{
			if(lbq.at(0).size() > 1)
			{
				lbq.move_select(false);
				return false;
			}
		}
		else if(wparam == VK_ESCAPE)
		{
			close();
			return false;
		}
		return true;
	});

	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	number_of_processors = sysinfo.dwNumberOfProcessors;

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

	::widgets::theme.contrast(conf.contrast);

	if(conf.cbtheme == 2)
	{
		if(system_supports_darkmode())
			system_theme(true);
		else conf.cbtheme = 1;
	}
	else if(conf.cbtheme == 0) dark_theme(true);
	if(conf.cbtheme == 1) dark_theme(false);

	get_releases();
	caption(title);
	make_form();

	if(system_theme())
		apply_theme(is_system_theme_dark());
	else apply_theme(dark_theme());

	get_versions();

	events().unload([&]
	{
		working = menu_working = false;
		if(thr_menu.joinable())
			thr_menu.join();
		if(thr.joinable())
			thr.join();
		if(thr_releases.joinable())
			thr_releases.detach();
		if(thr_releases_misc.joinable())
			thr_releases_misc.detach();
		if(thr_versions.joinable())
			thr_versions.detach();
		for(auto &bottom : bottoms)
		{
			auto &bot {*bottom.second};
			if(bot.info_thread.joinable())
			{
				bot.working_info = false;
				bot.info_thread.detach();
			}
			if(bot.dl_thread.joinable())
			{
				bot.working = false;
				bot.dl_thread.join();
			}
		}
		if(i_taskbar)
			i_taskbar.Release();

		conf.unfinished_queue_items.clear();
		for(auto item : lbq.at(0))
		{
			auto text {item.text(3)};
			if(text != "done" && text != "error")
				conf.unfinished_queue_items.push_back(to_utf8(item.value<std::wstring>()));
		}			
	});

	for(auto &url : conf.unfinished_queue_items)
		add_url(to_wstring(url));

	if(conf.cb_queue_autostart && lbq.at(0).size())
		on_btn_dl(lbq.at(0).at(0).value<std::wstring>());

	if(is_zoomed(true)) restore();
	center(1000, 800);
	api::track_window_size(*this, size(), false);
	collocate();
	show_queue(false);
}


void GUI::formats_dlg()
{
	using namespace nana;
	using ::widgets::theme;
	auto url {bottoms.visible()};
	auto id {to_utf8(url)};
	if(id.find("youtu.be") != -1)
		id = id.substr(id.rfind('/')+1, 11);
	else id = id.substr(id.rfind("?v=")+3, 11);
	auto &bottom {bottoms.at(url)};
	auto &vidinfo {bottom.vidinfo};

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.caption(title + " - YouTube video manual selection of formats");
	fm.bgcolor(theme.fmbg);
	fm.div(R"(
			vert margin=20 <weight=180px 
					<thumb weight=320px> <weight=20px>
					<vert
						<l_title weight=27px> <weight=10px>
						<sep1 weight=3px> <weight=10px>
						<weight=28px <l_dur weight=45%> <weight=10> <l_durtext> >
						<weight=28px <l_chap weight=45%> <weight=10> <l_chaptext> >
						<weight=28px <l_upl weight=45%> <weight=10> <l_upltext> >
						<weight=28px <l_date weight=45%> <weight=10> <l_datetext> > <weight=15px>
						<sep2 weight=3px>
					>
				 >
				<weight=20> <lbformats> <weight=20>
				<weight=35 <><btncancel weight=420> <weight=20> <btnok weight=420><>>
		)");

	::widgets::Title l_title {fm};
	::widgets::Label l_dur {fm, "Duration:", true}, l_chap {fm, "Chapters:", true},
		l_upl {fm, "Uploader:", true}, l_date {fm, "Upload date:", true};
	::widgets::Text l_durtext {fm, "", true}, l_chaptext {fm, "", true}, l_upltext {fm, "", true}, l_datetext {fm, "", true};
	nana::picture thumb {fm};
	::widgets::Separator sep1 {fm}, sep2 {fm};
	::widgets::Listbox lbformats {fm};
	::widgets::Button btnok {fm, "Use the selected format(s)"}, btncancel {fm, "Let yt-dlp choose the best formats (default)"};

	fm["l_title"] << l_title;
	fm["l_dur"] << l_dur;
	fm["l_durtext"] << l_durtext;
	fm["l_chap"] << l_chap;
	fm["l_chaptext"] << l_chaptext;
	fm["l_upl"] << l_upl;
	fm["l_upltext"] << l_upltext;
	fm["l_date"] << l_date;
	fm["l_datetext"] << l_datetext;
	fm["thumb"] << thumb;
	fm["sep1"] << sep1;
	fm["sep2"] << sep2;
	fm["lbformats"] << lbformats;
	fm["btncancel"] << btncancel;
	fm["btnok"] << btnok;

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

	lbformats.events().selected([&, this](const arg_listbox &arg)
	{
		if(arg.item.selected())
		{
			btnok.enabled(true);
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
			btnok.enabled(false);
	});

	btncancel.events().click([&, this]
	{
		auto &bottom {bottoms.current()};
		bottom.use_strfmt = false;
		fm.close();
	});

	btnok.events().click([&, this]
	{
		auto &bottom {bottoms.current()};
		auto &strfmt {bottom.strfmt};
		auto sel {lbformats.selected()};
		strfmt = lbformats.at(sel.front()).value<std::wstring>();
		bottom.fmt1 = conf.fmt1 = strfmt;
		if(sel.size() > 1)
		{
			bottom.fmt2 = conf.fmt2 = lbformats.at(sel.back()).value<std::wstring>();
			strfmt += L'+' + conf.fmt2;
		}
		else
		{
			bottom.fmt2.clear();
			conf.fmt2.clear();
		}
		bottom.use_strfmt = true;
		fm.close();
	});

	btnok.enabled(false);

	auto it {std::find_if(vidinfo["thumbnails"].begin(), vidinfo["thumbnails"].end(), [](const auto &el)
	{
		return std::string{el["url"]}.rfind("mqdefault.jpg") != -1;
	})};

	if(it == vidinfo["thumbnails"].end())
		it = std::find_if(vidinfo["thumbnails"].begin(), vidinfo["thumbnails"].end(), [](const auto &el)
		{
			return std::string {el["url"]}.rfind("mqdefault_live.jpg") != -1;
		});

	std::string thumb_url, title {vidinfo["title"]};
	if(it != vidinfo["thumbnails"].end())
		thumb_url = (*it)["url"];
	l_title.caption(title);

	int dur {0};
	if(bottom.vidinfo.contains("is_live") && bottom.vidinfo["is_live"] ||
	   bottom.vidinfo.contains("live_status") && bottom.vidinfo["live_status"] == "is_live")
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

	thr_thumb = std::thread([&]
	{
		thumbthr_working = true;
		auto thumb_jpg {util::get_inet_res(thumb_url)};
		if(!thumb_jpg.empty())
		{
			paint::image img;
			img.open(thumb_jpg.data(), thumb_jpg.size());
			if(thumbthr_working)
				thumb.load(img);
			thumbthr_working = false;
			if(thr_thumb.joinable())
				thr_thumb.detach();
		}
	});	

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
			if(!bottom.fmt1.empty() || !bottom.fmt2.empty())
			{
				if(idstr == bottom.fmt1 || idstr == bottom.fmt2)
					lbformats.at(catidx).back().select(true);
			}
			else if(idstr == conf.fmt1 || idstr == conf.fmt2)
				lbformats.at(catidx).back().select(true);
		}
	}

	fm.events().unload([&]
	{
		if(thr_thumb.joinable())
		{
			thumbthr_working = false;
			thr_thumb.detach();
		}
	});

	fm.theme_callback([&, this](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme.fmbg);
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.center(1000, max(384.0 + lbformats.item_count() * 20.4, 800));

	fm.collocate();
	fm.show();
	fm.modality();
}


bool GUI::process_queue_item(std::wstring url)
{
	if(lbq.item_from_value(url).text(3) == "error") return false;
	using widgets::theme;
	auto &bottom {bottoms.at(url)};
	auto &tbpipe {outbox};
	auto &tbpipe_overlay {overlay};
	auto &com_args {bottom.com_args};
	auto &com_rate {bottom.com_rate};
	auto &tbrate {bottom.tbrate};
	auto &btn_ytfmtlist {bottom.btn_ytfmtlist};
	auto &cbmp3 {bottom.cbmp3};
	auto &cbsplit {bottom.cbsplit};
	auto &cbkeyframes {bottom.cbkeyframes};
	auto &btndl {bottom.btndl};
	auto &prog {bottom.prog};
	auto &working {bottom.working};
	auto &graceful_exit {bottom.graceful_exit};
	const auto argset {com_args.caption_wstring()};

	if(!bottom.started())
	{
		bottom.btndl.caption("Stop download");
		bottom.btndl.cancel_mode(true);
		if(tbpipe.current() == url)
		{
			tbpipe.clear();
			if(tbpipe_overlay.visible() && bottom.btnq.caption().find("queue") != -1)
			{
				tbpipe.show(url);
			}
		}
		else tbpipe.clear(url);
		if(conf.cbargs)
		{
			const auto args {bottom.com_args.caption()};
			auto idx {com_args.caption_index()};
			if(idx == -1)
			{
				const auto size {com_args.the_number_of_options()};
				com_args.push_back(args);
				com_args.option(size);
				conf.com_args = size;
				if(size >= 10)
				{
					com_args.erase(0);
					conf.argsets.erase(conf.argsets.begin());
				}
				conf.argsets.push_back(args);
			}
			else conf.com_args = idx;
			com_args.option(idx);
			if(conf.common_dl_options)
				bottoms.propagate_args_options(bottom);
		}

		std::wstring cmd {L'\"' + conf.ytdlp_path.wstring() + L'\"'};
		if(!conf.cbargs || com_args.caption_wstring().find(L"-f ") == -1)
		{
			if(bottom.use_strfmt) cmd += L" -f " + bottom.strfmt;
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
			}
		}
		cmd += L" --windows-filenames ";

		if(conf.cbchaps && argset.find(L"--embed-chapters") == -1)
			cmd += L"--embed-chapters ";
		if(conf.cbsubs && argset.find(L"--embed-subs") == -1)
			cmd += L"--embed-subs ";
		if(conf.cbthumb && argset.find(L"--embed-thumbnail") == -1)
			cmd += L"--embed-thumbnail ";
		if(conf.cbtime && argset.find(L"--no-mtime") == -1)
			cmd += L"--no-mtime ";
		if(conf.cbkeyframes && argset.find(L"--force-keyframes-at-cuts") == -1)
			cmd += L"--force-keyframes-at-cuts ";
		if(!ffmpeg_loc.empty() && ffmpeg_loc.parent_path() != self_path.parent_path())
			if(argset.find(L"--ffmpeg-location") == -1)
				cmd += L"--ffmpeg-location \"" + ffmpeg_loc.wstring() + L"\" ";
		if(tbrate.to_double() && argset.find(L"-r ") == -1)
			cmd += L"-r " + tbrate.caption_wstring() + (com_rate.option() ? L"M " : L"K ");
		if(cbmp3.checked() && argset.find(L"--audio-format") == -1)
		{
			cmd += L"-x --audio-format mp3 ";
			if(argset.find(L"--audio-quality") == -1) 
				L"--audio-quality 0 ";
		}
		if(cbsplit.checked() && argset.find(L"--split-chapters") == -1)
			cmd += L"--split-chapters -o chapter:\"" + bottom.outpath.wstring() + L"\\%(title)s - %(section_number)s -%(section_title)s.%(ext)s\" ";
		if(cbkeyframes.checked() && argset.find(L"--force-keyframes-at-cuts") == -1)
			cmd += L"--force-keyframes-at-cuts ";
		if(conf.cbargs && !argset.empty())
			cmd += argset + L" ";

		auto display_cmd {cmd};

		std::wstringstream ss;
		ss << hwnd;
		auto strhwnd {ss.str()};
		strhwnd.erase(0, strhwnd.find_first_not_of('0'));
		cmd += L"--exec \"after_video:\\\"" + self_path.wstring() + L"\\\" ytdlp_status " + std::to_wstring(YTDLP_POSTPROCESS)
			+ L" " + strhwnd + L" \\\"" + url + L"\\\"\" ";
		cmd += L"--exec \"before_dl:\\\"" + self_path.wstring() + L"\\\" ytdlp_status " + std::to_wstring(YTDLP_DOWNLOAD)
			+ L" " + strhwnd + L" \\\"" + url + L"\\\"\" ";
		std::wstring cmd2;
		if(!conf.cbargs || argset.find(L"-P ") == -1)
		{
			auto wstr {bottom.outpath.wstring()};
			if(wstr.find(' ') == -1)
				cmd2 += L" -P " + wstr;
			else cmd2 += L" -P \"" + wstr + L"\"";
		}
		if((!conf.cbargs || argset.find(L"-o ") == -1) && !conf.output_template.empty())
			cmd2 += L" -o " + conf.output_template;
		cmd2 += L" " + url;
		display_cmd += cmd2;
		cmd += cmd2;

		if(tbpipe.current() == url)
			tbpipe.clear();

		bottom.dl_thread = std::thread([&, this, display_cmd, cmd, url]
		{
			working = true;
			auto ca {tbpipe.colored_area_access()};
			if(outbox.current() == url)
				ca->clear();
			if(fs::exists(conf.ytdlp_path))
				tbpipe.append(url, L"[GUI] executing command line: " + display_cmd + L"\n\n"/*, false*/);
			else tbpipe.append(url, L"ytdlp.exe not found: " + conf.ytdlp_path.wstring()/*, false*/);
			auto p {ca->get(0)};
			p->count = 1;
			if(!fs::exists(conf.ytdlp_path))
			{
				p->fgcolor = theme.is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
				nana::API::refresh_window(tbpipe);
				btndl.caption("Start download");
				btndl.cancel_mode(false);
				if(bottom.dl_thread.joinable())
					bottom.dl_thread.detach();
				return;
			}
			p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
			nana::API::refresh_window(tbpipe);
			ULONGLONG prev_val {0};

			auto cb_progress = [&, this](ULONGLONG completed, ULONGLONG total, std::string text)
			{
				lbq.item_from_value(url).text(3, (std::stringstream {} << static_cast<double>(completed) / 10).str() + '%');
				if(i_taskbar && lbq.at(0).size() == 1)
					i_taskbar->SetProgressValue(hwnd, completed, total);
				if(completed < 1000)
				{
					prog.caption(text);
				}
				else
				{
					if(prev_val)
					{
						if(text == "[ExtractAudio]" || text == "[Merger]" || text == "[FixupM3u8]")
						{
							COPYDATASTRUCT cds;
							cds.dwData = YTDLP_POSTPROCESS;
							cds.cbData = url.size() * 2;
							cds.lpData = const_cast<std::wstring&>(url).data();
							SendMessageW(hwnd, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));
						}
						else
						{
							auto pos {text.find_last_not_of(" \t")};
							if(text.size() > pos)
								text.erase(pos + 1);
							prog.caption(text);
						}
					}
				}
				prog.value(completed);
				prev_val = completed;
			};

			auto item {lbq.item_from_value(url)};
			prog.value(0);
			prog.caption("");
			if(i_taskbar && lbq.at(0).size() == 1) i_taskbar->SetProgressState(hwnd, TBPF_NORMAL);
			item.text(3, "started");
			auto cb_append = [url, this](std::string text, bool keyword)
			{
				if(keyword)
					outbox.set_keyword(text);
				else outbox.append(url, text);
			};
			util::run_piped_process(cmd, &working, cb_append, cb_progress, &graceful_exit);
			bottom.received_procmsg = false;

			if(working)
			{
				if(bottom.timer_proc.started())
					bottom.timer_proc.stop();
				item.text(3, "done");
				taskbar_overall_progress();
				if(i_taskbar && lbq.at(0).size() == 1)
					i_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
				btndl.enabled(true);
				tbpipe.append(url, "\n[GUI] yt-dlp.exe process has exited\n"/*, false*/);
				if(tbpipe.current() == url)
				{
					ca = tbpipe.colored_area_access();
					p = ca->get(tbpipe.text_line_count() - 2);
					p->count = 1;
					p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
					nana::API::refresh_window(tbpipe);
				}
				btndl.caption("Start download");
				btndl.cancel_mode(false);
				auto next_url {next_startable_url(url)};
				if(!next_url.empty())
					on_btn_dl(next_url);
			}
			if(working && bottom.dl_thread.joinable())
				bottom.dl_thread.detach();
		});
		return true; // started
	}
	else
	{
		btndl.enabled(false);
		if(bottom.timer_proc.started())
			bottom.timer_proc.stop();
		if(bottom.dl_thread.joinable())
		{
			working = false;
			bottom.dl_thread.join();
		}
		bottom.received_procmsg = false;
		if(graceful_exit)
			tbpipe.append(url, "\n[GUI] yt-dlp.exe process was ended gracefully via Ctrl+C signal\n");
		else tbpipe.append(url, "\n[GUI] yt-dlp.exe process was ended forcefully via WM_CLOSE message\n");
		auto item {lbq.item_from_value(url)};
		auto text {item.text(3)};
		if(text.find('%') != -1)
			item.text(3, "stopped (" + text + ")");
		else item.text(3, "stopped");
		auto ca {tbpipe.colored_area_access()};
		auto p {ca->get(tbpipe.text_line_count()-2)};
		p->count = 1;
		if(graceful_exit)
			p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
		else p->fgcolor = theme.is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
		nana::API::refresh_window(tbpipe);
		btndl.enabled(true);
		btndl.caption("Start download");
		btndl.cancel_mode(false);
		if(conf.cb_autostart)
		{
			auto next_url {next_startable_url(url)};
			if(!next_url.empty())
				on_btn_dl(next_url);
		}
		return false; // stopped
	}
}


void GUI::add_url(std::wstring url)
{
	auto it {std::find_if(lbq.at(0).begin(), lbq.at(0).end(), [&, this](auto &el)
	{
		std::wstring val;
		try { val = el->value<std::wstring>(); }
		catch(std::runtime_error) { return false; }
		return val == url;
	})};

	if(it == lbq.at(0).end())
	{
		auto stridx {std::to_string(lbq.at(0).size() + 1)};
		lbq.at(0).append({stridx, "...", "...", "queued"});
		lbq.at(0).back().value(url);
		adjust_lbq_headers();

		auto &bottom {bottoms.add(url)};
		if(conf.common_dl_options)
			bottom.gpopt.caption("Download options");
		else bottom.gpopt.caption("Download options for queue item #" + stridx);
		bottom.show_btncopy(!conf.common_dl_options);
		if(lbq.at(0).size() == 1)
			lbq.item_from_value(url).select(true);
		else if(!conf.common_dl_options)
			bottom.show_btncopy(true);

		bottom.info_thread = std::thread([&, this, url]
		{
			bottom.working_info = true;
			static std::atomic_int active_threads {0};
			while(active_threads >= number_of_processors)
			{
				Sleep(50);
				if(!bottom.working_info)
				{
					if(bottom.info_thread.joinable())
						bottom.info_thread.detach();
					return;
				}
			}
			active_threads++;

			std::string media_info, media_website, media_title;
			bool is_ytplaylist {bottom.is_ytlink && (url.find(L"?list=") != -1 || url.find(L"&list=") != -1)};
			if(bottom.is_ytlink)
			{
				if(is_ytplaylist)
				{
					media_info = util::run_piped_process(conf.ytdlp_path.wstring() +
						L" --no-warnings --flat-playlist --print :%(webpage_url_domain)s:%(playlist_title)s " + url, &bottom.working_info);
					media_info.erase(media_info.find('\n'));
				}
				else
				{
					media_info = util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + L" --no-warnings -j " + url,
															&bottom.working_info);
					if(bottom.working_info)
					{
						auto pos {media_info.find("{\"id\":")};
						if(pos != -1)
						{
							media_info.erase(0, pos);
							bottom.vidinfo = nlohmann::json::parse(media_info);
						}
					}
				}
			}
			else media_info = util::run_piped_process(conf.ytdlp_path.wstring() + L" --no-warnings --print :%(webpage_url_domain)s:%(title)s "
														+ url, &bottom.working_info);
			if(!media_info.empty() && bottom.working_info)
			{
				if(media_info[0] == ':' || (bottom.is_ytlink && media_info.find("{\"id\":") == 0))
				{
					if(bottom.is_ytlink && !is_ytplaylist)
					{
						media_website = bottom.vidinfo["webpage_url_domain"];
						media_title = bottom.vidinfo["title"];
						if(bottom.vidinfo.contains("is_live") && bottom.vidinfo["is_live"] || 
						   bottom.vidinfo.contains("live_status") && bottom.vidinfo["live_status"] == "is_live")
							media_title = "[live] " + media_title;
						bottom.show_btnytfmt(true);
						get_place().collocate();
						nana::api::update_window(*this);
					}
					else
					{
						auto pos {media_info.find(':', 1)};
						media_website = media_info.substr(1, pos - 1);
						media_title = media_info.substr(pos + 1);
						if(is_ytplaylist)
							media_title = "[playlist] " + media_title;
					}
					lbq.item_from_value(url).text(1, media_website);
					lbq.item_from_value(url).text(2, media_title);
				}
				else
				{
					auto stridx {lbq.item_from_value(url).text(0)};
					lbq.item_from_value(url).text(1, "error");
					lbq.item_from_value(url).text(2, "yt-dlp failed to get info (see output)");
					lbq.item_from_value(url).text(3, "error");
					outbox.caption("GUI: yt-dlp is unable to process queue item #" + stridx +
												" (output below)\n\n" + media_info + "\n", url);
					bottom.btndl.enable(false);
					if(outbox.current() == url)
					{
						auto ca {outbox.colored_area_access()};
						ca->clear();
						auto p {ca->get(0)};
						p->fgcolor = widgets::theme.is_dark() ? widgets::theme.path_link_fg : nana::color {"#569"};
					}
				}
			}
			active_threads--;
			if(bottom.working_info)
				bottom.info_thread.detach();
		});
	}
}


void GUI::pop_queue_menu(int x, int y)
{
	using namespace nana;

	auto sel {lbq.selected()};
	if(!sel.empty() && !thr_menu.joinable())
	{
		auto item {lbq.at(sel.back())};
		auto item_name {"item #" + item.text(0)};
		auto url {item.value<std::wstring>()};
		auto &bottom {bottoms.current()};
		std::vector<drawerbase::listbox::item_proxy> stoppable, startable;
		std::vector<std::wstring> completed;
		for(auto &item : lbq.at(0))
		{
			const auto text {item.text(3)};
			if(text == "done")
				completed.push_back(item.value<std::wstring>());
			else if(text.find("stopped") == -1 && text.find("queued") == -1)
				stoppable.push_back(item);
			else startable.push_back(item);
		}

		::widgets::Menu m;
		m.item_pixels(dpi_transform(24).width);
		auto verb {bottom.btndl.caption().substr(0, 5)};
		if(verb.back() == ' ')
			verb.pop_back();
		if(item.text(3).find("stopped") != -1)
			verb = "Resume";
		m.append(verb + " " + item_name, [&, this](menu::item_proxy &)
		{
			on_btn_dl(url);
			api::refresh_window(bottom.btndl);
		});
		m.append("Remove " + item_name, [&, this](menu::item_proxy &)
		{
			remove_queue_item(url);
		});
		if(lbq.at(0).size() > 1)
		{
			m.append_splitter();
			if(!completed.empty())
			{
				m.append("Clear completed", [&, this](menu::item_proxy &)
				{
					lbq.auto_draw(false);
					autostart_next_item = false;
					for(auto &url : completed)
					{
						auto item {lbq.item_from_value(url)};
						if(item != lbq.at(0).end())
							remove_queue_item(url);
					}
					autostart_next_item = true;
					lbq.auto_draw(true);
				});
			}
			if(!startable.empty())
			{
				m.append("Start all", [&, this](menu::item_proxy &)
				{
					thr_menu = std::thread([startable, &bottom, this]
					{
						menu_working = true;
						autostart_next_item = false;
						for(auto item : startable)
						{
							if(!menu_working) break;
							auto url {item.value<std::wstring>()};
							process_queue_item(url);
						}
					});
					if(menu_working)
						api::refresh_window(bottom.btndl);
					autostart_next_item = true;
					if(thr_menu.joinable())
						thr_menu.detach();
				});
			}
			if(!stoppable.empty())
			{
				m.append("Stop all", [&, this](menu::item_proxy &)
				{
					thr_menu = std::thread([stoppable, &bottom, this]
					{
						menu_working = true;
						autostart_next_item = false;
						for(auto item : stoppable)
						{
							if(!menu_working) break;
							auto url {item.value<std::wstring>()};
							on_btn_dl(url);
						}
						if(menu_working)
							api::refresh_window(bottom.btndl);
						autostart_next_item = true;
						if(thr_menu.joinable())
							thr_menu.detach();
					});					
				});
			}
			if(completed.size() != lbq.at(0).size())
			{
				m.append("Remove all", [&, this](menu::item_proxy &)
				{
					menu_working = true;
					bottoms.show(L"");
					qurl = L"";
					l_url.update_caption();
					btnadd.enabled(false);
					lbq.auto_draw(false);
					auto item {lbq.at(0).begin()};
					while(item != lbq.at(0).end() && menu_working)
					{
						auto url {item.value<std::wstring>()};
						item = lbq.erase(item);
						auto &bottom {bottoms.at(url)};
						if(bottom.timer_proc.started())
							bottom.timer_proc.stop();
						if(bottom.info_thread.joinable())
						{
							bottom.working_info = false;
							bottom.info_thread.join();
						}
						if(bottom.dl_thread.joinable())
						{
							bottom.working = false;
							bottom.dl_thread.join();
						}
						bottoms.erase(url);
						outbox.erase(url);
					}
					adjust_lbq_headers();
					lbq.auto_draw(true);
				});
			}
		}
		if(m.size())
			m.popup_await(lbq, x, y);
	}
}


void GUI::make_queue_listbox()
{
	using namespace nana;

	lbq.sortable(false);
	lbq.enable_single(true, false);
	lbq.typeface(nana::paint::font_info {"Calibri", 12});
	lbq.scheme().item_height_ex = 8;
	lbq.append_header("#", dpi_transform(30).width);
	lbq.append_header("Website", dpi_transform(120).width);
	lbq.append_header("Media title", dpi_transform(685).width);
	lbq.append_header("Status", dpi_transform(116).width);
	lbq.column_movable(false);
	lbq.column_resizable(false);

	lbq.events().resized([this] (const arg_resized &arg) { adjust_lbq_headers(); });

	lbq.events().dbl_click([this](const arg_mouse &arg)
	{
		if(arg.is_left_button())
		{
			show_output();
			lbq_can_drag = false;
		}
	});

	lbq.events().mouse_down([this](const arg_mouse &arg)
	{
		auto hovered {lbq.cast(point(arg.pos.x, arg.pos.y))};
		if(hovered.empty())
		{
			lbq_can_drag = false;
			if(last_selected)
			{
				if(arg.pos.y > dpi_transform(21).width) // if below header
					last_selected = nullptr;
			}
		}
		else
		{
			lbq_can_drag = true;
			auto hovitem {lbq.at(hovered)};
			if(last_selected && last_selected->value<std::wstring>() == hovitem.value<std::wstring>())
			{
				if(!hovitem.selected())
				{
					lbq_no_action = true;
					hovitem.select(true);
				}
			}
		}
	});

	lbq.events().selected([this](const arg_listbox &arg)
	{
		static auto item {arg.item};
		if(arg.item.selected())
		{
			item = arg.item;
			last_selected = &item;
			if(!lbq_no_action)
			{
				auto url {arg.item.value<std::wstring>()};
				if(url != bottoms.visible())
				{
					bottoms.show(url);
					outbox.current(url);
					qurl = url;
					l_url.update_caption();
					btnadd.enable(false);
				}
			}
			else lbq_no_action = false;
		}
		else
		{
			if(last_selected == nullptr)
			{
				lbq_no_action = true;
				arg.item.select(true);
			}
		}
	});

	static color dragging_color {colors::green};
	static timer scroll_down_timer, scroll_up_timer;
	static bool dragging {false};

	lbq.events().mouse_move([this](const arg_mouse &arg)
	{
		if(!arg.is_left_button() || !lbq_can_drag) return;
		dragging = true;

		const auto lines {3};
		bool autoscroll {true};
		const auto delay {std::chrono::milliseconds{25}};

		dragging_color = ::widgets::theme.is_dark() ? ::widgets::theme.path_link_fg : colors::green;
		lbq.auto_draw(false);
		auto lb {lbq.at(0)};
		auto selection {lbq.selected()};
		if(selection.empty())
		{
			auto hovered {lbq.cast(point(arg.pos.x, arg.pos.y))};
			if(hovered.item != npos)
			{
				lbq.at(hovered).select(true);
				selection = lbq.selected();
			}
		}
		if(selection.size() == 1)
		{
			auto hovered {lbq.cast({arg.pos.x, arg.pos.y})}, selected {selection[0]};
			if(hovered.item != npos && hovered.item != selected.item)
			{
				std::string seltext0 {lb.at(selected.item).text(0)},
					seltext1 {lb.at(selected.item).text(1)},
					seltext2 {lb.at(selected.item).text(2)},
					seltext3 {lb.at(selected.item).text(3)};

				std::wstring selval {lb.at(selected.item).value<std::wstring>()};

				auto hovitem {lb.at(hovered.item)};

				// moving item upward, pushing list downward below insertion point
				if(hovered.item < selected.item)
				{
					for(auto n {selected.item}; n > hovered.item; n--)
					{
						lb.at(n).text(1, lb.at(n - 1).text(1));
						lb.at(n).text(2, lb.at(n - 1).text(2));
						lb.at(n).text(3, lb.at(n - 1).text(3));
						auto url {lb.at(n - 1).value<std::wstring>()};
						lb.at(n).value(url);
						if(!conf.common_dl_options)
							bottoms.at(url).gpopt.caption("Download options for queue item #" + lb.at(n).text(0));
						lb.at(n).fgcolor(lbq.fgcolor());
					}
					if(autoscroll)
					{
						if(arg.pos.y / lbq.scheme().item_height_ex <= 1)
						{
							if(!scroll_up_timer.started())
							{
								scroll_up_timer.elapse([&arg] {mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_WHEEL, arg.pos.x, arg.pos.y, WHEEL_DELTA, 0); });
								scroll_up_timer.interval(delay);
								if(scroll_down_timer.started()) scroll_down_timer.stop();
								lbq.scheme().mouse_wheel.lines = lines;
								scroll_up_timer.start();
							}
						}
						else
						{
							if(scroll_up_timer.started()) scroll_up_timer.stop();
							if(scroll_down_timer.started()) scroll_down_timer.stop();
							lbq.scheme().mouse_wheel.lines = lines;
						}
					}
				}

				// moving item downward, pushing list upward above insertion point
				else
				{
					for(auto n(selected.item); n < hovered.item; n++)
					{
						lb.at(n).text(1, lb.at(n + 1).text(1));
						lb.at(n).text(2, lb.at(n + 1).text(2));
						lb.at(n).text(3, lb.at(n + 1).text(3));
						auto url {lb.at(n + 1).value<std::wstring>()};
						lb.at(n).value(url);
						if(!conf.common_dl_options)
							bottoms.at(url).gpopt.caption("Download options for queue item #" + lb.at(n).text(0));
						lb.at(n).fgcolor(lbq.fgcolor());
					}
					if(autoscroll)
					{
						if(arg.pos.y > lbq.size().height - 3 * lbq.scheme().item_height_ex)
						{
							if(!scroll_down_timer.started())
							{
								scroll_down_timer.elapse([&arg]
								{
									mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_WHEEL, arg.pos.x, arg.pos.y, -WHEEL_DELTA, 0);
								});
								scroll_down_timer.interval(delay);
								if(scroll_up_timer.started()) scroll_up_timer.stop();
								lbq.scheme().mouse_wheel.lines = lines;
								scroll_down_timer.start();
							}
						}
						else
						{
							if(scroll_up_timer.started()) scroll_up_timer.stop();
							if(scroll_down_timer.started()) scroll_down_timer.stop();
							lbq.scheme().mouse_wheel.lines = lines;
						}
					}
				}
				hovitem.text(1, seltext1);
				hovitem.text(2, seltext2);
				hovitem.text(3, seltext3);
				hovitem.value(selval);
				hovitem.select(true);
				hovitem.fgcolor(dragging_color);
				if(!conf.common_dl_options)
					bottoms.at(selval).gpopt.caption("Download options for queue item #" + lb.at(lbq.selected().front().item).text(0));
				lbq.auto_draw(true);
			}
		}
	});

	static auto dragstop_fn = [this]
	{
		dragging = lbq_can_drag = false;
		auto sel {lbq.selected()};
		auto selitem {lbq.at(sel[0])};
		selitem.fgcolor(lbq.fgcolor());
		lbq.auto_draw(true);
		if(scroll_up_timer.started()) scroll_up_timer.stop();
		if(scroll_down_timer.started()) scroll_down_timer.stop();
		lbq.scheme().mouse_wheel.lines = 3;
		auto &curbot {bottoms.current()};
		if(curbot.started())
		{
			auto temp {conf.max_concurrent_downloads};
			conf.max_concurrent_downloads = -1;
			auto first_startable_url {next_startable_url(L"")};
			conf.max_concurrent_downloads = temp;
			if(!first_startable_url.empty())
			{
				auto first_startable_item {lbq.item_from_value(first_startable_url)};
				if(stoi(selitem.text(0)) > stoi(first_startable_item.text(0)))
				{
					autostart_next_item = false;
					process_queue_item(curbot.url);
					autostart_next_item = true;
					process_queue_item(first_startable_url);
				}
			}
		}
		else
		{
			std::wstring last_downloading_url;
			for(auto item : lbq.at(0))
			{
				auto &bot {bottoms.at(item.value<std::wstring>())};
				if(bot.started())
					last_downloading_url = bot.url;
			}
			if(!last_downloading_url.empty())
			{
				auto last_downloading_item {lbq.item_from_value(last_downloading_url)};
				if(stoi(selitem.text(0)) < stoi(last_downloading_item.text(0)))
				{
					autostart_next_item = false;
					process_queue_item(last_downloading_url);
					autostart_next_item = true;
					process_queue_item(curbot.url);
				}
			}
		}
	};

	lbq.events().mouse_up([this](const arg_mouse &arg)
	{
		if(arg.is_left_button() && dragging)
			dragstop_fn();
		else if(arg.button == mouse::right_button)
			pop_queue_menu(arg.pos.x, arg.pos.y);
	});

	lbq.events().mouse_leave([this]
	{
		if(GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON) & 0xff00)
			dragstop_fn();
	});
}


void GUI::make_form()
{
	using namespace nana;
	make_queue_listbox();

	div("vert margin=20 <Top> <weight=20> <Bottom weight=325>");
	auto &plc {get_place()};
	plc["Top"].fasten(queue_panel).fasten(outbox).fasten(overlay);
	auto &bottom {bottoms.add(L"", true)};
	bottom.btndl.enabled(false);
	auto &tbpipe {outbox};
	auto &tbpipe_overlay {overlay};
	overlay.events().dbl_click([&, this] { show_queue(); queue_panel.focus(); });

	plc_queue.div(R"(
		vert <lbq> <weight=20> 
		<weight=25 <btn_settings weight=106> <weight=15> <l_url> <weight=15> <btnadd weight=90> <weight=15> <btn_qact weight=126>>
	)");
	plc_queue["lbq"] << lbq;
	plc_queue["l_url"] << l_url;
	plc_queue["btnadd"] << btnadd;
	plc_queue["btn_qact"] << btn_qact;
	plc_queue["btn_settings"] << btn_settings;

	apply_theme(::widgets::theme.is_dark());

	btn_settings.image(arr_config16_ico);
	btn_settings.typeface(paint::font_info {"", 11, {800}});
	btn_settings.events().click([this] { settings_dlg(); });

	btnadd.enable(false);
	btnadd.typeface(paint::font_info {"", 11, {800}});
	btn_qact.typeface(paint::font_info {"", 11, {800}});
	btn_qact.tooltip("Pops up a menu with actions that can be performed on\nthe queue items (same as right-clicking on the queue).");

	l_url.events().mouse_up([this]
	{
		auto text {util::get_clipboard_text()};
		if(text.find('\n') != -1)
		{
			qurl = L"* multiple lines of text, make sure they're URLs *";
			l_url.update_caption();
			multiple_url_text.str(text);
			btnadd.enable(true);
		}
		else if(!text.empty())
		{
			if(text.find(LR"(https://www.youtube.com/watch?v=)") == 0)
				if(text.find(L"&list=") == 43)
					text.erase(43);
			auto item {lbq.item_from_value(text)};
			if(item == lbq.at(0).end())
			{
				qurl = text;
				l_url.update_caption();
				btnadd.enable(true);
				multiple_url_text.clear();
			}
			else
			{
				l_url.caption("The URL in the clipboard is already added (queue item #" + item.text(0) + ").");
			}
		}
	});

	btnadd.events().click([this]
	{
		if(multiple_url_text.view().empty())
			add_url(qurl);
		else
		{
			std::wstring line;
			while(std::getline(multiple_url_text, line))
			{
				if(line.back() == '\r')
					line.pop_back();
				if(!line.empty()) add_url(line);
			}
			multiple_url_text.clear();
		}
		btnadd.enable(false);
	});

	btnadd.events().key_press([&, this](const arg_keyboard &arg)
	{
		if(arg.key == keyboard::enter)
		{
			btnadd.events().click.emit({}, btnadd);
		}
	});

	btn_qact.events().click([this]
	{
		pop_queue_menu(btn_qact.pos().x, btn_qact.pos().y + btn_qact.size().height);
	});

	plc.collocate();
}


void GUI::settings_dlg()
{
	using widgets::theme;

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.center(585, 619);
	fm.caption(title + " - settings");
	fm.bgcolor(theme.fmbg);
	fm.div(R"(vert margin=20
		<weight=25 <l_res weight=148> <weight=10> <com_res weight=52> <> <cbfps weight=196> <weight=54> > <weight=20>
		<weight=25
			<l_video weight=184> <weight=10> <com_video weight=60> <>
			<l_audio weight=185> <weight=10> <com_audio weight=60>
		>
		<weight=20> <sep1 weight=3px> <weight=20>
		<weight=25 <l_ytdlp weight=158> <weight=10> <l_path> > <weight=20>
		<weight=25 <l_template weight=122> <weight=10> <tb_template> <weight=15> <btn_default weight=140>> <weight=20>
		<sep2 weight=3px> <weight=20>
		<weight=25 <l_maxdl weight=196> <weight=10> <sb_maxdl weight=40> <> <cb_lengthyproc weight=282>> <weight=20>
		<weight=25 <cb_autostart weight=460>> <weight=20>
		<weight=25 <cb_common weight=400>> <weight=20>
		<weight=25 <cb_queue_autostart weight=485>> <weight=20>
		<sep3 weight=3px> <weight=20>
		<weight=25 <l_theme weight=92> <weight=20> <cbtheme_dark weight=57> <weight=20> 
			<cbtheme_light weight=58> <weight=20> <cbtheme_system weight=152> > <weight=20>
		<weight=25 <l_contrast weight=64> <weight=20> <slider> > <weight=20>
		<sep4 weight=3px> <weight=21>
		<weight=35 <> <btn_close weight=100> <weight=20> <btn_updater weight=150> <>>
	)");

	widgets::Label l_res {fm, "Preferred resolution:"}, l_latest {fm, "Latest version:"},
		l_video {fm, "Preferred video container:"}, l_audio {fm, "Preferred audio container:"},
		l_theme {fm, "Color theme:"}, l_contrast {fm, "Contrast:"}, l_ytdlp {fm, "Location of yt-dlp.exe:"},
		l_template {fm, "Output template:"}, l_maxdl {fm, "Max concurrent downloads:"};
	widgets::path_label l_path {fm, &conf.ytdlp_path};
	widgets::Textbox tb_template {fm};
	widgets::Combox com_res {fm}, com_video {fm}, com_audio {fm};
	widgets::cbox cbfps {fm, "Prefer a higher framerate"}, cbtheme_dark {fm, "Dark"}, cbtheme_light {fm, "Light"},
		cbtheme_system {fm, "System preference"}, cb_lengthyproc {fm, "Start next item on lengthy processing"},
		cb_common {fm, "Each queue item has its own download options"}, 
		cb_autostart {fm, "When stopping a queue item, automatically start the next one"},
		cb_queue_autostart {fm, "When the program starts, automatically start processing the queue"};
	widgets::Separator sep1 {fm}, sep2 {fm}, sep3 {fm}, sep4 {fm};
	widgets::Button btn_close {fm, " Close"}, btn_updater {fm, " Updater"}, btn_default {fm, "Reset to default"};
	btn_updater.image(arr_updater_ico);
	btn_close.events().click([&] {fm.close(); });
	btn_updater.events().click([&, this] {updater_dlg(fm); l_path.update_caption(); });
	btn_default.typeface(nana::paint::font_info {"", 11, {800}});
	btn_default.tooltip(nana::to_utf8(conf.output_template_default));
	tb_template.multi_lines(false);
	tb_template.padding(0, 5, 0, 5);
	tb_template.caption(conf.output_template);
	tb_template.typeface(nana::paint::font_info {"Tahoma", 10});
	btn_default.events().click([&, this]
	{
		tb_template.caption(conf.output_template_default);
		conf.output_template = conf.output_template_default;
	});

	widgets::Spinbox sb_maxdl {fm};
	sb_maxdl.range(1, 10, 1);
	sb_maxdl.value(std::to_string(conf.max_concurrent_downloads));

	widgets::Slider slider {fm};
	{
		slider.maximum(30);
		slider.value(conf.contrast*100);
		slider.events().value_changed([&]
		{
			conf.contrast = static_cast<double>(slider.value()) / 100;
			theme.contrast(conf.contrast);
			fm.bgcolor(theme.fmbg);
			apply_theme(theme.is_dark());
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

	widgets::Group gpupdate {fm, "Updater"};

	fm["l_res"] << l_res;
	fm["com_res"] << com_res;
	fm["cbfps"] << cbfps;
	fm["l_ytdlp"] << l_ytdlp;
	fm["l_path"] << l_path;
	fm["l_video"] << l_video;
	fm["com_video"] << com_video;
	fm["l_audio"] << l_audio;
	fm["com_audio"] << com_audio;
	fm["sep1"] << sep1;
	fm["l_template"] << l_template;
	fm["tb_template"] << tb_template;
	fm["btn_default"] << btn_default;
	fm["sep2"] << sep2;
	fm["sep4"] << sep4;
	fm["l_theme"] << l_theme;
	fm["cbtheme_dark"] << cbtheme_dark;
	fm["cbtheme_light"] << cbtheme_light;
	if(system_supports_darkmode())
		fm["cbtheme_system"] << cbtheme_system;
	fm["l_contrast"] << l_contrast;
	fm["slider"] << slider;
	fm["gpupdate"] << gpupdate;
	fm["sep3"] << sep3;
	fm["btn_close"] << btn_close;
	fm["btn_updater"] << btn_updater;
	fm["l_maxdl"] << l_maxdl;
	fm["sb_maxdl"] << sb_maxdl;
	fm["cb_lengthyproc"] << cb_lengthyproc;
	fm["cb_autostart"] << cb_autostart;
	fm["cb_common"] << cb_common;
	fm["cb_queue_autostart"] << cb_queue_autostart;

	cbfps.tooltip("If several different formats have the same resolution,\ndownload the one with the highest framerate.");
	cb_lengthyproc.tooltip("When yt-dlp finishes downloading all the files associated with a queue item,\n"
	"it performs actions on the files referred to as \"post-processing\". These actions\ncan be a number of things, depending "
	"on the context, and on the options used.\n\nSometimes this post-processing phase can take a long time, so this setting\n"
	"makes the program automatically start the next queue item when processing\ntakes longer that 3 seconds.");

	const auto res_tip {"Download the best video with the largest resolution available that is\n"
		"not higher than the selected value. Resolution is determined using the\n"
		"smallest dimension, so this works correctly for vertical videos as well."},

		maxdl_tip {"When a queue item finishes, the program automatically starts the next one.\nThis setting lets you make it "
		"start the next two or more items (up to 10).\n\nDoes not limit the number of manually started queue items "
		"(you can have\nmore than 10 concurrent downloads if you want, but you have to start\nthe ones after the 10th manually)."};


	for(auto &opt : com_video_options)
		com_video.push_back(" " + nana::to_utf8(opt));
	com_video.option(conf.pref_video);

	for(auto &opt : com_audio_options)
		com_audio.push_back(" " + nana::to_utf8(opt));
	com_audio.option(conf.pref_audio);

	for(auto &opt : com_res_options)
		com_res.push_back(" " + nana::to_utf8(opt));

	com_res.option(conf.pref_res);
	com_res.tooltip(res_tip);
	l_res.tooltip(res_tip);
	l_maxdl.tooltip(maxdl_tip);
	sb_maxdl.tooltip(maxdl_tip);

	com_res.option(conf.pref_res);
	com_video.option(conf.pref_video);
	com_audio.option(conf.pref_audio);
	cbfps.check(conf.pref_fps);
	cb_lengthyproc.check(conf.cb_lengthyproc);
	cb_autostart.check(conf.cb_autostart);
	cb_common.check(!conf.common_dl_options);
	cb_queue_autostart.check(conf.cb_queue_autostart);

	l_path.events().click([&, this]
	{
		nana::filebox fb {fm, true};
		if(!conf.ytdlp_path.empty())
			fb.init_file(conf.ytdlp_path);
		fb.allow_multi_select(false);
		fb.add_filter("", "yt-dlp.exe");
		fb.title("Locate and select yt-dlp.exe");
		auto res {fb()};
		if(res.size())
		{
			auto path {res.front()};
			if(path.string().find("yt-dlp.exe") != -1)
			{
				conf.ytdlp_path = res.front();
				get_versions();
				l_path.update_caption();
				auto tmp {fs::path {conf.ytdlp_path}.replace_filename("ffmpeg.exe")};
				if(fs::exists(tmp))
					ffmpeg_loc = tmp;
			}
		}
	});

	fm.events().unload([&, this]
	{
		conf.pref_res = com_res.option();
		conf.pref_video = com_video.option();
		conf.pref_audio = com_audio.option();
		conf.pref_fps = cbfps.checked();
		conf.output_template = tb_template.caption_wstring();
		conf.max_concurrent_downloads = sb_maxdl.to_int();
		conf.cb_lengthyproc = cb_lengthyproc.checked();
		conf.cb_autostart = cb_autostart.checked();
		conf.cb_queue_autostart = cb_queue_autostart.checked();
		if(conf.common_dl_options == cb_common.checked())
		{
			if(conf.common_dl_options = !conf.common_dl_options)
			{
				auto &curbot {bottoms.current()};
				bottoms.propagate_cb_options(curbot);
				bottoms.propagate_args_options(curbot);
				bottoms.propagate_misc_options(curbot);
			}
			std::string cap {"Download options"};
			for(auto &pbot : bottoms)
			{
				if(pbot.second->index)
				{
					pbot.second->gpopt.caption(conf.common_dl_options ? cap : cap + " for queue item #" + std::to_string(pbot.second->index));
					pbot.second->show_btncopy(!conf.common_dl_options);
					nana::api::refresh_window(pbot.second->gpopt);
				}
			}
		}
	});

	fm.theme_callback([&, this](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme.fmbg);
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.collocate();

	if(conf.ytdlp_path.empty())
		l_path.caption("!!!  YT-DLP.EXE NOT FOUND IN PROGRAM FOLDER  !!!");
	else if(!fs::exists(conf.ytdlp_path))
	{
		conf.ytdlp_path.clear();
		l_path.caption("!!!  YT-DLP.EXE NOT FOUND AT ITS SAVED LOCATION  !!!");
	}
	fm.show();
	fm.modality();
}


void GUI::changes_dlg(nana::window parent)
{
	using widgets::theme;

	themed_form fm {nullptr, parent, nana::API::make_center(parent, dpi_transform(900, 445)), appear::decorate<appear::sizable>{}};
	fm.theme_callback([&, this](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme.fmbg);
		return false;
	});
	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.caption("ytdlp-interface - release notes");
	fm.bgcolor(theme.fmbg);
	fm.div(R"(vert margin=20 <tb> <weight=20>
				<weight=25 <> <l_history weight=164> <weight=10> <com_history weight=55> <weight=20> <cblogview weight=132> <> >)");

	widgets::Textbox tb {fm};
	{
		fm["tb"] << tb;
		nana::API::effects_edge_nimbus(tb, nana::effects::edge_nimbus::none);
		tb.typeface(nana::paint::font_info {"Courier New", 12});
		tb.line_wrapped(true);
		tb.editable(false);
	}

	widgets::cbox cblogview {fm, "Changelog view"};
	fm["cblogview"] << cblogview;

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
		tb.colored_area_access()->clear();
		nana::api::refresh_window(tb);
	};

	auto display_changelog = [&]
	{
		tb.caption("");
		auto ca {tb.colored_area_access()};
		ca->clear();
		for(const auto &el : releases)
		{
			std::string 
				tag_name {el["tag_name"]}, 
				date     {el["published_at"]},
				body     {el["body"]}, 
				block;

			if(!body.empty())
			{
				body.erase(0, body.find('-'));
				body.erase(body.find_first_of("\r\n", body.rfind("- ")));
			}
			date.erase(date.find('T'));
			auto title {tag_name + " (" + date + ")\n"};
			title += std::string(title.size() - 1, '=') + "\n\n";
			block += title + body + "\n\n";
			if(tag_name != "v1.0.0")
			{
				block += "-----------------------------------------------------------------------------------\n\n";
				tb.caption(tb.caption() + block);
				auto p {ca->get(tb.text_line_count() - 3)};
				p->count = 1;
				p->fgcolor = theme.is_dark() ? nana::color {"#777"} : nana::color {"#bbb"};
			}
			else tb.caption(tb.caption() + block);
		}
	};

	display_release_notes(0);

	widgets::Label l_history {fm, "Show release notes for:"};
	fm["l_history"] << l_history;

	widgets::Combox com_history {fm};
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

	cblogview.events().click([&]
	{
		if(cblogview.checked())
			display_changelog();
		else display_release_notes(com_history.option());
		com_history.enabled(!cblogview.checked());
	});

	tb.events().mouse_wheel([&](const nana::arg_wheel &arg)
	{
		if(!cblogview.checked() && !tb.scroll_operation()->visible(true) && arg.which == nana::arg_wheel::wheel::vertical)
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
			nana::api::refresh_window(com_history);
		}
	});

	fm.collocate();
	fm.show();
	fm.modality();
}


bool GUI::apply_theme(bool dark)
{
	using widgets::theme;

	if(dark) theme.make_dark();
	else theme.make_light();

	if(!outbox.current_buffer().empty())
	{
		auto ca {outbox.colored_area_access()};
		ca->clear();
		auto p {ca->get(0)};
		p->count = 1;
		p->fgcolor = theme.is_dark() ? theme.path_link_fg : nana::color {"#569"};
		auto &text {outbox.current_buffer()};
		if(text.find("[GUI] ") != text.rfind("[GUI] "))
		{
			p = ca->get(outbox.text_line_count() - 2);
			p->count = 1;
			if(text.rfind("WM_CLOSE") == -1)
				p->fgcolor = widgets::theme.is_dark() ? widgets::theme.path_link_fg : nana::color {"#569"};
			else p->fgcolor = widgets::theme.is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
		}
		nana::API::refresh_window(outbox);
	}

	bgcolor(theme.fmbg);
	refresh_widgets();

	for(auto &bot : bottoms)
		bot.second->bgcolor(theme.fmbg);

	return false; // don't refresh widgets a second time, already done once in this function
}


void GUI::show_queue(bool freeze_redraw)
{
	if(freeze_redraw)
		SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
	change_field_weight(get_place(), "Bottom", 298);
	for(auto &bot : bottoms)
	{
		auto &plc {bot.second->plc};
		bot.second->btnq.caption("Show output");
		plc.field_display("prog", false);
		plc.field_display("separator", true);
		plc.collocate();
	}
	outbox.hide();
	overlay.hide();
	queue_panel.show();
	if(freeze_redraw)
		SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
	nana::api::refresh_window(*this);
}


void GUI::show_output()
{
	SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
	change_field_weight(get_place(), "Bottom", 325);
	for(auto &bot : bottoms)
	{		
		bot.second->btnq.caption("Show queue");
	}
	auto &curbot {bottoms.current()};
	auto &plc {curbot.plc};
	plc.field_display("prog", true);
	plc.field_display("separator", false);
	plc.collocate();
	queue_panel.hide();
	outbox.show(curbot.url);
	SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
}


void GUI::get_releases()
{
	thr_releases = std::thread {[this]
	{
		using json = nlohmann::json;
		auto jtext {util::get_inet_res("https://api.github.com/repos/ErrorFlynn/ytdlp-interface/releases", &inet_error)};
		if(!jtext.empty())
		{
			releases = json::parse(jtext);
			std::string tag_name {releases[0]["tag_name"]};
			if(is_tag_a_new_version(tag_name))
				caption(title + "   (" + tag_name + " is available)");
		}
		thr_releases.detach();
	}};
}


void GUI::get_releases_misc()
{
	thr_releases_misc = std::thread {[this]
	{
		using json = nlohmann::json;
		auto jtext {util::get_inet_res("https://api.github.com/repos/yt-dlp/FFmpeg-Builds/releases", &inet_error)};
		if(!jtext.empty())
		{
			auto json_ffmpeg {json::parse(jtext)};
			if(!json_ffmpeg.empty())
			{
				for(auto &el : json_ffmpeg[0]["assets"])
				{
					std::string url {el["browser_download_url"]};
					if(url.find("win64-gpl.zip") != -1)
					{
						url_latest_ffmpeg = url;
						size_latest_ffmpeg = el["size"];
						std::string date {json_ffmpeg[0]["published_at"]};
						ver_ffmpeg_latest.year = stoi(date.substr(0, 4));
						ver_ffmpeg_latest.month = stoi(date.substr(5, 2));
						ver_ffmpeg_latest.day = stoi(date.substr(8, 2));
						break;
					}
				}
			}

			jtext = util::get_inet_res("https://api.github.com/repos/yt-dlp/yt-dlp/releases", &inet_error);
			if(!jtext.empty())
			{
				auto json_ytdlp {json::parse(jtext)};
				if(!json_ytdlp.empty())
				{
					for(auto &el : json_ytdlp[0]["assets"])
					{
						std::string url {el["browser_download_url"]};
						if(url.find("yt-dlp.exe") != -1)
						{
							url_latest_ytdlp = url;
							size_latest_ytdlp = el["size"];
							url_latest_ytdlp_relnotes = json_ytdlp[0]["html_url"];
							std::string date {json_ytdlp[0]["published_at"]};
							ver_ytdlp_latest.year = stoi(date.substr(0, 4));
							ver_ytdlp_latest.month = stoi(date.substr(5, 2));
							ver_ytdlp_latest.day = stoi(date.substr(8, 2));
							break;
						}
					}
				}
			}
		}
		if(thr_releases_misc.joinable())
			thr_releases_misc.detach();
	}};
}


void GUI::get_versions()
{
	thr_versions = std::thread {[this]
	{
		std::thread thr_ffmpeg([this]
		{
			if(!ffmpeg_loc.empty())
			{
				std::wstring cmd {L'\"' + ffmpeg_loc.wstring() + L"\" -version"};
				auto ver {util::run_piped_process(cmd)};
				auto pos {ver.find("--extra-version=")};
				if(pos != -1)
				{
					auto rawver {ver.substr(pos + 16, 8)};
					ver_ffmpeg.year = stoi(rawver.substr(0, 4));
					ver_ffmpeg.month = stoi(rawver.substr(4, 2));
					ver_ffmpeg.day = stoi(rawver.substr(6, 2));
				}
			}
		});

		if(!conf.ytdlp_path.empty())
		{
			if(fs::exists(conf.ytdlp_path))
			{
				auto ver {util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L"\" --version")};
				if(ver.size() >= 10)
				{
					auto rawver {ver.substr(0, 10)};
					try { ver_ytdlp.year = stoi(rawver.substr(0, 4)); }
					catch(const std::invalid_argument&)
					{
						thr_versions.detach();
						if(thr_ffmpeg.joinable())
							thr_ffmpeg.join();
						return;
					}
					ver_ytdlp.month = stoi(rawver.substr(5, 2));
					ver_ytdlp.day = stoi(rawver.substr(8, 2));
				}
			}
			else conf.ytdlp_path.clear();
		}
		if(thr_ffmpeg.joinable())
			thr_ffmpeg.join();
		if(thr_versions.joinable())
			thr_versions.detach();
	}};
}


bool GUI::is_ytlink(std::wstring text)
{
	auto pos {text.find(L"m.youtube.")};
	if(pos != -1)
		text.replace(pos, 1, L"www");
	if(text.find(LR"(https://www.youtube.com/watch?v=)") == 0)
		if(text.size() == 43)
			return true;
		else if(text.size() > 43 && text[43] == '&')
			return true;
	if(text.find(LR"(https://youtu.be/)") == 0)
		if(text.size() == 28)
			return true;
		else if(text.size() > 28 && text[28] == '?')
			return true;
	if(text.find(LR"(https://www.youtube.com/playlist?list=)") == 0)
		return true;
	return false;
}


bool GUI::is_tag_a_new_version(std::string tag_name)
{
	if(tag_name[1] > ver_tag[1]) return true;
	if(tag_name[1] == ver_tag[1] && tag_name[3] > ver_tag[3]) return true;
	if(tag_name[1] == ver_tag[1] && tag_name[3] == ver_tag[3])
	{
		if(tag_name.size() == 6)
		{
			if(ver_tag.size() == 6)
			{
				if(tag_name[5] > ver_tag[5])
					return true;
			}
			else return true;
		}
	}
	return false;
}


void GUI::updater_dlg(nana::window parent)
{
	using widgets::theme;
	themed_form fm {nullptr, parent, nana::API::make_center(parent, dpi_transform(575, 347)), appear::decorate<appear::minimize>{}};

	fm.caption(title + " - updater");
	fm.bgcolor(theme.fmbg);
	fm.div(R"(
		vert margin=20
		<weight=30 <l_ver weight=101> <weight=10> <l_vertext> <weight=20> <btn_changes weight=150> >
		<weight=20>
		<weight=30 <btn_update weight=100> <weight=20> <prog> > <weight=25>
		<separator weight=3px> <weight=20>
		<weight=30 <l_ver_ytdlp weight=158> <weight=10> <l_ytdlp_text> > <weight=10>
		<weight=30 <l_ver_ffmpeg weight=158> <weight=10> <l_ffmpeg_text> > <weight=20>
		<weight=30 <prog_misc> > <weight=25>
		<weight=30 <> <btn_update_ytdlp weight=150> <weight=20> <btn_update_ffmpeg weight=160> <> >
	)");

	widgets::Label
		l_ver {fm, "Latest version:"},
		l_ver_ytdlp {fm, "Latest yt-dlp version:"},
		l_ver_ffmpeg {fm, "Latest ffmpeg version:"};

	widgets::Text
		l_vertext {fm, "checking..."},
		l_ytdlp_text {fm, "checking..."},
		l_ffmpeg_text {fm, "checking..."};

	widgets::Button
		btn_changes {fm, "Release notes"},
		btn_update {fm, "Update"},
		btn_update_ytdlp {fm, "Update yt-dlp"},
		btn_update_ffmpeg {fm, "Update FFmpeg"};

	btn_changes.typeface(nana::paint::font_info {"", 12, {800}});
	btn_changes.enabled(false);
	btn_changes.events().click([&] { changes_dlg(fm); });
	btn_update.typeface(nana::paint::font_info {"", 12, {800}});
	btn_update.enabled(false);
	btn_update_ytdlp.typeface(nana::paint::font_info {"", 12, {800}});
	btn_update_ytdlp.enabled(false);
	btn_update_ffmpeg.typeface(nana::paint::font_info {"", 12, {800}});
	btn_update_ffmpeg.enabled(false);
	l_ver_ytdlp.text_align(nana::align::right, nana::align_v::center);

	widgets::Progress prog {fm}, prog_misc {fm};
	widgets::Separator separator {fm};

	fm["l_ver"] << l_ver;
	fm["l_vertext"] << l_vertext;
	fm["btn_changes"] << btn_changes;
	fm["btn_update"] << btn_update;
	fm["prog"] << prog;
	fm["separator"] << separator;
	fm["l_ver_ytdlp"] << l_ver_ytdlp;
	fm["l_ver_ffmpeg"] << l_ver_ffmpeg;
	fm["l_ytdlp_text"] << l_ytdlp_text;
	fm["l_ffmpeg_text"] << l_ffmpeg_text;
	fm["btn_update_ytdlp"] << btn_update_ytdlp;
	fm["btn_update_ffmpeg"] << btn_update_ffmpeg;
	fm["prog_misc"] << prog_misc;

	bool working {false};
	std::thread thr;

	btn_update.events().click([&, this]
	{
		static fs::path arc_path;
		static bool btnffmpeg_state, btnytdlp_state;
		arc_path = fs::temp_directory_path() / "ytdlp-interface.7z";
		if(btn_update.caption() == "Update")
		{
			btn_update.caption("Cancel");
			btn_update.cancel_mode(true);
			btnffmpeg_state = btn_update_ffmpeg.enabled();
			btnytdlp_state = btn_update_ytdlp.enabled();
			btn_update_ffmpeg.enabled(false);
			btn_update_ytdlp.enabled(false);
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
						pct {std::to_string(progval / (arc_size / 100)) + '%'};
					prog.caption("downloading archive: " + pct + " of " + total);
					prog.value(progval);
				};
				auto dl_error {util::dl_inet_res(arc_url, arc_path, &working, cb_progress)};
				
				if(working)
				{
					if(dl_error.empty())
					{
						auto tempself {fs::temp_directory_path() / self_path.filename()};
						if(fs::exists(tempself))
							fs::remove(tempself);
						try
						{
							fs::copy_file(self_path, tempself);
							auto temp_7zlib {fs::temp_directory_path() / "7z.dll"};
							std::error_code ec;
							fs::remove(temp_7zlib, ec);
							fs::copy_file(appdir / "7z.dll", temp_7zlib);
							std::wstring params {L"update \"" + arc_path.wstring() + L"\" \"" + appdir.wstring() + L"\""};
							ShellExecuteW(NULL, L"runas", tempself.wstring().data(), params.data(), NULL, SW_SHOW);
							working = false;
							fm.close();
							close();
						}
						catch(fs::filesystem_error const &e) {
							nana::msgbox mbox {fm, "File copy error"};
							mbox.icon(nana::msgbox::icon_error);
							(mbox << e.what())();
						}
					}
					else prog.caption(dl_error);
					btn_update.caption("Update");
					btn_update.cancel_mode(false);
					btn_update_ffmpeg.enabled(btnffmpeg_state);
					btn_update_ytdlp.enabled(btnytdlp_state);
					working = false;
					thr.detach();
				}
			}};
		}
		else
		{
			btn_update.caption("Update");
			btn_update.cancel_mode(false);
			prog.caption("");
			prog.value(0);
			btn_update_ffmpeg.enabled(btnffmpeg_state);
			btn_update_ytdlp.enabled(btnytdlp_state);
			working = false;
			thr.detach();
		}
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

	auto display_version_misc = [&, this]
	{
		if(url_latest_ffmpeg.empty())
		{
			l_ffmpeg_text.error_mode(true);
			l_ffmpeg_text.caption("unable to get from GitHub");
			if(!inet_error.empty())
				l_ffmpeg_text.tooltip(inet_error);
		}
		else
		{
			if(ver_ffmpeg_latest > ver_ffmpeg)
			{
				l_ffmpeg_text.caption(ver_ffmpeg_latest.string() + "  (current = " + 
									  (ffmpeg_loc.empty() && !fs::exists(appdir / "ffmpeg.exe") ? "not present)" : ver_ffmpeg.string() + ")"));
				btn_update_ffmpeg.enabled(true);
				btn_update_ffmpeg.tooltip(url_latest_ffmpeg);
			}
			else l_ffmpeg_text.caption(ver_ffmpeg_latest.string() + "  (current)");
		}

		if(url_latest_ytdlp.empty())
		{
			l_ytdlp_text.error_mode(true);
			l_ytdlp_text.caption("unable to get from GitHub");
			if(!inet_error.empty())
				l_ytdlp_text.tooltip(inet_error);
		}
		else
		{
			bool not_present {conf.ytdlp_path.empty() && !fs::exists(appdir / "yt-dlp.exe")};
			if(ver_ytdlp_latest > ver_ytdlp)
			{
				l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current = " + 
					(not_present ? "not present)  [click 4 changes]" : ver_ytdlp.string() + ")  [click 4 changes]"));
				btn_update_ytdlp.enabled(true);
				btn_update_ytdlp.tooltip(url_latest_ytdlp);
			}
			else l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current)  [click to see changelog]");
		}
	};

	auto update_misc = [&, this](bool ytdlp, fs::path target = "")
	{
		static fs::path arc_path;
		arc_path = fs::temp_directory_path() / url_latest_ffmpeg.substr(url_latest_ffmpeg.rfind('/') + 1);

		static bool btnffmpeg_state, btnytdlp_state, btnupdate_state;

		static widgets::Button *btn;
		btn = ytdlp ? &btn_update_ytdlp : &btn_update_ffmpeg;

		static auto btntext_ffmpeg {btn_update_ffmpeg.caption()}, btntext_ytdlp {btn_update_ytdlp.caption()};

		if(btn->caption() != "Cancel")
		{
			btnffmpeg_state = btn_update_ffmpeg.enabled();
			btnytdlp_state = btn_update_ytdlp.enabled();
			btnupdate_state = btn_update.enabled();
			btn->caption("Cancel");
			btn->cancel_mode(true);
			thr = std::thread {[&, ytdlp, target, this]
			{
				working = true;
				auto arc_size {ytdlp ? size_latest_ytdlp : size_latest_ffmpeg}, progval {0u};
				auto arc_url {ytdlp ? url_latest_ytdlp : url_latest_ffmpeg};
				prog_misc.amount(arc_size);
				prog_misc.value(0);
				auto cb_progress = [&, this](unsigned prog_chunk)
				{
					progval += prog_chunk;
					auto total {util::int_to_filesize(arc_size, false)},
						pct {std::to_string(progval / (arc_size / 100)) + '%'};
					prog_misc.caption((ytdlp ? "yt-dlp.exe" : arc_path.filename().string()) + "  :  " + pct + " of " + total);
					prog_misc.value(progval);
				};
				std::string download_result;
				btn_update.enabled(false);
				if(ytdlp)
				{
					btn_update_ffmpeg.enabled(false);
					download_result = util::dl_inet_res(arc_url, target / "yt-dlp.exe", &working, cb_progress);
				}
				else
				{
					btn_update_ytdlp.enabled(false);
					download_result = util::dl_inet_res(arc_url, arc_path, &working, cb_progress);
				}

				if(working)
				{
					if(download_result.empty())
					{
						if(!ytdlp) // FFmpeg downloaded
						{
							fs::path tempdir {fs::temp_directory_path() / std::tmpnam(nullptr)};
							std::error_code ec;
							fs::create_directory(tempdir, ec);
							if(fs::exists(tempdir))
							{
								prog_misc.caption("Extracting files to temporary folder...");
								auto error {util::extract_7z(arc_path, tempdir, true)};
								if(error.empty())
								{
									prog_misc.caption(std::string {"Copying files to "} + (target == appdir ?
																						   "program folder..." : "yt-dlp folder..."));
									for(auto const &dir_entry : fs::recursive_directory_iterator {tempdir})
									{
										if(dir_entry.is_regular_file())
										{
											auto target_path {target / dir_entry.path().filename()};
											if(fs::exists(target_path))
												fs::remove(target_path, ec); // remove existing, otherwise copy operation fails
											fs::copy_file(dir_entry, target_path, ec);
											if(ec)
											{
												prog_misc.caption("Copy error: " + ec.message());
												break;
											}
										}
									}
									if(!ec)
									{
										ffmpeg_loc = target / "ffmpeg.exe";
										get_versions();
										btnffmpeg_state = false;
										prog_misc.caption("FFmpeg update complete");
										l_ffmpeg_text.caption(ver_ffmpeg_latest.string() + "  (current)");
										btn_update_ffmpeg.tooltip("");
									}
								}
								else prog_misc.caption("Error: " + error);
							}
							else prog_misc.caption("!!! FAILED TO CREATE TEMPORARY DIRECTORY !!!");

							fs::remove(arc_path, ec);
							fs::remove_all(tempdir, ec);
						}
						else // yt-dlp downloaded
						{
							conf.ytdlp_path = target / "yt-dlp.exe";
							get_versions();
							btnytdlp_state = false;
							prog_misc.caption("yt-dlp update complete");
							l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current)  [click to see changelog]");
							btn_update_ytdlp.tooltip("");
						}
					}
					else prog_misc.caption(download_result);
					
					btn_update_ytdlp.caption(btntext_ytdlp);
					btn_update_ffmpeg.caption(btntext_ffmpeg);
					btn->cancel_mode(false);
					btn_update_ffmpeg.enabled(btnffmpeg_state);
					btn_update_ytdlp.enabled(btnytdlp_state);
					btn_update.enabled(btnupdate_state);
					working = false;
					thr.detach();
				}
			}};
		}
		else
		{
			working = false;
			btn_update_ytdlp.caption(btntext_ytdlp);
			btn_update_ffmpeg.caption(btntext_ffmpeg);
			btn->cancel_mode(false);
			btn_update_ffmpeg.enabled(btnffmpeg_state);
			btn_update_ytdlp.enabled(btnytdlp_state);
			btn_update.enabled(btnupdate_state);
			thr.detach();
		}
	};

	btn_update_ffmpeg.events().click([&]
	{
		bool use_ytdlp_folder {!ffmpeg_loc.empty() && ffmpeg_loc.parent_path() == conf.ytdlp_path.parent_path()};
		if(ffmpeg_loc.empty())
		{
			if(util::is_dir_writable(appdir))
			{
				update_misc(false, appdir);
				return;
			}
			else use_ytdlp_folder = true;
		}

		if(use_ytdlp_folder && !util::is_dir_writable(conf.ytdlp_path.parent_path()))
			use_ytdlp_folder = false;

		if(!use_ytdlp_folder && !ffmpeg_loc.empty())
		{
			if(!util::is_dir_writable(ffmpeg_loc.parent_path()))
			{
				if(util::is_dir_writable(appdir))
					update_misc(false, appdir);
				else
				{
					nana::msgbox mbox {fm, "No place to put the ffmpeg files"};
					mbox.icon(nana::msgbox::icon_error);
					(mbox << "Neither the program folder, nor the specified yt-dlp folder can be written in. Run the program "
								"as administrator to fix that.")();
				}
				return;
			}
		}
		update_misc(false, use_ytdlp_folder ? conf.ytdlp_path.parent_path() : ffmpeg_loc.parent_path());
	});

	btn_update_ytdlp.events().click([&, this]
	{
		if(conf.ytdlp_path.empty())
		{
			if(util::is_dir_writable(appdir))
			{
				update_misc(true, appdir);
			}
			else
			{
				nana::msgbox mbox {fm, "No place to put yt-dlp.exe"};
				mbox.icon(nana::msgbox::icon_error);
				(mbox << "The path for yt-dlp is not defined, and the program can't write in the folder "
							"it's currently in (running it as administrator should fix that).")();
			}
		}
		else
		{
			if(util::is_dir_writable(conf.ytdlp_path.parent_path()))
			{
				update_misc(true, conf.ytdlp_path.parent_path());
			}
			else
			{
				if(util::is_dir_writable(appdir))
					update_misc(true, appdir);
				else
				{
					nana::msgbox mbox {fm, "No place to put yt-dlp.exe"};
					mbox.icon(nana::msgbox::icon_error);
					(mbox << "Neither the program folder, nor the specified yt-dlp folder can be written in. Running the program "
					 "as administrator should fix that.")();
				}
			}
		}
	});

	nana::timer t, t2;
	t.interval(std::chrono::milliseconds {100});
	t.elapse([&, this]
	{
		if(!thr_releases.joinable())
		{
			display_version();
			t.stop();
		}
	});

	t2.interval(std::chrono::milliseconds {300});
	t2.elapse([&, this]
	{
		if(!thr_releases_misc.joinable())
		{
			if(!url_latest_ytdlp_relnotes.empty())
			{
				nana::api::effects_edge_nimbus(l_ytdlp_text, nana::effects::edge_nimbus::over);
				l_ytdlp_text.tooltip("Click to view release notes in web browser.");
				l_ytdlp_text.events().click([this]
				{
					nana::system::open_url(url_latest_ytdlp_relnotes);
				});
			}
			display_version_misc();
			t2.stop();
		}
	});

	if(thr_releases.joinable())
		t.start();
	else display_version();

	if(url_latest_ffmpeg.empty())
	{
		get_releases_misc();
		t2.start();
	}
	else
	{
		if(!url_latest_ytdlp_relnotes.empty())
		{
			nana::api::effects_edge_nimbus(l_ytdlp_text, nana::effects::edge_nimbus::over);
			l_ytdlp_text.tooltip("Click to view release notes in web browser.");
			l_ytdlp_text.events().click([this]
			{
				nana::system::open_url(url_latest_ytdlp_relnotes);
			});
		}
		display_version_misc();
	}

	fm.theme_callback([&, this](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme.fmbg);
		return false;
	});
	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.events().unload([&, this]
	{
		working = false;

		if(thr.joinable())
			thr.join();
		if(thr_releases.joinable())
			thr_releases.detach();
		if(thr_releases_misc.joinable())
			thr_releases_misc.detach();
		if(thr_versions.joinable())
			thr_versions.detach();
	});

	fm.collocate();
	fm.show();
	nana::api::get_widget(parent)->hide();
	fm.modality();
	nana::api::get_widget(parent)->show();
}


void GUI::change_field_weight(nana::place &plc, std::string field, unsigned new_weight)
{
	std::string divtext {plc.div()};
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
			plc.div(divtext.substr(0, cut_pos) + std::to_string(new_weight) + divtext.substr(pos));
			plc.collocate();
		}
	}
}


GUI::settings_t GUI::conf;


void GUI::gui_bottom::show_btncopy(bool show)
{
	plc.field_display("btncopy", show);
	plc.field_display("btncopy_spacer", show);
	plc.collocate();
}


void GUI::gui_bottom::show_btnytfmt(bool show)
{
	plc.field_display("btn_ytfmtlist", show);
	plc.field_display("ytfm_spacer", show);
	plc.collocate();
}


GUI::gui_bottom::gui_bottom(GUI &gui, bool visible)
{
	using namespace nana;

	pgui = &gui;
	create(gui, visible);
	bgcolor(::widgets::theme.fmbg);

	auto prevbot {gui.bottoms.back()};
	if(prevbot)
		outpath = prevbot->outpath;
	else outpath = conf.outpath;

	plc.bind(*this);
	gpopt.create(*this, "Download options");
	gpopt.enabled(false);
	plcopt.bind(gpopt);
	tbrate.create(gpopt);
	prog.create(*this);
	separator.create(*this);
	separator.refresh_theme();
	btn_ytfmtlist.create(*this, "Select formats");
	btndl.create(*this, "Start download");
	btncopy.create(*this, "Apply options to all queue items");
	btnerase.create(gpopt);
	btnq.create(*this, gui.queue_panel.visible() ? "Show output" : "Show queue");
	l_out.create(gpopt, "Download folder:");
	l_rate.create(gpopt, "Download rate limit:");
	l_outpath.create(gpopt, &outpath);
	com_rate.create(gpopt);
	com_args.create(gpopt);
	cbsplit.create(gpopt, "Split chapters");
	cbkeyframes.create(gpopt, "Force keyframes at cuts");
	cbmp3.create(gpopt, "Convert audio to MP3");
	cbchaps.create(gpopt, "Embed chapters");
	cbsubs.create(gpopt, "Embed subtitles");
	cbthumb.create(gpopt, "Embed thumbnail");
	cbsubs.create(gpopt, "Embed subtitles");
	cbthumb.create(gpopt, "Embed thumbnail");
	cbtime.create(gpopt, "File modification time = time of writing");
	cbargs.create(gpopt, "Custom arguments:");

	btnq.events().click([&, this]
	{
		if(btnq.caption().find("queue") != -1)
			gui.show_queue();
		else gui.show_output();
	});

	btncopy.events().click([&, this]
	{
		gui.bottoms.propagate_cb_options(*this);
		gui.bottoms.propagate_args_options(*this);
		gui.bottoms.propagate_misc_options(*this);
	});

	plc.div(R"(vert
			<prog weight=30> 
			<separator weight=3px>
			<weight=20>
			<gpopt weight=220> <weight=20>
			<weight=35 <> <btn_ytfmtlist weight=190> <ytfmt_spacer weight=20> <btncopy weight=328> <btncopy_spacer weight=20> 
				<btnq weight=180> <weight=20> <btndl weight=200> <>>
		)");
	plc["prog"] << prog;
	plc["separator"] << separator;
	plc["gpopt"] << gpopt;
	plc["btncopy"] << btncopy;
	plc["btn_ytfmtlist"] << btn_ytfmtlist;
	plc["btndl"] << btndl;
	plc["btnq"] << btnq;

	plc.field_display("prog", false);
	plc.field_display("separator", true);
	plc.field_display("btncopy", false);
	plc.field_display("btncopy_spacer", false);
	show_btnytfmt(false);

	prog.amount(1000);
	prog.caption("");

	gpopt.size({10, 10}); // workaround for weird caption display bug

	gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=122> <weight=15> <l_outpath> > <weight=20>
			<weight=25 
				<l_rate weight=144> <weight=15> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbchaps weight=133> <> <cbsplit weight=116> <> <cbkeyframes weight=186>
			> <weight=20>
			<weight=25 <cbtime weight=296> <> <cbthumb weight=144> <> <cbsubs weight=132> <> <cbmp3 weight=173>>
			<weight=20> <weight=24 <cbargs weight=156> <weight=15> <com_args> <weight=10> <btnerase weight=24>>
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
	gpopt["com_args"] << com_args;
	gpopt["btnerase"] << btnerase;

	if(API::screen_dpi(true) > 96)
	{
		btnerase.image(arr_erase22_ico);
		btnerase.image_disabled(arr_erase22_disabled_ico);
	}
	else
	{
		btnerase.image(arr_erase16_ico);
		btnerase.image_disabled(arr_erase16_disabled_ico);
	}

	tbrate.multi_lines(false);
	tbrate.padding(0, 5, 0, 5);

	for(auto &str : conf.argsets)
		com_args.push_back(to_utf8(str));
	com_args.option(conf.com_args);
	com_args.editable(true);

	com_args.events().selected([&, this]
	{
		conf.com_args = com_args.option();
		if(conf.common_dl_options && api::focus_window() == com_args)
			gui.bottoms.propagate_args_options(*this);
	});

	com_args.events().text_changed([&, this]
	{
		auto idx {com_args.caption_index()};
		if(idx != -1) btnerase.enable(true);
		else btnerase.enable(false);
		if(api::focus_window() == com_args)
		{
			for(auto &pbot : gui.bottoms)
			{
				auto &bot {*pbot.second};
				if(bot.handle() != handle())
					bot.com_args.caption(com_args.caption());
			}
		}
	});

	btnerase.events().click([&, this]
	{
		auto idx {com_args.caption_index()};
		if(idx != -1)
		{
			com_args.erase(idx);
			com_args.caption("");
			conf.argsets.erase(conf.argsets.begin() + idx);
			auto size {conf.argsets.size()};
			if(size && idx == size) idx--;
			com_args.option(idx);
			if(conf.common_dl_options)
				gui.bottoms.propagate_args_options(*this);
		}
	});

	if(prevbot)
		tbrate.caption(prevbot->tbrate.caption());
	else if(conf.ratelim)
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

	tbrate.events().text_changed([&, this]
	{
		if(conf.common_dl_options && tbrate == api::focus_window())
			gui.bottoms.propagate_misc_options(*this);
	});

	com_rate.editable(false);
	com_rate.push_back(" KB/s");
	com_rate.push_back(" MB/s");
	if(prevbot)
		com_rate.option(prevbot->com_rate.option());
	else com_rate.option(conf.ratelim_unit);
	com_rate.events().selected([&]
	{
		conf.ratelim_unit = com_rate.option();
		if(conf.common_dl_options && com_rate == api::focus_window())
			gui.bottoms.propagate_misc_options(*this);
	});

	cbsplit.tooltip("Split into multiple files based on internal chapters. (<bold>--split-chapters)</>");
	cbkeyframes.tooltip("Force keyframes around the chapters before\nremoving/splitting them. Requires a\n"
						"reencode and thus is very slow, but the\nresulting video may have fewer artifacts\n"
						"around the cuts. (<bold>--force-keyframes-at-cuts</>)");
	cbtime.tooltip("Do not use the Last-modified header to set the file modification time (<bold>--no-mtime</>)");
	cbsubs.tooltip("Embed subtitles in the video (only for mp4, webm and mkv videos) (<bold>--embed-subs</>)");
	cbchaps.tooltip("Add chapter markers to the video file (<bold>--embed-chapters</>)");
	cbthumb.tooltip("Embed thumbnail in the video as cover art (<bold>--embed-thumbnail</>)");
	btnerase.tooltip("Remove this argument set from the list");
	btndl.tooltip("Starting a download after having stopped it, resumes it.");
	btncopy.tooltip("Copy the options for this queue item to all the other queue items.");
	btn_ytfmtlist.tooltip("Choose formats manually, instead of letting yt-dlp\nchoose automatically. "
		"By default, yt-dlp chooses the\nbest formats, according to the preferences you set\n"
		"(if you press the \"Settings\" button, you can set\nthe preferred resolution, container, and framerate).");
	std::string args_tip
	{
		"Custom options for yt-dlp, separated by space. Some useful examples:\n\n"
		"<bold>-f ba</> : best audio (downloads just audio-only format, if available)\n"
		"<bold>-f wa</> : worst audio\n"
		"<bold>-f wv+wa</> : combines the worst video format with the worst audio format\n"
		"<bold>--live-from-start</> : Downloads livestreams from the start. Currently \n"
		"								  only supported for YouTube (experimental).\n\n"
		"For more, read the yt-dlp documentation:\n"
		"https://github.com/yt-dlp/yt-dlp#usage-and-options"
	};
	cbargs.tooltip(args_tip);
	com_args.tooltip(args_tip);

	btndl.events().click([&gui, this] { gui.on_btn_dl(url); });

	btndl.events().key_press([&gui, this](const arg_keyboard &arg)
	{
		if(arg.key == '\r')
			gui.on_btn_dl(url);
	});

	btn_ytfmtlist.events().click([&gui, this]
	{
		gui.formats_dlg();
	});

	l_outpath.events().click([&gui, this]
	{
		auto clip_text = [this](const std::wstring &str, int max_pixels) -> std::wstring
		{
			nana::label l {*this, str};
			l.typeface(l_outpath.typeface());
			int offset {0};
			const auto strsize {str.size()};
			while(l.measure(1234).width > max_pixels)
			{
				offset += 1;
				if(strsize - offset < 4)
					return L"";
				l.caption(L"..." + str.substr(offset));
			}
			return l.caption_wstring();
		};

		auto pop_folder_selection_box = [&gui, this]
		{
			nana::folderbox fb {*this, gui.appdir};
			fb.allow_multi_select(false);
			fb.title("Locate and select the desired download folder");
			auto res {fb()};
			if(res.size())
			{
				conf.outpaths.insert(outpath);
				outpath = conf.outpath = res.front();
				l_outpath.caption(outpath.u8string());
				if(conf.common_dl_options)
					gui.bottoms.propagate_misc_options(*this);
			}
		};

		if(conf.outpaths.empty() || conf.outpaths.size() == 1 && *conf.outpaths.begin() == outpath)
		{
			pop_folder_selection_box();
			conf.outpaths.insert(outpath);
		}
		else
		{
			::widgets::Menu m;
			m.append("Choose folder...", [&, this](menu::item_proxy &)
			{
				pop_folder_selection_box();
				if(conf.outpaths.size() >= 11 && conf.outpaths.find(outpath) == conf.outpaths.end())
					conf.outpaths.erase(conf.outpaths.begin());
				conf.outpaths.insert(outpath);
			});
			m.append("Clear folder history", [&, this](menu::item_proxy &)
			{
				conf.outpaths.clear();
				conf.outpaths.insert(outpath);
			});
			m.append_splitter();
			for(auto &path : conf.outpaths)
			{
				if(path != outpath)
				{
					m.append(to_utf8(clip_text(path, 250)), [&, this](menu::item_proxy &)
					{
						outpath = conf.outpath = path;
						l_outpath.update_caption();
						conf.outpaths.insert(outpath);
						if(conf.common_dl_options)
							gui.bottoms.propagate_misc_options(*this);
					});
				}
			}
			m.max_pixels(280);
			m.item_pixels(gui.dpi_transform(24).width);
			auto curpos {api::cursor_position()};
			m.popup_await(nullptr, curpos.x - 142, curpos.y);
			gui.queue_panel.focus();
		}
		
	});

	if(prevbot)
	{
		cbsplit.check(prevbot->cbsplit.checked());
		cbchaps.check(prevbot->cbchaps.checked());
		cbsubs.check(prevbot->cbsubs.checked());
		cbthumb.check(prevbot->cbthumb.checked());
		cbtime.check(prevbot->cbtime.checked());
		cbkeyframes.check(prevbot->cbkeyframes.checked());
		cbmp3.check(prevbot->cbmp3.checked());
		cbargs.check(prevbot->cbargs.checked());
	}
	else
	{
		cbsplit.check(conf.cbsplit);
		cbchaps.check(conf.cbchaps);
		cbsubs.check(conf.cbsubs);
		cbthumb.check(conf.cbthumb);
		cbtime.check(conf.cbtime);
		cbkeyframes.check(conf.cbkeyframes);
		cbmp3.check(conf.cbmp3);
		cbargs.check(conf.cbargs);
	}

	cbsplit.radio(true);
	cbchaps.radio(true);

	cbsplit.events().checked([&, this]
	{
		if(cbsplit.checked() && cbchaps.checked())
			cbchaps.check(false);
		conf.cbsplit = cbsplit.checked();
		if(conf.common_dl_options && cbsplit == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});

	cbchaps.events().checked([&, this]
	{
		if(cbchaps.checked() && cbsplit.checked())
			cbsplit.check(false);
		conf.cbchaps = cbchaps.checked();
		if(conf.common_dl_options && cbchaps == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});

	cbtime.events().checked([&, this]
	{
		conf.cbtime = cbtime.checked();
		if(conf.common_dl_options && cbtime == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbthumb.events().checked([&, this]
	{
		conf.cbthumb = cbthumb.checked();
		if(conf.common_dl_options && cbthumb == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbsubs.events().checked([&, this]
	{
		conf.cbsubs = cbsubs.checked();
		if(conf.common_dl_options && cbsubs == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbkeyframes.events().checked([&, this]
	{
		conf.cbkeyframes = cbkeyframes.checked();
		if(conf.common_dl_options && cbkeyframes == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbmp3.events().checked([&, this]
	{
		conf.cbmp3 = cbmp3.checked();
		if(conf.common_dl_options && cbmp3 == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});
	cbargs.events().checked([&, this]
	{
		conf.cbargs = cbargs.checked();
		if(conf.common_dl_options && cbargs == api::focus_window())
			gui.bottoms.propagate_cb_options(*this);
	});

	gui.queue_panel.focus();
	plc.collocate();
}


void GUI::taskbar_overall_progress()
{
	if(i_taskbar && lbq.at(0).size() > 1)
	{
		ULONGLONG completed {0}, total {lbq.at(0).size()};
		for(auto &item : lbq.at(0))
			if(item.text(3) == "done")
				completed++;
		if(completed == total)
			i_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
		else i_taskbar->SetProgressValue(hwnd, completed, total);
	}
}


void GUI::on_btn_dl(std::wstring url)
{
	taskbar_overall_progress();
	if(!process_queue_item(url))
		return; // the queue item was stopped, not started

	auto items_currently_downloading {0};
	
	for(auto &bottom : bottoms)
		if(bottom.second->started())
			items_currently_downloading++;

	auto item_total {lbq.at(0).size()};
	if(item_total > 1)
	{
		auto pos {std::stoi(lbq.item_from_value(url).text(0)) - 1};
		while(++pos < item_total && items_currently_downloading < conf.max_concurrent_downloads)
		{
			auto next_item {lbq.at(0).at(pos)};
			auto next_url {next_item.value<std::wstring>()};
			auto text {next_item.text(3)};
			if(text == "queued" || text.find("stopped") == 0)
			{
				process_queue_item(next_url);
				items_currently_downloading++;
			}
		}
	}
}


void GUI::remove_queue_item(std::wstring url)
{
	auto item {lbq.item_from_value(url)};
	auto next_url {next_startable_url()};
	if(item != lbq.at(0).end())
	{
		auto &bottom {bottoms.at(url)};
		if(lbq.at(0).size() > 1)
			for(auto it {item}; it != lbq.at(0).end(); it++)
			{
				const auto stridx {std::to_string(std::stoi(it.text(0)) - 1)};
				it.text(0, stridx);
				if(!conf.common_dl_options)
					bottoms.at(it.value<std::wstring>()).gpopt.caption("Download options for queue item #" + stridx);
			}

		auto prev_idx {std::stoi(item.text(0)) - 1};
		auto next_item {lbq.erase(item)};
		taskbar_overall_progress();
		adjust_lbq_headers();
		if(next_item != lbq.at(0).end())
			next_item.select(true);
		else if(lbq.at(0).size() != 0)
			lbq.at(0).at(prev_idx > -1 ? prev_idx : 0).select(true);
		else
		{
			bottoms.show(L"");
			qurl = L"";
			l_url.update_caption();
		}
		if(bottom.timer_proc.started())
			bottom.timer_proc.stop();
		if(bottom.info_thread.joinable())
		{
			bottom.working_info = false;
			bottom.info_thread.join();
		}
		if(bottom.dl_thread.joinable())
		{
			bottom.working = false;
			bottom.dl_thread.join();
			if(!next_url.empty())
				on_btn_dl(next_url);
		}
		bottoms.erase(url);
		outbox.erase(url);
		if(bottoms.size() == 2)
		{
			SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
			if(bottoms.at(1).plc.field_display("btncopy"))
				bottoms.at(1).show_btncopy(false);
			SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
			nana::api::refresh_window(*this);
		}
	}
}


std::wstring GUI::next_startable_url(std::wstring current_url)
{
	if(autostart_next_item)
	{
		auto item_total {lbq.at(0).size()};
		if(item_total > 1)
		{
			auto items_currently_downloading {0};
			//auto &curbot {current_url == L"current" ? bottoms.current() : bottoms.at(current_url)};
			if(current_url == L"current")
				current_url = bottoms.current().url;
			for(auto item : lbq.at(0))
			{
				const auto text {item.text(3)};
				if(text == "downloading" || (text.find('%') != -1 && text.find("stopped") == -1) || text == "started")
					items_currently_downloading++;
			}

			auto pos {current_url.empty() ? -1 : stoi(lbq.item_from_value(current_url).text(0)) - 1};
			if(pos == item_total - 1)
				pos = -1;
			while(++pos < item_total && items_currently_downloading < conf.max_concurrent_downloads)
			{
				auto next_item {lbq.at(0).at(pos)};
				auto next_url {next_item.value<std::wstring>()};
				auto text {next_item.text(3)};
				if(text == "queued" || (text.find("stopped") != -1 && text.find("error") != -1))
					return next_url;
			}
		}
	}
	return L"";
}