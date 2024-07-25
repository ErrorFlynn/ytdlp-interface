#pragma warning(disable : 4244 4018)

#include "hsl_picker.hpp"
#include "util.hpp"
#include <nana/gui/drawing.hpp>
#include <nana/paint/image_process_selector.hpp>

#include <iostream>
#include <cassert>

namespace nana
{
	namespace drawerbase
	{
		namespace hsl_picker
		{
			trigger::trigger()
			{
				attr_.e_state = element_state::normal;
			}

			trigger::~trigger()
			{

			}

			void trigger::attached(widget_reference widget, graph_reference graph)
			{
				wdg_ = &widget;
				picker_ = reinterpret_cast<nana::hsl_picker*>(wdg_);

				API::dev::enable_space_click(widget, true);
				API::effects_edge_nimbus(widget, effects::edge_nimbus::none);
			}

			element::cite_bground& trigger::cite()
			{
				return cite_;
			}

			void trigger::refresh(graph_reference g)
			{
				bool eb = wdg_->enabled();

				attr_.bgcolor = wdg_->bgcolor();
				attr_.fgcolor = wdg_->fgcolor();

				element_state e_state = attr_.e_state;
				if(eb)
				{
					if(element_state::normal == e_state)
						e_state = element_state::focus_normal;
					else if(element_state::hovered == e_state)
						e_state = element_state::focus_hovered;
				}
				else
					e_state = element_state::disabled;

				if(API::is_transparent_background(*wdg_))
					API::dev::copy_transparent_background(*wdg_, g);
				else draw_background(g);
				if(!lightness_captured_ || API::is_transparent_background(*wdg_))
					draw_huesat(g);
				draw_lightness(g);
			}

			/*void trigger::mouse_enter(graph_reference g, const arg_mouse &arg)
			{

			}*/

			void trigger::mouse_down(graph_reference g, const arg_mouse &arg)
			{
				using namespace util;
				// y-coords: 0->29 = lightness bar & selector ; 30->39 = padding (empty space) ; 40->142 = huesat map
				if(arg.pos.y < scale(30))
				{
					wdg_->set_capture(true);
					lightness_captured_ = true;
					picker_->lbuf_selpos = arg.pos.x/(g.width()/101.0);
					refresh(g);
					API::refresh_window(*wdg_);
				}
				else if(arg.pos.y > scale(40) && arg.pos.y < g.height()-1 && arg.pos.x > 0 && arg.pos.x < g.width()-1)
				{
					wdg_->set_capture(true);
					lightness_captured_ = false;
					int x {arg.pos.x}, y {arg.pos.y- scale(40)};
					picker_->map_selpos.x = x/((g.width()-2)/360.0);
					picker_->map_selpos.y = y/((g.height()-scale(40)-2)/101.0);
					picker_->update_lightness();
					refresh(g);
				}
				if(picker_->cb)
					picker_->cb();
			}

			void trigger::mouse_move(graph_reference g, const arg_mouse &arg)
			{
				using namespace util;
				if(API::capture_window() == *wdg_)
				{
					if(lightness_captured_)
					{
						picker_->lbuf_selpos = std::clamp(static_cast<int>(arg.pos.x/(g.width()/101.0)), 0, 100);
						refresh(g);
						API::refresh_window(*wdg_);
					}
					else
					{
						int x {arg.pos.x-1}, y {arg.pos.y - scale(40)};
						picker_->map_selpos.x = std::clamp(static_cast<int>(x/((g.width()-2)/360.0)), 0, 359);
						picker_->map_selpos.y = std::clamp(static_cast<int>(y/((g.height()-scale(40)-2)/101.0)), 0, 100);
						picker_->update_lightness();
						refresh(g);
						API::refresh_window(*wdg_);
					}
					if(picker_->cb)
						picker_->cb();
				}
			}

			void trigger::mouse_up(graph_reference g, const arg_mouse &arg)
			{
				lightness_captured_ = false;
				wdg_->release_capture();
			}

			/*void trigger::mouse_leave(graph_reference g, const arg_mouse &arg)
			{

			}*/

			void trigger::draw_background(graph_reference g)
			{
				if(lightness_captured_)
					g.rectangle({0, 0, g.width(), util::scale_uint(39)}, true, picker_->bgcolor());
				else g.rectangle(true, picker_->bgcolor());
			}

