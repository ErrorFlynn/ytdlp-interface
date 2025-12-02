#include "../gui.hpp"
#include <regex>
#include <nana/gui/filebox.hpp>
#include <nana/system/platform.hpp>
#include <nana/paint/image_process_selector.hpp>


class about_label : public widgets::Label
{
public:
	about_label(nana::window parent, std::string_view text) : Label {parent, text}
	{
		text_align(nana::align::center, nana::align_v::top);
		format(true);
	}
};

void GUI::fm_settings()
{
	using widgets::theme;
	static std::string start_page {"ytdlp"};
	unsigned initial_maxdl {conf.max_concurrent_downloads};

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.center(820, 650);
	fm.caption(title + " - settings");
	fm.bgcolor(theme::fmbg);
	fm.snap(conf.cbsnap);

	fm.div("vert margin=20 < <tree weight=150> <weight=20> <switchable <ytdlp> <sblock> <queuing> <gui> <updater> <presets> <about>> >");

	fm.subclass_after(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == 'S')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
				fm.close();
		}
		return true;
	});

	widgets::conf_page ytdlp {fm}, queuing {fm}, gui {fm}, sblock {fm}, about {fm}, presets {fm};
	updater.create(fm);
	make_updater_page(fm);

	std::function<void(void)> update_conf, update_preset_widgets, update_ytdlp_page;

	auto page_callback = [&] (std::string name)
	{
		if(name == "updater")
		{
			updater_check_paths(false);
			if(!cb_chan_stable.checked() && !cb_chan_nightly.checked())
				updater_init_page(fm);
			else updater_display_version_ffmpeg();
			if(conf.ytdlp_path.filename().string() == "ytdl-patched-red.exe")
			{
				cb_chan_stable.enabled(false);
				cb_chan_nightly.enabled(false);
			}
			else
			{
				cb_chan_stable.enabled(true);
				cb_chan_nightly.enabled(true);
			}
			nana::api::refresh_window(updater);
		}
		else if(name == "presets")
		{
			if(update_conf) update_conf();
		}
	};

	widgets::conf_tree tree {fm, &fm.get_place(), page_callback};
	tree.add("yt-dlp", "ytdlp");
	tree.add("SponsorBlock", "sblock");
	tree.add("Queuing", "queuing");
	tree.add("Interface", "gui");
	tree.add("Updater", "updater");
	tree.add("Config presets", "presets");
	tree.add("About", "about");

	fm["tree"] << tree;
	fm["ytdlp"] << ytdlp;
	fm["sblock"] << sblock;
	fm["queuing"] << queuing;
	fm["gui"] << gui;
	fm["updater"] << updater;
	fm["presets"] << presets;
	fm["about"] << about;

	about.div(R"(vert
		<pnl_header weight=35> <weight=10> <l_about_ver weight=50> <weight=15>
		<about_sep1 weight=3> <weight=20> <libtitle weight=28> <weight=15>
		<weight=28 <l_nana> <weight=20> <l_nana_ver>>
		<weight=28 <l_json> <weight=20> <l_json_ver>>
		<weight=28 <l_jpeg> <weight=20> <l_jpeg_ver>>
		<weight=28 <l_png> <weight=20> <l_png_ver>>
		<weight=28 <l_bit7z> <weight=20> <l_bit7z_ver>> <weight=18>
		<about_sep2 weight=3> <weight=20> <kbtitle weight=28> <weight=15>
		<weight=28 <l_ctrls> <weight=20> <l_settings>>
		<weight=28 <l_ctrlf> <weight=20> <l_formats>>
		<weight=28 <l_ctrltab> <weight=20> <l_view>>
		<weight=28 <l_f2> <weight=20> <l_fname>>
		<weight=28 <l_del> <weight=20> <l_delitem>>
		<weight=28 <l_ctrlnum0> <weight=20> <l_winpos>>
		<weight=28 <l_esc> <weight=20> <l_close>>
	)");

	std::string vertext {ver_tag + " (" + (X64 ? "64-bit)" : "32-bit)") +
		"\n<color=0x url=\"https://github.com/ErrorFlynn/ytdlp-interface\">https://github.com/ErrorFlynn/ytdlp-interface</>"};
	std::string about_text {""};

	about_label l_about_ver {about, ""};
	nana::panel<true> pnl_header {about.handle()};
	widgets::Separator about_sep1 {about}, about_sep2 {about};
	widgets::Title libtitle {about, nana::to_string(u8"\u2606  Libraries used  \u2606")}, 
		kbtitle {about, nana::to_string(u8"\u2606  Keyboard shortcuts  \u2606")};
	widgets::Label l_nana {about, "Nana C++ GUI library"}, l_jpeg {about, "libjpeg-turbo"}, l_bit7z {about, "bit7z"},
		l_png {about, "libpng"}, l_json {about, "JSON for Modern C++"}, l_ctrls {about, "Ctrl+S"}, l_ctrlf {about, "Ctrl+F"},
		l_ctrltab {about, "Ctrl+Tab"}, l_f2 {about, "F2"}, l_del {about, "Delete"}, l_esc {about, "Esc"}, l_ctrlnum0 {about, "Ctrl+Num0"};
	widgets::Text l_nana_ver {about, "v1.8 (custom)"}, l_jpeg_ver {about, "v2.1.5.1"}, l_bit7z_ver {about, "v3.1.3"},
		l_png_ver {about, "v1.6.37"}, l_json_ver {about, "v3.12.0"}, l_settings {about, "Settings"}, l_formats {about, "Formats"},
		l_view {about, "Switch view (queue/output)"}, l_fname {about, "Set file name of queue item"}, 
		l_delitem {about, "Delete queue item(s)"}, l_close {about, "Close window"}, l_winpos {about, "Reset window size and position"};

	about["title"] << title;
	about["pnl_header"] << pnl_header;
	about["l_about_ver"] << l_about_ver;
	about["about_sep1"] << about_sep1;
	about["libtitle"] << libtitle;
	about["l_nana"] << l_nana;
	about["l_nana_ver"] << l_nana_ver;
	about["l_json"] << l_json;
	about["l_json_ver"] << l_json_ver;
	about["l_jpeg"] << l_jpeg;
	about["l_jpeg_ver"] << l_jpeg_ver;
	about["l_png"] << l_png;
	about["l_png_ver"] << l_png_ver;
	about["l_bit7z"] << l_bit7z;
	about["l_bit7z_ver"] << l_bit7z_ver;
	about["about_sep2"] << about_sep2;
	about["kbtitle"] << kbtitle;
	about["l_ctrls"] << l_ctrls;
	about["l_settings"] << l_settings;
	about["l_ctrlf"] << l_ctrlf;
	about["l_formats"] << l_formats;
	about["l_ctrltab"] << l_ctrltab;
	about["l_view"] << l_view;
	about["l_f2"] << l_f2;
	about["l_fname"] << l_fname;
	about["l_del"] << l_del;
	about["l_delitem"] << l_delitem;
	about["l_esc"] << l_esc;
	about["l_close"] << l_close;
	about["l_ctrlnum0"] << l_ctrlnum0;
	about["l_winpos"] << l_winpos;

	libtitle.format(true);

	nana::drawing {pnl_header}.draw([&](nana::paint::graphics &g)
	{
		using namespace nana;
		g.rectangle(true, theme::fmbg);
		paint::graphics g_str {g.size()};
		g_str.rectangle(true, theme::fmbg);
		g_str.typeface(paint::font_info {"Candara Bold Italic", 20});
		g_str.palette(true, theme::title_fg.blend(theme::is_dark() ? colors::black : colors::white, .05));
		g_str.string({}, "ytdlp-interface");
		auto tsize {g_str.text_extent_size("ytdlp-interface")};
		paint::image_process::selector().stretch("proximal interpolation");
		g_str.stretch(rectangle {{}, tsize}, g, rectangle {{int(g.width()/2 - (tsize.width*2)/2), 0}, {tsize.width*2, tsize.height}});
		paint::image_process::selector().stretch("bilinear interpolation");
	});

	widgets::Label l_res {ytdlp, "Preferred resolution:"}, l_vcodec {ytdlp, "Preferred video codec:"}, l_acodec {ytdlp, "Preferred audio codec:"},
		l_video {ytdlp, "Preferred video container:"}, l_audio {ytdlp, "Preferred audio container:"}, l_theme {gui, "Color theme:"}, l_contrast {gui, "Contrast:"}, 
		l_template {ytdlp, "Output template:"}, l_maxdl {queuing, "Max concurrent downloads:"}, l_playlist {ytdlp, "Playlist indexing:"},
		l_opendlg_origin {gui, "When browsing for the output folder, start in:"}, l_sblock {sblock, ""},
		l_cookies {ytdlp, "Load cookies from browser:"}, l_cookie_options {ytdlp, "Additional options:"},
		l_maxinfo {queuing, "Max number of concurrent yt-dlp instances used for getting data:"};

	widgets::path_label l_cookies_path {ytdlp, &conf.cookies_path};
	widgets::Textbox tb_template {ytdlp}, tb_playlist {ytdlp}, tb_proxy {ytdlp}, tb_cookies {ytdlp}, tb_aria {ytdlp};
	widgets::Combox com_res {ytdlp}, com_video {ytdlp}, com_audio {ytdlp}, com_vcodec {ytdlp}, com_acodec {ytdlp}, com_cookies {ytdlp};
	widgets::cbox cbfps {ytdlp, "Prefer a higher framerate"}, cbtheme_dark {gui, "Dark"}, cbtheme_light {gui, "Light"},
		cbtheme_system {gui, "System preference"}, cb_lengthyproc {queuing, "Start next item on lengthy processing"},
		cb_common {queuing, "Each queue item has its own download options"},
		cb_autostart {queuing, "When stopping a queue item, automatically start the next one"},
		cb_queue_autostart {queuing, "When the program starts, automatically start processing the queue"},
		cb_zeropadding {ytdlp, "Pad the indexed filenames with zeroes"}, cb_playlist_folder {ytdlp, "Put playlists in their own folders"},
		cb_origin_progdir {gui, "Program folder"}, cb_origin_curdir {gui, "Currently selected folder"},
		cb_mark {sblock, "Mark these categories:"}, cb_remove {sblock, "Remove these categories:"}, cb_proxy {ytdlp, "Use this proxy:"},
		cbsnap {gui, "Snap windows to screen edges"}, cbminw {gui, "No minimum width for the main window"},
		cb_premium {ytdlp, "[YouTube] For 1080p, prefer the \"premium\" format with enhanced bitrate"},
		cb_android {ytdlp, "[YouTube] Use the Android player client for video extraction"},
		cb_save_errors {queuing, "Save queue items with \"error\" status to the settings file"},
		cb_clear_done {queuing, "Automatically remove completed items (with \"done\" status)"},
		cb_formats_fsize_bytes {gui, "Formats window: display file sizes with exact byte value"},
		cb_add_on_focus {queuing, "When the main window is activated, automatically add the URL from clipboard"},
		cb_display_custom_filenames {queuing, "Display any custom file names in the \"Media title\" column"},
		cb_aria {ytdlp, "Tell yt-dlp to download with aria2c  (--downloader aria2c)"}, cb_cookies {ytdlp, "Load cookies from file:"};
	widgets::Separator sep1 {ytdlp}, sep2 {ytdlp}, sep3 {gui}, sep_aria {ytdlp, "aria2c integration"};
	widgets::Button btn_close {fm, " Close"}, btn_default {ytdlp, "Reset to default", true},
		btn_playlist_default {ytdlp, "Reset to default", true}, btn_info {ytdlp};
	widgets::Spinbox sb_maxdl {queuing}, sb_maxinfo {queuing};
	widgets::Slider slider {gui};
	widgets::sblock_listbox lbmark {sblock}, lbremove {sblock};
	widgets::Infobox l_info {sblock};
	widgets::Label l_aria_present {ytdlp, ""}, l_aria_options {ytdlp, "Options for aria2c (--downloader-args):"};
	l_aria_present.text_align(nana::align::left, nana::align_v::center);
	nana::picture pic_aria_present {ytdlp};
	pic_aria_present.align(nana::align::center, nana::align_v::center);
	nana::api::effects_bground(pic_aria_present, nana::effects::bground_transparent(0), 0.0);

	ytdlp.div(R"(vert
		<weight=26 <weight=42> <l_res weight=156> <weight=10> <com_res weight=56> <> 
			<cbfps weight=228> <btn_info weight=26>> <weight=18>
		<weight=25
			<l_video weight=198> <weight=10> <com_video weight=56> <>
			<l_audio weight=200> <weight=10> <com_audio weight=66>
		> <weight=18>
		<weight=26
			<l_vcodec weight=198> <weight=10> <com_vcodec weight=56> <>
			<l_acodec weight=198> <weight=10> <com_acodec weight=66>
		>
		<spacer_premium weight=18> <row_premium weight=26 <prem_pad weight=36><cb_premium>>
		<weight=18> <sep1 weight=3> <weight=18>
		<weight=25 <l_template weight=132> <weight=10> <tb_template> <weight=15> <btn_default weight=140>> <weight=18>
		<weight=25 <l_playlist weight=132> <weight=10> <tb_playlist> <weight=15> <btn_playlist_default weight=140>> <weight=18>
		<weight=25 <cb_zeropadding weight=323> <weight=18> <cb_playlist_folder>>
		<weight=18> <sep2 weight=3> <weight=18>
		<weight=25 <> <cb_proxy weight=125> <weight=10> <tb_proxy weight=468> > <weight=18>
		<weight=25 <l_cookies weight=200> <weight=10> <com_cookies weight=80> <weight=10> <l_cookie_options weight=140> <weight=10> <tb_cookies>>
		<weight=18> <weight=25 <weight=5> <cb_cookies weight=180> <weight=10> <l_cookies_path>>
		<weight=20> <sep_aria weight=3> <weight=20>
		<weight=25 <pic_aria_present weight=25> <aria_spacer weight=5> <l_aria_present>> <weight=18>
		<weight=25 <weight=5> <cb_aria>> <weight=17>
		<weight=25 <l_aria_options weight=283> <weight=10> <tb_aria>>
	)");

	ytdlp["l_res"] << l_res;
	ytdlp["com_res"] << com_res;
	ytdlp["l_vcodec"] << l_vcodec;
	ytdlp["l_acodec"] << l_acodec;
	ytdlp["com_vcodec"] << com_vcodec;
	ytdlp["com_acodec"] << com_acodec;
	ytdlp["cbfps"] << cbfps;
	ytdlp["btn_info"] << btn_info;
	ytdlp["l_video"] << l_video;
	ytdlp["com_video"] << com_video;
	ytdlp["l_audio"] << l_audio;
	ytdlp["com_audio"] << com_audio;
	ytdlp["cb_premium"] << cb_premium;
	ytdlp["cb_android"] << cb_android;
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
	ytdlp["l_cookies"] << l_cookies;
	ytdlp["com_cookies"] << com_cookies;
	ytdlp["l_cookie_options"] << l_cookie_options;
	ytdlp["tb_cookies"] << tb_cookies;
	ytdlp["sep_aria"] << sep_aria;
	ytdlp["l_aria_present"] << l_aria_present;
	ytdlp["pic_aria_present"] << pic_aria_present;
	ytdlp["cb_aria"] << cb_aria;
	ytdlp["l_aria_options"] << l_aria_options;
	ytdlp["tb_aria"] << tb_aria;
	ytdlp["cb_cookies"] << cb_cookies;
	ytdlp["l_cookies_path"] << l_cookies_path;

	cb_cookies.events().checked([&]
	{
		if(cb_cookies.checked())
			com_cookies.option(0);
	});

	com_cookies.events().selected([&]
	{
		if(com_cookies.option() != 0)
			cb_cookies.check(false);
	});

	l_cookies_path.events().click([&]
	{
		nana::filebox fb {ytdlp, true};
		if(!conf.cookies_path.empty())
		{
			if(conf.cookies_path.string().find(".\\") == 0)
				fb.init_file(appdir / conf.cookies_path.filename());
			else fb.init_file(conf.cookies_path);
		}
		else fb.init_path(appdir);
		fb.allow_multi_select(false);
		fb.add_filter("Cookies file", "*.txt;*.*");
		fb.title("Locate and select a text file that contains cookies exported from a browser");
		auto res {fb()};
		if(res.size())
		{
			auto path {res.front()};
			auto fname {path.filename().string()};
			conf.cookies_path = util::to_relative_path(path);
			l_cookies_path.update_caption();
		}
	});

	update_ytdlp_page = [&]
	{
		const auto dpi {nana::api::screen_dpi(true)};
		std::error_code ec;
		bool aria_in_ytdlp_dir {fs::exists(conf.ytdlp_path.parent_path() / "aria2c.exe")},
			aria_in_sys_path {false};
		if(!aria_in_ytdlp_dir)
			aria_in_sys_path = util::is_file_in_sys_path(L"aria2c.exe");
		if(aria_in_ytdlp_dir || aria_in_sys_path)
		{
			pic_aria_present.stretchable(false);
			nana::paint::image img;
			if(dpi >= 216)
			{
				ytdlp.get_place().field_display("aria_spacer", true);
				img.open(arr_checkmark64_png, sizeof arr_checkmark64_png);
			}
			else if(dpi >= 192)
			{
				ytdlp.get_place().field_display("aria_spacer", false);
				img.open(arr_checkmark48_png, sizeof arr_checkmark48_png);
			}
			else if(dpi >= 168)
			{
				ytdlp.get_place().field_display("aria_spacer", true);
				img.open(arr_checkmark48_png, sizeof arr_checkmark48_png);
			}
			else if(dpi >= 144)
			{
				ytdlp.get_place().field_display("aria_spacer", false);
				img.open(arr_checkmark32_png, sizeof arr_checkmark32_png);
			}
			else if(dpi > 96)
			{
				ytdlp.get_place().field_display("aria_spacer", true);
				img.open(arr_checkmark32_png, sizeof arr_checkmark32_png);
			}
			else 
			{
				ytdlp.get_place().field_display("aria_spacer", false);
				img.open(arr_checkmark22_png, sizeof arr_checkmark22_png);
			}
			ytdlp.get_place().collocate();
			pic_aria_present.load(img);
			if(aria_in_ytdlp_dir)
				l_aria_present.caption("aria2c.exe found in the yt-dlp folder");
			else l_aria_present.caption("aria2c.exe found in the %path% system variable");
		}
		else
		{
			nana::paint::image img;
			if(dpi == 120)
				pic_aria_present.stretchable(true);
			else pic_aria_present.stretchable(false);

			if(dpi >= 216)
				img.open(arr_info64_png, sizeof arr_info64_png);
			else if(dpi >= 192)
				img.open(arr_info48_png, sizeof arr_info48_png);
			else if(dpi >= 168)
				img.open(arr_info48_png, sizeof arr_info48_png);
			else if(dpi > 96)
				img.open(arr_info32_png, sizeof arr_info32_png);
			else img.open(arr_info22_ico, sizeof arr_info22_ico);
			pic_aria_present.load(img);
			l_aria_present.caption("aria2c.exe not found in the yt-dlp folder or the %path% system variable");
			ytdlp.get_place().field_display("aria_spacer", true);
			ytdlp.get_place().collocate();
		}
	};

	fm.events().focus([&](const nana::arg_focus &arg)
	{
		if(arg.getting && update_ytdlp_page)
			update_ytdlp_page();
	});

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

	tb_aria.multi_lines(false);
	tb_aria.padding(0, 5, 0, 5);
	tb_aria.typeface(nana::paint::font_info {"Tahoma", 10});
	
	tb_proxy.multi_lines(false);
	tb_proxy.padding(0, 5, 0, 5);
	tb_proxy.typeface(nana::paint::font_info {"Tahoma", 10});

	tb_cookies.padding(4, 5, 0, 5);
	tb_cookies.typeface(nana::paint::font_info {"Tahoma", 10});
	tb_cookies.multi_lines(false);

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
	cb_mark.tooltip("yt-dlp will create chapters for the segments in these categories\n(this passes <bold>--sponsorblock-mark</> to yt-dlp)");
	cb_remove.tooltip("yt-dlp will remove the segments in these categories\n(this passes <bold>--sponsorblock-remove</> to yt-dlp)");

	size_t hovitem_mark {nana::npos};
	lbmark.events().mouse_move([&](const nana::arg_mouse &arg)
	{
		auto hovered {lbmark.cast({arg.pos.x, arg.pos.y})};
		if(hovered.item != -1 && hovered.item != hovitem_mark)
		{
			hovitem_mark = hovered.item;
			l_info.caption(sblock_infos[lbmark.at(hovered).value<std::string>()].second);
		}
	});
	lbmark.events().mouse_leave([&](const nana::arg_mouse &arg)
	{
		l_info.caption("");
		hovitem_mark = nana::npos;
	});
	lbremove.events().mouse_move([&](const nana::arg_mouse &arg)
	{
		auto hovered {lbremove.cast({arg.pos.x, arg.pos.y})};
		if(hovered.item != -1 && hovered.item != hovitem_mark)
		{
			hovitem_mark = hovered.item;
			l_info.caption(sblock_infos[lbremove.at(hovered).value<std::string>()].second);
		}
	});
	lbremove.events().mouse_leave([&](const nana::arg_mouse &arg)
	{
		l_info.caption("");
		hovitem_mark = nana::npos;
	});

	std::string link_dark {"0xb0c0d0"}, link_light {"0x688878"}, link_hilited_dark {"0xd5e5f5"}, link_hilited_light {"0x436353"};
	bool sblock_hilite {false};
	l_sblock.events().mouse_move([&](const nana::arg_mouse &arg)
	{
		if(l_sblock.cursor() == nana::cursor::hand)
		{
			if(!sblock_hilite)
			{
				sblock_hilite = true;
				l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme::is_dark() ? link_hilited_dark : link_hilited_light));
			}
		}
		else if(sblock_hilite)
		{
			sblock_hilite = false;
			l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme::is_dark() ? link_dark : link_light));
		}
	});
	l_sblock.events().mouse_leave([&]
	{
		if(sblock_hilite)
		{
			sblock_hilite = false;
			l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme::is_dark() ? link_dark : link_light));
		}
	});

	queuing.div(R"(vert		
		<weight=25 <l_maxdl weight=196> <weight=10> <sb_maxdl weight=40> <> <cb_lengthyproc weight=290>> <weight=20>
		<weight=25 <l_maxinfo weight=453> <weight=10> <sb_maxinfo weight=40>> <weight=20>
		<weight=25 <cb_autostart weight=508>> <weight=20>
		<weight=25 <cb_common weight=408>> <weight=20>
		<weight=25 <cb_queue_autostart>> <weight=20>
		<weight=25 <cb_save_errors>> <weight=20>
		<weight=25 <cb_clear_done>> <weight=20>
		<weight=25 <cb_add_on_focus>> <weight=20>
		<weight=25 <cb_display_custom_filenames>>
	)");

	l_maxdl.text_align(nana::align::left, nana::align_v::center);
	l_maxinfo.text_align(nana::align::left, nana::align_v::center);

	queuing["l_maxdl"] << l_maxdl;
	queuing["sb_maxdl"] << sb_maxdl;
	queuing["l_maxinfo"] << l_maxinfo;
	queuing["sb_maxinfo"] << sb_maxinfo;
	queuing["cb_lengthyproc"] << cb_lengthyproc;
	queuing["cb_autostart"] << cb_autostart;
	queuing["cb_common"] << cb_common;
	queuing["cb_queue_autostart"] << cb_queue_autostart;
	queuing["cb_save_errors"] << cb_save_errors;
	queuing["cb_clear_done"] << cb_clear_done;
	queuing["cb_add_on_focus"] << cb_add_on_focus;
	queuing["cb_display_custom_filenames"] << cb_display_custom_filenames;

	gui.div(R"(vert		
		<weight=25 <l_theme weight=100> <weight=20> <cbtheme_dark weight=65> <weight=20> 
			<cbtheme_light weight=66> <weight=20> <cbtheme_system weight=178> > <weight=20>
		<weight=25 <l_contrast weight=70> <weight=20> <slider> >
		<weight=20>
		<
			weight=25 <switchable weight=230 <cb_custom_dark_theme> <cb_custom_light_theme>>
			<weight=20> <btn_edit_theme weight=155>
		>
		<weight=20> <sep3 weight=3> <weight=20>
		<weight=25 <cbsnap> <>> <weight=20> <weight=25 <cbminw weight=75%> <>> <weight=20>
		<weight=25 <cb_formats_fsize_bytes>> <weight=20>
		<weight=25 <l_opendlg_origin weight=345> <cb_origin_curdir>> <weight=10>
		<weight=25 <opendlg_spacer weight=345> <cb_origin_progdir>> <weight=20>
	)");

	widgets::cbox cb_custom_dark_theme {gui, "Use custom dark color theme"},
		cb_custom_light_theme {gui, "Use custom light color theme"};
	widgets::Button btn_edit_theme {gui, "Edit custom theme", true};
	btn_edit_theme.events().click([&] { fm_colors(fm); });

	gui["cb_custom_dark_theme"] << cb_custom_dark_theme;
	gui["cb_custom_light_theme"] << cb_custom_light_theme;
	gui["btn_edit_theme"] << btn_edit_theme;
	gui["l_theme"] << l_theme;
	gui["cbtheme_dark"] << cbtheme_dark;
	gui["cbtheme_light"] << cbtheme_light;
	if(system_supports_darkmode())
		gui["cbtheme_system"] << cbtheme_system;
	gui["l_contrast"] << l_contrast;
	gui["slider"] << slider;
	gui["cbsnap"] << cbsnap;
	gui["cbminw"] << cbminw;
	gui["sep3"] << sep3;
	gui["l_opendlg_origin"] << l_opendlg_origin;
	gui["cb_origin_progdir"] << cb_origin_progdir;
	gui["cb_origin_curdir"] << cb_origin_curdir;
	gui["cb_formats_fsize_bytes"] << cb_formats_fsize_bytes;

	presets.div(R"(vert
		<l_pinfo weight=90> <weight=18> <sep weight=3> <weight=20>
		<weight=25 <l_add weight=160> <weight=10> <tb_add> <weight=11> <btn_add weight=80>> <weight=20>
		<lb_presets> <weight=20>
		<weight=35 <><btn_save weight=100> <weight=20> <btn_load weight=100> <weight=20> <btn_delete weight=110> <weight=20> <btn_rename weight=110> <>>
	)");

	widgets::Label l_pinfo {presets, "<bold>Configuration presets</> are snapshots of the settings found in the first three categories "
		"(yt-dlp, SponsorBlock, Queuing), as well as the ones in the \"Download options\" group in the main window. The \"Save\" button saves the current "
		"settings to the selected preset, and the \"Load\" button loads the selected preset into the current settings."}, 
		l_add {presets, "Name for new preset:"};
	widgets::Separator sep_presets {presets};
	widgets::Listbox lb_presets {presets};
	widgets::Textbox tb_add {presets};
	widgets::Button btn_add {presets, "Create", true}, btn_save {presets, "Save"}, btn_load {presets, "Load"}, btn_delete {presets, "Delete"}, 
		btn_rename {presets, "Rename"};

	l_pinfo.text_align(nana::align::left, nana::align_v::top);
	l_pinfo.format(true);
	lb_presets.typeface(nana::paint::font_info {"Segoe UI", 15});
	lb_presets.enable_single(true, false);
	lb_presets.show_header(false);
	lb_presets.append_header("", 300);
	lb_presets.column_at(0).text_align(nana::align::center);
	lb_presets.events().resized([&](const nana::arg_resized &arg) { lb_presets.column_at(0).width(arg.width - util::scale(5)); });
	for(auto &el : conf_presets)
	{
		const auto &name {el.first};
		lb_presets.at(0).push_back(name);
		if(conf.equals_preset(el.second))
			lb_presets.at(0).back().select(true);
	}

	lb_presets.events().key_press([&](const nana::arg_keyboard &arg)
	{
		if(arg.key == nana::keyboard::del)
		{
			auto sel {lb_presets.selected()};
			if(!sel.empty())
				btn_delete.events().click.emit({}, btn_delete);
		}
	});

	tb_add.multi_lines(false);
	tb_add.padding(0, 5, 0, 5);
	tb_add.events().key_press([&](const nana::arg_keyboard &arg)
	{
		if(arg.key == nana::keyboard::enter)
			btn_add.events().click.emit({}, btn_add);
	});

	btn_save.enable(false);
	btn_load.enable(false);
	btn_delete.enable(false);
	btn_add.tooltip("Create a new preset and save the current settings to it.");

	presets["l_pinfo"] << l_pinfo;
	presets["lb_presets"] << lb_presets;
	presets["l_add"] << l_add;
	presets["tb_add"] << tb_add;
	presets["btn_add"] << btn_add;
	presets["sep"] << sep_presets;
	presets["btn_save"] << btn_save;
	presets["btn_load"] << btn_load;
	presets["btn_delete"] << btn_delete;
	presets["btn_rename"] << btn_rename;

	lb_presets.events().selected([&] (const nana::arg_listbox &arg)
	{
		btn_save.enable(false);
		btn_load.enable(false);
		btn_delete.enable(false);
		btn_rename.enable(false);
		if(arg.item.selected())
		{
			if(!conf.equals_preset(conf_presets[arg.item.text(0)]))
			{
				btn_save.enable(true);
				btn_load.enable(true);
			}
			btn_delete.enable(true);
			btn_rename.enable(true);
		}
	});

	btn_add.events().click([&]
	{
		const auto newname {tb_add.caption()};
		for(const auto &el : conf_presets)
			if(el.first == newname)
			{
				(widgets::msgbox {fm, "Duplicate name"}.icon(MB_ICONERROR) << "A preset with the name \"" << newname << "\" already exists.")();
				return;
			}
		if(newname.empty())
		{
			if((widgets::msgbox {fm, "Blank name", MB_YESNO} << "Are you sure you want to create a preset with a blank name?")() == IDNO)
				return;
		}
		for(const auto &el : conf_presets)
			if(conf.equals_preset(el.second))
			{
				(widgets::msgbox {fm, "Duplicate preset"}.icon(MB_ICONERROR) << "The preset \"" << el.first << "\" already contains the current settings.")();
				return;
			}
		conf_presets[newname] = conf;
		lb_presets.at(0).push_back(newname);
		lb_presets.at(0).back().select(true);
		tb_add.caption("");
	});

	btn_save.events().click([&]
	{
		const auto sel {lb_presets.selected()};
		if(!sel.empty())
		{
			const auto name {lb_presets.at(sel.front()).text(0)};
			for(const auto &el : conf_presets)
				if(conf.equals_preset(el.second))
				{
					(widgets::msgbox {fm, "Duplicate preset"}.icon(MB_ICONERROR) << "The preset \"" << el.first << "\" already contains the current settings.")();
					return;
				}
			conf_presets[name].from_preset(conf);
			btn_save.enable(false);
			btn_load.enable(false);
		}
	});

	btn_load.events().click([&]
	{
		const auto sel {lb_presets.selected()};
		if(!sel.empty())
		{
			const auto name {lb_presets.at(sel.front()).text(0)};
			if(conf_presets.contains(name))
			{
				conf.from_preset(conf_presets[name]);
				bottoms.current().argset = conf.argset;
				if(update_preset_widgets)
					update_preset_widgets();
				btn_save.enable(false);
				btn_load.enable(false);
			}
		}
	});

	btn_delete.events().click([&]
	{
		const auto sel {lb_presets.selected()};
		if(!sel.empty())
		{
			auto item {lb_presets.at(sel.front())};
			conf_presets.erase(item.text(0));
			lb_presets.erase(item);
		}
	});

	btn_rename.events().click([&]
	{
		const auto sel {lb_presets.selected()};
		if(!sel.empty())
		{
			auto item {lb_presets.at(sel.front())};
			const auto res {input_box(fm, "Rename preset", "New name for preset:")};
			if(res.first)
			{
				const auto new_name {res.second}, old_name {item.text(0)};
				for(const auto &el : conf_presets)
					if(el.first == new_name)
					{
						(widgets::msgbox {fm, "Duplicate name"}.icon(MB_ICONERROR) << "A preset with the name \"" << new_name << "\" already exists.")();
						return;
					}
				if(new_name.empty())
					if((widgets::msgbox {fm, "Blank name", MB_YESNO} << "Are you sure you want to make the preset name blank?")() == IDNO)
						return;
				item.text(0, new_name);
				auto node {conf_presets.extract(old_name)};
				node.key() = new_name;
				conf_presets.insert(std::move(node));
			}
		}
	});

	update_preset_widgets = [&]
	{
		cb_save_errors.check(conf.cb_save_errors);
		cb_clear_done.check(conf.cb_clear_done);
		cb_add_on_focus.check(conf.cb_add_on_focus);
		cb_display_custom_filenames.check(conf.cb_display_custom_filenames);

		const auto &bottom {bottoms.current()};
		const bool bctemplate {bottom.is_bcplaylist || bottom.is_bcchan};
		auto &output_template {bctemplate ? conf.output_template_bandcamp : conf.output_template};
		tb_template.caption(output_template);
		tb_cookies.caption(conf.cookie_options);
		tb_proxy.caption(conf.proxy);
		tb_playlist.caption(conf.playlist_indexing);

		com_vcodec.option(conf.pref_vcodec);
		com_acodec.option(conf.pref_acodec);
		com_video.option(conf.pref_video);
		com_audio.option(conf.pref_audio);
		com_cookies.option(conf.com_cookies);
		com_res.option(conf.pref_res);
		cbfps.check(conf.pref_fps);
		cb_lengthyproc.check(conf.cb_lengthyproc);
		cb_autostart.check(conf.cb_autostart);
		cb_common.check(!conf.common_dl_options);
		cb_queue_autostart.check(conf.cb_queue_autostart);
		cb_zeropadding.check(conf.cb_zeropadding);
		cb_playlist_folder.check(conf.cb_playlist_folder);
		cb_premium.check(conf.cb_premium);
		cb_proxy.check(conf.cb_proxy);
		cb_cookies.check(conf.cb_cookies);
		cb_aria.check(conf.cb_aria);
		tb_aria.caption(conf.aria_options);

		cb_mark.check(conf.cb_sblock_mark);
		cb_remove.check(conf.cb_sblock_remove);
		lbmark.auto_draw(false);
		lbremove.auto_draw(false);
		for(auto &ip : lbmark.at(0))
			ip.check(false);
		for(auto &ip : lbremove.at(0))
			ip.check(false);
		for(auto i : conf.sblock_mark)
			lbmark.at(0).at(i).check(true);
		for(auto i : conf.sblock_remove)
			lbremove.at(0).at(i).check(true);
		lbmark.auto_draw(true);
		lbremove.auto_draw(true);

		sb_maxdl.value(std::to_string(conf.max_concurrent_downloads));
		sb_maxinfo.value(std::to_string(conf.max_data_threads));

		com_args.caption(conf.argset);
		com_chap.option(conf.com_chap);
		com_rate.option(conf.ratelim_unit);
		bottoms.current().outpath = conf.outpath;
		l_outpath.caption(conf.outpath.u8string());
		bot_showing = true;
		cbsubs.check(conf.cbsubs);
		cbthumb.check(conf.cbthumb);
		cbtime.check(conf.cbtime);
		cbkeyframes.check(conf.cbkeyframes);
		cbmp3.check(conf.cbmp3);
		cbargs.check(conf.cbargs);
		bot_showing = false;

		if(conf.ratelim)
			tbrate.caption(util::format_double(conf.ratelim, 1));
		else tbrate.caption("");

		if(conf.common_dl_options)
		{
			bottoms.propagate_args_options(bottom);
			bottoms.propagate_cb_options(bottom);
			bottoms.propagate_misc_options(bottom);
		}
	};

	cb_formats_fsize_bytes.check(conf.cb_formats_fsize_bytes);

	cb_custom_light_theme.check(conf.cb_custom_light_theme);
	cb_custom_light_theme.events().checked([&]
	{
		conf.cb_custom_light_theme = cb_custom_light_theme.checked();
		slider.events().value_changed.emit({slider}, slider);
	});

	cb_custom_dark_theme.check(conf.cb_custom_dark_theme);
	cb_custom_dark_theme.events().checked([&]
	{
		conf.cb_custom_dark_theme = cb_custom_dark_theme.checked();
		slider.events().value_changed.emit({slider}, slider);
	});

	cbminw.check(conf.cbminw);
	cbminw.events().checked([&]
	{
		conf.cbminw = cbminw.checked();
		if(!conf.cbminw)
		{
			auto sz {nana::api::window_outline_size(*this)};
			if(sz.width < minw)
			{
				sz.width = minw;
				nana::api::window_outline_size(*this, sz);
			}
		}
	});

	cbsnap.check(conf.cbsnap);
	cbsnap.events().checked([&]
	{
		conf.cbsnap = cbsnap.checked();
		fm.snap(conf.cbsnap);
		snap(conf.cbsnap);
	});

	l_opendlg_origin.text_align(nana::align::left, nana::align_v::center);

	const auto dpi {nana::api::screen_dpi(true)};
	if(dpi >= 240)
		btn_info.image(arr_info48a_png, sizeof arr_info48a_png);
	else if(dpi >= 192)
		btn_info.image(arr_info32_png, sizeof arr_info32_png);
	else if(dpi > 96)
		btn_info.image(arr_info22_ico, sizeof arr_info22_ico);
	else btn_info.image(arr_info16_ico, sizeof arr_info16_ico);
	btn_info.events().click([&] {fm_settings_info(fm); });
	btn_close.events().click([&] {fm.close(); });

	const auto &bottom {bottoms.current()};
	const bool bctemplate {bottom.is_bcplaylist || bottom.is_bcchan};
	auto &output_template_default {bctemplate ? conf.output_template_default_bandcamp : conf.output_template_default};
	auto &output_template {bctemplate ? conf.output_template_bandcamp : conf.output_template};
	btn_default.tooltip(nana::to_utf8(output_template_default));
	btn_playlist_default.tooltip(nana::to_utf8(conf.playlist_indexing_default));
	tb_template.multi_lines(false);
	tb_template.padding(0, 5, 0, 5);
	tb_template.caption(output_template);
	tb_template.typeface(nana::paint::font_info {"Tahoma", 10});
	tb_playlist.multi_lines(false);
	tb_playlist.padding(0, 5, 0, 5);
	tb_playlist.typeface(nana::paint::font_info {"Tahoma", 10});
	if(bctemplate)
	{
		l_playlist.enabled(false);
		tb_playlist.enabled(false);
		btn_playlist_default.enabled(false);
		cb_zeropadding.enabled(false);
	}
	btn_default.events().click([&]
	{
		tb_template.caption(output_template_default);
		output_template = output_template_default;
	});
	btn_playlist_default.events().click([&]
	{
		tb_playlist.caption(conf.playlist_indexing_default);
		conf.playlist_indexing = conf.playlist_indexing_default;
	});

	sb_maxdl.range(1, 10, 1);
	sb_maxinfo.range(1, number_of_processors, 1);

	slider.maximum(30);
	slider.value(conf.contrast * 100);
	slider.events().value_changed([&]
	{
		conf.contrast = static_cast<double>(slider.value()) / 100;
		apply_theme(theme::is_dark());
		fm.bgcolor(theme::fmbg);
		nana::api::refresh_window(gpopt);
		ytdlp.refresh_theme();
		sblock.refresh_theme();
		queuing.refresh_theme();
		gui.refresh_theme();
		updater.refresh_theme();
		tree.refresh_theme();
		nana::api::refresh_window(tree);
	});

	nana::radio_group rg;
	rg.add(cbtheme_dark);
	rg.add(cbtheme_light);
	if(system_supports_darkmode())
		rg.add(cbtheme_system);
	rg.on_checked([&](const nana::arg_checkbox &arg)
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
	if(conf.cbtheme == 1) cbtheme_light.check(true);

	nana::radio_group rg_origin;
	rg_origin.add(cb_origin_progdir);
	rg_origin.add(cb_origin_curdir);
	if(conf.open_dialog_origin)
		cb_origin_curdir.check(true);
	else cb_origin_progdir.check(true);

	update_conf = [&]
	{
		conf.aria_options = tb_aria.caption();
		conf.pref_res = com_res.option();
		conf.pref_video = com_video.option();
		conf.pref_audio = com_audio.option();
		conf.pref_vcodec = com_vcodec.option();
		conf.pref_acodec = com_acodec.option();
		conf.com_cookies = com_cookies.option();
		conf.pref_fps = cbfps.checked();
		output_template = tb_template.caption_wstring();
		conf.playlist_indexing = tb_playlist.caption_wstring();
		conf.max_concurrent_downloads = sb_maxdl.to_int();
		conf.max_data_threads = sb_maxinfo.to_int();
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
		conf.cookie_options = tb_cookies.caption();
		conf.update_self_only = cb_selfonly.checked();
		conf.cb_premium = cb_premium.checked();
		conf.cb_save_errors = cb_save_errors.checked();
		conf.cb_clear_done = cb_clear_done.checked();
		conf.cb_formats_fsize_bytes = cb_formats_fsize_bytes.checked();
		conf.cb_add_on_focus = cb_add_on_focus.checked();
		conf.cb_display_custom_filenames = cb_display_custom_filenames.checked();
		conf.cb_aria = cb_aria.checked();
		conf.cb_cookies = cb_cookies.checked();

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
			auto &curbot {bottoms.current()};
			if(conf.common_dl_options = !conf.common_dl_options)
			{
				l_outpath.source_path(&conf.outpath);
				bottoms.propagate_cb_options(curbot);
				bottoms.propagate_args_options(curbot);
				bottoms.propagate_misc_options(curbot);
				show_btncopy(false);
			}
			else
			{
				l_outpath.source_path(&curbot.outpath);
				show_btncopy(true);
			}
			std::string cap {"Download options"};
			auto sel {lbq.selected()};
			if(!sel.empty())
				gpopt.caption(conf.common_dl_options ? cap : cap + " for queue item #" + lbq.at(sel.front()).text(0));
			nana::api::refresh_window(gpopt);
		}

		conf.get_releases_at_startup = cb_startup.checked();
		conf.cb_ffplay = cb_ffplay.checked();

		lb_presets.auto_draw(false);
		for(auto &ip : lb_presets.at(0))
			ip.select(false);
		for(auto &el : conf_presets)
		{
			const auto &name {el.first};
			if(conf.equals_preset(el.second))
			{
				for(auto &ip : lb_presets.at(0))
				{
					if(ip.text(0) == name)
					{
						ip.select(true);
						break;
					}
				}
			}
		}
		lb_presets.auto_draw(true);
	};

	cb_save_errors.tooltip("When the settings are saved, any incomplete queue items are also saved,\nexcept for those with the "
		"\"error\" status. This option lets you also save the\nitems with the \"error\" status, which can be useful when a "
		"download fails\ndue to connection issues, but can be resumed later.");

	cb_premium.tooltip("This option lets you override your video codec preference, in the case when a video\n"
		"has format 616 available (which has the \"premium\" bitrate). For example, if you're\ndownloading a 1080p video and "
		"you prefer the H264 codec, this option will override\nthat codec preference by requesting format 616 from yt-dlp "
		"(with the argument\n\"-f 616+ba\").");

	cb_android.tooltip("Forces yt-dlp to use the Android client for extracting the video by passing\n"
		"\"<bold>--extractor-args youtube:player_client=android</>\". This can be useful\nbecause some formats are only available "
		"to the Android client."
		"\n\nHowever, be aware of the warning from yt-dlp:\n\"<bold>Android client formats are broken and may yield HTTP Error 403</>\"");

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

	cb_add_on_focus.tooltip("If this option is checked, whenever the main window receives the input focus,\nany URL in the clipboard will "
		"be automatically added to the queue (if it's not\nalready there). This only works for a single URL (if you have a multi-line list "
		"of\nURLs, you'll have to add it manually)."
	);

	cb_display_custom_filenames.tooltip("You can provide a custom file name for a queue item by selecting\n<bold>Set file name for item #</> "
		"from the queue menu, or by pressing <bold>F2</>.\nThis name is \"custom\" because it overrides the output template\ndefined in the settings.\n\n"
		"If you check this option, then if a queue item has a custom file name,\nit will be shown in the \"Media title\" column instead of the media title."
	);

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
		"\tSOCKS proxy with authentication:  <bold>socks5://USER:PASS@IP_ADDRESS:PORT</>\t"},

		codec_tip {"In the context of digital video and audio, a codec is a processing algorithm\nused to compress data, and the "
		"process of compressing data with a codec\nis called \"encoding\".\n\nA website like YouTube may offer multiple formats that "
		"use the same\ncontainer, but different codecs, so specifying a preferred codec can prevent\nunwanted formats from being downloaded."},

		ffmpeg_tip {"Here you can tell the program where to put the FFmpeg files when\nit updates FFmpeg (via Settings->Updater). "
			"If the folder you choose\nis not the yt-dlp folder, the program will offer to set up yt-dlp\n(via config file) "
			"to look for the FFmpeg files in that folder.\n<bold>.\\ </>means the program folder (where ytdlp-interface.exe is located)."},

		ytdlp_tip {"<bold>.\\ </>means the program folder (where ytdlp-interface.exe is located)."},
		
		maxinfo_tip {"When a URL is added to the queue, the program uses yt-dlp to get info about it,\nand about the media that can be "
			"downloaded from it. When adding multiple\nURLs to the queue (either by pasting a list of URLs or by splitting a playlist), "
			"the\nprogram uses multiple concurrent yt-dlp instances, up to a maximum of 4 by\ndefault. You can increase this number "
			"up to the number of logical processors on\nyour system, but you should be aware of a few considerations before you do.\n\n"
			"First, if you're downloading a lot from YouTube, you should consider the\npossibility that you might get banned "
			"for making too many requests too quickly.\nI don't know how likely that is to happen, but I know it's possible.\n\n"
			"Second, yt-dlp eats a lot of CPU resources for what it does, because Python is an\ninterpreted language (yt-dlp.exe "
			"contains the Python interpreter, which runs the\nyt-dlp program). It's possible to max out your CPU utilization by "
			"increasing this\nnumber, and the responsiveness of the interface may be lowered."},
		
		cookie_options_tip {"Additional options for the <bold>--cookies-from-browser</> argument,\nin the format <bold>[:PROFILE][::CONTAINER]</>."
			"\n\nThe text you type is passed to yt-dlp as is, without any validation."},

		cookies_tip {"The program will use <bold>--cookies-from-browser</> when downloading\nand when getting data for a URL.\n\nThis will cause "
			"yt-dlp to automatically authenticate with the website\nyou're downloading from, if you're logged into it in your browser.\n\n"
			"Use the drop-down list to select which browser to get cookies from,\nand type any additional options in the textbox.\n\n"
			"WARNING: Due to recent changes to how Chrome handles cookies,\nthis will probably not work with Chrome."};

	l_cookie_options.tooltip(cookie_options_tip);
	tb_cookies.tooltip(cookie_options_tip);
	l_cookies.tooltip(cookies_tip);
	com_cookies.tooltip(cookies_tip);

	l_ffmpeg.tooltip(ffmpeg_tip);
	l_ffmpeg_path.tooltip(ffmpeg_tip);
	l_ytdlp.tooltip(ytdlp_tip);
	l_ytdlp_path.tooltip(ytdlp_tip);

	cb_proxy.tooltip(proxy_tip);
	tb_proxy.tooltip(proxy_tip);

	auto premium_handler = [&]
	{
		auto &plc {ytdlp.get_place()};
		if(com_res.option() > 4 || com_vcodec.option() == 0 || com_vcodec.option() == 3)
		{
			plc.field_display("row_premium", false);
			plc.field_display("spacer_premium", false);
		}
		else
		{
			plc.field_display("row_premium", true);
			plc.field_display("spacer_premium", true);
		}
		plc.collocate();
		nana::api::refresh_window(ytdlp);
	};

	for(auto &opt : com_vcodec_options)
		com_vcodec.push_back(" " + nana::to_utf8(opt));
	com_vcodec.events().selected(premium_handler);

	for(auto &opt : com_acodec_options)
		com_acodec.push_back(" " + nana::to_utf8(opt));

	for(auto &opt : com_video_options)
		com_video.push_back(" " + nana::to_utf8(opt));

	for(auto &opt : com_audio_options)
		com_audio.push_back(" " + nana::to_utf8(opt));

	for(auto &opt : com_res_options)
		com_res.push_back(" " + nana::to_utf8(opt));

	for(auto &opt : com_cookies_options)
		com_cookies.push_back(" " + nana::to_utf8(opt));

	com_res.events().selected(premium_handler);
	com_res.tooltip(res_tip);
	l_res.tooltip(res_tip);
	l_maxdl.tooltip(maxdl_tip);
	sb_maxdl.tooltip(maxdl_tip);
	l_maxinfo.tooltip(maxinfo_tip);
	sb_maxinfo.tooltip(maxinfo_tip);
	l_template.tooltip(template_tip);
	tb_template.tooltip(template_tip);
	l_playlist.tooltip(playlist_tip);
	tb_playlist.tooltip(playlist_tip);
	l_video.tooltip(container_tip);
	l_audio.tooltip(container_tip);
	com_video.tooltip(container_tip);
	com_audio.tooltip(container_tip);
	l_vcodec.tooltip(codec_tip);
	com_vcodec.tooltip(codec_tip);
	l_acodec.tooltip(codec_tip);
	com_acodec.tooltip(codec_tip);

	fm.events().unload([&]
	{
		start_page = tree.selected_page();
		update_conf();

		if(conf.max_concurrent_downloads > initial_maxdl && lbq.item_count() > 1)
		{
			for(size_t cat(0); cat < lbq.size_categ(); cat++)
				for(auto item : lbq.at(cat))
				{
					auto url {item.value<lbqval_t>().url};
					if(bottoms.at(url).started)
					{
						start_next_urls(url);
						cat = lbq.size_categ();
						break;
					}
				}
		}

		updater_working = false;
		if(thr_updater.joinable())
			thr_updater.join();
		if(thr_releases.joinable())
			thr_releases.detach();
		if(thr_releases_ffmpeg.joinable())
			thr_releases_ffmpeg.detach();
		if(thr_releases_ytdlp.joinable())
			thr_releases_ytdlp.detach();
		if(thr_versions.joinable())
			thr_versions.detach();
		if(thr_ver_ffmpeg.joinable())
			thr_ver_ffmpeg.detach();

		conf.unfinished_queue_items.clear();
		lbq.auto_draw(false);
		for(size_t cat {0}; cat < lbq.size_categ(); cat++)
		{
			auto icat {lbq.at(cat)};
			if(icat.size())
			{
				auto &qitems {conf.unfinished_queue_items.emplace_back(icat.text(), std::vector<std::string>{}).second};
				for(auto item : icat)
				{
					const auto &bottom {bottoms.at(item.value<lbqval_t>().url)};
					const auto outfile {bottom.outfile.filename().string()};
					if(conf.cb_display_custom_filenames)
					{
						if(!bottom.outfile.empty())
							item.text(2, outfile);
					}
					else
					{
						if(item.text(2).size() == outfile.size())
						{
							std::string media_title {bottom.media_title};
							if(!nana::is_utf8(media_title))
							{
								std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> u16conv;
								auto u16str {u16conv.from_bytes(media_title)};
								std::wstring wstr(u16str.size(), L'\0');
								memcpy(&wstr.front(), &u16str.front(), wstr.size() * 2);
								std::wstring_convert<std::codecvt_utf8<wchar_t>> u8conv;
								media_title = u8conv.to_bytes(wstr);
							}
							item.text(2, media_title);
						}
					}

					const auto text {item.text(3)};
					if(text != "done" && (text != "error" || text == "error" && conf.cb_save_errors))
						qitems.push_back(nana::to_utf8(item.value<lbqval_t>().url));
				}
			}
		}
		lbq.auto_draw(true);

		if(!fn_write_conf() && errno)
		{
			std::string error {std::strerror(errno)};
			widgets::msgbox mbox {fm, "ytdlp-interface - error writing settings file"};
			mbox.icon(MB_ICONERROR);
			(mbox << confpath.string() << "\n\nAn error occured when trying to save the settings file:\n\n" << error)();
		}
	});

	fm.theme_callback([&](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme::fmbg);
		ytdlp.bgcolor(theme::fmbg);
		queuing.bgcolor(theme::fmbg);
		gui.bgcolor(theme::fmbg);
		l_sblock.caption(std::regex_replace(sblock_text, std::regex {"\\b(0x)"}, theme::is_dark() ? link_dark : link_light));
		l_about_ver.caption(std::regex_replace(vertext, std::regex {"\\b(0x)"}, theme::is_dark() ? link_dark : link_light));
		updater_display_version_ytdlp();
		updater_display_version_ffmpeg();
		gui.get_place().field_display(dark ? "cb_custom_dark_theme" : "cb_custom_light_theme", true);
		gui.get_place().collocate();
		return true;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	update_preset_widgets();
	tree.select(start_page);
	fm.collocate();

	fm.modality();
}


