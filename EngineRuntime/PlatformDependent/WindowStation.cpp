#include "WindowStation.h"

#include "KeyCodes.h"
#include "../ImageCodec/CodecBase.h"

#undef ZeroMemory

#define ERTM_DESTROYWINDOW	(WM_USER + 0x001)
#define ERTM_RAISEEVENT		(WM_USER + 0x002)
#define ERTM_EXECUTEJOB		(WM_USER + 0x003)

namespace Engine
{
	namespace UI
	{
		namespace HandleWindowStationHelper
		{
			class WindowsCursor : public ICursor
			{
			public:
				HCURSOR Handle;
				bool Owned;
				WindowsCursor(HCURSOR handle, bool take_own = false) : Handle(handle), Owned(take_own) {}
				~WindowsCursor(void) override { if (Owned) DestroyCursor(Handle); }
			};
		}
		HandleWindowStation::HandleWindowStation(HWND window, IDesktopWindowFactory * Factory) : WindowStation(Factory), _window(window), _timers(0x10), _clear_background(false)
		{
			_arrow.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_ARROW)));
			_beam.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_IBEAM)));
			_link.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_HAND)));
			_size_left_right.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZEWE)));
			_size_up_down.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZENS)));
			_size_left_up_right_down.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZENWSE)));
			_size_left_down_right_up.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZENESW)));
			_size_all.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZEALL)));
			_fx_blur_behind = false;
			_fx_margins.cxLeftWidth = _fx_margins.cxRightWidth = _fx_margins.cyBottomHeight = _fx_margins.cyTopHeight = 0;
		}
		void HandleWindowStation::_reset_dwm(void)
		{
			BOOL dwm_enabled;
			if (DwmIsCompositionEnabled(&dwm_enabled) != S_OK) return;
			if (dwm_enabled) {
				DWM_BLURBEHIND blur;
				blur.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
				blur.fEnable = _fx_blur_behind;
				blur.hRgnBlur = 0;
				DwmExtendFrameIntoClientArea(_window, &_fx_margins);
				DwmEnableBlurBehindWindow(_window, &blur);
				_fx_clear_background = Color(0, 0, 0, 0);
			} else {
				auto rgb = GetSysColor(COLOR_BTNFACE);
				_fx_clear_background = Color(uint8(GetRValue(rgb)), uint8(GetGValue(rgb)), uint8(GetBValue(rgb)), uint(255));
			}
			_clear_background = _fx_blur_behind || _fx_margins.cxLeftWidth || _fx_margins.cyTopHeight || _fx_margins.cxRightWidth || _fx_margins.cyBottomHeight;
		}
		HandleWindowStation::HandleWindowStation(HWND window) : _window(window), _timers(0x10), _clear_background(false)
		{
			_arrow.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_ARROW)));
			_beam.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_IBEAM)));
			_link.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_HAND)));
			_size_left_right.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZEWE)));
			_size_up_down.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZENS)));
			_size_left_up_right_down.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZENWSE)));
			_size_left_down_right_up.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZENESW)));
			_size_all.SetReference(new HandleWindowStationHelper::WindowsCursor(LoadCursorW(0, IDC_SIZEALL)));
		}
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
		void HandleWindowStation::SetCursorPos(Point pos)
		{
			POINT p; p.x = pos.x; p.y = pos.y;
			ClientToScreen(_window, &p);
			::SetCursorPos(p.x, p.y);
		}
		bool HandleWindowStation::NativeHitTest(const Point & at)
		{
			POINT p = { at.x, at.y };
			ClientToScreen(_window, &p);
			return (WindowFromPoint(p) == _window);
		}
		ICursor * HandleWindowStation::LoadCursor(Streaming::Stream * Source)
		{
			SafePointer<Codec::Image> Image = Codec::DecodeImage(Source);
			if (Image) { return LoadCursor(Image); } else throw InvalidArgumentException();
		}
		ICursor * HandleWindowStation::LoadCursor(Codec::Image * Source) { return LoadCursor(Source->GetFrameBestDpiFit(UI::Zoom)); }
		ICursor * HandleWindowStation::LoadCursor(Codec::Frame * Source)
		{
			SafePointer<Codec::Frame> Conv = Source->ConvertFormat(Codec::FrameFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::BottomUp));
			BITMAPINFOHEADER hdr;
			Array<uint32> Fake(0x100);
			Fake.SetLength(Conv->GetWidth() * Conv->GetHeight());
			ZeroMemory(Fake.GetBuffer(), 4 * Conv->GetWidth() * Conv->GetHeight());
			ZeroMemory(&hdr, sizeof(hdr));
			hdr.biSize = sizeof(hdr);
			hdr.biWidth = Conv->GetWidth();
			hdr.biHeight = Conv->GetHeight();
			hdr.biBitCount = 32;
			hdr.biPlanes = 1;
			hdr.biSizeImage = 4 * Conv->GetWidth() * Conv->GetHeight();
			HDC DC = GetDC(0);
			HBITMAP ColorMap = CreateDIBitmap(DC, &hdr, CBM_INIT, Conv->GetData(), reinterpret_cast<LPBITMAPINFO>(&hdr), DIB_RGB_COLORS);
			HBITMAP MaskMap = CreateDIBitmap(DC, &hdr, CBM_INIT, Fake.GetBuffer(), reinterpret_cast<LPBITMAPINFO>(&hdr), DIB_RGB_COLORS);
			ICONINFO Icon;
			Icon.fIcon = FALSE;
			Icon.hbmColor = ColorMap;
			Icon.hbmMask = MaskMap;
			Icon.xHotspot = Conv->HotPointX;
			Icon.yHotspot = Conv->HotPointY;
			HCURSOR Cursor = CreateIconIndirect(&Icon);
			DeleteObject(ColorMap);
			DeleteObject(MaskMap);
			ReleaseDC(0, DC);
			return new HandleWindowStationHelper::WindowsCursor(Cursor, true);
		}
		ICursor * HandleWindowStation::GetSystemCursor(SystemCursor cursor)
		{
			if (cursor == SystemCursor::Null) return _null;
			else if (cursor == SystemCursor::Arrow) return _arrow;
			else if (cursor == SystemCursor::Beam) return _beam;
			else if (cursor == SystemCursor::Link) return _link;
			else if (cursor == SystemCursor::SizeLeftRight) return _size_left_right;
			else if (cursor == SystemCursor::SizeUpDown) return _size_up_down;
			else if (cursor == SystemCursor::SizeLeftUpRightDown) return _size_left_up_right_down;
			else if (cursor == SystemCursor::SizeLeftDownRightUp) return _size_left_down_right_up;
			else if (cursor == SystemCursor::SizeAll) return _size_all;
			else return 0;
		}
		void HandleWindowStation::SetSystemCursor(SystemCursor entity, ICursor * cursor)
		{
			if (entity == SystemCursor::Null) _null.SetRetain(cursor);
			else if (entity == SystemCursor::Arrow) _arrow.SetRetain(cursor);
			else if (entity == SystemCursor::Beam) _beam.SetRetain(cursor);
			else if (entity == SystemCursor::Link) _link.SetRetain(cursor);
			else if (entity == SystemCursor::SizeLeftRight) _size_left_right.SetRetain(cursor);
			else if (entity == SystemCursor::SizeUpDown) _size_up_down.SetRetain(cursor);
			else if (entity == SystemCursor::SizeLeftUpRightDown) _size_left_up_right_down.SetRetain(cursor);
			else if (entity == SystemCursor::SizeLeftDownRightUp) _size_left_down_right_up.SetRetain(cursor);
			else if (entity == SystemCursor::SizeAll) _size_all.SetRetain(cursor);
		}
		void HandleWindowStation::SetCursor(ICursor * cursor) { if (cursor) ::SetCursor(static_cast<HandleWindowStationHelper::WindowsCursor *>(cursor)->Handle); }
		void HandleWindowStation::SetTimer(Window * window, uint32 period)
		{
			if (period) {
				int entry = -1;
				for (int i = 0; i < _timers.Length(); i++) {
					if (_timers[i] == window) { entry = i; break; }
					else if (!_timers[i]) { entry = i; _timers[i] = window; break; }
				}
				if (entry == -1) {
					entry = _timers.Length();
					_timers << window;
				}
				::SetTimer(_window, entry + 2, period, 0);
			} else {
				int max_valid = -1;
				int entry = -1;
				for (int i = 0; i < _timers.Length(); i++) {
					if (_timers[i]) max_valid = i;
					if (_timers[i] == window) { entry = i; break; }
				}
				if (entry != -1) {
					::KillTimer(_window, entry + 2);
					_timers[entry] = 0;
					if (entry == _timers.Length() - 1) _timers.SetLength(max_valid + 1);
				}
			}
		}
		void HandleWindowStation::DeferredDestroy(Window * window) { PostMessageW(_window, ERTM_DESTROYWINDOW, reinterpret_cast<WPARAM>(window), 0); }
		void HandleWindowStation::DeferredRaiseEvent(Window * window, int ID) { PostMessageW(_window, ERTM_RAISEEVENT, reinterpret_cast<WPARAM>(window), eint(ID)); }
		void HandleWindowStation::PostJob(Tasks::ThreadJob * job) { job->Retain(); PostMessageW(_window, ERTM_EXECUTEJOB, 0, reinterpret_cast<eint>(job)); }
		handle HandleWindowStation::GetOSHandle(void) { return _window; }
		HWND HandleWindowStation::Handle(void) { return _window; }
		bool & HandleWindowStation::ClearBackgroundFlag(void) { return _clear_background; }
		Color HandleWindowStation::GetClearBackgroundColor(void) { return _fx_clear_background; }
		void HandleWindowStation::SetFrameMargins(int left, int top, int right, int bottom)
		{
			_fx_margins.cxLeftWidth = left;
			_fx_margins.cyTopHeight = top;
			_fx_margins.cxRightWidth = right;
			_fx_margins.cyBottomHeight = bottom;
			_reset_dwm();
		}
		void HandleWindowStation::SetBlurBehind(bool enable)
		{
			_fx_blur_behind = enable;
			_reset_dwm();
		}
		eint HandleWindowStation::ProcessWindowEvents(uint32 Msg, eint WParam, eint LParam)
		{
			if (Msg == WM_DWMCOMPOSITIONCHANGED) {
				_reset_dwm();
				return 0;
			} else if (Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN) {
				bool processed;
				if (WParam == VK_SHIFT) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) processed = KeyDown(KeyCodes::LeftShift);
					else processed = KeyDown(KeyCodes::RightShift);
				} else if (WParam == VK_CONTROL) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) processed = KeyDown(KeyCodes::LeftControl);
					else processed = KeyDown(KeyCodes::RightControl);
				} else if (WParam == VK_MENU) {
					if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) processed = KeyDown(KeyCodes::LeftAlternative);
					else processed = KeyDown(KeyCodes::RightAlternative);
				} else processed = KeyDown(int32(WParam));
				DefWindowProcW(_window, Msg, WParam, LParam);
				if (processed) return 0; else return 1;
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
				} else KeyUp(int32(WParam));
				DefWindowProcW(_window, Msg, WParam, LParam);
				return 0;
			} else if (Msg == WM_CHAR) {
				if ((WParam & 0xFC00) == 0xD800) {
					_surrogate = ((WParam & 0x3FF) << 10) + 0x10000;
				} else if ((WParam & 0xFC00) == 0xDC00) {
					_surrogate |= (WParam & 0x3FF) + 0x10000;
					CharDown(_surrogate);
					_surrogate = 0;
				} else {
					_surrogate = 0;
					CharDown(uint32(WParam));
				}
				return FALSE;
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
				ScrollVertically(-double(GET_WHEEL_DELTA_WPARAM(WParam)) * 3.0 / double(WHEEL_DELTA));
			} else if (Msg == WM_MOUSEHWHEEL) {
				ScrollHorizontally(double(GET_WHEEL_DELTA_WPARAM(WParam)) * 3.0 / double(WHEEL_DELTA));
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
			} else if (Msg == WM_TIMER) {
				int index = int(WParam) - 2;
				if (index >= 0 && index < _timers.Length() && _timers[index]) _timers[index]->Timer();
			} else if (Msg == ERTM_DESTROYWINDOW) {
				Window * window = reinterpret_cast<Window *>(WParam);
				if (window) window->Destroy();
			} else if (Msg == ERTM_RAISEEVENT) {
				Window * window = reinterpret_cast<Window *>(WParam);
				if (window) {
					window->RaiseEvent(int(LParam), Window::Event::Deferred, 0);
					window->RequireRedraw();
				}
			} else if (Msg == ERTM_EXECUTEJOB) {
				Tasks::ThreadJob * job = reinterpret_cast<Tasks::ThreadJob *>(LParam);
				job->DoJob(0);
				job->Release();
			}
			return DefWindowProcW(_window, Msg, WParam, LParam);
		}
		HICON CreateWinIcon(Codec::Frame * Source)
		{
			SafePointer<Codec::Frame> Conv;
			if (Source->GetPixelFormat() != Codec::PixelFormat::B8G8R8A8 || Source->GetAlphaMode() != Codec::AlphaMode::Normal ||
				Source->GetScanOrigin() != Codec::ScanOrigin::BottomUp || Source->GetScanLineLength() != 4 * Source->GetWidth()) {
				Conv = Source->ConvertFormat(Codec::FrameFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::BottomUp));
			} else Conv.SetRetain(Source);
			BITMAPINFOHEADER hdr;
			Array<uint32> Fake(0x100);
			Fake.SetLength(Conv->GetWidth() * Conv->GetHeight());
			ZeroMemory(Fake.GetBuffer(), 4 * Conv->GetWidth() * Conv->GetHeight());
			ZeroMemory(&hdr, sizeof(hdr));
			hdr.biSize = sizeof(hdr);
			hdr.biWidth = Conv->GetWidth();
			hdr.biHeight = Conv->GetHeight();
			hdr.biBitCount = 32;
			hdr.biPlanes = 1;
			hdr.biSizeImage = 4 * Conv->GetWidth() * Conv->GetHeight();
			HDC DC = GetDC(0);
			HBITMAP ColorMap = CreateDIBitmap(DC, &hdr, CBM_INIT, Conv->GetData(), reinterpret_cast<LPBITMAPINFO>(&hdr), DIB_RGB_COLORS);
			HBITMAP MaskMap = CreateDIBitmap(DC, &hdr, CBM_INIT, Fake.GetBuffer(), reinterpret_cast<LPBITMAPINFO>(&hdr), DIB_RGB_COLORS);
			ICONINFO Icon;
			Icon.fIcon = TRUE;
			Icon.hbmColor = ColorMap;
			Icon.hbmMask = MaskMap;
			Icon.xHotspot = 0;
			Icon.yHotspot = 0;
			HICON IconHandle = CreateIconIndirect(&Icon);
			DeleteObject(ColorMap);
			DeleteObject(MaskMap);
			ReleaseDC(0, DC);
			return IconHandle;
		}
	}
}