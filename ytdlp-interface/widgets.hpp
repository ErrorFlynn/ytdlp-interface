#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/gui/widgets/spinbox.hpp>

#include <variant>
#include <iostream>
#include <map>
#include <Windows.h>

#include "progress_ex.hpp"
#include "icons.hpp"
#include "util.hpp"

#pragma warning(disable : 4267)
#ifdef small
	#undef small
#endif

namespace fs = std::filesystem;

namespace widgets
{
	static struct theme_t
	{
	private:
		bool dark;
		double shade {0};

	public:
		nana::color nimbus, fmbg, Label_fg, Text_fg, Text_fg_error, cbox_fg, btn_bg, btn_fg, path_bg, path_fg, path_link_fg, 
			sep_bg, tbfg, tbbg, tbkw, tbkw_id, tbkw_special, tbkw_warning, tbkw_error, gpbg, lb_headerbg, title_fg, overlay_fg, border,
			tb_selbg, tb_selbg_unfocused, expcol_fg;
		std::string gpfg;

		void make_light()
		{
			dark = false;
			using namespace nana;
			nimbus = color {"#60c8fd"};
			fmbg = btn_bg = tbbg = color {colors::white}.blend(colors::light_grey, .3 - shade);
			gpbg = fmbg.blend(colors::black, .025);
			tbfg = colors::black;
			Label_fg = color {"#569"};
			Text_fg = color {"#575"};
			Text_fg_error = color {"#a55"};
			cbox_fg = color {"#458"};
			btn_fg = color {"#67a"};
			path_bg = color {"#eef6ee"}.blend(colors::light_grey, .3 - shade);
			path_fg = color {"#354"};
			path_link_fg = color {"#789"};
			sep_bg = color {"#cdcdcd"};
			expcol_fg = color {"#aaa"};
			gpfg = "0x81544F";
			title_fg = color {"#789"};
			tbkw = color {"#272"};
			tbkw_id = color {"#777"};
			tbkw_special = color {"#722"};
			tbkw_warning = color {"#B96C00"};
			tbkw_error = color {"#aa2222"};;
			overlay_fg = color {"#bcc0c3"};
			border = color {"#9CB6C5"};
			tb_selbg = color {"#5695D3"};
			tb_selbg_unfocused = tb_selbg.blend(nana::colors::white, .3);
		}

		void make_dark()
		{
			dark = true;
			using namespace nana;
			nimbus = color {"#cde"};
			fmbg = color {"#2c2b2b"}.blend(colors::black, shade);
			tbbg = fmbg.blend(colors::black, .035);
			gpbg = fmbg.blend(colors::white, .035);
			tbfg = color {"#f7f7f4"};
			btn_bg = color {"#2e2d2d"}.blend(colors::black, shade);
			Label_fg = btn_fg = cbox_fg = color {"#e6e6e3"};
			Text_fg = color {"#def"};
			Text_fg_error = color {"#f99"};
			path_bg = color {"#383737"}.blend(colors::black, shade);
			path_fg = colors::white;
			sep_bg = color {"#777"};
			expcol_fg = color {"#999"};
			gpfg = "0xE4D6BA";
			lb_headerbg = color {"#525658"}.blend(colors::black, shade);
			title_fg = nana::color {"#cde"};
			path_link_fg = color {"#E4D6BA"};
			tbkw = color {"#b5c5d5"};
			tbkw_id = color {"#ccc"};
			tbkw_special = color {"#F0B0A0"};
			tbkw_warning = color {"#EEBF00"};
			tbkw_error = color {"#CA86E3"};
			overlay_fg = border = color {"#666"};
			tb_selbg = color {"#95443B"};
			tb_selbg_unfocused = tb_selbg.blend(nana::colors::black , .3);
		}

		bool is_dark() { return dark; }
		void contrast(double factor)
		{
			shade = std::clamp(factor, .0, .3);
			if(dark) make_dark();
			else make_light();
		}
		double contrast() { return shade; }

		theme_t() { make_light(); }
	} theme;


	class Label : public nana::label
	{

	public:

		Label() : label() {}

