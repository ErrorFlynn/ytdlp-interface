#pragma once

#include "themed_form.hpp"
#include "msgbox_icons.hpp"

#ifdef max
	#undef max
#endif

namespace widgets
{
	class msgbox : public themed_form
	{
		class msg_label : public nana::label
		{
		public:
			msg_label() : label() {}
			void create(nana::window parent)
			{
				label::create(parent);
				fgcolor(theme::msg_label_fg);
				nana::API::effects_bground(*this, nana::effects::bground_transparent(0), 0);
				format(true);
				typeface(nana::paint::font_info {"Segoe UI", 12});
				text_align(nana::align::left, nana::align_v::top);
				events().expose([this] { fgcolor(theme::msg_label_fg); });
			}
		};

		std::stringstream msg;
		msg_label l_msg;
		nana::picture pic;
		Button btn1, btn2, btn3;
		nana::panel<true> bot_panel;
		nana::place plc_bot;
		int buttons {MB_OK}, user_choice {IDOK};

	public:

		msgbox() = delete;

		msgbox(nana::window owner = nullptr, const std::string &title = "", int buttons = MB_OK) : themed_form {nullptr, owner, {}, appear::decorate{}}, buttons {buttons}
		{
			using namespace nana;

			caption(title);
			themed_form::msg.umake(WM_KEYDOWN);
			SetWindowLong(hwnd, GWL_STYLE, GetWindowLong(hwnd, GWL_STYLE) & ~WS_SYSMENU);

			div("vert <msg margin=[15,20,25,15] <weight=68 <> <pic weight=48> <>><l_msg>> <bot_panel weight=74>");
			plc_bot.div("vert <btnbar margin=20 <> <btn1> <weight=20> <btn2> <weight=20> <btn3>>");

			bot_panel.create(*this);
			plc_bot.bind(bot_panel);

			pic.create(*this);
			pic.stretchable(true);
			pic.transparent(true);
			pic.align(align::center, align_v::top);
			paint::image img;
			if(buttons == MB_YESNO || buttons == MB_YESNOCANCEL)
				img.open(arr_question48_png, sizeof arr_question48_png);
			else img.open(arr_info48_png, sizeof arr_info48_png);
			pic.load(img);

			l_msg.create(*this);

			auto &plc {get_place()};
			plc["pic"] << pic;
			plc["l_msg"] << l_msg;
			plc["bot_panel"] << bot_panel;

			switch(buttons)
			{
			default:
			case MB_OK:
				btn3.create(bot_panel, "OK");
				btn3.events().click([this] {close(); });
				plc_bot["btn3"] << btn3;
				break;

			case nana::msgbox::yes_no:
			case MB_YESNO:
				btn2.create(bot_panel, "Yes");
				btn3.create(bot_panel, "No");
				btn2.events().click([this] {user_choice = IDYES; close(); });
				btn3.events().click([this] {user_choice = IDNO; close(); });
				plc_bot["btn2"] << btn2;
				plc_bot["btn3"] << btn3;
				break;

			case nana::msgbox::yes_no_cancel:
			case MB_YESNOCANCEL:
				btn1.create(bot_panel, "Yes");
				btn2.create(bot_panel, "No");
				btn3.create(bot_panel, "Cancel");
				btn1.events().click([this] {user_choice = IDYES; close(); });
				btn2.events().click([this] {user_choice = IDNO; close(); });
				btn3.events().click([this] {user_choice = IDCANCEL; close(); });
				plc_bot["btn1"] << btn1;
				plc_bot["btn2"] << btn2;
				plc_bot["btn3"] << btn3;
				break;
			}

			theme_callback([&](bool dark)
			{
				bgcolor(theme::fmbg);
				bot_panel.bgcolor(theme::fmbg.blend(colors::grey, .1));
				//nana::API::effects_bground(l_msg, nana::effects::bground_transparent(100), 0);
				//l_msg.bgcolor(nana::colors::light_salmon);
				return true;
			});
		}


		unsigned show()
		{
			if(auto p_owner {reinterpret_cast<themed_form*>(nana::api::get_widget(owner()))})
			{
				if(p_owner->system_theme())
					system_theme(true);
				else dark_theme(p_owner->dark_theme());
			}

			if(buttons == MB_OK)
				btn3.focus();

			l_msg.caption(msg.str());
			auto h_top {std::max(l_msg.measure(dpi_scale(477)).height, (unsigned)dpi_scale(40))};
			center(dpi_scale(580), dpi_scale(40) + dpi_scale(74) + h_top, false);
			bring_top(true);
			modality();
			return user_choice;
		}


		auto &icon(int icon)
		{
			using namespace nana;
			paint::image img;

			switch(icon)
			{
			case nana::msgbox::icon_error:
			case MB_ICONERROR:
				img.open(arr_error48_png, sizeof arr_error48_png);
				break;

			case nana::msgbox::icon_warning:
			case MB_ICONWARNING:
				img.open(arr_warning48_png, sizeof arr_warning48_png);
				break;

			case nana::msgbox::icon_question:
			case MB_ICONQUESTION:
				img.open(arr_question48_png, sizeof arr_question48_png);
				break;

			default:
			case nana::msgbox::icon_information:
			case MB_ICONINFORMATION:
				img.open(arr_info48_png, sizeof arr_info48_png);
				break;
			}

			pic.load(img);
			return *this;
		}


		auto operator()() { return show(); }


		template<typename T>
		msgbox &operator<<(T &&t)
		{
			using parameter_type = std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>;
			using namespace nana;

			if constexpr(std::is_same_v<parameter_type, std::wstring> || std::is_same_v<parameter_type, wchar_t> || std::is_same_v<parameter_type, std::wstring_view>)
			{
				if constexpr(std::is_same_v<std::remove_cv_t<T>, wchar_t>)
					msg << to_utf8(std::wstring_view {&t, 1});
				else
					msg << to_utf8(t);
			}
			else if constexpr(std::is_same_v<parameter_type, std::string> || std::is_same_v<parameter_type, char> || std::is_same_v<parameter_type, std::string_view>)
			{
				if constexpr(!std::is_same_v<std::remove_cv_t<T>, wchar_t>)
					review_utf8(t);

				msg << t;
			}
			else if constexpr(std::is_same_v<parameter_type, nana::charset>)
			{
				msg << t.to_bytes(unicode::utf8);
			}
#ifdef __cpp_char8_t
			else if constexpr(std::is_same_v<parameter_type, std::u8string> || std::is_same_v<parameter_type, char8_t> || std::is_same_v<parameter_type, std::u8string_view>)
			{
				if constexpr(std::is_same_v<std::remove_cv_t<T>, wchar_t>)
					msg << static_cast<char>(t);
				else
					msg << to_string(t);
			}
#endif
			else
				msg << t;

			return *this;
		}
	};
}