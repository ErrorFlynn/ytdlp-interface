#include "form_colors.hpp"
#include <nana/gui/filebox.hpp>
#include <iostream>

theme_t *container::t_custom, *container::t_conf, *container::t_def;
container *container::selected;
std::vector<container*> container::containers;
std::function<void()> container::btn_callback;

void container::refresh_buttons(bool state)
{
	for(auto c : containers)
		if(state)
			c->refresh_btn_state();
		else c->refresh_btn_theme();
}


void GUI::fm_colors(themed_form &parent)
{
	using widgets::theme;
	using namespace nana;

	fs::path theme_path;
	theme_t t_def;
	t_def.shade = theme::contrast();
	t_def.make_default(theme::is_dark());

	theme_t t_custom = theme::is_dark() ? conf.theme_dark : conf.theme_light;
	t_custom.contrast(conf.contrast, theme::is_dark());
	theme_t &t_conf {theme::is_dark() ? conf.theme_dark : conf.theme_light};
	t_custom.gpfg_clr = t_conf.gpfg_clr = color {"#" + t_custom.gpfg.substr(2)};
	container::t_custom = &t_custom;
	container::t_conf = &t_conf;
	container::t_def = &t_def;
	container::containers.clear();
	container::selected = nullptr;	

	themed_form fm {nullptr, parent, {}, appear::decorate<appear::minimize>{}};
	fm.center(1000, 696);
	fm.caption(title + " - custom " + (theme::is_dark() ? "dark " : "light ") + "color theme");
	fm.bgcolor(t_custom.fmbg);
	fm.snap(conf.cbsnap);

	fm.div(R"(
		vert margin=20
		<
			< vert
				<
					< vert weight=310
						<c_fmbg weight=33> <weight=6> <c_nimbus weight=33> <weight=6> <c_label weight=33> <weight=6>
						<c_itext weight=33> <weight=6> <c_itext_error weight=33> <weight=6> <c_sep weight=33> <weight=6>
						<c_gp weight=33> <weight=6> <c_cbox weight=33> <weight=6> <c_tbfg weight=33> <weight=6> <c_tbsel weight=33> <weight=6>
						<c_tbsel_unfocused weight=33> <weight=6> <c_title weight=33> <weight=6> <c_url weight=33> <weight=6> <c_path weight=33>
					>
					<weight=19>
					< vert weight=310
						<c_btnbg weight=33> <weight=6> <c_btnfg weight=33> <weight=6> <c_tree_selbg weight=33> <weight=6>
						<c_tree_selfg weight=33> <weight=6> <c_tree_hilitebg weight=33> <weight=6> <c_tree_hilitefg weight=33>
						<weight=6> <c_tree_parent weight=33> <weight=6> <c_json_key weight=33> <weight=6>
						<c_json_val weight=33> <weight=6> <c_headerbg weight=33> <weight=6> <c_lbselbg weight=33> <weight=6>
						<c_lbselfg weight=33> <weight=6> <c_lbhilite weight=33> <weight=6> <c_lbskip weight=33>
					>
				>
				<weight=22 <> <pic_info weight=22> <weight=5> <text_info weight=480> <>>
			>
			<weight=21>
			<vert weight=300 <pnl_picker weight=230> <weight=20> <gp weight=170> <weight=20><lb>>
		>
		<weight=20> <sep weight=3> <weight=20>
		<btnbar weight=35 <btnsave weight=90> <weight=20> <btnload weight=90> <>
			<btnapply weight=180> <weight=20> <btndef weight=190>>
	)");

	panel<true> pnl_picker {fm};
	place plc_picker {pnl_picker};
	pnl_picker.bgcolor(theme::fmbg.blend(theme::is_dark() ? colors::white : colors::black, .15));
	plc_picker.div("vert<<><picker weight=280><>><weight=10>");

	hsl_picker picker {pnl_picker};
	plc_picker["picker"] << picker;
	fm["pnl_picker"] << pnl_picker;
	picker.bgcolor(theme::fmbg.blend(theme::is_dark() ? colors::white : colors::black, .15));

	paint::image img_info;
	const auto dpi {nana::api::screen_dpi(true)};
	if(dpi >= 240)
		img_info.open(arr_info64_png, sizeof arr_info64_png);
	else if(dpi >= 192)
		img_info.open(arr_info48a_png, sizeof arr_info48a_png);
	else if(dpi >= 144)
		img_info.open(arr_info32_png, sizeof arr_info32_png);
	else if(dpi >= 96)
		img_info.open(arr_info22_ico, sizeof arr_info22_ico);

	picture pic_info {fm};
	pic_info.load(img_info);
	//pic_info.stretchable(true);
	pic_info.transparent(true);
	pic_info.align(align::center, align_v::center);

	container c_fmbg {fm}, c_label {fm}, c_itext {fm}, c_tbfg {fm}, c_tbsel {fm}, c_tbsel_unfocused {fm},
		c_nimbus {fm}, c_sep {fm}, c_btnbg {fm}, c_title {fm}, c_url {fm}, c_path {fm}, c_itext_error {fm}, c_lbskip {fm},
		c_btnfg {fm}, c_cbox {fm}, c_gp {fm}, c_headerbg {fm}, c_tree_selbg {fm}, c_tree_selfg {fm}, c_tree_hilitebg {fm},
		c_tree_hilitefg {fm}, c_tree_parent {fm}, c_json_key {fm}, c_json_val {fm}, c_lbselbg {fm}, c_lbselfg {fm}, c_lbhilite {fm};

	my_tbox tblabel {c_label, picker, &t_custom.Label_fg}, tbsep {c_sep, picker, &t_custom.sep_bg},
		tbitext {c_itext, picker, &t_custom.Text_fg}, tb_btnbg {c_btnbg, picker, &t_custom.btn_bg_raw},
		tbfmbg {c_fmbg, picker, &t_custom.fmbg_raw}, tb_btnfg {c_btnfg, picker, &t_custom.btn_fg},
		tbnimbus {c_nimbus, picker, &t_custom.nimbus}, tbcbox {c_cbox, picker, &t_custom.cbox_fg},
		tb_tbfg {c_tbfg, picker, &t_custom.tbfg}, tb_tbsel {c_tbsel, picker, &t_custom.tb_selbg},
		tb_tbsel_unfocused {c_tbsel_unfocused, picker, &t_custom.tb_selbg_unfocused}, tbtitle {c_title, picker, &t_custom.title_fg},
		tburl {c_url, picker, &t_custom.path_link_fg}, tbpath {c_path, picker, &t_custom.path_fg},
		tb_itext_error {c_itext_error, picker, &t_custom.Text_fg_error}, tbgp {c_gp, picker, &t_custom.gpfg_clr},
		tb_headerbg {c_headerbg, picker, &t_custom.lb_headerbg_raw}, tb_tree_selbg {c_tree_selbg, picker, &t_custom.tree_selbg},
		tb_tree_selfg {c_tree_selfg, picker, &t_custom.tree_selfg}, tb_tree_hilitebg {c_tree_hilitebg, picker, &t_custom.tree_hilitebg},
		tb_tree_hilitefg {c_tree_hilitefg, picker, &t_custom.tree_hilitefg}, tb_json_key {c_json_key, picker, &t_custom.tree_key_fg},
		tb_tree_parent {c_tree_parent, picker, &t_custom.tree_parent_node}, tb_json_val {c_json_val, picker, &t_custom.tree_val_fg},
		tb_lbselfg {c_lbselfg, picker, &t_custom.lbselfg}, tb_lbselbg {c_lbselbg, picker, &t_custom.lbselbg},
		tb_lbhilite {c_lbhilite , picker, &t_custom.lbhilite}, tb_lbskip {c_lbskip , picker, &t_custom.list_check_highlight_fg};

	my_label l_label {c_label, "Label"}, l_fmbg {c_fmbg, "Window background"}, l_nimbus {c_nimbus, "Focus rectangle"},
		l_sep {c_sep, "Separator"}, l_btnbg {c_btnbg, "Button background"}, l_btnfg {c_btnfg, "Button foreground"},
		l_tbfg {c_tbfg, "Textbox foreground"}, l_tbsel {c_tbsel, "Textbox selection"}, l_headerbg {c_headerbg, "Listbox header"},
		l_tbsel_unfocused {c_tbsel_unfocused, "Textbox sel. unfocused"}, l_gp {c_gp, "Group title"},
		l_tree_selbg {c_tree_selbg, "Tree selection bground"}, l_tree_selfg {c_tree_selfg, "Tree selection border"},
		l_tree_hilitebg {c_tree_hilitebg, "Tree hovered bground"}, l_tree_hilitefg {c_tree_hilitefg, "Tree hovered border"},
		l_tree_parent {c_tree_parent, "Tree parent node"}, l_json_key {c_json_key, "JSON key"}, l_json_val {c_json_val, "JSON value"},
		l_lbselbg {c_lbselbg, "Listbox selection bg"}, l_lbselfg {c_lbselfg, "Listbox sel. border"},
		l_lbhilite {c_lbhilite, "Listbox hovered item"}, l_lbskip {c_lbskip, "Listbox special item"};

	::widgets::Text l_itext {c_itext, "Info text"}, l_itext_error {c_itext_error, "Info text (error mode)"}, 
		text_info {fm, "Right-click a color description to reset the color to its default value."};
	l_itext_error.error_mode(true);
	::widgets::Separator sep {fm};
	::widgets::cbox cbox {c_cbox, "Checkbox"};
	::widgets::Button btnapply {fm, "Apply changes"}, btndef {fm, "Reset to default"}, btnsave {fm, "Save"}, btnload {fm, "Load"};
	::widgets::Title title {c_title, "Large header"};
	std::wstring url {L"URL box"};
	fs::path path {"c:\\path\\box"};
	::widgets::path_label l_url {c_url, &url}, l_path {c_path, &path};
	api::effects_edge_nimbus(l_url, effects::edge_nimbus::none);
	api::effects_edge_nimbus(l_path, effects::edge_nimbus::none);
	auto tip {"The custom color theme is automatically saved to\nthe settings file when you click the apply button,\n"
		"but you can also save/load it to/from an arbitrary file."};
	btnsave.tooltip(tip);
	btnload.tooltip(tip);

	::widgets::Group gp {fm, "JSON Viewer"};
	gp.div("jtree margin=10");

	nlohmann::json j
	{
		{"boolean", true},
		{"number", 3.141592},
		{"trio", {"one", "two", "three"}},
	};
	::widgets::JSON_Tree jtree {gp, j};
	gp["jtree"] << jtree;
	jtree.find("root").select(true);

	::widgets::Listbox lb {fm};
	lb.hilight_checked(true);
	lb.scheme().item_height_ex = 8;
	lb.typeface(paint::font_info {"Calibri", 12});
	lb.append_header("Media title", util::scale(200));
	lb.append_header("Status", util::scale(90));
	lb.at(0).append({"Good Media (part 1)", "queued"});
	lb.at(0).append({"Media I will download later", "skip"});
	lb.at(0).append({"Good Media (part 2)", "queued"});
	lb.at(0).at(1).check(true);
	lb.at(0).at(2).select(true);

	fm["c_fmbg"] << c_fmbg;
	fm["c_nimbus"] << c_nimbus;
	fm["c_label"] << c_label;
	fm["c_itext"] << c_itext;
	fm["c_itext_error"] << c_itext_error;
	fm["c_sep"] << c_sep;
	fm["c_btnbg"] << c_btnbg;
	fm["c_btnfg"] << c_btnfg;
	fm["c_cbox"] << c_cbox;
	fm["c_tbfg"] << c_tbfg;
	fm["c_tbsel"] << c_tbsel;
	fm["c_tbsel_unfocused"] << c_tbsel_unfocused;
	fm["c_title"] << c_title;
	fm["c_url"] << c_url;
	fm["c_path"] << c_path;
	fm["c_gp"] << c_gp;
	fm["c_headerbg"] << c_headerbg;
	fm["c_tree_selbg"] << c_tree_selbg;
	fm["c_tree_selfg"] << c_tree_selfg;
	fm["c_tree_hilitebg"] << c_tree_hilitebg;
	fm["c_tree_hilitefg"] << c_tree_hilitefg;
	fm["c_tree_parent"] << c_tree_parent;
	fm["c_json_key"] << c_json_key;
	fm["c_json_val"] << c_json_val;
	fm["c_lbselbg"] << c_lbselbg;
	fm["c_lbselfg"] << c_lbselfg;
	fm["c_lbhilite"] << c_lbhilite;
	fm["c_lbskip"] << c_lbskip;

	fm["pic_info"] << pic_info;
	fm["text_info"] << text_info;
	fm["sep"] << sep;
	fm["btnapply"] << btnapply;
	fm["btndef"] << btndef;
	fm["btnsave"] << btnsave;
	fm["btnload"] << btnload;
	fm["gp"] << gp;
	fm["lb"] << lb;

	c_fmbg["first"] << tbfmbg;
	c_fmbg["second"] << l_fmbg;
	c_nimbus["first"] << tbnimbus;
	c_nimbus["second"] << l_nimbus;
	c_label["first"] << tblabel;
	c_itext["first"] << tbitext;
	c_label["second"] << l_label;
	c_itext["second"] << l_itext;
	c_sep["first"] << tbsep;
	c_sep["second"] << l_sep;
	c_sep.target = &sep;
	c_btnbg["first"] << tb_btnbg;
	c_btnbg["second"] << l_btnbg;
	c_btnfg["first"] << tb_btnfg;
	c_btnfg["second"] << l_btnfg;
	c_cbox["first"] << tbcbox;
	c_cbox["second"] << cbox;
	c_tbfg["first"] << tb_tbfg;
	c_tbfg["second"] << l_tbfg;
	c_tbsel["first"] << tb_tbsel;
	c_tbsel["second"] << l_tbsel;
	c_tbsel_unfocused["first"] << tb_tbsel_unfocused;
	c_tbsel_unfocused["second"] << l_tbsel_unfocused;
	c_title["first"] << tbtitle;
	c_title["second"] << title;
	c_url["first"] << tburl;
	c_url["second"] << l_url;
	c_path["first"] << tbpath;
	c_path["second"] << l_path;
	c_itext_error["first"] << tb_itext_error;
	c_itext_error["second"] << l_itext_error;
	c_gp["first"] << tbgp;
	c_gp["second"] << l_gp;
	c_headerbg["first"] << tb_headerbg;
	c_headerbg["second"] << l_headerbg;
	c_tree_selbg["first"] << tb_tree_selbg;
	c_tree_selbg["second"] << l_tree_selbg;
	c_tree_selbg.target = &jtree;
	c_tree_selfg["first"] << tb_tree_selfg;
	c_tree_selfg["second"] << l_tree_selfg;
	c_tree_selfg.target = &jtree;
	c_tree_hilitebg["first"] << tb_tree_hilitebg;
	c_tree_hilitebg["second"] << l_tree_hilitebg;
	c_tree_hilitebg.target = &jtree;
	c_tree_hilitefg["first"] << tb_tree_hilitefg;
	c_tree_hilitefg["second"] << l_tree_hilitefg;
	c_tree_hilitefg.target = &jtree;
	c_tree_parent["first"] << tb_tree_parent;
	c_tree_parent["second"] << l_tree_parent;
	c_tree_parent.target = &jtree;
	c_json_key["first"] << tb_json_key;
	c_json_key["second"] << l_json_key;
	c_json_key.target = &jtree;
	c_json_val["first"] << tb_json_val;
	c_json_val["second"] << l_json_val;
	c_json_val.target = &jtree;
	c_lbselbg["first"] << tb_lbselbg;
	c_lbselbg["second"] << l_lbselbg;
	c_lbselbg.target = &lb;
	c_lbselfg["first"] << tb_lbselfg;
	c_lbselfg["second"] << l_lbselfg;
	c_lbselfg.target = &lb;
	c_lbhilite["first"] << tb_lbhilite;
	c_lbhilite["second"] << l_lbhilite;
	c_lbhilite.target = &lb;
	c_lbskip["first"] << tb_lbskip;
	c_lbskip["second"] << l_lbskip;
	c_lbskip.target = &lb;

	cbox.check(true);
	cbox.events().checked([&] { if(!cbox.checked()) cbox.check(true); });

	picker.on_change([&]
	{
		if(auto c {container::selected})
		{
			auto selclr {picker.color_value()};
			auto &tb {*dynamic_cast<my_tbox*>(c->widgets.front())};
			if(!tb.noupdate)
			{
				tb.noupdate = true;
				tb.caption(picker.rgbstr());
				tb.noupdate = false;
			}
			*tb.clr = selclr;
			c->refresh_btn_state();
			nana::api::effects_edge_nimbus(tb, nana::effects::edge_nimbus::none);
			nana::api::refresh_window(*c);

			auto wd_target {c->target->handle()};
			if(wd_target == l_fmbg)
			{
				t_custom.contrast(conf.contrast, theme::is_dark());
				theme::fmbg = t_custom.fmbg;
				theme::gpbg = t_custom.gpbg;
				theme::lbbg = t_custom.lbbg;
				theme::tree_bg = t_custom.tree_bg;
				fm.bgcolor(t_custom.fmbg);
				for(auto c : container::containers)
				{
					c->bgcolor(t_custom.fmbg);
					c->widgets.front()->events().expose.emit({}, *c->widgets.front());
				}
				pnl_picker.bgcolor(t_custom.fmbg.blend(theme::is_dark() ? colors::white : colors::black, .15));
				picker.bgcolor(pnl_picker.bgcolor());
				gp.refresh_theme();
				jtree.refresh_theme();
				lb.refresh_theme();
			}
			else if(wd_target == l_nimbus)
			{
				theme::nimbus = t_custom.nimbus;
				container::refresh_buttons(false);
				btndef.refresh_theme();
				btnapply.refresh_theme();
				btnsave.refresh_theme();
				btnload.refresh_theme();
				api::refresh_window(fm);
			}
			else if(wd_target == l_btnbg)
			{
				t_custom.contrast(conf.contrast, theme::is_dark());
				theme::btn_bg = t_custom.btn_bg;
				container::refresh_buttons(false);
				btndef.refresh_theme();
				btnapply.refresh_theme();
				btnsave.refresh_theme();
				btnload.refresh_theme();
			}
			else if(wd_target == l_btnfg)
			{
				theme::btn_fg = t_custom.btn_fg;
				container::refresh_buttons(false);
				btndef.refresh_theme();
				btnapply.refresh_theme();
				btnsave.refresh_theme();
				btnload.refresh_theme();
			}
			else if(wd_target == l_tbfg)
			{
				theme::tbfg = t_custom.tbfg;
				for(auto c : container::containers)
					c->widgets.front()->fgcolor(theme::tbfg);
			}
			else if(wd_target == l_itext)
			{
				theme::Text_fg = t_custom.Text_fg;
				l_itext.events().expose.emit({}, l_itext);
				text_info.events().expose.emit({}, text_info);
			}
			else if(wd_target == l_tbsel)
			{
				theme::tb_selbg = t_custom.tb_selbg;
				for(auto c : container::containers)
				{
					auto &tb {*dynamic_cast<my_tbox*>(c->widgets.front())};
					tb.scheme().selection = theme::tb_selbg;
				}
				tb_tbsel.focus();
				tb_tbsel.select(true);
			}
			else if(wd_target == l_tbsel_unfocused)
			{
				theme::tb_selbg_unfocused = t_custom.tb_selbg_unfocused;
				for(auto c : container::containers)
				{
					auto &tb {*dynamic_cast<my_tbox*>(c->widgets.front())};
					tb.scheme().selection_unfocused = theme::tb_selbg_unfocused;
				}
				tb_tbsel_unfocused.select(true);
			}
			else if(wd_target == l_label)
			{
				theme::Label_fg = t_custom.Label_fg;
				fm.refresh_widgets();
			}
			else if(wd_target == l_gp)
			{
				auto clrstr {"0x" + picker.rgbstr(&t_custom.gpfg_clr)};
				theme::gpfg = t_custom.gpfg = clrstr;
				gp.refresh_theme();
			}
			else if(wd_target == l_headerbg)
			{
				t_custom.contrast(conf.contrast, theme::is_dark());
				theme::lb_headerbg = t_custom.lb_headerbg;
				lb.refresh_theme();
			}
			else if(wd_target == jtree)
			{
				t_custom.contrast(conf.contrast, theme::is_dark());
				theme::import(t_custom);
				jtree.refresh_theme();
				api::refresh_window(jtree);
			}
			else
			{
				theme::import(t_custom);
				c->target->events().expose.emit({}, wd_target);
				api::refresh_window(*c->target);
			}
		}
		btnapply.enabled(t_custom != t_conf);
		btndef.enabled(t_custom != t_def);
	});

	container::btn_callback = [&]
	{
		btnapply.enabled(t_custom != t_conf);
		btndef.enabled(t_custom != t_def);
	};

	picker.events().mouse_up([&] {api::refresh_window(picker); });
	picker.events().mouse_down([&] {api::refresh_window(picker); });

	btnapply.events().click([&]
	{
		t_custom.contrast(conf.contrast, theme::is_dark());
		t_conf = t_custom;
		theme::import(t_custom);
		container::refresh_buttons(true);
		if(theme::is_dark())
		{
			if(conf.cb_custom_dark_theme)
			{
				parent.dark_theme(true);
				dark_theme(true);
			}
		}
		else
		{
			if(conf.cb_custom_light_theme)
			{
				parent.dark_theme(false);
				dark_theme(false);
			}
		}		
		btnapply.enabled(false);
		fn_write_conf();
	});

	btndef.events().click([&]
	{
		t_conf = t_custom = t_def;
		theme::import(t_custom);
		if(theme::is_dark())
		{
			if(conf.cb_custom_dark_theme)
			{
				parent.dark_theme(true);
				dark_theme(true);
			}
		}
		else
		{
			if(conf.cb_custom_light_theme)
			{
				parent.dark_theme(false);
				dark_theme(false);
			}
		}
		fm.refresh_widgets();
		fm.bgcolor(theme::fmbg);

		for(auto c : container::containers)
		{
			auto &tb {*dynamic_cast<my_tbox*>(c->widgets.front())};
			if(c == container::selected)
			{
				tb.apply();
				auto off {reinterpret_cast<intptr_t>(tb.clr) - reinterpret_cast<intptr_t>(&t_custom)};
				auto clrconf {reinterpret_cast<color*>(reinterpret_cast<intptr_t>(&t_conf) + off)};
				*tb.clr = *clrconf;
			}
			tb.noupdate = true;
			tb.caption(picker.rgbstr(tb.clr));
			tb.noupdate = false;
		}
		btndef.enabled(false);
		pnl_picker.bgcolor(t_custom.fmbg.blend(theme::is_dark() ? colors::white : colors::black, .15));
		picker.bgcolor(pnl_picker.bgcolor());
		container::refresh_buttons(true);
	});

	btnsave.events().click([&]
	{
		nana::filebox fb {fm, false};
		fb.allow_multi_select(false);
		if(theme::is_dark())
			fb.add_filter("Dark theme file", "*.dtheme");
		else fb.add_filter("Light theme file", "*.ltheme");
		fb.title("Choose which file to save to");
		if(!theme_path.empty())
			fb.init_file(theme_path);
		else
		{
			theme_path = confpath;
			if(theme::is_dark())
				fb.init_file(theme_path.replace_filename("custom dark theme"));
			else fb.init_file(theme_path.replace_filename("custom light theme"));
		}
		auto res {fb()};
		if(res.size())
		{
			theme_path = res.front();
			nlohmann::json j;
			if(theme::is_dark())
				t_custom.to_json(j["theme"]["dark"]);
			else t_custom.to_json(j["theme"]["light"]);
			std::ofstream {theme_path} << std::setw(4) << j;
		}
	});

	btnload.events().click([&]
	{
		nana::filebox fb {fm, true};
		fb.allow_multi_select(false);
		if(theme::is_dark())
			fb.add_filter("Dark theme file", "*.dtheme");
		else fb.add_filter("Light theme file", "*.ltheme");
		fb.title("Choose which file to load from");
		if(!theme_path.empty())
			fb.init_file(theme_path);
		else
		{
			theme_path = confpath;
			if(theme::is_dark())
				fb.init_file(theme_path.replace_filename("custom dark theme"));
			else fb.init_file(theme_path.replace_filename("custom light theme"));
		}
		auto res {fb()};
		if(res.size())
		{
			theme_path = res.front();
			nlohmann::json j;
			std::ifstream {theme_path} >> j;
			if(theme::is_dark())
				t_custom.from_json(j["theme"]["dark"]);
			else t_custom.from_json(j["theme"]["light"]);
			t_custom.contrast(conf.contrast, theme::is_dark());
			theme::import(t_custom);
			container::refresh_buttons(true);
			fm.refresh_widgets();
			api::refresh_window(fm);
			fm.bgcolor(theme::fmbg);
			btnapply.enabled(t_custom != t_conf);
			btndef.enabled(t_custom != t_def);
			auto &tb {*dynamic_cast<my_tbox*>(container::selected->widgets.front())};
			tb.apply();
		}
	});

	drawing {fm}.draw([&](paint::graphics &g)
	{
		using util::scale;
		rectangle r {pnl_picker.pos(), pnl_picker.size()};
		r.pare_off(-1);
		g.rectangle(r, false, theme::border);
		if(container::selected)
		{
			rectangle r {container::selected->pos(), container::selected->size()};
			const auto scale2 {scale(2)};
			r.pare_off(-scale2);
			g.round_rectangle(r, scale2, scale2, t_custom.nimbus, true, t_custom.nimbus);
			g.line({r.x, r.y - 1}, {int(r.x + r.width-1), r.y - 1});
			g.line({r.x + 2, r.y - 2}, {int(r.x + r.width - 3), r.y - 2});
			r.pare_off(-scale(5));
			g.blur(r, scale(1));
		}
	});

	fm.subclass_before(WM_KILLFOCUS, [&](UINT, WPARAM, LPARAM lparam, LRESULT*)
	{
		for(auto c : container::containers)
			c->bgcolor(theme::fmbg);
		return false;
	});

	fm.subclass_after(WM_KEYDOWN, [&](UINT, WPARAM wparam, LPARAM, LRESULT *)
	{
		if(wparam == 'S')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
				btnsave.events().click.emit({}, btnsave);
		}
		else if(wparam == 'L')
		{
			if(GetAsyncKeyState(VK_CONTROL) & 0xff00)
				btnload.events().click.emit({}, btnload);
		}
		return true;
	});

	fm.events().unload([&]
	{
		if(theme::is_dark() && !conf.cb_custom_dark_theme || !theme::is_dark() && !conf.cb_custom_light_theme)
			theme::import(t_def);
	});

	btnapply.enabled(t_custom != t_conf);
	btndef.enabled(t_custom != t_def);
	pnl_picker.bgcolor(t_custom.fmbg.blend(theme::is_dark() ? colors::white : colors::black, .15));
	picker.bgcolor(pnl_picker.bgcolor());

	container::containers.front()->widgets.front()->focus();
	theme::import(t_custom);

	fm.refresh_widgets();
	fm.collocate();
	fm.modality();
}


