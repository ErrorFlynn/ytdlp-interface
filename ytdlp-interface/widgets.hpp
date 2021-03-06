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

#include <variant>

#include "progress_ex.hpp"
#include "icons.hpp"

namespace fs = std::filesystem;

namespace widgets
{
	struct theme_t
	{
	private:
		bool dark;
		double shade {0};

	public:
		nana::color nimbus, fmbg, Label_fg, Text_fg, Text_fg_error, cbox_fg, btn_bg, btn_fg,
			path_bg, path_fg, path_link_fg, sep_bg, tbfg, tbbg, tbkw, tbkw_special, gpbg, lb_headerbg, title_fg;
		std::string gpfg;

		void make_light()
		{
			dark = false;
			using namespace nana;
			nimbus = color {"#60c8fd"};
			fmbg = btn_bg = tbbg = gpbg = color {colors::white}.blend(colors::light_grey, .3 - shade);
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
			gpfg = "0x81544F";
			title_fg = color {"#789"};
			tbkw = color {"#272"};
			tbkw_special = color {"#722"};
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
			sep_bg = color {"#888"};
			gpfg = "0xE4D6BA";
			lb_headerbg = color {"#525658"}.blend(colors::black, shade);
			title_fg = nana::color {"#cde"};
			path_link_fg = color {"#E4D6BA"};
			tbkw = color {"#b5c5d5"};
			tbkw_special = color {"#F0B0A0"};
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
	};


	class Label : public nana::label
	{
		theme_t* theme {nullptr};

