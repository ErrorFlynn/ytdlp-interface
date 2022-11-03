#include <thread>
#include <sstream>
#include <unordered_set>
#include <iostream>
#include <atlbase.h> // CComPtr
#include <Shobjidl.h> // ITaskbarList3
#include <nana/gui/timer.hpp>

#include "widgets.hpp"
#include "themed_form.hpp"

namespace fs = std::filesystem;
constexpr int YTDLP_DOWNLOAD = 0, YTDLP_POSTPROCESS = 1;

class GUI : public themed_form
{	
public:

	GUI();

	static struct settings_t
	{
		fs::path ytdlp_path, outpath;
		const std::wstring output_template_default {L"%(title)s.%(ext)s"};
		std::wstring fmt1, fmt2, output_template {output_template_default};
		std::vector<std::string> argsets, unfinished_queue_items;
		std::unordered_set<std::wstring> outpaths;
		double ratelim {0}, contrast {.1};
		unsigned ratelim_unit {1}, pref_res {2}, pref_video {1}, pref_audio {1}, cbtheme {2}, max_argsets {10}, max_outpaths {10}, 
			max_concurrent_downloads {1};
		std::chrono::milliseconds max_proc_dur {3000};
		size_t com_args {0};
		bool cbsplit {false}, cbchaps {false}, cbsubs {false}, cbthumb {false}, cbtime {true}, cbkeyframes {false}, cbmp3 {false},
			cbargs {false}, kwhilite {true}, pref_fps {true}, cb_lengthyproc {true}, common_dl_options {true}, cb_autostart {true};
	}
	conf;

private:

	nlohmann::json releases;
	fs::path self_path, appdir, ffmpeg_loc;
	std::string inet_error, url_latest_ffmpeg, url_latest_ytdlp, url_latest_ytdlp_relnotes;
	std::wstring clipboard_buffer;
	std::wstringstream multiple_url_text;
	unsigned size_latest_ffmpeg {0}, size_latest_ytdlp {0}, number_of_processors {4};
	bool working {false}, menu_working {false}, lbq_no_action {false}, thumbthr_working {false}, 
		autostart_next_item {true}, lbq_can_drag {false};
	std::thread thr, thr_releases, thr_releases_misc, thr_versions, thr_thumb, thr_menu;
	CComPtr<ITaskbarList3> i_taskbar;
	UINT WM_TASKBAR_BUTTON_CREATED {0};
	const std::string ver_tag {"v1.6"}, title {"ytdlp-interface " + ver_tag/*.substr(0, 4)*/};
	nana::drawerbase::listbox::item_proxy *last_selected {nullptr};
	nana::timer tproc;

	const std::vector<std::wstring>
		com_res_options {L"2160", L"1440", L"1080", L"720", L"480", L"360"},
		com_audio_options {L"none", L"m4a", L"mp3", L"ogg", L"webm"},
		com_video_options {L"none", L"mp4", L"webm"};

	struct version_t
	{
		int year {0}, month {0}, day {0};
		bool empty() { return year == 0; }
		auto string()
		{
			auto fmt = [](int i)
			{
				auto str {std::to_string(i)};
				return str.size() == 1 ? '0' + str : str;
			};
			return std::to_string(year) + '.' + fmt(month) + '.' + fmt(day);
		}
		bool operator > (const version_t &o)
		{
			auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
			auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
			return self > other;
		}
	}
	ver_ffmpeg, ver_ffmpeg_latest, ver_ytdlp, ver_ytdlp_latest;


	class gui_bottom : public nana::panel<true>
	{
		GUI *pgui {nullptr};
	public:
		gui_bottom(GUI &gui, bool visible = false);

		bool is_ytlink {false}, use_strfmt {false}, working {false}, graceful_exit {false}, working_info {true}, received_procmsg {false};
		fs::path outpath;
		nlohmann::json vidinfo;
		std::wstring url, strfmt, fmt1, fmt2;
		std::thread dl_thread, info_thread;
		int index {0};

		widgets::Group gpopt;
		nana::place plc;
		nana::place plcopt;
		nana::timer timer_proc;
		widgets::Textbox tbrate;
		widgets::Progress prog;
		widgets::Button btn_ytfmtlist, btndl, btnerase, btnq, btncopy;
		widgets::Label l_out, l_rate;
		widgets::path_label l_outpath;
		widgets::Combox com_rate, com_args;
		widgets::cbox cbsplit, cbkeyframes, cbmp3, cbchaps, cbsubs, cbthumb, cbtime, cbargs;
		widgets::Separator separator;

		void show_btncopy(bool show);
		void show_btnytfmt(bool show);
	};


	class gui_bottoms
	{
		std::list<std::wstring> insertion_order;
		std::map<std::wstring, std::unique_ptr<gui_bottom>> bottoms;
		GUI *gui {nullptr};