void container::refresh_btn_state()
{
	auto &tb {*dynamic_cast<my_tbox*>(widgets.front())};
	auto off {reinterpret_cast<intptr_t>(tb.clr) - reinterpret_cast<intptr_t>(t_custom)};
	auto clrconf {reinterpret_cast<nana::color*>(reinterpret_cast<intptr_t>(t_conf) + off)};
	btn.enable(*tb.clr != *clrconf);
	if(btn.enabled())
		btn.tooltip("Revert unapplied changes");
	else btn.tooltip("");
}


void container::create(nana::window parent)
{
	using ::widgets::theme;
	using namespace nana;

	containers.push_back(this);
	panel<true>::create(parent);
	plc.bind(*this);
	btn.create(*this);
	ffi.plc = &plc;
	ffi.widgets = &widgets;
	ffi.target = &target;
	refresh_theme();
	div("margin=5 weight=25 <first weight=80> <weight=15> <second> <weight=15> <vert weight=24 <btn weight=24>>");
	plc.field("btn") << btn;

	if(!selected)
	{
		selected = this;
		api::refresh_window(parent);
	}
	events().expose([this] { refresh_theme(); });
	events().resized([this] { plc.collocate(); });
	events().mouse_enter([&]
	{
		if(this != selected)
		{
			for(auto c : containers)
				c->bgcolor(theme::fmbg);
			bgcolor(theme::fmbg.blend(theme::is_dark() ? colors::white : colors::grey, .15));
		}

	});

	events().mouse_leave([this]
	{
		if(selected != this)
			bgcolor(theme::fmbg);
	});

	events().mouse_down([parent, this]
	{
		selected = this;
		widgets.front()->focus();
		bgcolor(theme::fmbg);
		api::refresh_window(parent);
	});

	events().mouse_up([this](const arg_mouse &arg)
	{
		if(!arg.is_left_button())
		{
			auto &tb {*dynamic_cast<my_tbox*>(widgets.front())};
			auto off {reinterpret_cast<intptr_t>(tb.clr) - reinterpret_cast<intptr_t>(t_custom)};
			auto clrdef {reinterpret_cast<color*>(reinterpret_cast<intptr_t>(t_def) + off)};
			*tb.clr = *clrdef;
			tb.apply();
			refresh_btn_state();
		}
	});

	btn.events().click([this]
	{
		auto &tb {*dynamic_cast<my_tbox*>(widgets.front())};
		auto off {reinterpret_cast<intptr_t>(tb.clr) - reinterpret_cast<intptr_t>(t_custom)};
		auto clrconf {reinterpret_cast<color*>(reinterpret_cast<intptr_t>(t_conf) + off)};
		*tb.clr = *clrconf;
		tb.apply();
		*tb.clr = *clrconf;
		tb.noupdate = true;
		tb.caption(tb.picker->rgbstr(tb.clr));
		tb.noupdate = false;
		tb.select(true);
		refresh_btn_state();
		if(btn_callback) btn_callback();
	});
}


