#include "../gui.hpp"
#include <codecvt>


void GUI::fm_playlist()
{
	using widgets::theme;
	using std::string;
	using namespace nana;

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize>{}};
	fm.caption(title);
	fm.snap(conf.cbsnap);
	fm.div(R"(vert margin=20 
		<weight=25 <l_title> <btnall weight=100> <weight=20> <btnnone weight=120>> <weight=20> 
		<range_row weight=25 <l_first weight=36> <weight=10> <tbfirst weight=45> <weight=10> <slfirst> <weight=20> 
			<l_last weight=36> <weight=10> <tblast weight=45> <weight=10> <sllast> <weight=20> <btnrange weight=120>>
		<range_row_spacer weight=20> <lbv> <weight=20> 
		<weight=35 <> <btnclose weight=100> <>>
	)");

	auto &bottom {bottoms.current()};
	::widgets::Title l_title {fm, bottom.is_bcplaylist ? "Select which songs to download from the album" :
		"Select which videos to download from the playlist"};
	if(bottom.is_scplaylist)
		l_title.caption("Select which items to download from the playlist");
	::widgets::Button btnall {fm, "Select all", true}, btnnone {fm, "Select none", true}, btnclose {fm, "Close"},
		btnrange {fm, "Select range", true};
	::widgets::Listbox lbv {fm, nullptr, true};
	::widgets::Label l_first {fm, "First:"}, l_last {fm, "Last:"};
	::widgets::Textbox tbfirst {fm}, tblast {fm};
	::widgets::Slider slfirst {fm}, sllast {fm};

	fm["l_title"] << l_title;
	fm["btnall"] << btnall;
	fm["btnnone"] << btnnone;
	fm["lbv"] << lbv;
	fm["l_first"] << l_first;
	fm["l_last"] << l_last;
	fm["tbfirst"] << tbfirst;
	fm["tblast"] << tblast;
	fm["slfirst"] << slfirst;
	fm["sllast"] << sllast;
	fm["btnrange"] << btnrange;
	fm["btnclose"] << btnclose;

	int min {1}, max {static_cast<int>(bottom.playlist_selection.size())};

	slfirst.maximum(max - 1);
	slfirst.vernier([&](unsigned, unsigned val) { return std::to_string(val < sllast.value() ? val + 1 : sllast.value()); });
	sllast.maximum(max - 1);
	sllast.vernier([&](unsigned, unsigned val) { return std::to_string(val > slfirst.value() ? val + 1 : slfirst.value() + 2); });
	slfirst.events().value_changed([&]
	{
		if(slfirst.value() >= sllast.value())
			slfirst.value(sllast.value() - 1);
		else tbfirst.caption(std::to_string(slfirst.value() + 1));
	});
	sllast.events().value_changed([&]
	{
		if(sllast.value() <= slfirst.value())
			sllast.value(slfirst.value() + 1);
		else tblast.caption(std::to_string(sllast.value() + 1));
	});
	slfirst.events().click([&] { tbfirst.caption(std::to_string(slfirst.value() + 1)); });
	sllast.events().click([&] { tblast.caption(std::to_string(sllast.value() + 1)); });
	sllast.value(sllast.maximum());

	l_title.typeface(paint::font_info {"Arial", 14, {800}});
	l_title.text_align(align::left, align_v::center);
	tbfirst.caption("1");
	tbfirst.text_align(align::center);
	tbfirst.typeface(nana::paint::font_info {"Segoe UI", 11});
	tblast.caption(std::to_string(max));
	tblast.text_align(align::center);
	tblast.typeface(nana::paint::font_info {"Segoe UI", 11});


	lbv.sortable(false);
	lbv.checkable(true);
	lbv.enable_single(true, false);
	lbv.typeface(paint::font_info {"Calibri", 12});
	lbv.scheme().item_height_ex = 8;
	lbv.append_header("", dpi_scale(25));
	lbv.append_header("#", dpi_scale(45));
	lbv.append_header(bottom.is_bcplaylist || bottom.is_scplaylist ? "Song title" : "Video title", dpi_scale(theme::is_dark() ? 743 : 739));
	lbv.append_header("Duration", dpi_scale(75));
	lbv.column_movable(false);
	lbv.column_resizable(false);

	int cnt {0}, idx {1};
	for(auto entry : bottom.playlist_info["entries"])
	{
		int dur {entry["duration"] == nullptr ? 0 : int{entry["duration"]}};
		int hr {(dur / 60) / 60}, min {(dur / 60) % 60}, sec {dur % 60};
		if(dur < 60) sec = dur;
		std::string durstr {"---"};
		if(dur)
		{
			std::stringstream ss;
			ss.width(2);
			ss.fill('0');
			if(hr)
			{
				ss << min;
				durstr = std::to_string(hr) + ':' + ss.str();
				ss.str("");
				ss.width(2);
				ss.fill('0');
			}
			else durstr = std::to_string(min);
			ss << sec;
			durstr += ':' + ss.str();
		}

		string title {entry["title"]};
		if(!is_utf8(title))
		{
			std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> u16conv;
			auto u16str {u16conv.from_bytes(title)};
			std::wstring wstr(u16str.size(), L'\0');
			memcpy(&wstr.front(), &u16str.front(), wstr.size() * 2);
			std::wstring_convert<std::codecvt_utf8<wchar_t>> u8conv;
			title = u8conv.to_bytes(wstr);
		}
		lbv.at(0).append({"", std::to_string(idx), title, durstr});
		if(!dur && !bottom.is_bcplaylist)
			bottom.playlist_selection[idx - 1] = false;
		else 
		{
			if(idx < bottom.idx_error)
				bottom.playlist_selection[idx - 1] = false;
			lbv.at(0).back().check(bottom.playlist_selection[idx - 1]);
		}
		idx++;
	}

	btnclose.events().click([&] {fm.close(); });

	lbv.events().selected([&](const arg_listbox &arg)
	{
		if(arg.item.selected())
			arg.item.check(!arg.item.checked()).select(false);
	});

	lbv.events().checked([&](const arg_listbox &arg)
	{
		bottom.playlist_selection[arg.item.to_display().item] = arg.item.checked();
	});

	btnall.events().click([&]
	{
		for(auto item : lbv.at(0))
			item.check(true);
	});

	btnnone.events().click([&]
	{
		for(auto item : lbv.at(0))
			item.check(false);
	});

	btnrange.events().click([&]
	{
		const auto a {slfirst.value()}, b {sllast.value()};
		const auto cat {lbv.at(0)};
		const auto catsize {cat.size()};
		for(auto n {0}; n < a; n++)
			cat.at(n).check(false);
		for(auto n {a}; n <= b; n++)
			cat.at(n).check(true);
		for(auto n {b + 1}; n < catsize; n++)
			cat.at(n).check(false);
	});

	tbfirst.set_accept([&](char c)
	{
		auto sel {tbfirst.selection()};
		return c == keyboard::backspace || c == keyboard::del || (isdigit(c) &&
			(sel.second.x || tbfirst.text().size() < std::to_string(max).size()));
	});

	tbfirst.events().focus([&](const arg_focus &arg)
	{
		if(!arg.getting)
		{
			if(tbfirst.text().empty() || tbfirst.to_int() >= sllast.value() + 1 || tbfirst.text() == "0")
				tbfirst.caption(std::to_string(slfirst.value() + 1));
		}
	});

	tbfirst.events().text_changed([&]
	{
		if(tbfirst == api::focus_window())
		{
			auto text {tbfirst.text()};
			if(!text.empty() && text[0] == '0')
			{
				text.erase(0, 1);
				if(!text.empty())
				{
					tbfirst.caption(text);
					return;
				}
			}
			if(!tbfirst.text().empty() && tbfirst.to_int() < sllast.value() + 1 && tbfirst.text() != "0")
				slfirst.value(stoi(tbfirst.caption()) - 1);
		}
	});

	tblast.set_accept([&](char c)
	{
		auto sel {tblast.selection()};
		return c == keyboard::backspace || c == keyboard::del || (isdigit(c) &&
			(sel.second.x || tblast.text().size() < std::to_string(max).size()));
	});

	tblast.events().focus([&](const arg_focus &arg)
	{
		if(!arg.getting)
		{
			if(tblast.text().empty() || tblast.to_int() <= slfirst.value() + 1 || tblast.to_int() > max + 1)
				tblast.caption(std::to_string(sllast.value() + 1));
		}
	});

	tblast.events().text_changed([&]
	{
		if(tblast == api::focus_window())
		{
			auto text {tblast.text()};
			if(!text.empty() && text[0] == '0')
			{
				text.erase(0, 1);
				if(!text.empty())
				{
					tblast.caption(text);
					return;
				}
			}
			if(!tblast.text().empty() && tblast.to_int() > slfirst.value() + 1 && tblast.to_int() < max + 1)
				sllast.value(stoi(tblast.caption()) - 1);
		}
	});

	fm.events().unload([&]
	{
		const auto sel {bottom.playlist_selected()};
		if(0 < sel && sel < bottom.playlist_selection.size())
		{
			std::wstring str;
			bool stopped {true};
			const auto &playsel {bottom.playlist_selection};
			const auto size {playsel.size()};
			for(int n {0}, b {0}, e {0}; n < size; n++)
			{
				const auto val {playsel[n]};
				if(val)
				{
					if(stopped)
					{
						stopped = false;
						b = n;
					}
					if(n == size - 1)
					{
						e = n;
						stopped = true;
					}
				}
				else if(!stopped)
				{
					stopped = true;
					e = n - 1;
				}

				if(stopped && b != -1)
				{
					if(e == b)
					{
						if(playsel[b])
							str += std::to_wstring(b + 1) + L',';
					}
					else str += std::to_wstring(b + 1) + L':' + std::to_wstring(e + 1) + L',';
					b = e = -1;
				}
			}
			bottom.playsel_string = str.substr(0, str.size() - 1);
		}
		else
		{
			bottom.playlist_selection.assign(bottom.playlist_info["playlist_count"], true);
			bottom.playsel_string.clear();
		}
	});

	fm.theme_callback([&](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme::fmbg);
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	auto range_row_visible {bottom.playlist_selection.size() >= 15};
	if(!range_row_visible)
	{
		fm.get_place().field_display("range_row", false);
		fm.get_place().field_display("range_row_spacer", false);
	}

	nana::paint::graphics g {{100, 100}};
	g.typeface(lbv.typeface());
	const auto text_height {g.text_extent_size("mm").height};
	const auto item_height {text_height + lbv.scheme().item_height_ex};
	fm.center(950, std::max(212.0 - !range_row_visible * 45 + lbv.item_count() * item_height, 0.0));
	if(lbv.at(0).size() * item_height < lbv.size().height - 26)
		lbv.column_at(2).width(lbv.column_at(2).width() + dpi_scale(16));

	fm.modality();
}