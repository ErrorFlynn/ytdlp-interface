#pragma once
#include "themed_form.hpp"
#include "widgets.hpp"
#include <fstream>

extern bool g_enable_log;
extern std::ofstream g_logfile;

class logger
{
	std::unique_ptr<widgets::Textbox> tb;
	std::unique_ptr<themed_form> fm;
	std::mutex mtx;
	const std::thread::id main_thread_id {std::this_thread::get_id()};

public:

	bool use_window {true};
	themed_form *form_ptr() { return fm.get(); }

	logger() { make_form(); }
	~logger() { close(); }
	void make_form(nana::window owner = nullptr);

	bool close()
	{
		if(fm)
		{
			delete tb.release();
			delete fm.release();
			return true;
		}
		return false;
	}

	template<typename T> logger &operator<<(const T &t)
	{
		//std::lock_guard<std::mutex> lock {mtx};
		//if(!fm && g_enable_log && std::this_thread::get_id() == main_thread_id)
		//	make_form();
		std::stringstream ss;
		ss << t;
		if(g_logfile.is_open())
		{
			std::lock_guard<std::mutex> lock {mtx};
			g_logfile << t;
			if(ss.str().find('\n') != -1)
				g_logfile.flush();
		}
		if(fm && tb && use_window)
		{
			tb->append(ss.str(), false);
			if(!fm->visible())
			{
				fm->show();
				fm->bgcolor(widgets::theme::fmbg);
				tb->fgcolor(widgets::theme::tbfg);
				nana::api::refresh_window(fm->handle());
			}
		}
		return *this;
	}

	void operator()(std::string str)
	{
		if(g_logfile.is_open())
		{
			std::lock_guard<std::mutex> lock {mtx};
			g_logfile << str << "\n";
			g_logfile.flush();
		}
		if(fm && tb && use_window)
		{
			tb->append(str, false);
			tb->append("\n", false);
			if(!fm->visible())
			{
				fm->show();
				fm->bgcolor(widgets::theme::fmbg);
				tb->fgcolor(widgets::theme::tbfg);
				nana::api::refresh_window(fm->handle());
			}
		}
	}

	void clear()
	{
		if(tb)
		{
			tb->select(true);
			tb->del();
		}
	}
};
