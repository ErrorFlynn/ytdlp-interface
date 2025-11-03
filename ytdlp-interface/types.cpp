#include "types.hpp"
#include "icons.hpp"
#include "util.hpp"


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


void theme_t::to_json(nlohmann::json &j)
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
	j["cb_android"] = cb_android;
	j["com_chap"] = com_chap;
	j["com_cookies"] = com_cookies;
	j["max_data_threads"] = max_data_threads;
	j["cookie_options"] = to_utf8(cookie_options);
	j["cb_display_custom_filenames"] = cb_display_custom_filenames;
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
	cb_android = j["cb_android"];
	com_chap = j["com_chap"];
	com_cookies = j["com_cookies"];
	max_data_threads = j["max_data_threads"];
	cookie_options = j["cookie_options"].get<std::string>();
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
	cb_android = p.cb_android;
	com_chap = p.com_chap;
	com_cookies = p.com_cookies;
	max_data_threads = p.max_data_threads;
	cookie_options = p.cookie_options;
	cb_display_custom_filenames = p.cb_display_custom_filenames;
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
		cb_android == p.cb_android &&
		com_chap == p.com_chap &&
		com_cookies == p.com_cookies &&
		max_data_threads == p.max_data_threads &&
		cb_display_custom_filenames == p.cb_display_custom_filenames &&
		cookie_options == p.cookie_options;
}


/*void argset_t::set(std::string_view text)
{
	if(text.starts_with('['))
	{
		auto pos {text.find(']')};
		if(pos != -1)
		{
			label_ = text.substr(0, ++pos);
			auto pos2 {text.find_first_not_of(" \t", pos)};
			if(pos2 != -1)
			{
				argset_ = text.substr(pos2);
			}
		}
		else argset_ = text;
	}
	else argset_ = text;
}*/