			void trigger::draw_huesat(graph_reference g)
			{
				using namespace util;
				rectangle r {0, scale(39), g.width(), g.height()- scale(39)};
				r.pare_off(1);
				picker_->pixbuf.stretch(rectangle{picker_->pixbuf.size()}, g.handle(), r);

				// draw selector
				int x {1 + static_cast<int>(scale_uint(picker_->map_selpos.x) * r.width / scale_uint(360))};
				int y {scale(40) + static_cast<int>(scale_uint(picker_->map_selpos.y) * r.height / scale_uint(101))};
				g.palette(false, colors::white);
				if(scale(picker_->map_selpos.y) == scale(100))
					y = g.height()-2;
				const auto scale4 {scale(4)}, scale2 {scale(2)};
				if(y-2 > scale(40))
					g.line({x, std::clamp(y-scale4, scale(40), static_cast<int>(g.height()))}, {x, y-scale2});
				else if(y != scale(40)) g.set_pixel(x, scale(40));
				g.line({x, y+scale2}, {x, y+scale4});
				g.line({x-scale4, y}, {x-scale2, y});
				g.line({x+scale2, y}, {x+scale4, y});
				r.pare_off(-1);
				g.rectangle(r, false, nana::colors::black);
			}

			void trigger::draw_lightness(graph_reference g)
			{
				using namespace util;
				const auto selpos {picker_->lbuf_selpos};
				rectangle r {0, 0, g.width(), scale_uint(29)};
				rectangle r_sel {0, 0, g.width(), 7};
				int x {static_cast<int>(selpos * g.width() / 101.0)};
				int y {0};
				if(selpos == 0) x -= 6; else x -= 4;

				const color &clr {picker_->lbuf[selpos]};
				g.palette(false, clr);
				for(int n {1}; n<scale(6); n++)
					g.line({x+1+n, y+n}, {x+scale(12)-n, y+n});

				// draw selector
				color clr_outline {static_cast<color_rgb>(~clr.px_color().value)};
				const auto avg {(clr_outline.r() + clr_outline.g() + clr_outline.b())/3};
				clr_outline.from_rgb(avg, avg, avg);
				g.palette(false, clr_outline);
				g.line_begin(x, y);
				g.line_to({x+scale(12), y});
				g.line_to({x+scale(6), y+scale(6)});
				g.line_to({x, y});

				// draw color bar
				rectangle r_light {0, scale(9), g.width(), scale_uint(20)};
				paint::graphics g_light {{101, 20}};
				for(int n {0}; n<101; n++)
					g_light.line({n, 0}, {n, 19}, picker_->lbuf[n]);
				g_light.stretch(g, r_light);
			}
		} // namespace hsl_picker
	}// namespace drawerbase

	hsl_picker::hsl_picker()
	{
		make_pixels();
		update_lightness();
	}

	hsl_picker::hsl_picker(window wd, const rectangle& r)
	{
		create(wd, r, true);
		make_pixels();
		update_lightness();
	}

	hsl_picker& hsl_picker::set_bground(const pat::cloneable<element::element_interface>& rv)
	{
		internal_scope_guard lock;
		get_drawer_trigger().cite().set(rv);
		return *this;
	}

	hsl_picker& hsl_picker::set_bground(const std::string& name)
	{
		internal_scope_guard lock;
		get_drawer_trigger().cite().set(name.data());
		return *this;
	}

	hsl_picker& hsl_picker::transparent(bool enabled)
	{
		if(enabled)
			API::effects_bground(*this, effects::bground_transparent(0), 0.0);
		else
			API::effects_bground_remove(*this);
		return *this;
	}

	bool hsl_picker::transparent() const
	{
		return API::is_transparent_background(*this);
	}

	void hsl_picker::rgb(unsigned clr)
	{
		color_value(static_cast<nana::color_rgb>(clr));
	}

	std::string hsl_picker::rgbstr(const color *clr)
	{
		unsigned rgbval {clr ? clr->rgba().value >> 8 : rgb()};
		std::stringstream ss;
		ss.width(6);
		ss.fill('0');
		ss << std::hex << rgbval;
		auto hex {ss.str()};
		for(auto &c : hex)
			if(isalpha(c)) c -= 0x20;
		return hex;
	}