nana::place::field_reference container::fake_field_interface::operator<<(nana::window wd)
{
	auto wdg {nana::api::get_widget(wd)};
	widgets->push_back(wdg);
	if(widgets->size() == 1)
	{
		auto &c {*dynamic_cast<container*>(parent)};
		c.refresh_btn_state();
	}
	else
	{
		*target = wdg;
		wdg->events().mouse_up([wdg, this](nana::arg_mouse arg)
		{
			if(!arg.is_left_button())
			{
				arg.window_handle = parent->handle();
				parent->events().mouse_up.emit(arg, *wdg);
			}
		});
	}
	wdg->events().mouse_enter([this](nana::arg_mouse arg)
	{
		arg.window_handle = parent->handle();
		parent->events().mouse_enter.emit(arg, parent->handle());
	});
	wdg->events().mouse_down([this](nana::arg_mouse arg)
	{
		arg.window_handle = parent->handle();
		parent->events().mouse_down.emit(arg, parent->handle());
	});
	return plc->field(field_name.data()).operator<<(wd);
}


void mybtn::create(nana::window parent)
{
	Button::create(parent);

	events().mouse_enter([parent, this](nana::arg_mouse arg)
	{
		arg.window_handle = parent;
		nana::api::get_widget(parent)->events().mouse_enter.emit(arg, parent);
	});

	events().mouse_down([parent, this](nana::arg_mouse arg)
	{
		arg.window_handle = parent;
		nana::api::get_widget(parent)->events().mouse_down.emit(arg, parent);
	});

	const auto dpi {nana::api::screen_dpi(true)};
	if(dpi >= 240)
	{
		image(arr_erase48_png, sizeof arr_erase48_png);
		image_disabled(arr_erase48_disabled_png, sizeof arr_erase48_disabled_png);
	}
	else if(dpi >= 192)
	{
		image(arr_erase32_png, sizeof arr_erase32_png);
		image_disabled(arr_erase32_disabled_png, sizeof arr_erase32_disabled_png);
	}
	else if(dpi > 96)
	{
		image(arr_erase22_ico, sizeof arr_erase22_ico);
		image_disabled(arr_erase22_disabled_ico, sizeof arr_erase22_disabled_ico);
	}
	else
	{
		image(arr_erase16_ico, sizeof arr_erase16_ico);
		image_disabled(arr_erase16_disabled_ico, sizeof arr_erase16_disabled_ico);
	}
}