void GUI::fm_settings_info(nana::window owner)
{
	using namespace nana;
	using ::widgets::theme;

	themed_form fm {nullptr, owner, {}, appear::decorate{}};
	fm.caption(title + " - format sorting info");
	fm.bgcolor(theme::fmbg);
	fm.snap(conf.cbsnap);
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
			g.rectangle(rectangle {pic.pos(), pic.size()}.pare_off(-1), false, theme::border);
	});

	fm.theme_callback([&](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme::fmbg);
		pic.load(dark ? imgdark : imglight);
		p.caption(std::regex_replace(ptext, std::regex {"\\b(0x)"}, theme::is_dark() ? "0xd0c0b0" : "0x708099"));
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.collocate();
	fm.modality();
}


void GUI::make_updater_page(themed_form &parent)
{
	updater.div(R"(vert
		<weight=5> <sep1 weight=3> <weight=20>
		<weight=30 <l_ver weight=101> <weight=10> <l_vertext> <weight=20> <btn_changes weight=150> >
		<weight=15> <cb_startup weight=25>
		<weight=15> <cb_selfonly weight=25>
		<weight=20>
		<weight=30 <btn_update weight=100> <weight=20> <prog> > <weight=25>
		<sep2 weight=3> <weight=20>
		<weight=25 <l_ytdlp weight=132> <weight=10> <l_ytdlp_path> > <weight=20>
		<weight=25 <l_ffmpeg weight=132> <weight=10> <l_ffmpeg_path> > <weight=20>
		<weight=30 <l_ver_ytdlp weight=170> <weight=10> <l_ytdlp_text> > <weight=11>
		<weight=30 <l_ver_ffmpeg weight=170> <weight=10> <l_ffmpeg_text> > <weight=13>
		<channel weight=25 <l_channel weight=170> <weight=20> <cb_chan_stable weight=75> <weight=10> <cb_chan_nightly weight=85> <> >
		<channel_spacer weight=17> <weight=25 <weight=14> <cb_ffplay>> <weight=20>
		<weight=30 <prog_misc> > <weight=25>
		<weight=30 <> <btn_update_ytdlp weight=150> <weight=20> <btn_update_ffmpeg weight=160> <> >
	)");

	if(win7)
	{
		updater.get_place().field_display("channel", false);
		updater.get_place().field_display("channel_spacer", false);
	}

	l_ytdlp.create(updater, "Path to yt-dlp:");
	l_ffmpeg.create(updater, "FFmpeg folder:");
	l_ytdlp_path.create(updater, &conf.ytdlp_path);
	l_ffmpeg_path.create(updater, &conf.ffmpeg_path);
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
	cb_selfonly.create(updater, "Only extract ytdlp-interface.exe from the downloaded archive");
	cb_chan_stable.create(updater, "Stable");
	cb_chan_nightly.create(updater, "Nightly");
	cb_ffplay.create(updater, "When updating ffmpeg, also extract \"ffplay.exe\"");
	prog_updater.create(updater);
	prog_updater_misc.create(updater);
	sep1.create(updater, "ytdlp-interface");
	sep2.create(updater, "ffmpeg & yt-dlp");

	updater["sep1"] << sep1;
	updater["cb_startup"] << cb_startup;
	updater["cb_selfonly"] << cb_selfonly;
	updater["l_ver"] << l_ver;
	updater["l_vertext"] << l_vertext;
	updater["btn_changes"] << btn_changes;
	updater["btn_update"] << btn_update;
	updater["prog"] << prog_updater;
	updater["sep2"] << sep2;
	updater["l_channel"] << l_channel;
	updater["cb_chan_stable"] << cb_chan_stable;
	updater["cb_chan_nightly"] << cb_chan_nightly;
	updater["cb_ffplay"] << cb_ffplay;
	updater["l_ver_ytdlp"] << l_ver_ytdlp;
	updater["l_ver_ffmpeg"] << l_ver_ffmpeg;
	updater["l_ytdlp_text"] << l_ytdlp_text;
	updater["l_ffmpeg_text"] << l_ffmpeg_text;
	updater["btn_update_ytdlp"] << btn_update_ytdlp;
	updater["btn_update_ffmpeg"] << btn_update_ffmpeg;
	updater["prog_misc"] << prog_updater_misc;
	updater["l_ytdlp"] << l_ytdlp;
	updater["l_ytdlp_path"] << l_ytdlp_path;
	updater["l_ffmpeg"] << l_ffmpeg;
	updater["l_ffmpeg_path"] << l_ffmpeg_path;

	cb_selfonly.check(conf.update_self_only);
	cb_startup.check(conf.get_releases_at_startup);
	cb_ffplay.check(conf.cb_ffplay);
	cb_chan_stable.radio(true);
	cb_chan_nightly.radio(true);

	cb_chan_stable.tooltip("\"Stable\" releases are well tested and have no major bugs,\n"
		"but there's a relatively long time until one comes out.");
	cb_chan_nightly.tooltip("\"Nightly\" releases come out every day around midnight\nUTC and contain the latest patches and changes. "
		"This is\nthe recommended channel for regular users of yt-dlp.");
	std::string selfonly_tip {"To update the program, an archive is downloaded from GitHub,\nwhich contains the following files:\n\n<bold>7z.dll\n"};
	selfonly_tip += win7 || !X64 ? "libwinpthread-1.dll\nqjs.exe\n" : "deno.exe\n";
	selfonly_tip += "ffmpeg.exe\nffprobe.exe\nyt-dlp.exe\nytdlp-interface.exe</>\n\nCheck this option if you don't want your current "
		"versions of\nyt-dlp and ffmpeg to be overwritten with those in the archive.";
	cb_selfonly.tooltip(selfonly_tip);

	if(!rgp_chan.size())
	{
		rgp_chan.add(cb_chan_stable);
		rgp_chan.add(cb_chan_nightly);
	}

	btn_changes.typeface(nana::paint::font_info {"Segoe UI", 12, {800}});
	btn_changes.enabled(false);
	btn_changes.events().click([&] { fm_changes(updater.parent()); });
	btn_update.typeface(nana::paint::font_info {"Segoe UI", 12, {800}});
	btn_update.enabled(false);
	btn_update_ytdlp.typeface(nana::paint::font_info {"Segoe UI", 12, {800}});
	btn_update_ytdlp.enabled(false);
	btn_update_ffmpeg.typeface(nana::paint::font_info {"Segoe UI", 12, {800}});
	btn_update_ffmpeg.enabled(false);
	l_ver_ytdlp.text_align(nana::align::right, nana::align_v::center);
	l_ytdlp_text.format(true);
	l_ffmpeg_text.format(true);

	l_ytdlp_path.events().click([&]
	{
		nana::filebox fb {updater, true};
		if(!conf.ytdlp_path.empty())
		{
			if(conf.ytdlp_path.string().find(".\\") == 0)
				fb.init_file(appdir / conf.ytdlp_path.filename());
			else fb.init_file(conf.ytdlp_path);
		}
		else fb.init_path(appdir);
		fb.allow_multi_select(false);
		fb.add_filter("yt-dlp executable", ytdlp_fname + (win7 ? "" : ";ytdl-patched-red.exe"));
		fb.title("Locate and select " + ytdlp_fname + (win7 ? "" : " or ytdl-patched-red.exe"));
		auto res {fb()};
		if(res.size())
		{
			auto path {res.front()};
			auto fname {path.filename().string()};
			if(fname == ytdlp_fname || fname == "ytdl-patched-red.exe")
			{
				conf.ytdlp_path = util::to_relative_path(path);
				get_version_ytdlp();
				l_ytdlp_path.update_caption();
				if(!updater_t2.started())
					updater_t2.start();
			}
		}
	});

	l_ffmpeg_path.events().click([&](const nana::arg_click &arg)
	{
		nana::folderbox fb {updater, conf.ffmpeg_path.empty() ? (conf.ytdlp_path.empty() ? appdir : conf.ytdlp_path.parent_path()) : conf.ffmpeg_path};
		fb.allow_multi_select(false);
		fb.title("Locate and select the FFmpeg folder");
		auto res {fb()};
		if(res.size())
		{
			const auto orig_path {conf.ffmpeg_path};
			conf.ffmpeg_path = util::to_relative_path(res.front());
			l_ffmpeg_path.update_caption();
			if(!updater_check_paths(true))
			{
				widgets::msgbox mbox {parent, "Custom FFmpeg folder", nana::msgbox::yes_no};
				mbox << conf.ffmpeg_path.string() << "\n\nThe selected folder doesn't seem to contain ffmpeg.exe. "
					"Are you sure you want to use this folder for the FFmpeg files?\n\nPress \"Yes\" if you intend to "
					"add the FFmpeg files to the folder (you can use the updater to download the latest version).\n\n"
					"Press \"No\" to choose a different folder.";
				if(mbox() == IDNO)
				{
					conf.ffmpeg_path = orig_path;
					updater_check_paths(true);
					l_ffmpeg_path.events().click.emit(arg, arg.window_handle);
					return;
				}
			}

			get_version_ffmpeg();
			if(!updater_t1.started())
				updater_t1.start();
			if(!conf.ytdlp_path.empty())
			{
				auto ytdlp_dir {conf.ytdlp_path.parent_path()};
				if(ytdlp_dir.string() == ".")
					ytdlp_dir = ".\\";
				if(ytdlp_dir != conf.ffmpeg_path)
				{
					if(!fs::exists(ytdlp_dir / "yt-dlp.conf"))
					{
						widgets::msgbox mbox {parent, "Custom FFmpeg folder", nana::msgbox::yes_no};
						mbox.icon(nana::msgbox::icon_question);
						std::string msg {"When updating FFmpeg, the program will put the FFmpeg files in the "
							"folder you selected. There's just one problem though, the folder you selected is not "
							"the same as the folder that contains yt-dlp.exe, so yt-dlp will not know how to find "
							"the FFmpeg files.\n\n[ Yes ] To fix that, a configuration file (yt-dlp.conf) containing the "
							"argument \"--ffmpeg-location\" can be created in the yt-dlp folder.\n\n[ No ] Or, the program "
							"can pass \"--ffmpeg-location\" to yt-dlp through the command line.\n\n"
							"Should the program create yt-dlp.conf for you now?"};
						if((mbox << msg)() == IDYES)
						{
							auto conf_path {ytdlp_dir / "yt-dlp.conf"};
							std::ofstream of {conf_path};
							if(of.good())
							{
								of << "--ffmpeg-location " << conf.ffmpeg_path;
								widgets::msgbox mb {parent, "Custom FFmpeg folder"};
								mb.icon(nana::msgbox::icon_information) << conf_path.string() <<
									"\n\nThe file has been created, and yt-dlp should now know where to find the FFmpeg files.";
								mb.show();
							}
							else
							{
								widgets::msgbox err {parent, "Custom FFmpeg folder"};
								err.icon(nana::msgbox::icon_error);
								err << conf_path.string() << "\n\nCould not open the file for writing! "
									"Make sure the yt-dlp folder isn't \"Program Files\" or any other "
									"folder that requires elevated priviliges for write access. Otherwise, "
									"you must use \"Run as administrator\" to launch ytdlp-interface.";
								err.show();
							}
						}
					}
					else
					{
						// if yt-dlp.conf doesn't contain --ffmpeg-location, warn user
						std::ifstream infile {ytdlp_dir / "yt-dlp.conf"};
						if(infile)
						{
							std::stringstream ss;
							ss << infile.rdbuf();
							infile.close();
							const auto &str {ss.str()};
							auto pos {str.find("--ffmpeg-location")};
							if(pos == -1)
							{
								widgets::msgbox mbox {parent, "Custom FFmpeg folder", nana::msgbox::yes_no};
								mbox.icon(nana::msgbox::icon_question);
								std::string msg {"The yt-dlp folder contains a configuration file (yt-dlp.conf), "
									"but the file doesn't contain the argument \"--ffmpeg-location\". That argument "
									"must be present to tell yt-dlp where to find the FFmpeg files. "
									"Should the program add the argument to the config file?\n\n"
									"If you select \"no\", the program will have to pass \"--ffmpeg-location\" "
									"to yt-dlp through the command line."};
								if((mbox << msg)() == IDYES)
								{
									std::ofstream ofs {ytdlp_dir / "yt-dlp.conf", std::ios_base::app};
									if(ofs.good())
									{
										ofs << "\n\n--ffmpeg-location " << conf.ffmpeg_path;
										nana::msgbox mb {parent, "Custom FFmpeg folder"};
										mb.icon(nana::msgbox::icon_information) << "The argument has been added.";
										mb.show();
									}
									else
									{
										widgets::msgbox err {parent, "Custom FFmpeg folder"};
										err.icon(nana::msgbox::icon_error);
										err << "Could not open file for writing! "
											"Make sure the yt-dlp folder isn't \"Program Files\" or any other "
											"folder that requires elevated priviliges for write access. Otherwise, "
											"you must use \"Run as administrator\" to launch ytdlp-interface.";
										err.show();
									}
								}
							}
							else // the config file contains --ffmpeg-location, attempt to update it
							{
								pos += 18;
								if(pos < str.size() - 1 && str[pos] == '\"')
								{
									auto pos2 {str.find('\"', pos + 1)};
									if(pos2 != -1)
									{
										auto path_from_config_file {str.substr(pos + 1, pos2 - pos - 1)};
										if(conf.ffmpeg_path != path_from_config_file)
										{
											std::ofstream ofs {ytdlp_dir / "yt-dlp.conf", std::ios_base::trunc};
											if(ofs.good())
											{
												ofs.write(str.data(), pos);
												ofs << conf.ffmpeg_path;
												if(pos2 + 1 < str.size())
													ofs << str.substr(pos2 + 1);
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	});

	btn_update.events().click([&parent, this]
	{
		updater_update_self(parent);
	});

	btn_update_ffmpeg.events().click([&parent, this]
	{
		bool use_ytdlp_folder {!conf.ffmpeg_path.empty() && conf.ffmpeg_path == conf.ytdlp_path.parent_path()};
		if(conf.ffmpeg_path.empty())
		{
			if(util::is_dir_writable(appdir))
			{
				updater_update_misc(false, appdir);
				return;
			}
			else use_ytdlp_folder = true;
		}

		if(use_ytdlp_folder && !util::is_dir_writable(conf.ytdlp_path.parent_path()))
			use_ytdlp_folder = false;

		if(!use_ytdlp_folder && !conf.ffmpeg_path.empty())
		{
			if(!util::is_dir_writable(conf.ffmpeg_path))
			{
				if(util::is_dir_writable(appdir))
					updater_update_misc(false, appdir);
				else
				{
					::widgets::msgbox mbox {parent, "No place to put the ffmpeg files"};
					mbox.icon(nana::msgbox::icon_error);
					(mbox << "Neither the program folder, nor the specified yt-dlp folder can be written in. Run the program "
						"as administrator to fix that.")();
				}
				return;
			}
		}
		updater_update_misc(false, use_ytdlp_folder ? conf.ytdlp_path.parent_path() : conf.ffmpeg_path);
	});

	btn_update_ytdlp.events().click([&parent, this]
	{
		if(conf.ytdlp_path.empty())
		{
			if(util::is_dir_writable(appdir))
			{
				updater_update_misc(true, appdir);
			}
			else
			{
				::widgets::msgbox mbox {parent, "No place to put yt-dlp.exe"};
				mbox.icon(nana::msgbox::icon_error);
				(mbox << "The path for yt-dlp is not defined, and the program can't write in the folder "
					"it's currently in (running it as administrator should fix that).")();
			}
		}
		else
		{
			if(util::is_dir_writable(conf.ytdlp_path.parent_path()))
			{
				updater_update_misc(true, conf.ytdlp_path.parent_path());
			}
			else
			{
				if(util::is_dir_writable(appdir))
					updater_update_misc(true, appdir);
				else
				{
					::widgets::msgbox mbox {parent, "No place to put yt-dlp.exe"};
					mbox.icon(nana::msgbox::icon_error);
					(mbox << "Neither the program folder, nor the specified yt-dlp folder can be written in. Running the program "
						"as administrator should fix that.")();
				}
			}
		}
	});

	updater_t0.interval(std::chrono::milliseconds {100});
	updater_t0.elapse([this]
	{
		if(!thr_releases.joinable())
		{
			updater_display_version();
			updater_t0.stop();
		}
	});

	updater_t1.interval(std::chrono::milliseconds {300});
	updater_t1.elapse([this]
	{
		if(!thr_releases_ffmpeg.joinable())
		{
			updater_display_version_ffmpeg();
			updater_t1.stop();
		}
	});

	updater_t2.interval(std::chrono::milliseconds {300});
	updater_t2.elapse([this]
	{
		if(!thr_releases_ytdlp.joinable())
		{
			if(!url_latest_ytdlp_relnotes.empty())
			{
				nana::api::effects_edge_nimbus(l_ytdlp_text, nana::effects::edge_nimbus::over);
				l_ytdlp_text.tooltip(url_latest_ytdlp_relnotes);
				l_ytdlp_text.events().click.clear();
				l_ytdlp_text.events().click([this]
				{
					nana::system::open_url(url_latest_ytdlp_relnotes);
				});
			}
			updater_display_version_ytdlp();
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

	rgp_chan.on_checked([&](const nana::arg_checkbox &arg)
	{
		if(arg.widget->checked())
		{
			bool was_nightly {conf.ytdlp_nightly};
			conf.ytdlp_nightly = arg.widget->handle() == cb_chan_nightly;
			const auto fname {url_latest_ytdlp.substr(url_latest_ytdlp.rfind('/') + 1)};
			if(was_nightly && !conf.ytdlp_nightly || !was_nightly && conf.ytdlp_nightly || url_latest_ytdlp.empty() || fname != conf.ytdlp_path.filename().string())
			{
				if(thr_releases_ytdlp.joinable())
					thr_releases_ytdlp.detach();
				cb_chan_stable.enabled(false);
				cb_chan_nightly.enabled(false);
				l_ytdlp_text.error_mode(false);
				l_ytdlp_text.caption("checking...");
				get_latest_ytdlp(parent);
				updater_t2.start();
			}
		}
	});
}


void GUI::updater_display_version()
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
			if(!X64 && releases[0]["assets"].size() < 2)
			{
				l_vertext.error_mode(true);
				l_vertext.caption(tag_name + " lacks a 32-bit package!");
				return;
			}
			if(win7 && releases[0]["assets"].size() < 4)
			{
				l_vertext.error_mode(true);
				l_vertext.caption(tag_name + " lacks a Win7 package!");
				return;
			}
			vertext = tag_name + " (new version)";
			btn_update.enabled(true);
			std::string arc_url {releases[0]["assets"][X64 ? (win7 ? 2 : 0) : (win7 ? 3 : 1)]["browser_download_url"]};
			btn_update.tooltip(arc_url);
		}
		else vertext = tag_name + " (current)";
		l_vertext.caption(vertext);
		btn_changes.enabled(true);
	}
};


void GUI::updater_display_version_ffmpeg()
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
		l_ffmpeg_text.error_mode(false);
		if(ver_ffmpeg_latest > ver_ffmpeg)
		{
			std::string errclr {widgets::theme::is_dark() ? "<color=0xe09999>" : "<color=0xbb5555>"};
			l_ffmpeg_text.caption(ver_ffmpeg_latest.string() + "  (current = " +
				(!fs::exists(conf.ffmpeg_path / "ffmpeg.exe") ? errclr + "not present</>)" : ver_ffmpeg.string() + ")"));
			btn_update_ffmpeg.enabled(true);
			btn_update_ffmpeg.tooltip(url_latest_ffmpeg);
		}
		else l_ffmpeg_text.caption(ver_ffmpeg_latest.string() + "  (current)");
	}
};


void GUI::updater_display_version_ytdlp()
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
		l_ytdlp_text.error_mode(false);
		if(ver_ytdlp_latest != ver_ytdlp)
		{
			std::string errclr {widgets::theme::is_dark() ? "<color=0xe09999>" : "<color=0xbb5555>"};
			l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current = " +
				(not_present ? errclr + "not present</>)  [click for changelog]" : ver_ytdlp.string() + ")  [click for changelog]"));
			btn_update_ytdlp.enabled(true);
			btn_update_ytdlp.tooltip(url_latest_ytdlp);
		}
		else l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current)  [click to see changelog]");
	}
}


void GUI::updater_update_self(themed_form &parent)
{
	queue_save_data();
	static fs::path arc_path;
	static bool btnffmpeg_state, btnytdlp_state;
	arc_path = fs::temp_directory_path() / (X64 ? (win7 ? "ytdlp-interface_win7.7z" : "ytdlp-interface.7z") : (win7 ? "ytdlp-interface_x86_win7.7z" : "ytdlp-interface_x86.7z"));
	if(btn_update.caption() == "Update")
	{
		btn_update.caption("Cancel");
		btnffmpeg_state = btn_update_ffmpeg.enabled();
		btnytdlp_state = btn_update_ytdlp.enabled();
		btn_update_ffmpeg.enabled(false);
		btn_update_ytdlp.enabled(false);
		thr_updater = std::thread {[&parent, this]
		{
			updater_working = true;
			if(!X64 && releases[0]["assets"].size() < 2)
			{
				::widgets::msgbox mbox {parent, "ytdlp-interface update error"};
				mbox.icon(nana::msgbox::icon_error);
				(mbox << "The latest release on GitHub doesn't seem to contain a 32-bit build!")();
				btn_update.caption("Update");
				thr_updater.detach();
				return;
			}
			if(win7 && releases[0]["assets"].size() < 4)
			{
				::widgets::msgbox mbox {parent, "ytdlp-interface update error"};
				mbox.icon(nana::msgbox::icon_error);
				(mbox << "The latest release on GitHub doesn't seem to contain a Windows 7 package!")();
				btn_update.caption("Update");
				thr_updater.detach();
				return;
			}
			//               0    1       2        3
			// asset order: x64, x86, x64_win7, x86_win7
			std::map<std::string, unsigned> arc_sizes;
			std::map<std::string, std::string> arc_urls;
			auto j_assets {releases[0]["assets"]};
			for(const auto &asset : j_assets)
			{
				if(asset.contains("name"))
				{
					const auto name {asset["name"].get<std::string>()};
					arc_sizes[name] = asset["size"];
					arc_urls[name] = asset["browser_download_url"];
				}
			}
			std::string arc_url;
			unsigned arc_size {0}, progval {0};
			if(X64)
			{
				if(win7)
				{
					if(arc_sizes.contains("ytdlp-interface_win7.7z"))
						arc_size = arc_size["ytdlp-interface_win7.7z"];
					else arc_size = j_assets[2]["size"];
					if(arc_urls.contains("ytdlp-interface_win7.7z"))
						arc_url = arc_urls["ytdlp-interface_win7.7z"];
					else arc_url = j_assets[2]["browser_download_url"];
				}
				else
				{
					if(arc_sizes.contains("ytdlp-interface.7z"))
						arc_size = arc_size["ytdlp-interface.7z"];
					else arc_size = j_assets[0]["size"];
					if(arc_urls.contains("ytdlp-interface.7z"))
						arc_url = arc_urls["ytdlp-interface.7z"];
					else arc_url = j_assets[0]["browser_download_url"];
				}
			}
			else
			{
				if(win7)
				{
					if(arc_sizes.contains("ytdlp-interface_x86_win7.7z"))
						arc_size = arc_size["ytdlp-interface_x86_win7.7z"];
					else arc_size = j_assets[3]["size"];
					if(arc_urls.contains("ytdlp-interface_x86_win7.7z"))
						arc_url = arc_urls["ytdlp-interface_x86_win7.7z"];
					else arc_url = j_assets[3]["browser_download_url"];
				}
				else
				{
					if(arc_sizes.contains("ytdlp-interface_x86.7z"))
						arc_size = arc_size["ytdlp-interface_x86.7z"];
					else arc_size = j_assets[1]["size"];
					if(arc_urls.contains("ytdlp-interface_x86.7z"))
						arc_url = arc_urls["ytdlp-interface_x86.7z"];
					else arc_url = j_assets[1]["browser_download_url"];
				}
			}
			prog_updater.amount(arc_size);
			prog_updater.value(0);
			auto cb_progress = [&](unsigned prog_chunk)
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
						if(conf.update_self_only)
							params += L" self_only";
						ShellExecuteW(NULL, L"runas", tempself.wstring().data(), params.data(), NULL, SW_SHOW);
						updater_working = false;
						thr_updater.detach();
						ExitProcess(0);
						return;
					}
					catch(fs::filesystem_error const &e) {
						::widgets::msgbox mbox {parent, "File copy error"};
						mbox.icon(nana::msgbox::icon_error);
						(mbox << e.what())();
					}
				}
				else prog_updater.caption(dl_error);
				btn_update.caption("Update");
				btn_update_ffmpeg.enabled(btnffmpeg_state);
				btn_update_ytdlp.enabled(btnytdlp_state);
				updater_working = false;
				thr_updater.detach();
			}
		}};
	}
	else
	{
		btn_update.caption("Update");
		prog_updater.caption("");
		prog_updater.value(0);
		btn_update_ffmpeg.enabled(btnffmpeg_state);
		btn_update_ytdlp.enabled(btnytdlp_state);
		updater_working = false;
		thr_updater.detach();
	}
};


void GUI::updater_update_misc(bool ytdlp, fs::path target)
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
		thr_updater = std::thread {[ytdlp, target, this]
		{
			updater_working = true;
			unsigned progval {0};
			const auto arc_size {ytdlp ? size_latest_ytdlp : size_latest_ffmpeg};
			const auto arc_url {ytdlp ? url_latest_ytdlp : url_latest_ffmpeg};
			const auto fname {url_latest_ytdlp.substr(url_latest_ytdlp.rfind('/') + 1)};
			prog_updater_misc.amount(arc_size);
			prog_updater_misc.value(0);
			auto cb_progress = [&](unsigned prog_chunk)
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
				if(fs::exists(fs::temp_directory_path() / fname))
				{
					std::error_code ec;
					fs::remove(fs::temp_directory_path() / fname, ec);
				}
				download_result = util::dl_inet_res(arc_url, fs::temp_directory_path() / fname, &updater_working, cb_progress);
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
							auto error {util::extract_7z(arc_path, tempdir, cb_ffplay.checked() + 1)};
							if(error.empty())
							{
								prog_updater_misc.caption(std::string {"Copying files to "} + (target == appdir ?
																					   "program folder..." : "FFmpeg folder..."));
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
									conf.ffmpeg_path = target;
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
						std::error_code ec;
						fs::rename(fs::temp_directory_path() / fname, target / fname, ec);
						if(!ec)
						{
							conf.ytdlp_path = target / fname;
							ver_ytdlp = ver_ytdlp_latest;
							btnytdlp_state = false;
							prog_updater_misc.caption("yt-dlp update complete");
							l_ytdlp_text.caption(ver_ytdlp_latest.string() + "  (current)  [click to see changelog]");
							l_ytdlp_text.error_mode(false);
							btn_update_ytdlp.tooltip("");
						}
						else prog_updater_misc.caption("Failed to move " + fname + " to set folder: " + ec.message());
					}
				}
				else prog_updater_misc.caption(download_result);

				btn_update_ytdlp.caption(btntext_ytdlp);
				btn_update_ffmpeg.caption(btntext_ffmpeg);
				btn_update_ffmpeg.enabled(btnffmpeg_state);
				btn_update_ytdlp.enabled(btnytdlp_state);
				btn_update.enabled(btnupdate_state);
				updater_working = false;
				thr_updater.detach();
			}
		}};
	}
	else
	{
		updater_working = false;
		btn_update_ytdlp.caption(btntext_ytdlp);
		btn_update_ffmpeg.caption(btntext_ffmpeg);
		btn_update_ffmpeg.enabled(btnffmpeg_state);
		btn_update_ytdlp.enabled(btnytdlp_state);
		btn_update.enabled(btnupdate_state);
		thr_updater.detach();
	}
};


