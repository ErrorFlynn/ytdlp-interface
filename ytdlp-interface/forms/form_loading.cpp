#include "../gui.hpp"


void GUI::fm_loading()
{
	using widgets::theme;
	using namespace nana;

	themed_form fm {nullptr, *this, {}, appear::bald{}};
	fm.center(400, 110);
	fm.escape_key(false);

	const auto hwnd {fm.native_handle()};
	SetWindowLongPtrA(hwnd, GWL_STYLE, GetWindowLongPtrA(hwnd, GWL_STYLE) | WS_BORDER);
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	fm.div("vert <weight=20> <title weight=30> <text> <weight=10>");

	::widgets::Title title {fm, "Loading data for queue items"};
	::widgets::Text text {fm, "Please wait..."};
	fm["title"] << title;
	fm["text"] << text;

	text.text_align(align::center, align_v::center);

	fm.theme_callback([&](bool dark)
	{
		fm.bgcolor(theme::fmbg.blend(dark ? colors::white : colors::grey, .04));
		fm.refresh_widgets();
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.collocate();
	fm.bring_top(true);

	thr_qitem_data = std::thread {[&]
	{
		bool good {true};
		try { std::ifstream {infopath} >> unfinished_qitems_data; }
		catch(nlohmann::detail::exception e)
		{
			good = false;
			unfinished_qitems_data.clear();
			fm.close();
		}
		std::error_code ec;
		fs::remove(infopath, ec);
		if(good)
			title.caption("Recreating the queue");
		lbq.auto_draw(false);
		init_qitems();
		lbq.auto_draw(true);
		if(thr_qitem_data.joinable())
			thr_qitem_data.detach();
		if(good)
			fm.close();
	}};
	fm.modality();
}