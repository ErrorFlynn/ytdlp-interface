#include "progress_ex.hpp"
#include "types.hpp"


bool nana::progress_ex::create(window parent, const rectangle &r, bool visible, nana::form *pform)
{
	main_thread_id = std::this_thread::get_id();
	parent_form = pform;
	if(pform)
		parent_hwnd = reinterpret_cast<HWND>(pform->native_handle());

	if(progress::create(parent, r, visible))
	{
		typeface(paint::font_info {typeface().name(), typeface().size(), paint::font::font_style{800}});
		presets[color_presets::nana] = {
			scheme().lower_foreground, scheme().foreground, scheme().background, scheme().lower_background
		};

		drawing(*this).draw([this](paint::graphics &graph)
		{
			if(!img.empty() && !graph.empty())
			{
				const double unit {static_cast<double>(amount()) / static_cast<double>(graph.width() - 2)};
				unsigned pixels {static_cast<unsigned>(static_cast<double>(value()) / unit)};
				if(value() == amount()) pixels = graph.width();
				paint::graphics g {{pixels + 1, graph.height() - 2}};
				if(img.alpha())
					g.bitblt({0, 0, pixels + 1, graph.height()}, graph, {0, 1});
				unsigned img_h {graph.height() - 2}, img_w {img.size().width};
				if(img_h > img.size().height)
					img_w += img_h - img.size().height;

				for(int x {0}; x < g.width(); x += img_w)
					img.stretch(rectangle {img.size()}, g, rectangle {x, 0, img_w, img_h});
				graph.bitblt({1, 1, pixels, g.height()}, g);
				if(darkbg)
					graph.frame_rectangle(rectangle {graph.size()}, color {"#777"}, 0);
				auto width {graph.width()}, height {graph.height()};
				auto hdc {reinterpret_cast<HDC>(const_cast<void *>(graph.context()))};
				if(shadow_amount)
				{
					const double unit {static_cast<double>(shadow_amount) / static_cast<double>(width - 2)};
					const unsigned pixels {static_cast<unsigned>(static_cast<double>(shadow_value) / unit) + 2};
					for(int y {1}; y < height - 1; y++)
					{
						for(int x {1}; x < pixels; x++)
						{
							COLORREF color = GetPixel(hdc, x, y);
							BYTE red {GetRValue(color)};
							BYTE green {GetGValue(color)};
							BYTE blue {GetBValue(color)};

							if(darkbg)
							{
								red += 20;
								green += 20;
								blue += 20;
							}
							else
							{
								red -= 15;
								green -= 15;
								blue -= 15;
							}

							SetPixel(hdc, x, y, RGB(red, green, blue));
						}
					}
				}
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
					render_text(graph, std::to_string(value() / (amount() / 100)) + '%');
				break;
			}
		});

		return true;
	}
	return false;
}


nana::widget& nana::progress_ex::caption(std::string utf8)
{
	if(std::this_thread::get_id() == main_thread_id || !parent_hwnd)
	{
		text = utf8;
		API::refresh_window(*this);
		progress::caption(utf8);
	}
	else SendMessage(parent_hwnd, WM_PROGEX_CAPTION, reinterpret_cast<WPARAM>(this), reinterpret_cast<LPARAM>(&utf8));
	return *this;
}


nana::widget &nana::progress_ex::caption(std::wstring wstr)
{
	if(std::this_thread::get_id() == main_thread_id || !parent_hwnd)
	{
		text = charset {wstr}.to_bytes(unicode::utf8);
		API::refresh_window(*this);
		progress::caption(wstr);
	}
	else 
	{
		auto utf8 {charset {wstr}.to_bytes(unicode::utf8)};
		SendMessage(parent_hwnd, WM_PROGEX_CAPTION, reinterpret_cast<WPARAM>(this), reinterpret_cast<LPARAM>(&utf8));
	}
	return *this;
}


unsigned nana::progress_ex::value(unsigned val)
{
	if(std::this_thread::get_id() == main_thread_id || !parent_hwnd)
		return progress::value(val);
	else
	{
		SendMessage(parent_hwnd, WM_PROGEX_VALUE, reinterpret_cast<WPARAM>(this), val);
		return progress::value();
	}
}


void nana::progress_ex::render_text(paint::graphics &graph, const std::string &text)
{
	auto tsize {graph.text_extent_size(text)};
	auto gsize {graph.size()};
	nana::point pos {0,0};
	if(tsize.width < gsize.width)
		pos.x = gsize.width / 2 - tsize.width / 2;
	if(tsize.height < gsize.height)
		pos.y = gsize.height / 2 - tsize.height / 2;

	if(contrast && !unknown())
	{
		double unit = static_cast<double>(amount()) / static_cast<double>(graph.width() - 2);
		unsigned pixels = static_cast<double>(value()) / unit;

		if(pixels < pos.x)
			graph.string(pos, text, clr_right);
		else if(pixels > pos.x + tsize.width)
		{
			graph.string({pos.x - 1, pos.y - 1}, text, clr_outline);
			graph.string({pos.x - 1, pos.y + 1}, text, clr_outline);
			graph.string({pos.x + 1, pos.y - 1}, text, clr_outline);
			graph.string({pos.x + 1, pos.y + 1}, text, clr_outline);
			graph.string(pos, text, clr_left);
		}
		else
		{
			paint::graphics left {graph.size()}, right {graph.size()};
			graph.paste(left, 0, 0);
			graph.paste(right, 0, 0);

			left.typeface(typeface());
			right.typeface(typeface());

			left.string({pos.x - 1, pos.y - 1}, text, clr_outline);
			left.string({pos.x - 1, pos.y + 1}, text, clr_outline);
			left.string({pos.x + 1, pos.y - 1}, text, clr_outline);
			left.string({pos.x + 1, pos.y + 1}, text, clr_outline);
			left.string(pos, text, clr_left);
			right.string(pos, text, clr_right);

			graph.bitblt({0, 0, pixels + 1, graph.height()}, left);
			graph.bitblt({static_cast<int>(pixels) + 1, 0, graph.width() - 2 - pixels, graph.height()},
				right, {static_cast<int>(pixels) + 1, 0});
		}
	}
	else graph.string(pos, text, clr_normal);
}