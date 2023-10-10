#pragma once

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
#include "types.hpp"

#undef min
#undef max

namespace fs = std::filesystem;
constexpr int YTDLP_DOWNLOAD {0}, YTDLP_POSTPROCESS {1};
constexpr bool X64 {INTPTR_MAX == INT64_MAX};

class GUI : public themed_form, IDropTarget
{
public:

	GUI();

	static struct settings_t
	{
		fs::path ytdlp_path, outpath;
		const std::wstring output_template_default {L"%(title)s.%(ext)s"}, playlist_indexing_default {L"%(playlist_index)d - "},
			output_template_default_bandcamp {L"%(artist)s - %(album)s - %(track_number)02d - %(track)s.%(ext)s"};
		std::wstring fmt1, fmt2, output_template {output_template_default}, playlist_indexing {playlist_indexing_default},
			output_template_bandcamp {output_template_default_bandcamp}, proxy;
		std::string argset;
		std::vector<std::string> argsets, unfinished_queue_items;
		std::unordered_set<std::wstring> outpaths;
		std::map<std::wstring, std::string> playsel_strings;
		double ratelim {0}, contrast {.1};
		unsigned ratelim_unit {1}, pref_res {0}, pref_video {0}, pref_audio {0}, cbtheme {2}, max_argsets {10}, max_outpaths {10}, 
			max_concurrent_downloads {1}, output_buffer_size {30000}, pref_vcodec {0}, pref_acodec {0};
		std::chrono::milliseconds max_proc_dur {3000};
		bool cbsplit {false}, cbchaps {false}, cbsubs {false}, cbthumb {false}, cbtime {true}, cbkeyframes {false}, cbmp3 {false},
			cbargs {false}, kwhilite {true}, pref_fps {false}, cb_lengthyproc {true}, common_dl_options {true}, cb_autostart {true},
			cb_queue_autostart {false}, gpopt_hidden {false}, open_dialog_origin {false}, cb_zeropadding {true}, cb_playlist_folder {true},
			zoomed {false}, get_releases_at_startup {true}, col_format {false}, col_format_note {true}, col_ext {true}, col_fsize {false},
			json_hide_null {false}, col_site_icon {true}, col_site_text {false}, ytdlp_nightly {false}, audio_multistreams {false},
			cb_sblock_mark {false}, cb_sblock_remove {false}, cb_proxy {false}, cbsnap {true}, limit_output_buffer {true}, 
			update_self_only {true};
		nana::rectangle winrect;
		int dpi {96};
		std::vector<int> sblock_mark, sblock_remove;
	}
	conf;

	fs::path confpath;
	std::function<bool()> fn_write_conf;

private:

	nlohmann::json releases;
	fs::path self_path, appdir, ffmpeg_loc;
	std::string inet_error, url_latest_ffmpeg, url_latest_ytdlp, url_latest_ytdlp_relnotes;
	std::wstring drop_cliptext_temp;
	std::wstringstream multiple_url_text;
	long minw {0}, minh {0};
	unsigned size_latest_ffmpeg {0}, size_latest_ytdlp {0}, number_of_processors {4};
	bool menu_working {false}, lbq_no_action {false}, thumbthr_working {false}, 
		autostart_next_item {true}, lbq_can_drag {false}, cnlang {false}, no_draw_freeze {true};
	std::thread thr, thr_releases, thr_versions, thr_thumb, thr_menu, thr_releases_ffmpeg, thr_releases_ytdlp, thr_update;
	CComPtr<ITaskbarList3> i_taskbar;
	UINT WM_TASKBAR_BUTTON_CREATED {0};
	const std::string ver_tag {"v2.7.0"}, title {"ytdlp-interface " + ver_tag/*.substr(0, 4)*/},
		ytdlp_fname {X64 ? "yt-dlp.exe" : "yt-dlp_x86.exe"};
	const unsigned MINW {900}, MINH {700}; // min client area size
	nana::drawerbase::listbox::item_proxy *last_selected {nullptr};
	nana::timer tproc;

	struct { nana::menu *m {nullptr}; std::size_t pos {0}; } vidsel_item;

