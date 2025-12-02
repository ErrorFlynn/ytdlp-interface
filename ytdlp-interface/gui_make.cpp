#include "gui.hpp"
#include <nana/gui/filebox.hpp>
#include <TlHelp32.h>


void GUI::make_form()
{
	using widgets::theme;
	using namespace nana;
	queue_make_listbox();

	div(R"(vert margin=20 <Top> <weight=20>
			<Bottom weight=325 vert
				<prog weight=30> 
				<separator weight=3>
				<weight=20 <> <expcol weight=20>>
				<gpopt weight=220> <gpopt_spacer weight=20>
				<weight=35 <> <btn_ytfmtlist weight=190> <ytfmt_spacer weight=20> <btncopy weight=328> <btncopy_spacer weight=20> 
					<btnq weight=180> <weight=20> <btndl weight=200> <>>
			>
		)");
	(*this)["Top"].fasten(queue_panel).fasten(outbox).fasten(overlay);

	make_form_bottom();

	auto &tbpipe {outbox};
	auto &tbpipe_overlay {overlay};
	overlay.events().dbl_click([&] { show_queue(); queue_panel.focus(); });
	btn_settings.tooltip("Ctrl+S");

	plc_queue.div(R"(
		vert <lbq> <weight=20> 
		<weight=25 <btn_settings weight=106> <weight=15> <l_url> <weight=15> <btn_qact weight=126>>
	)");
	plc_queue["lbq"] << lbq;
	plc_queue["l_url"] << l_url;
	plc_queue["btn_qact"] << btn_qact;
	plc_queue["btn_settings"] << btn_settings;

	apply_theme(::widgets::theme::is_dark());

	const auto dpi {API::screen_dpi(true)};
	if(dpi == 288)
		btn_settings.image(arr_config48_png, sizeof arr_config48_png);
	else if(dpi >= 192)
		btn_settings.image(arr_config32_png, sizeof arr_config32_png);
	else if(dpi > 96)
		btn_settings.image(arr_config22_png, sizeof arr_config22_png);
	else btn_settings.image(arr_config16_png, sizeof arr_config16_png);

	if(dpi == 288)
		btnlabel.image(arr_text_field_48_png, sizeof arr_text_field_48_png);
	else if(dpi >= 192)
		btnlabel.image(arr_text_field_32_png, sizeof arr_text_field_32_png);
	else if(dpi > 96)
		btnlabel.image(arr_text_field_22_png, sizeof arr_text_field_22_png);
	else btnlabel.image(arr_text_field_16_png, sizeof arr_text_field_16_png);

	btn_settings.events().click([this] { fm_settings(); });
	btn_qact.tooltip("Pops up a menu with actions that can be performed on\nthe queue items (same as right-clicking on the queue).");

	l_url.events().mouse_enter([this]
	{
		auto text {util::get_clipboard_text()};
		if(text.find(LR"(youtube.com/watch?v=)") != -1)
		{
			auto pos {text.find(L"&list=")};
			if(pos != -1)
				text.erase(pos);
		}

		if(text.empty())
			l_url.caption("the clipboard does not contain any text");
		else if(text.find('\n') != -1)
			l_url.caption("* multiple lines of text, make sure they're URLs *");
		else
		{
			auto item {lbq.item_from_value(text)};
			if(item == lbq.empty_item)
			{
				if(text.size() > 300)
					text.erase(0, text.size() - 300);
				//qurl = text;
				//l_url.update_caption();
				l_url.caption(text);
			}
			else
			{
				//qurl = text + L" (queue item #" + to_wstring(item.text(0)) + L")";
				//l_url.update_caption();
				l_url.caption(text + L" (queue item #" + to_wstring(item.text(0)) + L")");
			}
		}
		if(theme::is_dark())
			l_url.fgcolor(theme::path_link_fg.blend(colors::black, .25));
		else l_url.fgcolor(theme::path_link_fg.blend(colors::white, .25));
	});

	l_url.events().mouse_leave([this]
	{
		auto sel {lbq.selected()};
		if(sel.empty())
			qurl.clear();
		else qurl = lbq.at(sel.front()).value<lbqval_t>();
		l_url.update_caption();
		if(theme::is_dark())
			l_url.fgcolor(theme::path_link_fg);
		else l_url.fgcolor(theme::path_link_fg);
	});

	l_url.events().mouse_up([this]
	{
		auto text {util::get_clipboard_text()};
		if(text.find('\n') != -1)
		{
			std::wstring line;
			std::wstringstream ss;
			ss.str(text);
			lbq.auto_draw(false);
			while(std::getline(ss, line))
			{
				if(line.back() == '\r')
					line.pop_back();
				if(!line.empty()) add_url(line);
			}
			if(items_initialized)
				lbq.auto_draw(true);
		}
		else if(!text.empty())
		{
			if(text.starts_with(LR"(https://www.youtube.com/watch?v=)"))
				if(text.find(L"&list=") == 43)
					text.erase(43);
			auto item {lbq.item_from_value(text)};
			if(item == lbq.empty_item)
			{
				add_url(text);
				if(items_initialized)
					lbq.auto_draw(true);
			}
			else l_url.caption("The URL in the clipboard is already added (queue item #" + item.text(0) + ").");

			if(theme::is_dark())
				l_url.fgcolor(theme::path_link_fg);
			else l_url.fgcolor(theme::path_link_fg);
		}
		adjust_lbq_headers();
	});

	btn_qact.events().click([this]
	{
		auto url {queue_pop_menu(btn_qact.pos().x, btn_qact.pos().y + btn_qact.size().height)};
		if(!url.empty())
		{
			lbq.auto_draw(false);
			queue_remove_item(url);
			lbq.auto_draw(true);
		}
	});

	events().unload([&](const arg_unload &arg)
	{
		if(g_exiting) return;
		g_exiting = true;
		for(auto &bottom : bottoms)
		{
			bottom.second->working_info = false;
			bottom.second->working = false;
		}
		if(thr_queue_remove.joinable())
		{
			arg.cancel = true;
			return;
		}
		RevokeDragDrop(hwnd);
		conf.zoomed = is_zoomed(true);
		if(conf.zoomed || is_zoomed(false)) restore();
		conf.winrect = nana::rectangle {pos(), size()};
		menu_working = false;
		if(thr_menu_start_stop.joinable())
			thr_menu_start_stop.join();
		if(thr_updater.joinable())
			thr_updater.join();
		if(thr_releases.joinable())
			thr_releases.detach();
		if(thr_releases_ffmpeg.joinable())
			thr_releases_ffmpeg.detach();
		if(thr_releases_ytdlp.joinable())
			thr_releases_ytdlp.detach();
		if(thr_versions.joinable())
			thr_versions.detach();
		if(thr_ver_ffmpeg.joinable())
			thr_ver_ffmpeg.detach();
		if(i_taskbar)
			i_taskbar.Release();

		conf.argset = bottoms.current().argset;

		queue_save_data();
		if(!fn_write_conf() && errno)
		{
			std::string error {std::strerror(errno)};
			::widgets::msgbox mbox {*this, "ytdlp-interface - error writing settings file"};
			mbox.icon(msgbox::icon_error);
			(mbox << confpath.string() << "\n\nAn error occured when trying to save the settings file:\n\n" << error)();
		}
		auto pfm {fm_alert("Shutting down active yt-dlp instances", "Please wait...", false)};
		if(bottoms.joinable_info_threads() || bottoms.joinable_dl_threads())
		{
			pfm->show();
			lbq.force_no_thread_safe_ops(true);
			api::refresh_window(*pfm);
			util::close_children();
			for(auto &bottom : bottoms)
			{
				auto &bot {*bottom.second};
				if(bot.url == L"") continue;

				TerminateThread(bot.info_thread.native_handle(), 0);
				if(bot.info_thread.joinable())
					bot.info_thread.detach();
				if(bot.dl_thread.joinable())
				{
					//fm_alert_text.caption(bot.url);
					TerminateThread(bot.dl_thread.native_handle(), 0);
					bot.dl_thread.detach();
				}
			}
		}
		HANDLE hSnapshot {CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)};
		if(hSnapshot)
		{
			PROCESSENTRY32 pe32;
			pe32.dwSize = sizeof(PROCESSENTRY32);
			if(Process32First(hSnapshot, &pe32))
			{
				auto program_PID {GetCurrentProcessId()};
				std::string ytdlp;
				if(!conf.ytdlp_path.empty())
					ytdlp = conf.ytdlp_path.filename().string();
				if(ytdlp.empty())
					ytdlp = ytdlp_fname;
				else std::transform(ytdlp.begin(), ytdlp.end(), ytdlp.begin(), ::tolower);
				do
				{
					auto exe {nana::to_utf8(pe32.szExeFile)};
					std::transform(exe.begin(), exe.end(), exe.begin(), ::tolower);
					if(exe == ytdlp)
					{
						bool is_running {false};
						HANDLE hProcess {OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pe32.th32ParentProcessID)};
						if(hProcess)
						{
							DWORD exitCode {0};
							is_running = (GetExitCodeProcess(hProcess, &exitCode) && exitCode == STILL_ACTIVE);
							CloseHandle(hProcess);
							if(!is_running || pe32.th32ParentProcessID == program_PID)
							{
								auto hwnd {util::hwnd_from_pid(pe32.th32ProcessID)};
								if(hwnd)
									SendMessageA(hwnd, WM_CLOSE, 0, 0);
							}
						}
						else
						{
							auto hwnd {util::hwnd_from_pid(pe32.th32ProcessID)};
							if(hwnd)
								SendMessageA(hwnd, WM_CLOSE, 0, 0);
						}
					}
				} while(Process32Next(hSnapshot, &pe32));
			}
			CloseHandle(hSnapshot);
		}
		g_log.close();
	});

	collocate();
}


