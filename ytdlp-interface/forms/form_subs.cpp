#include "../gui.hpp"


void GUI::fm_subs()
{
	using namespace nana;
	using ::widgets::theme;
	auto url {qurl};
	auto &bottom {bottoms.at(url)};
	auto &vidinfo {bottom.is_scplaylist ? bottom.playlist_info["entries"][0] : bottom.vidinfo};

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::minimize, appear::sizable>{}};
	fm.center(425, 505);
	fm.caption(title + " - subtitle selection");
	fm.bgcolor(theme::fmbg);
	fm.snap(conf.cbsnap);
	fm.div(R"(
		vert margin=20
		<help_row weight=22 <pic_help weight=22> <weight=3> <l_help>> <help_spacer weight=10>
		<msg_row weight=30 <l_message>> <msg_spacer weight=15>
		<lb_lang>
		<spacer1 weight=20>
		<fmt_row weight=25 <l_sub_format weight=110> <weight=10> <com_subs weight=60> <>> <spacer4 weight=15>
		<cb_row weight=26 <cb_sub_selection>> <spacer2 weight=20>
		<info_row weight=25 <l_info weight=100px> <weight=10> <btn_clear weight=25>> <spacer3 weight=20>
		<btn_row weight=35 <> <btn_ok weight=110> <> <btn_cancel weight=110> <>>
	)");

	::widgets::Combox com_subs {fm};
	::widgets::Title l_message {fm, "No subtitles available!"};
	::widgets::Label l_sub_format {fm, "Subtitle format:"};
	::widgets::Text l_info {fm}, l_help {fm, "Selection applies when using <bold>--embed-subs</> or <bold>--write-subs</>"};
	::widgets::cbox cb_sub_selection {fm, "Make this selection the default for all downloads"};
	::widgets::Button btn_ok {fm, "OK"}, btn_cancel {fm, "Cancel"}, btn_clear {fm};
	nana::picture pic_help {fm};

	nana::paint::image img_help;	
	const auto dpi {nana::api::screen_dpi(true)};
	if(dpi >= 240)
		img_help.open(arr_info48a_png, sizeof arr_info48a_png);
	else if(dpi >= 144)
		img_help.open(arr_info32_png, sizeof arr_info32_png);
	else if(dpi > 96)
		img_help.open(arr_info22_ico, sizeof arr_info22_ico);
	else img_help.open(arr_info16_ico, sizeof arr_info16_ico);
	pic_help.load(img_help);
	pic_help.transparent(true);
	pic_help.align(align::center, align_v::center);

	l_help.format(true);
	l_help.typeface(nana::paint::font_info {"Segoe UI", 10});

	::widgets::Listbox lb_lang {fm};
	lb_lang.sortable(false);
	lb_lang.checkable(true);
	lb_lang.enable_single(true, false);
	lb_lang.typeface(paint::font_info {"Calibri", 12});
	lb_lang.scheme().item_height_ex = 8;
	lb_lang.append_header("", 100);
	lb_lang.column_movable(false);
	lb_lang.column_resizable(false);
	lb_lang.show_header(false);

	btn_ok.tooltip("Use this selection when <bold>--embed-subs</> or <bold>--write-subs</> are used.");
	btn_clear.tooltip("Stop using this for all downloads by default");
	btn_clear.events().click([&]
	{
		conf.sub_langs.clear();
		conf.sub_format.clear();
		fm.get_place().field_display("info_row", false);
		fm.get_place().field_display("spacer3", false);
		fm.change_field_attr("spacer2", "weight", 20);
		if(!lb_lang.item_count())
			fm.get_place().field_display("spacer4", false);
		fm.collocate();
	});

	if(dpi >= 240)
	{
		btn_clear.image(arr_erase48_png, sizeof arr_erase48_png);
		btn_clear.image_disabled(arr_erase48_disabled_png, sizeof arr_erase48_disabled_png);
	}
	else if(dpi >= 192)
	{
		btn_clear.image(arr_erase32_png, sizeof arr_erase32_png);
		btn_clear.image_disabled(arr_erase32_disabled_png, sizeof arr_erase32_disabled_png);
	}
	else if(dpi > 96)
	{
		btn_clear.image(arr_erase22_ico, sizeof arr_erase22_ico);
		btn_clear.image_disabled(arr_erase22_disabled_ico, sizeof arr_erase22_disabled_ico);
	}
	else
	{
		btn_clear.image(arr_erase16_ico, sizeof arr_erase16_ico);
		btn_clear.image_disabled(arr_erase16_disabled_ico, sizeof arr_erase16_disabled_ico);
	}

	fm["pic_help"] << pic_help;
	fm["l_help"] << l_help;
	fm["lb_lang"] << lb_lang;
	fm["l_message"] << l_message;
	fm["l_sub_format"] << l_sub_format;
	fm["com_subs"] << com_subs;
	fm["cb_sub_selection"] << cb_sub_selection;
	fm["l_info"] << l_info;
	fm["btn_ok"] << btn_ok;
	fm["btn_cancel"] << btn_cancel;
	fm["btn_clear"] << btn_clear;

	if(conf.sub_langs.empty() && conf.sub_format.empty())
	{
		fm.get_place().field_display("info_row", false);
		fm.get_place().field_display("spacer3", false);
	}
	else
	{
		fm.change_field_attr("spacer2", "weight", 10);
		std::string strfmt {conf.sub_format.empty() ? "" : conf.sub_format};
		l_info.caption("Current default: " + strfmt + (conf.sub_langs.empty() ? "" : (strfmt.empty() ? conf.sub_langs : " | " + conf.sub_langs)));
		fm.change_field_attr("l_info", "weight", l_info.measure(0).width);
	}

	std::vector<std::string> lang_codes;
	if(!bottom.sub_langs.empty())
	{
		std::stringstream ss {bottom.sub_langs};
		std::string lang_code;
		while(std::getline(ss, lang_code, ','))
			lang_codes.push_back(lang_code);
	}

	if(!vidinfo.empty() && vidinfo.contains("subtitles") && vidinfo.at("subtitles").is_object())
	{
		const auto &jsubs {vidinfo.at("subtitles")};
		if(!jsubs.empty() && jsubs.begin().key() != "live_chat")
		{
			lb_lang.auto_draw(false);
			for(auto it {jsubs.begin()}; it != jsubs.end(); it++)
			{
				if(it.key() != "live_chat" && it->is_array() && it->size() && it->front().contains("name"))
				{
					lb_lang.at(0).push_back(" " + it->front()["name"].get<std::string>());
					lb_lang.at(0).back().value(it.key());
					if(!lang_codes.empty())
					{
						auto it_find {std::find(lang_codes.begin(), lang_codes.end(), it.key())};
						if(it_find != lang_codes.end())
							lb_lang.at(0).back().check(true);
					}
					if(com_subs.the_number_of_options() == 0)
					{
						com_subs["<any>"].text("<any>");
						com_subs.option(0);
					}
					if(it == jsubs.begin())
					{
						for(auto &el : *it)
						{
							if(el.contains("ext"))
							{
								auto strfmt {' ' + el["ext"].get<std::string>()};
								com_subs[strfmt].text(strfmt);
							}
						}
					}
				}
			}
			lb_lang.sort_col(0);
			lb_lang.auto_draw(true);
		}
	}

	try
	{
		if(!bottom.sub_format.empty())
			com_subs[bottom.sub_format].select();
	}
	catch(...) {}

	if(lb_lang.item_count())
	{
		fm.get_place().field_display("msg_row", false);
		fm.get_place().field_display("msg_spacer", false);
	}
	else
	{
		if(!conf.sub_format.empty() || !conf.sub_langs.empty())
			fm.get_place().field_display("cb_row", true);
		else fm.get_place().field_display("spacer4", false);
		fm.get_place().field_display("spacer1", false);
		fm.get_place().field_display("spacer2", false);
		fm.get_place().field_display("spacer3", false);
		fm.get_place().field_display("fmt_row", false);
		fm.get_place().field_display("cb_row", false);
		fm.get_place().field_display("btn_row", false);
		fm.get_place().field_display("help_row", false);
	}

	lb_lang.events().selected([&](const arg_listbox &arg)
	{
		if(arg.item.selected())
			arg.item.check(!arg.item.checked()).select(false);
	});

	lb_lang.events().resized([&](const arg_resized &arg)
	{
		nana::paint::graphics g {{100, 100}};
		g.typeface(lb_lang.typeface());
		const auto text_height {g.text_extent_size("mm").height};
		const auto item_height {text_height + lb_lang.scheme().item_height_ex};
		const bool scroll_bar {lb_lang.at(0).size() * item_height > lb_lang.size().height};
		lb_lang.column_at(0).width(arg.width - util::scale(5) - (scroll_bar ? util::scale(16) : 0));
	});

	btn_ok.events().click([&]
	{
		conf.cb_sub_selection = cb_sub_selection.checked();
		if(conf.cb_sub_selection)
			conf.sub_langs.clear();

		auto &bottom {bottoms.current()};
		if(com_subs.caption() == "<any>")
			bottom.sub_format.clear();
		else bottom.sub_format = com_subs.caption();
		bottom.sub_langs.clear();
		for(auto &ip : lb_lang.at(0))
		{
			if(ip.checked())
			{
				if(!bottom.sub_langs.empty())
					bottom.sub_langs += L',';
				bottom.sub_langs += ip.value<std::string>();
			}
		}
		if(conf.cb_sub_selection)
		{
			conf.sub_format = bottom.sub_format;
			conf.sub_langs = bottom.sub_langs;
		}
		fm.close();
	});

	btn_cancel.events().click([&]
	{
		fm.close();
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
	api::track_window_size(fm, fm.size(), false);

	fm.collocate();

	const auto min_w {l_info.size().width + util::scale(75)};
	const auto cur_size {fm.size()};
	if(min_w > cur_size.width)
		fm.center(min_w, cur_size.height, false);

	fm.modality();
}