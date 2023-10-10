#include "gui.hpp"
#include "icons.hpp"

#include <regex>
#include <codecvt>
#include <algorithm>
#include <nana/gui/filebox.hpp>

GUI::settings_t GUI::conf;


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

	msg.make_after(WM_SYSCOMMAND, [&](UINT, WPARAM wparam, LPARAM, LRESULT*)
	{
		if(wparam == 1337)
		{
			restore();
			bring_top(true);
			center(1000, 703 - bottoms.current().expcol.collapsed() * 240);
		}
		return true;
	});

	msg.make_after(WM_ENDSESSION, [&](UINT, WPARAM wparam, LPARAM, LRESULT*)
	{
		if(wparam) close();
		return true;
	});

	msg.make_after(WM_POWERBROADCAST, [&](UINT, WPARAM wparam, LPARAM, LRESULT*)
	{
		if(wparam == PBT_APMSUSPEND) close();
		return true;
	});

	msg.make_after(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT*)
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
				if(api::focus_window() != bottoms.current().com_args)
				{
					l_url.events().mouse_up.emit({}, l_url);
					if(!queue_panel.visible())
						show_queue(true);
				}
			}
		}
		else if(wparam == VK_NUMPAD0)
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				if(is_zoomed(true)) restore();
				center(1000, 703 - bottoms.current().expcol.collapsed() * 240);
			}
		}
		else if(wparam == VK_DELETE)
		{
			if(queue_panel.visible())
			{
				auto sel {lbq.selected()};
				if(!sel.empty())
				{
					if(sel.size() == 1)
					{
						auto fwnd {api::focus_window()};
						auto &bottom {bottoms.current()};
						if(fwnd != bottom.com_args && fwnd != bottom.tbrate)
						{
							outbox.clear(bottom.url);
							remove_queue_item(bottom.url);
						}
					}
					else
					{
						if(sel.size() == lbq.at(0).size())
							queue_remove_all();
						else queue_remove_selected();
					}
				}
			}
		}
		else if(wparam == 'A')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00 && queue_panel.visible())
			{
				auto hsel {lbq.events().selected.connect_front([](const nana::arg_listbox &arg) {arg.stop_propagation(); })};
				lbq.auto_draw(false);
				for(auto item : lbq.at(0))
					item.select(true);
				lbq.auto_draw(true);
				lbq.events().selected.remove(hsel);
			}
		}
		else if(wparam == VK_APPS)
		{
			auto curpos {api::cursor_position()};
			api::calc_window_point(*this, curpos); 
			auto pos {btn_qact.pos()};
			auto url {pop_queue_menu(pos.x, pos.y)};
			if(!url.empty())
			{
				lbq.auto_draw(false);
				remove_queue_item(url);
				lbq.auto_draw(true);
			}
		}
		return true;
	});

	msg.make_before(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT*)
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

	msg.make_before(WM_GETMINMAXINFO, [&](UINT, WPARAM, LPARAM lparam, LRESULT*)
	{
		auto info {reinterpret_cast<PMINMAXINFO>(lparam)};
		info->ptMinTrackSize.x = minw;
		info->ptMinTrackSize.y = minh;
		return false;
	});

	auto langid {GetUserDefaultUILanguage()};
	if(langid == 2052 || langid == 3076 || langid == 5124 || langid == 4100 || langid == 1028)
		cnlang = true;

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

	::widgets::theme::contrast(conf.contrast);

	if(conf.cbtheme == 2)
	{
		if(system_supports_darkmode())
			system_theme(true);
		else conf.cbtheme = 1;
	}
	else if(conf.cbtheme == 0) dark_theme(true);
	if(conf.cbtheme == 1) dark_theme(false);

	if(conf.get_releases_at_startup)
		get_releases();
	caption(title);
	snap(conf.cbsnap);
	make_form();
	RegisterDragDrop(hwnd, this);

	if(system_theme())
		apply_theme(is_system_theme_dark());
	else apply_theme(dark_theme());

	get_versions();
	
	events().unload([&]
	{
		RevokeDragDrop(hwnd);
		conf.zoomed = is_zoomed(true);
		if(conf.zoomed || is_zoomed(false)) restore();
		conf.winrect = nana::rectangle {pos(), size()};
		menu_working = false;
		if(thr_menu.joinable())
			thr_menu.join();
		if(thr.joinable())
			thr.join();
		if(thr_releases.joinable())
			thr_releases.detach();
		if(thr_releases_ffmpeg.joinable())
			thr_releases_ffmpeg.detach();
		if(thr_releases_ytdlp.joinable())
			thr_releases_ytdlp.detach();
		if(thr_versions.joinable())
			thr_versions.detach();
		for(auto &bottom : bottoms)
		{
			auto &bot {*bottom.second};
			if(bot.info_thread.joinable())
			{
				bot.working_info = false;
				bot.info_thread.join();
			}
			if(bot.dl_thread.joinable())
			{
				bot.working = false;
				bot.dl_thread.detach();
			}
		}
		if(i_taskbar)
			i_taskbar.Release();

		conf.argset = bottoms.current().com_args.caption();

		conf.unfinished_queue_items.clear();
		for(auto item : lbq.at(0))
		{
			auto text {item.text(3)};
			if(text != "done" && text != "error")
				conf.unfinished_queue_items.push_back(to_utf8(item.value<lbqval_t>().url));
		}
	});

	for(auto &url : conf.unfinished_queue_items)
		add_url(to_wstring(url));

	if(conf.cb_queue_autostart && lbq.at(0).size())
		on_btn_dl(lbq.at(0).at(0).value<lbqval_t>());

	center(MINW, MINH);
	show_queue(false);
	if(conf.gpopt_hidden)
	{
		auto &expcol {bottoms.current().expcol};
		expcol.events().click.emit({}, expcol);
		if(conf.winrect.empty())
			center();
	}
	auto sz {api::window_outline_size(*this)};
	minw = sz.width;
	minh = sz.height;
	if(!conf.winrect.empty())
	{
		conf.winrect.width = dpi_transform(conf.winrect.width, conf.dpi);
		conf.winrect.height = dpi_transform(conf.winrect.height, conf.dpi);
		const auto maxh {screen {}.from_window(*this).area().dimension().height};
		if(conf.winrect.height < size().height)
			conf.winrect.height = size().height;
		move(conf.winrect);
		size(conf.winrect.dimension());
		sz = api::window_outline_size(*this);
		if(sz.height > maxh)
		{
			sz.height = maxh;
			api::window_outline_size(*this, sz);
		}
	}
	else center(1000, 703);
	bring_top(true);
	if(conf.zoomed) zoom(true);
	no_draw_freeze = false;

	auto m {GetSystemMenu(hwnd, FALSE)};
	if(m)
	{
		MENUITEMINFOA i {0};
		i.cbSize = sizeof MENUITEMINFO;
		i.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_ID;
		i.fType = MFT_STRING;
		i.wID = 1337;
		i.dwTypeData = (CHAR*)"Reset size and position\tCtrl+Num0";
		InsertMenuItemA(m, SC_CLOSE, FALSE, &i);
		i.dwTypeData = (CHAR*)"Close\tEsc, Alt+F4";
		i.fMask = MIIM_STRING;
		SetMenuItemInfoA(m, SC_CLOSE, FALSE, &i);
	}
}