	public:
		gui_bottoms(GUI *g) : gui {g} {}
		auto contains(std::wstring key) { return bottoms.contains(key); }
		gui_bottom &at(std::wstring key) { return *bottoms.at(key); }
		gui_bottom &at(int idx)
		{
			int i {0};
			for(auto &key : insertion_order)
				if(i++ == idx)
					return *bottoms.at(key);
			return *bottoms.begin()->second;
		}
		auto url_at(int idx)
		{
			int i {0};
			for(auto &key : insertion_order)
				if(i++ == idx)
					return key;
			return bottoms.begin()->first;
		}
		gui_bottom &current() { return at(visible()); }
		auto begin() noexcept { return bottoms.begin(); }
		const auto begin() const noexcept { return bottoms.begin(); }
		auto rbegin() noexcept { return bottoms.rbegin(); }
		auto end() noexcept { return bottoms.end(); }
		auto rend() noexcept { return bottoms.rend(); }
		const auto end() const noexcept { return bottoms.end(); }
		auto size() const noexcept { return bottoms.size(); }
		gui_bottom* back() const
		{
			if(!insertion_order.empty())
				return bottoms.at(insertion_order.back()).get();
			return nullptr;
		}

		void show(std::wstring key)
		{
			// first show the target, and only then hide the others, to avoid flickering

			for(auto &bottom : bottoms)
			{
				if(bottom.first == key)
				{
					bottom.second->show();
					bottom.second->btndl.focus();
					break;
				}
			}

			for(auto &bottom : bottoms)
				if(bottom.first != key)
					bottom.second->hide();

			gui->get_place().collocate();
		}

		std::wstring visible()
		{
			for(auto &bottom : bottoms)
				if(bottom.second->visible())
					return bottom.first;
			return L"";
		}

		auto &add(std::wstring url, bool visible = false)
		{
			if(url.empty() && !bottoms.empty()) 
				bottoms.clear();
			auto it {bottoms.find(url)};
			if(it == bottoms.end())
			{
				auto &pbot {bottoms[url] = std::make_unique<gui_bottom>(*gui, visible)};
				insertion_order.push_back(url);
				gui->get_place()["Bottom"].fasten(*pbot);
				pbot->index = insertion_order.size() - 1;
				pbot->url = url;
				pbot->is_ytlink = gui->is_ytlink(nana::to_utf8(url));
				return *pbot;
			}
			return *it->second;
		}

		void erase(std::wstring url)
		{
			if(bottoms.contains(url))
			{
				insertion_order.remove(url);
				gui->get_place().erase(*bottoms.at(url));
				bottoms.erase(url);
			}
		}

		void propagate_cb_options(const gui_bottom &srcbot)
		{
			for(auto &pbot : *this)
			{
				auto &bot {*pbot.second};
				if(bot.handle() != srcbot.handle())
				{
					bot.cbargs.check(srcbot.cbargs.checked());
					bot.cbchaps.check(srcbot.cbchaps.checked());
					bot.cbkeyframes.check(srcbot.cbkeyframes.checked());
					bot.cbmp3.check(srcbot.cbmp3.checked());
					bot.cbsplit.check(srcbot.cbsplit.checked());
					bot.cbsubs.check(srcbot.cbsubs.checked());
					bot.cbthumb.check(srcbot.cbthumb.checked());
					bot.cbtime.check(srcbot.cbtime.checked());
				}
			}
		}

		void propagate_args_options(const gui_bottom &srcbot)
		{
			for(auto &pbot : *this)
			{
				auto &bot {*pbot.second};
				if(bot.handle() != srcbot.handle())
				{
					bot.com_args.clear();
					if(srcbot.com_args.the_number_of_options())
					{
						for(int n {0}; n < srcbot.com_args.the_number_of_options(); n++)
							bot.com_args.push_back(srcbot.com_args.text(n));
						bot.com_args.caption(srcbot.com_args.caption());
					}
				}
			}
		}

		void propagate_misc_options(const gui_bottom &srcbot)
		{
			for(auto &pbot : *this)
			{
				auto &bot {*pbot.second};
				if(bot.handle() != srcbot.handle())
				{
					bot.com_rate.option(srcbot.com_rate.option());
					bot.tbrate.caption(srcbot.tbrate.caption());
					bot.outpath = srcbot.outpath;
					bot.l_outpath.update_caption();
				}
			}
		}
	};


	class Outbox : public widgets::Textbox
	{
		std::map<std::wstring, std::string> buffers {{L"", ""}};
		std::wstring current_;
		GUI *pgui {nullptr};

	public:

		Outbox() : Textbox() {};

		Outbox(GUI *parent, bool visible = true)
		{
			create(parent, visible);
		}

