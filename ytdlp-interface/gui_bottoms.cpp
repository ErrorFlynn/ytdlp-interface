#include "gui.hpp"


GUI::gui_bottom &GUI::gui_bottoms::at(int idx)
{
	int i {0};
	for(auto &key : insertion_order)
		if(i++ == idx)
			return *bottoms.at(key);
	return *bottoms.begin()->second;
}


auto GUI::gui_bottoms::url_at(int idx)
{
	int i {0};
	for(auto &key : insertion_order)
		if(i++ == idx)
			return key;
	return bottoms.begin()->first;
}


GUI::gui_bottom *GUI::gui_bottoms::back() const
{
	if(!insertion_order.empty())
		return bottoms.at(insertion_order.back()).get();
	return nullptr;
}


void GUI::gui_bottoms::show(std::wstring key)
{
	// first show the target, and only then hide the others, to avoid flickering

	for(auto &bottom : bottoms)
	{
		if(bottom.first == key)
		{
			bottom.second->show();
			bottom.second->btndl.focus();
			break;
		}
	}

	for(auto &bottom : bottoms)
		if(bottom.first != key)
			bottom.second->hide();

	gui->get_place().collocate();
}


std::wstring GUI::gui_bottoms::visible()
{
	for(auto &bottom : bottoms)
		if(bottom.second->visible())
			return bottom.first;
	return L"";
}


GUI::gui_bottom &GUI::gui_bottoms::add(std::wstring url, bool visible)
{
	if(url.empty() && !bottoms.empty())
		bottoms.clear();
	auto it {bottoms.find(url)};
	if(it == bottoms.end())
	{
		auto &pbot {bottoms[url] = std::make_unique<gui_bottom>(*gui, visible)};
		insertion_order.push_back(url);
		gui->get_place()["Bottom"].fasten(*pbot);
		pbot->index = insertion_order.size() - 1;
		pbot->url = url;
		pbot->is_ytlink = gui->is_ytlink(url);
		pbot->is_ytplaylist = pbot->is_ytlink && (url.find(L"?list=") != -1 || url.find(L"&list=") != -1);
		pbot->is_ytchan = gui->is_ytchan(url);
		pbot->is_bcplaylist = url.find(L"bandcamp.com/album/") != -1;
		pbot->is_scplaylist = gui->is_scplaylist(url);
		pbot->is_bclink = url.find(L"bandcamp.com") != -1;
		pbot->is_bcchan = url.find(L".bandcamp.com/music") != -1 || url.rfind(L".bandcamp.com") == url.size() - 13
			|| url.rfind(L".bandcamp.com/") == url.size() - 14;
		if(gui->conf.gpopt_hidden && !gui->no_draw_freeze)
		{
			pbot->expcol.operate(true);
			pbot->plc.field_display("gpopt", false);
			pbot->plc.field_display("gpopt_spacer", false);
			pbot->plc.collocate();
		}
		return *pbot;
	}
	return *it->second;
}


void GUI::gui_bottoms::erase(std::wstring url)
{
	if(bottoms.contains(url))
	{
		insertion_order.remove(url);
		gui->get_place().erase(*bottoms.at(url));
		bottoms.erase(url);
	}
}


void GUI::gui_bottoms::propagate_cb_options(const gui_bottom &srcbot)
{
	for(auto &pbot : *this)
	{
		auto &bot {*pbot.second};
		if(bot.handle() != srcbot.handle())
		{
			bot.cbargs.check(srcbot.cbargs.checked());
			bot.cbkeyframes.check(srcbot.cbkeyframes.checked());
			bot.cbmp3.check(srcbot.cbmp3.checked());
			bot.cbsubs.check(srcbot.cbsubs.checked());
			bot.cbthumb.check(srcbot.cbthumb.checked());
			bot.cbtime.check(srcbot.cbtime.checked());
		}
	}
}


void GUI::gui_bottoms::propagate_args_options(const gui_bottom &srcbot)
{
	for(auto &pbot : *this)
	{
		auto &bot {*pbot.second};
		if(bot.handle() != srcbot.handle())
		{
			bot.com_args.clear();
			if(srcbot.com_args.the_number_of_options())
			{
				for(int n {0}; n < srcbot.com_args.the_number_of_options(); n++)
					bot.com_args.push_back(srcbot.com_args.text(n));
			}
			bot.com_args.caption(srcbot.com_args.caption());
		}
	}
}


void GUI::gui_bottoms::propagate_misc_options(const gui_bottom &srcbot)
{
	for(auto &pbot : *this)
	{
		auto &bot {*pbot.second};
		if(bot.handle() != srcbot.handle())
		{
			bot.com_chap.option(srcbot.com_chap.option());
			bot.com_rate.option(srcbot.com_rate.option());
			bot.tbrate.caption(srcbot.tbrate.caption());
			bot.outpath = srcbot.outpath;
			bot.l_outpath.update_caption();
		}
	}
}