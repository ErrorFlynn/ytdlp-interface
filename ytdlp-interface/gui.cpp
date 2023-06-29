#include "gui.hpp"
#include "icons.hpp"

#include <regex>
#include <algorithm>
#include <nana/gui/filebox.hpp>
#include <nana/system/platform.hpp>


GUI::GUI() : themed_form {std::bind(&GUI::apply_theme, this, std::placeholders::_1)}
{
	using namespace nana;

	if(WM_TASKBAR_BUTTON_CREATED = RegisterWindowMessageW(L"TaskbarButtonCreated-d7f3e5e9-c8a4-49b1-b55a-87c5d9b5d2f6"))
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

	msg.make_after(WM_SYSCOMMAND, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == 1337)
		{
			restore();
			bring_top(true);
			center(1000, 703 - bottoms.current().expcol.collapsed() * 240);
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
		if(wparam == 'S')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
			{
				dlg_settings();
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
				auto fwnd {api::focus_window()};
				auto &bottom {bottoms.current()};
				if(fwnd != bottom.com_args && fwnd != bottom.tbrate && !lbq.selected().empty())
				{
					outbox.clear(bottom.url);
					remove_queue_item(bottom.url);
				}
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

	msg.make_before(WM_GETMINMAXINFO, [&](UINT, WPARAM, LPARAM lparam, LRESULT *)
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

	::widgets::theme.contrast(conf.contrast);

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
	make_form();

	if(system_theme())
		apply_theme(is_system_theme_dark());
	else apply_theme(dark_theme());

	get_versions();
	
	events().unload([&]
	{
		conf.zoomed = is_zoomed(true);
		if(conf.zoomed || is_zoomed(false)) restore();
		conf.winrect = nana::rectangle {pos(), size()};
		working = menu_working = false;
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


void GUI::dlg_formats()
{
	using namespace nana;
	using ::widgets::theme;
	auto url {bottoms.visible()};
	auto &bottom {bottoms.at(url)};
	auto &vidinfo {bottom.vidinfo};

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.caption(title + " - manual selection of formats");
	fm.bgcolor(theme.fmbg);
	fm.div(R"(
			vert margin=20
				<weight=180px 
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
				<weight=20> <cb_streams weight=25>
				<weight=20> <list> <weight=20>
				<weight=35 <><btncancel weight=420> <weight=20> <btnok weight=420><>>
		)");

	::widgets::Title l_title {fm};
	::widgets::Label l_dur {fm, "Duration:", true}, l_chap {fm, "Chapters:", true},
		l_upl {fm, "Uploader:", true}, l_date {fm, "Upload date:", true};
	::widgets::Text l_durtext {fm, "", true}, l_chaptext {fm, "", true}, l_upltext {fm, "", true}, l_datetext {fm, "", true};
	nana::picture thumb {fm};
	::widgets::Separator sep1 {fm}, sep2 {fm};
	::widgets::cbox cb_streams {fm, "Select multiple audio formats to merge into .mkv file as audio tracks "
		"(this passes --audio-multistreams to yt-dlp)"};
	::widgets::Listbox list {fm, true};
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
	fm["cb_streams"] << cb_streams;
	fm["list"] << list;
	fm["btncancel"] << btncancel;
	fm["btnok"] << btnok;

	cb_streams.check(conf.audio_multistreams);

	list.sortable(false);
	list.checkable(true);
	list.hilight_checked(true);
	list.enable_single(true, false);
	list.scheme().text_margin = util::scale(10) + (nana::api::screen_dpi(true) > 96)*4;
	list.append_header("format", dpi_transform(280));
	list.append_header("acodec", dpi_transform(90));
	list.append_header("vcodec", dpi_transform(90));
	list.append_header("ext", dpi_transform(50));
	list.append_header("fps", dpi_transform(32));
	list.append_header("vbr", dpi_transform(40));
	list.append_header("abr", dpi_transform(40));
	list.append_header("asr", dpi_transform(50));
	list.append_header("filesize", dpi_transform(cnlang ? 170 : 160));

	list.events().selected([&](const arg_listbox &arg)
	{
		if(arg.item.selected())
			arg.item.check(!arg.item.checked()).select(false);
	});

	list.events().mouse_down([&](const arg_mouse &arg)
	{
		if(conf.audio_multistreams)
		{
			auto pos {list.cast({arg.pos.x, arg.pos.y})};
			if(pos.is_category() && list.at(pos.cat).text() == "Audio only")
			{
				list.auto_draw(false);
				for(auto ip : list.at(pos.cat))
					ip.check(!ip.checked());
				list.auto_draw(true);
			}
		}
	});

	list.events().checked([&](const arg_listbox &arg)
	{
		auto item {arg.item};
		auto pos {item.pos()};

		if(item.checked())
		{
			item.fgcolor(theme.list_check_highlight_fg);
			item.bgcolor(theme.list_check_highlight_bg);
			list.auto_draw(false);
			if(pos.cat == 0)
			{
				for(auto ip : list.at(0))
					if(ip != item)
						ip.check(false);
				if(list.size_categ() == 3)
					for(auto ip : list.at(2))
						ip.check(false);
				if(list.size_categ() > 1 && (!conf.audio_multistreams || list.at(1).text() == "Video only"))
					for(auto ip : list.at(1))
						ip.check(false);
			}
			else if(list.size_categ() > 1)
			{
				if(list.at(pos.cat).text() == "Video only")
				{
					for(auto ip : list.at(0))
						ip.check(false);
					for(auto ip : list.at(2))
						if(ip != item)
							ip.check(false);
				}
				else if(!conf.audio_multistreams)
				{
					for(auto ip : list.at(1))
						if(ip != item)
							ip.check(false);
					for(auto ip : list.at(0))
						if(ip != item)
							ip.check(false);
				}
			}
			list.auto_draw(true);
		}
		else 
		{
			item.fgcolor(list.fgcolor());
			item.bgcolor(list.bgcolor());
		}
		btnok.enable(!list.checked().empty());
	});

	cb_streams.events().checked([&]
	{
		conf.audio_multistreams = cb_streams.checked();
		if(!conf.audio_multistreams && list.size_categ() > 1 && list.at(1).text() == "Audio only")
		{
			list.auto_draw(false);
			int count {0};
			for(auto ip : list.at(0))
				if(ip.checked())
				{
					count++;
					break;
				}
			for(auto ip : list.at(1))
				if(ip.checked())
				{
					if(!count)
					{
						count++;
						continue;
					}
					ip.check(false);
				}
			list.auto_draw(true);
		}
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
		auto &strfmt {bottom.strfmt}, &fmt1 {bottom.fmt1}, &fmt2 {bottom.fmt2};
		fmt1.clear();
		fmt2.clear();

		size_t vidcat {0};
		if(list.size_categ() == 3)
			vidcat = 2;
		else if(list.size_categ() == 2 && list.at(1).text() == "Video only")
			vidcat = 1;
		
		bool mergeall {false};
		if(vidcat == 2)
		{
			size_t cnt {0};
			for(const auto ip : list.at(1))
				if(ip.checked())
					cnt++;
			if(list.at(1).size() == cnt)
			{
				fmt2 = L"mergeall";
				mergeall = true;
			}
		}

		auto sel {list.checked()};
		for(const auto &pos : sel)
		{
			auto &val {list.at(pos).value<std::wstring>()};
			if(pos.cat == vidcat || pos.cat == 0)
			{
				fmt1 = std::move(val);
				if(mergeall) break;
			}
			else if(!mergeall)
			{
				if(!fmt2.empty())
					fmt2 += '+';
				fmt2 += std::move(val);
			}
		}
		strfmt = fmt1;
		if(!fmt2.empty())
		{
			if(!strfmt.empty())
				strfmt += L'+';
			strfmt += fmt2;
		}

		conf.fmt1 = fmt1;
		conf.fmt2 = fmt2;

		bottom.use_strfmt = true;
		if(bottom.using_custom_fmt())
		{
			nana::msgbox mbox {fm, "Warning: conflicting -f arguments"};
			std::string text {"The \"Custom arguments\" checkbox is checked, and \"-f\" is present as a custom argument.\n\n"
				"If you don't uncheck that checkbox, the \"-f\" custom argument will override the selection you have made here."};
			mbox.icon(nana::msgbox::icon_warning);
			(mbox << text)();
		}
		fm.close();
	});

	btnok.enabled(false);
	thumb.transparent(true);

	std::string thumb_url, title {"[title missing]"};
	if(bottom.vidinfo_contains("title"))
		title = vidinfo["title"];
	if(!bottom.is_ytlink)
	{
		if(bottom.vidinfo_contains("thumbnail"))
			thumb_url = vidinfo["thumbnail"];
		thumb.stretchable(true);
		thumb.align(align::center, align_v::center);
	}
	else
	{
		auto it {std::find_if(vidinfo["thumbnails"].begin(), vidinfo["thumbnails"].end(), [](const auto &el)
		{
			return std::string{el["url"]}.rfind("mqdefault.jpg") != -1;
		})};

		if(it == vidinfo["thumbnails"].end())
			it = std::find_if(vidinfo["thumbnails"].begin(), vidinfo["thumbnails"].end(), [](const auto &el)
		{
			return std::string {el["url"]}.rfind("mqdefault_live.jpg") != -1;
		});

		if(it != vidinfo["thumbnails"].end())
			thumb_url = (*it)["url"];
	}

	l_title.caption(title);
	list.append({"Audio only", "Video only"});

	int dur {0};
	bool live {bottom.vidinfo_contains("is_live") && bottom.vidinfo["is_live"] ||
			bottom.vidinfo_contains("live_status") && bottom.vidinfo["live_status"] == "is_live"};
	if(!live && bottom.vidinfo_contains("duration"))
		dur = vidinfo["duration"];
	int hr {(dur/60)/60}, min {(dur/60)%60}, sec {dur%60};
	if(dur < 60) sec = dur;
	std::string durstr {live ? "live" : "---"};
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
			ss.width(2);
			ss.fill('0');
		}
		else durstr = std::to_string(min);
		ss << sec;
		durstr += ':' + ss.str();
	}
	else if(bottom.vidinfo_contains("duration_string"))
		durstr = vidinfo["duration_string"];
	l_durtext.caption(durstr);

	std::string strchap;
	if(bottom.vidinfo_contains("chapters"))
		strchap = std::to_string(vidinfo["chapters"].size());
	if(strchap.empty() || strchap == "0") 
		l_chaptext.caption("none");
	else l_chaptext.caption(strchap);

	if(bottom.vidinfo_contains("uploader"))
		l_upltext.caption(std::string {vidinfo["uploader"]});
	else l_upltext.caption("---");

	std::string strdate {"---"};
	if(bottom.vidinfo_contains("upload_date"))
	{
		strdate = vidinfo["upload_date"];
		strdate = strdate.substr(0, 4) + '-' + strdate.substr(4, 2) + '-' + strdate.substr(6, 2);
	}
	l_datetext.caption(strdate);

	if(!thumb_url.empty())
	{
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
	}

	std::string format_id1, format_id2;
	if(bottom.vidinfo_contains("format_id"))
	{
		std::string format_id {vidinfo["format_id"]};
		auto pos(format_id.find('+'));
		if(pos != -1)
		{
			format_id1 = format_id.substr(0, pos);
			format_id2 = format_id.substr(pos + 1);
		}
		else format_id1 = format_id;
	}

	auto get_int = [](const nlohmann::json &j, const std::string &key) -> std::string
	{
		if(j.contains(key))
		{
			if(j[key] == nullptr)
				return "null";
			else return std::to_string(j[key].get<unsigned>());
		}
		else return "---";
	};

	auto get_string = [](const nlohmann::json &j, const std::string &key) -> std::string
	{
		if(j.contains(key))
		{
			if(j[key] == nullptr)
				return "null";
			else return j[key].get<std::string>();
		}
		else return "---";
	};

	std::vector<bool> colmask(9, false);
	for(auto &fmt : vidinfo["formats"])
	{
		std::string format {fmt["format"]}, acodec, vcodec, ext, fps, vbr, abr, asr, filesize;
		if(format.find("storyboard") == -1)
		{
			abr = get_int(fmt, "abr");
			vbr = get_int(fmt, "vbr");
			asr = get_int(fmt, "asr");
			fps = get_int(fmt, "fps");
			ext = get_string(fmt, "ext");
			acodec = get_string(fmt, "acodec");
			vcodec = get_string(fmt, "vcodec");
			if(fmt.contains("filesize"))
			{
				if(fmt["filesize"] != nullptr)
					filesize = util::int_to_filesize(fmt["filesize"]);
				else filesize = "null";
			}
			else filesize = "---";
			unsigned catidx {0};
			if(acodec == "none")
				catidx = 2; // video only
			else if(vcodec == "none")
				catidx = 1; // audio only
			list.at(catidx).append({format, acodec, vcodec, ext, fps, vbr, abr, asr, filesize});
			auto idstr {to_wstring(fmt["format_id"].get<std::string>())};
			auto item {list.at(catidx).back()};
			item.value(idstr);
			if(idstr == conf.fmt1 || (conf.audio_multistreams ? conf.fmt2.find(idstr) != -1 : idstr == conf.fmt2))
				list.at(catidx).back().select(true);
			if(!format_id1.empty() && format_id1 == to_utf8(idstr))
				item.text(0, format + " *");
			if(!format_id2.empty() && format_id2 == to_utf8(idstr))
				item.text(0, format + " *");
			for(int n {1}; n < 9; n++)
			{
				const auto text {item.text(n)};
				if(!colmask[n] && text != "---" && text != "null" && text != "none")
					colmask[n] = true;
			}
		}
	}

	for(int n {1}; n < 9; n++)
		list.column_at(n).visible(colmask[n]);

	if(list.size_categ() == 3)
	{
		if(list.at(2).size() == 0)
			list.erase(2);
		if(list.at(1).size() == 0)
			list.erase(1);
	}

	list.refresh_theme();

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

	fm.center(1000, std::max(429.0 + list.item_count() * (cnlang ? 22.4 : 20.4), double(600)));

	fm.collocate();
	fm.show();
	fm.modality();
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
			else if(bottom.is_bcplaylist)
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

		if(tbpipe.current() == url)
			tbpipe.clear();

		bottom.dl_thread = std::thread([&, this, tempfile, display_cmd, cmd, url]
		{
			working = true;
			auto ca {tbpipe.colored_area_access()};
			if(outbox.current() == url)
				ca->clear();
			if(fs::exists(conf.ytdlp_path))
				tbpipe.append(url, L"[GUI] executing command line: " + display_cmd + L"\n\n");
			else tbpipe.append(url, L"ytdlp.exe not found: " + conf.ytdlp_path.wstring());
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
				while(text.find_last_of("\r\n") != -1)
					text.pop_back();
				auto item {lbq.item_from_value(url)};
				if(total != -1)
				{
					item.text(3, (std::stringstream {} << static_cast<double>(completed) / 10).str() + '%');
					if(i_taskbar && lbq.at(0).size() == 1)
						i_taskbar->SetProgressValue(hwnd, completed, total);
				}
				if(completed < 1000 && total != -1)
				{
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
							prog.caption(text);
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
				if(completed <= 1000 && (completed > prog.value() || completed == 0 || prog.value() - completed > 50))
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
			else if(bottom.is_ytplaylist)
			{
				bottom.printed_path = bottom.outpath / bottom.playlist_info["title"].get<std::string>();
			}
			else if(bottom.is_bcplaylist)
			{
				// todo
			}

			if(working)
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
		const auto fname {conf.ytdlp_path.filename().string()};
		if(graceful_exit)
			tbpipe.append(url, "\n[GUI] " + fname + " process was ended gracefully via Ctrl+C signal\n");
		else tbpipe.append(url, "\n[GUI] " + fname + " process was ended forcefully via WM_CLOSE message\n");
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
	using namespace nana;

	auto it {std::find_if(lbq.at(0).begin(), lbq.at(0).end(), [&, this](auto &el)
	{
		std::wstring val;
		try { val = el->value<lbqval_t>().url; }
		catch(std::runtime_error) { return false; }
		return val == url;
	})};

	if(it == lbq.at(0).end())
	{
		auto stridx {std::to_string(lbq.at(0).size() + 1)};
		lbq.at(0).append({stridx, "...", "...", "queued", "...", "...", "...", "..."});
		lbq.at(0).back().value(lbqval_t {url, nullptr});
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
					p->fgcolor = ::widgets::theme.is_dark() ? ::widgets::theme.path_link_fg : color {"#569"};
				}
				if(!queue_panel.visible() && overlay.visible())
				{
					outbox.widget::show();
					overlay.hide();
				}
			};

			if(bottom.is_ytlink || bottom.is_bcplaylist)
			{
				if(bottom.is_ytplaylist || bottom.is_bcplaylist)
				{
					std::wstring compat_options {bottom.is_ytplaylist ? L" --compat-options no-youtube-unavailable-videos" : L""},
					             cmd {L" --no-warnings --flat-playlist -J" + compat_options + L" \"" + url + L'\"'};
					if(conf.cb_proxy && !conf.proxy.empty())
						cmd = L" --proxy " + conf.proxy + cmd;
					bottom.cmdinfo = conf.ytdlp_path.filename().wstring() + cmd;
					media_info = util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info);
					if(media_info.front() == '{')
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
							std::string URL {bottom.playlist_info["entries"][0]["url"]};
							cmd = L" --no-warnings -j " + fmt_sort + to_wstring(URL);
							media_info = {util::run_piped_process(L'\"' + conf.ytdlp_path.wstring() + L'\"' + cmd, &bottom.working_info)};
							if(media_info.front() == '{')
							{
								try { bottom.vidinfo = nlohmann::json::parse(media_info); }
								catch(nlohmann::detail::exception e)
								{
									bottom.vidinfo.clear();
									if(lbq.item_from_value(url) != lbq.at(0).end())
										json_error(e);
								}
								if(!bottom.vidinfo.empty())
								{
									bottom.show_btnfmt(true);
									if(bottom.is_bcplaylist)
									{
										if(bottom.playlist_info["entries"].size() < 2)
											bottom.is_bcplaylist = false;
									}
								}
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
								break;
							}
					lbq.item_from_value(url).text(1, bottom.is_bcchan ? "bandcamp.com" : "youtube.com");
					lbq.item_from_value(url).text(2, tab + std::string {bottom.playlist_info["title"]});
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
							media_title = "[playlist] " + std::string {bottom.playlist_info["title"]};
							if(vidsel_item.m && lbq.item_from_value(url).selected())
							{
								auto &m {*vidsel_item.m};
								auto pos {vidsel_item.pos};
								auto str {std::to_string(playlist_size)};
								if(bottom.playsel_string.empty())
									m.text(pos, (bottom.is_ytplaylist ? "Select videos (" : "Select songs (") + str + '/' + str + ")");
								else m.text(pos, (bottom.is_ytplaylist ? "Select videos (" : "Select songs (") + std::to_string(bottom.playlist_selected()) + '/' + str + ")");
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
							filesize = '~' + util::int_to_filesize(fsize, false);
						}

						if(bottom.vidinfo_contains("formats"))
							bottom.show_btnfmt(true);
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
						p->fgcolor = ::widgets::theme.is_dark() ? ::widgets::theme.path_link_fg : color {"#569"};
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
			if(bottom.working_info && bottom.info_thread.joinable())
				bottom.info_thread.detach();
		});
	}
}


void GUI::dlg_playlist()
{
	using widgets::theme;
	using std::string;
	using namespace nana;

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.caption(title);
	fm.div(R"(vert margin=20 
		<weight=25 <l_title> <btnall weight=100> <weight=20> <btnnone weight=120>> <weight=20> 
		<range_row weight=25 <l_first weight=36> <weight=10> <tbfirst weight=45> <weight=10> <slfirst> <weight=20> 
			<l_last weight=36> <weight=10> <tblast weight=45> <weight=10> <sllast> <weight=20> <btnrange weight=120>>
		<range_row_spacer weight=20> <lbv> <weight=20> 
		<weight=35 <> <btnclose weight=100> <>>
	)");

	auto &bottom {bottoms.current()};
	::widgets::Title l_title {fm, bottom.is_bcplaylist ? "Select which songs to download from the album" : 
		"Select which videos to download from the playlist"};
	::widgets::Button btnall {fm, "Select all", true}, btnnone {fm, "Select none", true}, btnclose {fm, "Close"}, 
		btnrange {fm, "Select range", true};
	::widgets::Listbox lbv {fm, true};
	::widgets::Label l_first {fm, "First:"}, l_last {fm, "Last:"};
	::widgets::Textbox tbfirst {fm}, tblast {fm};
	::widgets::Slider slfirst {fm}, sllast {fm};

	fm["l_title"] << l_title;
	fm["btnall"] << btnall;
	fm["btnnone"] << btnnone;
	fm["lbv"] << lbv;
	fm["l_first"] << l_first;
	fm["l_last"] << l_last;
	fm["tbfirst"] << tbfirst;
	fm["tblast"] << tblast;
	fm["slfirst"] << slfirst;
	fm["sllast"] << sllast;
	fm["btnrange"] << btnrange;
	fm["btnclose"] << btnclose;

	int min {1}, max {static_cast<int>(bottom.playlist_selection.size())};

	slfirst.maximum(max-1);
	slfirst.vernier([&](unsigned, unsigned val) { return std::to_string(val < sllast.value() ? val+1 : sllast.value()); });
	sllast.maximum(max-1);
	sllast.vernier([&](unsigned, unsigned val) { return std::to_string(val > slfirst.value() ? val+1 : slfirst.value() + 2); });
	slfirst.events().value_changed([&]
	{
		if(slfirst.value() >= sllast.value())
			slfirst.value(sllast.value() - 1);
		else tbfirst.caption(std::to_string(slfirst.value() + 1));
	});
	sllast.events().value_changed([&]
	{
		if(sllast.value() <= slfirst.value())
			sllast.value(slfirst.value() + 1);
		else tblast.caption(std::to_string(sllast.value() + 1));
	});
	slfirst.events().click([&] { tbfirst.caption(std::to_string(slfirst.value() + 1)); });
	sllast.events().click([&] { tblast.caption(std::to_string(sllast.value() + 1)); });
	sllast.value(sllast.maximum());

	l_title.typeface(paint::font_info {"Arial", 14, {800}});
	l_title.text_align(align::left, align_v::center);
	tbfirst.caption("1");
	tbfirst.text_align(align::center);
	tbfirst.typeface(nana::paint::font_info {"", 11});
	tblast.caption(std::to_string(max));
	tblast.text_align(align::center);
	tblast.typeface(nana::paint::font_info {"", 11});


	lbv.sortable(false);
	lbv.checkable(true);
	lbv.enable_single(true, false);
	lbv.typeface(paint::font_info {"Calibri", 12});
	lbv.scheme().item_height_ex = 8;
	lbv.append_header("", dpi_transform(25));
	lbv.append_header("#", dpi_transform(33));
	lbv.append_header(bottom.is_bcplaylist ? "Song title" : "Video title", dpi_transform(theme.is_dark() ? 740 : 735));
	lbv.append_header("Duration", dpi_transform(75));
	lbv.column_movable(false);
	lbv.column_resizable(false);

	int cnt {0}, idx {1};
	for(auto entry : bottom.playlist_info["entries"])
	{
		int dur {entry["duration"] == nullptr ? 0 : int{entry["duration"]}};
		int hr {(dur / 60) / 60}, min {(dur / 60) % 60}, sec {dur % 60};
		if(dur < 60) sec = dur;
		std::string durstr {"---"};
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
				ss.width(2);
				ss.fill('0');
			}
			else durstr = std::to_string(min);
			ss << sec;
			durstr += ':' + ss.str();
		}

		const string title {entry["title"]};
		lbv.at(0).append({"", std::to_string(idx), title, durstr});
		if(!dur && !bottom.is_bcplaylist)
			bottom.playlist_selection[idx - 1] = false;
		else lbv.at(0).back().check(bottom.playlist_selection[idx - 1]);
		idx++;
	}

	btnclose.events().click([&] {fm.close(); });

	lbv.events().selected([&](const arg_listbox &arg)
	{
		if(arg.item.selected())
			arg.item.check(!arg.item.checked()).select(false);
	});

	lbv.events().checked([&](const arg_listbox &arg)
	{
		bottom.playlist_selection[arg.item.to_display().item] = arg.item.checked();
	});

	btnall.events().click([&]
	{
		for(auto item : lbv.at(0))
			item.check(true);
	});

	btnnone.events().click([&]
	{
		for(auto item : lbv.at(0))
			item.check(false);
	});

	btnrange.events().click([&]
	{
		const auto a {slfirst.value()}, b {sllast.value()};
		const auto cat {lbv.at(0)};
		const auto catsize {cat.size()};
		for(auto n {0}; n < a; n++)
			cat.at(n).check(false);
		for(auto n {a}; n <= b; n++)
			cat.at(n).check(true);
		for(auto n {b+1}; n < catsize; n++)
			cat.at(n).check(false);
	});

	tbfirst.set_accept([&](char c)
	{
		auto sel {tbfirst.selection()};
		return c == keyboard::backspace || c == keyboard::del || (isdigit(c) && 
			(sel.second.x || tbfirst.text().size() < std::to_string(max).size()));
	});

	tbfirst.events().focus([&](const arg_focus &arg)
	{
		if(!arg.getting)
		{
			if(tbfirst.text().empty() || tbfirst.to_int() >= sllast.value() + 1 || tbfirst.text() == "0")
				tbfirst.caption(std::to_string(slfirst.value()+1));
		}
	});

	tbfirst.events().text_changed([&, this]
	{
		if(tbfirst == api::focus_window())
		{
			auto text {tbfirst.text()};
			if(!text.empty() && text[0] == '0')
			{
				text.erase(0, 1);
				if(!text.empty())
				{
					tbfirst.caption(text);
					return;
				}
			}
			if(!tbfirst.text().empty() && tbfirst.to_int() < sllast.value() + 1 && tbfirst.text() != "0")
				slfirst.value(stoi(tbfirst.caption())-1);
		}
	});

	tblast.set_accept([&](char c)
	{
		auto sel {tblast.selection()};
		return c == keyboard::backspace || c == keyboard::del || (isdigit(c) && 
			(sel.second.x || tblast.text().size() < std::to_string(max).size()));
	});

	tblast.events().focus([&](const arg_focus &arg)
	{
		if(!arg.getting)
		{
			if(tblast.text().empty() || tblast.to_int() <= slfirst.value() + 1 || tblast.to_int() > max+1)
				tblast.caption(std::to_string(sllast.value()+1));
		}
	});

	tblast.events().text_changed([&, this]
	{
		if(tblast == api::focus_window())
		{
			auto text {tblast.text()};
			if(!text.empty() && text[0] == '0')
			{
				text.erase(0, 1);
				if(!text.empty())
				{
					tblast.caption(text);
					return;
				}
			}
			if(!tblast.text().empty() && tblast.to_int() > slfirst.value() + 1 && tblast.to_int() < max+1)
				sllast.value(stoi(tblast.caption())-1);
		}
	});

	fm.events().unload([&, this]
	{
		const auto sel {bottom.playlist_selected()};
		if(0 < sel && sel < bottom.playlist_selection.size())
		{
			std::wstring str;
			bool stopped {true};
			const auto &playsel {bottom.playlist_selection};
			const auto size {playsel.size()};
			for(int n {0}, b {0}, e {0}; n < size; n++)
			{
				const auto val {playsel[n]};
				if(val)
				{
					if(stopped)
					{
						stopped = false;
						b = n;
					}
					if(n == size - 1)
					{
						e = n;
						stopped = true;
					}
				}
				else if(!stopped)
				{
					stopped = true;
					e = n - 1;
				}

				if(stopped && b != -1)
				{
					if(e == b)
					{
						if(playsel[b])
							str += std::to_wstring(b + 1) + L',';
					}
					else str += std::to_wstring(b + 1) + L':' + std::to_wstring(e + 1) + L',';
					b = e = -1;
				}
			}
			bottom.playsel_string = str.substr(0, str.size() - 1);
		}
		else 
		{
			bottom.playlist_selection.assign(bottom.playlist_info["playlist_count"], true);
			bottom.playsel_string.clear();
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

	auto range_row_visible {bottom.playlist_selection.size() >= 15};
	if(!range_row_visible)
	{
		fm.get_place().field_display("range_row", false);
		fm.get_place().field_display("range_row_spacer", false);
	}

	nana::paint::graphics g {{100, 100}};
	g.typeface(lbv.typeface());
	const auto text_height {g.text_extent_size("mm").height};
	const auto item_height {text_height + lbv.scheme().item_height_ex};
	auto overflow {fm.center(950, std::max(211.0 - !range_row_visible*45 + lbv.item_count() * item_height, 0.0))};

	if(!overflow)
		lbv.column_at(2).width(lbv.column_at(2).width() + dpi_transform(16));

	fm.show();
	fm.modality();
}


void GUI::dlg_sections()
{
	using widgets::theme;
	using std::string;
	using namespace nana;

	auto &bottom {bottoms.current()};
	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.caption(title + " - media sections");
	if(cnlang) fm.center(800, 678);
	else fm.center(788, 678);
	if(cnlang) fm.div(R"(vert margin=[15,20,20,20] 
		<weight=130 <l_help>> <weight=20> 
		<weight=25
			<l_start weight=154> <weight=10> <tbfirst weight=80> <weight=10> <l_end weight=16> <weight=10> <tbsecond weight=80>
			<weight=20> <btnadd weight=100> <weight=20> <btnremove weight=150> <weight=20> <btnclear weight=90>
		>
		<weight=20> <lbs> <weight=20> 
		<weight=35 <> <btnclose weight=100> <>>
	)");
	else fm.div(R"(vert margin=[15,20,20,20] 
		<weight=130 <l_help>> <weight=20> 
		<weight=25
			<l_start weight=142> <weight=10> <tbfirst weight=80> <weight=10> <l_end weight=16> <weight=10> <tbsecond weight=80>
			<weight=20> <btnadd weight=100> <weight=20> <btnremove weight=150> <weight=20> <btnclear weight=90>
		>
		<weight=20> <lbs> <weight=20> 
		<weight=35 <> <btnclose weight=100> <>>
	)");

	class tbox : public ::widgets::Textbox
	{
	public:
		tbox(nana::window parent) : Textbox(parent)
		{
			multi_lines(false);
			text_align(align::center);
			typeface(nana::paint::font_info {"", 11});
			caption("0");
			set_accept([this](char c)
			{
				return c == keyboard::backspace || c == keyboard::del || (text().size() < 9 && (isdigit(c) || c == ':'));
			});
			events().focus([this](const arg_focus &arg)
			{
				if(arg.getting)
					select(true);
				else select(false);
			});
		}
	};

	::widgets::Text l_start {fm, "Define section from"}, l_end {fm, "to"};
	::widgets::Label l_help {fm, ""};
	::widgets::Button btnadd {fm, "Add to list", true}, btnremove {fm, "Remove from list", true}, btnclear {fm, "Clear list", true},
		btnclose {fm, "Close"};
	::widgets::Listbox lbs {fm, true};
	tbox tbfirst {fm}, tbsecond {fm};

	fm["l_help"] << l_help;
	fm["l_start"] << l_start;
	fm["l_end"] << l_end;
	fm["tbfirst"] << tbfirst;
	fm["tbsecond"] << tbsecond;
	fm["lbs"] << lbs;
	fm["btnclose"] << btnclose;
	fm["btnadd"] << btnadd;
	fm["btnremove"] << btnremove;
	fm["btnclear"] << btnclear;

	lbs.sortable(false);
	lbs.show_header(false);
	lbs.enable_single(true, false);
	lbs.typeface(paint::font_info {"Calibri", 12});
	lbs.scheme().item_height_ex = 8;
	tbfirst.focus();

	l_help.typeface(paint::font_info {"Tahoma", 10});
	l_help.text_align(align::left, align_v::top);
	l_help.format(true);
	std::string helptext {"Here you can tell yt-dlp to download only a section (or multiple sections) of the media. "
		"Define a section by specifying a start point and an end point, then add it to the list. The time points are "
		"in the format <bold color=0x>[hour:][minute:]second</>.\n\nFor example, <bold color=0x>54</> means second 54, "
		"<bold color=0x>9:54</> means minute 9 second 54, "
		"and <bold color=0x>1:9:54</> means hour 1 minute 9 second 54.\nTo indicate the end of the media, put <bold color=0x>0</> "
		"in the \"to\" field or leave it blank. \n\nWARNING: your input is not validated in any way, what you "
		"enter is what is passed to yt-dlp. The downloading of sections is done through FFmpeg (which is significantly slower), "
		"and each section is downloaded to its own file."};

	for(auto &val : bottom.sections)
	{
		const auto text {to_utf8(val.first.empty() ? L"0" : val.first) + " -> " +
			(val.second.empty() || val.second == L"0" ? "end" : to_utf8(val.second))};
		lbs.at(0).push_back(text);
		lbs.at(0).back().value(val);
	}

	lbs.events().selected([&](const arg_listbox &arg)
	{
		if(arg.item.selected())
		{
			try
			{
				auto val {arg.item.value<std::pair<std::wstring, std::wstring>>()};
				tbfirst.caption(val.first);
				tbsecond.caption(val.second);
			}
			catch(...) {}
		}
	});

	if(lbs.at(0).size())
		lbs.at(0).at(0).select(true);

	tbsecond.events().key_press([&](const arg_keyboard &arg)
	{
		if(arg.key == keyboard::enter)
			btnadd.events().click.emit({}, btnadd);
	});

	tbfirst.events().key_press([&](const arg_keyboard &arg)
	{
		if(arg.key == keyboard::enter)
			tbsecond.focus();
	});

	btnclose.events().click([&] { fm.close(); });
	btnclear.events().click([&] { lbs.clear(); tbfirst.caption(""); tbsecond.caption(""); });

	btnremove.events().click([&]
	{
		auto sel {lbs.selected()};
		if(!sel.empty())
			lbs.erase(lbs.at(sel.front()));
		tbfirst.caption("");
		tbsecond.caption("");
		tbfirst.focus();
	});

	btnadd.events().click([&]
	{
		auto first {tbfirst.text()}, second {tbsecond.text()};
		const auto text {(first.empty() ? "0" : first) + " -> " + (second.empty() || second == "0" ? "end" : second)};
		for(auto item : lbs.at(0))
			if(item.text(0) == text)
			{
				item.select(true);
				return;
			}
		lbs.at(0).append(text).select(true);
		lbs.at(0).back().value(std::make_pair<std::wstring, std::wstring>(to_wstring(first), to_wstring(second)));
		tbfirst.focus();
	});

	fm.events().unload([&]
	{
		bottom.sections.clear();
		for(auto item : lbs.at(0))
			bottom.sections.push_back(item.value<std::pair<std::wstring, std::wstring>>());
	});

	fm.theme_callback([&, this](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme.fmbg);
		l_help.caption(std::regex_replace(helptext, std::regex {"\\b(0x)"}, theme.is_dark() ? "0xf5c040" : "0x4C4C9C"));
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.collocate();
	lbs.append_header("", lbs.size().width - fm.dpi_transform(25));
	fm.show();
	fm.modality();
}


std::wstring GUI::pop_queue_menu(int x, int y)
{
	using namespace nana;

	std::wstring url_of_item_to_delete;
	::widgets::Menu m;
	m.item_pixels(dpi_transform(24));
	auto sel {lbq.selected()};
	if(!sel.empty() && !thr_menu.joinable())
	{
		auto item {lbq.at(sel.back())};
		auto item_name {"item #" + item.text(0)};
		auto url {item.value<lbqval_t>().url};
		auto &bottom {bottoms.current()};
		const auto is_live {bottom.vidinfo_contains("is_live") && bottom.vidinfo["is_live"] ||
						   bottom.vidinfo_contains("live_status") && bottom.vidinfo["live_status"] == "is_live"};
		static std::vector<drawerbase::listbox::item_proxy> stoppable, startable;
		static std::vector<std::wstring> completed;
		stoppable.clear();
		startable.clear();
		completed.clear();
		for(auto &item : lbq.at(0))
		{
			const auto text {item.text(3)};
			if(text == "done")
				completed.push_back(item.value<lbqval_t>().url);
			else if(text.find("stopped") == -1 && text.find("queued") == -1 && text.find("error") == -1)
				stoppable.push_back(item);
			else startable.push_back(item);
		}

		auto verb {bottom.btndl.caption().substr(0, 5)};
		if(verb.back() == ' ')
			verb.pop_back();
		if(item.text(3).find("stopped") != -1)
			verb = "Resume";
		m.append(verb + " " + item_name, [&, url, this](menu::item_proxy)
		{
			on_btn_dl(url);
			api::refresh_window(bottom.btndl);
		});
		m.append("Remove " + item_name, [&, url, this](menu::item_proxy)
		{
			url_of_item_to_delete = url;
		});

		fs::path file;
		if(!bottom.is_ytplaylist && !bottom.is_bcplaylist)
		{
			file = bottom.file_path();
			if(!file.empty())
			{
				if(fs::exists(file))
					m.append_splitter();
				else file.clear();
			}
		}

		m.append("Open folder of " + item_name, [&, file, this](menu::item_proxy)
		{
			if(!file.empty())
			{
				auto comres {CoInitialize(0)};
				if(comres == S_OK || comres == S_FALSE)
				{
					auto pidlist {ILCreateFromPathW(file.wstring().data())};
					SHOpenFolderAndSelectItems(pidlist, 0, 0, 0);
					ILFree(pidlist);
					CoUninitialize();
				}
				else ShellExecuteW(NULL, L"open", bottom.outpath.wstring().data(), NULL, NULL, SW_NORMAL);
			}
			else 
			{
				if(bottom.is_ytplaylist && fs::exists(bottom.printed_path))
					ShellExecuteW(NULL, L"open", bottom.printed_path.wstring().data(), NULL, NULL, SW_NORMAL);
				else ShellExecuteW(NULL, L"open", bottom.outpath.wstring().data(), NULL, NULL, SW_NORMAL);
			}
		});

		if(!file.empty()) m.append("Open file of " + item_name, [&, file, this](menu::item_proxy)
		{
			ShellExecuteW(NULL, L"open", file.wstring().data(), NULL, NULL, SW_NORMAL);
		});

		if(item.text(3) != "error")
		{
			m.append_splitter();
			if(bottom.is_ytplaylist || bottom.is_bcplaylist)
			{
				auto count {bottom.playlist_selection.size()};
				std::string item_text {(bottom.is_ytplaylist ? "Select videos (" : "Select songs (") + (count && !bottom.playlist_info.empty() ?
					std::to_string(bottom.playlist_selected()) + '/' + std::to_string(count) + ")" : "getting data...)")};
				auto item = m.append(item_text, [this](menu::item_proxy)
				{
					dlg_playlist();
				});
				if(!count || bottom.playlist_info.empty())
				{
					item.enabled(false);
					vidsel_item = {&m, item.index()};
				}
			}
			else if(!bottom.is_ytchan && !bottom.is_bcchan && !is_live && item.text(2).find("[live event scheduled to begin in") != 0)
			{
				m.append("Download sections", [this](menu::item_proxy)
				{
					dlg_sections();
				});
			}

			if(item.text(2) == "...")
				m.append("Select formats", [this](menu::item_proxy) { dlg_formats(); }).enabled(false);
			else if(bottom.btnfmt_visible())
			{
				m.append("Select formats", [this](menu::item_proxy)
				{
					dlg_formats();
				});
			}

			auto jitem = m.append("View JSON data", [this](menu::item_proxy)
			{
				dlg_json();
			});
			jitem.enabled(!bottom.vidinfo.empty() || !bottom.playlist_info.empty());
			if(!jitem.enabled())
				vidsel_item = {&m, vidsel_item.pos};
		}

		if(lbq.at(0).size() > 1)
		{
			m.append_splitter();
			if(!completed.empty())
			{
				m.append("Clear completed", [this](menu::item_proxy)
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
				m.append("Start all", [&, this](menu::item_proxy)
				{
					thr_menu = std::thread([&, this]
					{
						menu_working = true;
						autostart_next_item = false;
						for(auto item : startable)
						{
							if(!menu_working) break;
							auto url {item.value<lbqval_t>().url};
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
				m.append("Stop all", [&, this](menu::item_proxy)
				{
					thr_menu = std::thread([&, this]
					{
						menu_working = true;
						autostart_next_item = false;
						for(auto item : stoppable)
						{
							if(!menu_working) break;
							auto url {item.value<lbqval_t>().url};
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
				m.append("Remove all", [&, this](menu::item_proxy)
				{
					menu_working = true;
					bottoms.show(L"");
					qurl = L"";
					l_url.update_caption();
					lbq.auto_draw(false);
					auto item {lbq.at(0).begin()};
					while(item != lbq.at(0).end() && menu_working)
					{
						auto url {item.value<lbqval_t>().url};
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

		m.append_splitter();
	}

	auto update_inline_widgets = [this]
	{
		lbq.auto_draw(false);
		for(auto item : lbq.at(0))
			item.text(1, '%' + std::to_string(conf.col_site_icon + conf.col_site_text * 2) + item.text(1));
		lbq.auto_draw(true);
		adjust_lbq_headers();
	};

	make_columns_menu(m.create_sub_menu(m.append("Extra columns").index()));
	auto m2 {m.create_sub_menu(m.append("Website column").index())};

	m2->append("Favicon", [&, this](menu::item_proxy)
	{
		conf.col_site_icon = !conf.col_site_icon;
		update_inline_widgets();
	}).checked(conf.col_site_icon);

	m2->append("Text", [&, this](menu::item_proxy)
	{
		conf.col_site_text = !conf.col_site_text;
		update_inline_widgets();
	}).checked(conf.col_site_text);

	m.popup_await(lbq, x, y);
	vidsel_item.m = nullptr;
	return url_of_item_to_delete;
}


void GUI::make_queue_listbox()
{
	using namespace nana;
	using namespace util;

	lbq.sortable(false);
	lbq.enable_single(true, false);
	lbq.typeface(nana::paint::font_info {"Calibri", 12});
	lbq.scheme().item_height_ex = 8;
	lbq.append_header("#", scale(30));
	lbq.append_header("Website", scale(20 + !conf.col_site_text * 10) * conf.col_site_icon + scale(110) * conf.col_site_text);
	lbq.append_header("Media title", scale(584));
	lbq.append_header("Status", scale(116));
	lbq.append_header("Format", scale(130));
	lbq.append_header("Format note", scale(150));
	lbq.append_header("Ext", scale(60));
	lbq.append_header("Filesize", scale(100));
	lbq.column_movable(false);
	lbq.column_resizable(false);
	lbq.column_at(4).visible(conf.col_format);
	lbq.column_at(5).visible(conf.col_format_note);
	lbq.column_at(6).visible(conf.col_ext);
	lbq.column_at(7).visible(conf.col_fsize);
	lbq.at(0).inline_factory(1, pat::make_factory<inline_widget>());

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
				if(arg.pos.y > dpi_transform(21)) // if below header
					last_selected = nullptr;
			}
		}
		else
		{
			lbq_can_drag = true;
			auto hovitem {lbq.at(hovered)};
			if(last_selected && last_selected->value<lbqval_t>() == hovitem.value<lbqval_t>().url)
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
				const auto &url {arg.item.value<lbqval_t>().url};
				if(url != bottoms.visible())
				{
					bottoms.show(url);
					outbox.current(url);
					qurl = url;
					l_url.update_caption();
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
					seltext3 {lb.at(selected.item).text(3)},
					seltext4 {lb.at(selected.item).text(4)},
					seltext5 {lb.at(selected.item).text(5)},
					seltext6 {lb.at(selected.item).text(6)},
					seltext7 {lb.at(selected.item).text(7)};

				lbqval_t selval {lb.at(selected.item).value<lbqval_t>()};
				std::string sel_favicon_url {lbq.favicon_url_from_value(selval)};

				auto hovitem {lb.at(hovered.item)};

				// moving item upward, pushing list downward below insertion point
				if(hovered.item < selected.item)
				{
					for(auto n {selected.item}; n > hovered.item; n--)
					{
						const auto &val {lb.at(n - 1).value<lbqval_t>()};
						lb.at(n).value(val);
						lb.at(n).text(1, lb.at(n - 1).text(1));
						lb.at(n).text(2, lb.at(n - 1).text(2));
						lb.at(n).text(3, lb.at(n - 1).text(3));
						lb.at(n).text(4, lb.at(n - 1).text(4));
						lb.at(n).text(5, lb.at(n - 1).text(5));
						lb.at(n).text(6, lb.at(n - 1).text(6));
						lb.at(n).text(7, lb.at(n - 1).text(7));
						if(!conf.common_dl_options)
							bottoms.at(val).gpopt.caption("Download options for queue item #" + lb.at(n).text(0));
						lb.at(n).fgcolor(lbq.fgcolor());
					}
					if(autoscroll)
					{
						if(arg.pos.y / util::scale(27) <= 2)
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
						const auto &val {lb.at(n + 1).value<lbqval_t>()};
						lb.at(n).value(val);
						lb.at(n).text(1, lb.at(n + 1).text(1));
						lb.at(n).text(2, lb.at(n + 1).text(2));
						lb.at(n).text(3, lb.at(n + 1).text(3));
						lb.at(n).text(4, lb.at(n + 1).text(4));
						lb.at(n).text(5, lb.at(n + 1).text(5));
						lb.at(n).text(6, lb.at(n + 1).text(6));
						lb.at(n).text(7, lb.at(n + 1).text(7));
						if(!conf.common_dl_options)
							bottoms.at(val).gpopt.caption("Download options for queue item #" + lb.at(n).text(0));
						lb.at(n).fgcolor(lbq.fgcolor());
					}
					if(autoscroll)
					{
						if(arg.pos.y > lbq.size().height - 2 * util::scale(27))
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
				hovitem.value(selval);
				hovitem.text(1, seltext1);
				hovitem.text(2, seltext2);
				hovitem.text(3, seltext3);
				hovitem.text(4, seltext4);
				hovitem.text(5, seltext5);
				hovitem.text(6, seltext6);
				hovitem.text(7, seltext7);
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
		if(sel.empty()) return;
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
				auto &bot {bottoms.at(item.value<lbqval_t>())};
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
		{
			auto url {pop_queue_menu(arg.pos.x, arg.pos.y)};
			if(!url.empty())
			{
				lbq.auto_draw(false);
				remove_queue_item(url);
				lbq.auto_draw(true);
			}
		}
	});

	lbq.events().mouse_leave([this]
	{
		if(queue_panel.visible())
			if(GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON) & 0xff00)
				dragstop_fn();
	});
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

	plc_queue.div(R"(
		vert <lbq> <weight=20> 
		<weight=25 <btn_settings weight=106> <weight=15> <l_url> <weight=15> <btn_qact weight=126>>
	)");
	plc_queue["lbq"] << lbq;
	plc_queue["l_url"] << l_url;
	plc_queue["btn_qact"] << btn_qact;
	plc_queue["btn_settings"] << btn_settings;

	apply_theme(::widgets::theme.is_dark());

	if(API::screen_dpi(true) > 96)
		btn_settings.image(arr_config22_ico, sizeof arr_config22_ico);
	else btn_settings.image(arr_config16_ico, sizeof arr_config16_ico);
	btn_settings.events().click([this] { dlg_settings(); });
	btn_qact.tooltip("Pops up a menu with actions that can be performed on\nthe queue items (same as right-clicking on the queue).");

	l_url.events().mouse_enter([this]
	{
		auto text {util::get_clipboard_text()};
		if(text.starts_with(LR"(https://www.youtube.com/watch?v=)"))
			if(text.find(L"&list=") == 43)
				text.erase(43);
		
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
		if(theme.is_dark())
			l_url.fgcolor(theme.path_link_fg.blend(colors::black, .25));
		else l_url.fgcolor(theme.path_link_fg.blend(colors::white, .25));			
	});

	l_url.events().mouse_leave([this]
	{
		auto sel {lbq.selected()};
		if(sel.empty())
			qurl.clear();
		else qurl = lbq.at(sel.front()).value<lbqval_t>();
		l_url.update_caption();
		if(theme.is_dark())
			l_url.fgcolor(theme.path_link_fg);
		else l_url.fgcolor(theme.path_link_fg);
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

			if(theme.is_dark())
				l_url.fgcolor(theme.path_link_fg);
			else l_url.fgcolor(theme.path_link_fg);
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


void GUI::dlg_settings()
{
	using widgets::theme;

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.center(820, 500);
	fm.caption(title + " - settings");
	fm.bgcolor(theme.fmbg);

	fm.div(R"(vert margin=20 < <tree weight=150> <weight=20> <switchable <ytdlp> <sblock> <queuing> <gui> <updater>> >)");

	widgets::conf_page ytdlp {fm}, queuing {fm}, gui {fm}, sblock {fm};
	updater.create(fm);
	updater_init = true;
	make_updater_page(fm);

	widgets::conf_tree tree {fm, &fm.get_place()};
	tree.add("yt-dlp", "ytdlp");
	tree.add("SponsorBlock", "sblock");
	tree.add("Queuing", "queuing");
	tree.add("Interface", "gui");
	tree.add("Updater", "updater");

	fm["tree"] << tree;
	fm["ytdlp"] << ytdlp;
	fm["sblock"] << sblock;
	fm["queuing"] << queuing;
	fm["gui"] << gui;
	fm["updater"] << updater;

	widgets::Label l_res {ytdlp, "Preferred resolution:"},
		l_video {ytdlp, "Preferred video container:"}, l_audio {ytdlp, "Preferred audio container:"},
		l_theme {gui, "Color theme:"}, l_contrast {gui, "Contrast:"}, l_ytdlp {ytdlp, "Path to yt-dlp:"},
		l_template {ytdlp, "Output template:"}, l_maxdl {queuing, "Max concurrent downloads:"}, l_playlist {ytdlp, "Playlist indexing:"},
		l_opendlg_origin {gui, "When browsing for the output folder, start in:"}, l_sblock {sblock, ""};
	widgets::path_label l_path {ytdlp, &conf.ytdlp_path};
	widgets::Textbox tb_template {ytdlp}, tb_playlist {ytdlp}, tb_proxy {ytdlp};
	widgets::Combox com_res {ytdlp}, com_video {ytdlp}, com_audio {ytdlp};
	widgets::cbox cbfps {ytdlp, "Prefer a higher framerate"}, cbtheme_dark {gui, "Dark"}, cbtheme_light {gui, "Light"},
		cbtheme_system {gui, "System preference"}, cb_lengthyproc {queuing, "Start next item on lengthy processing"},
		cb_common {queuing, "Each queue item has its own download options"},
		cb_autostart {queuing, "When stopping a queue item, automatically start the next one"},
		cb_queue_autostart {queuing, "When the program starts, automatically start processing the queue"},
		cb_zeropadding {ytdlp, "Pad the indexed filenames with zeroes"}, cb_playlist_folder {ytdlp, "Put playlists in their own folders"},
		cb_origin_progdir {gui, "Program folder"}, cb_origin_curdir {gui, "Currently selected folder"}, 
		cb_mark {sblock, "Mark these categories:"}, cb_remove {sblock, "Remove these categories:"}, cb_proxy {ytdlp, "Use this proxy:"};
	widgets::Separator sep1 {ytdlp}, sep2 {ytdlp}, sep3 {gui}, sep4 {fm};
	widgets::Button btn_close {fm, " Close"}, btn_default {ytdlp, "Reset to default", true},
		btn_playlist_default {ytdlp, "Reset to default", true}, btn_info {ytdlp};
	widgets::Spinbox sb_maxdl {queuing};
	widgets::Slider slider {gui};
	widgets::sblock_listbox lbmark {sblock}, lbremove {sblock};
	widgets::Infobox l_info {sblock};

	ytdlp.div(R"(vert
		<weight=26 <weight=42> <l_res weight=156> <weight=10> <com_res weight=56> <> 
			<cbfps weight=228> <btn_info weight=26>> <weight=20>
		<weight=25
			<l_video weight=198> <weight=10> <com_video weight=61> <>
			<l_audio weight=200> <weight=10> <com_audio weight=61>
		>
		<weight=20> <sep1 weight=3px> <weight=20>
		<weight=25 <l_template weight=132> <weight=10> <tb_template> <weight=15> <btn_default weight=140>> <weight=20>
		<weight=25 <l_playlist weight=132> <weight=10> <tb_playlist> <weight=15> <btn_playlist_default weight=140>> <weight=20>
		<weight=25 <cb_zeropadding weight=323> <weight=20> <cb_playlist_folder>>
		<weight=20> <sep2 weight=3px> <weight=20>
		<weight=25 <l_ytdlp weight=132> <weight=10> <l_path> > <weight=20>
		<weight=25 <proxy_padding weight=11> <cb_proxy weight=121> <weight=10> <tb_proxy>>
	)");

	fm.subclass_before(WM_SETFOCUS, [&](UINT, WPARAM, LPARAM, LRESULT*)
	{
		bool upd {false}, path_empty {conf.ytdlp_path.empty()}, path_gone {!fs::exists(conf.ytdlp_path)};
		if(path_empty || path_gone)
		{
			if(fs::exists(appdir / "ytdl-patched-red.exe"))
			{
				GUI::conf.ytdlp_path = appdir / "ytdl-patched-red.exe";
				l_path.update_caption();
				l_path.refresh_theme();
				get_version_ytdlp();
				upd = true;
			}
			else if(fs::exists(appdir / ytdlp_fname))
			{
				GUI::conf.ytdlp_path = appdir / ytdlp_fname;
				l_path.update_caption();
				l_path.refresh_theme();
				get_version_ytdlp();
				upd = true;
			}
			else
			{
				if(path_empty)
					l_path.caption("!!!  YT-DLP.EXE NOT FOUND IN PROGRAM FOLDER  !!!");
				else
				{
					l_path.caption("!!!  YT-DLP.EXE NOT FOUND AT ITS SAVED LOCATION  !!!");
					conf.ytdlp_path.clear();
				}
				l_path.refresh_theme();
				ver_ytdlp = {0, 0, 0};
			}
			updater_t2.start();
		}
		else l_path.update_caption();
		return true;
	});

	if(cnlang)
	{
		ytdlp.get_place().field_display("proxy_padding", false);
		change_field_attr(ytdlp.get_place(), "cb_proxy", "weight", 132);
	}

	ytdlp["l_res"] << l_res;
	ytdlp["com_res"] << com_res;
	ytdlp["cbfps"] << cbfps;
	ytdlp["btn_info"] << btn_info;
	ytdlp["l_ytdlp"] << l_ytdlp;
	ytdlp["l_path"] << l_path;
	ytdlp["l_video"] << l_video;
	ytdlp["com_video"] << com_video;
	ytdlp["l_audio"] << l_audio;
	ytdlp["com_audio"] << com_audio;
	ytdlp["sep1"] << sep1;
	ytdlp["l_template"] << l_template;
	ytdlp["tb_template"] << tb_template;
	ytdlp["btn_default"] << btn_default;
	ytdlp["l_playlist"] << l_playlist;
	ytdlp["tb_playlist"] << tb_playlist;
	ytdlp["btn_playlist_default"] << btn_playlist_default;
	ytdlp["cb_zeropadding"] << cb_zeropadding;
	ytdlp["cb_playlist_folder"] << cb_playlist_folder;
	ytdlp["sep2"] << sep2;
	ytdlp["cb_proxy"] << cb_proxy;
	ytdlp["tb_proxy"] << tb_proxy;

	sblock.div(R"(vert		
		<l_sblock weight=25> <weight=18>
		<weight=25 <cb_mark weight=295> <weight=20> <cb_remove weight=295>> <weight=10>
		<<lbmark> <weight=20> <lbremove>> <weight=20>
		<l_info weight=62>
	)");

	sblock["l_sblock"] << l_sblock;
	sblock["cb_mark"] << cb_mark;
	sblock["cb_remove"] << cb_remove;
	sblock["lbmark"] << lbmark;
	sblock["lbremove"] << lbremove;
	sblock["l_info"] << l_info;

	cb_proxy.check(conf.cb_proxy);
	tb_proxy.caption(conf.proxy);
	tb_proxy.multi_lines(false);
	tb_proxy.padding(0, 5, 0, 5);
	tb_proxy.typeface(nana::paint::font_info {"Tahoma", 10});

	std::string sblock_text {"<color=0x url=\"https://sponsor.ajay.app\">SponsorBlock</> lets users mark or remove segments in YouTube videos"};
	l_sblock.format(true);
	l_sblock.tooltip("https://sponsor.ajay.app");
	l_sblock.text_align(nana::align::center, nana::align_v::center);
	for(const auto &cat : sblock_cats_mark)
	{
		auto key {nana::to_utf8(cat)};
		lbmark.at(0).push_back(sblock_infos[key].first);
		lbmark.at(0).back().value(key);
	}
	for(const auto &cat : sblock_cats_remove)
	{
		auto key {nana::to_utf8(cat)};
		lbremove.at(0).push_back(sblock_infos[key].first);
		lbremove.at(0).back().value(key);
	}
	for(auto i : conf.sblock_mark)
		lbmark.at(0).at(i).check(true);
	for(auto i : conf.sblock_remove)
		lbremove.at(0).at(i).check(true);
	cb_mark.check(conf.cb_sblock_mark);
	cb_remove.check(conf.cb_sblock_remove);
	cb_mark.tooltip("yt-dlp will create chapters for the segments in these categories\n(this passes <bold>--sponsorblock-mark</> to yt-dlp)");
	cb_remove.tooltip("yt-dlp will remove the segments in these categories\n(this passes <bold>--sponsorblock-remove</> to yt-dlp)");

	size_t hovitem_mark {nana::npos};
	lbmark.events().mouse_move([&, this](const nana::arg_mouse &arg)
	{
		auto hovered {lbmark.cast({arg.pos.x, arg.pos.y})};
		if(hovered.item != -1 && hovered.item != hovitem_mark)
		{
			hovitem_mark = hovered.item;
			l_info.caption(sblock_infos[lbmark.at(hovered).value<std::string>()].second);
		}
	});
	lbmark.events().mouse_leave([&, this](const nana::arg_mouse &arg)
	{
		l_info.caption("");
		hovitem_mark = nana::npos;
	});
	lbremove.events().mouse_move([&, this](const nana::arg_mouse &arg)
	{
		auto hovered {lbremove.cast({arg.pos.x, arg.pos.y})};
		if(hovered.item != -1 && hovered.item != hovitem_mark)
		{
			hovitem_mark = hovered.item;
			l_info.caption(sblock_infos[lbremove.at(hovered).value<std::string>()].second);
		}
	});
	lbremove.events().mouse_leave([&, this](const nana::arg_mouse &arg)
	{
		l_info.caption("");
		hovitem_mark = nana::npos;
	});
	
	std::string link_dark {"0xb0c0d0"}, link_light {"0x688878"}, link_hilited_dark {"0xd5e5f5"}, link_hilited_light {"0x436353"};
	bool sblock_hilite {false};
	l_sblock.events().mouse_move([&, this] (const nana::arg_mouse &arg)
	{
		if(l_sblock.cursor() == nana::cursor::hand)
		{
			if(!sblock_hilite)
			{
				sblock_hilite = true;
				l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme.is_dark() ? link_hilited_dark : link_hilited_light));
			}
		}
		else if(sblock_hilite)
		{
			sblock_hilite = false;
			l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme.is_dark() ? link_dark : link_light));
		}
	});
	l_sblock.events().mouse_leave([&, this]
	{
		if(sblock_hilite)
		{
			sblock_hilite = false;
			l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme.is_dark() ? link_dark : link_light));
		}
	});

	queuing.div(R"(vert		
		<weight=25 <l_maxdl weight=216> <weight=10> <sb_maxdl weight=40> <> <cb_lengthyproc weight=318>> <weight=20>
		<weight=25 <cb_autostart weight=508>> <weight=20>
		<weight=25 <cb_common weight=408>> <weight=20>
		<weight=25 <cb_queue_autostart weight=548>> <weight=20>
	)");
	
	l_maxdl.text_align(nana::align::left, nana::align_v::center);
	if(!cnlang)
	{
		change_field_attr(queuing.get_place(), "l_maxdl", "weight", 196);
		change_field_attr(queuing.get_place(), "cb_lengthyproc", "weight", 290);
	}

	queuing["l_maxdl"] << l_maxdl;
	queuing["sb_maxdl"] << sb_maxdl;
	queuing["cb_lengthyproc"] << cb_lengthyproc;
	queuing["cb_autostart"] << cb_autostart;
	queuing["cb_common"] << cb_common;
	queuing["cb_queue_autostart"] << cb_queue_autostart;

	gui.div(R"(vert		
		<weight=25 <l_theme weight=100> <weight=20> <cbtheme_dark weight=65> <weight=20> 
			<cbtheme_light weight=66> <weight=20> <cbtheme_system weight=178> > <weight=20>
		<weight=25 <l_contrast weight=70> <weight=20> <slider> >
		<weight=20> <sep3 weight=3px> <weight=20>
		<weight=25 <l_opendlg_origin weight=380> <cb_origin_curdir>> <weight=10>
		<weight=25 <opendlg_spacer weight=380> <cb_origin_progdir>> <weight=20>
	)");

	if(!cnlang)
	{
		change_field_attr(gui.get_place(), "l_opendlg_origin", "weight", 345);
		change_field_attr(gui.get_place(), "opendlg_spacer", "weight", 345);
	}

	gui["l_theme"] << l_theme;
	gui["cbtheme_dark"] << cbtheme_dark;
	gui["cbtheme_light"] << cbtheme_light;
	if(system_supports_darkmode())
		gui["cbtheme_system"] << cbtheme_system;
	gui["l_contrast"] << l_contrast;
	gui["slider"] << slider;
	gui["sep3"] << sep3;
	gui["l_opendlg_origin"] << l_opendlg_origin;
	gui["cb_origin_progdir"] << cb_origin_progdir;
	gui["cb_origin_curdir"] << cb_origin_curdir;

	l_opendlg_origin.text_align(nana::align::left, nana::align_v::center);
	if(nana::API::screen_dpi(true) > 96)
		btn_info.image(arr_info22_ico, sizeof arr_info22_ico);
	else btn_info.image(arr_info16_ico, sizeof arr_info22_ico);
	btn_info.events().click([&, this] {dlg_settings_info(fm); });
	btn_close.events().click([&] {fm.close(); });

	const auto &bottom {bottoms.current()};
	auto &output_template_default {bottom.is_bcplaylist ? conf.output_template_default_bandcamp : conf.output_template_default};
	auto &output_template {bottom.is_bcplaylist ? conf.output_template_bandcamp : conf.output_template};
	btn_default.tooltip(nana::to_utf8(output_template_default));
	btn_playlist_default.tooltip(nana::to_utf8(conf.playlist_indexing_default));
	tb_template.multi_lines(false);
	tb_template.padding(0, 5, 0, 5);
	tb_template.caption(output_template);
	tb_template.typeface(nana::paint::font_info {"Tahoma", 10});
	tb_playlist.multi_lines(false);
	tb_playlist.padding(0, 5, 0, 5);
	tb_playlist.caption(conf.playlist_indexing);
	tb_playlist.typeface(nana::paint::font_info {"Tahoma", 10});
	if(bottom.is_bcplaylist)
	{
		l_playlist.enabled(false);
		tb_playlist.enabled(false);
		btn_playlist_default.enabled(false);
		cb_zeropadding.enabled(false);
	}
	btn_default.events().click([&, this]
	{
		tb_template.caption(output_template_default);
		output_template = output_template_default;
	});
	btn_playlist_default.events().click([&, this]
	{
		tb_playlist.caption(conf.playlist_indexing_default);
		conf.playlist_indexing = conf.playlist_indexing_default;
	});

	sb_maxdl.range(1, 10, 1);
	sb_maxdl.value(std::to_string(conf.max_concurrent_downloads));

	slider.maximum(30);
	slider.value(conf.contrast * 100);
	slider.events().value_changed([&]
	{
		conf.contrast = static_cast<double>(slider.value()) / 100;
		theme.contrast(conf.contrast);
		fm.bgcolor(theme.fmbg);
		apply_theme(theme.is_dark());
		nana::api::refresh_window(bottoms.current().gpopt);
		tb_template.refresh_theme();
		tb_playlist.refresh_theme();
		com_audio.refresh_theme();
		com_video.refresh_theme();
		com_res.refresh_theme();
		cb_autostart.refresh_theme();
		cb_common.refresh_theme();
		sb_maxdl.refresh_theme();
		cbfps.refresh_theme();
		cb_queue_autostart.refresh_theme();
		btn_close.refresh_theme();
		btn_default.refresh_theme();
		btn_playlist_default.refresh_theme();
		cb_playlist_folder.refresh_theme();
		cb_zeropadding.refresh_theme();
		ytdlp.refresh_theme();
		queuing.refresh_theme();
		gui.refresh_theme();
		tree.refresh_theme();
	});

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

	nana::radio_group rg_origin;
	rg_origin.add(cb_origin_progdir);
	rg_origin.add(cb_origin_curdir);
	if(conf.open_dialog_origin)
		cb_origin_curdir.check(true);
	else cb_origin_progdir.check(true);

	cbfps.tooltip("If several different formats have the same resolution,\ndownload the one with the highest framerate.");

	cb_lengthyproc.tooltip("When yt-dlp finishes downloading all the files associated with a queue item,\n"
		"it performs actions on the files referred to as \"post-processing\". These actions\ncan be a number of things, depending "
		"on the context, and on the options used.\n\nSometimes this post-processing phase can take a long time, so this setting\n"
		"makes the program automatically start the next queue item when processing\ntakes longer that 3 seconds.");

	cb_zeropadding.tooltip("This option pads the index numbers in the filenames of playlist videos\nwith zeroes, when the playlist "
		"contains more than 9 videos. This allows\nthe filenames to be sorted properly when viewed in a file manager (for\nexample, "
		"<bold>\"2 - name\"</> comes after <bold>\"14 - name\"</> in the sort order, but\n<bold>\"02 - name\"</> comes before <bold>\"14 - name\"</>).\n\n"
		"This feature works by automatically editing the \"Playlist indexing\" string\ndefined above, as long as it contains "
		"<bold>\"%(playlist_index)d\"</>.\n\nAn appropriate amount of zeroes is added, according to the size of the\nplaylist "
		"(if it has tens of videos, up to one zero; if it has hundreds of\nvideos, up to two zeroes; etc).");

	cb_playlist_folder.tooltip("Put playlist videos in a subfolder of the output folder, using the\nplaylist title as the name of "
		"the subfolder.\n\nThis works by automatically prepending <bold>\"%(playlist_title)s\\\"</> to the\noutput template defined above, "
		"whenever a playlist is downloaded\n\nSince v1.9, this setting also applies to channels and channel tabs.\n\n"
		"Since v2.1, this setting also applies to Bandcamp albums (prepends\n<bold>\"%(artist)s\\%(album)s\\\"</> to the output template).");

	const auto res_tip {"Download the best video with the largest resolution available that is\n"
		"not higher than the selected value. Resolution is determined using the\n"
		"smallest dimension, so this works correctly for vertical videos as well."},

		maxdl_tip {"When a queue item finishes, the program automatically starts the next one.\nThis setting lets you make it "
		"start the next two or more items (up to 10).\n\nDoes not limit the number of manually started queue items "
		"(you can have\nmore than 10 concurrent downloads if you want, but you have to start\nthe ones after the 10th manually)."},

		template_tip {"The output template tells yt-dlp how to name the downloaded files.\nIt's a powerful way to compose the output "
		"file name, allowing many \ncharacteristics of the downloaded media to be incorporated \ninto the name. To learn how to use it, "
		"see the documentation at \n<bold>https://github.com/yt-dlp/yt-dlp#output-template</>"},

		playlist_tip {"This is a string that the program optionally incorporates into the \noutput template defined above, only when "
		"downloading \nYouTube playlists. It is prepended to the output template string, \nso with default values, the result would "
		"look like this:\n\n<bold>\"%(playlist_index)d - %(title)s.%(ext)s\"</>\n\nThis output template produces filenames that look like this:\n\n"
		"<bold>1 - title.ext\n2 - title.ext\n3 - title.ext</>\n\nLeave blank if you don't want playlist videos to be numbered."},

		container_tip {"A container format allows multiple data streams to be embedded\ninto a single file. A media container is a file that "
		"can contain multiple \nvideo and/or audio streams. The codec with which the video and \naudio streams are encoded affects which "
		"container they can fit in.\n\nFor example, the MPEG-4 video container format (<bold>.mp4</> file) can \ncontain audio streams encoded with the "
		"<bold>mp4a</> codec, but doesn't\nsupport audio encoded with the <bold>opus</> codec (which yt-dlp considers\nto be the better "
		"of the two).\n\nFor that reason, if you select <bold>mp4</> "
		"as the preferred video container,\nyou must select <bold>m4a</> as the preferred audio container. Otherwise,\nyt-dlp will be forced to "
		"merge the streams into an <bold>.mkv</> file instead\nof an <bold>.mp4</> file (when combining an <bold>.mp4</> video-only format "
		"with\nan incompatible audio-only format)."},

		proxy_tip {"Tell yt-dlp to use a proxy server (this passes <bold>--proxy</> to yt-dlp)\n\n"
		"\tHTTP proxy:  <bold>IP_ADDRESS:PORT</>\n\n\tSOCKS proxy:  <bold>socks5://IP_ADDRESS:PORT</>\n\n"
		"\tSOCKS proxy with authentication:  <bold>socks5://USER:PASS@IP_ADDRESS:PORT</>\t"};

	cb_proxy.tooltip(proxy_tip);
	tb_proxy.tooltip(proxy_tip);

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
	l_template.tooltip(template_tip);
	tb_template.tooltip(template_tip);
	l_playlist.tooltip(playlist_tip);
	tb_playlist.tooltip(playlist_tip);
	l_video.tooltip(container_tip);
	l_audio.tooltip(container_tip);
	com_video.tooltip(container_tip);
	com_audio.tooltip(container_tip);

	com_res.option(conf.pref_res);
	com_video.option(conf.pref_video);
	com_audio.option(conf.pref_audio);
	cbfps.check(conf.pref_fps);
	cb_lengthyproc.check(conf.cb_lengthyproc);
	cb_autostart.check(conf.cb_autostart);
	cb_common.check(!conf.common_dl_options);
	cb_queue_autostart.check(conf.cb_queue_autostart);
	cb_zeropadding.check(conf.cb_zeropadding);
	cb_playlist_folder.check(conf.cb_playlist_folder);

	l_path.events().click([&, this]
	{
		nana::filebox fb {fm, true};
		if(!conf.ytdlp_path.empty())
			fb.init_file(conf.ytdlp_path);
		fb.allow_multi_select(false);
		fb.add_filter("yt-dlp executable", ytdlp_fname + ";ytdl-patched-red.exe");
		fb.title("Locate and select yt-dlp.exe or ytdl-patched-red.exe");
		auto res {fb()};
		if(res.size())
		{
			auto path {res.front()};
			auto fname {path.filename().string()};
			if(fname == "yt-dlp.exe" || fname == "ytdl-patched-red.exe")
			{
				conf.ytdlp_path = path;
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
		output_template = tb_template.caption_wstring();
		conf.playlist_indexing = tb_playlist.caption_wstring();
		conf.max_concurrent_downloads = sb_maxdl.to_int();
		conf.cb_lengthyproc = cb_lengthyproc.checked();
		conf.cb_autostart = cb_autostart.checked();
		conf.cb_queue_autostart = cb_queue_autostart.checked();
		conf.cb_playlist_folder = cb_playlist_folder.checked();
		conf.cb_zeropadding = cb_zeropadding.checked();
		conf.open_dialog_origin = cb_origin_curdir.checked();
		conf.cb_sblock_mark = cb_mark.checked();
		conf.cb_sblock_remove = cb_remove.checked();
		conf.cb_proxy = cb_proxy.checked();
		conf.proxy = tb_proxy.caption_wstring();

		conf.sblock_mark.clear();
		for(auto ip : lbmark.at(0))
			if(ip.checked())
				conf.sblock_mark.push_back(ip.pos().item);

		conf.sblock_remove.clear();
		for(auto ip : lbremove.at(0))
			if(ip.checked())
				conf.sblock_remove.push_back(ip.pos().item);

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

		updater_working = false;
		conf.get_releases_at_startup = cb_startup.checked();

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
	});

	fm.theme_callback([&, this](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme.fmbg);
		ytdlp.bgcolor(theme.fmbg);
		queuing.bgcolor(theme.fmbg);
		gui.bgcolor(theme.fmbg);
		l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme.is_dark() ? link_dark : link_light));
		return true;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.collocate();

	tree.select("ytdlp");
	fm.show();
	fm.modality();
}


void GUI::dlg_settings_info(nana::window owner)
{
	using namespace nana;
	using ::widgets::theme;

	themed_form fm {nullptr, owner, {}, appear::decorate{}};
	fm.caption(title + " - format sorting info");
	fm.bgcolor(theme.fmbg);
	fm.center(621 + 40 * (nana::API::screen_dpi(true) >= 144), 680);
	fm.div("vert margin=20 <weight=284 <> <pic weight=581> <>> <weight=20> <p>");

	paint::image imgdark, imglight;
	imgdark.open(arr_settings_info_dark_png, sizeof arr_settings_info_dark_png);
	imglight.open(arr_settings_info_light_png, sizeof arr_settings_info_light_png);

	picture pic {fm};
	pic.stretchable(true);
	pic.transparent(true);
	pic.align(align::center, align_v::center);

	::widgets::Label p {fm, ""};

	p.typeface(paint::font_info {"Tahoma", 10});
	p.text_align(align::left, align_v::top);
	p.format(true);

	std::string ptext {R"(By default (when you don't explicitly request a specific format), yt-dlp downloads the best audio/video
formats, which it judges according to certain parameters. For example, the main criteria that determine the "best" YouTube video format are quality, resolution, and framerate (prioritized in that order).

The four settings pictured above allow you to change what yt-dlp considers "the best" in each of those categories, at the same time reordering how the categories are prioritised. For example, if you
select "1080" for the preferred resolution, then yt-dlp will consider 1080p to be the best resolution, instead of the highest available. With the preferences configured like in the above picture, yt-dlp will first identify all the video formats with 1080p resolution, then out of those, all with .mp4 extension, and out of those, all with the highest framerate, and so on until one format is chosen.

Of course, this is all optional – you can select "none" for each of these settings to let yt-dlp use its default priorities. Also, the program only uses these settings when you don't request a specific format
(like <bold color=0x>-f 303+251</>, or <bold color=0x>-f ba</>).

The program lets you manually select formats from a list, but it should be clear that you don't have to
do that. That feature can be useful in certain cases, but yt-dlp does a good job of automatically selecting the best formats for whatever you're downloading. The four preferences discussed here
(which the program communicates to yt-dlp with the <bold color=0x>-S</> argument) should be enough to ensure that in most cases, you can just click the download button without worrying about formats.)"};

	fm["pic"] << pic;
	fm["p"] << p;

	drawing {fm}.draw([&](paint::graphics &g)
	{
		if(api::is_window(pic))
			g.rectangle(rectangle {pic.pos(), pic.size()}.pare_off(-1), false, theme.border);
	});

	fm.theme_callback([&, this](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme.fmbg);
		pic.load(dark ? imgdark : imglight);
		p.caption(std::regex_replace(ptext, std::regex {"\\b(0x)"}, theme.is_dark() ? "0xd0c0b0" : "0x708099"));
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.collocate();
	fm.show();
	fm.modality();
}


void GUI::dlg_json()
{
	using widgets::theme;
	using namespace nana;

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::sizable, appear::minimize, appear::maximize>{}};
	fm.center(1000, 1006);
	fm.caption(title + " - JSON viewer");
	fm.bgcolor(theme.fmbg);

	fm.div("vert margin=20 <tree> <weight=20> <label weight=48>");

	auto &bottom {bottoms.current()};
	::widgets::JSON_Tree t {fm, bottom.playlist_info.empty() ? bottom.vidinfo : bottom.playlist_info, conf.json_hide_null};
	::widgets::Label lcmd {fm, ""};

	fm["tree"] << t;
	fm["label"] << lcmd;

	lcmd.typeface(paint::font_info {"Tahoma", 10});
	lcmd.text_align(align::left, align_v::top);
	lcmd.format(true);

	std::string lcmd_text_dark {"<color=0xb0b0b0>The data was obtained from yt-dlp using this command line:</>\n"
		"<bold color=0xa09080>" + to_utf8(bottom.cmdinfo) + "</>"};
	std::string lcmd_text_light {"<color=0x707070>The data was obtained from yt-dlp using this command line:</>\n"
		"<bold color=0x8090a0>" + to_utf8(bottom.cmdinfo) + "</>"};
	lcmd.events().expose([&]
	{
		lcmd.caption(theme.is_dark() ? lcmd_text_dark : lcmd_text_light);
	});

	t.events().mouse_up([&](const arg_mouse &arg)
	{
		if(arg.button == mouse::right_button && t.jdata())
		{
			auto file_selector = [&, this] (std::string desc, std::string filter) -> fs::path
			{
				filebox fb {fm, false};
				fb.init_path(bottom.outpath);
				std::string strname;
				if(bottom.vidinfo_contains("filename"))
					strname = bottom.vidinfo["filename"];
				else if(bottom.vidinfo_contains("_filename"))
					strname = bottom.vidinfo["_filename"];
				if(!strname.empty())
				{
					strname = strname.substr(0, strname.rfind('.'));
					fb.init_file(strname);
				}
				fb.allow_multi_select(false);
				fb.add_filter(desc, filter);
				fb.title("Save as");
				auto res {fb()};
				if(res.size())
					return res.front();
				return {};
			};

			::widgets::Menu m;
			m.item_pixels(dpi_transform(24));

			auto selitem {t.selected()};
			if(!selitem.empty())
			{
				auto seltext {selitem.text()};
				auto pos {seltext.find(':')};
				if(pos != -1)				
				{
					pos += 3;
					if(seltext[pos] == '\"')
					{
						pos++;
						seltext.pop_back();
					}
					auto val {seltext.substr(pos)};
					m.append("Copy selected value", [val, this](menu::item_proxy ip)
					{
						util::set_clipboard_text(hwnd, to_wstring(val));
					});
					m.append_splitter();
				}
			}

			m.append("Save as .txt", [&, this](menu::item_proxy ip)
			{
				auto path {file_selector("Text file", "*.txt")};
				if(!path.empty())
				{
					std::string text {"Text version of the JSON data for URL: " + to_utf8(bottom.url) + "\n\n"};
					std::function<void(treebox::item_proxy)> recfn = [&, this](treebox::item_proxy parent)
					{
						for(auto node : parent)
						{
							if(node.hidden()) continue;
							auto line {node.text()};
							if(node.size())
								line += ':';
							text += std::string(node.level() - 2, '\t') + line + '\n';
							if(node.size())
								recfn(node);
						}
					};
					recfn(t.find("root"));
					std::ofstream {path} << text;
				}
			});

			m.append("Save as .json", [&, this](menu::item_proxy ip)
			{
				auto path {file_selector("JSON data format", "*.json")};
				if(!path.empty())
					std::ofstream {path} << *t.jdata();
			});

			m.append("Save as .json (prettified)", [&, this](menu::item_proxy ip)
			{
				auto path {file_selector("JSON data format", "*.json")};
				if(!path.empty())
					std::ofstream {path} << std::setw(4) << *t.jdata();
			});

			m.append_splitter();

			m.append("Hide null values", [&, this](menu::item_proxy ip)
			{
				conf.json_hide_null = !conf.json_hide_null;
				ip.checked(conf.json_hide_null);

				std::function<void(treebox::item_proxy)> recfn = [&, this](treebox::item_proxy parent)
				{
					for(auto node : parent)
					{
						if(node.size())
							recfn(node);
						else if(node.text().find(":  null") != -1)
							node.hide(conf.json_hide_null);
					}
				};

				t.auto_draw(false);
				recfn(t.find("root"));
				t.auto_draw(true);
			}).checked(conf.json_hide_null);

			m.popup_await(t, arg.pos.x, arg.pos.y);
		}
	});

	drawing {fm}.draw([&](paint::graphics &g)
	{
		if(api::is_window(t))
			g.rectangle(rectangle {t.pos(), t.size()}.pare_off(-1), false, theme.border);
	});

	fm.events().resized([&] { api::refresh_window(fm); });

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
	fm.show();
	fm.modality();
}


void GUI::dlg_changes(nana::window parent)
{
	using widgets::theme;

	themed_form fm {nullptr, parent, {}, appear::decorate<appear::sizable>{}};
	fm.center(1030, 543);
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
	if(cnlang) fm.div(R"(vert margin=20 <tb> <weight=20>
					<weight=25 <> <l_history weight=180> <weight=10> <com_history weight=70> <weight=20> <cblogview weight=158> <> >)");

	else fm.div(R"(vert margin=20 <tb> <weight=20>
				<weight=25 <> <l_history weight=164> <weight=10> <com_history weight=65> <weight=20> <cblogview weight=140> <> >)");

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
				block += "---------------------------------------------------------------------------------------------\n\n";
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
			com_history.push_back(text);
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


void GUI::make_columns_menu(nana::menu *m)
{
	using namespace nana;

	if(m)
	{
		m->append("Format", [&, this](menu::item_proxy ip)
		{
			conf.col_format = !conf.col_format;
			ip.checked(conf.col_format);
			lbq.auto_draw(false);
			lbq.column_at(4).visible(conf.col_format);
			adjust_lbq_headers();
			lbq.auto_draw(true);
		}).checked(conf.col_format);

		m->append("Format note", [&, this](menu::item_proxy ip)
		{
			conf.col_format_note = !conf.col_format_note;
			ip.checked(conf.col_format_note);
			lbq.auto_draw(false);
			lbq.column_at(5).visible(conf.col_format_note);
			adjust_lbq_headers();
			lbq.auto_draw(true);
		}).checked(conf.col_format_note);

		m->append("Ext", [&, this](menu::item_proxy ip)
		{
			conf.col_ext = !conf.col_ext;
			ip.checked(conf.col_ext);
			lbq.auto_draw(false);
			lbq.column_at(6).visible(conf.col_ext);
			adjust_lbq_headers();
			lbq.auto_draw(true);
		}).checked(conf.col_ext);

		m->append("Filesize", [&, this](menu::item_proxy ip)
		{
			conf.col_fsize = !conf.col_fsize;
			ip.checked(conf.col_fsize);
			lbq.auto_draw(false);
			lbq.column_at(7).visible(conf.col_fsize);
			adjust_lbq_headers();
			lbq.auto_draw(true);
		}).checked(conf.col_fsize);
	}
}


bool GUI::is_ytlink(std::wstring text)
{
	auto pos {text.find(L"m.youtube.")};
	if(pos != -1)
		text.replace(pos, 1, L"www");
	if(text.starts_with(LR"(https://www.youtube.com/clip/)"))
		return true;
	if(text.starts_with(LR"(https://www.youtube.com/watch?v=)"))
		if(text.size() == 43)
			return true;
		else if(text.size() > 43 && text[43] == '&')
			return true;
	if(text.starts_with(LR"(https://youtu.be/)"))
		if(text.size() == 28)
			return true;
		else if(text.size() > 28 && text[28] == '?')
			return true;
	if(text.starts_with(LR"(https://www.youtube.com/playlist?list=)"))
		return true;
	return false;
}


void GUI::make_updater_page(themed_form &parent)
{
	updater.div(R"(vert
		<cb_startup weight=25> <weight=15>
		<weight=30 <l_ver weight=110> <weight=10> <l_vertext> <weight=20> <btn_changes weight=150> >
		<weight=20>
		<weight=30 <btn_update weight=100> <weight=20> <prog> > <weight=25>
		<separator weight=3px> <weight=20>
		<weight=25 <l_channel weight=170> <weight=20> <cb_chan_stable weight=75> <weight=10> <cb_chan_nightly weight=85> <> > <weight=15>
		<weight=30 <l_ver_ytdlp weight=170> <weight=10> <l_ytdlp_text> > <weight=10>
		<weight=30 <l_ver_ffmpeg weight=170> <weight=10> <l_ffmpeg_text> > <weight=20>
		<weight=30 <prog_misc> > <weight=25>
		<weight=30 <> <btn_update_ytdlp weight=150> <weight=20> <btn_update_ffmpeg weight=160> <> >
	)");

	if(!cnlang)
	{
		change_field_attr(updater.get_place(), "l_ver", "weight", 101);
	}

	l_ver.create(updater, "Latest version:");
	l_ver_ytdlp.create(updater, "Latest yt-dlp version:");
	l_ver_ffmpeg.create(updater, "Latest ffmpeg version:");
	l_channel.create(updater, "yt-dlp release channel:");
	l_vertext.create(updater, "checking...");
	l_ytdlp_text.create(updater, "checking...");
	l_ffmpeg_text.create(updater, "checking...");
	btn_changes.create(updater, "Release notes");
	btn_update.create(updater, "Update");
	btn_update_ytdlp.create(updater, "Update yt-dlp");
	btn_update_ffmpeg.create(updater, "Update FFmpeg");
	cb_startup.create(updater, "Check at program startup and display any new version in the title bar");
	cb_chan_stable.create(updater, "Stable");
	cb_chan_nightly.create(updater, "Nightly");
	prog_updater.create(updater);
	prog_updater_misc.create(updater);
	separator.create(updater);

	updater["cb_startup"] << cb_startup;
	updater["l_ver"] << l_ver;
	updater["l_vertext"] << l_vertext;
	updater["btn_changes"] << btn_changes;
	updater["btn_update"] << btn_update;
	updater["prog"] << prog_updater;
	updater["separator"] << separator;
	updater["l_channel"] << l_channel;
	updater["cb_chan_stable"] << cb_chan_stable;
	updater["cb_chan_nightly"] << cb_chan_nightly;
	updater["l_ver_ytdlp"] << l_ver_ytdlp;
	updater["l_ver_ffmpeg"] << l_ver_ffmpeg;
	updater["l_ytdlp_text"] << l_ytdlp_text;
	updater["l_ffmpeg_text"] << l_ffmpeg_text;
	updater["btn_update_ytdlp"] << btn_update_ytdlp;
	updater["btn_update_ffmpeg"] << btn_update_ffmpeg;
	updater["prog_misc"] << prog_updater_misc;

	cb_startup.check(conf.get_releases_at_startup);
	cb_chan_stable.radio(true);
	cb_chan_nightly.radio(true);

	cb_chan_stable.tooltip("\"Stable\" releases are well tested and have no major bugs,\n"
		"but there's a relatively long time until one comes out.");
	cb_chan_nightly.tooltip("\"Nightly\" releases come out frequently and contain the\nlatest changes, but some features may be broken.");

	if(!rgp_chan.size())
	{
		rgp_chan.add(cb_chan_stable);
		rgp_chan.add(cb_chan_nightly);
	}

	btn_changes.typeface(nana::paint::font_info {"", 12, {800}});
	btn_changes.enabled(false);
	btn_changes.events().click([&] { dlg_changes(updater.parent()); });
	btn_update.typeface(nana::paint::font_info {"", 12, {800}});
	btn_update.enabled(false);
	btn_update_ytdlp.typeface(nana::paint::font_info {"", 12, {800}});
	btn_update_ytdlp.enabled(false);
	btn_update_ffmpeg.typeface(nana::paint::font_info {"", 12, {800}});
	btn_update_ffmpeg.enabled(false);
	l_ver_ytdlp.text_align(nana::align::right, nana::align_v::center);

	btn_update.events().click([&parent, this]
	{
		static fs::path arc_path;
		static bool btnffmpeg_state, btnytdlp_state;
		arc_path = fs::temp_directory_path() / (X64 ? "ytdlp-interface.7z" : "ytdlp-interface_x86.7z");
		if(btn_update.caption() == "Update")
		{
			btn_update.caption("Cancel");
			btn_update.cancel_mode(true);
			btnffmpeg_state = btn_update_ffmpeg.enabled();
			btnytdlp_state = btn_update_ytdlp.enabled();
			btn_update_ffmpeg.enabled(false);
			btn_update_ytdlp.enabled(false);
			thr = std::thread {[&parent, this]
			{
				updater_working = true;
				if(!X64 && releases[0]["assets"].size() < 2)
				{
					nana::msgbox mbox {*this, "ytdlp-interface update error"};
					mbox.icon(nana::msgbox::icon_error);
					(mbox << "The latest release on GitHub doesn't seem to contain a 32-bit build!")();
					btn_update.caption("Update");
					btn_update.cancel_mode(false);
					thr.detach();
					return;
				}
				unsigned arc_size {releases[0]["assets"][X64 ? 0 : 1]["size"]}, progval {0};
				std::string arc_url {releases[0]["assets"][X64 ? 0 : 1]["browser_download_url"]};
				prog_updater.amount(arc_size);
				prog_updater.value(0);
				auto cb_progress = [&, this](unsigned prog_chunk)
				{
					progval += prog_chunk;
					auto total {util::int_to_filesize(arc_size, false)},
						pct {std::to_string(progval / (arc_size / 100)) + '%'};
					prog_updater.caption("downloading archive: " + pct + " of " + total);
					prog_updater.value(progval);
				};
				auto dl_error {util::dl_inet_res(arc_url, arc_path, &updater_working, cb_progress)};

				if(updater_working)
				{
					if(dl_error.empty())
					{
						auto tempself {fs::temp_directory_path() / self_path.filename()};
						if(fs::exists(tempself))
							fs::remove(tempself);
						try
						{
							prog_updater.caption("Unpacking archive and restarting...");
							fs::copy_file(self_path, tempself);
							auto temp_7zlib {fs::temp_directory_path() / "7z.dll"};
							std::error_code ec;
							fs::remove(temp_7zlib, ec);
							fs::copy_file(appdir / "7z.dll", temp_7zlib);
							std::wstring params {L"update \"" + arc_path.wstring() + L"\" \"" + appdir.wstring() + L"\""};
							ShellExecuteW(NULL, L"runas", tempself.wstring().data(), params.data(), NULL, SW_SHOW);
							updater_working = false;
							parent.close();
							close();
						}
						catch(fs::filesystem_error const &e) {
							nana::msgbox mbox {parent, "File copy error"};
							mbox.icon(nana::msgbox::icon_error);
							(mbox << e.what())();
						}
					}
					else prog_updater.caption(dl_error);
					btn_update.caption("Update");
					btn_update.cancel_mode(false);
					btn_update_ffmpeg.enabled(btnffmpeg_state);
					btn_update_ytdlp.enabled(btnytdlp_state);
					updater_working = false;
					thr.detach();
				}
			}};
		}
		else
		{
			btn_update.caption("Update");
			btn_update.cancel_mode(false);
			prog_updater.caption("");
			prog_updater.value(0);
			btn_update_ffmpeg.enabled(btnffmpeg_state);
			btn_update_ytdlp.enabled(btnytdlp_state);
			updater_working = false;
			thr.detach();
		}
	});

	auto display_version = [this]
	{
		if(releases.empty())
		{
			l_vertext.error_mode(true);
			l_vertext.caption("failed to get from GitHub!");
			if(!inet_error.empty())
				l_vertext.tooltip(inet_error);
		}
		else
		{
			std::string tag_name {releases[0]["tag_name"]}, vertext;
			if(is_tag_a_new_version(tag_name))
			{
				vertext = tag_name + " (new version)";
				btn_update.enabled(true);
				std::string url_latest;
				if(X64) url_latest = releases[0]["assets"][0]["browser_download_url"];
				else if(releases[0]["assets"].size() > 1)
					url_latest = releases[0]["assets"][1]["browser_download_url"];
				btn_update.tooltip(url_latest);
			}
			else vertext = tag_name + " (current)";
			l_vertext.caption(vertext);
			btn_changes.enabled(true);
		}
	};

	auto display_version_ffmpeg = [this]
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
	};

	auto display_version_ytdlp = [this]
	{
		if(url_latest_ytdlp.empty())
		{
			l_ytdlp_text.error_mode(true);
			l_ytdlp_text.caption("unable to get from GitHub");
			if(!inet_error.empty())
				l_ytdlp_text.tooltip(inet_error);
		}
		else
		{
			bool not_present {conf.ytdlp_path.empty() && !fs::exists(appdir / ytdlp_fname)};
			if(ver_ytdlp_latest != ver_ytdlp)
			{
				l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current = " +
					(not_present ? "not present)  [click for changelog]" : ver_ytdlp.string() + ")  [click for changelog]"));
				btn_update_ytdlp.enabled(true);
				btn_update_ytdlp.tooltip(url_latest_ytdlp);
			}
			else l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current)  [click to see changelog]");
		}
	};

	auto update_misc = [this](bool ytdlp, fs::path target = "")
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
			thr = std::thread {[ytdlp, target, this]
			{
				updater_working = true;
				unsigned progval {0};
				const auto arc_size {ytdlp ? size_latest_ytdlp : size_latest_ffmpeg};
				const auto arc_url {ytdlp ? url_latest_ytdlp : url_latest_ffmpeg};
				const auto fname {url_latest_ytdlp.substr(url_latest_ytdlp.rfind('/') + 1)};
				prog_updater_misc.amount(arc_size);
				prog_updater_misc.value(0);
				auto cb_progress = [&, this](unsigned prog_chunk)
				{
					progval += prog_chunk;
					auto total {util::int_to_filesize(arc_size, false)},
						pct {std::to_string(progval / (arc_size / 100)) + '%'};
					prog_updater_misc.caption((ytdlp ? fname : arc_path.filename().string()) + "  :  " + pct + " of " + total);
					prog_updater_misc.value(progval);
				};
				std::string download_result;
				btn_update.enabled(false);
				if(ytdlp)
				{
					btn_update_ffmpeg.enabled(false);
					download_result = util::dl_inet_res(arc_url, target / fname, &updater_working, cb_progress);
				}
				else
				{
					btn_update_ytdlp.enabled(false);
					download_result = util::dl_inet_res(arc_url, arc_path, &updater_working, cb_progress);
				}

				if(updater_working)
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
								prog_updater_misc.caption("Extracting files to temporary folder...");
								auto error {util::extract_7z(arc_path, tempdir, true)};
								if(error.empty())
								{
									prog_updater_misc.caption(std::string {"Copying files to "} + (target == appdir ?
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
												prog_updater_misc.caption("Copy error: " + ec.message());
												break;
											}
										}
									}
									if(!ec)
									{
										ffmpeg_loc = target / "ffmpeg.exe";
										ver_ffmpeg = ver_ffmpeg_latest;
										btnffmpeg_state = false;
										prog_updater_misc.caption("FFmpeg update complete");
										l_ffmpeg_text.caption(ver_ffmpeg_latest.string() + "  (current)");
										btn_update_ffmpeg.tooltip("");
									}
								}
								else prog_updater_misc.caption("Error: " + error);
							}
							else prog_updater_misc.caption("!!! FAILED TO CREATE TEMPORARY DIRECTORY !!!");

							fs::remove(arc_path, ec);
							fs::remove_all(tempdir, ec);
						}
						else // yt-dlp downloaded
						{
							conf.ytdlp_path = target / fname;
							ver_ytdlp = ver_ytdlp_latest;
							btnytdlp_state = false;
							prog_updater_misc.caption("yt-dlp update complete");
							l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current)  [click to see changelog]");
							btn_update_ytdlp.tooltip("");
						}
					}
					else prog_updater_misc.caption(download_result);

					btn_update_ytdlp.caption(btntext_ytdlp);
					btn_update_ffmpeg.caption(btntext_ffmpeg);
					btn->cancel_mode(false);
					btn_update_ffmpeg.enabled(btnffmpeg_state);
					btn_update_ytdlp.enabled(btnytdlp_state);
					btn_update.enabled(btnupdate_state);
					updater_working = false;
					thr.detach();
				}
			}};
		}
		else
		{
			updater_working = false;
			btn_update_ytdlp.caption(btntext_ytdlp);
			btn_update_ffmpeg.caption(btntext_ffmpeg);
			btn->cancel_mode(false);
			btn_update_ffmpeg.enabled(btnffmpeg_state);
			btn_update_ytdlp.enabled(btnytdlp_state);
			btn_update.enabled(btnupdate_state);
			thr.detach();
		}
	};

	btn_update_ffmpeg.events().click([update_misc, &parent, this]
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
					nana::msgbox mbox {parent, "No place to put the ffmpeg files"};
					mbox.icon(nana::msgbox::icon_error);
					(mbox << "Neither the program folder, nor the specified yt-dlp folder can be written in. Run the program "
						"as administrator to fix that.")();
				}
				return;
			}
		}
		update_misc(false, use_ytdlp_folder ? conf.ytdlp_path.parent_path() : ffmpeg_loc.parent_path());
	});

	btn_update_ytdlp.events().click([update_misc, &parent, this]
	{
		if(conf.ytdlp_path.empty())
		{
			if(util::is_dir_writable(appdir))
			{
				update_misc(true, appdir);
			}
			else
			{
				nana::msgbox mbox {parent, "No place to put yt-dlp.exe"};
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
					nana::msgbox mbox {parent, "No place to put yt-dlp.exe"};
					mbox.icon(nana::msgbox::icon_error);
					(mbox << "Neither the program folder, nor the specified yt-dlp folder can be written in. Running the program "
						"as administrator should fix that.")();
				}
			}
		}
	});
	
	updater_t0.interval(std::chrono::milliseconds {100});
	updater_t0.elapse([display_version, this]
	{
		if(!thr_releases.joinable())
		{
			display_version();
			updater_t0.stop();
		}
	});

	updater_t1.interval(std::chrono::milliseconds {300});
	updater_t1.elapse([display_version_ffmpeg, this]
	{
		if(!thr_releases_ffmpeg.joinable())
		{
			display_version_ffmpeg();
			updater_t1.stop();
		}
	});

	updater_t2.interval(std::chrono::milliseconds {300});
	updater_t2.elapse([display_version_ytdlp, this]
	{
		if(!thr_releases_ytdlp.joinable())
		{
			if(!url_latest_ytdlp_relnotes.empty())
			{
				nana::api::effects_edge_nimbus(l_ytdlp_text, nana::effects::edge_nimbus::over);
				l_ytdlp_text.tooltip("Click to view release notes in web browser.");
				l_ytdlp_text.events().click.clear();
				l_ytdlp_text.events().click([this]
				{
					nana::system::open_url(url_latest_ytdlp_relnotes);
				});
			}
			display_version_ytdlp();
			if(conf.ytdlp_path.filename().string() != "ytdl-patched-red.exe")
			{
				cb_chan_stable.enabled(true);
				cb_chan_nightly.enabled(true);
			}
			btn_update_ytdlp.enable(ver_ytdlp_latest != ver_ytdlp);
			if(!btn_update_ytdlp.enabled())
				btn_update_ytdlp.tooltip("");
			updater_t2.stop();
		}
	});

	updater.events().expose([display_version, display_version_ffmpeg, display_version_ytdlp, this](const nana::arg_expose &arg)
	{
		if(arg.exposed && updater_init)
		{
			updater_init = false;
			l_ffmpeg_text.caption("checking...");
			l_vertext.caption("checking...");

			if(prog_updater_misc.value() == prog_updater_misc.amount())
			{
				prog_updater_misc.value(0);
				prog_updater_misc.caption("");
			}

			if(conf.get_releases_at_startup)
			{
				if(thr_releases.joinable())
					updater_t0.start();
				else display_version();
			}
			else if(releases.empty())
			{
				get_releases();
				updater_t0.start();
			}
			else display_version();

			if(url_latest_ffmpeg.empty())
			{
				get_latest_ffmpeg();
				updater_t1.start();
			}
			else display_version_ffmpeg();

			if(!url_latest_ytdlp.empty())
			{
				const auto fname {url_latest_ytdlp.substr(url_latest_ytdlp.rfind('/') + 1)};
				if(fname != conf.ytdlp_path.filename().string())
				{
					get_latest_ytdlp();
					updater_t2.start();
				}
			}
			else if(!thr_releases_ytdlp.joinable())
			{
				cb_chan_stable.enabled(false);
				cb_chan_nightly.enabled(false);
				l_ytdlp_text.caption("checking...");
				nana::api::refresh_window(updater);
				get_latest_ytdlp();
				updater_t2.start();
			}

			if(!url_latest_ytdlp_relnotes.empty() && fs::path{url_latest_ytdlp}.filename() == conf.ytdlp_path.filename())
			{
				nana::api::effects_edge_nimbus(l_ytdlp_text, nana::effects::edge_nimbus::over);
				l_ytdlp_text.tooltip("Click to view release notes in web browser.");
				l_ytdlp_text.events().click([this]
				{
					nana::system::open_url(url_latest_ytdlp_relnotes);
				});
				display_version_ytdlp();
			}

			if(conf.ytdlp_nightly)
				cb_chan_nightly.check(true);
			else cb_chan_stable.check(true);

			if(conf.ytdlp_path.filename().string() == "ytdl-patched-red.exe")
			{
				cb_chan_stable.enabled(false);
				cb_chan_nightly.enabled(false);
			}
		}
	});

	rgp_chan.on_checked([&](const nana::arg_checkbox &arg)
	{
		if(arg.widget->checked())
		{
			if(thr_releases_ytdlp.joinable())
				thr_releases_ytdlp.detach();
			cb_chan_stable.enabled(false);
			cb_chan_nightly.enabled(false);
			l_ytdlp_text.caption("checking...");
			conf.ytdlp_nightly = arg.widget->handle() == cb_chan_nightly;
			get_latest_ytdlp();
			updater_t2.start();
		}
	});
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


GUI::settings_t GUI::conf;


void GUI::gui_bottom::show_btncopy(bool show)
{
	plc.field_display("btncopy", show);
	plc.field_display("btncopy_spacer", show);
	plc.collocate();
}


void GUI::gui_bottom::show_btnfmt(bool show)
{
	plc.field_display("btn_ytfmtlist", show);
	plc.field_display("ytfm_spacer", show);
	plc.collocate();
	pgui->get_place().collocate();
	nana::api::update_window(*pgui);
}

fs::path GUI::gui_bottom::file_path()
{
	if(!printed_path.empty())
	{
		if(printed_path.extension().string() != "NA")
		{
			if(fs::exists(printed_path))
				return printed_path;
		}
		if(!merger_path.empty())
		{
			printed_path.replace_extension(merger_path.extension());
			if(fs::exists(printed_path))
				return printed_path;
			if(!download_path.empty())
			{
				printed_path.replace_extension(download_path.extension());
				if(fs::exists(printed_path))
					return printed_path;
			}
			if(vidinfo_contains("ext"))
			{
				std::string ext {vidinfo["ext"]};
				printed_path.replace_extension(ext);
				if(fs::exists(printed_path))
					return printed_path;
			}
		}
	}

	if(!merger_path.empty() && fs::exists(merger_path))
		return merger_path;
	if(!download_path.empty() && fs::exists(download_path))
		return download_path;

	fs::path file;
	if(vidinfo_contains("filename"))
		file = fs::u8path(std::string {vidinfo["filename"]});
	
	if(!file.empty())
	{
		file = outpath / file;
		if(!fs::exists(file))
		{
			if(cbargs.checked() && com_args.caption().find("-f ") != -1)
			{
				auto args {com_args.caption()};
				auto pos {args.find("-f ")};
				if(pos != -1)
				{
					pos += 1;
					while(pos < args.size() && isspace(args[++pos]));
					std::string num;
					while(pos < args.size())
					{
						char c {args[pos++]};
						if(isdigit(c))
							num += c;
						else break;
					}
					if(!num.empty())
					{
						for(auto &fmt : vidinfo["formats"])
						{
							std::string format_id {fmt["format_id"]}, ext {fmt["ext"]};
							if(format_id == num)
							{
								file.replace_extension(ext);
								break;
							}
						}
						if(!fs::exists(file))
							file.clear();
					}
					else file.clear();
				}
				else file.clear();
			}
			else if(cbmp3.checked())
			{
				file.replace_extension("mp3");
				if(!fs::exists(file))
					file.clear();
			}
			else if(use_strfmt)
			{
				using nana::to_utf8;
				std::string ext1, ext2, vcodec;
				for(auto &fmt : vidinfo["formats"])
				{
					std::string format_id {fmt["format_id"]}, ext {fmt["ext"]};
					if(ext1.empty() && format_id == to_utf8(fmt1))
					{
						ext1 = to_utf8(ext);
						if(fmt.contains("vcodec") && fmt["vcodec"] != "none")
							vcodec = fmt["vcodec"];
						if(fmt2.empty()) break;
					}
					else if(!fmt2.empty() && format_id == to_utf8(fmt2))
					{
						ext2 = to_utf8(ext);
						if(fmt.contains("vcodec") && fmt["vcodec"] != "none")
							vcodec = fmt["vcodec"];
						if(!ext1.empty()) break;
					}
				}
				if(ext2.empty())
					file.replace_extension(ext1);
				else
				{
					std::string ext;
					if(ext2 == "webm")
						ext = (ext1 == "webm" || ext1 == "weba") ? "webm" : "mkv";
					else if(ext1 == "webm" || ext1 == "weba")
						ext = vcodec.starts_with("av01") ? "webm" : "mkv";
					else ext = "mp4";
					file.replace_extension(ext);
				}
				if(!fs::exists(file))
					file.clear();
			}
			else file.clear();
		}
	}
	return file;
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
	expcol.create(*this);
	btn_ytfmtlist.create(*this, "Select formats");
	btndl.create(*this, "Start download");
	btncopy.create(*this, "Apply options to all queue items");
	btnerase.create(gpopt);
	btnq.create(*this, gui.queue_panel.visible() ? "Show output" : "Show queue");
	l_out.create(gpopt, "Download folder:");
	l_out.text_align(nana::align::left, nana::align_v::center);
	l_rate.create(gpopt, "Download rate limit:");
	l_rate.text_align(nana::align::left, nana::align_v::center);
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
			<weight=20 <> <expcol weight=20>>
			<gpopt weight=220> <gpopt_spacer weight=20>
			<weight=35 <> <btn_ytfmtlist weight=190> <ytfmt_spacer weight=20> <btncopy weight=328> <btncopy_spacer weight=20> 
				<btnq weight=180> <weight=20> <btndl weight=200> <>>
		)");
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

	if(gui.cnlang) gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=140> <l_outpath> > <weight=20>
			<weight=25 
				<l_rate weight=163> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbchaps weight=153> <> <cbsplit weight=143> <> <cbkeyframes weight=213>
			> <weight=20>
			<weight=25 <cbtime weight=333> <> <cbthumb weight=168> <> <cbsubs weight=153> <> <cbmp3 weight=198>>
			<weight=20> <weight=24 <cbargs weight=178> <weight=15> <com_args> <weight=10> <btnerase weight=24>>
		)");

	else gpopt.div(R"(vert margin=20
			<weight=25 <l_out weight=122> <weight=15> <l_outpath> > <weight=20>
			<weight=25 
				<l_rate weight=144> <weight=15> <tbrate weight=45> <weight=15> <com_rate weight=55> 
				<> <cbchaps weight=141> <> <cbsplit weight=124> <> <cbkeyframes weight=194>
			> <weight=20>
			<weight=25 <cbtime weight=304> <> <cbthumb weight=152> <> <cbsubs weight=140> <> <cbmp3 weight=181>>
			<weight=20> <weight=24 <cbargs weight=164> <weight=15> <com_args> <weight=10> <btnerase weight=24>>
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
		btnerase.image(arr_erase22_ico, sizeof arr_erase22_ico);
		btnerase.image_disabled(arr_erase22_disabled_ico, sizeof arr_erase22_disabled_ico);
	}
	else
	{
		btnerase.image(arr_erase16_ico, sizeof arr_erase16_ico);
		btnerase.image_disabled(arr_erase16_disabled_ico, sizeof arr_erase16_disabled_ico);
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

	expcol.events().click([&, this]
	{
		auto wdsz {api::window_size(gui)};
		const auto px {(nana::API::screen_dpi(true) >= 144) * gui.queue_panel.visible()};
		gui.change_field_attr(gui.get_place(), "Bottom", "weight", 325 - 240 * expcol.collapsed() - 27 * gui.queue_panel.visible() - px);
		for(auto &bottom : gui.bottoms)
		{
			auto &bot {*bottom.second};
			if(expcol.collapsed())
			{
				if(&bot == this)
				{
					if(!gui.no_draw_freeze)
					{
						gui.conf.gpopt_hidden = true;
						SendMessageA(gui.hwnd, WM_SETREDRAW, FALSE, 0);
					}
					auto dh {gui.dpi_transform(240)};
					wdsz.height -= dh;
					gui.minh -= dh;
					api::window_size(gui, wdsz);
				}
				else bot.expcol.operate(true);
				bot.plc.field_display("gpopt", false);
				bot.plc.field_display("gpopt_spacer", false);				
				bot.plc.collocate();
				if(&bot == this && !gui.no_draw_freeze)
				{
					SendMessageA(gui.hwnd, WM_SETREDRAW, TRUE, 0);
					nana::api::refresh_window(gui);
				}
			}
			else
			{
				if(&bot == this)
				{
					if(!gui.no_draw_freeze)
					{
						gui.conf.gpopt_hidden = false;
						SendMessageA(gui.hwnd, WM_SETREDRAW, FALSE, 0);
					}
					auto dh {gui.dpi_transform(240)};
					wdsz.height += dh;
					gui.minh += dh;
					api::window_size(gui, wdsz);
				}
				else bot.expcol.operate(false);
				bot.plc.field_display("gpopt", true);
				bot.plc.field_display("gpopt_spacer", true);
				bot.plc.collocate();
				if(&bot == this && !gui.no_draw_freeze)
				{
					SendMessageA(gui.hwnd, WM_SETREDRAW, TRUE, 0);
					nana::api::refresh_window(gui);
				}
			}
		}
	});

	cbsplit.tooltip("Split into multiple files based on internal chapters. (<bold>--split-chapters)</>");
	cbkeyframes.tooltip("Force keyframes around the chapters before\nremoving/splitting them. Requires a\n"
						"reencode and thus is very slow, but the\nresulting video may have fewer artifacts\n"
						"around the cuts. (<bold>--force-keyframes-at-cuts</>)");
	cbtime.tooltip("Do not use the Last-modified header to set the file modification time (<bold>--no-mtime</>)");
	cbsubs.tooltip("Embed subtitles in the video (only for mp4, webm and mkv videos) (<bold>--embed-subs</>)");
	cbchaps.tooltip("Add chapter markers to the video file (<bold>--embed-chapters</>)");
	cbthumb.tooltip("Embed thumbnail in the video as cover art (<bold>--embed-thumbnail</>)");
	cbmp3.tooltip("Convert the source audio to MPEG Layer 3 format and save it to an .mp3 file.\n"
					"The video is discarded if present, so it's preferable to download an audio-only\n"
					"format if one is available. (<bold>-x --audio-format mp3</>)\n\n"
					"To download the best audio-only format available, use the custom\nargument <bold>-f ba</>");
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

	btndl.events().click([&gui, this] { gui.on_btn_dl(url); });

	btndl.events().key_press([&gui, this](const arg_keyboard &arg)
	{
		if(arg.key == '\r')
			gui.on_btn_dl(url);
	});

	btn_ytfmtlist.events().click([&gui, this]
	{
		gui.dlg_formats();
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
			nana::folderbox fb {*this, conf.open_dialog_origin ? outpath : gui.appdir};
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
					m.append(to_utf8(clip_text(path, gui.dpi_transform(250))), [&, this](menu::item_proxy &)
					{
						outpath = conf.outpath = path;
						l_outpath.update_caption();
						conf.outpaths.insert(outpath);
						if(conf.common_dl_options)
							gui.bottoms.propagate_misc_options(*this);
					});
				}
			}
			m.max_pixels(gui.dpi_transform(280));
			m.item_pixels(gui.dpi_transform(24));
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

	if(conf.common_dl_options && gui.bottoms.size())
		com_args.caption(gui.bottoms.at(0).com_args.caption());

	com_args.events().mouse_up([this](const arg_mouse &arg)
	{
		if(arg.button == mouse::right_button)
		{
			using namespace nana;
			::widgets::Menu m;
			m.item_pixels(util::scale(24));

			auto cliptext {util::get_clipboard_text()};

			m.append("Paste", [&, this](menu::item_proxy)
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