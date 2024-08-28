#include "../gui.hpp"


void GUI::fm_suspend()
{
	using widgets::theme;
	using std::string;
	using namespace nana;

	themed_form fm {nullptr, *this};
	fm.caption(title + " - queue finished");
	fm.snap(conf.cbsnap);
	fm.center(400, 170);

	fm.div("vert margin=20 <title weight=30> <weight=5> <countdown> <weight=35 <><btn weight=100><>>");

	::widgets::Title l_title {fm}, l_countdown {fm};
	fm["title"] << l_title;
	fm["countdown"] << l_countdown;

	if(pwr_shutdown)
		l_title.caption(pwr_can_shutdown ? "shutting down in:" : "attempting to shut down in:");
	else if(pwr_hibernate)
		l_title.caption(pwr_can_shutdown ? "hibernating in:" : "attempting to hibernate in:");
	else if(pwr_sleep)
		l_title.caption(pwr_can_shutdown ? "sleeping in:" : "attempting to sleep in:");

	int cnt {15};
	l_countdown.caption(std::to_string(cnt));
	auto finfo {l_countdown.typeface().info().value()};
	finfo.size_pt += 12;
	l_countdown.typeface(finfo);

	timer t;
	t.interval(std::chrono::milliseconds{1000});
	t.elapse([&]
	{
		auto errmsg = [&](std::string action)
		{
			::widgets::msgbox mbox {fm, title};
			mbox << "The attempt to " << action << " has failed. Error message from the system:\n\n" << util::get_last_error_str();
			mbox.icon(MB_ICONERROR)();
		};

		l_countdown.caption(std::to_string(--cnt));
		if(cnt == 0)
		{
			t.stop();
			if(pwr_shutdown)
			{
				pwr_shutdown = false;
				if(!util::pwr_shutdown())
					errmsg("shutdown");
			}
			else if(pwr_hibernate)
			{
				pwr_hibernate = false;
				if(!util::pwr_hibernate())
					errmsg("hibernate");
			}
			else if(pwr_sleep)
			{
				pwr_sleep = false;
				if(!util::pwr_hibernate())
					errmsg("sleep");
			}
			fm.close();
		}
	});

	::widgets::Button btn {fm, "Cancel"};
	fm["btn"] << btn;

	btn.events().click([&] {fm.close();});
	btn.focus();

	fm.theme_callback([&](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme::fmbg);
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);
	
	t.start();
	fm.collocate();
	fm.bring_top(true);
	fm.modality();
}