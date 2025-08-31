#include "../gui.hpp"


std::unique_ptr<themed_form> GUI::fm_alert(std::string main, std::string sub, bool show)
{
	using widgets::theme;
	using namespace nana;

	std::unique_ptr<themed_form> pfm {new themed_form {nullptr, *this, {}, appear::bald{}}};
	themed_form &fm {*pfm};
	fm.center(456, 110);
	fm.escape_key(false);
	fm.subclass_umake_before(WM_MOVING);
	fm.subclass_umake_before(WM_ENTERSIZEMOVE);

	const auto hwnd {fm.native_handle()};
	SetWindowLongPtrA(hwnd, GWL_STYLE, GetWindowLongPtrA(hwnd, GWL_STYLE) | WS_BORDER);
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
	fm.div("vert <weight=20> <title weight=30> <text> <weight=10>");

	if(!fm_alert_title.empty())
		fm_alert_title.close();
	fm_alert_title.create(fm, main);

	if(!fm_alert_text.empty())
		fm_alert_text.close();
	fm_alert_text.create(fm, sub);

	fm["title"] << fm_alert_title;
	fm["text"] << fm_alert_text;

	fm_alert_text.text_align(align::center, align_v::center);

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
	fm.activate();
	if(show) fm.show();

	return pfm;
}