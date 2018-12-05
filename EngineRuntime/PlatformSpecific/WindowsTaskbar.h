#pragma once

#include "../UserInterface/ControlBase.h"

namespace Engine
{
	namespace WindowsSpecific
	{
		enum class WindowTaskbarProgressDisplayMode { Hide, Normal, Paused, Error, Indeterminated };

		void SetWindowTaskbarProgressValue(UI::Window * window, double value);
		void SetWindowTaskbarProgressDisplayMode(UI::Window * window, WindowTaskbarProgressDisplayMode mode);
	}
}