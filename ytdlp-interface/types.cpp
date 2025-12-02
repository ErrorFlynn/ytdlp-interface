#include "types.hpp"
#include "icons.hpp"
#include "util.hpp"
#include "gui.hpp"


std::string version_t::string()
{
	auto fmt = [](int i)
	{
		auto str {std::to_string(i)};
		return str.size() == 1 ? '0' + str : str;
	};
	return std::to_string(year) + '.' + fmt(month) + '.' + fmt(day);
}


bool version_t::operator > (const version_t &o)
{
	auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
	auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
	return self > other;
}


bool version_t::operator == (const version_t &o)
{
	auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
	auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
	return self == other;
}


bool version_t::operator != (const version_t &o)
{
	auto self {std::chrono::days{day} + std::chrono::months{month} + std::chrono::years{year}};
	auto other {std::chrono::days{o.day} + std::chrono::months{o.month} + std::chrono::years{o.year}};
	return self != other;
}


semver_t::semver_t(std::string ver_tag)
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


bool semver_t::operator > (const semver_t &o)
{
	return major > o.major || major == o.major && minor > o.minor || major == o.major && minor == o.minor && patch > o.patch;
}


bool semver_t::operator < (const semver_t &o)
{
	return major < o.major || major == o.major && minor < o.minor || major == o.major && minor == o.minor && patch < o.patch;
}


bool semver_t::operator == (const semver_t &o)
{
	return major == o.major && minor == o.minor && patch == o.patch;
}


void favicon_t::add(std::string favicon_url, callback fn)
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
					res = util::get_inet_res(favicon_url, &error);
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


favicon_t::~favicon_t()
{
	if(thr.joinable())
	{
		working = false;
		thr.join();
	}
}


void theme_t::make_default(bool dark)
{
	using namespace nana;

	if(dark)
	{
		nimbus = color {"#cde"};
		fmbg_raw = color {"#2c2b2b"};
		tbfg = lbfg  = color {"#f7f7f4"};
		btn_bg_raw = color {"#2e2d2d"};
		Label_fg = btn_fg = cbox_fg = color {"#e6e6e3"};
		Text_fg = color {"#def"};
		Text_fg_error = color {"#f99"};
		path_fg = colors::white;
		sep_bg = color {"#777"};
		expcol_fg = color {"#999"};
		gpfg = "0xE4D6BA";
		gpfg_clr = color {"#E4D6BA"};
		lb_headerbg_raw = color {"#525658"};
		title_fg = nana::color {"#cde"};
		path_link_fg = color {"#E4D6BA"};
		tbkw = color {"#b5c5d5"};
		tbkw_id = color {"#ccc"};
		tbkw_special = color {"#F0B0A0"};
		tbkw_warning = color {"#EEBF00"};
		tbkw_error = color {"#CA86E3"};
		overlay_fg = border = color {"#666"};
		tb_selbg = color {"#95443B"};
		tree_selbg = color {"#354F5C"};
		tree_selfg = color {"#569EBD"};
		tree_hilitebg = color {"#28353D"};
		tree_hilitefg = color {"#48606A"};
		tree_parent_node = color {"#ddd"};
		tree_expander = colors::white;
		tree_expander_hovered = color {"#ade"};
		tree_key_fg = color {"#ebebe3"};
		tree_val_fg = color {"#d8e8ff"};
		list_check_highlight_fg = color {"#bbb"};
		list_check_highlight_bg = color {"#6D4941"};
		msg_label_fg = Label_fg;
		lbselfg = color {"#AC4F44"};
		lbselbg = color {"#B05348"};
		lbhilite = color {"#544"};
		contrast(shade, true);
	}
	else
	{
		nimbus = color {"#60c8fd"};
		fmbg_raw = btn_bg_raw = colors::white;
		tbfg = lbfg = colors::black;
		Label_fg = color {"#569"};
		Text_fg = color {"#575"};
		Text_fg_error = color {"#a55"};
		cbox_fg = color {"#458"};
		btn_fg = color {"#67a"};
		path_fg = color {"#354"};
		path_link_fg = color {"#789"};
		sep_bg = color {"#c5c5c5"};
		expcol_fg = color {"#aaa"};
		gpfg = "0x81544F";
		gpfg_clr = color {"#81544F"};
		lb_headerbg_raw = color {"#f1f2f4"};
		title_fg = color {"#789"};
		tbkw = color {"#272"};
		tbkw_id = color {"#777"};
		tbkw_special = color {"#722"};
		tbkw_warning = color {"#B96C00"};
		tbkw_error = color {"#aa2222"};
		overlay_fg = color {"#bcc0c3"};
		border = color {"#9CB6C5"};
		tb_selbg = color {"#5695D3"};
		tree_selbg = color {"#eaf0f6"};
		tree_selfg = color {"#97aeb4"};
		tree_hilitebg = color {"#e7eef4"};
		tree_hilitefg = color {"#d7dee4"};
		tree_parent_node = color {"#909090"};
		tree_expander = colors::black;
		tree_expander_hovered = colors::deep_sky_blue;
		tree_key_fg = color {"#459"};
		tree_val_fg = color {"#474"};
		list_check_highlight_fg = color {"#888"};
		list_check_highlight_bg = color {"#D5D8e0"};
		msg_label_fg = color {"#457"};
		lbselfg = color {"#c7dEe4"};
		lbselbg = color {"#d0e2e8"};
		lbhilite = color {"#eee"};
		contrast(shade, false);
	}
}