my_tbox::my_tbox(nana::window parent, nana::hsl_picker &picker, nana::color *pclr) : Textbox {parent}, parent {parent}, clr {pclr}, picker {&picker}
{
	using namespace nana;

	caption(picker.rgbstr(pclr));
	text_align(align::center);
	typeface(paint::font_info {"Segoe UI", 11});
	focus_behavior(textbox::text_focus_behavior::select_if_tabstop_or_click);
	api::effects_edge_nimbus(*this, effects::edge_nimbus::none);

	events().focus([&](const arg_focus &arg)
	{
		select(arg.getting);
		if(arg.getting)
		{
			picker.color_value(*clr);
			if(text().size() != 6)
			{
				scheme().activated = colors::red;
				api::effects_edge_nimbus(*this, effects::edge_nimbus::active);
			}
		}
		else api::effects_edge_nimbus(*this, effects::edge_nimbus::none);
	});

	events().key_char([this](const arg_keyboard &arg)
	{
		if(arg.key > 0x60)
			arg.key -= 0x20;
	});

	set_accept([this](char c)
	{
		const std::string str {"01234567890ABCDEF"};
		if(c == keyboard::paste)
		{
			std::string cliptext {to_utf8(util::get_clipboard_text())}, pasted;
			for(auto c : cliptext)
				if(str.find(c) != -1)
				{
					if(c > 0x60)
						c -= 0x20;
					pasted += c;
				}
			if(!pasted.empty())
			{
				util::set_clipboard_text(0, to_wstring(pasted.substr(0, 6)));
				select(true);
				paste();
			}
			return false;
		}
		const auto selpoints {selection()};
		const auto selcount {selpoints.second.x - selpoints.first.x};
		return c == keyboard::backspace || c == keyboard::del || c == keyboard::undo || c == keyboard::redo || c == keyboard::cut ||
			((text().size() < 6 || selcount) && str.find(c) != -1);
	});

	events().text_changed([parent, &picker, this]
	{
		if(!noupdate)
		{
			const auto str {text()};
			if(str.size() != 6)
			{
				scheme().activated = colors::red;
				api::effects_edge_nimbus(*this, effects::edge_nimbus::active);
			}
			else
			{
				*clr = color {"#" + str};
				noupdate = true;
				picker.color_value(*clr, true);
				nana::api::refresh_window(picker);
				noupdate = false;
			}
		}
	});
}