bool GUI::process_queue_item(std::wstring url)
{
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
		if(bottom.cbargs.checked())
		{
			const auto args {bottom.com_args.caption()};
			auto idx {com_args.caption_index()};
			if(idx == -1)
			{
				const auto size {com_args.the_number_of_options()};
				com_args.push_back(args);
				com_args.option(size);
				if(size >= 10)
				{
					com_args.erase(0);
					conf.argsets.erase(conf.argsets.begin());
				}
				conf.argsets.push_back(args);
			}
			com_args.option(idx);
			if(conf.common_dl_options)
				bottoms.propagate_args_options(bottom);
		}

		std::wstring cmd {L'\"' + conf.ytdlp_path.wstring() + L'\"'};

		if(!bottom.using_custom_fmt())
		{
			if(bottom.use_strfmt)
			{
				if(bottom.fmt2.find('+') != -1 || bottom.fmt2 == L"mergeall")
					cmd += L" --audio-multistreams";
				cmd += L" -f " + bottom.strfmt;
			}
			else
			{
				std::wstring strpref {L" -S \""};
				if(conf.pref_res)
					strpref += L"res:" + com_res_options[conf.pref_res];
				if(conf.pref_video)
				{
					if(strpref.size() > 5)
						strpref += ',';
					strpref += L"vext:" + com_video_options[conf.pref_video];
				}
				if(conf.pref_audio)
				{
					if(strpref.size() > 5)
						strpref += ',';
					strpref += L"aext:" + com_audio_options[conf.pref_audio];
				}
				if(conf.pref_fps)
				{
					if(strpref.size() > 5)
						strpref += ',';
					strpref += L"fps";
				}
				if(conf.pref_vcodec)
				{
					if(strpref.size() > 5)
						strpref += ',';
					strpref += L"vcodec:" + com_vcodec_options[conf.pref_vcodec];
				}
				if(conf.pref_acodec)
				{
					if(strpref.size() > 5)
						strpref += ',';
					strpref += L"acodec:" + com_acodec_options[conf.pref_acodec];
				}
				if(strpref.size() > 5)
					cmd += strpref + L"\" ";
			}
		}

		if(cmd.back() != ' ')
			cmd.push_back(' ');

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
		if(bottom.cbargs.checked() && !argset.empty())
			cmd += argset + L" ";
		for(auto &section : bottom.sections)
		{
			auto arg
			{
				L"--download-sections *" + (section.first.empty() ? L"0" : section.first) + L'-' +
				(section.second.empty() || section.second == L"0" ? L"inf" : section.second) + L' '
			};
			cmd += arg;
		}

		if(bottom.is_ytlink || bottom.is_ytchan)
		{
			std::wstring mark, remove;
			if(conf.cb_sblock_mark && !conf.sblock_mark.empty())
			{
				if(conf.sblock_mark.front() == 0)
				{
					mark = L"all";
				}
				else if((sblock_cats_mark.size() - 1) / 2 < conf.sblock_mark.size())
				{
					mark = L"all";
					for(int n{1}; n < sblock_cats_mark.size(); n++)
					{
						if(std::find(conf.sblock_mark.begin(), conf.sblock_mark.end(), n) == conf.sblock_mark.end())
							mark += L",-" + sblock_cats_mark[n];
					}
				}
				else
				{
					for(auto idx : conf.sblock_mark)
						mark += sblock_cats_mark[idx] + L',';
					mark.pop_back();
				}
			}

			if(conf.cb_sblock_remove && !conf.sblock_remove.empty())
			{
				if(conf.sblock_remove.front() == 0)
				{
					remove = L"all";
				}
				else if((sblock_cats_remove.size() - 1) / 2 < conf.sblock_remove.size())
				{
					remove = L"all";
					for(int n {1}; n < sblock_cats_remove.size(); n++)
					{
						if(std::find(conf.sblock_remove.begin(), conf.sblock_remove.end(), n) == conf.sblock_remove.end())
							remove += L",-" + sblock_cats_remove[n];
					}
				}
				else
				{
					for(auto idx : conf.sblock_remove)
						remove += sblock_cats_remove[idx] + L',';
					remove.pop_back();
				}
			}

			if(!mark.empty() && bottom.com_args.caption().find("--sponsorblock-mark ") == -1)
				cmd += L"--sponsorblock-mark " + mark + L' ';
			if(!remove.empty() && bottom.com_args.caption().find("--sponsorblock-remove ") == -1)
				cmd += L"--sponsorblock-remove " + remove + L' ';
		}

		if(conf.cb_proxy && !conf.proxy.empty() && bottom.com_args.caption().find("--proxy ") == -1)
			cmd += L"--proxy " + conf.proxy + L' ';

		auto display_cmd {cmd};

		cmd += L"--encoding UTF-8 ";

		std::wstringstream ss;
		ss << hwnd;
		auto strhwnd {ss.str()};
		strhwnd.erase(0, strhwnd.find_first_not_of('0'));
		cmd += L"--exec \"after_video:\\\"" + self_path.wstring() + L"\\\" ytdlp_status " + std::to_wstring(YTDLP_POSTPROCESS)
			+ L" " + strhwnd + L" \\\"" + url + L"\\\"\" ";
		cmd += L"--exec \"before_dl:\\\"" + self_path.wstring() + L"\\\" ytdlp_status " + std::to_wstring(YTDLP_DOWNLOAD)
			+ L" " + strhwnd + L" \\\"" + url + L"\\\"\" ";
		fs::path tempfile;
		if(!bottom.is_ytplaylist && !bottom.is_ytchan && !bottom.is_bcplaylist)
		{
			tempfile = fs::temp_directory_path() / std::tmpnam(nullptr);
			cmd += L"--print-to-file after_move:filepath " + tempfile.wstring();
		}
		std::wstring cmd2;
		if(!bottom.cbargs.checked() || argset.find(L"-P ") == -1)
		{
			auto wstr {bottom.outpath.wstring()};
			if(wstr.find(' ') == -1)
				cmd2 += L" -P " + wstr;
			else cmd2 += L" -P \"" + wstr + L"\"";
		}
		if((!bottom.cbargs.checked() || argset.find(L"-o ") == -1) && !conf.output_template.empty())
		{
			std::wstring folder;
			if(bottom.is_ytplaylist)
			{
				if(conf.cb_playlist_folder)
					folder = L"%(playlist_title)s\\";
				if(!conf.playlist_indexing.empty())
				{
					if(conf.cb_zeropadding && bottom.playlist_selection.size() > 9)
					{
						auto pos {conf.playlist_indexing.find(L"%(playlist_index)d")};
						if(pos != -1)
						{
							auto str1 {conf.playlist_indexing.substr(0, pos)},
								str2 {conf.playlist_indexing.substr(pos + 18)},
								str_padval {std::to_wstring(std::to_string(bottom.playlist_selection.size()).size())},
								newstr {str1 + L"%(playlist_index)0" + str_padval + L'd' + str2};
							cmd2 += L" -o \"" + folder + newstr + conf.output_template + L'\"';
						}
					}
					else cmd2 += L" -o \"" + folder + conf.playlist_indexing + conf.output_template + L'\"';
				}
				else cmd2 += L" -o \"" + folder + conf.output_template + L'\"';
			}
			else if(bottom.is_bcplaylist || bottom.is_bcchan)
			{
				if(conf.cb_playlist_folder)
					folder = L"%(artist)s\\%(album)s\\";
				cmd2 += L" -o \"" + folder + conf.output_template_bandcamp + L'\"';
			}
			else if(bottom.is_ytchan)
			{
				if(conf.cb_playlist_folder)
					folder = L"%(playlist_title)s\\";
				cmd2 += L" -o \"" + folder + conf.output_template + L'\"';
			}
			else if(bottom.sections.size() > 1)
			{
				cmd2 += L" -o \"" + conf.output_template;
				if(cmd2.rfind(L".%(ext)s") == cmd2.size() - 8)
					cmd2.erase(cmd2.size() - 8);
				cmd2 += L" - section %(section_start)d to %(section_end)d.%(ext)s\"";
			}
			else cmd2 += L" -o \"" + conf.output_template + L'\"';
		}
		if((bottom.is_ytplaylist || bottom.is_bcplaylist) && !bottom.playsel_string.empty())
			cmd2 += L" -I " + bottom.playsel_string + L" --compat-options no-youtube-unavailable-videos";
		cmd2 += L" \"" + url + L'\"';
		display_cmd += cmd2;
		cmd += cmd2;
		//display_cmd = cmd;

		if(tbpipe.current() == url)
			tbpipe.clear();

		bottom.dl_thread = std::thread([&, this, tempfile, display_cmd, cmd, url]
		{
			working = true;
			auto ca {tbpipe.colored_area_access()};
			bool ca_change {false};
			if(outbox.current() == url)
			{
				ca->clear();
				ca_change = true;
			}
			if(fs::exists(conf.ytdlp_path))
				tbpipe.append(url, L"[GUI] executing command line: " + display_cmd + L"\n\n");
			else tbpipe.append(url, L"ytdlp.exe not found: " + conf.ytdlp_path.wstring());
			auto p {ca_change ? ca->get(0) : nullptr};
			if(ca_change)
				p->count = 1;
			if(!fs::exists(conf.ytdlp_path))
			{
				if(ca_change)
					p->fgcolor = theme::is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
				nana::API::refresh_window(tbpipe);
				btndl.caption("Start download");
				btndl.cancel_mode(false);
				if(bottom.dl_thread.joinable())
					bottom.dl_thread.detach();
				return;
			}
			if(ca_change)
				p->fgcolor = theme::is_dark() ? theme::path_link_fg : nana::color {"#569"};
			nana::API::refresh_window(tbpipe);
			ULONGLONG prev_val {0};
			bool playlist_progress {false};

			auto cb_progress = [&, this, url](ULONGLONG completed, ULONGLONG total, std::string text, int playlist_completed, int playlist_total)
			{
				if(playlist_total && !playlist_progress)
				{
					playlist_progress = true;
					prog.amount(playlist_total);
				}
				while(text.find_last_of("\r\n") != -1)
					text.pop_back();
				auto item {lbq.item_from_value(url)};
				if(total != -1)
				{
					auto strpct {(std::stringstream {} << static_cast<double>(completed) / 10).str() + '%'};
					if(playlist_progress)
					{
						auto strprog {"[" + std::to_string(playlist_completed + 1) + "/" + std::to_string(playlist_total) + "] "};
						item.text(3, strprog + strpct);
						if(i_taskbar && lbq.at(0).size() == 1)
							i_taskbar->SetProgressValue(hwnd, playlist_completed, playlist_total);
					}
					else 
					{
						item.text(3, strpct);
						if(i_taskbar && lbq.at(0).size() == 1)
							i_taskbar->SetProgressValue(hwnd, completed, total);
					}
				}
				if(completed < 1000 && total != -1)
				{
					if(playlist_progress)
						text = "[" + std::to_string(playlist_completed + 1) + " of " + std::to_string(playlist_total) + "]\t" + text;
					prog.caption(text);
				}
				else
				{
					if(prev_val)
					{
						if(text == "[ExtractAudio]" || text.find("[Merger]") == 0 || text.find("[Fixup") == 0)
						{
							COPYDATASTRUCT cds;
							cds.dwData = YTDLP_POSTPROCESS;
							cds.cbData = url.size() * 2;
							cds.lpData = const_cast<std::wstring&>(url).data();
							SendMessageW(hwnd, WM_COPYDATA, NULL, reinterpret_cast<LPARAM>(&cds));

							if(text.find("[Merger]") == 0)
							{
								auto pos {text.find('\"')};
								if(pos++ != -1)
								{
									auto pos2 {text.find('\"', pos)};
									if(pos2 != -1)
									{
										try { bottom.merger_path = fs::u8path(text.substr(pos, pos2 - pos)); }
										catch(...) { bottom.merger_path.clear(); }
									}
								}
							}
						}
						else if(total != -1)
						{
							auto pos {text.find_last_not_of(" \t")};
							if(text.size() > pos)
								text.erase(pos + 1);
							if(playlist_progress)
							{
								prog.shadow_progress(1000, completed);
								prog.value(playlist_completed + 1);
								auto strprog {"[" + std::to_string(playlist_completed + 1) + " of " + std::to_string(playlist_total) + "]\t"};
								prog.caption(strprog + text);
							}
							else prog.caption(text);
						}
					}
					if(total == -1 && text.find("[download]") == 0)
					{
						auto pos {text.find("has already been downloaded")};
						if(pos != -1)
						{
							try { bottom.download_path = fs::u8path(text.substr(11, pos - 2)); }
							catch(...) { bottom.download_path.clear(); }
						}
						else
						{
							pos = text.find("Destination: ");
							if(pos != -1)
							{
								try { bottom.download_path = fs::u8path(text.substr(24)); }
								catch(...) { bottom.download_path.clear(); }
							}
							else bottom.download_path.clear();
						}
						return;
					}
				}
				if(playlist_progress)
					prog.shadow_progress(1000, completed);
				else if(completed <= 1000 && (completed > prog.value() || completed == 0 || prog.value() - completed > 50))
					prog.value(completed);
				prev_val = completed;
			};

			auto item {lbq.item_from_value(url)};
			prog.value(0);
			prog.caption("");
			if(i_taskbar && lbq.at(0).size() == 1) 
				i_taskbar->SetProgressState(hwnd, TBPF_NORMAL);
			if(!item.empty())
				item.text(3, "started");
			auto cb_append = [url, this](std::string text, bool keyword)
			{
				if(keyword)
					outbox.set_keyword(text);
				else outbox.append(url, text);
			};
			bottom.printed_path.clear();
			bottom.merger_path.clear();
			bottom.download_path.clear();
			auto outpath {bottom.outpath};
			util::run_piped_process(cmd, &working, cb_append, cb_progress, &graceful_exit, tempfile.filename().string());
			bottom.received_procmsg = false;
			if(!tempfile.empty() && fs::exists(tempfile))
			{
				if(std::ifstream is {tempfile, std::ios::binary | std::ios::ate})
				{
					auto size {is.tellg()};
					std::string str (size, '\0');
					is.seekg(0);
					if(is.read(&str.front(), size))
					{
						while(str.find_last_of("\r\n") != -1)
							str.pop_back();
						bottom.printed_path = fs::u8path(str);
					}
				}
				std::error_code ec;
				fs::remove(tempfile, ec);
			}
			else if(bottom.is_ytplaylist && bottom.playlist_info.contains("title") && bottom.playlist_info["title"] != nullptr)
			{
				bottom.printed_path = bottom.outpath / bottom.playlist_info["title"].get<std::string>();
			}
			else if(bottom.is_bcplaylist)
			{
				// todo
			}

			if(working && bottom.index > 0)
			{
				if(bottom.timer_proc.started())
					bottom.timer_proc.stop();
				item = lbq.item_from_value(url);
				item.text(3, "done");
				taskbar_overall_progress();
				if(i_taskbar && lbq.at(0).size() == 1)
					i_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
				btndl.enabled(true);
				tbpipe.append(url, "\n[GUI] " + conf.ytdlp_path.filename().string() + " process has exited\n");
				if(tbpipe.current() == url)
				{
					ca = tbpipe.colored_area_access();
					p = ca->get(tbpipe.text_line_count() - 2);
					p->count = 1;
					p->fgcolor = theme::is_dark() ? theme::path_link_fg : nana::color {"#569"};
					nana::API::refresh_window(tbpipe);
				}
				btndl.caption("Start download");
				btndl.cancel_mode(false);
				auto next_url {next_startable_url(url)};
				if(!next_url.empty())
					on_btn_dl(next_url);
				if(working && bottom.dl_thread.joinable())
					bottom.dl_thread.detach();
			}
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
		const auto fname {conf.ytdlp_path.filename().string()};
		if(graceful_exit)
			tbpipe.append(url, "\n[GUI] " + fname + " process was ended gracefully via Ctrl+C signal\n");
		else tbpipe.append(url, "\n[GUI] " + fname + " process was ended forcefully via WM_CLOSE message\n");
		auto item {lbq.item_from_value(url)};
		auto text {item.text(3)};
		auto pos {text.find(']')};
		if(pos != -1)
			item.text(3, "stopped (" + text.substr(1, pos-1) + ")");
		else if(text.find('%') != -1)
			item.text(3, "stopped (" + text + ")");
		else item.text(3, "stopped");
		if(tbpipe.current() == url)
		{
			auto ca {tbpipe.colored_area_access()};
			auto p {ca->get(tbpipe.text_line_count() - 2)};
			p->count = 1;
			if(graceful_exit)
				p->fgcolor = theme::is_dark() ? theme::path_link_fg : nana::color {"#569"};
			else p->fgcolor = theme::is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
		}
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


void GUI::add_url(std::wstring url, bool refresh)
{
	using namespace nana;

	auto item {lbq.item_from_value(url)};

	if(item == lbq.at(0).end() || refresh)
	{
		auto stridx {std::to_string(lbq.at(0).size() + 1)};
		if(refresh)
		{
			auto status_text {item.text(3)};
			for(int n {1}; n < item.columns(); n++)
				item.text(n, "...");
			item.text(3, status_text);
			stridx = item.text(0);
		}
		else 
		{
			lbq.at(0).append({stridx, "...", "...", "queued", "...", "...", "...", "..."});
			lbq.at(0).back().value(lbqval_t {url, nullptr});
			adjust_lbq_headers();
		}

		auto &bottom {bottoms.add(url)};
		if(!refresh)
		{
			if(conf.common_dl_options)
				bottom.gpopt.caption("Download options");
			else bottom.gpopt.caption("Download options for queue item #" + stridx);
			bottom.show_btncopy(!conf.common_dl_options);
			if(lbq.at(0).size() == 1)
				lbq.item_from_value(url).select(true);
			else if(!conf.common_dl_options)
				bottom.show_btncopy(true);
		}

		bottom.info_thread = std::thread([&, this, url, refresh]
		{
			bottom.working_info = true;
			static std::atomic_int active_threads {0};
			while(active_threads >= number_of_processors)
			{
				Sleep(100);
				if(!bottom.working_info)
				{
					if(bottom.info_thread.joinable())
						bottom.info_thread.detach();
					return;
				}
			}
			active_threads++;

			auto favicon_url {lbq.favicon_url_from_value(url)};
			if(!favicon_url.empty())
			{
				auto item {lbq.item_from_value(url)};
				std::function<void(paint::image &)> cbfn = [favicon_url, url, this](paint::image &img)
				{
					auto item {lbq.item_from_value(url)};
					if(item != lbq.at(0).end())
					{
						item.value<lbqval_t>().pimg = &img;
					}
				};
				favicons[favicon_url].add(favicon_url, cbfn);
			}

			std::wstring fmt_sort {' '}, strpref {L" -S \""};
			if(conf.pref_res)
				strpref += L"res:" + com_res_options[conf.pref_res];
			if(conf.pref_video)
			{
				if(strpref.size() > 5)
					strpref += ',';
				strpref += L"vext:" + com_video_options[conf.pref_video];
			}
			if(conf.pref_audio)
			{
				if(strpref.size() > 5)
					strpref += ',';
				strpref += L"aext:" + com_audio_options[conf.pref_audio];
			}
			if(conf.pref_fps)
			{
				if(strpref.size() > 5)
					strpref += ',';
				strpref += L"fps";
			}
			if(conf.pref_vcodec)
			{
				if(strpref.size() > 5)
					strpref += ',';
				strpref += L"vcodec:" + com_vcodec_options[conf.pref_vcodec];
			}
			if(strpref.size() > 5)
				fmt_sort = strpref + L"\" ";

			std::string media_info, media_website {"---"}, media_title, format_id {"---"}, format_note {"---"}, ext {"---"}, filesize {"---"};
			auto json_error = [&](const nlohmann::detail::exception &e)
			{
				media_title = "Can't parse the JSON data produced by yt-dlp! See output for details.";
				lbq.item_from_value(url).text(3, "error");
				outbox.caption(e.what() + std::string {"\n\n"} + media_info, url);
				if(outbox.current() == url)
				{
					auto ca {outbox.colored_area_access()};
					ca->clear();
					auto p {ca->get(0)};
					p->fgcolor = ::widgets::theme::is_dark() ? ::widgets::theme::path_link_fg : color {"#569"};
				}
				if(!queue_panel.visible() && overlay.visible())
				{
					outbox.widget::show();
					overlay.hide();
				}
			};

			if(bottom.is_ytlink && !bottom.is_ytchan || bottom.is_bcplaylist)
			{
				if(bottom.is_ytplaylist || bottom.is_bcplaylist)
				{
					std::wstring compat_options {bottom.is_ytplaylist ? L" --compat-options no-youtube-unavailable-videos" : L""},
					             cmd {L" --no-warnings --flat-playlist -J" + compat_options + L" \"" + url + L'\"'};
					if(conf.cb_proxy && !conf.proxy.empty())
						cmd = L" --proxy " + conf.proxy + cmd;
					bottom.cmdinfo = conf.ytdlp_path.filename().wstring() + cmd;
					media_info = util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info);
					bool incomplete_data_received {false};
					if(media_info.starts_with("ERROR: Incomplete data received"))
					{
						auto pos {media_info.find('{')};
						if(pos != -1)
						{
							media_info.erase(0, pos);
							incomplete_data_received = true;
						}
					}
					if(!media_info.empty() && media_info.front() == '{')
					{
						try { bottom.playlist_info = nlohmann::json::parse(media_info); }
						catch(nlohmann::detail::exception e)
						{
							bottom.playlist_info.clear();
							if(lbq.item_from_value(url) != lbq.at(0).end())
								json_error(e);
						}
						if(!bottom.playlist_info.empty())
						{
							if(incomplete_data_received)
							{
								auto it {bottom.playlist_info["entries"].end()};
								bottom.playlist_info["entries"].erase(--it);
							}
							std::string URL {bottom.playlist_info["entries"][0]["url"]};
							cmd = L" --no-warnings -j " + fmt_sort + to_wstring(URL);
							media_info = {util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info)};
							if(!media_info.empty() && media_info.front() == '{')
							{
								try { bottom.vidinfo = nlohmann::json::parse(media_info); }
								catch(nlohmann::detail::exception e)
								{
									bottom.vidinfo.clear();
									if(lbq.item_from_value(url) != lbq.at(0).end())
										json_error(e);
								}
								if(!bottom.vidinfo.empty())
									bottom.show_btnfmt(true);
							}
							else bottom.playlist_vid_cmdinfo = conf.ytdlp_path.filename().wstring() + cmd;
						}
					}
				}
				else // YouTube video
				{
					std::wstring cmd {L" --no-warnings -j " + (conf.output_template.empty() ? L"" : L"-o \"" + conf.output_template + L'\"') + 
						fmt_sort + L'\"' + url + L'\"'};
					if(conf.cb_proxy && !conf.proxy.empty())
						cmd = L" --proxy " + conf.proxy + cmd;
					bottom.cmdinfo = conf.ytdlp_path.filename().wstring() + cmd;
					media_info = util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info);
					if(media_info.find("ERROR:") == 0)
					{
						auto pos {media_info.find("This live event will begin in ")};
						if(pos != -1)
						{
							auto strtime {media_info.substr(pos + 30, media_info.rfind('.') - pos - 30)};
							lbq.item_from_value(url).text(1, "youtube.com");
							lbq.item_from_value(url).text(2, "[live event scheduled to begin in " + strtime + ']');
							lbq.item_from_value(url).text(4, "---");
							lbq.item_from_value(url).text(5, "---");
							lbq.item_from_value(url).text(6, "---");
							lbq.item_from_value(url).text(7, "---");
							bottom.vidinfo.clear();
							active_threads--;
							if(bottom.working_info)
								bottom.info_thread.detach();
							return;
						}
					}
					auto pos {media_info.rfind('}')};
					if(pos != -1)
						media_info.erase(pos+1);
					if(bottom.working_info)
					{
						auto pos {media_info.find("{\"id\":")};
						if(pos != -1)
						{
							media_info.erase(0, pos);
							try { bottom.vidinfo = nlohmann::json::parse(media_info); }
							catch(nlohmann::detail::exception e)
							{
								bottom.vidinfo.clear();
								if(lbq.item_from_value(url) != lbq.at(0).end())
									json_error(e);
							}
						}
					}
				}
			}
			else if(bottom.is_ytchan || bottom.is_bcchan)
			{
				std::wstring cmd {L" --no-warnings --flat-playlist -I :0 -J \"" + url + L'\"'};
				if(refresh)
					cmd = L" --no-warnings --flat-playlist -J \"" + url + L'\"';
				if(conf.cb_proxy && !conf.proxy.empty())
					cmd = L" --proxy " + conf.proxy + cmd;
				bottom.cmdinfo = conf.ytdlp_path.filename().wstring() + cmd;
				media_info = util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info);
				try { bottom.playlist_info = nlohmann::json::parse(media_info); }
				catch(nlohmann::detail::exception e)
				{
					bottom.playlist_info.clear();
					if(lbq.item_from_value(url) != lbq.at(0).end())
						json_error(e);
				}
				if(!bottom.playlist_info.empty())
				{
					std::string tab {bottom.is_bcchan ? "[user page] " : "[whole channel] "};
					std::vector<std::string> tabs {"videos", "featured", "playlists", "shorts", "streams", "community", "podcasts"};
					if(bottom.is_ytchan)
						for(const auto &t : tabs)
							if(url.rfind(L"/" + to_wstring(t)) == url.size() - t.size() - 1)
							{
								tab = "[channel tab] ";
								if(t == "videos" || t == "shorts" || t == "streams" || t == "podcasts")
									bottom.is_yttab = true;
								break;
							}
					auto media_title {bottom.playlist_info["title"].get<std::string>()};

					if(!is_utf8(media_title))
					{
						std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> u16conv;
						auto u16str {u16conv.from_bytes(media_title)};
						std::wstring wstr(u16str.size(), L'\0');
						memcpy(&wstr.front(), &u16str.front(), wstr.size() * 2);
						std::wstring_convert<std::codecvt_utf8<wchar_t>> u8conv;
						media_title = u8conv.to_bytes(wstr);
					}

					if(refresh)
					{
						std::string URL {bottom.playlist_info["entries"][0]["url"]};
						cmd = L" --no-warnings -j " + fmt_sort + to_wstring(URL);
						media_info = {util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info)};
						if(!media_info.empty() && media_info.front() == '{')
						{
							try { bottom.vidinfo = nlohmann::json::parse(media_info); }
							catch(nlohmann::detail::exception e)
							{
								bottom.vidinfo.clear();
								if(lbq.item_from_value(url) != lbq.at(0).end())
									json_error(e);
							}
							if(!bottom.vidinfo.empty())
								bottom.show_btnfmt(true);
						}
						else bottom.playlist_vid_cmdinfo = conf.ytdlp_path.filename().wstring() + cmd;
					}
					else
					{
						lbq.item_from_value(url).text(1, bottom.is_bcchan ? "bandcamp.com" : "youtube.com");
						lbq.item_from_value(url).text(2, tab + media_title);
						lbq.item_from_value(url).text(4, "---");
						lbq.item_from_value(url).text(5, "---");
						lbq.item_from_value(url).text(6, "---");
						lbq.item_from_value(url).text(7, "---");
						bottom.vidinfo.clear();
						if(vidsel_item.m && lbq.item_from_value(url).selected())
						{
							auto &m {*vidsel_item.m};
							for(int n {0}; n < m.size(); n++)
							{
								m.enabled(n, true);
								if(m.text(n) == "Select formats" && !bottom.btnfmt_visible())
									m.erase(n--);
							}
							api::refresh_window(m.handle());
							vidsel_item.m = nullptr;
						}

						active_threads--;
						if(bottom.working_info)
							bottom.info_thread.detach();
						return;
					}
				}
			}
			else // not YouTube
			{
				std::wstring cmd {L" --no-warnings -j " + (conf.output_template.empty() ? L"" : L"-o \"" + conf.output_template + L'\"') + 
					fmt_sort + L'\"' + url + L'\"'};
				if(conf.cb_proxy && !conf.proxy.empty())
					cmd = L" --proxy " + conf.proxy + cmd;
				bottom.cmdinfo = conf.ytdlp_path.filename().wstring() + cmd;
				media_info = util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info);
				auto pos {media_info.rfind('}')};
				if(pos != -1)
					media_info.erase(pos + 1);
				if(bottom.working_info)
				{
					auto pos {media_info.find('{')};
					if(pos != -1)
					{
						media_info.erase(0, pos);
						try { bottom.vidinfo = nlohmann::json::parse(media_info); }
						catch(nlohmann::detail::exception e)
						{
							bottom.vidinfo.clear();
							if(lbq.item_from_value(url) != lbq.at(0).end())
								json_error(e);
						}
					}
				}
			}
			if(!media_info.empty() && bottom.working_info)
			{
				if(media_info[0] == '{')
				{
					if(bottom.is_ytplaylist || bottom.is_bcplaylist)
					{
						if(!bottom.playlist_info.empty())
						{
							auto playlist_size {bottom.playlist_info["entries"].size()};
							if(bottom.playsel_string.size())
								bottom.apply_playsel_string();
							else bottom.playlist_selection.assign(playlist_size, true);
							if(bottom.is_bcplaylist)
								media_website = "bandcamp.com";
							else media_website = bottom.playlist_info["webpage_url_domain"];
							if(bottom.is_yttab)
								media_title = "[channel tab] " + std::string {bottom.playlist_info["title"]};
							else media_title = "[playlist] " + std::string {bottom.playlist_info["title"]};
							if(vidsel_item.m && lbq.item_from_value(url).selected())
							{
								auto &m {*vidsel_item.m};
								auto pos {vidsel_item.pos};
								auto str {std::to_string(playlist_size)};
								if(bottom.playsel_string.empty())
									m.text(pos, (bottom.is_ytplaylist ? "Select videos (" : "Select songs (") + str + '/' + str + ")");
								else m.text(pos, (bottom.is_ytplaylist ? "Select videos (" : "Select songs (") + std::to_string(bottom.playlist_selected()) + '/' + str + ")");
								m.enabled(pos, true);
								api::refresh_window(m.handle());
								vidsel_item.m = nullptr;
							}
							for(const auto &entry : bottom.playlist_info["entries"])
							{
								if(entry.contains("id") && entry["id"] != nullptr)
									outbox.set_keyword(entry["id"].get<std::string>() + ':', "id");
							}
						}
					}
					else if(!bottom.vidinfo.empty())
					{
						if(bottom.is_bclink)
							media_website = "bandcamp.com";
						else media_website = bottom.vidinfo["webpage_url_domain"];
						media_title = bottom.vidinfo["title"];
						if(bottom.vidinfo_contains("is_live") && bottom.vidinfo["is_live"] ||
							bottom.vidinfo_contains("live_status") && bottom.vidinfo["live_status"] == "is_live")
							media_title = "[live] " + media_title;
						if(bottom.vidinfo_contains("format_id"))
							format_id = bottom.vidinfo["format_id"];
						if(bottom.vidinfo_contains("resolution"))
							format_note = bottom.vidinfo["resolution"];
						else if(bottom.vidinfo_contains("format_note"))
							format_note = bottom.vidinfo["format_note"];
						if(bottom.vidinfo_contains("ext"))
							ext = bottom.vidinfo["ext"];
						if(bottom.vidinfo_contains("filesize"))
						{
							unsigned fsize {bottom.vidinfo["filesize"]};
							filesize = util::int_to_filesize(fsize, false);
						}
						else if(bottom.vidinfo_contains("filesize_approx"))
						{
							unsigned fsize {bottom.vidinfo["filesize_approx"]};
							if(bottom.vidinfo_contains("requested_formats"))
							{
								auto &reqfmt {bottom.vidinfo["requested_formats"]};
								if(reqfmt.size() == 2)
								{
									if(reqfmt[0].contains("filesize") && reqfmt[1].contains("filesize"))
										filesize = '~' + util::int_to_filesize(fsize, false);
									else filesize = "---";
								}
							}
							else filesize = '~' + util::int_to_filesize(fsize, false);
						}

						if(bottom.vidinfo_contains("formats"))
							bottom.show_btnfmt(true);
					}
					//if(!refresh)
					//{
						if(!is_utf8(media_title))
						{
							std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> u16conv;
							auto u16str {u16conv.from_bytes(media_title)};
							std::wstring wstr(u16str.size(), L'\0');
							memcpy(&wstr.front(), &u16str.front(), wstr.size() * 2);
							std::wstring_convert<std::codecvt_utf8<wchar_t>> u8conv;
							media_title = u8conv.to_bytes(wstr);
						}
						lbq.item_from_value(url).text(1, media_website);
						lbq.item_from_value(url).text(2, media_title);
						lbq.item_from_value(url).text(4, format_id);
						lbq.item_from_value(url).text(5, format_note);
						lbq.item_from_value(url).text(6, ext);
						lbq.item_from_value(url).text(7, filesize);

						if(!bottom.file_path().empty())
							lbq.item_from_value(url).text(3, "done");

						if(bottom.vidinfo_contains("id"))
						{
							std::string idstr {bottom.vidinfo["id"]};
							outbox.set_keyword(idstr + ':', "id");
						}
					//}
				}
				else
				{
					auto stridx {lbq.item_from_value(url).text(0)};
					lbq.item_from_value(url).text(1, "");
					lbq.item_from_value(url).text(2, "yt-dlp failed to get info (see output)");
					lbq.item_from_value(url).text(3, "error");
					lbq.item_from_value(url).text(4, "");
					lbq.item_from_value(url).text(5, "");
					lbq.item_from_value(url).text(6, "");
					lbq.item_from_value(url).text(7, "");

					auto cmdline {bottom.playlist_vid_cmdinfo.empty() ? to_utf8(bottom.cmdinfo) : to_utf8(bottom.playlist_vid_cmdinfo)};
					outbox.caption("[GUI] got error executing command line: " + cmdline + "\n\n" + media_info + "\n", url);
					if(outbox.current() == url)
					{
						auto ca {outbox.colored_area_access()};
						ca->clear();
						auto p {ca->get(0)};
						p->fgcolor = ::widgets::theme::is_dark() ? ::widgets::theme::path_link_fg : color {"#569"};
					}
					if(!queue_panel.visible() && overlay.visible())
					{
						outbox.widget::show();
						overlay.hide();
					}
					if(vidsel_item.m && lbq.item_from_value(url).selected())
					{
						auto &m {*vidsel_item.m};
						for(int n {0}; n < m.size(); n++)
						{
							if(!m.enabled(n))
								m.erase(n--);
						}
						for(int n {0}; n < m.size(); n++)
						{
							if(m.text(n).empty() && n < m.size() - 1 && m.text(n + 1).empty())
							{
								m.erase(n);
								break;
							}
						}						
						api::refresh_window(m.handle());
						vidsel_item.m = nullptr;
					}
				}
			}
			if(!refresh && vidsel_item.m && lbq.item_from_value(url).selected())
			{
				auto &m {*vidsel_item.m};
				for(int n {0}; n < m.size(); n++)
				{
					m.enabled(n, true);
					if(m.text(n) == "Select formats" && !bottom.btnfmt_visible())
						m.erase(n--);
				}
				api::refresh_window(m.handle());
				vidsel_item.m = nullptr;
			}
			active_threads--;
			if(bottom.working_info && bottom.info_thread.joinable())
				bottom.info_thread.detach();
		});
	}
}