void theme_t::contrast(double factor, bool dark)
{
	using namespace nana;
	shade = std::clamp(factor, .0, .3);
	if(dark)
	{
		fmbg = fmbg_raw.blend(colors::black, shade);
		lbbg = fmbg.blend(colors::black, .035);
		gpbg = tree_bg = fmbg.blend(colors::white, .035);
		btn_bg = btn_bg_raw.blend(colors::black, shade);
		lb_headerbg = lb_headerbg_raw.blend(colors::black, shade);
		tb_selbg_unfocused = tb_selbg.blend(colors::black, .3);
		tree_selhilitebg = tree_selbg.blend(colors::white, .06);
		tree_selhilitefg = tree_selfg.blend(colors::white, .2);
	}
	else
	{
		fmbg = lbbg = tree_bg = fmbg_raw.blend(colors::light_grey, .3 - shade);
		btn_bg = btn_bg_raw.blend(colors::light_grey, .3 - shade);
		gpbg = fmbg.blend(colors::black, .025);
		lb_headerbg = lb_headerbg_raw.blend(colors::white, shade);
		tb_selbg_unfocused = tb_selbg.blend(nana::colors::white, .3);
		tree_selhilitebg = tree_selbg.blend(colors::gray, .1);
		tree_selhilitefg = tree_selfg.blend(colors::gray, .1);
	}
}


namespace nana
{
	void to_json(nlohmann::json &j, const color &clr)
	{
		j = clr.rgba().value >> 8;
	}

	void from_json(const nlohmann::json &j, color &clr)
	{
		clr = static_cast<color_rgb>(j.get<unsigned>());
	}
}


void theme_t::to_json(nlohmann::json &j) const
{
	j["nimbus"] = nimbus;
	j["fmbg_raw"] = fmbg_raw;
	j["tbfg"] = tbfg;
	j["btn_bg_raw"] = btn_bg_raw;
	j["Label_fg"] = Label_fg;
	j["btn_fg"] = btn_fg;
	j["cbox_fg"] = cbox_fg;
	j["Text_fg"] = Text_fg;
	j["Text_fg_error"] = Text_fg_error;
	j["path_fg"] = path_fg;
	j["sep_bg"] = sep_bg;
	j["expcol_fg"] = expcol_fg;
	j["gpfg"] = gpfg;
	j["lb_headerbg_raw"] = lb_headerbg_raw;
	j["title_fg"] = title_fg;
	j["path_link_fg"] = path_link_fg;
	j["tbkw"] = tbkw;
	j["tbkw_id"] = tbkw_id;
	j["tbkw_special"] = tbkw_special;
	j["tbkw_warning"] = tbkw_warning;
	j["tbkw_error"] = tbkw_error;
	j["overlay_fg"] = overlay_fg;
	j["border"] = border;
	j["tb_selbg"] = tb_selbg;
	j["tree_selbg"] = tree_selbg;
	j["tree_selfg"] = tree_selfg;
	j["tree_hilitebg"] = tree_hilitebg;
	j["tree_hilitefg"] = tree_hilitefg;
	j["tree_selhilitebg"] = tree_selhilitebg;
	j["tree_selhilitefg"] = tree_selhilitefg;
	j["tree_parent_node"] = tree_parent_node;
	j["tree_expander"] = tree_expander;
	j["tree_expander_hovered"] = tree_expander_hovered;
	j["tree_key_fg"] = tree_key_fg;
	j["tree_val_fg"] = tree_val_fg;
	j["list_check_highlight_fg"] = list_check_highlight_fg;
	j["list_check_highlight_bg"] = list_check_highlight_bg;
	j["msg_label_fg"] = msg_label_fg;
	j["lbselbg"] = lbselbg;
	j["lbselfg"] = lbselfg;
	j["lbhilite"] = lbhilite;
	j["lbfg"] = lbfg;
}


