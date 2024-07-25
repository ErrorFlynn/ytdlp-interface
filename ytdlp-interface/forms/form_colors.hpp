#pragma once

#include "../gui.hpp"
#include "../hsl_picker.hpp"


class my_tbox : public ::widgets::Textbox
{
public:
	nana::color *clr {nullptr};
	nana::hsl_picker *picker {nullptr};
	bool noupdate {false};
	nana::window parent {nullptr};

	my_tbox(nana::window parent, nana::hsl_picker &picker, nana::color *pclr);

	void apply()
	{
		picker->color_value(*clr, true);
		nana::api::refresh_window(*picker);
	}
};


class my_label : public ::widgets::Label
{
public:
	my_label(nana::window parent, std::string_view text) : Label {parent, text}
	{
		text_align(nana::align::left, nana::align_v::center);
	}
};


class mybtn : public ::widgets::Button
{
public:

	mybtn() : Button {} {};
	void create(nana::window parent);
};

class container : public nana::panel<true>
{
	struct fake_field_interface
	{
		nana::widget *parent {nullptr}, **target {nullptr};
		nana::place *plc {nullptr};
		std::vector<nana::widget *> *widgets {nullptr};
		std::string field_name;

		fake_field_interface(nana::widget *parent) : parent {parent} {}
		nana::place::field_reference operator<<(nana::window wd);
	};

	mybtn btn;
	nana::place plc;
	fake_field_interface ffi {this};

	void refresh_btn_theme() { btn.refresh_theme(); }

public:

	static theme_t *t_custom, *t_conf, *t_def;
	static container *selected;
	nana::widget *target {nullptr};
	std::vector<nana::widget*> widgets;
	static std::vector<container*> containers;
	//nana::color *clrconf {nullptr};
	static std::function<void()> btn_callback;

	static void refresh_buttons(bool state);

	container() = delete;
	container(nana::window parent) { create(parent); }

	void create(nana::window parent);
	void refresh_btn_state();
	void refresh_theme() { bgcolor(t_custom->fmbg); }
	auto &get_place() { return plc; }
	void div(std::string div_text) { plc.div(div_text); }

	auto &operator[](const char *field_name)
	{
		ffi.field_name = field_name;
		return ffi;
	}

};