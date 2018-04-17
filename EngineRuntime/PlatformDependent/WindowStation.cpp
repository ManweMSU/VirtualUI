#include "WindowStation.h"

#include "KeyCodes.h"
#include "../ImageCodec/CodecBase.h"

#undef ZeroMemory

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
		HandleWindowStation::HandleWindowStation(HWND window, IDesktopWindowFactory * Factory) : WindowStation(Factory), _window(window)
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
		HandleWindowStation::HandleWindowStation(HWND window) : _window(window)
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
			SafePointer<Codec::Frame> Conv = Source->ConvertFormat(Codec::FrameFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaFormat::Normal, Codec::LineDirection::BottomUp));
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
				} else KeyDown(int32(WParam));
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
				ScrollVertically(-GET_WHEEL_DELTA_WPARAM(WParam) * 3 / WHEEL_DELTA);
			} else if (Msg == WM_MOUSEHWHEEL) {
				ScrollHorizontally(GET_WHEEL_DELTA_WPARAM(WParam) * 3 / WHEEL_DELTA);
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