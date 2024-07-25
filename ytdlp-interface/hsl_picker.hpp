#pragma once

#include <nana/gui/element.hpp>
#include <nana/gui/widgets/widget.hpp>
#include <nana/paint/pixel_buffer.hpp>
#include <future>
#include <algorithm>

namespace nana
{
	class hsl_picker;
	namespace drawerbase
	{
		namespace hsl_picker
		{
			struct scheme : public widget_geometrics
			{
				color_proxy border {static_cast<color_rgb>(0xaaa5a0)};
			};

			class trigger : public drawer_trigger
			{
			public:
				trigger();
				~trigger();

				element::cite_bground & cite();
			private:
				void attached(widget_reference, graph_reference) override;
				void refresh(graph_reference) override;
				//void mouse_enter(graph_reference, const arg_mouse&) override;
				void mouse_down(graph_reference, const arg_mouse&) override;
				void mouse_move(graph_reference, const arg_mouse&) override;
				void mouse_up(graph_reference, const arg_mouse&) override;
				//void mouse_leave(graph_reference, const arg_mouse&) override;
				void draw_background(graph_reference);
				void draw_huesat(graph_reference);
				void draw_lightness(graph_reference);
			private:
				widget *wdg_ {nullptr};
				nana::hsl_picker *picker_ {nullptr};
				bool lightness_captured_ {false};

				element::cite_bground cite_ {"hsl_picker"};

				struct attr_tag
				{
					element_state e_state;
					::nana::color bgcolor;
					::nana::color fgcolor;
				}attr_;
			};
		}
	}

	class hsl_picker : public widget_object<category::widget_tag, drawerbase::hsl_picker::trigger, general_events, drawerbase::hsl_picker::scheme>
	{
		using base_type = widget_object<category::widget_tag, drawerbase::hsl_picker::trigger, general_events, drawerbase::hsl_picker::scheme>;
		paint::pixel_buffer pixbuf {360, 101};
		color lbuf[101]; // lightness index, each position holds a shade of the color at map_selpos
		int lbuf_selpos {50}; // position of the selection cursor within the lightness index
		point map_selpos {180, 50}; // position of the selection cursor within the HueSat map
		std::function<void(void)> cb;

	public:

		struct hsl_color_t { double h {0.0}; double s {0.0}; double l {0.0}; };
		hsl_picker();
		hsl_picker(window parent, const nana::rectangle & = rectangle());
		~hsl_picker();

		hsl_picker& set_bground(const pat::cloneable<element::element_interface>&);	///< Sets a user-defined background element.
		hsl_picker& set_bground(const std::string&);	///< Sets a pre-defined background element by a name.

		hsl_picker& transparent(bool enable);
		bool transparent() const;

		void on_change(std::function<void(void)> callback) { cb = callback; }
		void on_change() { if(cb) cb(); }

		unsigned rgb() { return lbuf[lbuf_selpos].rgba().value >> 8; }
		void rgb(unsigned clr);
		std::string rgbstr(const color *clr = nullptr);

		hsl_color_t hsl() { return {static_cast<double>(map_selpos.x), static_cast<double>(map_selpos.y), static_cast<double>(lbuf_selpos)}; }
		void hsl(hsl_color_t clr);

		int hue() const noexcept { return map_selpos.x; }
		int sat() const noexcept { return map_selpos.y; }
		int lum() const noexcept { return lbuf_selpos; }
		int red() const noexcept { return static_cast<int>(lbuf[lbuf_selpos].r()); }
		int green() const noexcept { return static_cast<int>(lbuf[lbuf_selpos].g()); }
		int blue() const noexcept { return static_cast<int>(lbuf[lbuf_selpos].b()); }

		color color_value() const noexcept { return lbuf[lbuf_selpos]; }
		void color_value(nana::color clr, bool call_back = false);
		color hsl2rgb(double h, double s, double l);
		hsl_color_t rgb2hsl(double r, double g, double b);

	private:

		friend class drawerbase::hsl_picker::trigger;


		//void init();
		void make_pixels();
		void update_lightness();
		//void _m_fgcolor(const color&) override;
		//void _m_bgcolor(const color&) override;

		const nana::size natural_size_ {362, 141};
	};
}