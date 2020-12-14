#pragma once

#include "../UserInterface/ControlBase.h"

namespace Engine
{
	namespace WindowsSpecific
	{
		enum class RenderingDeviceFeatureClass { DontCare = 0, DeviceD2D = 1, DeviceD3D11 = 2, DeviceD2DEX = 3 };
		enum class RenderingDeviceFeature { D2D, D3D11, D2DEX };
		void SetRenderingDeviceFeatureClass(RenderingDeviceFeatureClass dev_class);
		RenderingDeviceFeatureClass GetRenderingDeviceFeatureClass(void);
		bool CheckFeatureLevel(RenderingDeviceFeature feature);
		void SetWindowTransparentcy(UI::Window * window, double value);
		void SetWindowBlurBehind(UI::Window * window, bool turn_on);
		void ExtendFrameIntoClient(UI::Window * window, int left, int top, int right, int bottom);
	}
}