#include "WindowsEffects.h"

#ifdef ENGINE_WINDOWS
#include "../PlatformDependent/NativeStation.h"
#include "../PlatformDependent/WindowStation.h"

#include <Windows.h>
#include <dwmapi.h>
#endif

namespace Engine
{
	namespace WindowsSpecific
	{
#ifdef ENGINE_WINDOWS
		RenderingDeviceFeatureClass system_dev_class = RenderingDeviceFeatureClass::D3DDevice11;
		void SetRenderingDeviceFeatureClass(RenderingDeviceFeatureClass dev_class) { system_dev_class = dev_class; }
		RenderingDeviceFeatureClass GetRenderingDeviceFeatureClass(void) { return system_dev_class; }
		void SetWindowTransparentcy(UI::Window * window, double value)
		{
			if (!window->GetStation()->IsNativeStationWrapper()) return;
			auto station = static_cast<UI::HandleWindowStation *>(window->GetStation());
			auto handle = station->Handle();
			if (value < 1.0) {
				SetWindowLongW(handle, GWL_EXSTYLE, GetWindowLongW(handle, GWL_EXSTYLE) | WS_EX_LAYERED);
			} else {
				SetWindowLongW(handle, GWL_EXSTYLE, GetWindowLongW(handle, GWL_EXSTYLE) & (~WS_EX_LAYERED));
			}
			SetLayeredWindowAttributes(handle, 0, uint8(max(min(int(value * 255.0), 255), 0)), LWA_ALPHA);
		}
		bool SetWindowBlurBehind(UI::Window * window, bool turn_on)
		{
			if (!window->GetStation()->IsNativeStationWrapper()) return false;
			auto station = static_cast<UI::HandleWindowStation *>(window->GetStation());
			auto handle = station->Handle();
			DWM_BLURBEHIND blur;
			blur.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
			blur.fEnable = turn_on;
			blur.hRgnBlur = 0;
			if (DwmEnableBlurBehindWindow(handle, &blur) != S_OK) return false;
			if (turn_on) station->ClearBackgroundFlag() = true;
			return true;
		}
		bool ExtendFrameIntoClient(UI::Window * window, int left, int top, int right, int bottom)
		{
			if (!window->GetStation()->IsNativeStationWrapper()) return false;
			auto station = static_cast<UI::HandleWindowStation *>(window->GetStation());
			auto handle = station->Handle();
			MARGINS m;
			m.cxLeftWidth = left;
			m.cyTopHeight = top;
			m.cxRightWidth = right;
			m.cyBottomHeight = bottom;
			if (DwmExtendFrameIntoClientArea(handle, &m) != S_OK) return false;
			station->ClearBackgroundFlag() = true;
			return true;
		}
#else
		void SetWindowTransparentcy(UI::Window * window, double value) {}
		bool SetWindowBlurBehind(UI::Window * window, bool turn_on) { return false; }
		bool ExtendFrameIntoClient(UI::Window * window, int left, int top, int right, int bottom) { return false; }
#endif
		
	}
}