void theme_t::from_json(const nlohmann::json &j)
{
	nimbus = j["nimbus"];
	fmbg_raw = j["fmbg_raw"];
	tbfg = j["tbfg"];
	btn_bg_raw = j["btn_bg_raw"];
	Label_fg = j["Label_fg"];
	btn_fg = j["btn_fg"];
	cbox_fg = j["cbox_fg"];
	Text_fg = j["Text_fg"];
	Text_fg_error = j["Text_fg_error"];
	path_fg = j["path_fg"];
	sep_bg = j["sep_bg"];
	expcol_fg = j["expcol_fg"];
	gpfg = j["gpfg"];
	lb_headerbg_raw = j["lb_headerbg_raw"];
	title_fg = j["title_fg"];
	path_link_fg = j["path_link_fg"];
	tbkw = j["tbkw"];
	tbkw_id = j["tbkw_id"];
	tbkw_special = j["tbkw_special"];
	tbkw_warning = j["tbkw_warning"];
	tbkw_error = j["tbkw_error"];
	overlay_fg = j["overlay_fg"];
	border = j["border"];
	tb_selbg = j["tb_selbg"];
	tree_selbg = j["tree_selbg"];
	tree_selfg = j["tree_selfg"];
	tree_hilitebg = j["tree_hilitebg"];
	tree_hilitefg = j["tree_hilitefg"];
	tree_selhilitebg = j["tree_selhilitebg"];
	tree_selhilitefg = j["tree_selhilitefg"];
	tree_parent_node = j["tree_parent_node"];
	tree_expander = j["tree_expander"];
	tree_expander_hovered = j["tree_expander_hovered"];
	tree_key_fg = j["tree_key_fg"];
	tree_val_fg = j["tree_val_fg"];
	list_check_highlight_fg = j["list_check_highlight_fg"];
	list_check_highlight_bg = j["list_check_highlight_bg"];
	msg_label_fg = j["msg_label_fg"];
	lbselbg = j["lbselbg"];
	lbselfg = j["lbselfg"];
	lbhilite = j["lbhilite"];
	lbfg = j["lbfg"];
}


void settings_t::to_jpreset(nlohmann::json &j) const
{
	using namespace nana;
	if(!j.empty())
		j.clear();
	j["outpath"] = outpath;
	j["fmt1"] = to_utf8(fmt1);
	j["fmt2"] = to_utf8(fmt2);
	j["ratelim"] = ratelim;
	j["ratelim_unit"] = ratelim_unit;
	j["cbsubs"] = cbsubs;
	j["cbthumb"] = cbthumb;
	j["cbtime"] = cbtime;
	j["cbkeyframes"] = cbkeyframes;
	j["cbmp3"] = cbmp3;
	j["pref_res"] = pref_res;
	j["pref_video"] = pref_video;
	j["pref_audio"] = pref_audio;
	j["pref_fps"] = pref_fps;
	j["cbargs"] = cbargs;
	if(argsets.empty())
		j["argsets"] = nlohmann::json::array();
	else for(auto &el : argsets)
		j["argsets"].push_back(el);
	j["output_template"] = to_utf8(output_template);
	j["max_concurrent_downloads"] = max_concurrent_downloads;
	j["cb_lengthyproc"] = cb_lengthyproc;
	j["max_proc_dur"] = max_proc_dur.count();
	if(outpaths.empty())
		j["outpaths"] = nlohmann::json::array();
	else for(auto &path : outpaths)
		j["outpaths"].push_back(to_utf8(path));
	j["common_dl_options"] = common_dl_options;
	j["cb_autostart"] = cb_autostart;
	j["cb_queue_autostart"] = cb_queue_autostart;
	j["open_dialog_origin"] = open_dialog_origin;
	j["playlist_indexing"] = to_utf8(playlist_indexing);
	j["cb_zeropadding"] = cb_zeropadding;
	j["cb_playlist_folder"] = cb_playlist_folder;
	j["output_template_bandcamp"] = to_utf8(output_template_bandcamp);
	j["json_hide_null"] = json_hide_null;
	j["audio_multistreams"] = audio_multistreams;
	for(auto i : sblock_mark)
		j["sblock"]["mark"].push_back(i);
	for(auto i : sblock_remove)
		j["sblock"]["remove"].push_back(i);
	j["sblock"]["cb_mark"] = cb_sblock_mark;
	j["sblock"]["cb_remove"] = cb_sblock_remove;
	j["proxy"]["enabled"] = cb_proxy;
	j["proxy"]["URL"] = to_utf8(proxy);
	j["pref_vcodec"] = pref_vcodec;
	j["pref_acodec"] = pref_acodec;
	j["argset"] = argset;
	j["cb_premium"] = cb_premium;
	j["cb_save_errors"] = cb_save_errors;
	j["cb_clear_done"] = cb_clear_done;
	j["cb_add_on_focus"] = cb_add_on_focus;
	j["com_chap"] = com_chap;
	j["com_cookies"] = com_cookies;
	j["max_data_threads"] = max_data_threads;
	j["cookie_options"] = to_utf8(cookie_options);
	j["cb_display_custom_filenames"] = cb_display_custom_filenames;
	j["cb_aria"] = cb_aria;
	j["aria_options"] = aria_options;
}


