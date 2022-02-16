#include <nana/gui/drawing.hpp>
#include <nana/gui/widgets/progress.hpp>
#include <array>

#pragma warning( disable : 4244 4018)

const uint8_t arr_progbar_jpg[1235] = {
	255,216,255,225,0,194,69,120,105,102,0,0,73,73,42,0,8,0,0,0,7,0,18,1,3,0,1,0,0,0,1,0,0,0,26,1,5,0,1,0,0,0,98,0,0,0,27,1,
	5,0,1,0,0,0,106,0,0,0,40,1,3,0,1,0,0,0,2,0,0,0,49,1,2,0,14,0,0,0,114,0,0,0,50,1,2,0,20,0,0,0,128,0,0,0,105,135,4,0,1,0,
	0,0,148,0,0,0,112,101,103,124,72,0,0,0,1,0,0,0,72,0,0,0,1,0,0,0,80,104,111,116,111,70,105,108,116,114,101,32,55,0,50,48,
	50,49,58,49,50,58,49,49,32,50,50,58,51,56,58,50,51,0,3,0,0,144,7,0,4,0,0,0,48,50,49,48,2,160,3,0,1,0,0,0,28,0,0,0,3,160,
	3,0,1,0,0,0,40,0,0,0,255,219,0,67,0,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,3,2,2,2,2,2,4,3,3,2,3,5,4,5,5,5,4,4,4,5,6,7,6,5,5,7,6,
	4,4,6,9,6,7,8,8,8,8,8,5,6,9,10,9,8,10,7,8,8,8,255,219,0,67,1,1,1,1,2,2,2,4,2,2,4,8,5,4,5,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
	8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,255,192,0,17,8,0,40,0,28,3,1,34,0,2,17,1,3,17,1,
	255,196,0,31,0,0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,255,196,0,181,16,0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,
	125,1,2,3,0,4,17,5,18,33,49,65,6,19,81,97,7,34,113,20,50,129,145,161,8,35,66,177,193,21,82,209,240,36,51,98,114,130,9,
	10,22,23,24,25,26,37,38,39,40,41,42,52,53,54,55,56,57,58,67,68,69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,
	103,104,105,106,115,116,117,118,119,120,121,122,131,132,133,134,135,136,137,138,146,147,148,149,150,151,152,153,154,162,
	163,164,165,166,167,168,169,170,178,179,180,181,182,183,184,185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,
	214,215,216,217,218,225,226,227,228,229,230,231,232,233,234,241,242,243,244,245,246,247,248,249,250,255,196,0,31,1,0,3,
	1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,255,196,0,181,17,0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,119,0,1,2,3,17,4,5,
	33,49,6,18,65,81,7,97,113,19,34,50,129,8,20,66,145,161,177,193,9,35,51,82,240,21,98,114,209,10,22,36,52,225,37,241,23,
	24,25,26,38,39,40,41,42,53,54,55,56,57,58,67,68,69,70,71,72,73,74,83,84,85,86,87,88,89,90,99,100,101,102,103,104,105,
	106,115,116,117,118,119,120,121,122,130,131,132,133,134,135,136,137,138,146,147,148,149,150,151,152,153,154,162,163,164,
	165,166,167,168,169,170,178,179,180,181,182,183,184,185,186,194,195,196,197,198,199,200,201,202,210,211,212,213,214,215,
	216,217,218,226,227,228,229,230,231,232,233,234,242,243,244,245,246,247,248,249,250,255,218,0,12,3,1,0,2,17,3,17,0,63,0,
	253,61,255,0,132,19,70,255,0,159,173,71,243,79,240,165,95,2,233,109,113,103,24,189,212,136,50,169,96,74,242,163,230,35,
	167,251,53,149,255,0,11,47,194,159,244,30,182,255,0,190,27,255,0,137,173,77,23,199,30,28,212,110,46,39,183,213,109,230,
	142,21,9,157,173,195,55,61,199,162,254,181,250,188,177,88,148,174,238,126,95,12,46,29,190,135,93,173,105,22,173,167,93,
	67,246,187,196,121,215,201,4,109,4,110,224,246,244,201,174,75,254,16,109,39,181,230,169,143,247,147,255,0,137,168,245,
	223,136,94,25,181,188,178,180,184,214,109,162,112,141,57,27,88,255,0,178,59,127,191,89,103,226,103,132,199,252,199,237,
	199,252,1,255,0,194,162,141,108,74,94,237,203,171,71,14,222,182,62,28,58,162,143,226,35,252,253,107,218,60,9,41,182,240,
	229,181,193,56,146,233,218,231,234,164,225,127,241,213,95,206,190,66,26,139,93,201,13,148,5,188,249,228,88,35,255,0,121,
	136,81,252,235,233,253,67,88,131,195,186,21,213,218,224,91,89,90,150,65,142,161,23,10,63,28,1,248,215,161,141,196,59,40,
	163,207,193,209,87,114,103,25,226,79,16,139,255,0,17,106,243,172,155,226,73,5,178,28,246,65,180,255,0,227,219,235,23,
	251,93,63,191,250,215,147,197,171,176,69,50,179,52,164,101,206,122,177,228,159,207,52,255,0,237,113,253,231,252,235,174,
	21,44,146,57,37,27,187,152,95,15,46,198,163,226,235,23,103,13,13,164,114,93,177,200,198,224,54,40,252,223,63,240,26,244,
	79,139,62,38,142,211,195,182,250,114,76,55,94,92,164,109,135,255,0,150,105,251,198,253,85,7,227,69,21,225,84,155,117,
	146,103,187,74,10,52,91,71,131,127,110,129,255,0,45,115,248,138,79,237,225,255,0,61,27,244,162,138,244,57,217,194,169,
	35,255,217
};