	void hsl_picker::hsl(hsl_color_t clr)
	{
		lbuf_selpos = clr.l;
		map_selpos = {static_cast<int>(clr.h), static_cast<int>(clr.s)};
		update_lightness();
		//if(cb) cb();
	}

	void hsl_picker::color_value(color clr, bool call_back)
	{
		auto hsl {rgb2hsl(clr.r(), clr.g(), clr.b())};
		lbuf_selpos = hsl.l;
		map_selpos = {static_cast<int>(hsl.h), static_cast<int>(hsl.s)};
		update_lightness();
		if(call_back && cb) cb();
	}

	void hsl_picker::make_pixels()
	{
		color clr;
		for(int s {0}; s<101; s++)
		{
			auto line {pixbuf[s]};
			for(int h {0}; h<360; h++)
			{
				clr.from_hsl(h, static_cast<double>(s)/100, .50);
				//clr = hsl2rgb(h, static_cast<double>(s)/100, .50);
				auto r {static_cast<unsigned>(round(clr.r()))};
				auto g {static_cast<unsigned>(round(clr.g()))};
				auto b {static_cast<unsigned>(round(clr.b()))};
				line[h] = color{r, g, b}.px_color();
				//line[h] = hsl2rgb(h, static_cast<double>(s)/100, .50).px_color();
			}
		}
	}

	hsl_picker::hsl_color_t hsl_picker::rgb2hsl(double r, double g, double b)
	{
		using std::max, std::min;

		r /= 255; g /= 255; b /= 255;
		double	max_color {max(max(r, g), b)},
			min_color {min(min(r, g), b)},
			h {0},
			s {0},
			l {(max_color + min_color) / 2};

		if(max_color != min_color)
		{
			if(l <= .5)
				s = (max_color - min_color) / (max_color + min_color);
			else s = (max_color - min_color) / (2.0 - max_color - min_color);

			if(max_color == r)
				h = (g - b) / (max_color - min_color);
			else if(max_color == g)
				h = 2.0 + (b - r) / (max_color - min_color);
			else if(max_color == b)
				h = 4.0 + (r - g) / (max_color - min_color);
			h *= 60;
			if(h < 0)
				h += 360;
		}
		return {round(h), round(s*100), round(l*100)};
	}

	color hsl_picker::hsl2rgb(double h, double s, double l)
	{
		double r, g, b;
		if(s == 0.0)
		{
			r = l * 255.0;
			g = r;
			b = r;
		}
		else
		{
			double t1;
			if(l < .5)
				t1 = l * (1.0 + s);
			else t1 = l + s - l * s;
			double t2 {2 * l - t1};
			h /= 360;

			auto h2rgb = [&](double val)
			{
				if(val < 0.0) val += 1.0;
				else if(val > 1.0) val -= 1.0;
				if(6.0 * val < 1.0)
					return t2 + (t1 - t2) * 6 * val;
				if(2 * val < 1.0)
					return t1;
				if(3 * val < 2.0)
					return t2 + (t1 - t2) * (.666 - val) * 6;
				return t2;
			};

			r = h2rgb(h + .333);
			g = h2rgb(h);
			b = h2rgb(h - .333);
		}
		return {static_cast<unsigned>(round(r*255)), static_cast<unsigned>(round(g*255)), static_cast<unsigned>(round(b*255))};
	}

	void hsl_picker::update_lightness()
	{
		color clr {static_cast<color_argb>(pixbuf[map_selpos.y][map_selpos.x].value)};
		if(lbuf[50] != clr)
		{
			auto sat {static_cast<double>(map_selpos.y)/100};
			for(int n {0}; n<101; n++)
			{
				lbuf[n].from_hsl(map_selpos.x, sat, static_cast<double>(n)/100);
				//lbuf[n] = hsl2rgb(map_selpos.x, sat, static_cast<double>(n)/100);
				auto r {static_cast<unsigned>(round(lbuf[n].r()))};
				auto g {static_cast<unsigned>(round(lbuf[n].g()))};
				auto b {static_cast<unsigned>(round(lbuf[n].b()))};
				lbuf[n] = {r, g, b};
			}
			if(API::is_window(*this))
				API::refresh_window(*this);
		}
	}

	hsl_picker::~hsl_picker()
	{

	}
}