#pragma once

#include "../UserInterface/ControlBase.h"

namespace Engine
{
	namespace WindowsSpecific
	{
		enum class RenderingDeviceFeatureClass { D3DDevice11, D2DDevice11 };
		void SetRenderingDeviceFeatureClass(RenderingDeviceFeatureClass dev_class);
		RenderingDeviceFeatureClass GetRenderingDeviceFeatureClass(void);
		void SetWindowTransparentcy(UI::Window * window, double value);
		bool SetWindowBlurBehind(UI::Window * window, bool turn_on);
		bool ExtendFrameIntoClient(UI::Window * window, int left, int top, int right, int bottom);
	}
}