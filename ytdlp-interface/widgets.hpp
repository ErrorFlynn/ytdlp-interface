#pragma once

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/picture.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/group.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/slider.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/gui/widgets/spinbox.hpp>
#include <nana/gui/widgets/treebox.hpp>
#include <nana/paint/text_renderer.hpp>

#include <variant>
#include <iostream>
#include <map>
#include <Windows.h>

#include "progress_ex.hpp"
#include "icons.hpp"
#include "types.hpp"

#pragma warning(disable : 4267)
#ifdef small
	#undef small
#endif

#ifdef min
	#undef min
#endif

namespace fs = std::filesystem;

struct lbqval_t
{
	std::wstring url;
	nana::paint::image *pimg {nullptr};
	operator const nana::paint::image *() const { return pimg; }
	operator const nana::paint::image *() { return pimg; }
	operator const std::wstring &() const { return url; }
	operator const std::wstring &() { return url; }
	bool operator == (const lbqval_t &o)
	{
		return url == o.url;
	}
};

static bool operator == (const lbqval_t &val, const std::wstring &url)
{
	return val.url == url;
}


namespace widgets
{
	class theme
	{
		static bool dark;
		static double shade;

	public:

		theme() { make_light({}); }

		static nana::color nimbus, fmbg, Label_fg, Text_fg, Text_fg_error, cbox_fg, btn_bg, btn_fg, /*path_bg, */path_fg, path_link_fg, 
			sep_bg, tbfg, tbkw, tbkw_id, tbkw_special, tbkw_warning, tbkw_error, gpbg, lb_headerbg, title_fg, overlay_fg, border,
			tb_selbg, tb_selbg_unfocused, expcol_fg, tree_selbg, tree_selfg, tree_hilitebg, tree_hilitefg, tree_selhilitebg, tree_selhilitefg,
			tree_parent_node, tree_expander, tree_expander_hovered, tree_bg, tree_key_fg, tree_val_fg, list_check_highlight_fg,
			list_check_highlight_bg, msg_label_fg, lbbg, lbselfg, lbselbg, lbhilite, lbfg;
		static std::string gpfg;

		static void import(const theme_t &src);
		static void make_light(const theme_t &src);
		static void make_dark(const theme_t &src);
		static void copy(theme_t &dest);

		static bool is_dark() { return dark; }
		static void contrast(double factor, theme_t &src);
		static double contrast() { return shade; }
	};


	class Label : public nana::label
	{

	public:

		Label() : label() {}

		Label(nana::window parent, std::string_view text, bool dpi_adjust = false)
		{
			create(parent, text, dpi_adjust);
		}

		void create(nana::window parent, std::string_view text, bool dpi_adjust = false);
	};


	class Text : public nana::label
	{
		bool error_mode_ {false};

	public:

		Text() : label() {}

		Text(nana::window parent, std::string_view text = "", bool dpi_adjust = false) : label {parent, text}
		{
			create(parent, text, dpi_adjust);
		}

		void create(nana::window parent, std::string_view text = "", bool dpi_adjust = false);

		void error_mode(bool enable)
		{
			error_mode_ = enable;
			fgcolor(error_mode_ ? theme::Text_fg_error : theme::Text_fg);
		}
	};


	class Separator : public nana::panel<false>
	{
		std::unique_ptr<nana::place> plc;
		nana::label sep1, sep2;

	public:

		Separator() = default;

		Separator(nana::window parent, std::string title = "")
		{
			create(parent, title);
		}

		void create(nana::window parent, std::string title = "");

		void refresh_theme()
		{
			sep1.bgcolor(theme::sep_bg);
			sep2.bgcolor(theme::sep_bg);
		}
	};


	class path_label : public nana::label
	{
		using variant = std::variant<fs::path*, std::wstring*>;
		variant v;		
		bool is_path {false};

	public:

		path_label() : label() {}
		path_label(nana::window parent, const variant var) { create(parent, var); }

		void create(nana::window parent, const variant var);
		void source_path(const variant var) { v = var; update_caption(); }
		void update_caption();
		void refresh_theme();
	};


	class cbox : public nana::checkbox
	{
	public:

		cbox() : checkbox() {}

		cbox(nana::window parent, std::string_view text)
		{
			create(parent, text);
		}

		void create(nana::window parent, std::string_view text);
		void refresh_theme();
	};


	class Button : public nana::button
	{
		nana::paint::image img, img_disabled;

		struct btn_bg : nana::element::element_interface
		{
			bool draw(nana::paint::graphics &g, const nana::color &bg,
				const nana::color &fg, const nana::rectangle &r, nana::element_state state) override;
		};

	public:

		Button() : button() {}