namespace nana
{
	class progress_ex : public progress
	{
	public:

		enum class text_modes { caption, percentage, value };
		enum class color_presets { nana, blue, green, orange, violet, brown };

		bool create(window parent, bool visible = true)
		{
			return create(parent, rectangle{}, visible);
		}

		bool create(window parent, const rectangle &r = {}, bool visible = true)
		{
			if(progress::create(parent, r, visible))
			{
				typeface(paint::font_info{typeface().name(), typeface().size(), paint::font::font_style{800}});
				presets[color_presets::nana] = {
					scheme().lower_foreground, scheme().foreground, scheme().background, scheme().lower_background
				};

				drawing(*this).draw([this](paint::graphics &graph)
				{
					if(!img.empty() && !graph.empty())
					{
						double unit = static_cast<double>(amount()) / static_cast<double>(graph.width()-2);
						unsigned pixels = static_cast<double>(value()) / unit;
						if(value() == amount()) pixels = graph.width();
						paint::graphics g {{pixels+1, graph.height()-2}};
						if(img.alpha())
							g.bitblt({0, 0, pixels+1, graph.height()}, graph, {0, 1});
						unsigned img_h {graph.height()-2}, img_w {img.size().width};
						if(img_h > img.size().height)
							img_w += img_h - img.size().height;

						for(int x {0}; x < g.width(); x += img_w)
							img.stretch(rectangle {img.size()}, g, rectangle {x, 0, img_w, img_h});
						graph.bitblt({1, 1, pixels, g.height()}, g);
						if(darkbg)
							graph.frame_rectangle(rectangle{graph.size()}, color {"#777"}, 0);
					}

					switch(tmode)
					{
					case text_modes::caption:
						if(!text.empty())
							render_text(graph, text);
						break;

					case text_modes::value:
						if(!unknown())
							render_text(graph, std::to_string(value()) + " / " + std::to_string(amount()));
						break;

					case text_modes::percentage:
						if(!unknown())
							render_text(graph, std::to_string(value()/(amount()/100)) + '%');
						break;
					}
				});

				return true;
			}
			return false;
		}

		progress_ex() : progress() {}
		progress_ex(window parent, bool visible) { create(parent, visible); }
		progress_ex(window parent, const rectangle &r = {}, bool visible = true) { create(parent, r, visible); }

