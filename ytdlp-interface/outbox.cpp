#include "gui.hpp"


void GUI::Outbox::create(GUI *parent, bool visible)
{
	using namespace nana;
	using ::widgets::theme;

	main_thread_id = std::this_thread::get_id();
	pgui = parent;
	Textbox::create(*parent, visible);
	editable(false);
	enable_caret();
	line_wrapped(true);
	set_undo_queue_length(0);
	enable_border_focused(false);
	highlight(pgui->conf.kwhilite);
	scheme().mouse_wheel.lines = 3;
	typeface(paint::font_info {"Arial", 10});
	api::effects_edge_nimbus(*this, effects::edge_nimbus::none);

	t_selcopy_flash.interval(std::chrono::milliseconds {20});
	t_selcopy_flash.elapse([this]
	{
		static color orig_bg;
		static double steps {1000};
		static int iterations {0};
		static bool increasing, active {false};
		if(!active)
			orig_bg = scheme().selection;
		active = true;
		scheme().selection = orig_bg.blend(theme::fmbg, 1.0 - steps / 1000);
		api::refresh_window(*this);
		if(increasing)
			steps += 100;
		else steps -= 350;
		if(steps >= 1000)
		{
			steps = 1000;
			iterations++;
			increasing = false;
		}
		if(steps <= 0)
		{
			steps = 0;
			increasing = true;
		}
		if(iterations == 1)
		{
			steps = 1000;
			iterations = 0;
			increasing = false;
			scheme().selection = orig_bg;
			api::refresh_window(*this);
			t_selcopy_flash.stop();
			active = false;
		}
	});

	t_cmdcopy_flash.interval(std::chrono::milliseconds {20});
	t_cmdcopy_flash.elapse([this]
	{
		using namespace nana;
		static color orig_bg;
		static double steps {1000};
		static int iterations {0};
		static bool increasing, active {false};
		if(!active)
			orig_bg = scheme().selection;
		active = true;
		auto ca {colored_area_access()};
		auto p {ca->get(0)};
		p->count = 1;
		p->fgcolor = (conf.kwhilite ? (theme::is_dark() ? theme::path_link_fg : color {"#569"}) : theme::tbfg).blend(theme::fmbg, 1.0 - steps / 1000);
		api::refresh_window(*this);
		if(increasing)
			steps += 100;
		else steps -= 300;
		if(steps >= 1000)
		{
			steps = 1000;
			iterations++;
			increasing = false;
		}
		if(steps <= 0)
		{
			steps = 0;
			increasing = true;
		}
		if(iterations == 1)
		{
			steps = 1000;
			iterations = 0;
			increasing = false;
			scheme().selection = orig_bg;
			api::refresh_window(*this);
			t_cmdcopy_flash.stop();
			active = false;
		}
	});

	events().dbl_click([&](const nana::arg_mouse &arg)
	{
		if(arg.is_left_button())
		{
			pgui->show_queue();
		}
	});

	events().key_char([&](const nana::arg_keyboard &arg)
	{
		using namespace nana;

		if(arg.key == nana::keyboard::copy)
		{
			if(!t_selcopy_flash.started())
				t_selcopy_flash.start();
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

			m.append("Copy selection", [&](menu::item_proxy)
			{
				if(!t_selcopy_flash.started())
					t_selcopy_flash.start();
				copy();
			}).enabled(selected());

			if(commands.contains(current_))
			{
				m.append("Copy command line", [this](menu::item_proxy)
				{
					if(!t_cmdcopy_flash.started())
						t_cmdcopy_flash.start();
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
			select(false);
		}
		widget::show();
		pgui->overlay.hide();
	}
}


void GUI::Outbox::append(std::wstring url, std::string text)
{
	if(!empty())
	{
		if(std::this_thread::get_id() == main_thread_id)
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
		else 
		{
			std::pair<std::wstring, std::string> lparam {url, text};
			SendMessage(pgui->hwnd, WM_OUTBOX_APPEND, reinterpret_cast<WPARAM>(this), reinterpret_cast<LPARAM>(&lparam));
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