		Button(nana::window parent, std::string_view text = "", bool small = false)
		{
			create(parent, text, small);
		}

		void create(nana::window parent, std::string_view text = "", bool small = false);
		void refresh_theme();
		void image(const void *data, unsigned size);
		void image_disabled(const void *data, unsigned size);
		void enable(bool state);
	};


	class Listbox : public nana::listbox
	{
		using thrid = std::thread::id;

		nana::drawing dw {*this};
		bool hicontrast {false}, hilite_checked {false};
		thrid main_thread_id;
		HWND hwnd_parent {nullptr};
		bool *no_thread_safe_ops {nullptr};
		bool force_no_thread_safe_ops_ {false};

	public:

		const item_proxy empty_item {listbox::at(0).end()};

		Listbox(nana::window parent, bool *exiting = nullptr, bool hicontrast = false) : listbox {parent}, no_thread_safe_ops {exiting}, hicontrast {hicontrast}
		{
			main_thread_id = std::this_thread::get_id();
			hwnd_parent = reinterpret_cast<HWND>(reinterpret_cast<nana::form*>(nana::api::get_widget(parent))->native_handle());
			refresh_theme();
			typeface(nana::paint::font_info {"Segoe UI", 9});
			events().expose([this] { refresh_theme(); });
			dw.draw([](nana::paint::graphics &g) { g.rectangle(false, theme::border); });
		}

		size_t item_count();
		std::string favicon_url_from_value(std::wstring val);
		item_proxy item_from_value(std::wstring val);
		void hilight_checked(bool enable) { hilite_checked = enable; refresh_theme(); }
		void refresh_theme();
		void fit_column_content();
		void set_line_text(std::wstring url, qline_t text);
		void set_item_bg(std::wstring url, nana::color bg);
		void auto_draw(bool enable) noexcept;
		cat_proxy at(size_type pos);
		item_proxy at(const index_pair &abs_pos);
		item_proxy erase(item_proxy ip);
		void erase(nana::listbox::size_type cat) { listbox::erase(cat); }
		void force_no_thread_safe_ops(bool enable) noexcept { force_no_thread_safe_ops_ = enable; }
		bool force_no_thread_safe_ops() noexcept { return force_no_thread_safe_ops_; }
		void refresh_window();
	};


	class Progress : public nana::progress_ex
	{
	public:

		Progress() : progress_ex() {}

		Progress(nana::window parent) : progress_ex {parent}
		{
			create(parent);
		}

		void create(nana::window parent);
		void refresh_theme();
	};


	class Group : public nana::group
	{
		std::string title;

	public:

		Group() : group() {}

		Group(nana::window parent, std::string title)
		{
			create(parent, title);
		}

		void create(nana::window parent, std::string title = "");
		nana::widget &caption(std::string utf8);
		std::string caption();
		void refresh_theme();
	};


	class Combox : public nana::combox
	{
		class my_renderer : public item_renderer
		{
			void image(bool enabled, unsigned pixels) override {}

			// gets called multiple times (once for each item)
			void item(widget_reference, graph_reference, const nana::rectangle&, const item_interface*, state_t) override;

			unsigned item_pixels(graph_reference g) const override;

			void background(widget_reference, graph_reference g)
			{
				using namespace nana;
				//g.rectangle(true, theme::is_dark() ? theme::fmbg.blend(colors::white, .08) : theme::fmbg);
				rectangle r {{0, 0}, g.size()};
				g.rectangle(r, false, color {"#999A9E"});
				g.rectangle(r.pare_off(1), false, theme::is_dark() ? theme::fmbg.blend(colors::white, .08) : theme::fmbg);
			}
		} renderer_;

	public:

		Combox() : combox() {}

		Combox(nana::window parent)
		{
			create(parent);
		}

		void create(nana::window parent);
		void refresh_theme();
		int caption_index();
	};


	class Textbox : public nana::textbox
	{
		bool highlighted {false};
		HWND hwnd_parent {nullptr};
		std::thread::id main_thread_id;

	public:

		Textbox() : textbox() {};

		Textbox(nana::window parent, bool visible = true)
		{
			create(parent, visible);
		}

		void set_keyword(std::string name, std::string category = "general")
		{
			if(std::this_thread::get_id() == main_thread_id)
				set_keywords(category, true, true, {name});
			else 
			{
				std::pair<std::string, std::string> params {category, name};
				SendMessage(hwnd_parent, WM_SET_KEYWORDS, reinterpret_cast<WPARAM>(this), reinterpret_cast<LPARAM>(&params));
			}
		}

		void create(nana::window parent, bool visible = true);
		void refresh_theme();
		void highlight(bool enable);
		bool highlight() { return highlighted; }
	};


	class Title : public nana::label
	{
	public:

