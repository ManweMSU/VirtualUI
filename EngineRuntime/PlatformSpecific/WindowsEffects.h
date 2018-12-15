#pragma once

#include "../UserInterface/ControlBase.h"

namespace Engine
{
	namespace WindowsSpecific
	{
		void SetWindowTransparentcy(UI::Window * window, double value);
		bool SetWindowBlurBehind(UI::Window * window, bool turn_on);
		bool ExtendFrameIntoClient(UI::Window * window, int left, int top, int right, int bottom);
	}
}