#include "../gui.hpp"
#include <nana/gui/filebox.hpp>


void GUI::fm_json()
{
	using widgets::theme;
	using namespace nana;

	themed_form fm {nullptr, *this, {}, appear::decorate<appear::sizable, appear::minimize, appear::maximize>{}};
	fm.center(1000, 1006);
	fm.caption(title + " - JSON viewer");
	fm.bgcolor(theme::fmbg);
	fm.snap(conf.cbsnap);

	fm.div("vert margin=20 <tree> <weight=20> <label weight=48>");

	auto &bottom {bottoms.current()};
	::widgets::JSON_Tree t {fm, bottom.playlist_info.empty() ? bottom.vidinfo : bottom.playlist_info, conf.json_hide_null};
	::widgets::Label lcmd {fm, ""};

	fm["tree"] << t;
	fm["label"] << lcmd;

	lcmd.typeface(paint::font_info {"Tahoma", 10});
	lcmd.text_align(align::left, align_v::top);
	lcmd.format(true);

	std::string lcmd_text_dark {"<color=0xb0b0b0>The data was obtained from yt-dlp using this command line:</>\n"
		"<bold color=0xa09080>" + to_utf8(bottom.cmdinfo) + "</>"};
	std::string lcmd_text_light {"<color=0x707070>The data was obtained from yt-dlp using this command line:</>\n"
		"<bold color=0x8090a0>" + to_utf8(bottom.cmdinfo) + "</>"};
	lcmd.events().expose([&]
	{
		lcmd.caption(theme::is_dark() ? lcmd_text_dark : lcmd_text_light);
	});

	t.events().mouse_up([&](const arg_mouse &arg)
	{
		if(arg.button == mouse::right_button && t.jdata())
		{
			auto file_selector = [&](std::string desc, std::string filter) -> fs::path
			{
				filebox fb {fm, false};
				fb.init_path(bottom.outpath);
				std::string strname;
				if(bottom.vidinfo_contains("filename"))
					strname = bottom.vidinfo["filename"];
				else if(bottom.vidinfo_contains("_filename"))
					strname = bottom.vidinfo["_filename"];
				if(!strname.empty())
				{
					strname = strname.substr(0, strname.rfind('.'));
					fb.init_file(strname);
				}
				fb.allow_multi_select(false);
				fb.add_filter(desc, filter);
				fb.title("Save as");
				auto res {fb()};
				if(res.size())
					return res.front();
				return {};
			};

			::widgets::Menu m;
			m.item_pixels(24);

			auto selitem {t.selected()};
			if(!selitem.empty())
			{
				auto seltext {selitem.text()};
				auto pos {seltext.find(':')};
				if(pos != -1)
				{
					pos += 3;
					if(seltext[pos] == '\"')
					{
						pos++;
						seltext.pop_back();
					}
					auto val {seltext.substr(pos)};
					m.append("Copy selected value", [val, this](menu::item_proxy ip)
					{
						util::set_clipboard_text(hwnd, to_wstring(val));
					});
					m.append_splitter();
				}
			}

			m.append("Save as .txt", [&](menu::item_proxy ip)
			{
				auto path {file_selector("Text file", "*.txt")};
				if(!path.empty())
				{
					std::string text {"Text version of the JSON data for URL: " + to_utf8(bottom.url) + "\n\n"};
					std::function<void(treebox::item_proxy)> recfn = [&](treebox::item_proxy parent)
					{
						for(auto node : parent)
						{
							if(node.hidden()) continue;
							auto line {node.text()};
							if(node.size())
								line += ':';
							text += std::string(node.level() - 2, '\t') + line + '\n';
							if(node.size())
								recfn(node);
						}
					};
					recfn(t.find("root"));
					std::ofstream {path} << text;
				}
			});

			m.append("Save as .json", [&](menu::item_proxy ip)
			{
				auto path {file_selector("JSON data format", "*.json")};
				if(!path.empty())
					std::ofstream {path} << *t.jdata();
			});

			m.append("Save as .json (prettified)", [&](menu::item_proxy ip)
			{
				auto path {file_selector("JSON data format", "*.json")};
				if(!path.empty())
					std::ofstream {path} << std::setw(4) << *t.jdata();
			});

			m.append_splitter();

			m.append("Hide null values", [&](menu::item_proxy ip)
			{
				conf.json_hide_null = !conf.json_hide_null;
				ip.checked(conf.json_hide_null);

				std::function<void(treebox::item_proxy)> recfn = [&](treebox::item_proxy parent)
				{
					for(auto node : parent)
					{
						if(node.size())
							recfn(node);
						else if(node.text().find(":  null") != -1)
							node.hide(conf.json_hide_null);
					}
				};

				t.auto_draw(false);
				recfn(t.find("root"));
				t.auto_draw(true);
			}).checked(conf.json_hide_null);

			m.popup_await(t, arg.pos.x, arg.pos.y);
		}
	});

	drawing {fm}.draw([&](paint::graphics &g)
	{
		if(api::is_window(t))
			g.rectangle(rectangle {t.pos(), t.size()}.pare_off(-1), false, theme::border);
	});

	fm.events().resized([&] { api::refresh_window(fm); });

	fm.theme_callback([&](bool dark)
	{
		apply_theme(dark);
		fm.bgcolor(theme::fmbg);
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	fm.collocate();
	fm.modality();
}