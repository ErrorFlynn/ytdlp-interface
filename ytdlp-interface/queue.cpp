#include "gui.hpp"
#include <codecvt>
#include <nana/gui/filebox.hpp>


void GUI::queue_make_listbox()
{
	using namespace nana;
	using namespace util;
	using ::widgets::theme;

	lbq.sortable(false);
	lbq.hilight_checked(true);
	lbq.column_movable(false);
	lbq.typeface(paint::font_info {"Calibri", 12});
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

	lbq.events().resized([this](const arg_resized &arg) { adjust_lbq_headers(); });

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
			if(arg.is_left_button() && last_selected)
			{
				if(arg.pos.y > dpi_scale(21)) // if below header
					last_selected = nullptr;
			}
		}
		else
		{
			auto hovitem {lbq.at(hovered)};
			auto url {hovitem.value<lbqval_t>().url};
			if(arg.button == mouse::left_button && !arg.ctrl && !arg.shift && url != bottoms.visible())
			{
				bottoms.show(url);
				outbox.current(url);
				qurl = url;
				l_url.update_caption();
			}

			if(lbq.selected().size() == 1)
			{
				lbq_can_drag = true;
				if(last_selected && last_selected->value<lbqval_t>() == url)
				{
					if(!hovitem.selected())
						hovitem.select(true);
				}
			}
			else lbq_can_drag = false;
		}
	});

	lbq.events().selected([this](const arg_listbox &arg)
	{
		static auto item {arg.item};
		auto sel {lbq.selected()};
		if(arg.item.selected())
		{
			if(arg.item.pos() == sel.front())
			{
				item = arg.item;
				last_selected = &item;
				const auto &url {arg.item.value<lbqval_t>().url};
				if(url != bottoms.visible())
				{
					bottoms.show(url);
					outbox.current(url);
					qurl = url;
					l_url.update_caption();
				}
			}
		}
		else if(last_selected == nullptr)
			arg.item.select(true);
	});

	lbq.set_deselect([&](mouse btn) { return !(btn != mouse::left_button); });

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

		dragging_color = ::widgets::theme::is_dark() ? ::widgets::theme::path_link_fg : colors::green;
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
				bool selcheck {lb.at(selected.item).checked()};

				auto hovitem {lb.at(hovered.item)};

				// moving item upward, pushing list downward below insertion point
				if(hovered.item < selected.item)
				{
					for(auto n {selected.item}; n > hovered.item; n--)
					{
						auto item {lb.at(n)}, prev_item {lb.at(n - 1)};
						const auto &val {prev_item.value<lbqval_t>()};
						item.value(val);
						item.text(1, prev_item.text(1));
						item.text(2, prev_item.text(2));
						item.text(3, prev_item.text(3));
						item.text(4, prev_item.text(4));
						item.text(5, prev_item.text(5));
						item.text(6, prev_item.text(6));
						item.text(7, prev_item.text(7));
						item.check(prev_item.checked());
						if(!conf.common_dl_options)
							bottoms.at(val).gpopt.caption("Download options for queue item #" + item.text(0));
						item.fgcolor(item.checked() ? theme::list_check_highlight_fg : lbq.fgcolor());
						item.select(false);
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
						auto item {lb.at(n)}, next_item {lb.at(n + 1)};
						const auto &val {next_item.value<lbqval_t>()};
						item.value(val);
						item.text(1, next_item.text(1));
						item.text(2, next_item.text(2));
						item.text(3, next_item.text(3));
						item.text(4, next_item.text(4));
						item.text(5, next_item.text(5));
						item.text(6, next_item.text(6));
						item.text(7, next_item.text(7));
						item.check(next_item.checked());
						if(!conf.common_dl_options)
							bottoms.at(val).gpopt.caption("Download options for queue item #" + item.text(0));
						item.fgcolor(item.checked() ? theme::list_check_highlight_fg : lbq.fgcolor());
						item.select(false);
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
				hovitem.check(selcheck);
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
		selitem.fgcolor(selitem.checked() ? theme::list_check_highlight_fg : lbq.fgcolor());
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
			auto url {queue_pop_menu(arg.pos.x, arg.pos.y)};
			if(!url.empty())
			{
				lbq.auto_draw(false);
				queue_remove_item(url);
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


std::wstring GUI::queue_pop_menu(int x, int y)
{
	using namespace nana;

	std::wstring url_of_item_to_delete;
	::widgets::Menu m;
	m.item_pixels(24);
	auto sel {lbq.selected()};
	if(!sel.empty() && !thr_menu.joinable())
	{
		if(sel.size() == 1)
		{
			auto item {lbq.at(sel.front())};
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
				else if(text.find("stopped") == -1 && text.find("queued") == -1 && text.find("error") == -1 && text.find("skip") == -1)
					stoppable.push_back(item);
				else startable.push_back(item);
			}

			if(vidsel_item.m)
				vidsel_item.m = &m;

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

			m.append("Open folder of " + item_name, [&, file](menu::item_proxy)
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

			if(!file.empty()) m.append("Open file of " + item_name, [&, file](menu::item_proxy)
			{
				ShellExecuteW(NULL, L"open", file.wstring().data(), NULL, NULL, SW_NORMAL);
			});

			if(!bottom.started() && !bottom.is_ytplaylist && !bottom.is_bcplaylist && !bottom.is_ytchan && 
				!bottom.is_yttab && !bottom.is_bcchan)
			{
				m.append("Set file name of " + item_name, [&](menu::item_proxy)
				{
					bottom.browse_for_filename();
				});
			}

			if(!bottom.outfile.empty())
			{
				m.append("Clear custom filename", [&](menu::item_proxy)
				{
					bottom.outfile.clear();
					bottom.l_outpath.tooltip("");
				});
			}

			if(item.text(3) != "error" || !bottom.vidinfo.empty())
			{
				m.append_splitter();
				if(bottom.is_ytplaylist || bottom.is_bcplaylist || bottom.is_scplaylist)
				{
					auto count {bottom.playlist_selection.size()};
					std::string item_text {(bottom.is_ytplaylist ? "Select videos (" : "Select songs (") + (count && !bottom.playlist_info.empty() ?
						std::to_string(bottom.playlist_selected()) + '/' + std::to_string(count) + ")" : "getting data...)")};
					auto item = m.append(item_text, [this](menu::item_proxy)
					{
						fm_playlist();
					}).enabled(count && !bottom.playlist_info.empty());
					auto playlist_size {bottom.playlist_info["entries"].size()};
					if(!count || bottom.playlist_info.empty())
					{
						item.enabled(false);
						vidsel_item = {&m, item.index()};
					}
				}
				else if(bottom.is_yttab)
				{
					m.append("Treat as playlist", [&, url](menu::item_proxy)
					{
						bottom.is_ytplaylist = true;
						add_url(url, true);
					}).enabled(item.text(2) != "...");
				}
				else if(!bottom.is_ytchan && !bottom.is_bcchan && !is_live && item.text(2).find("[live event scheduled to begin in") != 0)
				{
					m.append("Download sections", [this](menu::item_proxy)
					{
						fm_sections();
					});
				}

				if(item.text(2) == "..." && !bottom.is_ytchan)
					m.append("Select formats", [this](menu::item_proxy) { fm_formats(); }).enabled(false);
				else if(bottom.btnfmt_visible())
				{
					m.append("Select formats", [this](menu::item_proxy)
					{
						fm_formats();
					});
				}

				auto jitem = m.append("View JSON data", [this](menu::item_proxy)
				{
					fm_json();
				});
				jitem.enabled(item.text(2) != "...");
				if(!jitem.enabled())
					vidsel_item = {&m, vidsel_item.pos};				
			}

			if(item.text(2) != "...")
			{
				m.append("Refresh (reacquire data)", [&, url](menu::item_proxy ip)
				{
					vidsel_item = {&m, vidsel_item.pos};
					bottom.show_btnfmt(false);
					add_url(url, true, false);
				});
			}

			if(std::find(startable.begin(), startable.end(), item) != startable.end())
				m.append("Do not download", [&](menu::item_proxy)
				{
					item.check(!item.checked());
					if(item.checked())
						item.text(3, "skip");
					else item.text(3, "queued");
					lbq.refresh_theme();
				}).checked(item.checked());

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
								queue_remove_item(url, false);
						}
						autostart_next_item = true;
						auto sel {lbq.selected()};
						if(sel.size() > 1)
							for(auto n {1}; n < sel.size(); n++)
								lbq.at(sel[n]).select(false);
						lbq.auto_draw(true);
						queue_save();
					});
				}
				if(!startable.empty())
				{
					m.append("Start all", [&](menu::item_proxy)
					{
						thr_menu = std::thread([&]
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
					m.append("Stop all", [&](menu::item_proxy)
					{
						thr_menu = std::thread([&]
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
					m.append("Remove all", [this](menu::item_proxy)
					{
						queue_remove_all();
					});
				}
			}
		}
		else
		{
			auto sel {lbq.selected()};
			std::vector<std::wstring> startables, stoppables;
			std::vector<drawerbase::listbox::item_proxy> startables_not_done, skippers;

			for(auto &ip : sel)
			{
				auto item {lbq.at(ip)};
				auto url {item.value<lbqval_t>().url};
				if(bottoms.at(url).started())
					stoppables.push_back(url);
				else if(item.checked())
					skippers.push_back(item);
				else 
				{
					startables.push_back(url);
					if(item.text(3) != "done")
						startables_not_done.push_back(item);
				}
			}

			std::string cmdtext {"Start/stop selected"};
			if(startables.empty())
				cmdtext = "Stop selected";
			if(stoppables.empty())
				cmdtext = "Start selected";

			m.append(cmdtext, [startables, stoppables, this](menu::item_proxy)
			{
				thr_menu = std::thread([this, startables, stoppables]
				{
					menu_working = true;
					autostart_next_item = false;
					for(auto url : stoppables)
					{
						if(!menu_working) break;
						process_queue_item(url);
					}
					for(auto url : startables)
					{
						if(!menu_working) break;
						process_queue_item(url);
					}
					if(menu_working)
						api::refresh_window(bottoms.current().btndl);
					autostart_next_item = true;
					if(thr_menu.joinable())
						thr_menu.detach();
				});
			});

			m.append("Remove selected", [this](menu::item_proxy)
			{
				queue_remove_selected();
			});

			m.append("Refresh selected", [&, sel](menu::item_proxy)
			{
				vidsel_item = {&m, sel.front().item};
				for(auto &el : sel)
				{
					auto item {lbq.at(el)};
					auto url {item.value<lbqval_t>().url};
					bottoms.at(url).show_btnfmt(false);
					add_url(url, true);
				}
			});

			if(!skippers.empty() || !startables_not_done.empty())
				m.append_splitter();

			if(!startables_not_done.empty())
			{
				m.append("Do not download", [startables_not_done, this](menu::item_proxy)
				{
					lbq.auto_draw(false);
					for(auto item : startables_not_done)
					{
						item.text(3, "skip");
						item.check(true);
					}
					lbq.refresh_theme();
				});
			}

			if(!skippers.empty())
			{
				m.append("Make downloadable", [skippers, this](menu::item_proxy)
				{
					lbq.auto_draw(false);
					for(auto item : skippers)
					{
						item.text(3, "queued");
						item.check(false);
					}
					lbq.refresh_theme();
				});
			}

			if(!skippers.empty() && !startables_not_done.empty())
			{
				m.append("Toggle download ability", [startables_not_done, skippers, this](menu::item_proxy)
				{
					lbq.auto_draw(false);
					for(auto item : startables_not_done)
					{
						item.text(3, "skip");
						item.check(true);
					}
					for(auto item : skippers)
					{
						item.text(3, "queued");
						item.check(false);
					}
					lbq.refresh_theme();
				});
			}

			m.append("Refresh (reacquire data)", [sel, this](menu::item_proxy)
			{
				for(auto ip : sel)
				{
					auto url {lbq.at(ip).value<lbqval_t>().url};
					if(!bottoms.at(url).started())
						add_url(url, true, false);
				}
				api::refresh_window(lbq);
			});
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

	m2->append("Favicon", [&](menu::item_proxy)
	{
		conf.col_site_icon = !conf.col_site_icon;
		update_inline_widgets();
	}).checked(conf.col_site_icon);

	m2->append("Text", [&](menu::item_proxy)
	{
		conf.col_site_text = !conf.col_site_text;
		update_inline_widgets();
	}).checked(conf.col_site_text);

	m.append_splitter();
	auto m3 {m.create_sub_menu(m.append("When finished...").index())};

	auto warning_msg = [this](std::string action)
	{
		::widgets::msgbox mbox {*this, title};
		mbox << "The process does not seem to have shutdown privilege, possibly because it's running in a non-administrator "
			"user session. The program will attempt to " << action << ", but it might fail.";
		mbox.icon(MB_ICONEXCLAMATION)();
	};

	m3->append("Shutdown", [&](menu::item_proxy)
	{
		pwr_shutdown = !pwr_shutdown;
		pwr_hibernate = false;
		pwr_sleep = false;
		close_when_finished = false;
		if(pwr_shutdown && !pwr_can_shutdown) warning_msg("shut down the system");
	}).checked(pwr_shutdown);

	m3->append("Hibernate", [&](menu::item_proxy)
	{
		pwr_hibernate = !pwr_hibernate;
		pwr_sleep = false;
		pwr_shutdown = false;
		close_when_finished = false;
		if(pwr_hibernate && !pwr_can_shutdown) warning_msg("initiate hibernation");
	}).checked(pwr_hibernate).enabled(util::pwr_can_hibernate());

	m3->append("Sleep", [&](menu::item_proxy)
	{
		pwr_sleep = !pwr_sleep;
		pwr_hibernate = false;
		pwr_shutdown = false;
		close_when_finished = false;
		if(pwr_sleep && !pwr_can_shutdown) warning_msg("put the system to sleep");
	}).checked(pwr_sleep);

	m3->append("Exit", [&](menu::item_proxy)
	{
		close_when_finished = !close_when_finished;
		pwr_hibernate = false;
		pwr_shutdown = false;
		pwr_sleep = false;
	}).checked(close_when_finished);


	m.popup_await(lbq, x, y);
	vidsel_item.m = nullptr;
	return url_of_item_to_delete;
}


void GUI::queue_remove_all()
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
	queue_save();
}


void GUI::queue_remove_selected()
{
	using namespace nana;

	std::wstring val;
	auto sel {lbq.selected()};
	if(sel.back().item < lbq.at(0).size() - 1)
		val = lbq.at(drawerbase::listbox::index_pair {0, sel.back().item + 1}).value<lbqval_t>().url;
	else
	{
		for(auto n {sel.back()}; n.item != npos; n.item--)
			if(!lbq.at(n).selected())
			{
				val = lbq.at(n).value<lbqval_t>().url;
				break;
			}
	}
	menu_working = true;
	bottoms.show(to_wstring(val));
	qurl = to_wstring(val);
	l_url.update_caption();

	std::vector<std::wstring> sel_urls;

	for(auto ip : sel)
		sel_urls.push_back(lbq.at(ip).value<lbqval_t>().url);

	lbq.auto_draw(false);

	for(auto url : sel_urls)
	{
		lbq.erase(lbq.item_from_value(url));
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

	if(!val.empty())
	{
		auto item {lbq.item_from_value(val)};
		if(!item.empty())
			item.select(true);
	}

	adjust_lbq_headers();
	lbq.auto_draw(true);
	queue_save();
}


void GUI::make_columns_menu(nana::menu *m)
{
	using namespace nana;

	if(m)
	{
		m->append("Format", [this](menu::item_proxy ip)
		{
			conf.col_format = !conf.col_format;
			ip.checked(conf.col_format);
			lbq.auto_draw(false);
			lbq.column_at(4).visible(conf.col_format);
			adjust_lbq_headers();
			lbq.auto_draw(true);
		}).checked(conf.col_format);

		m->append("Format note", [this](menu::item_proxy ip)
		{
			conf.col_format_note = !conf.col_format_note;
			ip.checked(conf.col_format_note);
			lbq.auto_draw(false);
			lbq.column_at(5).visible(conf.col_format_note);
			adjust_lbq_headers();
			lbq.auto_draw(true);
		}).checked(conf.col_format_note);

		m->append("Ext", [this](menu::item_proxy ip)
		{
			conf.col_ext = !conf.col_ext;
			ip.checked(conf.col_ext);
			lbq.auto_draw(false);
			lbq.column_at(6).visible(conf.col_ext);
			adjust_lbq_headers();
			lbq.auto_draw(true);
		}).checked(conf.col_ext);

		m->append("Filesize", [this](menu::item_proxy ip)
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


bool GUI::queue_save()
{
	static std::mutex mtx;
	std::lock_guard<std::mutex> lock {mtx};
	if(fn_write_conf && lbq.at(0).size())
	{
		conf.unfinished_queue_items.clear();
		for(auto item : lbq.at(0))
		{
			auto text {item.text(3)};
			if(text != "done" && (text != "error" || text == "error" && conf.cb_save_errors))
				conf.unfinished_queue_items.push_back(nana::to_utf8(item.value<lbqval_t>().url));
		}
		return fn_write_conf();
	}
	return true;
}


void GUI::queue_remove_item(std::wstring url, bool save)
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
		if(save) queue_save();
	}
}