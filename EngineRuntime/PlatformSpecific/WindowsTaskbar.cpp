#include "WindowsTaskbar.h"

#include "../PlatformDependent/NativeStation.h"
#include "../PlatformDependent/WindowStation.h"

#include <Windows.h>
#include <ShlObj.h>

namespace Engine
{
	namespace WindowsSpecific
	{
#ifdef ENGINE_WINDOWS
		ITaskbarList3 * Taskbar = 0;
		void InitTaskbarComponent(void)
		{
			if (!Taskbar) {
				CoCreateInstance(CLSID_TaskbarList, 0, CLSCTX_INPROC_SERVER, __uuidof(ITaskbarList3), reinterpret_cast<void **>(&Taskbar));
			}
		}
		void SetWindowTaskbarProgressValue(UI::Window * window, double value)
		{
			if (!window->GetStation()->IsNativeStationWrapper()) return;
			auto station = static_cast<UI::HandleWindowStation *>(window->GetStation());
			auto handle = station->Handle();
			InitTaskbarComponent();
			if (Taskbar) Taskbar->SetProgressValue(handle, uint64(value * 10000.0), 10000);
		}
		void SetWindowTaskbarProgressDisplayMode(UI::Window * window, WindowTaskbarProgressDisplayMode mode)
		{
			if (!window->GetStation()->IsNativeStationWrapper()) return;
			auto station = static_cast<UI::HandleWindowStation *>(window->GetStation());
			auto handle = station->Handle();
			InitTaskbarComponent();
			if (Taskbar) {
				if (mode == WindowTaskbarProgressDisplayMode::Hide) Taskbar->SetProgressState(handle, TBPF_NOPROGRESS);
				else if (mode == WindowTaskbarProgressDisplayMode::Normal) Taskbar->SetProgressState(handle, TBPF_NORMAL);
				else if (mode == WindowTaskbarProgressDisplayMode::Paused) Taskbar->SetProgressState(handle, TBPF_PAUSED);
				else if (mode == WindowTaskbarProgressDisplayMode::Error) Taskbar->SetProgressState(handle, TBPF_ERROR);
				else if (mode == WindowTaskbarProgressDisplayMode::Indeterminated) Taskbar->SetProgressState(handle, TBPF_INDETERMINATE);
			}
		}
#else
		void SetWindowTaskbarProgressValue(UI::Window * window, double value) {}
		void SetWindowTaskbarProgressDisplayMode(UI::Window * window, WindowTaskbarProgressDisplayMode mode) {}
#endif
	}
}