	const std::vector<std::wstring>
		com_res_options {L"none", L"4320", L"2160", L"1440", L"1080", L"720", L"480", L"360"},
		com_audio_options {L"none", L"m4a", L"mp3", L"ogg", L"webm", L"flac"},
		com_video_options {L"none", L"mp4", L"webm"},
		com_vcodec_options {L"none", L"av01", L"vp9.2", L"vp9", L"h265", L"h264", L"vp8", L"h263", L"theora"},
		com_acodec_options {L"none", L"flac", L"alac", L"wav", L"aiff", L"opus", L"vorbis", L"aac", L"mp4a", 
			L"mp3", L"ac4", L"eac3", L"ac3", L"dts"},
		sblock_cats_mark {L"all", L"sponsor", L"intro", L"outro", L"selfpromo", L"preview", L"filler", L"interaction", L"music_offtopic",
			L"poi_highlight", L"chapter"},
		sblock_cats_remove {L"all", L"sponsor", L"intro", L"outro", L"selfpromo", L"preview", L"filler", L"interaction", L"music_offtopic"};

	
	version_t ver_ffmpeg, ver_ffmpeg_latest, ver_ytdlp, ver_ytdlp_latest;
	semver_t cur_ver {ver_tag};

	class gui_bottom : public nana::panel<true>
	{
		GUI *pgui {nullptr};
	public:
		gui_bottom(GUI &gui, bool visible = false);

		bool is_ytlink {false}, use_strfmt {false}, working {false}, graceful_exit {false}, working_info {true}, received_procmsg {false},
			is_ytplaylist {false}, is_ytchan {false}, is_bcplaylist {false}, is_bclink {false}, is_bcchan {false}, is_yttab {false};
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
		bool started() { return btndl.caption().starts_with("Stop"); }
		bool using_custom_fmt() { return cbargs.checked() && com_args.caption_wstring().find(L"-f ") != -1; }
		int playlist_selected();
		bool vidinfo_contains(std::string key);
		void apply_playsel_string();
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
		gui_bottom &at(int idx);
		auto url_at(int idx);
		gui_bottom &current() { return at(visible()); }
		auto begin() noexcept { return bottoms.begin(); }
		const auto begin() const noexcept { return bottoms.begin(); }
		auto rbegin() noexcept { return bottoms.rbegin(); }
		auto end() noexcept { return bottoms.end(); }
		auto rend() noexcept { return bottoms.rend(); }
		const auto end() const noexcept { return bottoms.end(); }
		auto size() const noexcept { return bottoms.size(); }
		gui_bottom *back() const;

		void show(std::wstring key);
		std::wstring visible();
		gui_bottom &add(std::wstring url, bool visible = false);
		void erase(std::wstring url);
		void propagate_cb_options(const gui_bottom &srcbot);
		void propagate_args_options(const gui_bottom &srcbot);
		void propagate_misc_options(const gui_bottom &srcbot);
	};


	class Outbox : public widgets::Textbox
	{
		std::map<std::wstring, std::string> buffers {{L"", ""}};
		std::map<std::wstring, std::string> commands;
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

		void create(GUI *parent, bool visible = true);
		void show(std::wstring url);
		void erase(std::wstring url);
		void append(std::wstring url, std::string text);
		void caption(std::string text, std::wstring url = L"");
		auto caption() const { return textbox::caption(); }
		const auto &current_buffer() { return buffers[current_]; }
		void current(std::wstring url) { current_ = url; }
		auto current() { return current_; }
		void clear(std::wstring url = L"");

		void append(std::wstring url, std::wstring text)
		{
			append(url, nana::to_utf8(text));
		}

