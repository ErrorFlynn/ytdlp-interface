#pragma once

#include <chrono>
#include <unordered_set>
#include <nana/gui.hpp>
#include "util.hpp"
#include "icons.hpp"

struct version_t
{
	int year {0}, month {0}, day {0};
	version_t() = default;
	version_t(int y, int m, int d) : year {y}, month {m}, day {d} {}
	bool empty() const { return year == 0; }
	std::string string();
	bool operator > (const version_t &o);
	bool operator == (const version_t &o);
	bool operator != (const version_t &o);
};


struct semver_t
{
	int major {0}, minor {0}, patch {0};
	semver_t() = default;
	semver_t(int maj, int min, int pat) : major {maj}, minor {min}, patch {pat} {}
	semver_t(std::string ver_tag);

	bool operator > (const semver_t &o);
	bool operator < (const semver_t &o);
	bool operator == (const semver_t &o);
};


struct favicon_t
{
	using image = nana::paint::image;
	using callback = std::function<void(image &)>;

	favicon_t() = default;
	~favicon_t();
	void add(std::string favicon_url, callback fn);
	operator const image &() const { return img; }

private:
	image img;
	std::thread thr;
	std::mutex mtx;
	bool working {false};
	std::vector<callback> callbacks;
};


struct theme_t
{
	double shade {0}; // value that determines the contrast for some bg colors
	nana::color nimbus, fmbg, Label_fg, Text_fg, Text_fg_error, cbox_fg, btn_bg, btn_fg, path_fg, path_link_fg, lbfg,
		sep_bg, tbfg, tbkw, tbkw_id, tbkw_special, tbkw_warning, tbkw_error, gpbg, lb_headerbg, title_fg, overlay_fg, border,
		tb_selbg, tb_selbg_unfocused, expcol_fg, tree_selbg, tree_selfg, tree_hilitebg, tree_hilitefg, tree_selhilitebg, tree_selhilitefg,
		tree_parent_node, tree_expander, tree_expander_hovered, tree_bg, tree_key_fg, tree_val_fg, list_check_highlight_fg,
		list_check_highlight_bg, msg_label_fg, fmbg_raw, btn_bg_raw, lb_headerbg_raw, lbbg, gpfg_clr, lbselbg, lbselfg, lbhilite;
	std::string gpfg;

	bool operator==(theme_t const &) const = default;

	theme_t(bool dark = false) { make_default(dark); }
	void make_default(bool dark = false);
	void contrast(double factor, bool dark);
	void to_json(nlohmann::json &j);
	void from_json(const nlohmann::json &j);
};


struct settings_t
{
	std::filesystem::path ytdlp_path, ffmpeg_path, outpath;
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
		zoomed {false}, get_releases_at_startup {false}, col_format {false}, col_format_note {true}, col_ext {true}, col_fsize {false},
		json_hide_null {false}, col_site_icon {true}, col_site_text {false}, ytdlp_nightly {false}, audio_multistreams {false},
		cb_sblock_mark {false}, cb_sblock_remove {false}, cb_proxy {false}, cbsnap {true}, limit_output_buffer {true},
		update_self_only {true}, cb_premium {true}, cbminw {false}, cb_save_errors {false}, cb_ffplay {false}, cb_clear_done {false},
		cb_formats_fsize_bytes {false}, cb_add_on_focus {false}, cb_custom_dark_theme {false}, cb_custom_light_theme {false},
		cb_android {false};
	nana::rectangle winrect;
	int dpi {96};
	std::vector<int> sblock_mark, sblock_remove;
	std::wstring url_passed_as_arg;
	theme_t theme_dark {true}, theme_light;
};