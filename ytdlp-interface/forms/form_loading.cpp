#include "../gui.hpp"


void GUI::fm_loading(bool saving)
{
	using widgets::theme;
	using namespace nana;

	themed_form fm {nullptr, *this, {}, appear::bald{}};
	fm.center(456, 110);
	fm.escape_key(false);

	const auto hwnd {fm.native_handle()};
	SetWindowLongPtrA(hwnd, GWL_STYLE, GetWindowLongPtrA(hwnd, GWL_STYLE) | WS_BORDER);
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

	fm.div("vert <weight=20> <title weight=30> <text> <weight=10>");

	::widgets::Title title {fm, saving ? "Compiling data for unfinished queue items" : "Loading data for queue items"};
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
		if(saving)
		{
			for(size_t cat {0}; cat < lbq.size_categ(); cat++)
			{
				auto icat {lbq.at(cat)};
				if(icat.size())
				{
					auto &qitems {conf.unfinished_queue_items.emplace_back(icat.text(), std::vector<std::string>{}).second};	
					for(auto item : icat)
					{
						auto text {item.text(3)};
						if(text != "done" && (text != "error" || text == "error" && conf.cb_save_errors))
						{
							const auto wurl {item.value<lbqval_t>().url};
							const auto url {to_utf8(wurl)};
							qitems.push_back(url);
							auto &bottom {bottoms.at(url)};
							if(!bottom.vidinfo.empty() || !bottom.playlist_info.empty())
							{
								auto &j {unfinished_qitems_data[url]};
								bottom.to_json(j);
								j["columns"]["website"] = item.text(1);
								j["columns"]["status"] = text;
								j["columns"]["format"] = item.text(4);
								j["columns"]["format_note"] = item.text(5);
								j["columns"]["ext"] = item.text(6);
								j["columns"]["fsize"] = item.text(7);
							}
						}
					}
				}
			}
			title.caption("Saving the data");
			std::ofstream {infopath} << unfinished_qitems_data;
			if(thr_qitem_data.joinable())
				thr_qitem_data.detach();
			fm.close();
		}
		else
		{
			bool good {true};
			try { std::ifstream {infopath} >> unfinished_qitems_data; }
			catch(nlohmann::detail::exception)
			{
				good = false;
				unfinished_qitems_data.clear();
				fm.close();
			}
			std::error_code ec;
			fs::remove(infopath, ec);
			if(good)
				title.caption("Recreating the queue");
			init_qitems();
			if(thr_qitem_data.joinable())
				thr_qitem_data.detach();
			if(good)
			{
				while(!items_initialized)
				{
					Sleep(50);
					if(unfinished_qitems_data.empty())
						break;
				}
				lbq.auto_draw(true);
				fm.close();
			}
		}
	}};
	fm.modality();
}