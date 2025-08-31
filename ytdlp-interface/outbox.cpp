#include "gui.hpp"


void GUI::Outbox::create(GUI *parent, bool visible)
{
	pgui = parent;
	Textbox::create(*parent, visible);
	editable(false);
	line_wrapped(true);
	set_undo_queue_length(0);
	enable_border_focused(false);
	highlight(pgui->conf.kwhilite);
	scheme().mouse_wheel.lines = 3;
	typeface(nana::paint::font_info {"Arial", 10});
	nana::API::effects_edge_nimbus(*this, nana::effects::edge_nimbus::none);

	events().dbl_click([&](const nana::arg_mouse &arg)
	{
		if(arg.is_left_button())
		{
			pgui->show_queue();
		}
	});

	events().mouse_up([&](const nana::arg_mouse &arg)
	{
		using namespace nana;
		using ::widgets::theme;

		const auto &text {buffers[current_]};

		if(arg.button == mouse::right_button)
		{
			::widgets::Menu m;
			m.item_pixels(24);

			m.append("Copy to clipboard", [&](menu::item_proxy)
			{
				util::set_clipboard_text(pgui->hwnd, to_wstring(text));

				if(!thr.joinable()) thr = std::thread([this]
				{
					working = true;
					auto bgclr {bgcolor()};
					auto blendclr {theme::is_dark() ? colors::dark_orange : colors::orange};
					std::vector<color> blended_colors;
					for(auto n {.1}; n > 0; n -= .01)
						blended_colors.push_back(bgcolor().blend(blendclr, n));
					int n {0};
					if(theme::is_dark())
					{
						for(auto i {blended_colors.size() / 2}; i < blended_colors.size(); i++)
						{
							bgcolor(blended_colors[i]);
							Sleep(90 - n);
							n += 18;
						}
					}
					else for(const auto &clr : blended_colors)
					{
						bgcolor(clr);
						Sleep(90 - n);
						n += 9;
					}
					bgcolor(bgclr);
					if(working)
					{
						working = false;
						if(thr.joinable())
							thr.detach();
					}
				});
			}).enabled(arg.window_handle == handle());

			if(commands.contains(current_))
			{
				m.append("Copy command only", [this](menu::item_proxy)
				{
					util::set_clipboard_text(pgui->hwnd, to_wstring(commands[current_]));
				}).enabled(arg.window_handle == handle());

				m.append_splitter();
			}

			m.append("Keyword highlighting", [this](menu::item_proxy)
			{
				conf.kwhilite = !conf.kwhilite;
				highlight(conf.kwhilite);
				const auto &text {buffers[current_]};
				if(!text.empty())
				{
					auto ca {colored_area_access()};
					ca->clear();
					if(conf.kwhilite)
					{
						auto p {ca->get(0)};
						if(text.starts_with("[GUI]"))
						{
							p->count = 1;
							p->fgcolor = theme::is_dark() ? theme::path_link_fg : nana::color {"#569"};
						}
						auto pos {text.rfind("[GUI] ")};
						if(pos != -1 && pos > 0)
						{
							p = ca->get(text_line_count() - 2);
							p->count = 1;
							if(text.rfind("WM_CLOSE") == -1)
								p->fgcolor = theme::is_dark() ? theme::path_link_fg : nana::color {"#569"};
							else p->fgcolor = theme::is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
						}
					}
					nana::API::refresh_window(*this);
				}
			}).checked(conf.kwhilite);

			m.append("Limited buffer size", [this](menu::item_proxy)
			{
				conf.limit_output_buffer = !conf.limit_output_buffer;
			}).checked(conf.limit_output_buffer);

			m.popup_await(*this, arg.pos.x, arg.pos.y);
		}
	});
}


void GUI::Outbox::show(std::wstring url)
{
	if(!buffers.contains(url))
		buffers.emplace(url, "");
	if(buffers[url].empty())
	{
		widget::hide();
		pgui->overlay.show();
	}
	else if(!empty() && buffers.contains(url))
	{
		current_ = url;
		const auto &text {buffers[url]};
		caption(text);
		if(scroll_operation()->visible(true))
		{
			if(text.substr(0, 5) != "[json")
			{
				editable(true);
				caret_pos({0, static_cast<unsigned>(text_line_count() - 1)});
				editable(false);
			}
		}
		if(!text.empty())
		{
			auto ca {colored_area_access()};
			ca->clear();
			if(conf.kwhilite)
			{
				auto p {ca->get(0)};
				if(text.starts_with("[GUI]"))
				{
					p->count = 1;
					p->fgcolor = widgets::theme::is_dark() ? widgets::theme::path_link_fg : nana::color {"#569"};
				}
				auto pos {text.rfind("[GUI] ")};
				if(pos != -1 && pos > 0)
				{
					p = ca->get(text_line_count() - 2);
					p->count = 1;
					if(text.rfind("WM_CLOSE") == -1)
						p->fgcolor = widgets::theme::is_dark() ? widgets::theme::path_link_fg : nana::color {"#569"};
					else p->fgcolor = widgets::theme::is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
				}
				nana::API::refresh_window(*this);
			}
		}
		widget::show();
		pgui->overlay.hide();
	}
}


void GUI::Outbox::append(std::wstring url, std::string text)
{
	if(!empty())
	{
		if(!visible() && url == pgui->qurl && pgui->btnq.caption().find("queue") != -1)
		{
			widget::show();
			pgui->overlay.hide();
		}
		auto &buf {buffers[url]};
		if(buf.empty() && text.starts_with("[GUI]"))
		{
			auto cmd {text};
			while(cmd.back() == '\r' || cmd.back() == '\n')
				cmd.pop_back();
			auto pos {cmd.find('\"')};
			if(pos != -1)
				commands[url] = cmd.substr(pos);
		}
		size_t buflim {conf.output_buffer_size}, bufsize {buf.size()}, textsize {text.size()};
		if(conf.limit_output_buffer && bufsize + textsize > buflim)
		{
			if(url == current_)
			{
				auto ca {colored_area_access()};
				if(ca->size())
					ca->remove(0);
			}
			std::string newbuf(buflim, ' ');
			if(textsize > buflim)
				buf = text.substr(textsize - buflim, buflim);
			else
			{
				newbuf = buf.substr(bufsize + textsize - buflim);
				newbuf += text;
				buf = std::move(newbuf);
			}
			if(url == current_)
			{
				textbox::caption(buf);
				editable(true);
				caret_pos({0, static_cast<unsigned>(text_line_count() - 1)});
				editable(false);
			}
		}
		else
		{
			buf += text;
			if(url == current_)
				textbox::append(text, false);
		}
	}
}


void GUI::Outbox::caption(std::string text, std::wstring url)
{
	if(!empty())
	{
		std::stringstream ss {text};
		std::string line;
		while(std::getline(ss, line))
		{
			if(line.size() > 3 && line.front() == '[')
			{
				const auto pos {line.find(']', 1)};
				if(pos != -1)
				{
					const auto kw {line.substr(0, pos + 1)};
					if(kw != "[GUI]" && kw != "[download]")
						set_keyword(kw);
				}
			}
		}

		if(url.empty())
			buffers[current_] = text;
		else buffers[url] = text;
		textbox::caption(text);
	}
}


void GUI::Outbox::erase(std::wstring url)
{
	if(buffers.contains(url))
	{
		buffers.erase(url);
		if(buffers.size() == 1)
			current_.clear();
	}
}


void GUI::Outbox::clear(std::wstring url)
{
	if(url.empty())
	{
		select(true);
		del();
		buffers[current_].clear();
	}
	else buffers[url].clear();
}