#include <thread>
#include <sstream>
#include <algorithm>
#include <unordered_set>
#include <iostream>
#include <atlbase.h> // CComPtr
#include <Shobjidl.h> // ITaskbarList3
#include <nana/gui/timer.hpp>

#include "widgets.hpp"
#include "themed_form.hpp"

#undef min
#undef max

namespace fs = std::filesystem;
constexpr int YTDLP_DOWNLOAD {0}, YTDLP_POSTPROCESS {1};
constexpr bool X64 {INTPTR_MAX == INT64_MAX};

class GUI : public themed_form
{	
public:

	GUI();

	static struct settings_t
	{
		fs::path ytdlp_path, outpath;
		const std::wstring output_template_default {L"%(title)s.%(ext)s"}, playlist_indexing_default {L"%(playlist_index)d - "},
			output_template_default_bandcamp {L"%(artist)s - %(album)s - %(track_number)02d - %(track)s.%(ext)s"};
		std::wstring fmt1, fmt2, output_template {output_template_default}, playlist_indexing {playlist_indexing_default},
			output_template_bandcamp {output_template_default_bandcamp};
		std::vector<std::string> argsets, unfinished_queue_items;
		std::unordered_set<std::wstring> outpaths;
		std::map<std::wstring, std::string> playsel_strings;
		double ratelim {0}, contrast {.1};
		unsigned ratelim_unit {1}, pref_res {0}, pref_video {0}, pref_audio {0}, cbtheme {2}, max_argsets {10}, max_outpaths {10}, 
			max_concurrent_downloads {1};
		std::chrono::milliseconds max_proc_dur {3000};
		size_t com_args {0};
		bool cbsplit {false}, cbchaps {false}, cbsubs {false}, cbthumb {false}, cbtime {true}, cbkeyframes {false}, cbmp3 {false},
			cbargs {false}, kwhilite {true}, pref_fps {false}, cb_lengthyproc {true}, common_dl_options {true}, cb_autostart {true},
			cb_queue_autostart {false}, gpopt_hidden {false}, open_dialog_origin {false}, cb_zeropadding {true}, cb_playlist_folder {true},
			zoomed {false}, get_releases_at_startup {true}, col_format {false}, col_format_note {true}, col_ext {true}, col_fsize {false},
			json_hide_null {false}, col_site_icon {true}, col_site_text {false}, ytdlp_nightly {false}, audio_multistreams {false};
		nana::rectangle winrect;
		int dpi {96};
	}
	conf;

private:

	nlohmann::json releases;
	fs::path self_path, appdir, ffmpeg_loc;
	std::string inet_error, url_latest_ffmpeg, url_latest_ytdlp, url_latest_ytdlp_relnotes;
	std::wstringstream multiple_url_text;
	long minw {0}, minh {0};
	unsigned size_latest_ffmpeg {0}, size_latest_ytdlp {0}, number_of_processors {4};
	bool working {false}, menu_working {false}, lbq_no_action {false}, thumbthr_working {false}, 
		autostart_next_item {true}, lbq_can_drag {false}, cnlang {false}, no_draw_freeze {true};
	std::thread thr, thr_releases, thr_versions, thr_thumb, thr_menu, thr_releases_ffmpeg, thr_releases_ytdlp;
	CComPtr<ITaskbarList3> i_taskbar;
	UINT WM_TASKBAR_BUTTON_CREATED {0};
	const std::string ver_tag {"v2.3.0"}, title {"ytdlp-interface " + ver_tag/*.substr(0, 4)*/},
		ytdlp_fname {X64 ? "yt-dlp.exe" : "yt-dlp_x86.exe"};
	const unsigned MINW {900}, MINH {700}; // min client area size
	nana::drawerbase::listbox::item_proxy *last_selected {nullptr};
	nana::timer tproc;

	struct { nana::menu *m {nullptr}; std::size_t pos {0}; } vidsel_item;

