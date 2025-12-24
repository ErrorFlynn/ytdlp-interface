#pragma once

#include <nana/gui/drawing.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <array>
#include <thread>
#include <nana/gui.hpp>
#include <Windows.h>

#pragma warning( disable : 4244 4018)

namespace nana
{
	class progress_ex : public progress
	{
	public:

		enum class text_modes { caption, percentage, value };
		enum class color_presets { nana, blue, green, orange, violet, brown };

		bool create(window parent, bool visible = true)
		{
			return create(parent, {}, visible);
		}

		bool create(window parent, const rectangle &r = {}, bool visible = true, nana::form *pform = nullptr);

		progress_ex() : progress() {}
		progress_ex(window parent, bool visible) { create(parent, visible); }
		progress_ex(window parent, const rectangle &r = {}, bool visible = true, nana::form *pform = nullptr) { create(parent, r, visible, pform); }

		widget &caption(std::string utf8);
		widget &caption(std::wstring wstr);
		unsigned value(unsigned val);
		unsigned value() const { return progress::value(); }

		void text_mode(text_modes m) { tmode = m; API::refresh_window(*this); }
		text_modes text_mode() const noexcept { return tmode; }

		// this color is used if text contrast is disabled
		void text_color(color clr) { clr_normal = clr;  API::refresh_window(*this); }
		color text_color() const { return clr_normal; }
		void outline_color(color clr) { clr_outline = clr; }
		
		void text_contrast_colors(color full, color empty) { clr_left = full; clr_right = empty; API::refresh_window(*this); }
		std::pair<color, color> text_contrast_colors() { return std::make_pair(clr_left, clr_right); }

		void text_contrast(bool enable) { contrast = enable; API::refresh_window(*this); }
		bool text_contrast() const { return contrast; }

		void dark_bg(bool enable) { darkbg = enable; }

		// shade: [-1.0] to [1.0] (negative = darker, zero = default, positive = lighter)
		void color_preset(color_presets preset, double shade = 0)
		{
			auto colors = presets[preset];
			auto blend_color = shade < 0 ? colors::black : colors::white;
			shade = abs(shade);
			scheme().lower_foreground = colors[0].blend(blend_color, shade);
			scheme().foreground = colors[1].blend(blend_color, shade);
			scheme().background = colors[2];
			scheme().lower_background = colors[3];
		}

		void image(paint::image &i) { img = i; }
		void image(paint::image &&i) { img = i; }
		auto &image() const { return img; }
		void shadow_progress(int amount, int value) { shadow_amount = amount; shadow_value = value; }

	private:

		std::thread::id main_thread_id;
		nana::form *parent_form {nullptr};
		HWND parent_hwnd {nullptr};
		paint::image img;
		std::string text;
		text_modes tmode{text_modes::caption};
		color clr_left{"#ffffff"}, clr_right{"#333333"}, clr_normal{clr_right}, clr_outline;
		bool contrast {true}, darkbg {false};
		int shadow_amount {0}, shadow_value {0};

		std::unordered_map<color_presets, std::array<color, 4>> presets
		{
			//                           __/top fg\__      __/bot fg\__      __/top bg\__      __/bot bg\__
			{ color_presets::blue,   { color{"#bbd7df"}, color{"#407f91"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::orange, { color{"#F2C9B5"}, color{"#CF6430"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::green,  { color{"#AFD5AA"}, color{"#508D47"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::violet, { color{"#ccc8ce"}, color{"#75617E"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::brown,  { color{"#DFC9B3"}, color{"#8B633A"}, color{"#d8d6d3"}, color{"#fbf9f6"} } }
		};

		void render_text(paint::graphics &graph, const std::string &text);
	};
}