		void create(GUI *parent, bool visible = true)
		{
			pgui = parent;
			Textbox::create(*parent, visible);
			editable(false);
			line_wrapped(true);
			highlight(pgui->conf.kwhilite);
			scheme().mouse_wheel.lines = 3;
			typeface(nana::paint::font_info {"Arial", 10});
			nana::API::effects_edge_nimbus(*this, nana::effects::edge_nimbus::none);

			events().click([&, this]
			{
				pgui->clipboard_buffer = util::get_clipboard_text();
				select(true);
				copy();
				select(false);
			});

			events().dbl_click([&, this] (const nana::arg_mouse &arg)
			{
				if(arg.is_left_button())
				{
					util::set_clipboard_text(pgui->hwnd, pgui->clipboard_buffer);
					pgui->show_queue();
				}
			});

			events().mouse_up([&, this](const nana::arg_mouse &arg)
			{
				if(arg.button == nana::mouse::right_button)
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
							p->count = 1;
							p->fgcolor = widgets::theme.is_dark() ? widgets::theme.path_link_fg : nana::color {"#569"};
							if(text.find("[GUI] ") != text.rfind("[GUI] "))
							{
								p = ca->get(text_line_count() - 2);
								p->count = 1;
								if(text.rfind("WM_CLOSE") == -1)
									p->fgcolor = widgets::theme.is_dark() ? widgets::theme.path_link_fg : nana::color {"#569"};
								else p->fgcolor = widgets::theme.is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
							}
						}
						nana::API::refresh_window(*this);
					}
				}
			});
		}

		void show(std::wstring url)
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
					editable(true);
					caret_pos({0, static_cast<unsigned>(text_line_count() - 1)});
					editable(false);
				}
				if(!text.empty())
				{
					auto ca {colored_area_access()};
					ca->clear();
					if(conf.kwhilite)
					{
						auto p {ca->get(0)};
						p->count = 1;
						p->fgcolor = widgets::theme.is_dark() ? widgets::theme.path_link_fg : nana::color {"#569"};
						if(text.find("[GUI] ") != text.rfind("[GUI] "))
						{
							p = ca->get(text_line_count() - 2);
							p->count = 1;
							if(text.rfind("WM_CLOSE") == -1)
								p->fgcolor = widgets::theme.is_dark() ? widgets::theme.path_link_fg : nana::color {"#569"};
							else p->fgcolor = widgets::theme.is_dark() ? nana::color {"#f99"} : nana::color {"#832"};
						}
						nana::API::refresh_window(*this);
					}
				}
				widget::show();
				pgui->overlay.hide();
			}
		}

		void append(std::wstring url, std::string text)
		{
			if(!empty())
			{
				if(!visible() && url == pgui->bottoms.current().url && pgui->bottoms.at(url).btnq.caption().find("queue") != -1)
				{
					widget::show();
					pgui->overlay.hide();
				}
				buffers[url] += text;
				if(url == current_)
					textbox::append(text, false);
			}
		}

		void append(std::wstring url, std::wstring text)
		{
			append(url, nana::to_utf8(text));
		}

		void caption(std::string text, std::wstring url = L"")
		{
			if(!empty())
			{
				if(url.empty())
					buffers[current_] = text;
				else buffers[url] = text;
				textbox::caption(text);
			}
		}

		void caption(std::wstring text, std::wstring url = L"")
		{
			caption(nana::to_utf8(text), url);
		}

		void erase(std::wstring url)
		{
			if(buffers.contains(url))
				buffers.erase(url);
		}

		auto caption() const { return textbox::caption(); }
		const auto &current_buffer() { return buffers[current_]; }

		void current(std::wstring url) { current_ = url; }
		auto current() { return current_; }

		void clear(std::wstring url = L"")
		{
			if(url.empty())
			{
				select(true);
				del();
				buffers[current_].clear();
			}
			else buffers[url].clear();
		}
	};


	gui_bottoms bottoms {this};
	Outbox outbox {this};
	widgets::Overlay overlay {*this};
	nana::panel<false> queue_panel {*this};
	nana::place plc_queue {queue_panel};
	widgets::Listbox lbq {queue_panel};
	widgets::Button btnadd {queue_panel, "Add link"}, btn_qact {queue_panel, "Queue actions"}, btn_settings {queue_panel, "Settings"};
	std::wstring qurl;
	widgets::path_label l_url {queue_panel, &qurl};


	void pop_queue_menu(int x, int y);
	void make_queue_listbox();
	void formats_dlg();
	bool process_queue_item(std::wstring url);
	void settings_dlg();
	void changes_dlg(nana::window parent);
	void make_form();
	bool apply_theme(bool dark);
	void get_releases();
	void get_releases_misc();
	void get_versions();
	bool is_ytlink(std::string_view text);
	void change_field_weight(nana::place &plc, std::string field, unsigned new_weight);
	bool is_tag_a_new_version(std::string tag_name);
	void updater_dlg(nana::window parent);
	void show_queue(bool freeze_redraw = true);
	void show_output();
	void add_url(std::wstring url);
	void taskbar_overall_progress();
	void on_btn_dl(std::wstring url);
	void remove_queue_item(std::wstring url);
	std::wstring next_startable_url(std::wstring current_url = L"");
	void adjust_lbq_headers()
	{
		auto zero {dpi_transform(30).width};
		auto one {dpi_transform(120).width + dpi_transform(16 * lbq.scroll_operation()->visible(true)).width};
		auto three {dpi_transform(116).width};
		lbq.column_at(2).width(lbq.size().width - (zero + one + three) - dpi_transform(9).width);
	}
};
