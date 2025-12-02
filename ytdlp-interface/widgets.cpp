#include "widgets.hpp"
#include "util.hpp"
#include <codecvt>
#include <Windows.h>

using namespace widgets;

bool theme::dark {false};
double theme::shade {0};
std::string theme::gpfg;

nana::color theme::nimbus, theme::fmbg, theme::Label_fg, theme::Text_fg, theme::Text_fg_error, theme::cbox_fg, theme::btn_bg,  
	theme::btn_fg, theme::path_fg, theme::path_link_fg, theme::sep_bg, theme::tbfg, theme::tbkw, theme::tbkw_id, 
	theme::tbkw_special, theme::tbkw_warning, theme::tbkw_error, theme::gpbg, theme::lb_headerbg, theme::title_fg, theme::overlay_fg, 
	theme::border, theme::tb_selbg, theme::tb_selbg_unfocused, theme::expcol_fg, theme::tree_selbg, theme::tree_selfg, theme::tree_hilitebg, 
	theme::tree_hilitefg, theme::tree_selhilitebg, theme::tree_selhilitefg, theme::tree_parent_node, theme::tree_expander, 
	theme::tree_expander_hovered, theme::tree_bg, theme::tree_key_fg, theme::tree_val_fg, theme::list_check_highlight_fg, 
	theme::list_check_highlight_bg, theme::msg_label_fg, theme::lbbg, theme::lbselfg, theme::lbselbg, theme::lbhilite, theme::lbfg;


void widgets::theme::import(const theme_t &src)
{
	nimbus = src.nimbus;
	fmbg = src.fmbg;
	lbbg = src.lbbg;
	btn_bg = src.btn_bg;
	gpbg = src.gpbg;
	tbfg = src.tbfg;
	Label_fg = src.Label_fg;
	Text_fg = src.Text_fg;
	Text_fg_error = src.Text_fg_error;
	cbox_fg = src.cbox_fg;
	btn_fg = src.btn_fg;
	path_fg = src.path_fg;
	path_link_fg = src.path_link_fg;
	sep_bg = src.sep_bg;
	expcol_fg = src.expcol_fg;
	gpfg = src.gpfg;
	title_fg = src.title_fg;
	tbkw = src.tbkw;
	tbkw_id = src.tbkw_id;
	tbkw_special = src.tbkw_special;
	tbkw_warning = src.tbkw_warning;
	tbkw_error = src.tbkw_error;
	overlay_fg = src.overlay_fg;
	border = src.border;
	tb_selbg = src.tb_selbg;
	tb_selbg_unfocused = src.tb_selbg_unfocused;
	tree_selbg = src.tree_selbg;
	tree_selfg = src.tree_selfg;
	tree_hilitebg = src.tree_hilitebg;
	tree_hilitefg = src.tree_hilitefg;
	tree_selhilitebg = src.tree_selhilitebg;
	tree_selhilitefg = src.tree_selfg;
	tree_parent_node = src.tree_parent_node;
	tree_expander = src.tree_expander;
	tree_expander_hovered = src.tree_expander_hovered;
	tree_bg = src.tree_bg;
	tree_key_fg = src.tree_key_fg;
	tree_val_fg = src.tree_val_fg;
	list_check_highlight_fg = src.list_check_highlight_fg;
	list_check_highlight_bg = src.list_check_highlight_bg;
	msg_label_fg = src.msg_label_fg;
	lb_headerbg = src.lb_headerbg;
	lbselbg = src.lbselbg;
	lbselfg = src.lbselfg;
	lbhilite = src.lbhilite;
	lbfg = src.lbfg;
}

void theme::copy(theme_t &dest)
{
	dest.nimbus = nimbus;
	dest.fmbg = fmbg;
	dest.lbbg = lbbg;
	dest.btn_bg = btn_bg;
	dest.gpbg = gpbg;
	dest.tbfg = tbfg;
	dest.Label_fg = Label_fg;
	dest.Text_fg = Text_fg;
	dest.Text_fg_error = Text_fg_error;
	dest.cbox_fg = cbox_fg;
	dest.btn_fg = btn_fg;
	//dest.path_bg = path_bg;
	dest.path_fg = path_fg;
	dest.path_link_fg = path_link_fg;
	dest.sep_bg = sep_bg;
	dest.expcol_fg = expcol_fg;
	dest.gpfg = gpfg;
	dest.title_fg = title_fg;
	dest.tbkw = tbkw;
	dest.tbkw_id = tbkw_id;
	dest.tbkw_special = tbkw_special;
	dest.tbkw_warning = tbkw_warning;
	dest.tbkw_error = tbkw_error;
	dest.overlay_fg = overlay_fg;
	dest.border = border;
	dest.tb_selbg = tb_selbg;
	dest.tb_selbg_unfocused = tb_selbg_unfocused;
	dest.tree_selbg = tree_selbg;
	dest.tree_selfg = tree_selfg;
	dest.tree_hilitebg = tree_hilitebg;
	dest.tree_hilitefg = tree_hilitefg;
	dest.tree_selhilitebg = tree_selhilitebg;
	dest.tree_selfg = tree_selhilitefg;
	dest.tree_parent_node = tree_parent_node;
	dest.tree_expander = tree_expander;
	dest.tree_expander_hovered = tree_expander_hovered;
	dest.fmbg = tree_bg;
	dest.tree_key_fg = tree_key_fg;
	dest.tree_val_fg = tree_val_fg;
	dest.list_check_highlight_fg = list_check_highlight_fg;
	dest.list_check_highlight_bg = list_check_highlight_bg;
	dest.msg_label_fg = msg_label_fg;
	dest.lb_headerbg = lb_headerbg;
	dest.lbselbg = lbselbg;
	dest.lbselfg = lbselfg;
	dest.lbhilite = lbhilite;
	dest.lbfg = lbfg;
}


void theme::make_light(const theme_t &src)
{
	dark = false;
	import(src);
}


void theme::make_dark(const theme_t &src)
{
	dark = true;
	import(src);
}


void theme::contrast(double factor, theme_t &src)
{
	shade = std::clamp(factor, .0, .3);
	src.contrast(shade, dark);
	import(src);
}


void Label::create(nana::window parent, std::string_view text, bool dpi_adjust)
{
	label::create(parent);
	caption(text);
	fgcolor(theme::Label_fg);
	typeface(nana::paint::font_info {"Segoe UI", 12 - (double)(nana::API::screen_dpi(true) > 96) * 2 * dpi_adjust});
	text_align(nana::align::right, nana::align_v::center);
	nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
	//bgcolor(nana::colors::dark_red);
	events().expose([this] { fgcolor(theme::Label_fg); });
}


void Text::create(nana::window parent, std::string_view text, bool dpi_adjust)
{
	nana::label::create(parent, true);
	caption(text);
	fgcolor(theme::Text_fg);
	typeface(nana::paint::font_info {"Segoe UI", 12 - (double)(nana::API::screen_dpi(true) > 96) * 2 * dpi_adjust});
	text_align(nana::align::left, nana::align_v::center);
	nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
	events().expose([this]
	{
		fgcolor(error_mode_ ? theme::Text_fg_error : theme::Text_fg);
		scheme().activated = theme::nimbus;
	});
}


