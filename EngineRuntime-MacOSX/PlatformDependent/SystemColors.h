#pragma once

#include "../UserInterface/ShapeBase.h"

namespace Engine
{
	namespace UI
	{
		enum class SystemColor { Theme, WindowBackgroup, WindowText, SelectedBackground, SelectedText, MenuBackground, MenuText, MenuHotBackground, MenuHotText, GrayedText, Hyperlink };

		Color GetSystemColor(SystemColor color);
	}
}