#include "gui.hpp"
#include <nana/gui/filebox.hpp>


void GUI::show_btncopy(bool show)
{
	auto &plc {get_place()};
	plc.field_display("btncopy", show);
	plc.field_display("btncopy_spacer", show);
	plc.collocate();
}


void GUI::show_btnfmt(bool show)
{
	if(std::this_thread::get_id() == main_thread_id)
	{
		auto &plc {get_place()};
		plc.field_display("btn_ytfmtlist", show);
		plc.field_display("ytfm_spacer", show);
		plc.collocate();
	}
	else PostMessage(hwnd, WM_SHOW_BTNFMT, show, 0);
}


fs::path GUI::gui_bottom::file_path()
{
	if(!printed_path.empty())
	{
		if(printed_path.extension().string() != "NA")
		{
			if(fs::exists(printed_path))
				return printed_path;
		}
		if(!merger_path.empty())
		{
			printed_path.replace_extension(merger_path.extension());
			if(fs::exists(printed_path))
				return printed_path;
			if(!download_path.empty())
			{
				printed_path.replace_extension(download_path.extension());
				if(fs::exists(printed_path))
					return printed_path;
			}
			if(vidinfo_contains("ext"))
			{
				std::string ext {vidinfo["ext"]};
				printed_path.replace_extension(ext);
				if(fs::exists(printed_path))
					return printed_path;
			}
		}
	}

	if(!merger_path.empty() && fs::exists(merger_path))
		return merger_path;
	if(!download_path.empty() && fs::exists(download_path))
		return download_path;

	fs::path file;
	if(vidinfo_contains("filename"))
		file = fs::u8path(vidinfo["filename"].get<std::string>());

	if(!file.empty())
	{
		file = outpath / file;
		if(!fs::exists(file))
		{
			if(cbargs && argset.find("-f ") != -1)
			{
				auto pos {argset.find("-f ")};
				if(pos != -1)
				{
					pos += 1;
					while(pos < argset.size() && isspace(argset[++pos]));
					std::string num;
					while(pos < argset.size())
					{
						char c {argset[pos++]};
						if(isdigit(c))
							num += c;
						else break;
					}
					if(!num.empty())
					{
						for(auto &fmt : vidinfo["formats"])
						{
							std::string format_id {fmt["format_id"]}, ext {fmt["ext"]};
							if(format_id == num)
							{
								file.replace_extension(ext);
								break;
							}
						}
						if(!fs::exists(file))
							file.clear();
					}
					else file.clear();
				}
				else file.clear();
			}
			else if(cbmp3)
			{
				file.replace_extension("mp3");
				if(!fs::exists(file))
					file.clear();
			}
			else if(use_strfmt)
			{
				using nana::to_utf8;
				std::string ext1, ext2, vcodec;
				for(auto &fmt : vidinfo["formats"])
				{
					std::string format_id {fmt["format_id"]}, ext {fmt["ext"]};
					if(ext1.empty() && format_id == to_utf8(fmt1))
					{
						ext1 = to_utf8(ext);
						if(fmt.contains("vcodec") && fmt["vcodec"] != "none")
							vcodec = fmt["vcodec"];
						if(fmt2.empty()) break;
					}
					else if(!fmt2.empty() && format_id == to_utf8(fmt2))
					{
						ext2 = to_utf8(ext);
						if(fmt.contains("vcodec") && fmt["vcodec"] != "none")
							vcodec = fmt["vcodec"];
						if(!ext1.empty()) break;
					}
				}
				if(ext2.empty())
					file.replace_extension(ext1);
				else
				{
					std::string ext;
					if(ext2 == "webm")
						ext = (ext1 == "webm" || ext1 == "weba") ? "webm" : "mkv";
					else if(ext1 == "webm" || ext1 == "weba")
						ext = vcodec.starts_with("av01") ? "webm" : "mkv";
					else ext = "mp4";
					file.replace_extension(ext);
				}
				if(!fs::exists(file))
					file.clear();
			}
			else file.clear();
		}
	}
	return file;
}


GUI::gui_bottom::gui_bottom(GUI &gui)
{
	using namespace nana;

	auto &conf {GUI::conf};
	pgui = &gui;

	auto prevbot {gui.bottoms.back()};
	if(prevbot)
		outpath = prevbot->outpath;
	else outpath = conf.outpath;

	if(prevbot)
	{
		cbsubs = prevbot->cbsubs;
		cbthumb = prevbot->cbthumb;
		cbtime = prevbot->cbtime;
		cbkeyframes = prevbot->cbkeyframes;
		cbmp3 = prevbot->cbmp3;
		cbargs = prevbot->cbargs;
		argset = prevbot->argset;
	}
	else
	{
		cbsubs = conf.cbsubs;
		cbthumb = conf.cbthumb;
		cbtime = conf.cbtime;
		cbkeyframes = conf.cbkeyframes;
		cbmp3 = conf.cbmp3;
		cbargs = conf.cbargs;
		argset = conf.argset;
	}

	//gui.queue_panel.focus();
}


