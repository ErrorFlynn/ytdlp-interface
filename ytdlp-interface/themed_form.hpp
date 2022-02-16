#pragma once
#include <nana/gui.hpp>
#include "util.hpp"

class themed_form : public nana::form
{
public:
	using theme_cb = std::function<void(bool)>;
	themed_form(theme_cb theme_change_callback = nullptr, 
				nana::window owner = nullptr, 
				nana::rectangle r = {}, 
				const nana::appearance& appear = {});
	void dark_theme(bool enable);
	bool dark_theme() { return use_dark_mode; }
	void system_theme(bool enable);
	bool system_theme() { return use_system_setting; }
	bool system_supports_darkmode() { return dark_mode_supported; }
	void theme_callback(theme_cb cb) { callback = cb; }

protected:
	HWND hwnd {nullptr};
	subclass msg {*this};

private:
	theme_cb callback;

	enum IMMERSIVE_HC_CACHE_MODE
	{
		IHCM_USE_CACHED_VALUE,
		IHCM_REFRESH
	};

	// 1903 18362
	enum PreferredAppMode
	{
		Default,
		AllowDark,
		ForceDark,
		ForceLight,
		Max
	};

	enum WINDOWCOMPOSITIONATTRIB
	{
		WCA_UNDEFINED = 0,
		WCA_NCRENDERING_ENABLED = 1,
		WCA_NCRENDERING_POLICY = 2,
		WCA_TRANSITIONS_FORCEDISABLED = 3,
		WCA_ALLOW_NCPAINT = 4,
		WCA_CAPTION_BUTTON_BOUNDS = 5,
		WCA_NONCLIENT_RTL_LAYOUT = 6,
		WCA_FORCE_ICONIC_REPRESENTATION = 7,
		WCA_EXTENDED_FRAME_BOUNDS = 8,
		WCA_HAS_ICONIC_BITMAP = 9,
		WCA_THEME_ATTRIBUTES = 10,
		WCA_NCRENDERING_EXILED = 11,
		WCA_NCADORNMENTINFO = 12,
		WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
		WCA_VIDEO_OVERLAY_ACTIVE = 14,
		WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
		WCA_DISALLOW_PEEK = 16,
		WCA_CLOAK = 17,
		WCA_CLOAKED = 18,
		WCA_ACCENT_POLICY = 19,
		WCA_FREEZE_REPRESENTATION = 20,
		WCA_EVER_UNCLOAKED = 21,
		WCA_VISUAL_OWNER = 22,
		WCA_HOLOGRAPHIC = 23,
		WCA_EXCLUDED_FROM_DDA = 24,
		WCA_PASSIVEUPDATEMODE = 25,
		WCA_USEDARKMODECOLORS = 26,
		WCA_LAST = 27
	};

	struct WINDOWCOMPOSITIONATTRIBDATA
	{
		WINDOWCOMPOSITIONATTRIB Attrib;
		PVOID pvData;
		SIZE_T cbData;
	};

	using fnRtlGetNtVersionNumbers = void (WINAPI*)(LPDWORD major, LPDWORD minor, LPDWORD build);
	using fnSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
	// 1809 17763
	using fnShouldAppsUseDarkMode = bool (WINAPI*)(); // ordinal 132
	using fnAllowDarkModeForWindow = bool (WINAPI*)(HWND hWnd, bool allow); // ordinal 133
	using fnAllowDarkModeForApp = bool (WINAPI*)(bool allow); // ordinal 135, in 1809
	using fnFlushMenuThemes = void (WINAPI*)(); // ordinal 136
	using fnRefreshImmersiveColorPolicyState = void (WINAPI*)(); // ordinal 104
	using fnIsDarkModeAllowedForWindow = bool (WINAPI*)(HWND hWnd); // ordinal 137
	using fnOpenNcThemeData = HTHEME(WINAPI*)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
	// 1903 18362
	using fnShouldSystemUseDarkMode = bool (WINAPI*)(); // ordinal 138
	using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode); // ordinal 135, in 1903
	using fnIsDarkModeAllowedForApp = bool (WINAPI*)(); // ordinal 139

	fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
	fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
	fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
	fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
	fnFlushMenuThemes _FlushMenuThemes = nullptr;
	fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
	fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
	fnOpenNcThemeData _OpenNcThemeData = nullptr;
	// 1903 18362
	fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode = nullptr;
	fnSetPreferredAppMode _SetPreferredAppMode = nullptr;

	bool dark_mode_supported = false, use_system_setting = true, use_dark_mode = false;
	DWORD g_buildNumber = 0;

	bool AllowDarkModeForWindow(HWND hWnd, bool allow)
	{
		if(dark_mode_supported)
			return _AllowDarkModeForWindow(hWnd, allow);
		return false;
	}

	void RefreshTitleBarThemeColor(HWND hWnd)
	{
		BOOL dark = FALSE;
		if(use_system_setting)
		{
			if(_IsDarkModeAllowedForWindow(hWnd) && _ShouldAppsUseDarkMode())
				dark = TRUE;
		}
		else dark = use_dark_mode;
		if(g_buildNumber < 18362)
			SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
		else if(_SetWindowCompositionAttribute)
		{
			WINDOWCOMPOSITIONATTRIBDATA data = {WCA_USEDARKMODECOLORS, &dark, sizeof(dark)};
			_SetWindowCompositionAttribute(hWnd, &data);
		}
	}

	bool IsColorSchemeChangeMessage(LPARAM lParam)
	{
		bool is = false;
		if(lParam && CompareStringOrdinal(reinterpret_cast<LPCWCH>(lParam), -1, L"ImmersiveColorSet", -1, TRUE) == CSTR_EQUAL)
		{
			_RefreshImmersiveColorPolicyState();
			is = true;
		}
		return is;
	}

	bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
	{
		if(message == WM_SETTINGCHANGE)
			return IsColorSchemeChangeMessage(lParam);
		return false;
	}

	void AllowDarkModeForApp(bool allow)
	{
		if(_AllowDarkModeForApp)
			_AllowDarkModeForApp(allow);
		else if(_SetPreferredAppMode)
			_SetPreferredAppMode(allow ? AllowDark : Default);
	}

	constexpr bool CheckBuildNumber(DWORD buildNumber)
	{
		return (buildNumber == 17763 || // 1809
				buildNumber == 18362 || // 1903
				buildNumber == 18363 || // 1909
				buildNumber == 19041 || // 2004
				buildNumber >= 22000);
	}

	void InitDarkMode();
	void refresh_widgets();
};