void settings_t::from_jpreset(const nlohmann::json &j)
{
	using nana::to_wstring;
	outpath = j["outpath"].get<std::string>();
	fmt1 = to_wstring(j["fmt1"].get<std::string>());
	fmt2 = to_wstring(j["fmt2"].get<std::string>());
	ratelim = j["ratelim"];
	ratelim_unit = j["ratelim_unit"];
	cbsubs = j["cbsubs"];
	cbthumb = j["cbthumb"];
	cbtime = j["cbtime"];
	cbkeyframes = j["cbkeyframes"];
	cbmp3 = j["cbmp3"];
	pref_res = j["pref_res"];
	pref_video = j["pref_video"];
	pref_audio = j["pref_audio"];
	pref_fps = j["pref_fps"];
	cbargs = j["cbargs"];
	for(auto &el : j["argsets"])
		argsets.push_back(el.get<std::string>());
	output_template = to_wstring(j["output_template"].get<std::string>());
	max_concurrent_downloads = j["max_concurrent_downloads"];
	cb_lengthyproc = j["cb_lengthyproc"];
	max_proc_dur = std::chrono::milliseconds {j["max_proc_dur"].get<int>()};
	if(j.contains("outpaths"))
		for(auto &el : j["outpaths"])
			outpaths.insert(to_wstring(el.get<std::string>()));
	common_dl_options = j["common_dl_options"];
	cb_autostart = j["cb_autostart"];
	cb_queue_autostart = j["cb_queue_autostart"];
	open_dialog_origin = j["open_dialog_origin"];
	playlist_indexing = to_wstring(j["playlist_indexing"].get<std::string>());
	cb_zeropadding = j["cb_zeropadding"];
	cb_playlist_folder = j["cb_playlist_folder"];
	output_template_bandcamp = to_wstring(j["output_template_bandcamp"].get<std::string>());
	json_hide_null = j["json_hide_null"];
	audio_multistreams = j["audio_multistreams"];
	const auto &jblock {j["sblock"]};
	if(jblock.contains("mark"))
		for(auto &el : jblock["mark"])
			sblock_mark.push_back(el.get<int>());
	if(jblock.contains("remove"))
		for(auto &el : jblock["remove"])
			sblock_remove.push_back(el.get<int>());
	cb_sblock_mark = j["sblock"]["cb_mark"];
	cb_sblock_remove = j["sblock"]["cb_remove"];
	cb_proxy = j["proxy"]["enabled"];
	proxy = to_wstring(j["proxy"]["URL"].get<std::string>());
	pref_vcodec = j["pref_vcodec"];
	pref_acodec = j["pref_acodec"];
	argset = j["argset"];
	cb_premium = j["cb_premium"];
	cb_save_errors = j["cb_save_errors"];
	cb_clear_done = j["cb_clear_done"];
	cb_add_on_focus = j["cb_add_on_focus"];
	com_chap = j["com_chap"];
	com_cookies = j["com_cookies"];
	max_data_threads = j["max_data_threads"];
	cookie_options = j["cookie_options"].get<std::string>();
	/*if(j.contains("cb_aria")); // inexplicably fails (returns true when key "cb_aria" doesn't exist)
	{
		cb_aria = j["cb_aria"];
		aria_options = j["aria_options"].get<std::string>();
	}*/
	try
	{
		cb_aria = j.at("cb_aria");
		aria_options = j.at("aria_options").get<std::string>();
	}
	catch(const nlohmann::json::out_of_range) {}
	try
	{
		cb_cookies = j.at("cb_cookies");
		cookies_path = j.at("cookies_path").get<std::string>();
	}
	catch(const nlohmann::json::out_of_range) {}
	if(jblock.contains("cb_display_custom_filenames"))
		cb_display_custom_filenames = j["cb_display_custom_filenames"];
}