void GUI::make_form_bottom()
{
	using namespace nana;

	auto &bottom {bottoms.at("")};
	auto &plc {get_place()};

	btndl.enabled(false);
	plc["prog"] << prog;
	plc["separator"] << separator;
	plc["expcol"] << expcol;
	plc["gpopt"] << gpopt;
	plc["btncopy"] << btncopy;
	plc["btn_ytfmtlist"] << btn_ytfmtlist;
	plc["btndl"] << btndl;
	plc["btnq"] << btnq;

	plc.field_display("prog", false);
	plc.field_display("separator", true);
	plc.field_display("btncopy", false);
	plc.field_display("btncopy_spacer", false);
	show_btnfmt(false);

	prog.amount(1000);
	prog.caption("");

	gpopt.size({10, 10}); // workaround for weird caption display bug
	gpopt.enable_format_caption(true);

	gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=122> <weight=15> <l_outpath> > <weight=20>
			<weight=25 
				<l_rate weight=144> <weight=15> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbtime weight=304> <> <cbsubs weight=140>
			> <weight=20>
			<weight=25 <l_chap weight=67> <weight=10> <com_chap weight=65> <> <cbkeyframes weight=194> <> <cbthumb weight=152> <> <cbmp3 weight=181>>
			<weight=20> <weight=24 <cbargs weight=164> <weight=15> <com_args> <weight=10> <btnlabel weight=24> <weight=10> <btnerase weight=24>>
		)");

	gpopt["l_out"] << l_out;
	gpopt["l_outpath"] << l_outpath;
	gpopt["l_rate"] << l_rate;
	gpopt["tbrate"] << tbrate;
	gpopt["com_rate"] << com_rate;
	gpopt["l_chap"] << l_chap;
	gpopt["cbkeyframes"] << cbkeyframes;
	gpopt["cbmp3"] << cbmp3;
	gpopt["com_chap"] << com_chap;
	gpopt["cbsubs"] << cbsubs;
	gpopt["cbthumb"] << cbthumb;
	gpopt["cbtime"] << cbtime;
	gpopt["cbargs"] << cbargs;
	gpopt["com_args"] << com_args;
	gpopt["btnlabel"] << btnlabel;
	gpopt["btnerase"] << btnerase;

		const auto dpi {API::screen_dpi(true)};
		if(dpi >= 240)
		{
			btnerase.image(arr_erase48_png, sizeof arr_erase48_png);
			btnerase.image_disabled(arr_erase48_disabled_png, sizeof arr_erase48_disabled_png);
		}
		else if(dpi >= 192)
		{
			btnerase.image(arr_erase32_png, sizeof arr_erase32_png);
			btnerase.image_disabled(arr_erase32_disabled_png, sizeof arr_erase32_disabled_png);
		}
		else if(dpi > 96)
		{
			btnerase.image(arr_erase22_ico, sizeof arr_erase22_ico);
			btnerase.image_disabled(arr_erase22_disabled_ico, sizeof arr_erase22_disabled_ico);
		}
		else
		{
			btnerase.image(arr_erase16_ico, sizeof arr_erase16_ico);
			btnerase.image_disabled(arr_erase16_disabled_ico, sizeof arr_erase16_disabled_ico);
		}

	tbrate.multi_lines(false);

	//for(auto &str : conf.argsets)
	//	com_args.push_back(to_utf8(str));
	for(int n {0}; n < conf.argsets.size(); n++)
	{
		const auto argset {to_utf8(conf.argsets[n])};
		const auto key {argset_without_label(argset)};
		com_args[key].text(argset);
	}
	com_args.caption(conf.argset);
	com_args.editable(true);

	com_args.events().focus([&](const arg_focus &arg)
	{
		if(!arg.getting)
		{
			bottoms.current().argset = conf.argset = com_args.caption();
		}
	});

	com_args.events().selected([&]
	{
		conf.argset = com_args.caption();
		if(conf.common_dl_options && api::focus_window() == com_args)
		{
			for(auto &pbot : bottoms)
			{
				auto &bot {*pbot.second};
				bot.argset = com_args.caption();
			}
		}
		else bottoms.current().argset = conf.argset;
	});

	com_args.events().text_changed([&]
	{
		auto idx {com_args.caption_index()};
		if(idx != -1) btnerase.enable(true);
		else btnerase.enable(false);
		if(api::focus_window() == com_args && !bot_showing)
		{
			if(conf.common_dl_options)
			{
				for(auto &pbot : bottoms)
				{
					auto &bot {*pbot.second};
					bot.argset = com_args.caption();
				}
			}
			else bottoms.current().argset = com_args.caption();
		}
	});

	btnlabel.events().click([&]
	{
		const auto existing_label {label_from_argset(com_args.caption())};
		auto input {input_box(*this, "Argset label", "Label for argument set:", existing_label.data())};
		if(input.first)
		{
			while(!input.second.empty() && input.second.front() == '[')
				input.second.erase(0, 1);
			while(!input.second.empty() && input.second.back() == ']')
				input.second.pop_back();
			const auto caption {com_args.caption()};
			const auto argset {argset_without_label(caption)};
			const auto new_text {input.second.empty() ? argset : '[' + input.second + "] " + argset};
			auto it {std::find(conf.argsets.begin(), conf.argsets.end(), caption)};
			if(it != conf.argsets.end())
				*it = new_text;
			if(com_args.caption_index() != -1)
				com_args[argset].text(new_text);
			com_args.caption(new_text);
			if(conf.common_dl_options)
			{
				for(auto &pbot : bottoms)
				{
					auto &bot {*pbot.second};
					bot.argset = new_text;
				}
			}
			else bottoms.current().argset = com_args.caption();
		}
	});

	btnerase.events().click([&]
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
				bottoms.propagate_args_options(bottoms.current());
		}
	});

	btnq.events().click([&]
	{
		if(btnq.caption().find("queue") != -1)
			show_queue();
		else show_output();
	});

	btncopy.events().click([&]
	{
		auto &curbot {bottoms.current()};
		bottoms.propagate_cb_options(curbot);
		bottoms.propagate_args_options(curbot);
		bottoms.propagate_misc_options(curbot);
	});

	if(conf.ratelim)
		tbrate.caption(util::format_double(conf.ratelim, 1));
	else tbrate.caption("");

	tbrate.set_accept([this](wchar_t wc)->bool
	{
		const auto selpoints {tbrate.selection()};
		const auto selcount {selpoints.second.x - selpoints.first.x};
		return wc == keyboard::backspace || wc == keyboard::del || ((isdigit(wc) || wc == '.') && tbrate.text().size() < 5 || selcount);
	});

	tbrate.events().focus([this](const arg_focus &arg)
	{
		if(!arg.getting)
		{
			auto str {tbrate.caption()};
			if(!str.empty() && str.back() == '.')
			{
				str.pop_back();
				tbrate.caption(str);
			}
			GUI::conf.ratelim = tbrate.to_double();
		}
	});

	tbrate.events().text_changed([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			curbot.rate = tbrate.text();
			if(conf.common_dl_options && tbrate == api::focus_window())
				bottoms.propagate_misc_options(curbot);
		}
	});

	com_chap.editable(false);
	com_chap.push_back(" ignore");
	com_chap.push_back(" embed");
	com_chap.push_back(" split");

	com_rate.editable(false);
	com_rate.push_back(" KB/s");
	com_rate.push_back(" MB/s");

	com_chap.option(conf.com_chap);
	com_rate.option(conf.ratelim_unit);

	com_rate.events().selected([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			curbot.ratelim_unit = conf.ratelim_unit = com_rate.option();
			if(conf.common_dl_options && com_rate == api::focus_window())
				bottoms.propagate_misc_options(curbot);
		}
	});

	com_chap.events().selected([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			curbot.com_chap = conf.com_chap = com_chap.option();
			if(conf.common_dl_options)
				bottoms.propagate_misc_options(curbot);
		}
	});

	expcol.events().click([&]
	{
		auto wdsz {api::window_size(*this)};
		const auto px {(nana::API::screen_dpi(true) >= 144) * queue_panel.visible()};
		auto &plc {get_place()};
		change_field_attr(plc, "Bottom", "weight", 325 - 240 * expcol.collapsed() - 27 * queue_panel.visible() - px);
		auto &bot {bottoms.current()};
		if(expcol.collapsed())
		{
			if(!no_draw_freeze)
			{
				conf.gpopt_hidden = true;
				SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
			}
			auto dh {dpi_scale(240)};
			wdsz.height -= dh;
			minh -= dh;
			api::window_size(*this, wdsz);
			plc.field_display("gpopt", false);
			plc.field_display("gpopt_spacer", false);
			plc.collocate();
			if(!no_draw_freeze)
			{
				SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
				nana::api::refresh_window(*this);
			}
		}
		else
		{
			if(!no_draw_freeze)
			{
				conf.gpopt_hidden = false;
				SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
			}
			auto dh {dpi_scale(240)};
			wdsz.height += dh;
			minh += dh;
			api::window_size(*this, wdsz);
			plc.field_display("gpopt", true);
			plc.field_display("gpopt_spacer", true);
			plc.collocate();
			if(!no_draw_freeze)
			{
				SendMessageA(hwnd, WM_SETREDRAW, TRUE, 0);
				nana::api::refresh_window(*this);
			}
		}
	});

	com_chap.tooltip("<bold>--embed-chapters</>\tAdd chapter markers to the video file.\n"
		"<bold>--split-chapters</> \t\tSplit video into multiple files based on chapters.");
	cbkeyframes.tooltip("Force keyframes around the chapters before\nremoving/splitting them. Requires a\n"
		"reencode and thus is very slow, but the\nresulting video may have fewer artifacts\n"
		"around the cuts. (<bold>--force-keyframes-at-cuts</>)");
	cbtime.tooltip("Do not use the Last-modified header to set the file modification time (<bold>--no-mtime</>)");
	cbsubs.tooltip("Embed subtitles in the video (only for mp4, webm and mkv videos) (<bold>--embed-subs</>)");
	cbthumb.tooltip("Embed thumbnail in the video as cover art (<bold>--embed-thumbnail</>)");
	cbmp3.tooltip("Convert the source audio to MPEG Layer 3 format and save it to an .mp3 file.\n"
		"The video is discarded if present, so it's preferable to download an audio-only\n"
		"format if one is available. (<bold>-x --audio-format mp3</>)\n\n"
		"To download the best audio-only format available, use the custom\nargument <bold>-f ba</>");
	btnlabel.tooltip("Label this argument set");
	btnerase.tooltip("Remove this argument set from the list");
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

	btndl.events().click([this] { on_btn_dl(bottoms.current().url); });

	btndl.events().key_press([this](const arg_keyboard &arg)
	{
		if(arg.key == '\r')
			on_btn_dl(bottoms.current().url);
	});

	btn_ytfmtlist.events().click([this] { fm_formats(); });

	l_outpath.events().click([this]
	{
		auto &outpath {bottoms.current().outpath};

		auto clip_text = [&](const std::wstring &str, int max_pixels) -> std::wstring
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

		auto pop_folder_selection_box = [&]
		{
			nana::folderbox fb {*this, conf.open_dialog_origin ? outpath : appdir};
			fb.allow_multi_select(false);
			fb.title("Locate and select the desired download folder");
			auto res {fb()};
			if(res.size())
			{
				auto &curbot {bottoms.current()};
				conf.outpaths.insert(outpath);
				auto newpath {util::to_relative_path(res.front())};
				outpath = conf.outpath = newpath;
				l_outpath.caption(outpath.u8string());
				if(conf.common_dl_options)
					bottoms.propagate_misc_options(curbot);
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
			m.append("Choose folder...", [&](menu::item_proxy &)
			{
				pop_folder_selection_box();
				if(conf.outpaths.size() >= 11 && conf.outpaths.find(outpath) == conf.outpaths.end())
					conf.outpaths.erase(conf.outpaths.begin());
				conf.outpaths.insert(outpath);
			});
			m.append("Clear folder history", [&](menu::item_proxy &)
			{
				conf.outpaths.clear();
				conf.outpaths.insert(outpath);
			});
			m.append_splitter();
			for(auto &path : conf.outpaths)
			{
				if(path != outpath)
				{
					m.append(to_utf8(clip_text(path, dpi_scale(250))), [&](menu::item_proxy &)
					{
						outpath = conf.outpath = path;
						l_outpath.update_caption();
						conf.outpaths.insert(outpath);
						if(conf.common_dl_options)
							bottoms.propagate_misc_options(bottoms.current());
					});
				}
			}
			m.max_pixels(dpi_scale(280));
			m.item_pixels(24);
			auto curpos {api::cursor_position()};
			m.popup_await(nullptr, curpos.x - 142, curpos.y);
			queue_panel.focus();
		}
	});

	cbsubs.check(conf.cbsubs);
	cbthumb.check(conf.cbthumb);
	cbtime.check(conf.cbtime);
	cbkeyframes.check(conf.cbkeyframes);
	cbmp3.check(conf.cbmp3);
	cbargs.check(conf.cbargs);

	cbtime.events().checked([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			conf.cbtime = curbot.cbtime = cbtime.checked();
			if(conf.common_dl_options && cbtime == api::focus_window())
				bottoms.propagate_cb_options(curbot);
		}
	});
	cbthumb.events().checked([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			conf.cbthumb = curbot.cbthumb = cbthumb.checked();
			if(conf.common_dl_options && cbthumb == api::focus_window())
				bottoms.propagate_cb_options(curbot);
		}
	});
	cbsubs.events().checked([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			conf.cbsubs = curbot.cbsubs = cbsubs.checked();
			if(conf.common_dl_options && cbsubs == api::focus_window())
				bottoms.propagate_cb_options(curbot);
		}
	});
	cbkeyframes.events().checked([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			conf.cbkeyframes = curbot.cbkeyframes = cbkeyframes.checked();
			if(conf.common_dl_options && cbkeyframes == api::focus_window())
				bottoms.propagate_cb_options(curbot);
		}
	});
	cbmp3.events().checked([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			conf.cbmp3 = curbot.cbmp3 = cbmp3.checked();
			if(conf.common_dl_options && cbmp3 == api::focus_window())
				bottoms.propagate_cb_options(curbot);
		}
	});
	cbargs.events().checked([&]
	{
		if(!bot_showing)
		{
			auto &curbot {bottoms.current()};
			conf.cbargs = curbot.cbargs = cbargs.checked();
			if(conf.common_dl_options && cbargs == api::focus_window())
				bottoms.propagate_cb_options(curbot);
		}
	});

	com_args.events().mouse_up([this](const arg_mouse &arg)
	{
		if(arg.button == mouse::right_button)
		{
			using namespace nana;
			::widgets::Menu m;
			m.item_pixels(24);

			auto cliptext {util::get_clipboard_text()};

			m.append("Paste", [&](menu::item_proxy)
			{
				com_args.focus();
				keybd_event(VK_LCONTROL, 0, 0, 0);
				keybd_event('V', 0, 0, 0);
				keybd_event('V', 0, KEYEVENTF_KEYUP, 0);
				keybd_event(VK_LCONTROL, 0, KEYEVENTF_KEYUP, 0);
			}).enabled(!cliptext.empty());

			m.popup_await(com_args, arg.pos.x, arg.pos.y);
		}
	});

	queue_panel.focus();
	plc.collocate();
}


