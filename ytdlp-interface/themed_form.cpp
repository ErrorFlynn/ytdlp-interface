#include "themed_form.hpp"
#include <iostream>
#include <dwmapi.h>


themed_form::themed_form(theme_cb theme_change_callback, nana::window owner, nana::rectangle r, const nana::appearance& appear) : form{owner, r, appear}
{
	InitDarkMode();

	hwnd = reinterpret_cast<HWND>(form::native_handle());
	if(theme_change_callback)
		callback = std::move(theme_change_callback);
	if(dark_mode_supported)
	{
		_AllowDarkModeForWindow(hwnd, true);
		RefreshTitleBarThemeColor(hwnd);

		msg.make_after(WM_SETTINGCHANGE, [this](UINT, WPARAM, LPARAM lParam, LRESULT*)
		{
			if(IsColorSchemeChangeMessage(lParam))
				SendMessageW(hwnd, WM_THEMECHANGED, 0, 0);
			return false;
		});

		msg.make_after(WM_THEMECHANGED, [this](UINT, WPARAM, LPARAM, LRESULT*)
		{
			if(use_system_setting)
			{
				_AllowDarkModeForWindow(hwnd, _ShouldAppsUseDarkMode());
				RefreshTitleBarThemeColor(hwnd);
				if(callback) if(callback(_ShouldAppsUseDarkMode())) refresh_widgets();
				else refresh_widgets();
			}
			return false;
		});

		msg.make_before(WM_KEYDOWN, [this](UINT, WPARAM wparam, LPARAM, LRESULT *)
		{
			if(wparam == VK_ESCAPE)
			{
				close();
				return false;
			}
			return true;
		});
	}

	msg.make_after(WM_ENTERSIZEMOVE, [this](UINT, WPARAM, LPARAM, LRESULT*)
	{
		DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rsnap, sizeof rsnap);
		GetCursorPos(&snap_cur_pos);

		snap_x = snap_cur_pos.x - rsnap.left;
		snap_y = snap_cur_pos.y - rsnap.top;

		return false;
	});

	msg.make_after(WM_MOVING, [this](UINT, WPARAM, LPARAM lparam, LRESULT*)
	{
		if(snap_)
		{
			auto  pr_current     {reinterpret_cast<LPRECT>(lparam)};
			auto  width          {pr_current->right - pr_current->left}, 
			      correct_width  {rsnap.right - rsnap.left};

			int offset {0};
			if(width > correct_width)
				offset = dpi_transform((width - correct_width) / 2) + 1;

			GetCursorPos(&snap_cur_pos);
			OffsetRect(pr_current, snap_cur_pos.x - (pr_current->left + snap_x), snap_cur_pos.y - (pr_current->top + snap_y));

			SystemParametersInfo(SPI_GETWORKAREA, 0, &snap_wa, 0);

			if(IsSnapClose(pr_current->left + offset, snap_wa.left))
				OffsetRect(pr_current, snap_wa.left - pr_current->left - offset, 0);
			else if(IsSnapClose(snap_wa.right + offset, pr_current->right))
				OffsetRect(pr_current, snap_wa.right - pr_current->right + offset, 0);

			if(IsSnapClose(pr_current->top, snap_wa.top))
				OffsetRect(pr_current, 0, snap_wa.top - pr_current->top);
			else if(IsSnapClose(snap_wa.bottom, pr_current->bottom - offset))
				OffsetRect(pr_current, 0, snap_wa.bottom - pr_current->bottom + offset);

			return true;
		}

		return false;
	});
}

void themed_form::dark_theme(bool enable)
{
	use_system_setting = false;
	use_dark_mode = enable;
	if(dark_mode_supported)
	{
		AllowDarkModeForWindow(hwnd, enable);
		RefreshTitleBarThemeColor(hwnd);
	}
	if(callback) if(callback(enable)) refresh_widgets();
	else refresh_widgets();
}

void themed_form::system_theme(bool enable)
{
	if(dark_mode_supported)
	{
		if(enable)
		{
			use_system_setting = true;
			_AllowDarkModeForWindow(hwnd, _ShouldAppsUseDarkMode());
			RefreshTitleBarThemeColor(hwnd);
			if(callback)
				if(callback(_ShouldAppsUseDarkMode()))
					refresh_widgets();
			else refresh_widgets();
		}
		else if(use_system_setting)
			dark_theme(false);
	}
}