	public:
		Label(nana::window parent, theme_t* ptheme, std::string_view text, bool dpi_adjust = false) : nana::label {parent, text}
		{
			theme = ptheme;
			fgcolor(theme->Label_fg);
			typeface(nana::paint::font_info {"", 12 - (double)(nana::API::screen_dpi(true) > 96) * 2 * dpi_adjust});
			text_align(nana::align::right, nana::align_v::center);
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this] { fgcolor(theme->Label_fg); });
		}
	};


	class Text : public nana::label
	{
		theme_t* theme {nullptr};
		bool error_mode_ {false};

	public:
		Text(nana::window parent, theme_t* ptheme, std::string_view text = "") : nana::label {parent, text}
		{
			theme = ptheme;
			fgcolor(theme->Text_fg);
			typeface(nana::paint::font_info {"", 12 - (double)(nana::API::screen_dpi(true) > 96) * 2});
			text_align(nana::align::left, nana::align_v::center);
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this]
			{
				fgcolor(error_mode_ ? theme->Text_fg_error : theme->Text_fg);
				scheme().activated = theme->nimbus;
			});
		}
		void error_mode(bool enable)
		{
			error_mode_ = enable;
			fgcolor(error_mode_ ? theme->Text_fg_error : theme->Text_fg);
		}
	};


	class separator : public nana::label
	{
		theme_t* theme {nullptr};

	public:
		separator(nana::window parent, theme_t* ptheme) : nana::label {parent}
		{
			theme = ptheme;
			events().expose([this] { bgcolor(theme->sep_bg); });
		}
	};


	class path_label : public nana::label
	{
		using variant = std::variant<fs::path*, std::wstring*>;
		variant v;
		theme_t* theme {nullptr};
		nana::drawing border {*this};
		bool is_path {std::holds_alternative<fs::path*>(v)};

	public:
		path_label(nana::window parent, theme_t* ptheme, const variant var) : nana::label {parent}, v {var}
		{
			theme = ptheme;
			refresh_theme();
			if(is_path) typeface(nana::paint::font_info {"Tahoma", 10});
			else typeface(nana::paint::font_info {"", 11, {700}});
			text_align(nana::align::center, nana::align_v::center);
			nana::API::effects_edge_nimbus(*this, nana::effects::edge_nimbus::over);
			events().expose([this] { refresh_theme(); });
			events().resized([this] { update_caption(); });
			border.draw([this](nana::paint::graphics& g)
			{
				if(theme->is_dark())
					g.rectangle(false, theme->path_bg.blend(nana::colors::white, .3));
				else g.rectangle(false, nana::color {"#9aa"});
			});
		}

		void update_caption()
		{
			const std::wstring wstr {is_path ? std::get<fs::path*>(v)->wstring() : *std::get<std::wstring*>(v)};
			if(!is_path && wstr.empty())
				caption("Press Ctrl+V or click here to paste media link");
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
			scheme().activated = theme->nimbus;
			if(theme->is_dark())
			{
				bgcolor(theme->path_bg);
				fgcolor(is_path ? theme->path_fg : theme->path_link_fg);
			}
			else
			{
				bgcolor(theme->fmbg);
				fgcolor(is_path ? theme->path_fg : theme->path_link_fg);
			}
		}
	};


	class cbox : public nana::checkbox
	{
		theme_t* theme {nullptr};

	public:
		cbox(nana::window parent, theme_t* ptheme, std::string_view text) : nana::checkbox {parent, text}
		{
			theme = ptheme;
			refresh_theme();
			typeface(nana::paint::font_info {"", 12});
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			fgcolor(theme->cbox_fg);
			scheme().square_border_color = fgcolor();
			auto parent {nana::API::get_widget(this->parent())};
			scheme().square_bgcolor = parent->bgcolor();
		}
	};


	class Button : public nana::button
	{
		theme_t* theme {nullptr};
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
		Button(nana::window parent, theme_t* ptheme, std::string_view text = "") : nana::button {parent, text}
		{
			theme = ptheme;
			refresh_theme();
			typeface(nana::paint::font_info {"", 14, {800}});
			enable_focus_color(false);
			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			fgcolor(theme->btn_fg);
			bgcolor(theme->btn_bg);
			scheme().activated = theme->nimbus;
			if(theme->is_dark())
				set_bground(btn_bg {});
			else set_bground("");
		}

		void cancel_mode(bool enable)
		{
			if(enable)
			{
				if(theme->is_dark())
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
				fgcolor(theme->btn_fg);
				scheme().activated = theme->nimbus;
			}
		}

		void image(const void *data)
		{
			img.open(data, sizeof data);
			if(enabled()) icon(img);
		}

		void image_disabled(const void *data)
		{
			img_disabled.open(data, sizeof data);
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
		theme_t* theme {nullptr};
		nana::drawing dw {*this};

	public:

		Listbox(nana::window parent, theme_t* ptheme) : listbox {parent}
		{
			theme = ptheme;
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

		void refresh_theme()
		{
			bgcolor(theme->tbbg);
			fgcolor(theme->tbfg);
			if(theme->is_dark())
			{
				borderless(true);
				scheme().header_bgcolor = theme->lb_headerbg;
				scheme().header_fgcolor = nana::colors::white;
				scheme().cat_fgcolor = theme->nimbus;
				scheme().item_selected = nana::color {"#BA4B3E"};
				scheme().item_selected_border = nana::color {"#BA4B3E"}.blend(nana::colors::black, .15);
				scheme().item_highlighted = nana::color {"#544"};
				dw.draw([](nana::paint::graphics& g)
				{
					g.rectangle(false, nana::color {"#666"});
				});
			}
			else
			{
				borderless(false);
				scheme().header_bgcolor = nana::color {"#f1f2f4"};
				scheme().header_fgcolor = nana::colors::black;
				scheme().cat_fgcolor = nana::color {"#039"};
				scheme().item_selected = nana::color {"#c7dEe4"}.blend(nana::colors::grey, .1 - theme->contrast());
				scheme().item_selected_border = nana::color {"#a7cEd4"}.blend(nana::colors::grey, .1 - theme->contrast());
				scheme().item_highlighted = nana::color {"#ECFCFF"};
				dw.clear();
			}
		}
	};


	class Progress : public nana::progress_ex
	{
		theme_t* theme {nullptr};

	public:

		Progress(nana::window parent, theme_t* ptheme) : progress_ex {parent}
		{
			theme = ptheme;
			refresh_theme();
			typeface(nana::paint::font_info {"", 11, {800}});
			nana::paint::image img;
			img.open(arr_progbar_jpg, sizeof arr_progbar_jpg);
			image(img);
			events().expose([this] { refresh_theme(); });
		}

		void refresh_theme()
		{
			if(theme->is_dark())
			{
				dark_bg(true);
				outline_color(nana::color {"#345"});
				text_contrast_colors(theme->Label_fg, theme->Label_fg);
				scheme().background = theme->fmbg.blend(nana::colors::black, .1);
				scheme().lower_background = theme->fmbg.blend(nana::colors::white, .1);
			}
			else
			{
				dark_bg(false);
				outline_color(nana::color {"#678"});
				text_contrast_colors(nana::colors::white, nana::color {"#678"});
				scheme().background = nana::color {nana::colors::white}.blend(nana::colors::light_grey, .3 - theme->contrast());
				scheme().lower_background = nana::color {"#f5f5f5"}.blend(nana::colors::light_grey, .3 - theme->contrast());
			}
		}
	};


	class Group : public nana::group
	{
		theme_t* theme {nullptr};
		std::string title;

	public:

		Group(nana::window parent, std::string title, theme_t* ptheme) : group {parent}, title {title}
		{
			theme = ptheme;
			enable_format_caption(true);
			caption(title);
			caption_align(nana::align::center);
			refresh_theme();
			events().expose([this] { refresh_theme(); });
		}

		nana::widget& caption(std::string utf8)
		{
			return nana::group::caption("<bold color=" + theme->gpfg + " size=11 font=\"Tahoma\"> " + utf8 + " </>");
		}

		void refresh_theme()
		{
			bgcolor(theme->gpbg);
			caption(title);
		}
	};


	class Combox : public nana::combox
	{
		theme_t* theme {nullptr};

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
				const nana::color bg_hilighted {"#BA4B3E"};

				g.rectangle(r, true, bg_normal);
				if(state == StateHighlighted)
				{
					g.rectangle(r, false, bg_hilighted.blend(nana::colors::white, .1));
					g.rectangle(nana::rectangle {r}.pare_off(1), false, bg_hilighted.blend(nana::colors::black, .4));
					g.palette(false, bg_normal);
					nana::paint::draw {g}.corner(r, 1);
					g.gradual_rectangle(nana::rectangle {r}.pare_off(2), bg_hilighted, bg_hilighted.blend(nana::colors::black, .2), true);
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
				const unsigned padding {4}; // empty space padding the top and bottom of the text area
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
		Combox(nana::window parent, theme_t* ptheme) : nana::combox {parent}
		{
			theme = ptheme;
			refresh_theme();
			events().expose([this] { refresh_theme(); });
			nana::drawing {*this}.draw([this](nana::paint::graphics& g)
			{
				if(theme->is_dark())
					g.rectangle(false, nana::color {"#999A9E"});
			});
		}

		void refresh_theme()
		{
			auto parent {nana::API::get_widget(this->parent())};
			bgcolor(parent->bgcolor());
			scheme().activated = theme->nimbus;
			if(theme->is_dark())
			{
				fgcolor(theme->Label_fg);
				renderer(&renderer_);
				scheme().selection = nana::color {"#d8d4c8"};
				scheme().selection_text = bgcolor();
			}
			else
			{
				fgcolor(nana::colors::black);
				renderer(nullptr);
				scheme().selection = nana::color {"#7D8393"};
				scheme().selection_text = bgcolor();
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
		theme_t* theme {nullptr};
		bool highlighted {true};

	public:
		Textbox(nana::window parent, theme_t* ptheme) : nana::textbox {parent}
		{
			theme = ptheme;
			refresh_theme();
			events().expose([this] { refresh_theme(); });
			nana::drawing {*this}.draw([](nana::paint::graphics &g)
			{
				g.rectangle(false, nana::color {"#999A9E"});
			});
		}

		void refresh_theme()
		{
			scheme().activated = theme->nimbus;
			auto bgparent {nana::API::get_widget(parent())->bgcolor()};
			bgcolor(theme->is_dark() ? bgparent.blend(nana::colors::black, .045) : theme->tbbg);
			fgcolor(theme->tbfg);
			highlight(true);
			set_keywords("special", true, true, {"[download]"});
		}

		void set_keyword(std::string name)
		{
			set_keywords("general", true, true, {name});
		}

		void highlight(bool enable)
		{
			highlighted = enable;
			if(enable)
			{
				set_highlight("general", theme->tbkw, theme->tbbg);
				set_highlight("special", theme->tbkw_special, theme->tbbg);
			}
			else
			{
				erase_highlight("general");
				erase_highlight("special");
			}
		}

		bool highlight() { return highlighted; }
	};


	class Title : public nana::label
	{
		theme_t* theme {nullptr};

	public:
		Title(nana::window parent, theme_t* ptheme) : nana::label {parent}
		{
			theme = ptheme;
			fgcolor(theme->title_fg);
			typeface(nana::paint::font_info {"Arial", 15 - (double)(nana::API::screen_dpi(true) > 96) * 3, {800}});
			text_align(nana::align::center, nana::align_v::top);
			nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
			events().expose([this] { fgcolor(theme->title_fg); });
		}
	};

	class Slider : public nana::slider
	{
		theme_t* theme {nullptr};

	public:
		Slider(nana::window parent, theme_t* ptheme) : nana::slider {parent}
		{
			theme = ptheme;
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
			if(theme->is_dark())
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
}