void Separator::create(nana::window parent, std::string title)
{
	using namespace nana;
	panel<false>::create(parent);
	plc = std::make_unique<place>(*this);
	sep1.create(*this);
	sep2.create(*this);
	paint::graphics g {{100, 100}};
	g.typeface(paint::font_info {"Verdana", 8, {700}});
	auto tes {g.text_extent_size(title)};
	auto padding {util::scale(7)};
	if(title.empty())
		plc->div("vert <sep1> <> <sep2>");
	else plc->div("<weight=" + std::to_string(tes.width + padding) + "px> <vert <<weight=1px><sep1>> <> <sep2>>");
	plc->field("sep1") << sep1;
	plc->field("sep2") << sep2;
	events().expose([this] { refresh_theme(); });
	if(!title.empty())
		drawing {parent}.draw([tes, padding, title, this](paint::graphics &g)
		{
			auto p {pos()};
			if(p.y > 0)
			{
				auto x {p.x}, y {p.y}, w {static_cast<int>(tes.width)}, h {static_cast<int>(tes.height)};
				g.palette(false, theme::sep_bg);
				auto val {(w + padding) - ((y + h / 2) - (y + 2))};
				g.line({w + padding, y + 2}, {val, y + h / 2});
				g.line({w + padding + 1, y + 2}, {val + 1, y + h / 2});
				color clr1 {theme::sep_bg}, clr2 {theme::sep_bg.blend(colors::black, .3)};
				if(!theme::is_dark())
				{
					clr1 = theme::sep_bg.blend(colors::white, .1);
					clr2 = theme::sep_bg.blend(colors::white, .5);
				}
				g.line({x, y + h / 2 + 1}, {val, y + h / 2 + 1}, clr1);
				g.line({x, y + h / 2 + 2}, {val-1, y + h / 2 + 2}, clr2);
				p.x += 2;
				p.y -= h / 2;
				g.typeface(paint::font_info {"Verdana", 8, {700}});
				g.string(p, title, theme::sep_bg.blend(theme::is_dark() ? colors::white : colors::black, .2));
			}
		});
}


void path_label::create(nana::window parent, const variant var)
{
	label::create(parent);
	v = var;
	is_path = std::holds_alternative<fs::path*>(v);
	refresh_theme();
	if(is_path) typeface(nana::paint::font_info {"Tahoma", 10});
	else typeface(nana::paint::font_info {"Segoe UI", 11, {700}});
	text_align(nana::align::center, nana::align_v::center);
	nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
	nana::API::effects_edge_nimbus(*this, nana::effects::edge_nimbus::over);
	events().expose([this] { refresh_theme(); });
	events().resized([this] { update_caption(); });

	nana::drawing {*this}.draw([this](nana::paint::graphics &g)
	{
		if(theme::is_dark())
			g.rectangle(false, theme::fmbg.blend(nana::colors::white, .3));
		else g.rectangle(false, nana::color {"#9aa"});
	});
}


void path_label::update_caption()
{
	const std::wstring wstr {is_path ? std::get<fs::path*>(v)->wstring() : *std::get<std::wstring*>(v)};
	if(!is_path && wstr.empty())
		caption("Press Ctrl+V or click here to paste and add media link");
	else if(!size().empty())
	{
		caption(wstr);
		int offset {0};
		while(measure(1234).width > size().width)
		{
			offset += 1;
			if(wstr.size() - offset < 4)
			{
				caption("");
				return;
			}
			caption(L"..." + wstr.substr(offset));
		}
	}
}


void path_label::refresh_theme()
{
	scheme().activated = theme::nimbus;
	if(is_path)
		fgcolor(caption().find("!!!") == -1 ? theme::path_fg : theme::tbkw_special);
	else fgcolor(theme::path_link_fg);
}


void cbox::create(nana::window parent, std::string_view text)
{
	checkbox::create(parent);
	caption(text);
	refresh_theme();
	typeface(nana::paint::font_info {"Segoe UI", 12});
	nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
	events().expose([this] { refresh_theme(); });
}


void cbox::refresh_theme()
{
	fgcolor(theme::cbox_fg);
	scheme().square_border_color = fgcolor();
	auto parent {nana::API::get_widget(this->parent())};
	scheme().square_bgcolor = parent->bgcolor();
}


bool Button::btn_bg::draw(nana::paint::graphics &g, const nana::color &bg,
	const nana::color &fg, const nana::rectangle &r, nana::element_state state)
{
	auto from {bg}, to {bg.blend(nana::color {"#e6e6e8"}, .15)};
	if(state != nana::element_state::pressed)
		g.gradual_rectangle(r, from, to, true);
	else g.gradual_rectangle(r, to, from, true);
	g.rectangle(r, false, static_cast<nana::color_rgb>(0x7f7f7f));
	return true;
}


void Button::create(nana::window parent, std::string_view text, bool small)
{
	button::create(parent);
	caption(text);
	refresh_theme();
	typeface(nana::paint::font_info {"Segoe UI", small ? 11e0 : 14, {800}});
	enable_focus_color(false);
	events().expose([this] { refresh_theme(); });
}


void Button::refresh_theme()
{
	fgcolor(theme::btn_fg);
	bgcolor(theme::btn_bg);
	scheme().activated = theme::nimbus;
	if(theme::is_dark())
		set_bground(btn_bg {});
	else set_bground("");
}


void Button::image(const void *data, unsigned size)
{
	img.open(data, size);
	if(enabled()) icon(img);
}


void Button::image_disabled(const void *data, unsigned size)
{
	img_disabled.open(data, size);
	if(!enabled()) icon(img_disabled);
}


void Button::enable(bool state)
{
	enabled(state);
	if(state)
	{
		if(img) icon(img);
	}
	else
	{
		if(img_disabled) icon(img_disabled);
	}
}


size_t Listbox::item_count()
{
	size_t count {0};
	for(size_t cat_idx {0}; cat_idx < size_categ(); cat_idx++)
		count += at(cat_idx).size();
	return count;
}