		void caption(std::wstring text, std::wstring url = L"")
		{
			caption(nana::to_utf8(text), url);
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

	widgets::conf_page updater;	
	widgets::Label l_ver, l_ver_ytdlp, l_ver_ffmpeg, l_channel;
	widgets::Text l_vertext, l_ytdlp_text, l_ffmpeg_text;
	widgets::Button btn_changes, btn_update, btn_update_ytdlp, btn_update_ffmpeg;
	widgets::cbox cb_startup, cb_selfonly, cb_chan_stable, cb_chan_nightly;
	nana::radio_group rgp_chan;
	widgets::Progress prog_updater, prog_updater_misc;
	widgets::Separator sep1, sep2;
	nana::timer updater_t0, updater_t1, updater_t2;
	bool updater_working {false};

	void updater_init_page();
	void updater_display_version();
	void updater_display_version_ffmpeg();
	void updater_display_version_ytdlp();
	void updater_update_self(themed_form &parent);
	void updater_update_misc(bool ytdlp, fs::path target = "");
	void fm_settings();
	void fm_settings_info(nana::window owner);
	void fm_changes(nana::window parent);
	void fm_json();
	void fm_sections();
	void fm_playlist();
	void fm_formats();

	void queue_remove_all();
	void queue_remove_selected();
	void make_queue_listbox();
	std::wstring pop_queue_menu(int x, int y);

	void make_columns_menu(nana::menu *m = nullptr);
	bool process_queue_item(std::wstring url);
	void make_form();
	bool apply_theme(bool dark);
	void get_releases();
	void get_latest_ffmpeg();
	void get_latest_ytdlp();
	void get_versions();
	void get_version_ytdlp();
	bool is_ytlink(std::wstring url);
	bool is_ytchan(std::wstring url);
	void make_updater_page(themed_form &parent);
	void change_field_attr(nana::place &plc, std::string field, std::string attr, unsigned new_val);
	bool is_tag_a_new_version(std::string tag_name) { return semver_t {tag_name} > semver_t {ver_tag}; }
	void show_queue(bool freeze_redraw = true);
	void show_output();
	void add_url(std::wstring url, bool refresh = false);
	void taskbar_overall_progress();
	void on_btn_dl(std::wstring url);
	void remove_queue_item(std::wstring url);
	std::wstring next_startable_url(std::wstring current_url = L"current");
	bool lbq_has_scrollbar();
	void adjust_lbq_headers();
	void write_settings() { events().unload.emit({}, *this); }

	std::unordered_map<std::string, std::pair<std::string, std::string>> sblock_infos
	{
		{"all", {"All", "Select this to indicate all categories."}},
		{"sponsor", {"Sponsor", "Segments promoting a product or service not directly related to the creator."}},

		{"intro", {"Intermission/Intro Animation", "Segments typically found at the start of a video that include an animation, "
		          "still frame or clip which are also seen in other videos by the same creator. This can include livestream pauses "
		          "with no content\n(looping animations or chat windows) and Copyright / Fair Use disclaimers."}},

		{"outro", {"Endcards/Credits (Outro)", "Typically near or at the end of the video when the credits pop up and/or endcards are shown."}},

		{"selfpromo", {"Unpaid/Self Promotion", "Segments promoting a product or service that is directly related to the creator themselves.\n"
		              "This usually includes merchandise or promotion of monetized platforms."}},

		{"preview", {"Preview/Recap", "Collection of clips that show what is coming up in in this video or other videos in a series."}},

		{"filler", {"Filler Tangent/Jokes", "Tangential scenes added only for filler or humor, that are not required to understand the main "
		           "content of the video. This can also include: Timelapses / B-Roll, Fake Sponsors and slow-motion clips that do not provide "
		           "any context or are used as replays or B-roll."}},

		{"interaction", {"Interaction Reminder (Subscribe)", "Explicit reminders to like, subscribe or interact with them on any paid or "
			            "free platform(s)\n(e.g. click on a video)."}},

		{"music_offtopic", {"Music: Non-Music Section", "Section devoid of music in videos which feature music as the primary content."}},
		{"poi_highlight", {"Highlight", "A point of interest in the video, possibly the most important part of the video."}},
		{"chapter", {"Chapter", "Chapters designated by SponsorBlock (presumably in a video that doesn't have chapters otherwise)."}}
	};

	public:

	gui_bottoms &botref() { return bottoms; }
	unsigned res_options_size() { return com_res_options.size(); }


	HRESULT STDMETHODCALLTYPE DragEnter(IDataObject*, DWORD, POINTL, DWORD*) override;
	HRESULT STDMETHODCALLTYPE DragLeave() override;
	HRESULT STDMETHODCALLTYPE DragOver(DWORD, POINTL, DWORD*) override { return S_OK; }
	HRESULT STDMETHODCALLTYPE Drop(IDataObject*, DWORD, POINTL, DWORD*) override;
	ULONG STDMETHODCALLTYPE AddRef() override { return S_OK; }
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID, void**) override { return S_OK; }
	ULONG STDMETHODCALLTYPE Release() override { return S_OK; }
};