void settings_t::from_preset(const settings_t &p)
{
	outpath = p.outpath;
	fmt1 = p.fmt1;
	fmt2 = p.fmt2;
	ratelim = p.ratelim;
	ratelim_unit = p.ratelim_unit;
	cbsubs = p.cbsubs;
	cbthumb = p.cbthumb;
	cbtime = p.cbtime;
	cbkeyframes = p.cbkeyframes;
	cbmp3 = p.cbmp3;
	pref_res = p.pref_res;
	pref_video = p.pref_video;
	pref_audio = p.pref_audio;
	pref_fps = p.pref_fps;
	cbargs = p.cbargs;
	argsets = p.argsets;
	output_template = p.output_template;
	max_concurrent_downloads = p.max_concurrent_downloads;
	cb_lengthyproc = p.cb_lengthyproc;
	max_proc_dur = p.max_proc_dur;
	outpaths = p.outpaths;
	common_dl_options = p.common_dl_options;
	cb_autostart = p.cb_autostart;
	cb_queue_autostart = p.cb_queue_autostart;
	open_dialog_origin = p.open_dialog_origin;
	playlist_indexing = p.playlist_indexing;
	cb_zeropadding = p.cb_zeropadding;
	cb_playlist_folder = p.cb_playlist_folder;
	output_template_bandcamp = p.output_template_bandcamp;
	json_hide_null = p.json_hide_null;
	audio_multistreams = p.audio_multistreams;
	sblock_mark = p.sblock_mark;
	sblock_remove = p.sblock_remove;
	cb_sblock_mark = p.cb_sblock_mark;
	cb_sblock_remove = p.cb_sblock_remove;
	cb_proxy = p.cb_proxy;
	proxy = p.proxy;
	pref_vcodec = p.pref_vcodec;
	pref_acodec = p.pref_acodec;
	argset = p.argset;
	cb_premium = p.cb_premium;
	cb_save_errors = p.cb_save_errors;
	cb_clear_done = p.cb_clear_done;
	cb_add_on_focus = p.cb_add_on_focus;
	com_chap = p.com_chap;
	com_cookies = p.com_cookies;
	max_data_threads = p.max_data_threads;
	cookie_options = p.cookie_options;
	cb_display_custom_filenames = p.cb_display_custom_filenames;
	cb_aria = p.cb_aria;
	aria_options = p.aria_options;
	cb_cookies = p.cb_cookies;
	cookies_path = p.cookies_path;
}


bool settings_t::equals_preset(const settings_t &p)
{
	return 
		outpath == p.outpath &&
		fmt1 == p.fmt1 &&
		fmt2 == p.fmt2 &&
		ratelim == p.ratelim &&
		ratelim_unit == p.ratelim_unit &&
		cbsubs == p.cbsubs &&
		cbthumb == p.cbthumb &&
		cbtime == p.cbtime &&
		cbkeyframes == p.cbkeyframes &&
		cbmp3 == p.cbmp3 &&
		pref_res == p.pref_res &&
		pref_video == p.pref_video &&
		pref_audio == p.pref_audio &&
		pref_fps == p.pref_fps &&
		cbargs == p.cbargs &&
		argsets == p.argsets &&
		output_template == p.output_template &&
		max_concurrent_downloads == p.max_concurrent_downloads &&
		cb_lengthyproc == p.cb_lengthyproc &&
		max_proc_dur == p.max_proc_dur &&
		outpaths == p.outpaths &&
		common_dl_options == p.common_dl_options &&
		cb_autostart == p.cb_autostart &&
		cb_queue_autostart == p.cb_queue_autostart &&
		open_dialog_origin == p.open_dialog_origin &&
		playlist_indexing == p.playlist_indexing &&
		cb_zeropadding == p.cb_zeropadding &&
		cb_playlist_folder == p.cb_playlist_folder &&
		output_template_bandcamp == p.output_template_bandcamp &&
		json_hide_null == p.json_hide_null &&
		audio_multistreams == p.audio_multistreams &&
		sblock_mark == p.sblock_mark &&
		sblock_remove == p.sblock_remove &&
		cb_sblock_mark == p.cb_sblock_mark &&
		cb_sblock_remove == p.cb_sblock_remove &&
		cb_proxy == p.cb_proxy &&
		proxy == p.proxy &&
		pref_vcodec == p.pref_vcodec &&
		pref_acodec == p.pref_acodec &&
		argset == p.argset &&
		cb_premium == p.cb_premium &&
		cb_save_errors == p.cb_save_errors &&
		cb_clear_done == p.cb_clear_done &&
		cb_add_on_focus == p.cb_add_on_focus &&
		com_chap == p.com_chap &&
		com_cookies == p.com_cookies &&
		max_data_threads == p.max_data_threads &&
		cb_display_custom_filenames == p.cb_display_custom_filenames &&
		cb_aria == p.cb_aria &&
		aria_options == p.aria_options &&
		cookie_options == p.cookie_options &&
		cb_cookies == p.cb_cookies &&
		cookies_path == p.cookies_path;
}


