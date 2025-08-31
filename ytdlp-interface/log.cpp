#include "log.hpp"

bool g_enable_log {false};
std::ofstream g_logfile;


void logger::make_form(nana::window owner)
{
	using namespace nana;
	if(!fm && g_enable_log)
	{
		fm = std::make_unique<themed_form>(nullptr, owner, rectangle {0, 200, 645, 1000}, appear::decorate<appear::minimize, appear::sizable> {});
		fm->caption("debug log");
		fm->snap(true);
		fm->div("vert margin=15 <tb>");
		tb = std::make_unique<::widgets::Textbox>(*fm);
		(*fm)["tb"] << *tb;
		fm->collocate();
		fm->theme_callback([this](bool dark)
		{
			fm->bgcolor(::widgets::theme::fmbg);
			return false;
		});
		fm->system_theme(true);
		tb->typeface(paint::font_info {"Liberation Mono", 10});
	}
}
