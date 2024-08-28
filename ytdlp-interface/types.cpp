#include "types.hpp"


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