void settings_t::to_json(nlohmann::json &j) const
{
	using nana::to_utf8;

	j["cb_custom_dark_theme"] = cb_custom_dark_theme;
	j["cb_custom_light_theme"] = cb_custom_light_theme;
	theme_dark.to_json(j["theme"]["dark"]);
	theme_light.to_json(j["theme"]["light"]);

	auto &jitems {j["unfinished_queue_items"] = nlohmann::json::array()};

	for(const auto &el : unfinished_queue_items)
	{
		if(el.first.empty())
			for(const auto &url : el.second)
				jitems.push_back(url);
		else if(!el.second.empty())
		{
			jitems.push_back(nlohmann::json::object());
			auto &jobj {jitems.back()};
			jobj["name"] = el.first;
			auto &jcat {jobj["items"] = nlohmann::json::array()};
			for(const auto &url : el.second)
				jcat.push_back(url);
		}
	}

	auto &jpresets {j["presets"] = nlohmann::json::array()};
	for(const auto &el : GUI::conf_presets)
	{
		jpresets.push_back(nlohmann::json::object());
		auto &jpreset {jpresets.back()};
		jpreset["name"] = el.first;
		el.second.to_jpreset(jpreset["data"]);
	}

	j["com_cookies"] = com_cookies;
	j["ytdlp_path"] = ytdlp_path;
	j["outpath"] = outpath;
	j["fmt1"] = to_utf8(fmt1);
	j["fmt2"] = to_utf8(fmt2);
	j["ratelim"] = ratelim;
	j["com_chap"] = com_chap;
	j["ratelim_unit"] = ratelim_unit;
	j["cbsubs"] = cbsubs;
	j["cbthumb"] = cbthumb;
	j["cbtime"] = cbtime;
	j["cbkeyframes"] = cbkeyframes;
	j["cbmp3"] = cbmp3;
	j["pref_res"] = pref_res;
	j["pref_video"] = pref_video;
	j["pref_audio"] = pref_audio;
	j["pref_fps"] = pref_fps;
	j["cbtheme"] = cbtheme;
	j["contrast"] = contrast;
	j["cbargs"] = cbargs;
	j["kwhilite"] = kwhilite;
	j["argsets"] = argsets;
	j["output_template"] = to_utf8(output_template);
	j["max_concurrent_downloads"] = max_concurrent_downloads;
	j["cb_lengthyproc"] = cb_lengthyproc;
	j["max_proc_dur"] = max_proc_dur.count();
	j["common_dl_options"] = common_dl_options;
	j["cb_autostart"] = cb_autostart;
	j["cb_queue_autostart"] = cb_queue_autostart;
	j["gpopt_hidden"] = gpopt_hidden;
	j["open_dialog_origin"] = open_dialog_origin;
	j["playlist_indexing"] = to_utf8(playlist_indexing);
	j["cb_zeropadding"] = cb_zeropadding;
	j["cb_playlist_folder"] = cb_playlist_folder;
	j["window"]["x"] = winrect.x;
	j["window"]["y"] = winrect.y;
	j["window"]["w"] = winrect.width;
	j["window"]["h"] = winrect.height;
	j["window"]["zoomed"] = zoomed;
	j["window"]["dpi"] = nana::api::screen_dpi(true);
	for(auto &path : outpaths)
		j["outpaths"].push_back(to_utf8(path));
	j["get_releases_at_startup"] = get_releases_at_startup;
	j["queue_columns"]["format"] = col_format;
	j["queue_columns"]["format_note"] = col_format_note;
	j["queue_columns"]["ext"] = col_ext;
	j["queue_columns"]["fsize"] = col_fsize;
	j["queue_columns"]["website_icon"] = col_site_icon;
	j["queue_columns"]["website_text"] = col_site_text;
	j["output_template_bandcamp"] = to_utf8(output_template_bandcamp);
	j["json_hide_null"] = json_hide_null;
	j["ytdlp_nightly"] = ytdlp_nightly;
	j["audio_multistreams"] = audio_multistreams;
	j["cbsnap"] = cbsnap;
	j["limit_output_buffer"] = limit_output_buffer;
	j["output_buffer_size"] = output_buffer_size;
	j["update_self_only"] = update_self_only;
	j["pref_vcodec"] = pref_vcodec;
	j["pref_acodec"] = pref_acodec;
	j["argset"] = argset;
	j["cb_premium"] = cb_premium;
	j["cbminw"] = cbminw;
	j["cb_save_errors"] = cb_save_errors;
	j["cb_ffplay"] = cb_ffplay;
	j["ffmpeg_path"] = ffmpeg_path;
	j["cb_clear_done"] = cb_clear_done;
	j["cb_formats_fsize_bytes"] = cb_formats_fsize_bytes;
	j["cb_add_on_focus"] = cb_add_on_focus;
	j["max_data_threads"] = max_data_threads;
	j["cookie_options"] = cookie_options;
	j["cb_aria"] = cb_aria;
	j["aria_options"] = aria_options;
	j["sub_format"] = sub_format;
	j["sub_langs"] = sub_langs;

	for(auto i : sblock_mark)
		j["sblock"]["mark"].push_back(i);
	for(auto i : sblock_remove)
		j["sblock"]["remove"].push_back(i);

	j["sblock"]["cb_mark"] = cb_sblock_mark;
	j["sblock"]["cb_remove"] = cb_sblock_remove;
	j["proxy"]["enabled"] = cb_proxy;
	j["proxy"]["URL"] = to_utf8(proxy);
	j["cb_display_custom_filenames"] = cb_display_custom_filenames;
	j["cb_cookies"] = cb_cookies;
	j["cookies_path"] = cookies_path;
}


