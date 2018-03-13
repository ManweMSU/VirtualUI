#include "WindowStation.h"

#include "KeyCodes.h"

namespace Engine
{
	namespace UI
	{
		HandleWindowStation::HandleWindowStation(HWND window) : _window(window) {}
		HandleWindowStation::~HandleWindowStation(void) {}
		void HandleWindowStation::SetFocus(Window * window) { if (::SetFocus(_window)) WindowStation::SetFocus(window); }
		Window * HandleWindowStation::GetFocus(void) { if (::GetFocus() == _window) return WindowStation::GetFocus(); else return 0; }
		void HandleWindowStation::SetCapture(Window * window) { if (window) ::SetCapture(_window); else if (!WindowStation::GetExclusiveWindow()) ::ReleaseCapture(); WindowStation::SetCapture(window); }
		Window * HandleWindowStation::GetCapture(void) { if (::GetCapture() == _window) return WindowStation::GetCapture(); else return 0; }
		void HandleWindowStation::ReleaseCapture(void) { if (!WindowStation::GetExclusiveWindow()) ::ReleaseCapture(); WindowStation::SetCapture(0); }
		void HandleWindowStation::SetExclusiveWindow(Window * window)
		{
			if (window) {
				::SetCapture(_window);
				WindowStation::SetExclusiveWindow(window);
			} else {
				if (!WindowStation::GetCapture()) ::ReleaseCapture();
				WindowStation::SetExclusiveWindow(0);
			}
		}
		Window * HandleWindowStation::GetExclusiveWindow(void) { if (::GetCapture() == _window) return WindowStation::GetExclusiveWindow(); else return 0; }
		Point HandleWindowStation::GetCursorPos(void)
		{
			POINT p;
			::GetCursorPos(&p);
			ScreenToClient(_window, &p);
			return Point(p.x, p.y);
		}
		eint HandleWindowStation::ProcessWindowEvents(uint32 Msg, eint WParam, eint LParam)
		{
			if (Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN) {
				if (WParam == VK_SHIFT) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyDown(KeyCodes::LeftShift);
					else KeyDown(KeyCodes::RightShift);
				} else if (WParam == VK_CONTROL) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyDown(KeyCodes::LeftControl);
					else KeyDown(KeyCodes::RightControl);
				} else if (WParam == VK_MENU) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyDown(KeyCodes::LeftAlternative);
					else KeyDown(KeyCodes::RightAlternative);
				} else KeyDown(WParam);
			} else if (Msg == WM_KEYUP || Msg == WM_SYSKEYUP) {
				if (WParam == VK_SHIFT) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyUp(KeyCodes::LeftShift);
					else KeyUp(KeyCodes::RightShift);
				} else if (WParam == VK_CONTROL) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyUp(KeyCodes::LeftControl);
					else KeyUp(KeyCodes::RightControl);
				} else if (WParam == VK_MENU) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyUp(KeyCodes::LeftAlternative);
					else KeyUp(KeyCodes::RightAlternative);
				} else KeyUp(WParam);
			} else if (Msg == WM_UNICHAR) {
				if (WParam == UNICODE_NOCHAR) return TRUE;
				else {
					CharDown(uint32(WParam));
					return FALSE;
				}
			} else if (Msg == WM_MOUSEMOVE) {
				POINTS p = MAKEPOINTS(LParam);
				MouseMove(Point(p.x, p.y));
			} else if (Msg == WM_LBUTTONDOWN) {
				POINTS p = MAKEPOINTS(LParam);
				LeftButtonDown(Point(p.x, p.y));
			} else if (Msg == WM_LBUTTONUP) {
				POINTS p = MAKEPOINTS(LParam);
				LeftButtonUp(Point(p.x, p.y));
			} else if (Msg == WM_LBUTTONDBLCLK) {
				POINTS p = MAKEPOINTS(LParam);
				LeftButtonDoubleClick(Point(p.x, p.y));
			} else if (Msg == WM_RBUTTONDOWN) {
				POINTS p = MAKEPOINTS(LParam);
				RightButtonDown(Point(p.x, p.y));
			} else if (Msg == WM_RBUTTONUP) {
				POINTS p = MAKEPOINTS(LParam);
				RightButtonUp(Point(p.x, p.y));
			} else if (Msg == WM_RBUTTONDBLCLK) {
				POINTS p = MAKEPOINTS(LParam);
				RightButtonDoubleClick(Point(p.x, p.y));
			} else if (Msg == WM_MOUSEWHEEL) {
				ScrollVertically(GET_WHEEL_DELTA_WPARAM(WParam));
			} else if (Msg == WM_MOUSEHWHEEL) {
				ScrollHorizontally(GET_WHEEL_DELTA_WPARAM(WParam));
			} else if (Msg == WM_SIZE) {
				RECT Rect;
				GetClientRect(_window, &Rect);
				if (Rect.right != 0 && Rect.bottom != 0) SetBox(Box(0, 0, Rect.right, Rect.bottom));
			} else if (Msg == WM_SETFOCUS) {
				FocusChanged(true);
			} else if (Msg == WM_KILLFOCUS) {
				FocusChanged(false);
			} else if (Msg == WM_CAPTURECHANGED) {
				if (reinterpret_cast<HWND>(LParam) != _window) CaptureChanged(false);
				else CaptureChanged(true);
			}
			return DefWindowProcW(_window, Msg, WParam, LParam);
		}
	}
}