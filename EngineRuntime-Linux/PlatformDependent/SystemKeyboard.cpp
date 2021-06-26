#include "../Interfaces/KeyCodes.h"
#include "CoreX11.h"

namespace Engine
{
	namespace Keyboard
	{
		bool IsKeyPressed(uint key_code)
		{
			auto ws = Windows::GetWindowSystem(); if (!ws) return false;
			return static_cast<X11::IXWindowSystem *>(ws)->IsKeyPressed(key_code);
		}
		bool IsKeyToggled(uint key_code)
		{
			auto ws = Windows::GetWindowSystem(); if (!ws) return false;
			return static_cast<X11::IXWindowSystem *>(ws)->IsKeyToggled(key_code);
		}
		int GetKeyboardDelay(void)
		{
			uint primary, period;
			Windows::IWindowSystem * ws;
			if ((ws = Windows::GetWindowSystem()) && static_cast<X11::IXWindowSystem *>(ws)->GetAutoRepeatTimes(primary, period)) return primary; else return 500;
		}
		int GetKeyboardSpeed(void)
		{
			uint primary, period;
			Windows::IWindowSystem * ws;
			if ((ws = Windows::GetWindowSystem()) && static_cast<X11::IXWindowSystem *>(ws)->GetAutoRepeatTimes(primary, period)) return period; else return 100;
		}
	}
}