void settings_t::from_json(const nlohmann::json &j)
{
	using nana::to_wstring;

	ytdlp_path = j["ytdlp_path"].get<std::string>();
	if(!ytdlp_path.empty())
		ytdlp_path = util::to_relative_path(ytdlp_path);
	if(ytdlp_path.empty() || !fs::exists(ytdlp_path))
	{
		auto appdir {util::appdir()};
		if(fs::exists(appdir / "ytdl-patched-red.exe"))
			ytdlp_path = ".\\ytdl-patched-red.exe";
		else if(fs::exists(appdir / ytdlp_fname))
			ytdlp_path = ".\\" + ytdlp_fname;
	}
	outpath = util::to_relative_path(j["outpath"].get<std::string>());
	fmt1 = to_wstring(j["fmt1"].get<std::string>());
	fmt2 = to_wstring(j["fmt2"].get<std::string>());
	ratelim = j["ratelim"];
	ratelim_unit = j["ratelim_unit"];
	if(j.contains("cbkeyframes")) // v1.1
	{
		cbsubs = j["cbsubs"];
		cbthumb = j["cbthumb"];
		cbtime = j["cbtime"];
		cbkeyframes = j["cbkeyframes"];
		cbmp3 = j["cbmp3"];
	}
	if(j.contains("pref_res")) // v1.2
	{
		if(!j.contains("json_hide_null")) // v2.2 added two options at the top
			pref_res = j["pref_res"] + 2;
		else pref_res = j["pref_res"];
		pref_video = j["pref_video"];
		pref_audio = j["pref_audio"];
		pref_fps = j["pref_fps"];
	}
	if(j.contains("cbtheme")) // v1.3
		cbtheme = j["cbtheme"];
	if(j.contains("contrast")) // v1.4
	{
		contrast = j["contrast"];
		cbargs = j["cbargs"];
		if(j.contains("args"))
		{
			argsets.push_back(j["args"].get<std::string>());
		}
	}
	if(j.contains("kwhilite")) // v1.5
	{
		kwhilite = j["kwhilite"];
		for(auto &el : j["argsets"])
			argsets.push_back(el.get<std::string>());
	}
	if(j.contains("output_template")) // v1.6
	{
		output_template = to_wstring(j["output_template"].get<std::string>());
		max_concurrent_downloads = j["max_concurrent_downloads"];
		cb_lengthyproc = j["cb_lengthyproc"];
		max_proc_dur = std::chrono::milliseconds {j["max_proc_dur"].get<int>()};
		if(!j["unfinished_queue_items"].empty())
		{
			auto &cat0 {unfinished_queue_items.emplace_back("", std::vector<std::string>{}).second};
			for(auto &el : j["unfinished_queue_items"])
			{
				if(el.is_string())
					cat0.push_back(el);
				else if(el.is_object())
				{
					auto &cat {unfinished_queue_items.emplace_back(el["name"], std::vector<std::string>{}).second};
					for(auto &url : el["items"])
						cat.push_back(url);
				}
			}
		}
		if(j.contains("outpaths"))
			for(auto &el : j["outpaths"])
				outpaths.insert(to_wstring(el.get<std::string>()));
		common_dl_options = j["common_dl_options"];
		cb_autostart = j["cb_autostart"];
	}
	else
	{
		output_template = output_template_default;
		outpaths.insert(outpath);
	}
	if(j.contains("cb_queue_autostart")) // v1.6.1
	{
		cb_queue_autostart = j["cb_queue_autostart"];
	}
	if(j.contains("open_dialog_origin")) // v1.7
	{
		gpopt_hidden = j["gpopt_hidden"];
		open_dialog_origin = j["open_dialog_origin"];
		playlist_indexing = to_wstring(j["playlist_indexing"].get<std::string>());
	}
	if(j.contains("cb_zeropadding")) // v1.8
	{
		cb_zeropadding = j["cb_zeropadding"];
		cb_playlist_folder = j["cb_playlist_folder"];
		winrect.x = j["window"]["x"];
		winrect.y = j["window"]["y"];
		winrect.width = j["window"]["w"];
		winrect.height = j["window"]["h"];
		zoomed = j["window"]["zoomed"];
		if(j["window"].contains("dpi"))
			dpi = j["window"]["dpi"];
		else dpi = nana::api::screen_dpi(true);
	}
	if(j.contains("get_releases_at_startup")) // v2.0
	{
		get_releases_at_startup = j["get_releases_at_startup"];
		col_format = j["queue_columns"]["format"];
		col_format_note = j["queue_columns"]["format_note"];
		col_ext = j["queue_columns"]["ext"];
		col_fsize = j["queue_columns"]["fsize"];
	}
	if(j.contains("output_template_bandcamp")) // v2.1
	{
		output_template_bandcamp = to_wstring(j["output_template_bandcamp"].get<std::string>());
	}
	if(j.contains("json_hide_null")) // v2.2
	{
		json_hide_null = j["json_hide_null"];
	}
	if(j.contains("ytdlp_nightly")) // v2.3
	{
		col_site_icon = j["queue_columns"]["website_icon"];
		col_site_text = j["queue_columns"]["website_text"];
		ytdlp_nightly = j["ytdlp_nightly"];
		audio_multistreams = j["audio_multistreams"];
	}
	if(j.contains("sblock")) // v2.4
	{
		const auto &jblock {j["sblock"]};
		if(jblock.contains("mark"))
			for(auto &el : jblock["mark"])
				sblock_mark.push_back(el.get<int>());
		if(jblock.contains("remove"))
			for(auto &el : jblock["remove"])
				sblock_remove.push_back(el.get<int>());
		cb_sblock_mark = j["sblock"]["cb_mark"];
		cb_sblock_remove = j["sblock"]["cb_remove"];
		cb_proxy = j["proxy"]["enabled"];
		proxy = to_wstring(j["proxy"]["URL"].get<std::string>());
	}
	if(j.contains("cbsnap")) // v2.5
	{
		cbsnap = j["cbsnap"];
	}
	if(j.contains("limit_output_buffer")) // v2.6
	{
		limit_output_buffer = j["limit_output_buffer"];
		output_buffer_size = j["output_buffer_size"];
	}
	if(j.contains("update_self_only")) // v2.7
	{
		update_self_only = j["update_self_only"];
		pref_vcodec = j["pref_vcodec"];
		pref_acodec = j["pref_acodec"];
		argset = j["argset"];
	}
	if(j.contains("cb_premium")) // v2.8
	{
		cb_premium = j["cb_premium"];
		cbminw = j["cbminw"];
		cb_save_errors = j["cb_save_errors"];
		cb_ffplay = j["cb_ffplay"];
	}
	if(j.contains("ffmpeg_path")) // v2.9
	{
		ffmpeg_path = j["ffmpeg_path"].get<std::string>();
		if(!ffmpeg_path.empty())
			ffmpeg_path = util::to_relative_path(ffmpeg_path);
	}
	if(j.contains("cb_clear_done")) // v2.11
	{
		cb_clear_done = j["cb_clear_done"];
		cb_formats_fsize_bytes = j["cb_formats_fsize_bytes"];
		cb_add_on_focus = j["cb_add_on_focus"];
	}
	if(j.contains("theme")) // v2.13
	{
		cb_custom_dark_theme = j["cb_custom_dark_theme"];
		cb_custom_light_theme = j["cb_custom_light_theme"];
		theme_dark.from_json(j["theme"]["dark"]);
		theme_dark.contrast(contrast, true);
		theme_light.from_json(j["theme"]["light"]);
		theme_light.contrast(contrast, false);
	}
	if(j.contains("com_chap")) // v2.13.3
	{
		com_chap = j["com_chap"];
	}
	if(j.contains("com_cookies")) // v2.14
	{
		com_cookies = j["com_cookies"];
	}
	if(j.contains("max_data_threads")) // v2.15
	{
		max_data_threads = j["max_data_threads"];
		cookie_options = j["cookie_options"].get<std::string>();
	}
	if(j.contains("presets")) // v2.16
	{
		for(auto &jpreset : j["presets"])
		{
			settings_t tempset;
			tempset.from_jpreset(jpreset["data"]);
			GUI::conf_presets[jpreset["name"]] = std::move(tempset);
		}
	}
	if(j.contains("cb_display_custom_filenames")) // v2.17
	{
		cb_display_custom_filenames = j["cb_display_custom_filenames"];
	}
	if(j.contains("cb_aria")) // v2.18
	{
		cb_aria = j["cb_aria"];
		aria_options = j["aria_options"].get<std::string>();
		sub_format = j["sub_format"].get<std::string>();
		sub_langs = j["sub_langs"].get<std::string>();
		cb_cookies = j["cb_cookies"];
		cookies_path = j["cookies_path"].get<std::string>();
	}
}