void themed_form::InitDarkMode()
{
	{
		auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
		if(RtlGetNtVersionNumbers)
		{
			DWORD major, minor;
			RtlGetNtVersionNumbers(&major, &minor, &g_buildNumber);
			g_buildNumber &= ~0xF0000000;
			if(major == 10 && minor == 0 && CheckBuildNumber(g_buildNumber))
			{
				HMODULE hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
				if(hUxtheme)
				{
					_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
					_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
					_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
					_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

					auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
					if(g_buildNumber < 18362)
						_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
					else
						_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);

					_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

					_SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

					if(_OpenNcThemeData &&
					   _RefreshImmersiveColorPolicyState &&
					   _ShouldAppsUseDarkMode &&
					   _AllowDarkModeForWindow &&
					   (_AllowDarkModeForApp || _SetPreferredAppMode) &&
					   _IsDarkModeAllowedForWindow)
					{
						dark_mode_supported = true;

						AllowDarkModeForApp(true);
						_RefreshImmersiveColorPolicyState();
					}
				}
			}
		}
	}
}

void themed_form::refresh_widgets()
{
	nana::API::enum_widgets(*this, true, [](widget& wdg)
	{
		wdg.events().expose.emit({}, wdg);
		nana::API::refresh_window(wdg);
	});
}


nana::size themed_form::dpi_transform_size(double w, double h)
{
	double dpi_horz {static_cast<double>(nana::API::screen_dpi(true))},
		dpi_vert {static_cast<double>(nana::API::screen_dpi(false))};

	if(dpi_horz > 96)
	{
		w = round(w * dpi_horz / 96.0);
		h = round(h * dpi_vert / 96.0);
	}
	return nana::size {static_cast<unsigned>(w), static_cast<unsigned>(h)};
}


int themed_form::dpi_transform(int val, double from_dpi)
{
	double dpi {static_cast<double>(nana::API::screen_dpi(true))};
	if(dpi != from_dpi)
		val = round(val * dpi / from_dpi);
	return val;
}


bool themed_form::center(double w, double h)
{
	using namespace nana;

	auto hmon {MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST)};
	MONITORINFO moninfo {};
	moninfo.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hmon, &moninfo);
	const auto maxh {moninfo.rcWork.bottom};
	const auto maxw {screen {}.from_window(*this).area().width};
	auto r {API::make_center(w ? dpi_transform_size(w, h) : size())};
	move(r);
	auto sz {API::window_outline_size(*this)};
	const auto wnd {api::get_owner_window(*this)};
	if(api::is_window(wnd))
	{
		RECT rect;
		DwmGetWindowAttribute(reinterpret_cast<themed_form*>(api::get_widget(wnd))->hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &rect, sizeof rect);
		point owner_pos {rect.left, rect.top}, centered_pos;
		nana::size owner_size {unsigned(rect.right - rect.left), unsigned(rect.bottom - rect.top)};
		centered_pos.x = owner_pos.x + (owner_size.width / 2 - sz.width / 2);
		centered_pos.y = owner_pos.y + (owner_size.height / 2 - sz.height / 2);
		sz.height = min(sz.height, maxh);
		if(centered_pos.y + sz.height > maxh)
			centered_pos.y -= sz.height - (maxh - centered_pos.y);
		if(centered_pos.x + sz.width > maxw)
			centered_pos.x -= sz.width - (maxw - centered_pos.x);
		MoveWindow(hwnd, max(0, centered_pos.x), max(0, centered_pos.y), min(sz.width, maxw), min(sz.height, maxh), TRUE);
	}
	else
	{
		if(sz.height > maxh)
			MoveWindow(hwnd, (maxw - sz.width) / 2, 0, sz.width, maxh, TRUE);
		else MoveWindow(hwnd, (maxw - sz.width) / 2, (maxh - sz.height) / 2, sz.width, sz.height, TRUE);
	}
	return sz.height > maxh;
}