bool GUI::gui_bottom::browse_for_filename()
{
	nana::filebox fb {*pgui, false};
	fb.allow_multi_select(false);
	fb.title("Specify the name to give the downloaded file");
	if(outfile.empty())
	{
		if(!vidinfo.empty() && vidinfo_contains("_filename"))
		{
			std::string fname {vidinfo["_filename"]};
			if(vidinfo_contains("requested_formats") && vidinfo["requested_formats"].size() > 1)
			{
				auto pos {fname.rfind('.')};
				if(pos != -1)
					fname = fname.substr(0, pos);
			}
			fb.init_file(outpath / fname);
		}
		else fb.init_file(outpath / "type the file name here (this overrides the output template from settings)");
	}
	else fb.init_file(outpath / outfile.filename());
	auto res {fb()};
	if(res.size())
	{
		outfile = res.front();
		outpath = conf.outpath = outfile.parent_path();
		pgui->l_outpath.caption(outpath.u8string());
		pgui->l_outpath.tooltip("Custom file name:\n<bold>" + outfile.filename().string() +
			"</>\n(this overrides the output template from the settings)");
		if(conf.common_dl_options)
			pgui->bottoms.propagate_misc_options(*this);
		if(conf.outpaths.size() >= 11 && conf.outpaths.find(outpath) == conf.outpaths.end())
			conf.outpaths.erase(conf.outpaths.begin());
		conf.outpaths.insert(outpath);
		if(conf.cb_display_custom_filenames)
			pgui->lbq.item_from_value(url).text(2, outfile.filename().string());
		return true;
	}
	return false;
}


void GUI::gui_bottom::apply_playsel_string()
{
	auto &str {playsel_string};
	playlist_selection.assign(playlist_info["entries"].size(), false);

	auto pos0 {str.rfind('|')};
	if(pos0 != -1)
		str.erase(0, pos0 + 1);

	int a {0}, b {0};
	size_t pos {0}, pos1 {0};
	pos1 = str.find_first_of(L",:", pos);
	if(pos1 == -1)
		playlist_selection[stoi(str) - 1] = true;
	while(pos1 != -1)
	{
		a = stoi(str.substr(pos, pos1 - pos)) - 1;
		pos = pos1 + 1;
		if(str[pos1] == ',')
		{
			playlist_selection[a] = true;
			pos1 = str.find_first_of(L",:", pos);
			if(pos1 == -1)
				playlist_selection[stoi(str.substr(pos)) - 1] = true;
		}
		else // ':'
		{
			pos1 = str.find(',', pos);
			if(pos1 != -1)
			{
				b = stoi(str.substr(pos, pos1 - pos)) - 1;
				pos = pos1 + 1;
				pos1 = str.find_first_of(L",:", pos);
				if(pos1 == -1)
					playlist_selection[stoi(str.substr(pos)) - 1] = true;
			}
			else b = stoi(str.substr(pos)) - 1;

			for(auto n {a}; n <= b; n++)
				playlist_selection[n] = true;
		}
	}
}


int GUI::gui_bottom::playlist_selected()
{
	int cnt {0};
	for(auto el : playlist_selection)
		if(el) cnt++;
	return cnt;
}


bool GUI::gui_bottom::vidinfo_contains(std::string key)
{
	if(vidinfo.contains(key) && vidinfo[key] != nullptr)
		return true;
	return false;
}


