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
	lbq.column_at(0).text_align(align::center);
	lbq.at(0).inline_factory(1, pat::make_factory<inline_widget>());

	lbq.events().resized([this](const arg_resized &arg) { adjust_lbq_headers(); });

	lbq.events().dbl_click([this](const arg_mouse &arg)
	{
		if(arg.is_left_button())
		{
			const auto hovered {lbq.cast(point(arg.pos.x, arg.pos.y))};
			if(!hovered.is_category())
			{
				show_output();
				lbq_can_drag = false;
			}
		}
	});

	lbq.events().mouse_down([this](const arg_mouse &arg)
	{
		const auto hovered {lbq.cast(point(arg.pos.x, arg.pos.y))};

		if(!hovered.is_category())
		{
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
				if(arg.button == mouse::left_button && !arg.ctrl && !arg.shift && url != qurl)
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
				outbox.current(url);
				if(url != qurl)
				{
					bottoms.show(url);
					qurl = url;
					l_url.update_caption();
				}
			}
		}
		else if(last_selected == nullptr)
			arg.item.select(true);
	});

	lbq.set_deselect([&](mouse btn) { return btn == mouse::left_button; });

	static color dragging_color {colors::green};
	static timer scroll_down_timer, scroll_up_timer;
	static bool dragging {false};

	lbq.events().mouse_move([this](const arg_mouse &arg)
	{
		const auto hovered {lbq.cast({arg.pos.x, arg.pos.y})};
		if(!arg.is_left_button() || !lbq_can_drag || hovered.cat != last_selected->pos().cat) return;
		dragging = true;

		const auto lines {3};
		bool autoscroll {true};
		const auto delay {std::chrono::milliseconds{25}};

		dragging_color = ::widgets::theme::is_dark() ? ::widgets::theme::path_link_fg : colors::green;
		lbq.auto_draw(false);
		auto lb {hovered.item != npos ? lbq.at(hovered.cat) : lbq.at(0)};
		auto selection {lbq.selected()};
		if(selection.empty())
		{
			if(hovered.item != npos)
			{
				lbq.at(hovered).select(true);
				selection = lbq.selected();
			}
		}
		if(selection.size() == 1)
		{
			const auto selected {selection[0]};
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
							gpopt.caption("Download options for queue item #" + item.text(0));
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
							gpopt.caption("Download options for queue item #" + item.text(0));
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
					gpopt.caption("Download options for queue item #" + lb.at(lbq.selected().front().item).text(0));
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
	if(!sel.empty() && !thr_menu_start_stop.joinable())
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
			for(auto &item : lbq.at(item.pos().cat))
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

			auto verb {btndl.caption().substr(0, 5)};
			if(verb.back() == ' ')
				verb.pop_back();
			if(item.text(3).find("stopped") != -1)
				verb = "Resume";
			m.append(verb + " " + item_name, [&, url, this](menu::item_proxy)
			{
				on_btn_dl(url);
				api::refresh_window(btndl);
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

			if(!bottom.started && !bottom.is_ytplaylist && !bottom.is_bcplaylist && !bottom.is_ytchan && 
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
					l_outpath.tooltip("");
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

					auto mitem = m.append(item_text, [this](menu::item_proxy)
					{
						fm_playlist();
					}).enabled(count && !bottom.playlist_info.empty());

					if(!count || bottom.playlist_info.empty())
					{
						mitem.enabled(false);
						vidsel_item = {&m, mitem.index()};
					}
					else m.append("Split playlist (add videos to queue)", [&, url](menu::item_proxy)
					{
						url_of_item_to_delete = url;
						lbq.auto_draw(false);
						auto cat {lbq.append(item.text(2))};
						cat.inline_factory(1, nana::pat::make_factory<inline_widget>());
						for(unsigned n {0}; n < count; n++)
						{
							if(bottom.playlist_selection[n])
							{
								auto url {to_wstring(bottom.playlist_info["entries"][n]["url"].get<std::string>())};
								add_url(url, false, false, cat.position());
							}
						}
						if(cat.size())
						{
							cat.at(0).select(true, true);
							if(cat.size() > 1)
							{
								cat.at(1).select(true);
								cat.at(1).select(false);
							}
							else last_selected = nullptr;
						}
						lbq.auto_draw(true);
					});
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
					show_btnfmt(false);
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
					taskbar_overall_progress();
				}).checked(item.checked());

			if(lbq.at(item.pos().cat).size() > 1)
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
							if(item != lbq.empty_item)
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
						thr_menu_start_stop = std::thread([&]
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
							api::refresh_window(btndl);
						autostart_next_item = true;
						if(thr_menu_start_stop.joinable())
							thr_menu_start_stop.detach();
					});
				}
				if(!stoppable.empty())
				{
					m.append("Stop all", [&](menu::item_proxy)
					{
						thr_menu_start_stop = std::thread([&]
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
								api::refresh_window(btndl);
							autostart_next_item = true;
							if(thr_menu_start_stop.joinable())
								thr_menu_start_stop.detach();
						});
					});
				}
				if(completed.size() != lbq.at(item.pos().cat).size())
				{
					const auto cat {lbq.selected().front().cat};
					m.append(cat ? "Remove this playlist" : "Remove all", [this, cat](menu::item_proxy)
					{
						queue_remove_all(cat);
					});
				}
			}
		}
		else
		{
			std::vector<std::wstring> startables, stoppables;
			std::vector<drawerbase::listbox::item_proxy> startables_not_done, skippers;

			for(auto &ip : sel)
			{
				auto item {lbq.at(ip)};
				auto url {item.value<lbqval_t>().url};
				if(bottoms.at(url).started)
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
				thr_menu_start_stop = std::thread([this, startables, stoppables]
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
						api::refresh_window(btndl);
					autostart_next_item = true;
					if(thr_menu_start_stop.joinable())
						thr_menu_start_stop.detach();
				});
			});

			m.append("Remove selected", [this](menu::item_proxy)
			{
				const auto sel {lbq.selected()};
				if(sel.size() == lbq.at(sel.front().cat).size())
					queue_remove_all(sel.front().cat);
				else queue_remove_selected();
			});

			m.append("Refresh selected", [&, sel](menu::item_proxy)
			{
				vidsel_item = {&m, sel.front().item};
				for(auto &el : sel)
				{
					auto item {lbq.at(el)};
					auto url {item.value<lbqval_t>().url};
					show_btnfmt(false);
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
					if(!bottoms.at(url).started)
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
		for(auto cat {0}; cat < lbq.size_categ(); cat++)
			for(auto item : lbq.at(cat))
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


void GUI::queue_remove_all(size_t cat)
{
	if(!thr_queue_remove.joinable())
	{
		bottoms.show(L"");
		qurl = L"";
		l_url.update_caption();
		auto *pfm {fm_alert("Shutting down active yt-dlp instances", "Please wait...", false).release()};
		activate();
		thr_queue_remove = std::thread([this, cat, pfm]
		{
			if(active_info_threads || bottoms.joinable_dl_threads())
			{
				pfm->show();
				PostMessage(hwnd, WM_REFRESH, reinterpret_cast<WPARAM>(pfm->handle()), 0);
				subclass::eat_mouse = true;
			}
			for(auto &item : lbq.at(cat))
			{
				auto url {item.value<lbqval_t>().url};
				auto &bottom {bottoms.at(url)};
				bottom.working_info = false;
				bottom.working = false;
			}
			lbq.auto_draw(false);
			auto item {lbq.at(cat).begin()};
			while(item != lbq.at(cat).end() && !exiting)
			{
				auto url {item.value<lbqval_t>().url};
				auto &bottom {bottoms.at(url)};
				if(bottom.timer_proc.started())
					bottom.timer_proc.stop();

				if(bottom.info_thread.joinable())
				{
					bottom.info_thread.join();
				}
				if(bottom.dl_thread.joinable())
				{
					//fm_alert_text.caption(url);
					bottom.dl_thread.join();
				}
				item = lbq.listbox::erase(item);
				bottoms.erase(url);
				outbox.erase(url);
			}
			if(cat)
			{
				lbq.erase(cat);
				if(lbq.at(cat - 1).size())
					lbq.at(cat - 1).back().select(true, true);
			}
			else
			{
				auto vis {lbq.visibles()};
				if(!vis.empty())
					lbq.at(vis.front().cat).at(0).select(true);
			}
			adjust_lbq_headers();
			lbq.auto_draw(true);
			queue_save();
			pfm->close();
			std::default_delete<themed_form>(pfm);
			subclass::eat_mouse = false;
			if(thr_queue_remove.joinable())
				thr_queue_remove.detach();
		});
	}
}


void GUI::queue_remove_selected()
{
	using namespace nana;

	if(!thr_queue_remove.joinable())
	{
		std::wstring val;
		auto sel {lbq.selected()};
		if(sel.back().item < lbq.at(sel.back().cat).size() - 1)
			val = lbq.at(listbox::index_pair {sel.back().cat, sel.back().item + 1}).value<lbqval_t>().url;
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
		lbq.force_no_thread_safe_ops(true);

		auto *pfm {fm_alert("Shutting down active yt-dlp instances", "Please wait...", false).release()};
		activate();

		thr_queue_remove = std::thread([this, pfm, sel_urls, val]
		{
			if(active_info_threads || bottoms.joinable_dl_threads())
			{
				pfm->show();
				//nana::api::refresh_window(*pfm);
				PostMessage(hwnd, WM_REFRESH, reinterpret_cast<WPARAM>(pfm->handle()), 0);
				subclass::eat_mouse = true;
			}

			for(const auto &url : sel_urls)
			{
				auto &bottom {bottoms.at(url)};
				bottom.working_info = false;
				bottom.working = false;
			}

			for(const auto &url : sel_urls)
			{
				auto &bottom {bottoms.at(url)};
				if(bottom.timer_proc.started())
					bottom.timer_proc.stop();
				if(bottom.info_thread.joinable())
				{
					bottom.info_thread.join();
				}
				if(bottom.dl_thread.joinable())
					bottom.dl_thread.join();
				auto item {lbq.item_from_value(url)};
				if(item != lbq.empty_item)
					lbq.erase(item);
				bottoms.erase(url);
				outbox.erase(url);
			}
			lbq.force_no_thread_safe_ops(false);

			if(!val.empty())
			{
				auto item {lbq.item_from_value(val)};
				if(!item.empty())
				{
					item.select(true);
					auto cat {lbq.at(item.pos().cat)};
					int idx {1};
					for(auto ip : cat)
						ip.text(0, std::to_string(idx++));
				}
			}

			adjust_lbq_headers();
			lbq.auto_draw(true);
			queue_save();
			pfm->close();
			std::default_delete<themed_form>(pfm);
			subclass::eat_mouse = false;
			if(thr_queue_remove.joinable())
				thr_queue_remove.detach();
		});
	}
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
	if(fn_write_conf && lbq.item_count())
	{
		conf.unfinished_queue_items.clear();
		for(size_t cat {0}; cat < lbq.size_categ(); cat++)
		{
			auto icat {lbq.at(cat)};
			if(icat.size())
			{
				auto &qitems {conf.unfinished_queue_items.emplace_back(icat.text(), std::vector<std::string>{}).second};
				for(auto item : icat)
				{
					auto text {item.text(3)};
					if(text != "done" && (text != "error" || text == "error" && conf.cb_save_errors))
						qitems.push_back(nana::to_utf8(item.value<lbqval_t>().url));
				}
			}
		}
		return fn_write_conf();
	}
	return true;
}


void GUI::queue_save_data()
{
	conf.unfinished_queue_items.clear();
	unfinished_qitems_data.clear();
	unsigned unfinished_qitems_with_data {0};
	for(size_t cat {0}; cat < lbq.size_categ(); cat++)
	{
		auto icat {lbq.at(cat)};
		if(icat.size())
		{
			for(auto item : icat)
			{
				auto text {item.text(3)};
				if(text != "done" && (text != "error" || text == "error" && conf.cb_save_errors))
				{
					auto &bot {bottoms.at(item.value<lbqval_t>())};
					if(!bot.vidinfo.empty() || !bot.playlist_info.empty())
						unfinished_qitems_with_data++;
				}
			}
		}
	}

	if(unfinished_qitems_with_data < 50)
	{
		for(size_t cat {0}; cat < lbq.size_categ(); cat++)
		{
			auto icat {lbq.at(cat)};
			if(icat.size())
			{
				auto &qitems {conf.unfinished_queue_items.emplace_back(icat.text(), std::vector<std::string>{}).second};
				for(auto item : icat)
				{
					auto text {item.text(3)};
					if(text != "done" && (text != "error" || text == "error" && conf.cb_save_errors))
					{
						const auto wurl {item.value<lbqval_t>().url};
						const auto url {nana::to_utf8(wurl)};
						qitems.push_back(url);
						auto &bottom {bottoms.at(url)};
						if(!bottom.vidinfo.empty() || !bottom.playlist_info.empty())
						{
							auto &j {unfinished_qitems_data[url]};
							bottom.to_json(j);
							j["columns"]["website"] = item.text(1);
							j["columns"]["status"] = text;
							j["columns"]["format"] = item.text(4);
							j["columns"]["format_note"] = item.text(5);
							j["columns"]["ext"] = item.text(6);
							j["columns"]["fsize"] = item.text(7);
						}
					}
				}
			}
		}
		if(!unfinished_qitems_data.empty())
			std::ofstream {infopath} << unfinished_qitems_data;
	}
	else fm_loading(true);
}


void GUI::queue_remove_item(std::wstring url, bool save)
{
	if(!thr_queue_remove.joinable())
	{
		auto item {lbq.item_from_value(url)};
		const auto cat {item.pos().cat};
		if(item != lbq.empty_item)
		{
			auto &bottom {bottoms.at(url)};
			if(lbq.at(cat).size() > 1)
				for(auto it {item}; it != lbq.at(cat).end(); it++)
				{
					const auto stridx {std::to_string(std::stoi(it.text(0)) - 1)};
					it.text(0, stridx);
				}

			if(bottom.timer_proc.started())
				bottom.timer_proc.stop();

			thr_queue_remove = std::thread([this, cat, url, save, &bottom, item]
			{
				if(bottom.info_thread.joinable())
				{
					bottom.working_info = false;
					bottom.info_thread.join();
				}
				if(bottom.dl_thread.joinable())
				{
					bottom.working = false;
					bottom.dl_thread.join();
					if(!qurl.empty())
						on_btn_dl(qurl);
				}

				auto prev_idx {std::stoi(item.text(0)) - 1};
				auto next_item {lbq.erase(item)};
				PostMessage(hwnd, WM_REFRESH, reinterpret_cast<WPARAM>(lbq.handle()), 0);
				taskbar_overall_progress();
				adjust_lbq_headers();
				if(next_item != lbq.at(cat).end())
					SendMessage(hwnd, WM_LBQ_SELECT_EX, reinterpret_cast<WPARAM>(&next_item), false);
				else if(lbq.at(cat).size() != 0)
				{
					auto item {lbq.at(cat).at(prev_idx > -1 ? prev_idx : 0)};
					SendMessage(hwnd, WM_LBQ_SELECT_EX, reinterpret_cast<WPARAM>(&item), false);
				}
				else if(cat && lbq.at(cat - 1).size() != 0)
				{
					auto item {lbq.at(cat - 1).back()};
					SendMessage(hwnd, WM_LBQ_SELECT_EX, reinterpret_cast<WPARAM>(&item), true);
				}
				else
				{
					bottoms.show(L"");
					qurl = L"";
					l_url.update_caption();
				}
				bottoms.erase(url);
				outbox.erase(url);
				if(cat && !lbq.at(cat).size())
					lbq.erase(cat);
				if(!conf.common_dl_options)
				{
					auto sel {lbq.selected()};
					if(!sel.empty())
					{
						gpopt.caption("Download options for queue item #" + lbq.at(sel.front()).text(0));
						PostMessage(hwnd, WM_REFRESH, reinterpret_cast<WPARAM>(gpopt.handle()), 0);
					}
					else
					{
						gpopt.caption("Download options");
						show_btncopy(false);
						show_btnfmt(false);
					}
				}
				if(save) queue_save();

				if(thr_queue_remove.joinable())
					thr_queue_remove.detach();
			});
		}
	}
}