		widget& caption(std::string utf8)
		{
			text = utf8;
			API::refresh_window(*this);
			return progress::caption(utf8);
		}

		widget& caption(std::wstring wstr)
		{
			text = charset{wstr}.to_bytes(unicode::utf8);
			API::refresh_window(*this);
			return progress::caption(wstr);
		}

		void text_mode(text_modes m) { tmode = m; API::refresh_window(*this); }
		text_modes text_mode() const noexcept { return tmode; }

		// this color is used if text contrast is disabled
		void text_color(color clr) { clr_normal = clr;  API::refresh_window(*this); }
		color text_color() { return clr_normal; }
		void outline_color(color clr) { clr_outline = clr; }
		
		void text_contrast_colors(color full, color empty) { clr_left = full; clr_right = empty; API::refresh_window(*this); }
		std::pair<color, color> text_contrast_colors() { return std::make_pair(clr_left, clr_right); }

		void text_contrast(bool enable) { contrast = enable; API::refresh_window(*this); }
		bool text_contrast() { return contrast; }

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
		auto &image() { return img; }

	private:

		paint::image img;
		std::string text;
		text_modes tmode{text_modes::caption};
		color clr_left{"#ffffff"}, clr_right{"#333333"}, clr_normal{clr_right}, clr_outline;
		bool contrast {true}, darkbg {false};

		std::unordered_map<color_presets, std::array<color, 4>> presets
		{
			//                           __/top fg\__      __/bot fg\__      __/top bg\__      __/bot bg\__
			{ color_presets::blue,   { color{"#bbd7df"}, color{"#407f91"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::orange, { color{"#F2C9B5"}, color{"#CF6430"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::green,  { color{"#AFD5AA"}, color{"#508D47"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::violet, { color{"#ccc8ce"}, color{"#75617E"}, color{"#d8d6d3"}, color{"#fbf9f6"} } },
			{ color_presets::brown,  { color{"#DFC9B3"}, color{"#8B633A"}, color{"#d8d6d3"}, color{"#fbf9f6"} } }
		};

		void render_text(paint::graphics &graph, const std::string &text)
		{
			auto tsize{graph.text_extent_size(text)};
			auto gsize{graph.size()};
			nana::point pos{0,0};
			if(tsize.width < gsize.width)
				pos.x = gsize.width/2 - tsize.width/2;
			if(tsize.height < gsize.height)
				pos.y = gsize.height/2 - tsize.height/2;

			if(contrast && !unknown())
			{
				double unit = static_cast<double>(amount()) / static_cast<double>(graph.width()-2);
				unsigned pixels = static_cast<double>(value()) / unit;

				if(pixels < pos.x)
					graph.string(pos, text, clr_right);
				else if(pixels > pos.x + tsize.width)
				{
					graph.string({pos.x-1, pos.y-1}, text, clr_outline);
					graph.string({pos.x-1, pos.y+1}, text, clr_outline);
					graph.string({pos.x+1, pos.y-1}, text, clr_outline);
					graph.string({pos.x+1, pos.y+1}, text, clr_outline);
					graph.string(pos, text, clr_left);
				}
				else
				{
					paint::graphics left{graph.size()}, right{graph.size()};
					graph.paste(left, 0, 0);
					graph.paste(right, 0, 0);

					left.typeface(typeface());
					right.typeface(typeface());

					left.string({pos.x-1, pos.y-1}, text, clr_outline);
					left.string({pos.x-1, pos.y+1}, text, clr_outline);
					left.string({pos.x+1, pos.y-1}, text, clr_outline);
					left.string({pos.x+1, pos.y+1}, text, clr_outline);
					left.string(pos, text, clr_left);
					right.string(pos, text, clr_right);

					graph.bitblt({0, 0, pixels+1, graph.height()}, left);
					graph.bitblt({static_cast<int>(pixels)+1, 0, graph.width()-2-pixels, graph.height()},
						right, {static_cast<int>(pixels)+1, 0});
				}
			}
			else graph.string(pos, text, clr_normal);
		}
	};
}