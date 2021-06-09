#include "../Interfaces/SystemColors.h"

#include <Windows.h>
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace Engine
{
	namespace UI
	{
		Color GetSystemColor(SystemColor color)
		{
			if (color == SystemColor::Theme) {
				Color Result;
				BOOL Opaque;
				if (DwmGetColorizationColor(reinterpret_cast<LPDWORD>(&Result), &Opaque) != S_OK) return GetSysColor(COLOR_ACTIVECAPTION) | 0xFF000000;
				swap(Result.r, Result.b);
				return Result;
			} else if (color == SystemColor::WindowBackgroup) return GetSysColor(COLOR_BTNFACE) | 0xFF000000;
			else if (color == SystemColor::WindowText) return GetSysColor(COLOR_BTNTEXT) | 0xFF000000;
			else if (color == SystemColor::SelectedBackground) return GetSysColor(COLOR_HIGHLIGHT) | 0xFF000000;
			else if (color == SystemColor::SelectedText) return GetSysColor(COLOR_HIGHLIGHTTEXT) | 0xFF000000;
			else if (color == SystemColor::MenuBackground) return GetSysColor(COLOR_MENU) | 0xFF000000;
			else if (color == SystemColor::MenuText) return GetSysColor(COLOR_MENUTEXT) | 0xFF000000;
			else if (color == SystemColor::MenuHotBackground) return GetSysColor(COLOR_HIGHLIGHT) | 0xFF000000;
			else if (color == SystemColor::MenuHotText) return GetSysColor(COLOR_HIGHLIGHTTEXT) | 0xFF000000;
			else if (color == SystemColor::GrayedText) return GetSysColor(COLOR_GRAYTEXT) | 0xFF000000;
			else if (color == SystemColor::Hyperlink) return GetSysColor(COLOR_HOTLIGHT) | 0xFF000000;
			else return 0;
		}
	}
}