void GUI::make_form()
{
	using widgets::theme;
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

	if(API::screen_dpi(true) > 96)
		btn_settings.image(arr_config22_ico, sizeof arr_config22_ico);
	else btn_settings.image(arr_config16_ico, sizeof arr_config16_ico);
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
			if(item == lbq.at(0).end())
			{
				if(text.size() > 300)
					text.erase(0, text.size() - 300);
				qurl = text;
				l_url.update_caption();
			}
			else
			{
				qurl = text + L" (queue item #" + to_wstring(item.text(0)) + L")";
				l_url.update_caption();
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
			while(std::getline(ss, line))
			{
				if(line.back() == '\r')
					line.pop_back();
				if(!line.empty()) add_url(line);
			}
		}
		else if(!text.empty())
		{
			if(text.starts_with(LR"(https://www.youtube.com/watch?v=)"))
				if(text.find(L"&list=") == 43)
					text.erase(43);
			auto item {lbq.item_from_value(text)};
			if(item == lbq.at(0).end())
			{
				l_url.update_caption();
				add_url(text);
			}
			else l_url.caption("The URL in the clipboard is already added (queue item #" + item.text(0) + ").");

			if(theme::is_dark())
				l_url.fgcolor(theme::path_link_fg);
			else l_url.fgcolor(theme::path_link_fg);
		}
	});

	btn_qact.events().click([this]
	{
		auto url {pop_queue_menu(btn_qact.pos().x, btn_qact.pos().y + btn_qact.size().height)};
		if(!url.empty())
		{
			lbq.auto_draw(false);
			remove_queue_item(url);
			lbq.auto_draw(true);
		}
	});

	plc.collocate();
}