void GUI::gui_bottom::from_json(const nlohmann::json &j)
{
	using nana::to_wstring;

	if(j.contains("vidinfo"))
	{
		vidinfo = j["vidinfo"];
	}

	if(j.contains("playlist_info"))
	{
		playlist_info = j["playlist_info"];
		if(j.contains("playsel_string"))
		{
			playsel_string = to_wstring(j["playsel_string"].get<std::string>());
			apply_playsel_string();
		}
		else playlist_selection.assign(playlist_info["entries"].size(), true);
	}
	if(j.contains("strfmt"))
	{
		strfmt = to_wstring(j["strfmt"].get<std::string>());
		use_strfmt = j["use_strfmt"];
	}
	if(j.contains("fmt1"))
		fmt1 = to_wstring(j["fmt1"].get<std::string>());
	if(j.contains("fmt2"))
		fmt2 = to_wstring(j["fmt2"].get<std::string>());
	if(j.contains("cmdinfo"))
		cmdinfo = to_wstring(j["cmdinfo"].get<std::string>());
	if(j.contains("playlist_vid_cmdinfo"))
		playlist_vid_cmdinfo = to_wstring(j["playlist_vid_cmdinfo"].get<std::string>());
	if(j.contains("idx_error"))
		idx_error = j["idx_error"].get<int>();
	if(j.contains("is_yttab"))
		is_yttab = j["is_yttab"];
	if(j.contains("is_ytplaylist"))
		is_ytplaylist = j["is_ytplaylist"];
	if(j.contains("is_gen_playlist"))
		is_gen_playlist = j["is_gen_playlist"];
	if(j.contains("output_buffer"))
		pgui->outbox.buffer(url, j["output_buffer"].get<std::string>());
	if(j.contains("dlcmd"))
		pgui->outbox.commands[url] = j["dlcmd"].get<std::string>();
	if(j.contains("media_title"))
		media_title = j["media_title"].get<std::string>();
	if(j.contains("outfile"))
	{
		outfile = fs::u8path(j["outfile"].get<std::string>());
		outpath = conf.outpath = outfile.parent_path();
		pgui->l_outpath.caption(outpath.u8string());
		pgui->l_outpath.tooltip("Custom file name:\n<bold>" + outfile.filename().string() +
			"</>\n(this overrides the output template from the settings)");
	}
	if(j.contains("outpath"))
	{
		outpath = fs::u8path(j["outpath"].get<std::string>());
		rate = j["tbrate"].get<std::string>();
		ratelim_unit = j["com_rate"];
		cbtime = j["cbtime"];
		cbsubs = j["cbsubs"];
		com_chap = j["com_chap"];
		cbkeyframes = j["cbkeyframes"];
		cbthumb = j["cbthumb"];
		cbmp3 = j["cbmp3"];
		cbargs = j["cbargs"];
		argset = j["com_args"].get<std::string>();
		progtext = j["progtext"].get<std::string>();
		progval = j["progval"];
		prog_amount = j["prog_amount"];
		progval_shadow = j["progval_shadow"];
	}
}


void GUI::gui_bottom::to_json(nlohmann::json &j)
{
	using namespace nana;
	if(!vidinfo.empty())
		j["vidinfo"] = vidinfo;
	if(!playlist_info.empty())
		j["playlist_info"] = playlist_info;
	if(!strfmt.empty())
	{
		j["strfmt"] = to_utf8(strfmt);
		j["use_strfmt"] = use_strfmt;
	}
	if(!fmt1.empty())
		j["fmt1"] = to_utf8(fmt1);
	if(!fmt2.empty())
		j["fmt2"] = to_utf8(fmt2);
	if(!cmdinfo.empty())
		j["cmdinfo"] = to_utf8(cmdinfo);
	if(!playlist_vid_cmdinfo.empty())
		j["playlist_vid_cmdinfo"] = to_utf8(playlist_vid_cmdinfo);
	if(!playsel_string.empty())
		j["playsel_string"] = to_utf8(playsel_string);
	if(idx_error)
		j["idx_error"] = idx_error;
	if(is_yttab)
		j["is_yttab"] = true;
	if(is_ytplaylist)
		j["is_ytplaylist"] = true;
	if(is_gen_playlist)
		j["is_gen_playlist"] = is_gen_playlist;
	if(!pgui->outbox.buffer(url).empty())
		j["output_buffer"] = pgui->outbox.buffer(url);
	if(pgui->outbox.commands.contains(url))
		j["dlcmd"] = pgui->outbox.commands[url];
	if(!outfile.empty())
		j["outfile"] = outfile.string();
	if(!media_title.empty())
		j["media_title"] = media_title;

	j["outpath"] = outpath.string();
	j["tbrate"] = rate;
	j["com_rate"] = ratelim_unit;
	j["cbtime"] = cbtime;
	j["cbsubs"] = cbsubs;
	j["com_chap"] = com_chap;
	j["cbkeyframes"] = cbkeyframes;
	j["cbthumb"] = cbthumb;
	j["cbmp3"] = cbmp3;
	j["cbargs"] = cbargs;
	j["com_args"] = argset;
	j["progtext"] = progtext;
	j["progval"] = progval;
	j["prog_amount"] = prog_amount;
	j["progval_shadow"] = progval_shadow;
}