	const std::vector<std::wstring>
		com_res_options {L"none", L"4320", L"2160", L"1440", L"1080", L"720", L"480", L"360"},
		com_audio_options {L"none", L"m4a", L"mp3", L"ogg", L"webm", L"flac"},
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
		bool operator == (const version_t &o)
		{
			auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
			auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
			return self == other;
		}
		bool operator != (const version_t &o)
		{
			auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
			auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
			return self != other;
		}
	}
	ver_ffmpeg, ver_ffmpeg_latest, ver_ytdlp, ver_ytdlp_latest;

	struct semver_t
	{
		int major {0}, minor {0}, patch {0};
		semver_t() = default;
		semver_t(int maj, int min, int pat) : major {maj}, minor {min}, patch {pat} {}
		semver_t(std::string ver_tag)
		{
			if(!ver_tag.empty())
			{
				std::string strmajor, strminor, strpatch;
				if(ver_tag[0] == 'v')
					ver_tag.erase(0, 1);
				auto pos {ver_tag.find('.')};
				if(pos != -1)
				{
					strmajor = ver_tag.substr(0, pos++);
					auto pos2 {ver_tag.find('.', pos)};
					if(pos2 != -1)
					{
						strminor = ver_tag.substr(pos, pos2++ - pos);
						strpatch = ver_tag.substr(pos2);
					}
				}
				
				try
				{
					major = std::stoi(strmajor);
					minor = std::stoi(strminor);
					patch = std::stoi(strpatch);
				}
				catch(...) {}
			}
		}

		bool operator > (const semver_t &o)
		{
			return major > o.major || major == o.major && minor > o.minor || major == o.major && minor == o.minor && patch > o.patch;
		}

		bool operator < (const semver_t &o)
		{
			return major < o.major || major == o.major && minor < o.minor || major == o.major && minor == o.minor && patch < o.patch;
		}

		bool operator == (const semver_t &o)
		{
			return major == o.major && minor == o.minor && patch == o.patch;
		}
	}
	cur_ver {ver_tag};


	struct favicon_t
	{
		using image = nana::paint::image;
		using callback = std::function<void(image&)>;

		favicon_t() = default;

		void add(std::string favicon_url, callback fn)
		{
			if(img.empty())
			{
				std::lock_guard<std::mutex> lock {mtx};
				callbacks.push_back(std::move(fn));
				if(!thr.joinable())
					thr = std::thread {[favicon_url, this]
					{
						working = true;
						if(!favicon_url.empty())
						{
							std::string res, error;
							res = util::get_inet_res(favicon_url.data(), &error);
							if(working)
							{
								std::lock_guard<std::mutex> lock {mtx};
								if(!img.open(res.data(), res.size()))
									img.open(arr_url22_png, sizeof arr_url22_png);
								while(!callbacks.empty())
								{
									callbacks.back()(img);
									callbacks.pop_back();
								}
								working = false;
								if(thr.joinable())
									thr.detach();
							}
						}
					}};
			}
			else fn(img);
		}

		~favicon_t()
		{
			if(thr.joinable())
			{
				working = false;
				thr.join();
			}
		}

		operator const image &() const { return img; }

	private:
		image img;
		std::thread thr;
		std::mutex mtx;
		bool working {false};
		std::vector<callback> callbacks;
	};


	class gui_bottom : public nana::panel<true>
	{
		GUI *pgui {nullptr};
	public:
		gui_bottom(GUI &gui, bool visible = false);

		bool is_ytlink {false}, use_strfmt {false}, working {false}, graceful_exit {false}, working_info {true}, received_procmsg {false},
			is_ytplaylist {false}, is_ytchan {false}, is_bcplaylist {false}, is_bclink {false}, is_bcchan {false};
		fs::path outpath, merger_path, download_path, printed_path;
		nlohmann::json vidinfo, playlist_info;
		std::vector<bool> playlist_selection;
		std::vector<std::pair<std::wstring, std::wstring>> sections;
		std::wstring url, strfmt, fmt1, fmt2, playsel_string, cmdinfo, playlist_vid_cmdinfo;
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
		widgets::Expcol expcol;

		void show_btncopy(bool show);
		void show_btnfmt(bool show);
		bool btnfmt_visible() { return plc.field_visible("btn_ytfmtlist"); }
		fs::path file_path();

		bool started() { return btndl.caption().find("Stop") == 0; }
		bool using_custom_fmt() {return cbargs.checked() && com_args.caption_wstring().find(L"-f ") != -1;}

		auto playlist_selected()
		{
			int cnt {0};
			for(auto el : playlist_selection)
				if(el) cnt++;
			return cnt;
		}

		auto vidinfo_contains(std::string key)
		{
			if(vidinfo.contains(key) && vidinfo[key] != nullptr)
				return true;
			return false;
		}

		void apply_playsel_string()
		{
			auto &str {playsel_string};
			playlist_selection.assign(playlist_info["entries"].size(), false);
			auto pos0 {str.find('|')};
			std::wstring id_first_then {str.substr(0, pos0)};
			str.erase(0, pos0 + 1);
			
			int idx_offset {0};
			for(const auto &entry : playlist_info["entries"])
			{
				if(nana::to_wstring(std::string {entry["id"]}) == id_first_then)
					break;
				idx_offset++;
			}

			int a {0}, b {0};
			size_t pos {0}, pos1 {0};
			pos1 = str.find_first_of(L",:", pos);
			if(pos1 == -1)
				playlist_selection[stoi(str) - 1] = true;
			while(pos1 != -1)
			{
				a = stoi(str.substr(pos, pos1 - pos)) - 1 + idx_offset;
				pos = pos1 + 1;
				if(str[pos1] == ',')
				{
					playlist_selection[a] = true;
					pos1 = str.find_first_of(L",:", pos);
					if(pos1 == -1)
						playlist_selection[stoi(str.substr(pos)) - 1 + idx_offset] = true;
				}
				else // ':'
				{
					pos1 = str.find(',', pos);
					if(pos1 != -1)
					{
						b = stoi(str.substr(pos, pos1 - pos)) - 1 + idx_offset;
						pos = pos1 + 1;
						pos1 = str.find_first_of(L",:", pos);
						if(pos1 == -1)
							playlist_selection[stoi(str.substr(pos)) - 1 + idx_offset] = true;
					}
					else b = stoi(str.substr(pos)) - 1 + idx_offset;

					for(auto n {a}; n <= b; n++)
						playlist_selection[n] = true;
				}
			}
		}
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
		gui_bottom &at(std::string key) { return *bottoms.at(nana::to_wstring(key)); }
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
				pbot->is_ytlink = gui->is_ytlink(url);
				pbot->is_ytplaylist = pbot->is_ytlink && (url.find(L"?list=") != -1 || url.find(L"&list=") != -1);
				pbot->is_ytchan = url.find(L"www.youtube.com/c/") != -1 || url.find(L"www.youtube.com/@") != -1 ||
					url.find(L"www.youtube.com/channel/") != -1 || url.find(L"www.youtube.com/user/") != -1;
				pbot->is_bcplaylist = url.find(L"bandcamp.com/album/") != -1;
				pbot->is_bclink = url.find(L"bandcamp.com") != -1;
				pbot->is_bcchan = url.find(L".bandcamp.com/music") != -1 || url.rfind(L".bandcamp.com") == url.size() - 13
					|| url.rfind(L".bandcamp.com/") == url.size() - 14;
				if(gui->conf.gpopt_hidden && !gui->no_draw_freeze)
				{
					pbot->expcol.operate(true);
					pbot->plc.field_display("gpopt", false);
					pbot->plc.field_display("gpopt_spacer", false);
					pbot->plc.collocate();
				}
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
					}
					bot.com_args.caption(srcbot.com_args.caption());
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
		std::thread thr;
		bool working {false};
		GUI *pgui {nullptr};

	public:

		Outbox() : Textbox() {};

		Outbox(GUI *parent, bool visible = true)
		{
			create(parent, visible);
		}

		~Outbox()
		{
			if(thr.joinable())
			{
				working = false;
				thr.join();
			}
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

			events().dbl_click([&, this] (const nana::arg_mouse &arg)
			{
				if(arg.is_left_button())
				{
					pgui->show_queue();
				}
			});

			events().mouse_up([&, this](const nana::arg_mouse &arg)
			{
				using namespace nana;
				using ::widgets::theme;

				const auto &text {buffers[current_]};

				if(arg.button == mouse::right_button)
				{
					::widgets::Menu m;
					m.item_pixels(pgui->dpi_transform(24));

					m.append("Copy to clipboard", [&, this](menu::item_proxy)
					{
						util::set_clipboard_text(pgui->hwnd, to_wstring(text));

						if(!thr.joinable()) thr = std::thread([this]
						{
							working = true;
							auto bgclr {bgcolor()};
							auto blendclr {theme.is_dark() ? colors::dark_orange : colors::orange};
							std::vector<color> blended_colors;
							for(auto n {.1}; n > 0; n -= .01)
								blended_colors.push_back(bgcolor().blend(blendclr, n));
							int n {0};
							if(theme.is_dark())
							{
								for(auto i {blended_colors.size()/2}; i < blended_colors.size(); i++)
								{
									bgcolor(blended_colors[i]);
									Sleep(90 - n);
									n += 18;
								}
							}
							else for(const auto &clr : blended_colors)
							{
								bgcolor(clr);
								Sleep(80 - n);
								n += 8;
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

					if(!current_.empty() && pgui->lbq.at(pgui->lbq.selected().front()).text(3) != "error")
					{
						m.append("Copy command only", [&, this](menu::item_proxy)
						{
							auto pos {text.find_first_of("\r\n")};
							if(pos != -1)
							{
								auto pos2 {text.find(':') + 2};
								auto cmd {to_wstring(text.substr(pos2, pos-pos2))};
								util::set_clipboard_text(pgui->hwnd, cmd);
							}
						}).enabled(arg.window_handle == handle());

						m.append_splitter();
					}

					m.append("Keyword highlighting", [&, this](menu::item_proxy)
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
								p->fgcolor = theme.is_dark() ? theme.path_link_fg : color {"#569"};
								if(text.find("[GUI] ") != text.rfind("[GUI] "))
								{
									p = ca->get(text_line_count() - 2);
									p->count = 1;
									if(text.rfind("WM_CLOSE") == -1)
										p->fgcolor = theme.is_dark() ? theme.path_link_fg : color {"#569"};
									else p->fgcolor = theme.is_dark() ? color {"#f99"} : color {"#832"};
								}
							}
							nana::API::refresh_window(*this);
						}
					}).checked(conf.kwhilite);

					m.popup_await(*this, arg.pos.x, arg.pos.y);
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
	widgets::Overlay overlay {*this, &outbox};
	nana::panel<false> queue_panel {*this};
	nana::place plc_queue {queue_panel};
	widgets::Listbox lbq {queue_panel};
	widgets::Button btn_qact {queue_panel, "Queue actions", true}, btn_settings {queue_panel, "Settings", true};
	std::wstring qurl;
	widgets::path_label l_url {queue_panel, &qurl};
	std::unordered_map<std::string, favicon_t> favicons;

	void dlg_settings_info(nana::window owner);
	void dlg_json();
	void dlg_sections();
	void dlg_playlist();
	std::wstring pop_queue_menu(int x, int y);
	void make_columns_menu(nana::menu *m = nullptr);
	void make_queue_listbox();
	void dlg_formats();
	bool process_queue_item(std::wstring url);
	void dlg_settings();
	void dlg_changes(nana::window parent);
	void make_form();
	bool apply_theme(bool dark);
	void get_releases();
	void get_latest_ffmpeg();
	void get_latest_ytdlp();
	//void get_releases_misc(bool ytdlp_only = false);
	void get_versions();
	bool is_ytlink(std::wstring text);
	void change_field_attr(nana::place &plc, std::string field, std::string attr, unsigned new_val);
	bool is_tag_a_new_version(std::string tag_name) { return semver_t {tag_name} > semver_t {ver_tag}; }
	void dlg_updater(nana::window parent);
	void show_queue(bool freeze_redraw = true);
	void show_output();
	void add_url(std::wstring url);
	void taskbar_overall_progress();
	void on_btn_dl(std::wstring url);
	void remove_queue_item(std::wstring url);
	std::wstring next_startable_url(std::wstring current_url = L"current");

	bool lbq_has_scrollbar()
	{
		nana::paint::graphics g {{100, 100}};
		g.typeface(lbq.typeface());
		const auto text_height {g.text_extent_size("mm").height};
		const auto item_height {text_height + lbq.scheme().item_height_ex};
		if(lbq.at(0).size() * item_height > lbq.size().height - 25)
			return true;
		return false;
	}

	void adjust_lbq_headers()
	{
		using namespace util;

		auto zero  {scale(30)},
		     one   {scale(20 + !conf.col_site_text * 10) * conf.col_site_icon + scale(110) * conf.col_site_text},
		     three {scale(116)},
		     four  {scale(130) * lbq.column_at(4).visible() + scale(16) * lbq_has_scrollbar()},
		     five  {scale(150) * lbq.column_at(5).visible()},
		     six   {scale(60) * lbq.column_at(6).visible()},
		     seven {scale(90) * lbq.column_at(7).visible()};
		
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

	public:
		gui_bottoms &botref() { return bottoms; }
		unsigned res_options_size() { return com_res_options.size(); }
};