void GUI::updater_init_page(nana::window parent_for_msgbox)
{
	l_ffmpeg_text.error_mode(false);
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
		else updater_display_version();
	}
	else if(releases.empty())
	{
		get_releases(parent_for_msgbox);
		updater_t0.start();
	}
	else updater_display_version();

	if(url_latest_ffmpeg.empty())
	{
		get_latest_ffmpeg(parent_for_msgbox);
		updater_t1.start();
	}
	else updater_display_version_ffmpeg();

	if(!url_latest_ytdlp_relnotes.empty() && fs::path {url_latest_ytdlp}.filename() == conf.ytdlp_path.filename())
	{
		nana::api::effects_edge_nimbus(l_ytdlp_text, nana::effects::edge_nimbus::over);
		l_ytdlp_text.tooltip("Click to view release notes in web browser.");
		l_ytdlp_text.events().click([this]
		{
			nana::system::open_url(url_latest_ytdlp_relnotes);
		});
		updater_display_version_ytdlp();
	}

	if(conf.ytdlp_nightly)
		cb_chan_nightly.check(true);
	else cb_chan_stable.check(true);
}


bool GUI::updater_check_paths(bool ffmpeg_only)
{
	if(!fs::exists(conf.ffmpeg_path / "ffmpeg.exe"))
	{
		/*if(conf.ffmpeg_path == fs::current_path())
			l_ffmpeg_path.caption("!!!  FFMPEG.EXE NOT FOUND IN THE PROGRAM FOLDER  !!!");
		else l_ffmpeg_path.caption("!!!  FFMPEG.EXE NOT FOUND IN THE SELECTED FOLDER  !!!");*/
		ver_ffmpeg = {0, 0, 0};
		//l_ffmpeg_path.refresh_theme();
		updater_t1.start();
		if(ffmpeg_only) return false;
	}
	else
	{
		l_ffmpeg_path.update_caption();
		l_ffmpeg_path.refresh_theme();
	}

	if(!ffmpeg_only)
	{
		bool path_empty {conf.ytdlp_path.empty()}, path_gone {!fs::exists(conf.ytdlp_path)};
		if(path_empty || path_gone)
		{
			if(!win7 && fs::exists(".\\ytdl-patched-red.exe"))
			{
				GUI::conf.ytdlp_path = ".\\ytdl-patched-red.exe";
				l_ytdlp_path.update_caption();
				get_version_ytdlp();
			}
			else if(fs::exists(appdir / ytdlp_fname))
			{
				GUI::conf.ytdlp_path = ".\\" + ytdlp_fname;
				l_ytdlp_path.update_caption();
				get_version_ytdlp();
			}
			else
			{
				if(path_empty)
					l_ytdlp_path.caption("!!!  YT-DLP EXECUTABLE NOT FOUND IN PROGRAM FOLDER  !!!");
				else
				{
					l_ytdlp_path.caption("!!!  YT-DLP EXECUTABLE NOT FOUND AT ITS SELECTED LOCATION  !!!");
					conf.ytdlp_path.clear();
				}
				ver_ytdlp = {0, 0, 0};
			}
			updater_t2.start();
		}
		else l_ytdlp_path.update_caption();
		l_ytdlp_path.refresh_theme();
	}
	return true;
}