void GUI::make_message_handlers()
{
	using namespace nana;

	if(WM_TASKBAR_BUTTON_CREATED = RegisterWindowMessageW(L"TaskbarButtonCreated"))
	{
		msg.make_after(WM_TASKBAR_BUTTON_CREATED, [&](UINT, WPARAM, LPARAM, LRESULT *)
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


	msg.make_after(WM_COPYDATA, [&](UINT, WPARAM, LPARAM lparam, LRESULT *)
	{
		auto pcds {reinterpret_cast<PCOPYDATASTRUCT>(lparam)};
		std::wstring url {reinterpret_cast<LPCWSTR>(pcds->lpData), pcds->cbData / 2};
		auto item {lbq.item_from_value(url)};
		if(!item.empty())
		{
			switch(pcds->dwData)
			{
			case YTDLP_POSTPROCESS:
				if(bottoms.at(url).started)
					item.text(3, "processing");
				else return true;
				if(conf.cb_lengthyproc && bottoms.contains(url))
				{
					auto &bottom {bottoms.at(url)};
					if(!bottom.received_procmsg)
					{
						bottom.received_procmsg = true;
						bottom.timer_proc.interval(conf.max_proc_dur);
						bottom.timer_proc.elapse([&, url]
						{
							if(bottoms.contains(url))
							{
								bottom.timer_proc.stop();
								bottom.started = false;
								start_next_urls(url);
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
		else if(pcds->dwData == ADD_URL)
			add_url(url);
		return true;
	});


	msg.make_after(WM_SYSCOMMAND, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == 1337)
		{
			restore();
			bring_top(true);
			center(1000, 703 - expcol.collapsed() * 240);
		}
		return true;
	});


	msg.make_after(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == 'S')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				fm_settings();
			}
		}
		if(wparam == 'V')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				if(api::focus_window() != com_args)
				{
					l_url.events().mouse_up.emit({}, l_url);
					if(!queue_panel.visible())
						show_queue(true);
				}
			}
		}
		else if(wparam == 'C')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				if(queue_panel.visible() && api::focus_window() != com_args && api::focus_window() != tbrate)
				{
					std::wstring text;
					const auto sel {lbq.selected()};
					for(const auto &ipair : sel)
					{
						if(!text.empty())
							text += L'\n';
						text += lbq.at(ipair).value<lbqval_t>().url;
					}
					util::set_clipboard_text(hwnd, text);
				}
			}
		}
		else if(wparam == VK_NUMPAD0)
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				if(is_zoomed(true)) restore();
				center(1000, 703 - expcol.collapsed() * 240);
			}
		}
		else if(wparam == VK_DELETE)
		{
			if(queue_panel.visible())
			{
				const auto sel {lbq.selected()};
				if(!sel.empty())
				{
					if(sel.size() == 1)
					{
						auto fwnd {api::focus_window()};
						auto &bottom {bottoms.current()};
						if(fwnd != com_args && fwnd != tbrate)
						{
							outbox.clear(bottom.url);
							queue_remove_item(bottom.url);
						}
					}
					else
					{
						if(sel.size() == lbq.at(sel.front().cat).size())
							queue_remove_all(sel.front().cat);
						else queue_remove_items(sel);
					}
				}
			}
		}
		else if(wparam == 'F')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				if(bottoms.current().btnfmt_visible())
					fm_formats();
			}
		}
		else if(wparam == 'A' && GetAsyncKeyState(VK_CONTROL) & 0xff00)
		{
			if(api::focus_window() != com_args && api::focus_window() != tbrate)
			{
				if(queue_panel.visible() && !lbq.first_visible().empty())
				{
					if(api::focus_window() == lbq)
						outbox.focus();
					size_t cat {0};
					auto sel {lbq.selected()};
					if(!sel.empty())
						cat = sel.front().cat;
					auto hsel {lbq.events().selected.connect_front([](const nana::arg_listbox &arg) {arg.stop_propagation(); })};
					lbq.auto_draw(false);
					for(auto item : lbq.at(cat))
						item.select(true);
					lbq.auto_draw(true);
					lbq.events().selected.remove(hsel);
				}
			}
		}
		else if(wparam == VK_F2)
		{
			if(queue_panel.visible() && lbq.selected().size())
			{
				auto &bottom {bottoms.current()};
				if(!bottom.is_ytplaylist && !bottom.is_bcplaylist && !bottom.is_ytchan && !bottom.is_yttab && !bottom.is_bcchan)
					bottom.browse_for_filename();
			}
		}
		else if(wparam == VK_APPS)
		{
			auto curpos {api::cursor_position()};
			api::calc_window_point(*this, curpos);
			auto pos {btn_qact.pos()};
			auto url {queue_pop_menu(pos.x, pos.y)};
			if(!url.empty())
			{
				lbq.auto_draw(false);
				queue_remove_item(url);
				lbq.auto_draw(true);
			}
		}
		return true;
	});


	msg.make_before(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(queue_panel.visible())
		{
			if(wparam == VK_UP)
			{
				const auto sel {lbq.selected()};
				if(!sel.empty())
				{
					const auto cat {sel.front().cat};
					if(lbq.at(cat).size() > 1)
					{
						if(GetAsyncKeyState(VK_SHIFT) & 0xff00)
						{
							if(!sel.empty() && sel.front().item > 0)
								lbq.at(listbox::index_pair {cat, sel.front().item - 1}).select(true, true);
						}
						else if(lbq.at(cat).size() > 1)
							lbq.move_select(true);
						return false;
					}
				}
			}
			else if(wparam == VK_DOWN)
			{
				const auto sel {lbq.selected()};
				if(!sel.empty())
				{
					const auto cat {sel.front().cat};
					if(lbq.at(cat).size() > 1)
					{
						if(GetAsyncKeyState(VK_SHIFT) & 0xff00)
						{
							if(!sel.empty() && sel.back().item < lbq.at(cat).size() - 1)
								lbq.at(listbox::index_pair {cat, sel.back().item + 1}).select(true, true);
						}
						else lbq.move_select(false);
						return false;
					}
				}
			}
			else if(wparam == VK_HOME && api::focus_window() != com_args && api::focus_window() == tbrate)
			{
				if(!lbq.first_visible().empty())
				{
					const auto sel {lbq.selected()};
					auto cat {lbq.at(sel.empty() ? 0 : sel.front().cat)};
					if(cat.size() > 1)
					{
						lbq.auto_draw(false);
						auto first_item {cat.at(0)};
						if(GetAsyncKeyState(VK_SHIFT) & 0xff00)
						{
							if(!sel.empty() && sel.front().item > 0)
							{
								auto hsel {lbq.events().selected.connect_front([](const nana::arg_listbox &arg) {arg.stop_propagation(); })};
								for(auto n {sel.front().item}; n != npos; n--)
								{
									if(n - 1 == npos)
										lbq.events().selected.remove(hsel);
									cat.at(n).select(true);
								}
								const auto visibles {lbq.visibles()};
								if(std::find(visibles.begin(), visibles.end(), cat.at(0).pos()) == visibles.end())
									lbq.scroll(false, cat.position());
							}
						}
						else
						{
							cat.select(false);
							cat.at(0).select(true, true);
						}
						lbq.auto_draw(true);
						return false;
					}
				}
			}
			else if(wparam == VK_END && api::focus_window() != com_args && api::focus_window() == tbrate)
			{
				if(!lbq.first_visible().empty())
				{
					const auto sel {lbq.selected()};
					auto cat {lbq.at(sel.empty() ? 0 : sel.front().cat)};
					if(cat.size() > 1)
					{
						lbq.auto_draw(false);
						if(GetAsyncKeyState(VK_SHIFT) & 0xff00)
						{
							if(!sel.empty() && sel.back().item < cat.size() - 1)
							{
								auto hsel {lbq.events().selected.connect_front([](const nana::arg_listbox &arg) {arg.stop_propagation(); })};
								const auto catsize {cat.size()};
								for(auto n {sel.back().item}; n < catsize; n++)
								{
									if(n + 1 == catsize)
										lbq.events().selected.remove(hsel);
									cat.at(n).select(true);
								}
								const auto visibles {lbq.visibles()};
								if(std::find(visibles.begin(), visibles.end(), cat.at(cat.size()-1).pos()) == visibles.end())
									lbq.scroll(true, cat.position());
							}
						}
						else
						{
							cat.select(false);
							cat.at(cat.size() - 1).select(true, true);
						}
						lbq.auto_draw(true);
						return false;
					}
				}
			}
		}
		if(wparam == VK_ESCAPE)
		{
			close();
			return false;
		}
		else if(wparam == VK_TAB)
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				if(queue_panel.visible())
					show_output();
				else show_queue();
				api::refresh_window(*this);
				return false;
			}
		}
		return true;
	});


	msg.make_before(WM_GETMINMAXINFO, [&](UINT, WPARAM, LPARAM lparam, LRESULT *)
	{
		auto info {reinterpret_cast<PMINMAXINFO>(lparam)};
		info->ptMinTrackSize.x = conf.cbminw ? 0 : minw;
		info->ptMinTrackSize.y = minh;
		return false;
	});


	msg.make_before(WM_SETFOCUS, [&](UINT, WPARAM, LPARAM lparam, LRESULT *)
	{
		if(conf.cb_add_on_focus)
		{
			auto text {util::get_clipboard_text()};
			if((text.starts_with(L"http://") || text.starts_with(L"https://")) && text.find('\n') == -1)
			{
				if(text.starts_with(LR"(https://www.youtube.com/watch?v=)"))
					if(text.find(L"&list=") == 43)
						text.erase(43);
				auto item {lbq.item_from_value(text)};
				if(item == lbq.empty_item)
				{
					l_url.update_caption();
					add_url(text);
				}

				if(::widgets::theme::is_dark())
					l_url.fgcolor(::widgets::theme::path_link_fg);
				else l_url.fgcolor(::widgets::theme::path_link_fg);
			}
		}
		return false;
	});


	msg.make_before(WM_SHOW_BTNFMT, [&](UINT, WPARAM show, LPARAM, LRESULT *)
	{
		auto &plc {get_place()};
		plc.field_display("btn_ytfmtlist", show);
		plc.field_display("ytfm_spacer", show);
		plc.collocate();
		return false;
	});


	msg.make_before(WM_SET_QLINE_TEXT, [&](UINT, WPARAM url, LPARAM text, LRESULT *)
	{
		auto purl {reinterpret_cast<std::wstring*>(url)};
		auto ptext {reinterpret_cast<qline_t*>(text)};
		lbq.set_line_text(*purl, *ptext);
		delete purl;
		delete ptext;
		return false;
	});


	msg.make_before(WM_LBQ_AUTODRAW, [&](UINT, WPARAM enable, LPARAM, LRESULT *)
	{
		lbq.auto_draw(enable);
		return false;
	});


	msg.make_before(WM_REFRESH, [&](UINT, WPARAM wd, LPARAM, LRESULT *)
	{
		api::refresh_window(reinterpret_cast<nana::window>(wd));
		return false;
	});


	msg.make_before(WM_LBQ_SKIP, [&](UINT, WPARAM, LPARAM, LRESULT *)
	{
		if(!items_initialized)
			lbq.auto_draw(false);
		for(int cat {0}; cat < lbq.size_categ(); cat++)
			for(auto item : lbq.at(cat))
				if(item.text(3) == "skip")
					item.check(true);
		lbq.refresh_theme();
		if(!items_initialized)
			lbq.auto_draw(true);
		return false;
	});


	msg.make_before(WM_LBQ_NEWITEM, [&](UINT, WPARAM cat, LPARAM url, LRESULT *)
	{
		auto stridx {std::to_string(lbq.at(cat).size() + 1)};
		lbq.at(cat).append({stridx, "...", "...", "queued", "...", "...", "...", "..."});
		lbq.at(cat).back().value(lbqval_t {*reinterpret_cast<std::wstring*>(url), nullptr});
		//adjust_lbq_headers();
		return false;
	});


	msg.make_before(WM_LBQ_ITEMBG, [&](UINT, WPARAM url, LPARAM bg, LRESULT *)
	{
		lbq.item_from_value(*reinterpret_cast<std::wstring*>(url)).bgcolor(*reinterpret_cast<nana::color*>(bg));
		return false;
	});


	msg.make_before(WM_LBQ_ERASE, [&](UINT, WPARAM target, LPARAM res, LRESULT *)
	{
		*reinterpret_cast<nana::listbox::item_proxy*>(res) = lbq.listbox::erase(*reinterpret_cast<nana::listbox::item_proxy*>(target));
		return false;
	});


	msg.make_before(WM_LBQ_SELECT, [&](UINT, WPARAM url, LPARAM sel, LRESULT *)
	{
		auto item {lbq.item_from_value(*reinterpret_cast<std::wstring*>(url))};
		item.select(sel);
		return false;
	});


	msg.make_before(WM_LBQ_SELECT_EX, [&](UINT, WPARAM ip, LPARAM scroll, LRESULT *)
	{
		auto item {*reinterpret_cast<listbox::item_proxy*>(ip)};
		item.select(true, scroll);
		return false;
	});


	msg.make_before(WM_LBQ_URL2ITEM, [&](UINT, WPARAM url, LPARAM item, LRESULT *)
	{
		*reinterpret_cast<nana::listbox::item_proxy*>(item) = lbq.item_from_value(*reinterpret_cast<std::wstring*>(url));
		return false;
	});


	msg.make_before(WM_LBQ_IDX2ITEM, [&](UINT, WPARAM idx, LPARAM item, LRESULT *)
	{
		*reinterpret_cast<nana::listbox::item_proxy*>(item) = lbq.listbox::at(*reinterpret_cast<nana::listbox::index_pair*>(idx));
		return false;
	});


	msg.make_before(WM_LBQ_GETCAT, [&](UINT, WPARAM pos, LPARAM cat, LRESULT *)
	{
		*reinterpret_cast<nana::listbox::cat_proxy*>(cat) = lbq.listbox::at(pos);
		return false;
	});


	msg.make_before(WM_LBQ_HEADERS, [&](UINT, WPARAM, LPARAM, LRESULT *)
	{
		adjust_lbq_headers();
		return false;
	});


	msg.make_before(WM_SET_KEYWORDS, [&](UINT, WPARAM target, LPARAM params, LRESULT *)
	{
		const auto ppair {reinterpret_cast<std::pair<std::string, std::string>*>(params)};
		reinterpret_cast<::widgets::Textbox*>(target)->set_keywords(ppair->first, true, true, {ppair->second});
		delete ppair;
		return false;
	});


	msg.make_before(WM_COLORED_AREA_CLEAR, [&](UINT, WPARAM ca, LPARAM, LRESULT *)
	{
		reinterpret_cast<textbox::colored_area_access_interface*>(ca)->clear();
		return false;
	});


	msg.make_before(WM_PROGEX_CAPTION, [&](UINT, WPARAM progex, LPARAM utf8, LRESULT *)
	{
		reinterpret_cast<progress_ex*>(progex)->caption(*reinterpret_cast<std::string*>(utf8));
		return false;
	});


	msg.make_before(WM_PROGEX_VALUE, [&](UINT, WPARAM progex, LPARAM value, LRESULT *)
	{
		reinterpret_cast<progress_ex*>(progex)->value(value);
		return false;
	});


	msg.make_before(WM_OUTBOX_APPEND, [&](UINT, WPARAM wparam, LPARAM lparam, LRESULT *)
	{
		auto &outbox {*reinterpret_cast<GUI::Outbox*>(wparam)};
		auto &args {*reinterpret_cast<std::pair<std::wstring, std::string>*>(lparam)};
		outbox.append(args.first, args.second);
		return false;
	});
}