bool GUI::apply_theme(bool dark)
{
	using widgets::theme;

	if(dark) theme::make_dark();
	else theme::make_light();

	const auto &text {outbox.current_buffer()};
	if(!text.empty())
	{
		auto ca {outbox.colored_area_access()};
		ca->clear();
		auto p {ca->get(0)};
		if(text.starts_with("[GUI]"))
		{
			p->count = 1;
			p->fgcolor = theme::is_dark() ? theme::path_link_fg : nana::color {"#569"};
		}
		if(text.rfind("[GUI] ") != 0)
		{
			p = ca->get(outbox.text_line_count() - 2);
			p->count = 1;
			if(text.rfind("WM_CLOSE") == -1)
				p->fgcolor = widgets::theme::is_dark() ? widgets::theme::path_link_fg : nana::color {"#569"};
			else p->fgcolor = widgets::theme::is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
		}
		nana::API::refresh_window(outbox);
	}

	bgcolor(theme::fmbg);
	refresh_widgets();

	for(auto &bot : bottoms)
		bot.second->bgcolor(theme::fmbg);

	return false; // don't refresh widgets a second time, already done once in this function
}


void GUI::show_queue(bool freeze_redraw)
{
	if(freeze_redraw)
		SendMessageA(hwnd, WM_SETREDRAW, FALSE, 0);
	const auto px {nana::API::screen_dpi(true) >= 144};
	change_field_attr(get_place(), "Bottom", "weight", 298 - 240 * bottoms.current().expcol.collapsed() - px);
	for(auto &bot : bottoms)
	{
		auto &plc {bot.second->plc};
		bot.second->btnq.caption("Show output");
		plc.field_display("prog", false);
		plc.field_display("separator", true);
		plc.collocate();
	}
	outbox.highlight(false);
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
	change_field_attr(get_place(), "Bottom", "weight", 325 - 240 * bottoms.current().expcol.collapsed());
	for(auto &bot : bottoms)
		bot.second->btnq.caption("Show queue");
	auto &curbot {bottoms.current()};
	auto &plc {curbot.plc};
	plc.field_display("prog", true);
	plc.field_display("separator", false);
	plc.collocate();
	queue_panel.hide();
	outbox.show(curbot.url);
	outbox.highlight(conf.kwhilite);
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
			try { releases = json::parse(jtext); }
			catch(nlohmann::detail::exception e)
			{
				releases.clear();
				nana::msgbox mbox {*this, "ytdlp-interface JSON error"};
				mbox.icon(nana::msgbox::icon_error);
				std::string str;
				if(jtext.size() > 600)
				{
					jtext.erase(600);
					str = " (showing the first 600 characters)";
				}
				(mbox << "Got an unexpected response when checking GitHub for a new version" + str + ":\n\n" << jtext
					<< "\n\nError from the JSON parser:\n\n" << e.what())();
				thr_releases.detach();
				return;
			}
			if(releases.is_array())
			{
				if(releases[0].contains("tag_name"))
				{
					std::string tag_name {releases[0]["tag_name"]};
					if(tag_name.size() < 4)
					{
						nana::msgbox mbox {*this, "ytdlp-interface error"};
						mbox.icon(nana::msgbox::icon_error);
						(mbox << "Got an unexpected response when checking GitHub for a new version. "
							<< "The response is valid JSON and is properly formatted, but the value of the key \"tag_name\" "
							<< "for the latest release is less that 4 characters in length: \"" << tag_name << "\"")();
						releases.clear();
					}
					else if(is_tag_a_new_version(tag_name) && conf.get_releases_at_startup)
						caption(title + "   (" + tag_name + " is available)");
				}
				else
				{
					nana::msgbox mbox {*this, "ytdlp-interface error"};
					mbox.icon(nana::msgbox::icon_error);
					std::string str;
					if(jtext.size() > 600)
					{
						jtext.erase(600);
						str = " (showing the first 600 characters)";
					}
					(mbox << "Got an unexpected response when checking GitHub for a new version" + str + ":\n\n" << jtext
						<< "\n\nThe response is valid JSON and is formatted as an array (as expected), but the first element "
						<< "does not contain the key \"tag_name\".")();
					releases.clear();
				}
			}
			else
			{
				nana::msgbox mbox {*this, "ytdlp-interface error"};
				mbox.icon(nana::msgbox::icon_error);
				std::string str;
				if(jtext.size() > 600)
				{
					jtext.erase(600);
					str = " (showing the first 600 characters)";
				}
				(mbox << "Got an unexpected response when checking GitHub for a new version" + str +":\n\n" << jtext
					<< "\n\nThe response is valid JSON, but is not formatted as an array.")();
				releases.clear();
			}
		}
		thr_releases.detach();
	}};
}


