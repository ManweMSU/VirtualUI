#include "../Interfaces/SystemWindows.h"

namespace Engine
{
	namespace Windows
	{
		void IWindowCallback::Created(IWindow * window) {}
		void IWindowCallback::Destroyed(IWindow * window) {}
		void IWindowCallback::Shown(IWindow * window, bool show) {}
		void IWindowCallback::RenderWindow(IWindow * window) {}
		void IWindowCallback::WindowClose(IWindow * window) {}
		void IWindowCallback::WindowMaximize(IWindow * window) {}
		void IWindowCallback::WindowMinimize(IWindow * window) {}
		void IWindowCallback::WindowRestore(IWindow * window) {}
		void IWindowCallback::WindowHelp(IWindow * window) {}
		void IWindowCallback::WindowActivate(IWindow * window) {}
		void IWindowCallback::WindowDeactivate(IWindow * window) {}
		void IWindowCallback::WindowMove(IWindow * window) {}
		void IWindowCallback::WindowSize(IWindow * window) {}
		void IWindowCallback::FocusChanged(IWindow * window, bool got) {}
		bool IWindowCallback::KeyDown(IWindow * window, int key_code) { return false; }
		void IWindowCallback::KeyUp(IWindow * window, int key_code) {}
		void IWindowCallback::CharDown(IWindow * window, uint32 ucs_code) {}
		void IWindowCallback::CaptureChanged(IWindow * window, bool got) {}
		void IWindowCallback::SetCursor(IWindow * window, UI::Point at) { GetWindowSystem()->SetCursor(GetWindowSystem()->GetSystemCursor(SystemCursorClass::Arrow)); }
		void IWindowCallback::MouseMove(IWindow * window, UI::Point at) {}
		void IWindowCallback::LeftButtonDown(IWindow * window, UI::Point at) {}
		void IWindowCallback::LeftButtonUp(IWindow * window, UI::Point at) {}
		void IWindowCallback::LeftButtonDoubleClick(IWindow * window, UI::Point at) {}
		void IWindowCallback::RightButtonDown(IWindow * window, UI::Point at) {}
		void IWindowCallback::RightButtonUp(IWindow * window, UI::Point at) {}
		void IWindowCallback::RightButtonDoubleClick(IWindow * window, UI::Point at) {}
		void IWindowCallback::ScrollVertically(IWindow * window, double delta) {}
		void IWindowCallback::ScrollHorizontally(IWindow * window, double delta) {}
		void IWindowCallback::Timer(IWindow * window, int timer_id) {}
		void IWindowCallback::ThemeChanged(IWindow * window) {}
		bool IWindowCallback::IsWindowEventEnabled(IWindow * window, WindowHandler handler) { return false; }
		void IWindowCallback::HandleWindowEvent(IWindow * window, WindowHandler handler) {}

		bool IApplicationCallback::IsHandlerEnabled(ApplicationHandler event) { return false; }
		bool IApplicationCallback::IsWindowEventAccessible(WindowHandler handler) { return false; }
		void IApplicationCallback::CreateNewFile(void) {}
		void IApplicationCallback::OpenSomeFile(void) {}
		bool IApplicationCallback::OpenExactFile(const string & path) { return false; }
		void IApplicationCallback::ShowHelp(void) {}
		void IApplicationCallback::ShowAbout(void) {}
		void IApplicationCallback::ShowProperties(void) {}
		void IApplicationCallback::HotKeyEvent(int event_id) {}
		bool IApplicationCallback::DataExchangeReceive(handle client, const string & verb, const DataBlock * data) { return false; }
		DataBlock * IApplicationCallback::DataExchangeRespond(handle client, const string & verb) { return 0; }
		void IApplicationCallback::DataExchangeDisconnect(handle client) {}
		bool IApplicationCallback::Terminate(void) { return true; }

		void IMenuItemCallback::MenuItemDisposed(IMenuItem * item) {}

		void IStatusCallback::StatusIconCommand(IStatusBarIcon * icon, int id) {}
	}
}