		Title() : label() {};

		Title(nana::window parent, std::string_view text = "")
		{
			create(parent, text);
		}

		void create(nana::window parent, std::string_view text = "");
		void refresh_theme();
	};


	class Slider : public nana::slider
	{
	public:
		Slider(nana::window parent);
		void refresh_theme();
	};


	class Overlay : public nana::label
	{
	public:

		Overlay() : label() {}

		Overlay(nana::window parent, nana::widget *outbox, std::string_view text = "", bool visible = true)
		{
			create(parent, outbox, text, visible);
		}

		void create(nana::window parent, nana::widget *outbox, std::string_view text = "", bool visible = true);
	};


	class Menu : public nana::menu
	{
		class menu_renderer : public nana::menu::renderer_interface
		{

		public:
			menu_renderer(const nana::pat::cloneable<renderer_interface> &rd) : reuse_ {rd}, crook_ {"menu_crook"}
			{
				crook_.check(nana::facade<nana::element::crook>::state::checked);
			}

		private:
			void background(graph_reference graph, nana::window wd) override;
			void item(graph_reference g, const nana::rectangle &r, const attr &attr) override;

			void item_image(graph_reference graph, const nana::point &pos, unsigned image_px, const nana::paint::image &img) override
			{
				reuse_->item_image(graph, pos, image_px, img);
			}

			void item_text(graph_reference g, const nana::point &pos, const std::string &text, unsigned pixels, const attr &attr) override;

			void item_text(graph_reference graph, const nana::point &pos, std::u8string_view text, unsigned pixels, const attr &atr) override
			{
				reuse_->item_text(graph, pos, text, pixels, atr);
			}

			void sub_arrow(graph_reference graph, const nana::point &pos, unsigned pixels, const attr &atr) override;

		private:
			nana::pat::cloneable<renderer_interface> reuse_;
			nana::facade<nana::element::crook> crook_;
			HWND hwnd {nullptr};
		};

	public:

		Menu()
		{
			renderer(menu_renderer {renderer()});
		}
	};


	class Spinbox : public nana::spinbox
	{
	public:

		Spinbox() = default;
		Spinbox(nana::window parent) { create(parent); }
		void create(nana::window parent);
		void refresh_theme();
	};


	class Expcol : public nana::label
	{
		bool hovered {false}, downward {false};

	public:

		Expcol() : label() {}
		Expcol(nana::window parent) : label {parent} { create(parent); }

		void create(nana::window parent);
		void refresh_theme();
		bool collapsed() { return downward; }
		void operate(bool collapse);
	};


	class JSON_Tree : public nana::treebox
	{
		using json = nlohmann::json;
		using item_proxy = nana::treebox::item_proxy;

		const json *jptr {nullptr};
		bool no_nulls {false};
		HWND hwnd {nullptr};

		class jtree_renderer : public nana::treebox::renderer_interface
		{
			using clonable_renderer = nana::pat::cloneable<renderer_interface>;
			clonable_renderer renderer_; // wraps a pointer to the library's internal renderer

		public:
			jtree_renderer(const clonable_renderer &rd) : renderer_(rd) {}

		private:

			nana::window htree_ {nullptr}; // treebox window handle

			void begin_paint(nana::widget &wdg) override
			{
				htree_ = wdg.handle();
				renderer_->begin_paint(wdg);
			}

			void bground(graph_reference graph, const compset_interface *compset) const override
			{
				renderer_->bground(graph, compset);
			}

			void expander(graph_reference graph, const compset_interface *compset) const override;

			void crook(graph_reference graph, const compset_interface *compset) const override
			{
				renderer_->crook(graph, compset);
			}

			virtual void icon(graph_reference graph, const compset_interface *compset) const override
			{
				renderer_->icon(graph, compset);
			}

			virtual void text(graph_reference graph, const compset_interface *compset) const override;
		};

		class jtree_placer : public nana::treebox::compset_placer_interface
		{
			using clonable_placer = nana::pat::cloneable<nana::treebox::compset_placer_interface>;
			clonable_placer placer_;

		public:
			jtree_placer(const clonable_placer &r) : placer_(r) {}

		private:

			virtual void enable(component_t comp, bool enabled) override
			{
				placer_->enable(comp, enabled);
			}

			virtual bool enabled(component_t comp) const override
			{
				return placer_->enabled(comp);
			}

			virtual unsigned item_height(graph_reference graph) const override
			{
				return placer_->item_height(graph);
			}

			virtual unsigned item_width(graph_reference graph, const item_attribute_t &attr) const override;

			virtual bool locate(component_t comp, const item_attribute_t &attr, nana::rectangle *r) const override
			{
				return placer_->locate(comp, attr, r);
			}
		};

	public:

		JSON_Tree() : treebox() {}

		JSON_Tree(nana::window parent, const json &data, bool hide_null = false)
		{
			create(parent, data, hide_null);
		}

		auto jdata() { return jptr; }
		void create(nana::window parent, const json &data, bool hide_null = false);
		void refresh_theme();

	private:

		void populate();
	};


	class conf_tree : public nana::treebox
	{
		using json = nlohmann::json;
		using item_proxy = nana::treebox::item_proxy;
		using page_callback = std::function<void(std::string)>;

		nana::place *plc {nullptr};

		class ctree_renderer : public nana::treebox::renderer_interface
		{
			using clonable_renderer = nana::pat::cloneable<renderer_interface>;
			clonable_renderer renderer_; // wraps a pointer to the library's internal renderer

		public:
			ctree_renderer(const clonable_renderer &rd) : renderer_(rd) {}

		private:

			nana::window htree_ {nullptr};

			void begin_paint(nana::widget &wdg) override;

			void bground(graph_reference graph, const compset_interface *compset) const override
			{
				renderer_->bground(graph, compset);
			}

			void expander(graph_reference graph, const compset_interface *compset) const override
			{
				renderer_->expander(graph, compset);
			}

			void crook(graph_reference graph, const compset_interface *compset) const override
			{
				renderer_->crook(graph, compset);
			}

			virtual void icon(graph_reference graph, const compset_interface *compset) const override
			{
				renderer_->icon(graph, compset);
			}

			virtual void text(graph_reference graph, const compset_interface *compset) const override;
		};

		class ctree_placer : public nana::treebox::compset_placer_interface
		{
			using clonable_placer = nana::pat::cloneable<nana::treebox::compset_placer_interface>;
			clonable_placer placer_;

		public:
			ctree_placer(const clonable_placer &r) : placer_(r) {}

		private:

			virtual void enable(component_t comp, bool enabled) override
			{
				placer_->enable(comp, enabled);
			}

			virtual bool enabled(component_t comp) const override
			{
				return placer_->enabled(comp);
			}

			virtual unsigned item_height(graph_reference graph) const override
			{
				return placer_->item_height(graph);
			}

			virtual unsigned item_width(graph_reference graph, const item_attribute_t &attr) const override;

			virtual bool locate(component_t comp, const item_attribute_t &attr, nana::rectangle *r) const override
			{
				return placer_->locate(comp, attr, r);
			}
		};

	public:

		conf_tree() : treebox() {}

		conf_tree(nana::window parent, nana::place *place, page_callback callback)
		{
			create(parent, place, callback);
		}

		void create(nana::window parent, nana::place *place, page_callback callback);
		void refresh_theme();
		void add(std::string item_text, std::string field_name);
		void select(std::string field_name);
		auto selected_page() const { return selected().value<std::string>(); }
	};

	class conf_page : public nana::panel<true>
	{
		std::unique_ptr<nana::place> plc;

	public:

		conf_page() = default;
		conf_page(nana::window parent) { create(parent); }

		void create(nana::window parent);
		void refresh_theme();
		auto &get_place() { return *plc; }
		void div(std::string div_text) { plc->div(div_text); }
		auto &operator[](const char *field_name) { return plc->field(field_name); }
	};


	class sblock_listbox : public nana::listbox
	{
		bool hicontrast {false}, hilite_checked {false};
		HWND hwnd {nullptr};

	public:

		sblock_listbox(nana::window parent, bool high_contrast = true);
		void refresh_theme();
	};


	class Infobox : public nana::label
	{
		bool highlighted {false};

	public:

		Infobox() : label() {};

		Infobox(nana::window parent, bool visible = true)
		{
			create(parent, visible);
		}

		void create(nana::window parent, bool visible = true);
		void refresh_theme();
	};


	class thumb_label : public nana::label
	{
	public:

		thumb_label() : label() {}

		thumb_label(nana::window parent, std::string_view text = "", bool visible = true)
		{
			create(parent, text, visible);
		}

		void create(nana::window parent, std::string_view text = "", bool visible = true);
		void refresh_theme();
	};
}


class inline_widget : public nana::listbox::inline_notifier_interface
{
	virtual void create(nana::window wd) override;
	virtual void activate(inline_indicator &ind, index_type pos);
	void resize(const nana::size &sz) override;
	virtual void set(const value_type &value);
	void clip_text(const std::string &str);
	bool whether_to_draw() const override { return false; }
	void notify_status(status_type status, bool status_on) override {}

	inline_indicator *indicator_ {nullptr};
	index_type pos_;
	nana::label text;
	nana::picture pic;
	widgets::Listbox *lb {nullptr};
	int conf {3};
	HWND hwnd {nullptr};
};