std::string Listbox::favicon_url_from_value(std::wstring val)
{
	if(val.find(L"bandcamp.com") != -1)
	{
		std::string res, error;
		res = util::get_inet_res(nana::to_utf8(val), &error, true);
		if(!res.empty())
		{
			auto pos1 {res.find(R"(<link rel="shortcut icon" href=")")};
			if(pos1 != -1)
			{
				pos1 += 32;
				auto pos2 {res.find('\"', pos1)};
				if(pos2 != -1)
					return res.substr(pos1, pos2 - pos1);
			}
		}
	}
	if(val.find(L"://") == -1)
		val = L"https://" + val;
	auto pos {val.rfind('.')};
	if(pos != -1)
	{
		while(++pos < val.size())
			if(!isalpha(val[pos]))
				break;
		return nana::to_utf8(val.substr(0, pos)) + "/favicon.ico";
	}
	return "";
}


nana::listbox::item_proxy Listbox::item_from_value(std::wstring val)
{
	const auto this_thread_id {std::this_thread::get_id()};
	if(this_thread_id == main_thread_id)
	{
		for(size_t cat {0}; cat < size_categ(); cat++)
			for(auto item : listbox::at(cat))
				if(item.value<lbqval_t>() == val)
					return item;
	}
	else if(!g_exiting)
	{
		auto item {empty_item};
		SendMessage(hwnd_parent, WM_LBQ_URL2ITEM, reinterpret_cast<WPARAM>(&val), reinterpret_cast<LPARAM>(&item));
		return item;
	}
	return empty_item;
}


nana::listbox::cat_proxy Listbox::at(size_type pos)
{
	const auto this_thread_id {std::this_thread::get_id()};
	if(this_thread_id != main_thread_id)
	{
		cat_proxy cat;
		SendMessage(hwnd_parent, WM_LBQ_GETCAT, pos, reinterpret_cast<LPARAM>(&cat));
		return cat;
	}
	return listbox::at(pos);
}


nana::listbox::item_proxy Listbox::at(const index_pair &abs_pos)
{
	const auto this_thread_id {std::this_thread::get_id()};
	if(this_thread_id != main_thread_id)
	{
		auto item {empty_item};
		SendMessage(hwnd_parent, WM_LBQ_IDX2ITEM, reinterpret_cast<WPARAM>(&abs_pos), reinterpret_cast<LPARAM>(&item));
	}
	return listbox::at(abs_pos);
}


void widgets::Listbox::set_line_text(std::wstring url, qline_t text, bool post)
{
	if(std::this_thread::get_id() == main_thread_id)
	{
		auto item {item_from_value(url)};
		if(item != empty_item)
		{
			for(int n{0}; n<7; n++)
				if(!text[n].empty())
					item.text(n+1, text[n]);
		}
	}
	else if(!force_no_thread_safe_ops_ && no_thread_safe_ops && !*no_thread_safe_ops)
	{
		auto purl {new std::wstring {url}};
		auto ptext {new qline_t {text}};
		if(post) PostMessage(hwnd_parent, WM_SET_QLINE_TEXT, reinterpret_cast<WPARAM>(purl), reinterpret_cast<LPARAM>(ptext));
		else SendMessage(hwnd_parent, WM_SET_QLINE_TEXT, reinterpret_cast<WPARAM>(purl), reinterpret_cast<LPARAM>(ptext));
	}
}


void widgets::Listbox::refresh_window()
{
	if(std::this_thread::get_id() == main_thread_id)
		nana::api::refresh_window(*this);
	else SendMessage(hwnd_parent, WM_REFRESH, reinterpret_cast<WPARAM>(handle()), 0);
}


Listbox::item_proxy widgets::Listbox::erase(item_proxy ip)
{
	if(std::this_thread::get_id() == main_thread_id)
	{
		return listbox::erase(ip);
	}
	else
	{
		item_proxy res {empty_item};
		SendMessage(hwnd_parent, WM_LBQ_ERASE, reinterpret_cast<WPARAM>(&ip), reinterpret_cast<LPARAM>(&res));
		return res;
	}
}


void widgets::Listbox::set_item_bg(std::wstring url, nana::color bg)
{
	if(std::this_thread::get_id() == main_thread_id)
	{
		auto item {item_from_value(url)};
		if(item != empty_item)
			item.bgcolor(bg);
	}
	else if(!force_no_thread_safe_ops_ && no_thread_safe_ops && !*no_thread_safe_ops)
		SendMessage(hwnd_parent, WM_LBQ_ITEMBG, reinterpret_cast<WPARAM>(&url), reinterpret_cast<LPARAM>(&bg));
}


void widgets::Listbox::auto_draw(bool enable) noexcept
{
	if(std::this_thread::get_id() == main_thread_id)
		return listbox::auto_draw(enable);
	else if(!force_no_thread_safe_ops_ && no_thread_safe_ops && !*no_thread_safe_ops)
		SendMessage(hwnd_parent, WM_LBQ_AUTODRAW, enable, 0);
}


void Listbox::refresh_theme()
{
	using namespace nana;
	bgcolor(theme::lbbg);
	fgcolor(theme::lbfg);
	dw.draw([](paint::graphics &g) { g.rectangle(false, theme::border); });

	if(theme::is_dark())
	{
		borderless(true);
		scheme().header_bgcolor = theme::lb_headerbg;
		scheme().header_fgcolor = colors::white;
		scheme().cat_fgcolor = theme::nimbus;
		scheme().item_selected = theme::lbselbg;
		scheme().item_selected_border = theme::lbselfg.blend(colors::black, .15);
		if(hicontrast)
			scheme().item_highlighted = theme::lbhilite.blend(colors::light_grey, .15 - theme::contrast() / 2);
		else scheme().item_highlighted = theme::lbhilite;
	}
	else
	{
		borderless(false);
		scheme().header_bgcolor = theme::lb_headerbg;
		scheme().header_fgcolor = colors::black;
		scheme().cat_fgcolor = color {"#039"};
		auto c {theme::contrast()};
		scheme().item_selected = theme::lbselbg.blend(colors::grey, .1 - c);
		scheme().item_selected_border = theme::lbselfg.blend(colors::grey, .1 - c);
		scheme().item_highlighted = theme::lbhilite.blend(colors::dark_grey, .25 - c / 2);
		dw.clear();
	}
	auto_draw(false);
	for(size_t cat {0}; cat < size_categ(); cat++)
	{
		for(auto el : listbox::at(cat))
		{
			el.bgcolor(theme::lbbg);
			if(el.checked() && hilite_checked)
			{
				el.fgcolor(theme::list_check_highlight_fg);
			}
			else el.fgcolor(theme::lbfg);
		}
	}
	auto_draw(true);
}

unsigned widgets::Listbox::fit_column_content()
{
	const auto number_of_columns {column_size()};
	nana::form fm;
	nana::label l {fm};
	int total_width {0};
	const auto padding {util::scale(20)};
	l.typeface(typeface());
	for(size_t n {0}; n < number_of_columns; n++)
	{
		auto &col {column_at(n)};
		if(col.visible())
		{
			col.fit_content();
			l.caption(col.text());
			const auto minw {l.measure(0).width};
			if(col.width() < minw + padding)
				col.width(minw + padding);
			total_width += col.width();
		}
	}
	return total_width;
}


void Progress::create(nana::window parent)
{
	nana::progress_ex::create(parent, true);
	refresh_theme();
	typeface(nana::paint::font_info {"Segoe UI", 11, {800}});
	nana::paint::image img;
	img.open(arr_progbar_jpg, sizeof arr_progbar_jpg);
	image(img);
	events().expose([this] { refresh_theme(); });
}


void Progress::refresh_theme()
{
	if(theme::is_dark())
	{
		dark_bg(true);
		outline_color(nana::color {"#2a3b5c"});
		text_contrast_colors(theme::Label_fg, theme::Label_fg);
		scheme().background = theme::fmbg.blend(nana::colors::black, .1);
		scheme().lower_background = theme::fmbg.blend(nana::colors::white, .1);
	}
	else
	{
		dark_bg(false);
		outline_color(nana::color {"#678"});
		text_contrast_colors(nana::colors::white, nana::color {"#678"});
		scheme().background = nana::color {nana::colors::white}.blend(nana::colors::light_grey, .3 - theme::contrast());
		scheme().lower_background = nana::color {"#f5f5f5"}.blend(nana::colors::light_grey, .3 - theme::contrast());
	}
}


void Group::create(nana::window parent, std::string title)
{
	group::create(parent);
	this->title = title;
	caption(title);
	//enable_format_caption(true);
	caption_align(nana::align::center);
	refresh_theme();
	events().expose([this] { refresh_theme(); });
}


nana::widget &Group::caption(std::string utf8)
{
	title = utf8;
	widget::typeface(nana::paint::font_info{}); // workaround for bug: group.hpp declares undefined method `typeface`
	return group::caption("<bold color=" + theme::gpfg + " size=11 font=\"Tahoma\"> " + utf8 + " </>");
}


std::string Group::caption()
{
	auto cap {group::caption()};
	auto pos1 {cap.find('>') + 2}, pos2 {cap.rfind('<') - 2};
	return cap.substr(pos1, pos2 - pos1 + 1);
}


void Group::refresh_theme()
{
	bgcolor(theme::gpbg);
	caption(title);
}


void Combox::my_renderer::item(widget_reference wdg, graph_reference g, const nana::rectangle &r, const item_interface *ii, state_t state)
{
	using namespace nana;
	const unsigned margin {8};
	const auto &img {ii->image()};
	const color fg_normal {theme::lbfg},
	            bg_normal {theme::is_dark() ? theme::fmbg.blend(colors::white, .08) : theme::fmbg},
	            bg_hilighted {theme::lbselbg},
	            bg_icon_area {theme::is_dark() ? theme::fmbg.blend(colors::white, .18) : theme::fmbg.blend(colors::black, .03)};

	g.rectangle(r, true, bg_normal);
	if(state == StateHighlighted)
	{
		const auto clr_blend {theme::is_dark() ? colors::black : colors::white};
		g.rectangle(r, false, theme::lbselfg);
		g.rectangle(rectangle {r}.pare_off(1), false, bg_hilighted.blend(clr_blend, .4));
		g.palette(false, bg_normal);
		paint::draw {g}.corner(r, 1);
		g.gradual_rectangle(rectangle {r}.pare_off(2), bg_hilighted, bg_hilighted.blend(clr_blend, .25), true);
	}
	auto size {g.text_extent_size(ii->text())};
	auto pos {r.position()};
	pos.x += margin;
	if(!img.empty())
	{
		int yoff {static_cast<int>(r.height / 2) - 8};
		img.stretch(rectangle {img.size()}, g, {pos.x, pos.y + yoff, 16, 16});
		pos.x += 16 + margin;
	}
	pos.y += static_cast<int>(r.height / 2 - size.height / 2);
	g.string(pos, ii->text(), fg_normal);
}


unsigned Combox::my_renderer::item_pixels(graph_reference g) const
{
	const unsigned padding {6}; // empty space padding the top and bottom of the text area
	unsigned ascent, descent, internal_leading; // font metrics
	g.text_metrics(ascent, descent, internal_leading);
	return ascent + descent + padding;
}


void Combox::create(nana::window parent)
{
	combox::create(parent);
	typeface(nana::paint::font_info {"Segoe UI", 9});
	renderer(&renderer_);
	refresh_theme();
	events().expose([this] { refresh_theme(); });
	nana::drawing {*this}.draw([this](nana::paint::graphics &g)
	{
		if(theme::is_dark())
			g.rectangle(false, nana::color {"#999A9E"});
	});
}


void Combox::refresh_theme()
{
	bgcolor(theme::is_dark() ? theme::fmbg.blend(nana::colors::white, .08) : theme::fmbg);
	fgcolor(theme::lbfg);
	scheme().activated = theme::nimbus;
	scheme().selection = theme::tb_selbg;
	scheme().selection_unfocused = theme::tb_selbg_unfocused;
}

int Combox::caption_index()
{
	const auto cap {caption()};
	size_t size {the_number_of_options()}, idx {0};
	for(; idx < size; idx++)
		if(cap == text(idx))
			break;
	if(idx == size) return -1;
	return idx;
}


void Textbox::create(nana::window parent, bool visible)
{
	main_thread_id = std::this_thread::get_id();
	hwnd_parent = reinterpret_cast<HWND>(reinterpret_cast<nana::form*>(nana::api::get_widget(parent))->native_handle());
	textbox::create(parent, visible);
	refresh_theme();
	events().expose([this] { refresh_theme(); });
	nana::drawing {*this}.draw([](nana::paint::graphics &g)
	{
		g.rectangle(false, nana::color {"#999A9E"});
	});
}


void Textbox::refresh_theme()
{
	scheme().activated = theme::nimbus;
	auto bgparent {nana::API::get_widget(parent())->bgcolor()};
	bgcolor(theme::is_dark() ? bgparent.blend(nana::colors::black, .050) : bgparent);
	fgcolor(theme::tbfg);
	scheme().selection = theme::tb_selbg;
	scheme().selection_unfocused = theme::tb_selbg_unfocused;
	highlight(highlighted);
	set_keywords("special", true, true, {"[download]"});
	set_keywords("warning", true, true, {"WARNING:"});
	set_keywords("error", true, true, {"ERROR:"});
}


void Textbox::highlight(bool enable)
{
	highlighted = enable;
	if(enable)
	{
		set_highlight("id", theme::tbkw_id, theme::lbbg);
		set_highlight("general", theme::tbkw, theme::lbbg);
		set_highlight("special", theme::tbkw_special, theme::lbbg);
		set_highlight("warning", theme::tbkw_warning, theme::lbbg);
		set_highlight("error", theme::tbkw_error, theme::lbbg);
	}
	else
	{
		erase_highlight("id");
		erase_highlight("general");
		erase_highlight("special");
		erase_highlight("warning");
		erase_highlight("error");
	}
}


Slider::Slider(nana::window parent) : slider {parent}
{
	transparent(true);
	refresh_theme();
	events().expose([this] {refresh_theme(); });
	vernier([&](unsigned max, unsigned val)
	{
		return std::to_string(val);
	});
}


void Slider::refresh_theme()
{
	if(theme::is_dark())
	{
		scheme().color_adorn = scheme().color_slider = nana::color {"#a8967a"};
		scheme().color_slider_highlighted = nana::colors::dark_goldenrod;
		scheme().color_vernier_text = nana::colors::white;
		scheme().color_vernier = nana::colors::brown;
	}
	else
	{
		scheme().color_adorn = nana::color {"#3da3ce"};
		scheme().color_slider = nana::color {"#287DA2"};
		scheme().color_slider_highlighted = nana::color {"#287DA2"};
		scheme().color_vernier_text = nana::colors::white;
		scheme().color_vernier = nana::colors::dark_green;
	}
}


void Overlay::create(nana::window parent, nana::widget *outbox, std::string_view text, bool visible)
{
	if(text.empty())
		caption("output from yt-dlp.exe appears here\n\nright-click for options\n\ndouble-click to show queue");
	label::create(parent, visible);
	fgcolor(theme::overlay_fg);
	text_align(nana::align::center, nana::align_v::center);
	typeface(nana::paint::font_info {"Segoe UI", 15, {700}});
	nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
	events().expose([this] { fgcolor(theme::overlay_fg); });
	nana::drawing {*this}.draw([](nana::paint::graphics &g) { g.rectangle(false, theme::border); });
	events().mouse_up([outbox, this](const nana::arg_mouse &arg)
	{
		outbox->events().mouse_up.emit(arg, *this);
	});
}


void Menu::menu_renderer::background(graph_reference graph, nana::window wd)
{
	using namespace nana;
	const nana::color
		bg_normal {theme::is_dark() ? theme::fmbg.blend(colors::white, .08) : theme::fmbg},
		clr_border {theme::is_dark() ? "#A3A2A1" : "#666"},
		bg_icon_area {theme::is_dark() ? theme::fmbg.blend(colors::white, .18) : theme::fmbg.blend(colors::black, .03)};
	graph.rectangle(true, bg_normal); // entire area
	graph.rectangle({1,1,unsigned(util::scale(28)),graph.height() - 2}, true, bg_icon_area); // icon area
	graph.rectangle(false, clr_border); // border
}


void Menu::menu_renderer::item(graph_reference g, const nana::rectangle &r, const attr &attr)
{
	if(!attr.enabled) return;
	using namespace nana;
	using util::scale;
	const unsigned margin {8};
	const color
		fg_normal    {theme::lbfg},
		bg_normal    {theme::is_dark() ? theme::fmbg.blend(colors::white, .08) : theme::fmbg},
		bg_hilighted {theme::lbselbg},
		bg_icon_area {theme::is_dark() ? theme::fmbg.blend(colors::white, .18) : theme::fmbg.blend(colors::black, .03)};

	g.rectangle(r, true, bg_normal);
	if(attr.item_state == state::active)
	{
		g.rectangle(r, false, theme::lbselfg);
		g.rectangle(rectangle {r}.pare_off(1), false, bg_hilighted.blend(theme::is_dark() ? colors::black : colors::white, .4));
		g.palette(false, bg_normal);
		paint::draw {g}.corner(r, 1);
		if(theme::is_dark())
			g.gradual_rectangle(rectangle {r}.pare_off(2), bg_hilighted, bg_hilighted.blend(colors::black, .3), true);
		else g.gradual_rectangle(rectangle {r}.pare_off(2), bg_hilighted, bg_hilighted.blend(colors::white, .5), true);
		g.set_pixel(r.x, r.y, bg_icon_area);
		g.set_pixel(r.x, r.y + r.height - 1, bg_icon_area);
	}
	else g.rectangle({1,r.y,unsigned(scale(28)),r.height}, true, bg_icon_area); // icon area

	if(attr.checked)
	{
		int sx {(scale(28) - scale(8)) / 2},
			sy {r.y + (static_cast<int>(r.height) - scale(4)) / 2};

		g.palette(false, fg_normal);

		for(int i {0}; i < scale(3); i++)
		{
			sx++;
			sy++;
			g.line({sx, sy}, {sx, sy + scale(2)});
		}

		for(int i {0}; i < scale(5); i++)
		{
			sx++;
			sy--;
			g.line({sx, sy}, {sx, sy + scale(2)});
		}
	}
}


void Menu::menu_renderer::item_text(graph_reference g, const nana::point &pos, const std::string &text, unsigned pixels, const attr &attr)
{
	auto size {g.text_extent_size(text)};
	g.typeface(nana::paint::font_info {"Segoe UI", 9});
	if(theme::is_dark())
		g.string(pos, text, attr.enabled ? nana::colors::white : nana::color {"#aaa"});
	else g.string(pos, text, attr.enabled ? nana::colors::black : nana::colors::gray_border);
}


void Menu::menu_renderer::sub_arrow(graph_reference graph, const nana::point &pos, unsigned pixels, const attr &atr)
{
	using namespace nana;
	using util::scale;
	int w {scale(5)}, h {scale(9)};
	if(h%2 == 0) h++;
	int l {(h - 1) / 2}, x {pos.x}, y {pos.y + (static_cast<int>(pixels) / 2 - scale(9) / 2)};
	graph.palette(false, theme::is_dark() ? colors::light_grey : colors::black);
	graph.line({x, y}, {x, y + h-1});
	graph.line({x+1, y+1}, {x+l, y+l});
	graph.line({x+1, y+h-2}, {x+l, y+l});
}


void Spinbox::create(nana::window parent)
{
	spinbox::create(parent);
	editable(false);
	events().expose([this] { refresh_theme(); });
}


void Spinbox::refresh_theme()
{
	bgcolor(theme::lbbg);
	fgcolor(theme::tbfg);
	scheme().activated = theme::nimbus;
}


void Expcol::create(nana::window parent)
{
	using namespace nana;
	label::create(parent);
	refresh_theme();
	events().expose([this] { refresh_theme(); });
	events().mouse_enter([this] { hovered = true; api::refresh_window(*this); });
	events().mouse_leave([this] { hovered = false; api::refresh_window(*this); });
	events().click([this] { operate(!downward); });

	drawing {*this}.draw([&](paint::graphics &g)
	{
		auto draw_v = [&](const point pos, const int w)
		{
			const int x {pos.x}, y {pos.y};
			if(downward)
			{
				g.line({x, y}, {x + w / 2, y + w / 2});
				g.line({x + w / 2, y + w / 2}, {x + w, y});
			}
			else
			{
				g.line({x + w / 2, y}, {x, y + w / 2});
				g.line({x + w / 2, y}, {x + w, y + w / 2});
			}
		};
		int pad {util::scale(4)};

		const auto w {g.width() - pad * 2};
		auto x {pad}, y {pad + (int)(g.height() / 2 - w / 1.5)};

		if(hovered)
		{
			g.palette(false, theme::nimbus);
			draw_v({x - 1, y - 1}, w);
			draw_v({x, y - 2}, w);
			draw_v({x + 1, y - 1}, w);
			draw_v({x + 1, y}, w);
			draw_v({x, y}, w);
			draw_v({x - 1, y}, w);
			draw_v({x, y + 1}, w);
			g.blur({0, 0, g.width(), g.height()}, 1);
			g.palette(false, fgcolor());
			draw_v({x, y + 1}, w);
		}
		else
		{
			g.palette(false, fgcolor());
			draw_v({x, y}, w);
		}
	});
}


void Expcol::refresh_theme()
{
	bgcolor(theme::fmbg);
	fgcolor(theme::expcol_fg);
}


void Expcol::operate(bool collapse)
{
	downward = collapse;
	nana::api::refresh_window(*this);
}


void JSON_Tree::jtree_renderer::expander(graph_reference graph, const compset_interface *compset) const
{
	using namespace nana;
	comp_attribute_t attr;
	if(compset->comp_attribute(component::expander, attr))
	{
		facade<element::arrow> arrow("solid_triangle");
		arrow.direction(direction::southeast);
		if(!compset->item_attribute().expended)
		{
			arrow.switch_to("hollow_triangle");
			arrow.direction(direction::east);
		}
		auto r = attr.area;
		r.y += (attr.area.height - 16) / 2;
		r.width = r.height = 16;
		auto clr {attr.mouse_pointed ? theme::tree_expander_hovered : theme::tree_expander};
		arrow.draw(graph, api::bgcolor(htree_), clr, r, element_state::normal);
	}
}


void JSON_Tree::jtree_renderer::text(graph_reference graph, const compset_interface *compset) const
{
	auto iattr {compset->item_attribute()};
	comp_attribute_t cattr;
	compset->comp_attribute(component::text, cattr);
	auto tf {graph.typeface()};
	if(iattr.has_children)
		graph.typeface({tf.name(), tf.size(), {800}});

	auto tsize {graph.text_extent_size(iattr.text)};
	auto gsize {cattr.area.dimension()};
	nana::point tpos {cattr.area.position()};
	if(tsize.height < gsize.height)
		tpos.y += gsize.height / 2 - tsize.height / 2;

	if(iattr.has_children)
	{
		graph.string(tpos, iattr.text, theme::tree_parent_node);
		graph.typeface(tf);
	}
	else
	{
		auto pos {iattr.text.find(':')};
		if(pos != -1)
		{
			std::string strkey {iattr.text.substr(0, pos + 3)}, strval {iattr.text.substr(pos + 3)};
			graph.string(tpos, strkey, theme::tree_key_fg);
			tpos.x += graph.text_extent_size(strkey).width;
			graph.string(tpos, strval, theme::tree_val_fg);
		}
		else graph.string(tpos, iattr.text, theme::tree_parent_node);
	}
}


unsigned JSON_Tree::jtree_placer::item_width(graph_reference graph, const item_attribute_t &attr) const
{
	if(attr.has_children) // if the item has children, calc width using bold font
	{
		auto tf {graph.typeface()};
		graph.typeface({tf.name(), tf.size(), {800}}); // bold font
		auto result {placer_->item_width(graph, attr)};
		graph.typeface(tf); // restore original font
		return result;
	}
	return placer_->item_width(graph, attr);
}


void JSON_Tree::create(nana::window parent, const json &data, bool hide_null)
{
	unsigned icon_size {util::scale_uint(16)};

	jptr = &data;
	no_nulls = hide_null;
	treebox::create(parent);
	use_entire_line(true);
	scheme().text_offset = 5;
	scheme().icon_size = icon_size;
	scheme().indent_displacement = 33;
	typeface(nana::paint::font_info {"Segoe UI", 12});
	placer(jtree_placer {placer()});
	renderer(jtree_renderer {renderer()});
	icon("json_light").normal.open(arr_json_light_ico, sizeof arr_json_light_ico);
	icon("array_light").normal.open(arr_array_light_ico, sizeof arr_array_light_ico);
	icon("object_light").normal.open(arr_object_light_ico, sizeof arr_object_light_ico);
	icon("text_light").normal.open(arr_text_light_ico, sizeof arr_text_light_ico);
	icon("number_light").normal.open(arr_number_light_ico, sizeof arr_number_light_ico);
	icon("bool_light").normal.open(arr_bool_light_ico, sizeof arr_bool_light_ico);
	icon("null_light").normal.open(arr_null_light_ico, sizeof arr_null_light_ico);
	icon("json_dark").normal.open(arr_json_dark_ico, sizeof arr_json_dark_ico);
	icon("array_dark").normal.open(arr_array_dark_ico, sizeof arr_array_dark_ico);
	icon("object_dark").normal.open(arr_object_dark_ico, sizeof arr_object_dark_ico);
	icon("text_dark").normal.open(arr_text_dark_ico, sizeof arr_text_dark_ico);
	icon("number_dark").normal.open(arr_number_dark_ico, sizeof arr_number_dark_ico);
	icon("bool_dark").normal.open(arr_bool_dark_ico, sizeof arr_bool_dark_ico);
	icon("null_dark").normal.open(arr_null_dark_ico, sizeof arr_null_dark_ico);
	events().expose([this] { refresh_theme(); });
	populate();
}


void JSON_Tree::refresh_theme()
{
	bgcolor(theme::tree_bg);
	fgcolor(theme::tbfg);
	scheme().item_bg_selected = theme::tree_selbg;
	scheme().item_fg_selected = theme::tree_selfg;
	scheme().item_bg_highlighted = theme::tree_hilitebg;
	scheme().item_fg_highlighted = theme::tree_hilitefg;
	scheme().item_bg_selected_and_highlighted = theme::tree_selhilitebg;
	scheme().item_fg_selected_and_highlighted = theme::tree_selhilitefg;

	auto root {find("root")};
	if(!root.empty())
	{
		std::string icon_object {"object_light"}, icon_array {"array_light"}, icon_text {"text_light"}, icon_number {"number_light"},
			icon_bool {"bool_light"}, icon_null {"null_light"};
		if(theme::is_dark())
		{
			icon_object = "object_dark";
			icon_array = "array_dark";
			icon_text = "text_dark";
			icon_number = "number_dark";
			icon_bool = "bool_dark";
			icon_null = "null_dark";
		}

		std::function<void(item_proxy)> recfn = [&](item_proxy parent)
		{
			for(auto node : parent)
			{
				if(node.size())
				{
					if(node.icon().find("array") != -1)
						node.icon(icon_array);
					else if(node.icon().find("object") != -1)
						node.icon(icon_object);
					recfn(node);
				}
				else if(node.icon().find("text") != -1)
					node.icon(icon_text);
				else if(node.icon().find("number") != -1)
					node.icon(icon_number);
				else if(node.icon().find("bool") != -1)
					node.icon(icon_bool);
				else if(node.icon().find("null") != -1)
					node.icon(icon_null);
			}
		};

		if(theme::is_dark())
		{
			if(root.icon().find("dark") == -1)
			{
				root.icon("json_dark");
				auto_draw(false);
				recfn(root);
				auto_draw(true);
			}
		}
		else if(root.icon().find("light") == -1)
		{
			root.icon("json_light");
			auto_draw(false);
			recfn(root);
			auto_draw(true);
		}
	}
}


void JSON_Tree::populate()
{
	if(jptr == nullptr) return;
	clear();
	const auto dark {theme::is_dark()};
	auto root {insert("root", "JSON").expand(true).icon(dark ? "json_dark" : "json_light")};

	std::function<void(item_proxy, json)> recpop = [&](item_proxy parent, const json &j)
	{
		for(const auto &el : j.items())
		{
			auto key {el.key()};
			auto val {el.value()};

			if(val.is_object())
			{
				auto node {parent.append(key, key)};
				node.icon(dark ? "object_dark" : "object_light");
				recpop(node, el.value());
			}
			else if(val.is_array())
			{
				auto node {parent.append(key, key + " (" + std::to_string(val.size()) + ")")};
				node.icon(dark ? "array_dark" : "array_light");
				recpop(node, el.value());
			}
			else if(val.is_string())
			{
				auto str {val.get<std::string>()};
				if(!nana::is_utf8(str))
				{
					std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> u16conv;
					auto u16str {u16conv.from_bytes(str)};
					std::wstring wstr(u16str.size(), L'\0');
					memcpy(&wstr.front(), &u16str.front(), wstr.size() * 2);
					std::wstring_convert<std::codecvt_utf8<wchar_t>> u8conv;
					str = u8conv.to_bytes(wstr);
				}
				auto node {parent.append(key, key + ":  \"" + str + '\"')};
				node.icon(dark ? "text_dark" : "text_light");
			}
			else if(val.is_boolean())
			{
				auto node {parent.append(key, key + ":  " + (val.get<bool>() ? "true" : "false"))};
				node.icon(dark ? "bool_dark" : "bool_light");
			}
			else if(val.is_number_unsigned())
			{
				auto node {parent.append(key, key + ":  " + std::to_string(val.get<std::uint64_t>()))};
				node.icon(dark ? "number_dark" : "number_light");
			}
			else if(val.is_number_integer())
			{
				auto node {parent.append(key, key + ":  " + std::to_string(val.get<std::int64_t>()))};
				node.icon(dark ? "number_dark" : "number_light");
			}
			else if(val.is_number_float())
			{
				auto node {parent.append(key, key + ":  " + std::to_string(val.get<float>()))};
				node.icon(dark ? "number_dark" : "number_light");
			}
			else if(val.is_null())
			{
				auto node {parent.append(key, key + ":  null")};
				node.icon(dark ? "null_dark" : "null_light");
				if(no_nulls)
					node.hide(true);
			}
		}
	};

	auto_draw(false);
	recpop(root, *jptr);
	auto_draw(true);
}


void conf_tree::ctree_renderer::begin_paint(nana::widget &wdg)
{
	htree_ = wdg.handle();
	renderer_->begin_paint(wdg);
}


void conf_tree::ctree_renderer::text(graph_reference graph, const compset_interface *compset) const
{
	auto iattr {compset->item_attribute()};
	comp_attribute_t cattr;
	compset->comp_attribute(component::text, cattr);
	auto tf {graph.typeface()};
	if(iattr.has_children)
		graph.typeface({tf.name(), tf.size(), {800}});

	auto tsize {graph.text_extent_size(iattr.text)};
	auto gsize {cattr.area.dimension()};
	nana::point tpos {cattr.area.position()};
	if(tsize.height < gsize.height)
		tpos.y += gsize.height / 2 - tsize.height / 2 - 1;

	if(iattr.has_children)
	{
		graph.string(tpos, iattr.text, theme::tree_parent_node);
		graph.typeface(tf);
	}
	else graph.string(tpos, iattr.text, theme::Label_fg);
}


unsigned conf_tree::ctree_placer::item_width(graph_reference graph, const item_attribute_t &attr) const
{
	if(attr.has_children) // if the item has children, calc width using bold font
	{
		auto tf {graph.typeface()};
		graph.typeface({tf.name(), tf.size(), {800}}); // bold font
		auto result {placer_->item_width(graph, attr)};
		graph.typeface(tf); // restore original font
		return result;
	}
	return placer_->item_width(graph, attr);
}


void conf_tree::create(nana::window parent, nana::place *place, page_callback callback)
{
	plc = place;
	treebox::create(parent);
	use_entire_line(true);
	scheme().text_offset = util::scale(5);
	typeface(nana::paint::font_info {"Segoe UI", 12});
	placer(ctree_placer {placer()});
	renderer(ctree_renderer {renderer()});
	events().expose([this] { refresh_theme(); });
	events().selected([callback, this](const nana::arg_treebox &arg)
	{
		if(arg.item.selected())
		{
			auto field_name {arg.item.value<std::string>().data()};
			plc->field_display(field_name, true);
			plc->collocate();
			if(callback) callback(field_name);
		}
	});
}


void conf_tree::refresh_theme()
{
	bgcolor(theme::fmbg.blend(theme::is_dark() ? nana::colors::white : nana::colors::black, .06));
	if(theme::is_dark())
	{
		scheme().item_bg_selected = theme::tree_selbg;
		scheme().item_fg_selected = theme::tree_selfg;
		scheme().item_bg_highlighted = theme::tree_hilitebg;
		scheme().item_fg_highlighted = theme::tree_hilitefg;
		scheme().item_bg_selected_and_highlighted = theme::tree_selhilitebg;
		scheme().item_fg_selected_and_highlighted = theme::tree_selhilitefg;
	}
	else
	{
		scheme().item_bg_selected = theme::tree_selbg.blend(nana::colors::black, .06);
		scheme().item_fg_selected = theme::tree_selfg.blend(nana::colors::black, .06);
		scheme().item_bg_highlighted = theme::tree_hilitebg.blend(nana::colors::black, .06);
		scheme().item_fg_highlighted = theme::tree_hilitefg.blend(nana::colors::black, .06);
		scheme().item_bg_selected_and_highlighted = theme::tree_selhilitebg.blend(nana::colors::black, .06);
		scheme().item_fg_selected_and_highlighted = theme::tree_selhilitefg.blend(nana::colors::black, .06);
	}
}


void conf_tree::add(std::string item_text, std::string field_name)
{
	if(plc)
	{
		insert(field_name, item_text).value(field_name);
	}
}


void conf_tree::select(std::string field_name)
{
	auto item {find(field_name)};
	if(!item.empty())
	{
		item.select(true);
	}
}


void conf_page::create(nana::window parent)
{
	nana::panel<true>::create(parent);
	plc = std::make_unique<nana::place>(*this);
	refresh_theme();
	events().expose([this] { refresh_theme(); });
	events().resized([this] { plc->collocate(); });
}


void widgets::conf_page::refresh_theme()
{
	bgcolor(theme::fmbg);
	nana::API::enum_widgets(*this, true, [](widget &wdg)
	{
		wdg.events().expose.emit({}, wdg);
		nana::API::refresh_window(wdg);
	});
}


sblock_listbox::sblock_listbox(nana::window parent, bool high_contrast) : listbox {parent}, hicontrast {hicontrast}
{
	refresh_theme();
	events().expose([this] { refresh_theme(); });
	checkable(true);
	enable_single(true, false);
	typeface(nana::paint::font_info {"Calibri", 12});
	scheme().item_height_ex = util::scale(15);
	scheme().text_margin = util::scale(10) + (nana::api::screen_dpi(true) > 96) * 4;
	append_header("", util::scale(286));
	show_header(false);

	events().selected([&](const nana::arg_listbox &arg)
	{
		if(arg.item.selected())
			arg.item.check(!arg.item.checked()).select(false);
	});

	events().checked([&](const nana::arg_listbox &arg)
	{
		static bool ignore {false};
		if(!ignore && arg.item.checked())
		{
			ignore = true;
			auto_draw(false);
			auto cat {at(0)};
			auto catsize {cat.size()};
			if(arg.item.pos().item == 0)
			{
				for(auto ip : cat)
					ip.check(false);
				cat.at(0).check(true);
			}
			else
			{
				cat.at(0).check(false);
				if(checked().size() == catsize - 1)
				{
					for(int n {1}; n < catsize; n++)
						cat.at(n).check(false);
					cat.at(0).check(true);
				}
			}
			auto_draw(true);
			ignore = false;
		}
	});
}


void sblock_listbox::refresh_theme()
{
	using namespace nana;
	auto bgparent {nana::API::get_widget(parent())->bgcolor()};
	fgcolor(theme::is_dark() ? nana::color {"#e5e5e5"} : nana::color {"#404040"});
	bgcolor(theme::is_dark() ? bgparent.blend(nana::colors::white, .020) : bgparent.blend(nana::colors::black, .020));

	if(theme::is_dark())
	{
		borderless(true);
		scheme().header_bgcolor = theme::lb_headerbg;
		scheme().header_fgcolor = colors::white;
		scheme().cat_fgcolor = theme::nimbus;
		scheme().item_selected = color {"#AC4F44"};
		scheme().item_selected_border = color {"#B05348"}.blend(colors::black, .15);
		if(hicontrast)
			scheme().item_highlighted = theme::lbhilite.blend(colors::light_grey, .15 - theme::contrast() / 2);
		else scheme().item_highlighted = theme::lbhilite;
	}
	else
	{
		borderless(false);
		scheme().header_bgcolor = color {"#f1f2f4"};
		scheme().header_fgcolor = colors::black;
		scheme().cat_fgcolor = color {"#039"};
		auto c {theme::contrast()};
		scheme().item_selected = color {"#c7dEe4"}.blend(colors::grey, .1 - c);
		scheme().item_selected_border = color {"#a7cEd4"}.blend(colors::grey, .1 - c);
		scheme().item_highlighted = bgcolor().blend(nana::colors::black, .07);
	}
	if(borderless())
		nana::drawing {*this}.draw([](paint::graphics &g) { g.rectangle(false, theme::border); });
	if(hilite_checked)
	{
		auto chk {checked()};
		for(const auto &el : chk)
		{
			at(el).fgcolor(theme::list_check_highlight_fg);
			at(el).bgcolor(theme::list_check_highlight_bg);
		}
	}
}


void Infobox::create(nana::window parent, bool visible)
{
	label::create(parent, visible);
	text_align(nana::align::center, nana::align_v::center);
	typeface(nana::paint::font_info {"Arial", 10, {400, true}});
	refresh_theme();
	events().expose([this] { refresh_theme(); });
	nana::drawing {*this}.draw([](nana::paint::graphics &g) { g.rectangle(false, theme::border); });
}


void Infobox::refresh_theme()
{
	auto bgparent {nana::API::get_widget(parent())->bgcolor()};
	bgcolor(theme::is_dark() ? bgparent.blend(nana::colors::white, .020) : bgparent.blend(nana::colors::black, .020));
	fgcolor(theme::is_dark() ? nana::color {"#bbb"} : nana::color {"#666"});
}


void inline_widget::create(nana::window wd)
{
	using widgets::theme;

	pic.create(wd);
	pic.align(nana::align::center, nana::align_v::center);
	pic.transparent(true);
	pic.stretchable(true);
	pic.events().click([this] {indicator_->selected(pos_); });
	pic.events().mouse_move([this] {indicator_->hovered(pos_); });

	text.create(wd);
	text.transparent(true);
	text.text_align(nana::align::left, nana::align_v::center);
	text.typeface(nana::paint::font_info {"Calibri", 12});
	text.events().click([this] {indicator_->selected(pos_); });
	text.events().mouse_move([this] {indicator_->hovered(pos_); });
}


void inline_widget::activate(inline_indicator &ind, index_type pos)
{
	indicator_ = &ind;
	pos_ = pos;
	if(!lb) lb = dynamic_cast<widgets::Listbox*>(&indicator_->host());
	text.fgcolor(lb->fgcolor());
}


void inline_widget::resize(const nana::size &sz)
{
	const unsigned picsz {std::min(util::scale_uint(18), sz.height)};
	int pic_centered {int(sz.width - picsz) / 2};
	if(conf)
	{
		pic.move({conf > 1 ? 1 : int(sz.width - picsz) / 2, (sz.height > picsz ? int(sz.height - picsz) / 2 + 1 : int(picsz) + 1), picsz, picsz});
		if(conf > 1) text.move({conf == 2 ? 4 : int(picsz) + 6, 0, conf == 2 ? sz.width - 4 : sz.width - (picsz + 5), sz.height});
	}
}


void inline_widget::set(const value_type &value)
{
	if(!value.empty() && text.caption().substr(0, 8) != value.substr(0, 8))
	{
		if(value.front() == '%')
		{
			conf = value[1] - 0x30;
			if(conf > 1)
				text.show();
			else text.hide();
			if(conf == 1 || conf == 3)
				pic.show();
			else pic.hide();
			indicator_->modify(pos_, value.substr(2));
		}
		else
		{
			if(value.size() < 4)
				text.caption(value);
			else
			{
				clip_text(value);
				auto pimg {lb->at(pos_).value<lbqval_t>().pimg};
				if(pimg) pic.load(*pimg);
			}
			if(lb->column_at(1).visible())
			{
				const auto w {lb->column_at(1).width()};
				if(w == util::scale(30))
					conf = 1;
				else if(w == util::scale(130))
					conf = 3;
				else conf = 2;
			}
			else conf = 0;
			if(conf > 1)
				text.show();
			else text.hide();
			if(conf == 1 || conf == 3)
				pic.show();
			else pic.hide();
		}
	}
}


void inline_widget::clip_text(const std::string &str)
{
	const auto max_pixels {text.size().width};
	int offset {0};
	text.caption(str);
	const auto strsize {str.size()};
	while(text.measure(1234).width > max_pixels)
	{
		offset += 1;
		if(strsize - offset < 4)
			break;
		text.caption(str.substr(0, strsize - offset) + "...");
	}
}


void thumb_label::create(nana::window parent, std::string_view text, bool visible)
{
	label::create(parent, visible);
	text_align(nana::align::center, nana::align_v::center);
	typeface(nana::paint::font_info {"Segoe UI", 14, {700}});
	events().expose([this] { refresh_theme(); });
	nana::drawing {*this}.draw([](nana::paint::graphics &g) { g.rectangle(false, theme::border); });
}


void thumb_label::refresh_theme()
{
	auto bgparent {nana::API::get_widget(parent())->bgcolor()};
	bgcolor(theme::is_dark() ? bgparent.blend(nana::colors::white, .020) : bgparent.blend(nana::colors::black, .020));
	fgcolor(theme::overlay_fg);
}

void widgets::Title::create(nana::window parent, std::string_view text)
{
	label::create(parent);
	caption(text);
	fgcolor(theme::title_fg);
	typeface(nana::paint::font_info {"Arial", 15/* - (double)(nana::API::screen_dpi(true) > 96) * 3*/, {800}});
	text_align(nana::align::center, nana::align_v::top);
	nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
	events().expose([this] { refresh_theme(); });
}

void widgets::Title::refresh_theme()
{
	fgcolor(theme::title_fg);
}
