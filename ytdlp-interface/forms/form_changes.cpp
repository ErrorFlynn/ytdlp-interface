#include "../gui.hpp"


void GUI::fm_changes(nana::window parent)
{
	using widgets::theme;

	themed_form fm {nullptr, parent, {}, appear::decorate<appear::sizable>{}};
	fm.center(1030, 543);
	fm.snap(conf.cbsnap);
	fm.caption("ytdlp-interface - release notes");
	fm.bgcolor(theme::fmbg);
	fm.div(R"(vert margin=20 <tb> <weight=20>
				<weight=25 <> <l_history weight=164> <weight=10> <com_history weight=75> <weight=20> <cblogview weight=140> <> >)");

	widgets::Textbox tb {fm};
	{
		fm["tb"] << tb;
		nana::API::effects_edge_nimbus(tb, nana::effects::edge_nimbus::none);
		tb.typeface(nana::paint::font_info {"Consolas", 12});
		tb.line_wrapped(true);
		tb.editable(false);
	}

	widgets::cbox cblogview {fm, "Changelog view"};
	fm["cblogview"] << cblogview;

	auto display_release_notes = [&](const unsigned ver)
	{
		std::string tag_name {releases[ver]["tag_name"]};
		std::string body {releases[ver]["body"]};
		body.erase(0, body.find('-'));
		std::string date {releases[ver]["published_at"]};
		date.erase(date.find('T'));
		auto title {tag_name + " (" + date + ")\n"};
		title += std::string(title.size() - 1, '=') + "\n\n";
		tb.caption(title + body);
		tb.colored_area_access()->clear();
		nana::api::refresh_window(tb);
	};

	auto display_changelog = [&]
	{
		tb.caption("");
		auto ca {tb.colored_area_access()};
		ca->clear();
		for(const auto &el : releases)
		{
			std::string
				tag_name {el["tag_name"]},
				date {el["published_at"]},
				body {el["body"]},
				block;

			if(!body.empty())
			{
				body.erase(0, body.find('-'));
				body.erase(body.find_first_of("\r\n", body.rfind("- ")));
			}
			date.erase(date.find('T'));
			auto title {tag_name + " (" + date + ")\n"};
			title += std::string(title.size() - 1, '=') + "\n\n";
			block += title + body + "\n\n";
			if(tag_name != "v1.0.0")
			{
				block += "---------------------------------------------------------------------------------------------\n\n";
				tb.caption(tb.caption() + block);
				auto p {ca->get(tb.text_line_count() - 3)};
				p->count = 1;
				p->fgcolor = theme::is_dark() ? nana::color {"#777"} : nana::color {"#bbb"};
			}
			else tb.caption(tb.caption() + block);
		}
	};

	display_release_notes(0);

	widgets::Label l_history {fm, "Show release notes for:"};
	fm["l_history"] << l_history;

	widgets::Combox com_history {fm};
	{
		fm["com_history"] << com_history;
		com_history.editable(false);
		for(auto &release : releases)
		{
			std::string text {release["tag_name"]};
			com_history.push_back(text);
		}
		com_history.option(0);
		com_history.events().selected([&]
		{
			display_release_notes(com_history.option());
		});
	}

	cblogview.events().click([&]
	{
		if(cblogview.checked())
			display_changelog();
		else display_release_notes(com_history.option());
		com_history.enabled(!cblogview.checked());
	});

	tb.events().mouse_wheel([&](const nana::arg_wheel &arg)
	{
		if(!cblogview.checked() && !tb.scroll_operation()->visible(true) && arg.which == nana::arg_wheel::wheel::vertical)
		{
			const auto idx_sel {com_history.option()}, idx_last {com_history.the_number_of_options() - 1};
			if(arg.upwards)
			{
				if(idx_sel == 0)
					com_history.option(idx_last);
				else com_history.option(idx_sel - 1);
			}
			else
			{
				if(idx_sel == idx_last)
					com_history.option(0);
				else com_history.option(idx_sel + 1);
			}
			nana::api::refresh_window(com_history);
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
	if(theme::is_dark())
		tb.fgcolor(theme::tbfg.blend(nana::colors::black, .15));
	else tb.fgcolor(theme::tbfg.blend(nana::colors::white, .15));

	fm.collocate();
	fm.show();
	fm.modality();
}