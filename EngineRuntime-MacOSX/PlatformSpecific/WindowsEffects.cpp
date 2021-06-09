#include "WindowsEffects.h"

#ifdef ENGINE_WINDOWS
#include "../Interfaces/NativeStation.h"
#include "../PlatformDependent/WindowStation.h"
#include "../PlatformDependent/Direct3D.h"
#include "../PlatformDependent/Direct2D.h"

#include <Windows.h>
#include <dwmapi.h>
#endif

namespace Engine
{
	namespace WindowsSpecific
	{
#ifdef ENGINE_WINDOWS
		RenderingDeviceFeatureClass system_dev_class = RenderingDeviceFeatureClass::DontCare;
		void SetRenderingDeviceFeatureClass(RenderingDeviceFeatureClass dev_class) { system_dev_class = dev_class; }
		RenderingDeviceFeatureClass GetRenderingDeviceFeatureClass(void) { return system_dev_class; }
		bool CheckFeatureLevel(RenderingDeviceFeature feature)
		{
			Direct2D::InitializeFactory();
			Direct3D::CreateDevices();
			if (feature == RenderingDeviceFeature::D2D) return Direct2D::D2DFactory;
			else if (feature == RenderingDeviceFeature::D3D11) return Direct3D::D3DDevice;
			else if (feature == RenderingDeviceFeature::D2DEX) return (Direct3D::D3DDevice && Direct3D::D2DDevice && Direct3D::DXGIDevice && Direct3D::D3DDeviceContext);
			return false;
		}
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
		void SetWindowBlurBehind(UI::Window * window, bool turn_on)
		{
			if (!window->GetStation()->IsNativeStationWrapper()) return;
			auto station = static_cast<UI::HandleWindowStation *>(window->GetStation());
			station->SetBlurBehind(turn_on);
		}
		void ExtendFrameIntoClient(UI::Window * window, int left, int top, int right, int bottom)
		{
			if (!window->GetStation()->IsNativeStationWrapper()) return;
			auto station = static_cast<UI::HandleWindowStation *>(window->GetStation());
			station->SetFrameMargins(left, top, right, bottom);
		}
#else
		void SetRenderingDeviceFeatureClass(RenderingDeviceFeatureClass dev_class) {}
		RenderingDeviceFeatureClass GetRenderingDeviceFeatureClass(void) { return RenderingDeviceFeatureClass::DontCare; }
		bool CheckFeatureLevel(RenderingDeviceFeature feature) { return false; }
		void SetWindowTransparentcy(UI::Window * window, double value) {}
		void SetWindowBlurBehind(UI::Window * window, bool turn_on) {}
		void ExtendFrameIntoClient(UI::Window * window, int left, int top, int right, int bottom) {}
#endif
		
	}
}