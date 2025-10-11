#include "../gui.hpp"


std::pair<bool, std::string> GUI::input_box(nana::window owner, std::string caption, std::string prompt)
{
	using widgets::theme;
	using namespace nana;
	std::pair<bool, std::string> ret;

	themed_form fm {nullptr, owner, {}, appear::decorate{}};
	fm.center(400, 165);
	fm.caption(caption);
	fm.snap(conf.cbsnap);

	fm.div(R"(vert margin=20
		<l_prompt weight=30> <weight=15>
		<weight=25 <tb>> <weight=20>
		<<><btn_ok weight=90> <weight=20> <btn_cancel weight=90> <>>
	)");

	::widgets::Label l_prompt {fm, prompt};
	l_prompt.text_align(align::center, align_v::center);

	::widgets::Textbox tb {fm};
	tb.multi_lines(false);
	tb.padding(0, 5, 0, 5);
	tb.events().key_press([&](const arg_keyboard &arg)
	{
		if(arg.key == keyboard::enter)
		{
			ret = {true, tb.caption()};
			fm.close();
		}
	});

	::widgets::Button btn_ok {fm, "OK"}, btn_cancel {fm, "Cancel"};
	btn_ok.events().click([&] { ret = {true, tb.caption()}; fm.close(); });
	btn_cancel.events().click([&] { ret = {false, ""}; fm.close(); });
	
	fm["l_prompt"] << l_prompt;
	fm["tb"] << tb;
	fm["btn_ok"] << btn_ok;
	fm["btn_cancel"] << btn_cancel;

	fm.theme_callback([&](bool dark)
	{
		fm.bgcolor(theme::fmbg.blend(dark ? colors::white : colors::grey, .04));
		fm.refresh_widgets();
		return false;
	});

	if(conf.cbtheme == 2)
		fm.system_theme(true);
	else fm.dark_theme(conf.cbtheme == 0);

	tb.focus();
	fm.collocate();
	fm.activate();
	fm.show();
	fm.modality();
	return ret;
}