		Label(nana::window parent, std::string_view text, bool dpi_adjust = false)
		{
			create(parent, text, dpi_adjust);
		}

		void create(nana::window parent, std::string_view text, bool dpi_adjust = false)
		{
			label::create(parent);
			caption(text);
			fgcolor(theme.Label_fg);
			typeface(nana::paint::font_info {"", 12 - (double)(nana::API::screen_dpi(true) > 96) * 2 * dpi_adjust});
			text_align(nana::align::right, nana::align_v::center);
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this] { fgcolor(theme.Label_fg); });
		}
	};


	class Text : public nana::label
	{
		bool error_mode_ {false};

	public:
		Text(nana::window parent, std::string_view text = "", bool dpi_adjust = false) : label {parent, text}
		{
			fgcolor(theme.Text_fg);
			typeface(nana::paint::font_info {"", 12 - (double)(nana::API::screen_dpi(true) > 96) * 2 * dpi_adjust});
			text_align(nana::align::left, nana::align_v::center);
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this]
			{
				fgcolor(error_mode_ ? theme.Text_fg_error : theme.Text_fg);
				scheme().activated = theme.nimbus;
			});
		}

		void error_mode(bool enable)
		{
			error_mode_ = enable;
			fgcolor(error_mode_ ? theme.Text_fg_error : theme.Text_fg);
		}
	};


	class Separator : public nana::panel<false>
	{
		nana::place plc;
		nana::label sep1, sep2;

	public:

		Separator() {}

		Separator(nana::window parent)
		{
			create(parent);
		}

		void create(nana::window parent)
		{
			panel<false>::create(parent);
			plc.bind(*this);
			sep1.create(*this);
			sep2.create(*this);
			plc.div("vert <sep1> <> <sep2>");
			plc["sep1"] << sep1;
			plc["sep2"] << sep2;

			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			sep1.bgcolor(theme.sep_bg);
			sep2.bgcolor(theme.sep_bg);
		}
	};


	class path_label : public nana::label
	{
		using variant = std::variant<fs::path*, std::wstring*>;
		variant v;		
		bool is_path {false};

	public:

		path_label() : label() {}

		path_label(nana::window parent, const variant var) { create(parent, var); }

		void create(nana::window parent, const variant var)
		{
			label::create(parent);
			v = var;
			is_path = std::holds_alternative<fs::path *>(v);
			refresh_theme();
			if(is_path) typeface(nana::paint::font_info {"Tahoma", 10});
			else typeface(nana::paint::font_info {"", 11, {700}});
			text_align(nana::align::center, nana::align_v::center);
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			nana::API::effects_edge_nimbus(*this, nana::effects::edge_nimbus::over);
			events().expose([this] { refresh_theme(); });
			events().resized([this] { update_caption(); });

			nana::drawing {*this}.draw([this](nana::paint::graphics &g)
			{
				if(theme.is_dark())
					g.rectangle(false, theme.path_bg.blend(nana::colors::white, .3));
				else g.rectangle(false, nana::color {"#9aa"});
			});
		}

		void update_caption()
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

		void refresh_theme()
		{
			scheme().activated = theme.nimbus;
			if(is_path)
				fgcolor(caption().find("!!!") == -1 ? theme.path_fg : theme.tbkw_special);
			else fgcolor(theme.path_link_fg);
		}
	};


	class cbox : public nana::checkbox
	{
	public:

		cbox() : checkbox() {}

		cbox(nana::window parent, std::string_view text)
		{
			create(parent, text);
		}

		void create(nana::window parent, std::string_view text)
		{
			checkbox::create(parent);
			caption(text);
			refresh_theme();
			typeface(nana::paint::font_info {"", 12});
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			fgcolor(theme.cbox_fg);
			scheme().square_border_color = fgcolor();
			auto parent {nana::API::get_widget(this->parent())};
			scheme().square_bgcolor = parent->bgcolor();
		}
	};


	class Button : public nana::button
	{
		nana::paint::image img, img_disabled;

		class btn_bg : public nana::element::element_interface
		{
		public:
			bool draw(nana::paint::graphics& g, const nana::color& bg,
					  const nana::color& fg, const nana::rectangle& r, nana::element_state state) override
			{
				auto from {bg}, to {bg.blend(nana::color {"#e6e6e8"}, .15)};
				if(state != nana::element_state::pressed)
					g.gradual_rectangle(r, from, to, true);
				else g.gradual_rectangle(r, to, from, true);
				g.rectangle(r, false, static_cast<nana::color_rgb>(0x7f7f7f));
				return true;
			}
		};

	public:

		Button() : button() {}

		Button(nana::window parent, std::string_view text = "", bool small = false)
		{
			create(parent, text, small);
		}

		void create(nana::window parent, std::string_view text = "", bool small = false)
		{
			button::create(parent);
			caption(text);
			refresh_theme();
			typeface(nana::paint::font_info {"", small ? 11e0 : 14, {800}});
			enable_focus_color(false);
			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			fgcolor(theme.btn_fg);
			bgcolor(theme.btn_bg);
			scheme().activated = theme.nimbus;
			if(theme.is_dark())
				set_bground(btn_bg {});
			else set_bground("");
		}

		void cancel_mode(bool enable)
		{
			if(enable)
			{
				if(theme.is_dark())
				{
					fgcolor(nana::color {"#E4D6BA"});
					scheme().activated = nana::color {"#E4D6BA"};
				}
				else
				{
					fgcolor(nana::color {"#966"});
					scheme().activated = nana::color {"#a77"};
				}
			}
			else
			{
				fgcolor(theme.btn_fg);
				scheme().activated = theme.nimbus;
			}
		}

		void image(const void *data, unsigned size)
		{
			img.open(data, size);
			if(enabled()) icon(img);
		}

		void image_disabled(const void *data, unsigned size)
		{
			img_disabled.open(data, size);
			if(!enabled()) icon(img_disabled);
		}

		void enable(bool state)
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
	};


	class Listbox : public nana::listbox
	{
		nana::drawing dw {*this};
		bool hicontrast {false};

	public:

		index_pairs highlighted_lines;

		Listbox(nana::window parent, bool hicontrast = false) : listbox {parent}, hicontrast {hicontrast}
		{
			refresh_theme();
			events().expose([this] { refresh_theme(); });
		}

		size_t item_count()
		{
			size_t count {0};
			for(size_t cat_idx {0}; cat_idx < size_categ(); cat_idx++)
				count += at(cat_idx).size();
			return count;
		}

		auto item_from_value(std::wstring val, size_t cat = 0)
		{
			for(auto item : at(cat))
				if(item.value<std::wstring>() == val)
					return item;
			return at(cat).end();
		}

		void refresh_theme()
		{
			using namespace nana;
			bgcolor(theme.tbbg);
			fgcolor(theme.tbfg);
			dw.draw([](paint::graphics &g) { g.rectangle(false, theme.border); });

			if(theme.is_dark())
			{
				borderless(true);
				scheme().header_bgcolor = theme.lb_headerbg;
				scheme().header_fgcolor = colors::white;
				scheme().cat_fgcolor = theme.nimbus;
				scheme().item_selected = color {"#AC4F44"};
				scheme().item_selected_border = color {"#B05348"}.blend(colors::black, .15);
				if(hicontrast)
					scheme().item_highlighted = color {"#544"}.blend(colors::light_grey, .15 - theme.contrast()/2);
				else scheme().item_highlighted = color {"#544"};
			}
			else
			{
				borderless(false);
				scheme().header_bgcolor = color {"#f1f2f4"};
				scheme().header_fgcolor = colors::black;
				scheme().cat_fgcolor = color {"#039"};
				auto c {theme.contrast()};
				scheme().item_selected = color {"#c7dEe4"}.blend(colors::grey, .1 - c);
				scheme().item_selected_border = color {"#a7cEd4"}.blend(colors::grey, .1 - c);
				scheme().item_highlighted = color {"#eee"}.blend(colors::dark_grey, .25 - c / 2);
				dw.clear();
			}

			for(const auto &ip : highlighted_lines)
			{
				try
				{
					at(ip).fgcolor(theme.is_dark() ? color {"#d4c6aa"} : colors::green);
					at(ip).bgcolor(theme.tbbg.blend(theme.is_dark() ? color {"#eee"} : color {"#222"}, .06));
				}
				catch(...) {}
			}
		}
	};


	class Progress : public nana::progress_ex
	{
	public:

		Progress() : progress_ex() {}

		Progress(nana::window parent) : progress_ex {parent}
		{
			refresh_theme();
			typeface(nana::paint::font_info {"", 11, {800}});
			nana::paint::image img;
			img.open(arr_progbar_jpg, sizeof arr_progbar_jpg);
			image(img);
			events().expose([this] { refresh_theme(); });
		}

		void create(nana::window parent)
		{
			nana::progress_ex::create(parent, true);
			refresh_theme();
			typeface(nana::paint::font_info {"", 11, {800}});
			nana::paint::image img;
			img.open(arr_progbar_jpg, sizeof arr_progbar_jpg);
			image(img);
			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			if(theme.is_dark())
			{
				dark_bg(true);
				outline_color(nana::color {"#345"});
				text_contrast_colors(theme.Label_fg, theme.Label_fg);
				scheme().background = theme.fmbg.blend(nana::colors::black, .1);
				scheme().lower_background = theme.fmbg.blend(nana::colors::white, .1);
			}
			else
			{
				dark_bg(false);
				outline_color(nana::color {"#678"});
				text_contrast_colors(nana::colors::white, nana::color {"#678"});
				scheme().background = nana::color {nana::colors::white}.blend(nana::colors::light_grey, .3 - theme.contrast());
				scheme().lower_background = nana::color {"#f5f5f5"}.blend(nana::colors::light_grey, .3 - theme.contrast());
			}
		}
	};


	class Group : public nana::group
	{
		std::string title;

	public:

		Group() : group() {}

		Group(nana::window parent, std::string title)
		{
			create(parent, title);
		}

		void create(nana::window parent, std::string title = "")
		{
			group::create(parent);
			this->title = title;
			caption(title);
			enable_format_caption(true);
			caption_align(nana::align::center);
			refresh_theme();
			events().expose([this] { refresh_theme(); });
		}

		nana::widget& caption(std::string utf8)
		{
			title = utf8;
			widget::typeface(nana::paint::font_info{}); // workaround for bug: group.hpp declares undefined method `typeface`
			return group::caption("<bold color=" + theme.gpfg + " size=11 font=\"Tahoma\"> " + utf8 + " </>");
		}

		std::string caption()
		{
			auto cap {group::caption()};
			auto pos1 {cap.find('>') + 2}, pos2 {cap.rfind('<') - 2};
			return cap.substr(pos1, pos2 - pos1 + 1);
		}

		void refresh_theme()
		{
			bgcolor(theme.gpbg);
			caption(title);
		}
	};


	class Combox : public nana::combox
	{
		class my_renderer : public nana::combox::item_renderer
		{
			void image(bool enabled, unsigned pixels) override {}

			// gets called multiple times (once for each item)
			void item(widget_reference, graph_reference g, const nana::rectangle& r, const item_interface* ii, state_t state) override
			{
				const unsigned margin {8};
				const auto& img {ii->image()};
				const nana::color fg_normal {"#e6e6e3"};
				const nana::color fg_hilighted {nana::colors::white};
				const nana::color bg_normal {"#33373e"};
				const nana::color bg_hilighted {"#A94c41"};

				g.rectangle(r, true, bg_normal);
				if(state == StateHighlighted)
				{
					g.rectangle(r, false, bg_hilighted.blend(nana::colors::white, .1));
					g.rectangle(nana::rectangle {r}.pare_off(1), false, bg_hilighted.blend(nana::colors::black, .4));
					g.palette(false, bg_normal);
					nana::paint::draw {g}.corner(r, 1);
					g.gradual_rectangle(nana::rectangle {r}.pare_off(2), bg_hilighted, bg_hilighted.blend(nana::colors::black, .25), true);
				}
				auto size {g.text_extent_size(ii->text())};
				auto pos {r.position()};
				pos.x += margin;
				if(!img.empty())
				{
					int yoff {static_cast<int>(r.height / 2) - 8};
					img.stretch(nana::rectangle {img.size()}, g, {pos.x, pos.y + yoff, 16, 16});
					pos.x += 16 + margin;
				}
				pos.y += static_cast<int>(r.height / 2 - size.height / 2);
				g.string(pos, ii->text(), (state == StateNone ? fg_normal : fg_hilighted));
			}

			unsigned item_pixels(graph_reference g) const override
			{
				const unsigned padding {6}; // empty space padding the top and bottom of the text area
				unsigned ascent, descent, internal_leading; // font metrics
				g.text_metrics(ascent, descent, internal_leading);
				return ascent + descent + padding;
			}

			void background(widget_reference, graph_reference g)
			{
				g.rectangle(false, nana::color {"#999A9E"});
			}
		} renderer_;

	public:

		Combox() : combox() {}

		Combox(nana::window parent)
		{
			create(parent);
		}

		void create(nana::window parent)
		{
			combox::create(parent);
			refresh_theme();
			events().expose([this] { refresh_theme(); });
			nana::drawing {*this}.draw([this](nana::paint::graphics &g)
			{
				if(theme.is_dark())
					g.rectangle(false, nana::color {"#999A9E"});
			});
		}

		void refresh_theme()
		{
			bgcolor(nana::API::get_widget(parent())->bgcolor());
			scheme().activated = theme.nimbus;
			scheme().selection = theme.tb_selbg;
			scheme().selection_unfocused = theme.tb_selbg_unfocused;

			if(theme.is_dark())
			{
				fgcolor(theme.Label_fg);
				renderer(&renderer_);
			}
			else
			{
				fgcolor(nana::colors::black);
				renderer(nullptr);
			}
		}

		int caption_index()
		{
			const auto cap {caption()};
			size_t size {the_number_of_options()}, idx {0};
			for(; idx < size; idx++)
				if(cap == text(idx))
					break;
			if(idx == size) return -1;
			return idx;
		}
	};


	class Textbox : public nana::textbox
	{
		bool highlighted {false};

	public:

		Textbox() : textbox() {};

		Textbox(nana::window parent, bool visible = true)
		{
			create(parent, visible);
		}

		void create(nana::window parent, bool visible = true)
		{
			textbox::create(parent, visible);
			refresh_theme();
			events().expose([this] { refresh_theme(); });
			nana::drawing {*this}.draw([](nana::paint::graphics &g)
			{
				g.rectangle(false, nana::color {"#999A9E"});
			});
		}

		void refresh_theme()
		{
			scheme().activated = theme.nimbus;
			auto bgparent {nana::API::get_widget(parent())->bgcolor()};
			bgcolor(theme.is_dark() ? bgparent.blend(nana::colors::black, .045) : bgparent);
			fgcolor(theme.tbfg);
			scheme().selection = theme.tb_selbg;
			scheme().selection_unfocused = theme.tb_selbg_unfocused;
			highlight(highlighted);
			set_keywords("special", true, true, {"[download]"});
			set_keywords("warning", true, true, {"WARNING:"});
			set_keywords("error", true, true, {"ERROR:"});
		}

		void set_keyword(std::string name, std::string category = "general")
		{
			set_keywords(category, true, true, {name});
		}

		void highlight(bool enable)
		{
			highlighted = enable;
			if(enable)
			{
				set_highlight("id", theme.tbkw_id, theme.tbbg);
				set_highlight("general", theme.tbkw, theme.tbbg);
				set_highlight("special", theme.tbkw_special, theme.tbbg);
				set_highlight("warning", theme.tbkw_warning, theme.tbbg);
				set_highlight("error", theme.tbkw_error, theme.tbbg);
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

		bool highlight() { return highlighted; }
	};


	class Title : public nana::label
	{
	public:
		Title(nana::window parent, std::string text = "") : label {parent, text}
		{
			fgcolor(theme.title_fg);
			typeface(nana::paint::font_info {"Arial", 15 - (double)(nana::API::screen_dpi(true) > 96) * 3, {800}});
			text_align(nana::align::center, nana::align_v::top);
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this] { fgcolor(theme.title_fg); });
		}
	};


	class Slider : public nana::slider
	{
	public:
		Slider(nana::window parent) : slider {parent}
		{
			transparent(true);
			refresh_theme();
			events().expose([this] {refresh_theme(); });
			vernier([&](unsigned max, unsigned val)
			{
				return std::to_string(val);
			});
		}

		void refresh_theme()
		{
			if(theme.is_dark())
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
	};


	class Overlay : public nana::label
	{
	public:

		Overlay() : label() {}

		Overlay(nana::window parent, nana::widget *outbox, std::string_view text = "", bool visible = true) : label {parent, text, visible}
		{
			create(parent, outbox, text, visible);
		}

		void create(nana::window parent, nana::widget *outbox, std::string_view text = "", bool visible = true)
		{
			if(text.empty())
				caption("output from yt-dlp.exe appears here\n\nright-click for options\n\ndouble-click to show queue");
			label::create(parent, visible);
			fgcolor(theme.overlay_fg);
			text_align(nana::align::center, nana::align_v::center);
			typeface(nana::paint::font_info {"", 15, {700}});
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this] { fgcolor(theme.overlay_fg); });
			nana::drawing {*this}.draw([](nana::paint::graphics &g) { g.rectangle(false, theme.border); });
			events().mouse_up([outbox, this](const nana::arg_mouse &arg)
			{
				outbox->events().mouse_up.emit(arg, *this);
			});
		}
	};


	class Menu : public nana::menu
	{
		class menu_renderer : public nana::menu::renderer_interface
		{

		public:
			menu_renderer(const nana::pat::cloneable<renderer_interface> &rd) : reuse_ {rd}, crook_ {"menu_crook"}
			{
				crook_.check(nana::facade<nana::element::crook>::state::checked);
			}

		private:
			void background(graph_reference graph, nana::window wd) override
			{
				const nana::color
					bg_normal {theme.is_dark() ? "#33373e" : "#fff"},
					clr_border {theme.is_dark() ? "#A3A2A1" : "#666"},
					bg_icon_area {theme.is_dark() ? "#43474e" : "#f6f6f6"};
				graph.rectangle(true, bg_normal); // entire area
				graph.rectangle({1,1,28,graph.height() - 2}, true, bg_icon_area); // icon area
				graph.rectangle(false, clr_border); // border
			}

			void item(graph_reference g, const nana::rectangle &r, const attr &attr) override
			{
				if(!attr.enabled) return;
				using namespace nana;
				const unsigned margin {8};
				const color 
					fg_normal {theme.is_dark() ? "#e6e6e3" : "#678"},
					bg_normal {theme.is_dark() ? "#33373e" : "#fff"},
					bg_hilighted {theme.is_dark() ? "#AC4F44" : "#d8eaf0"},
					bg_icon_area {theme.is_dark() ? "#43474e" : "#f6f6f6"};

				g.rectangle(r, true, bg_normal);				
				if(attr.item_state == state::active)
				{
					g.rectangle(r, false, bg_hilighted.blend(theme.is_dark() ? colors::white : colors::black, .1));
					g.rectangle(rectangle {r}.pare_off(1), false, bg_hilighted.blend(theme.is_dark() ? colors::black : colors::white, .4));
					g.palette(false, bg_normal);
					paint::draw {g}.corner(r, 1);
					if(theme.is_dark())
						g.gradual_rectangle(rectangle {r}.pare_off(2), bg_hilighted, bg_hilighted.blend(colors::black, .3), true);
					else g.gradual_rectangle(rectangle {r}.pare_off(2), bg_hilighted, bg_hilighted.blend(colors::white, .5), true);
					g.set_pixel(r.x, r.y, bg_icon_area);
					g.set_pixel(r.x, r.y+r.height-1, bg_icon_area);
				}
				else g.rectangle({1,r.y,28,r.height}, true, bg_icon_area); // icon area

				if(attr.checked)
				{
					auto scale = [](int val)
					{
						const static double dpi {static_cast<double>(API::screen_dpi(true))};
						if(dpi != 96)
							val = round(val * dpi / 96);
						return val;
					};

					int sx {(28 - scale(8)) / 2},
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

			void item_image(graph_reference graph, const nana::point &pos, unsigned image_px, const nana::paint::image &img) override
			{
				reuse_->item_image(graph, pos, image_px, img);
			}

			void item_text(graph_reference g, const nana::point &pos, const std::string &text, unsigned pixels, const attr &attr) override
			{
				auto size {g.text_extent_size(text)};
				if(theme.is_dark())
					g.string(pos, text, attr.enabled ? nana::colors::white : nana::color {"#aaa"});
				else g.string(pos, text, attr.enabled ? nana::colors::black : nana::colors::gray_border);
			}

			void item_text(graph_reference graph, const nana::point &pos, std::u8string_view text, unsigned pixels, const attr &atr) override
			{
				reuse_->item_text(graph, pos, text, pixels, atr);
			}

			void sub_arrow(graph_reference graph, const nana::point &pos, unsigned pixels, const attr &atr) override
			{
				using namespace nana;
				facade<element::arrow> arrow {"hollow_triangle"};
				arrow.direction(direction::east);
				arrow.draw(graph, {}, theme.is_dark() ? colors::light_grey : colors::black,
					{pos.x, pos.y + static_cast<int>(pixels - 16) / 2, 16, 16}, element_state::normal);
			}

		private:
			nana::pat::cloneable<renderer_interface> reuse_;
			nana::facade<nana::element::crook> crook_;
		};

	public:

		Menu()
		{
			//if(theme.is_dark())
				renderer(menu_renderer {renderer()});
		}
	};


	class Spinbox : public nana::spinbox
	{
	public:

		Spinbox() = default;

		Spinbox(nana::window parent)
		{
			create(parent);
		}

		void create(nana::window parent)
		{
			spinbox::create(parent);
			editable(false);

			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			bgcolor(theme.tbbg);
			fgcolor(theme.tbfg);
			scheme().activated = theme.nimbus;
		}
	};


	class Expcol : public nana::label
	{
		bool hovered {false}, downward {false};

	public:

		Expcol() : label() {}
		Expcol(nana::window parent) : label {parent} { create(parent); }

		void create(nana::window parent)
		{
			using namespace nana;
			label::create(parent);
			refresh_theme();
			events().expose([this] { refresh_theme(); });
			events().mouse_enter([this] { hovered = true; api::refresh_window(*this); });
			events().mouse_leave([this] { hovered = false; api::refresh_window(*this); });
			events().click([this] { operate(!downward); });

			drawing {*this}.draw([&, this](paint::graphics &g)
			{
				auto draw_v = [&](const point pos, const int w)
				{
					const int x {pos.x}, y {pos.y};
					if(downward)
					{
						g.line({x, y}, {x + w/2, y + w/2});
						g.line({x + w/2, y + w/2}, {x + w, y});
					}
					else
					{
						g.line({x + w/2, y}, {x, y + w/2});
						g.line({x + w/2, y}, {x + w, y + w/2});
					}
				};
				int pad {4};
				const double dpi {static_cast<double>(nana::API::screen_dpi(true))};
				if(dpi > 96)
					pad = round(pad * dpi / 96.0);

				const auto w {g.width() - pad * 2};
				auto x {pad}, y {pad + (int)(g.height()/2 - w/1.5)};

				if(hovered)
				{
					g.palette(false, theme.nimbus);
					draw_v({x-1, y-1}, w);
					draw_v({x, y-2}, w);
					draw_v({x+1, y-1}, w);
					draw_v({x+1, y}, w);
					draw_v({x, y}, w);
					draw_v({x-1, y}, w);
					draw_v({x, y+1}, w);
					g.blur({0, 0, g.width(), g.height()}, 1);
					g.palette(false, fgcolor());
					draw_v({x, y+1}, w);
				}
				else
				{
					g.palette(false, fgcolor());
					draw_v({x, y}, w);
				}
			});
		}

		void refresh_theme()
		{
			bgcolor(theme.fmbg);
			fgcolor(theme.expcol_fg);
		}

		bool collapsed() { return downward; }

		void operate(bool collapse)
		{
			downward = collapse;
			nana::api::refresh_window(*this);
		}
	};

}