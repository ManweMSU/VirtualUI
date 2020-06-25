#pragma once

#include "../EngineBase.h"
#include "../UserInterface/ControlBase.h"

namespace Engine
{
	namespace MacOSXSpecific
	{
		namespace CreationAttribute
		{
			enum CreationAttributeFlags {
				Normal				= 0x0000,
				TransparentTitle	= 0x0001,
				EffectBackground	= 0x0002,
				Transparent			= 0x0004,
				LightTheme			= 0x0008,
				DarkTheme			= 0x0010,
				Shadowless			= 0x0020,
			};
		}
		enum class EffectBackgroundMaterial {
			Titlebar, Selection, Menu, Popover, Sidebar, HeaderView, Sheet, WindowBackground, HUD, FullScreenUI, ToolTip
		};
		enum class RenderingDeviceFeatureClass { DontCare = 0, Quartz = 1, Metal = 2 };
		void SetWindowCreationAttribute(int attribute);
		int GetWindowCreationAttribute(void);
		void SetRenderingDeviceFeatureClass(RenderingDeviceFeatureClass dev_class);
		RenderingDeviceFeatureClass GetRenderingDeviceFeatureClass(void);
		void SetWindowBackgroundColor(UI::Window * window, UI::Color color);
		void SetWindowTransparentcy(UI::Window * window, double value);
		void SetEffectBackgroundMaterial(UI::Window * window, EffectBackgroundMaterial material);
	}
}