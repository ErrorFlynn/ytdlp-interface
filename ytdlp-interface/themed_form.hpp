#pragma once
#include <nana/gui.hpp>
#include "subclass.hpp"

class themed_form : public nana::form
{
public:
	using theme_cb = std::function<bool(bool)>;
	themed_form(theme_cb theme_change_callback = nullptr, 
				nana::window owner = nullptr, 
				nana::rectangle r = {}, 
				const nana::appearance& appear = {});
	void dark_theme(bool enable);
	bool dark_theme() { return use_dark_mode; }
	void system_theme(bool enable);
	bool system_theme() { return use_system_setting; }
	bool system_supports_darkmode() { return dark_mode_supported; }
	bool is_system_theme_dark();
	void theme_callback(theme_cb cb) { callback = cb; }
	void refresh_widgets();
	nana::size dpi_scale_size(double w, double h = 0);
	int dpi_scale(int val, double from_dpi = 96);
	bool center(double w = 0, double h = 0, bool autoscale = true);
	void snap(bool enable) { snap_ = enable; }
	void escape_key(bool enable) { escape_ = enable; }
	HWND native_handle() { return hwnd; }
	void subclass_before(UINT msgid, std::function<bool(UINT, WPARAM, LPARAM, LRESULT*)> handler) { msg.make_before(msgid, handler); }
	void subclass_after(UINT msgid, std::function<bool(UINT, WPARAM, LPARAM, LRESULT*)> handler) { msg.make_after(msgid, handler); }
	void subclass_umake_after(UINT msgid) { msg.umake_after(msgid); }
	void subclass_umake_before(UINT msgid) { msg.umake_before(msgid); }
	void subclass_clear() { msg.clear(); }

	void change_field_attr(nana::place &plc, std::string field, std::string attr, unsigned new_val);
	void change_field_attr(std::string field, std::string attr, unsigned new_val) { change_field_attr(get_place(), field, attr, new_val); }

protected:
	HWND hwnd {nullptr};
	subclass msg {*this};
	bool snap_ {false}, escape_ {true};

private:
	theme_cb callback;
	POINT snap_cur_pos {};
	RECT rsnap {}, snap_wa;
	int snap_x, snap_y, snap_margin {15};

	enum IMMERSIVE_HC_CACHE_MODE
	{
		IHCM_USE_CACHED_VALUE,
		IHCM_REFRESH
	};

	bool dark_mode_supported = false, use_system_setting = true, use_dark_mode = false;
	DWORD g_buildNumber = 0;

	bool AllowDarkModeForWindow(HWND hWnd, bool allow);
	void RefreshTitleBarThemeColor(HWND hWnd);
	bool IsColorSchemeChangeMessage(LPARAM lParam);

	bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
	{
		if(message == WM_SETTINGCHANGE)
			return IsColorSchemeChangeMessage(lParam);
		return false;
	}

	void AllowDarkModeForApp(bool allow);

	constexpr bool CheckBuildNumber(DWORD buildNumber)
	{
		return (buildNumber == 17763 || // 1809
				buildNumber == 18362 || // 1903
				buildNumber == 18363 || // 1909
				buildNumber == 19041 || // 2004
				buildNumber == 19042 || // 20H2
				buildNumber == 19043 || // 21H1
				buildNumber == 19044 || // 21H2
				(buildNumber > 19044 && buildNumber < 22000) || // Windows 10 any version > 21H2 
				buildNumber >= 22000);  // Windows 11 builds
	}

	void InitDarkMode();

	BOOL IsSnapClose(int a, int b) { return (abs(a - b) < dpi_scale(snap_margin)); }
};