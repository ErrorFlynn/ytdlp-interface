#include "gui.hpp"


unsigned GUI::gui_bottoms::joinable_dl_threads() const
{
	unsigned count {0};
	for(const auto &bot : bottoms)
		if(bot.second->dl_thread.joinable())
			++count;
	return count;
}


unsigned GUI::gui_bottoms::joinable_info_threads(bool active) const
{
	unsigned count {0};
	for(const auto &bot : bottoms)
	{
		if(active)
		{
			if(bot.second->info_thread_active)
				++count;
		}
		else if(bot.second->info_thread.joinable())
			++count;
	}
	return count;
}


unsigned GUI::gui_bottoms::total_threads() const
{
	return joinable_dl_threads() + joinable_info_threads();
}


int GUI::gui_bottoms::index(std::wstring url) const
{
	int idx {0};
	for(const auto &url_ : insertion_order)
	{
		if(url_ == url) return idx;
		++idx;
	}
	return -1;
}


GUI::gui_bottom &GUI::gui_bottoms::at(int idx)
{
	int i {0};
	for(const auto &key : insertion_order)
		if(i++ == idx)
			return *bottoms.at(key);
	return *bottoms.begin()->second;
}


GUI::gui_bottom &GUI::gui_bottoms::at(int idx) const
{
	int i {0};
	for(const auto &key : insertion_order)
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
	
	auto &bot {*bottoms.at(key)};
	gui->prog.caption(bot.progtext);
	gui->prog.amount(bot.prog_amount);
	gui->prog.value(bot.progval);
	gui->prog.shadow_progress(1000, bot.progval_shadow);
	gui->bot_showing = true;
	gui->btndl.enable(index(key) > 0);
	if(!conf.common_dl_options)
		gui->l_outpath.source_path(&bot.outpath);
	gui->qurl = bot.url;
	gui->l_url.update_caption();
	gui->tbrate.caption(bot.rate);
	gui->com_rate.option(bot.ratelim_unit);
	gui->cbtime.check(bot.cbtime);
	gui->cbsubs.check(bot.cbsubs);
	gui->com_chap.option(bot.com_chap);
	gui->cbkeyframes.check(bot.cbkeyframes);
	gui->cbthumb.check(bot.cbthumb);
	gui->cbmp3.check(bot.cbmp3);
	gui->cbargs.check(bot.cbargs);
	gui->com_args.caption(bot.argset);
	gui->bot_showing = false;
	if(conf.common_dl_options)
		gui->gpopt.caption("Download options");
	else
	{
		auto item {gui->lbq.item_from_value(key)};
		if(item != gui->lbq.empty_item)
			gui->gpopt.caption("Download options for queue item #" + item.text(0));
	}
	nana::api::refresh_window(gui->gpopt);
	if(bot.outfile.empty())
		gui->l_outpath.tooltip("");
	else
	{
		gui->l_outpath.tooltip("Custom file name:\n<bold>" + bot.outfile.filename().string() +
			"</>\n(this overrides the output template from the settings)");
	}
	gui->show_btnfmt(!bot.vidinfo.empty());
	if(!conf.common_dl_options && gui->lbq.item_count() > 1)
		gui->show_btncopy(true);
	gui->btndl.caption(bot.started ? "Stop download" : "Start download");
}


GUI::gui_bottom &GUI::gui_bottoms::add(std::wstring url)
{
	if(url.empty() && !bottoms.empty())
		bottoms.clear();
	auto it {bottoms.find(url)};
	if(it == bottoms.end())
	{
		auto &pbot {bottoms[url] = std::make_unique<gui_bottom>(*gui)};
		insertion_order.push_back(url);
		pbot->url = url;
		pbot->is_ytlink = gui->is_ytlink(url);
		pbot->is_ytplaylist = pbot->is_ytlink && (url.find(L"?list=") != -1 || url.find(L"&list=") != -1);
		pbot->is_ytchan = gui->is_ytchan(url);
		pbot->is_bcplaylist = url.find(L"bandcamp.com/album/") != -1;
		pbot->is_scplaylist = gui->is_scplaylist(url);
		pbot->is_bclink = url.find(L"bandcamp.com") != -1;
		pbot->is_bcchan = url.find(L".bandcamp.com/music") != -1 || url.rfind(L".bandcamp.com") == url.size() - 13
			|| url.rfind(L".bandcamp.com/") == url.size() - 14;
		if(url.find(L"rutube.ru/metainfo/tv/") != -1 || url.find(L"vkvideo.ru/playlist/") != -1 )
			pbot->is_gen_playlist = true;
		return *pbot;
	}
	return *it->second;
}


void GUI::gui_bottoms::erase(std::wstring url)
{
	if(bottoms.contains(url))
	{
		insertion_order.remove(url);
		bottoms.erase(url);
	}
}


void GUI::gui_bottoms::propagate_cb_options(const gui_bottom &srcbot)
{
	for(auto &pbot : *this)
	{
		auto &bot {*pbot.second};
		if(&bot != &srcbot)
		{
			bot.cbargs = srcbot.cbargs;
			bot.cbkeyframes = srcbot.cbkeyframes;
			bot.cbmp3 = srcbot.cbmp3;
			bot.cbsubs = srcbot.cbsubs;
			bot.cbthumb = srcbot.cbthumb;
			bot.cbtime = srcbot.cbtime;
		}
	}
}


void GUI::gui_bottoms::propagate_args_options(const gui_bottom &srcbot)
{
	for(auto &pbot : *this)
	{
		auto &bot {*pbot.second};
		if(&bot != &srcbot)
		{
			bot.argset = srcbot.argset;
		}
	}
}


void GUI::gui_bottoms::propagate_misc_options(const gui_bottom &srcbot)
{
	for(auto &pbot : *this)
	{
		auto &bot {*pbot.second};
		if(&bot != &srcbot)
		{
			bot.com_chap = srcbot.com_chap;
			bot.rate = srcbot.rate;
			bot.ratelim_unit = srcbot.ratelim_unit;
			bot.outpath = srcbot.outpath;
		}
	}
}