void GUI::get_latest_ffmpeg()
{
	using json = nlohmann::json;

	thr_releases_ffmpeg = std::thread {[this]
	{
		auto jtext {util::get_inet_res("https://api.github.com/repos/yt-dlp/FFmpeg-Builds/releases", &inet_error)};
		if(!jtext.empty())
		{
			json json_ffmpeg;
			try { json_ffmpeg = json::parse(jtext); }
			catch(nlohmann::detail::exception e)
			{
				nana::msgbox mbox {*this, "ytdlp-interface JSON error"};
				mbox.icon(nana::msgbox::icon_error);
				if(jtext.size() > 700)
					jtext.erase(700);
				(mbox << "Got an unexpected response when checking GitHub for a new FFmpeg version:\n\n" << jtext
					<< "\n\nError from the JSON parser:\n\n" << e.what())();
				if(thr_releases_ffmpeg.joinable())
					thr_releases_ffmpeg.detach();
				return;
			}
			if(!json_ffmpeg.empty())
			{
				for(auto &el : json_ffmpeg[0]["assets"])
				{
					std::string url {el["browser_download_url"]};
					if(url.find(X64 ? "win64-gpl.zip" : "win32-gpl.zip") != -1)
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
		}
		if(thr_releases_ffmpeg.joinable())
			thr_releases_ffmpeg.detach();
	}};
}


void GUI::get_latest_ytdlp()
{
	using json = nlohmann::json;

	thr_releases_ytdlp = std::thread {[this]
	{
		std::string jtext;
		auto fname {conf.ytdlp_path.filename().string()};
		if(fname.empty())
			fname = ytdlp_fname;
		if(fname == "yt-dlp.exe" || fname == "yt-dlp_x86.exe")
		{
			if(conf.ytdlp_nightly)
				jtext = util::get_inet_res("https://api.github.com/repos/yt-dlp/yt-dlp-nightly-builds/releases/latest", &inet_error);
			else jtext = util::get_inet_res("https://api.github.com/repos/yt-dlp/yt-dlp/releases/latest", &inet_error);
		}
		else jtext = util::get_inet_res("https://api.github.com/repos/ytdl-patched/ytdl-patched/releases/latest", &inet_error);
		if(!jtext.empty() && thr_releases_ytdlp.joinable())
		{
			json json_ytdlp;
			try { json_ytdlp = json::parse(jtext); }
			catch(nlohmann::detail::exception e)
			{
				nana::msgbox mbox {*this, "ytdlp-interface JSON error"};
				mbox.icon(nana::msgbox::icon_error);
				if(jtext.size() > 700)
					jtext.erase(700);
				(mbox << "Got an unexpected response when checking GitHub for a new yt-dlp version:\n\n" << jtext
					<< "\n\nError from the JSON parser:\n\n" << e.what())();
				if(thr_releases_ytdlp.joinable())
					thr_releases_ytdlp.detach();
				return;
			}
			if(!json_ytdlp.empty() && thr_releases_ytdlp.joinable())
			{
				for(auto &el : json_ytdlp["assets"])
				{
					std::string url {el["browser_download_url"]};
					if(url.find(fname) != -1)
					{
						url_latest_ytdlp = url;
						size_latest_ytdlp = el["size"];
						url_latest_ytdlp_relnotes = json_ytdlp["html_url"];
						std::string date {json_ytdlp["published_at"]};
						ver_ytdlp_latest.year = stoi(date.substr(0, 4));
						ver_ytdlp_latest.month = stoi(date.substr(5, 2));
						ver_ytdlp_latest.day = stoi(date.substr(8, 2));
						break;
					}
				}
			}
		}
		if(thr_releases_ytdlp.joinable())
			thr_releases_ytdlp.detach();
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


void GUI::get_version_ytdlp()
{
	if(!conf.ytdlp_path.empty())
	{
		if(fs::exists(conf.ytdlp_path))
		{
			auto ver {util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L"\" --version")};
			if(ver.size() >= 10)
			{
				ver_ytdlp = {-1, -1, -1};
				auto rawver {ver.substr(0, 10)};
				try { ver_ytdlp.year = stoi(rawver.substr(0, 4)); }
				catch(...) { return; }
				ver_ytdlp.month = stoi(rawver.substr(5, 2));
				ver_ytdlp.day = stoi(rawver.substr(8, 2));
			}
		}
		else conf.ytdlp_path.clear();
	}
}


bool GUI::is_ytlink(std::wstring url)
{
	auto pos {url.find(L"www.")};
	if(pos != -1)
		url.erase(pos, 4);
	pos = url.find(L"m.youtube.");
	if(pos != -1)
		url.erase(pos, 2);
	if(url.starts_with(L"https://youtube.com/") && url.size() > 20 || url.starts_with(L"youtube.com/") && url.size() > 12)
		return true;
	if(url.starts_with(L"https://youtu.be/") && url.size() > 17 || url.starts_with(L"youtu.be/") && url.size() > 9)
		return true;
	return false;
}


bool GUI::is_ytchan(std::wstring url)
{
	if(url.find(L"youtube.com/c/") != -1 || url.find(L"youtube.com/@") != -1 ||
		url.find(L"youtube.com/channel/") != -1 || url.find(L"youtube.com/user/") != -1)
		return true;
	auto pos {url.find(L"youtube.com/")};
	if(pos != -1 && url.size() > pos + 12 && url.find(L"watch?v=") == -1 && url.find(L"list=") == -1)
		return true;
	return false;
}


void GUI::change_field_attr(nana::place &plc, std::string field, std::string attr, unsigned new_val)
{
	std::string divtext {plc.div()};
	auto pos {divtext.find(field)};
	if(pos != -1)
	{
		pos = divtext.find(attr + '=', pos);
		if(pos != -1)
		{
			pos += attr.size()+1;
			auto cut_pos {pos};
			while(isdigit(divtext[pos]))
				pos++;
			plc.div(divtext.substr(0, cut_pos) + std::to_string(new_val) + divtext.substr(pos));
			plc.collocate();
		}
	}
}


void GUI::taskbar_overall_progress()
{
	if(i_taskbar && lbq.at(0).size() > 1)
	{
		ULONGLONG completed {0}, total {lbq.at(0).size()};
		for(auto &item : lbq.at(0))
			if(item.text(3) == "done")
				completed++;
		if(completed)
			if(completed == total)
				i_taskbar->SetProgressState(hwnd, TBPF_NOPROGRESS);
			else i_taskbar->SetProgressValue(hwnd, completed, total);
		else i_taskbar->SetProgressValue(hwnd, 1, -1);
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
			std::wstring next_url {next_item.value<lbqval_t>()};
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
					bottoms.at(it.value<lbqval_t>()).gpopt.caption("Download options for queue item #" + stridx);
			}

		auto prev_idx {std::stoi(item.text(0)) - 1};
		auto next_item {lbq.erase(item)};
		nana::api::refresh_window(lbq);
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
				const auto &next_url {next_item.value<lbqval_t>().url};
				auto text {next_item.text(3)};
				if(text == "queued" || (text.find("stopped") != -1 && text.find("error") != -1))
					return next_url;
			}
		}
	}
	return L"";
}


bool GUI::lbq_has_scrollbar()
{
	nana::paint::graphics g {{100, 100}};
	g.typeface(lbq.typeface());
	const auto text_height {g.text_extent_size("mm").height};
	const auto item_height {text_height + lbq.scheme().item_height_ex};
	if(lbq.at(0).size() * item_height > lbq.size().height - 25)
		return true;
	return false;
}


void GUI::adjust_lbq_headers()
{
	using namespace util;

	auto zero {scale(30)},
		one {scale(20 + !conf.col_site_text * 10) * conf.col_site_icon + scale(110) * conf.col_site_text},
		three {scale(116)},
		four {scale(130) * lbq.column_at(4).visible() + scale(16) * lbq_has_scrollbar()},
		five {scale(150) * lbq.column_at(5).visible()},
		six {scale(60) * lbq.column_at(6).visible()},
		seven {scale(100) * lbq.column_at(7).visible()};

	if(conf.col_site_icon || conf.col_site_text)
	{
		lbq.column_at(1).width(one);
		lbq.column_at(1).text(conf.col_site_text ? "Website" : "");
		if(!lbq.column_at(1).visible())
			lbq.column_at(1).visible(true);
	}
	else lbq.column_at(1).visible(false);

	const auto two_computed {lbq.size().width - (zero + one + three + four + five + six + seven) - scale(9)};
	lbq.column_at(2).width(two_computed);
}


HRESULT STDMETHODCALLTYPE GUI::DragEnter(IDataObject *pDataObj, DWORD, POINTL, DWORD*)
{
	FORMATETC fmtetc {CF_TEXT, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM stgmedium;
	HRESULT hr {pDataObj->GetData(&fmtetc, &stgmedium)};
	if(FAILED(hr)) return hr;

	auto ptext {reinterpret_cast<const char *>(GlobalLock(stgmedium.hGlobal))};
	if(ptext)
	{
		std::string text {ptext};
		if(text.starts_with("http://") || text.starts_with("https://"))
		{
			drop_cliptext_temp = util::get_clipboard_text();
			util::set_clipboard_text(hwnd, nana::to_wstring(text));
			l_url.events().mouse_enter.emit({}, l_url);
			ReleaseStgMedium(&stgmedium);
			return S_OK;
		}
	}
	ReleaseStgMedium(&stgmedium);
	return S_FALSE;
}


HRESULT STDMETHODCALLTYPE GUI::DragLeave()
{
	l_url.events().mouse_leave.emit({}, l_url);
	util::set_clipboard_text(hwnd, drop_cliptext_temp);
	return S_OK;
}


HRESULT STDMETHODCALLTYPE GUI::Drop(IDataObject *pDataObj, DWORD, POINTL, DWORD*)
{
	l_url.events().mouse_up.emit({}, l_url);
	util::set_clipboard_text(hwnd, drop_cliptext_temp);
	return S_OK;
}