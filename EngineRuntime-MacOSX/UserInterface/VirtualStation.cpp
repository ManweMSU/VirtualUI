#include "VirtualStation.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			VirtualStation::VirtualWindowStation::VirtualWindowStation(VirtualStation * host_window) : host(host_window) { host_station = host->GetStation(); }
			VirtualStation::VirtualWindowStation::~VirtualWindowStation(void) {}
			void VirtualStation::VirtualWindowStation::SetFocus(Window * window)
			{
				host_station->SetFocus(host);
				if (host_station->GetFocus() == host) WindowStation::SetFocus(window);
			}
			Window * VirtualStation::VirtualWindowStation::GetFocus(void) { if (host_station->GetFocus() == host) return WindowStation::GetFocus(); else return 0; }
			void VirtualStation::VirtualWindowStation::SetCapture(Window * window)
			{
				if (window) {
					if (host_station->GetCapture() != host) host_station->SetCapture(host);
				} else if (!WindowStation::GetExclusiveWindow() && host_station->GetCapture()) host_station->ReleaseCapture();
				WindowStation::SetCapture(window);
			}
			Window * VirtualStation::VirtualWindowStation::GetCapture(void) { if (host_station->GetCapture() == host) return WindowStation::GetCapture(); else return 0; }
			void VirtualStation::VirtualWindowStation::ReleaseCapture(void)
			{
				if (!WindowStation::GetExclusiveWindow() && host_station->GetCapture()) host_station->ReleaseCapture();
				WindowStation::SetCapture(0);
			}
			void VirtualStation::VirtualWindowStation::SetExclusiveWindow(Window * window)
			{
				if (window) {
					if (host_station->GetCapture() != host) host_station->SetCapture(host);
					WindowStation::SetExclusiveWindow(window);
				} else {
					if (!WindowStation::GetCapture() && host_station->GetCapture()) host_station->ReleaseCapture();
					WindowStation::SetExclusiveWindow(0);
				}
			}
			Window * VirtualStation::VirtualWindowStation::GetExclusiveWindow(void) { if (host_station->GetCapture() == host) return WindowStation::GetExclusiveWindow(); else return 0; }
			bool VirtualStation::VirtualWindowStation::NativeHitTest(const Point & at)
			{
				auto p = host->InnerToOuter(at);
				p = host_station->CalculateGlobalPoint(host, p);
				return host_station->HitTest(p) == host;
			}
			Point VirtualStation::VirtualWindowStation::GetCursorPos(void)
			{
				auto p = host_station->GetCursorPos();
				p = host_station->CalculateLocalPoint(host, p);
				p = host->OuterToInner(p);
				return p;
			}
			void VirtualStation::VirtualWindowStation::SetCursorPos(Point pos)
			{
				auto p = host->InnerToOuter(pos);
				p = host_station->CalculateGlobalPoint(host, p);
				host_station->SetCursorPos(p);
			}
			ICursor * VirtualStation::VirtualWindowStation::LoadCursor(Streaming::Stream * Source) { return host_station->LoadCursor(Source); }
			ICursor * VirtualStation::VirtualWindowStation::LoadCursor(Codec::Image * Source) { return host_station->LoadCursor(Source); }
			ICursor * VirtualStation::VirtualWindowStation::LoadCursor(Codec::Frame * Source) { return host_station->LoadCursor(Source); }
			ICursor * VirtualStation::VirtualWindowStation::GetSystemCursor(SystemCursor cursor) { return host_station->GetSystemCursor(cursor); }
			void VirtualStation::VirtualWindowStation::SetSystemCursor(SystemCursor entity, ICursor * cursor) { host_station->SetSystemCursor(entity, cursor); }
			void VirtualStation::VirtualWindowStation::SetCursor(ICursor * cursor) { host_station->SetCursor(cursor); }
			void VirtualStation::VirtualWindowStation::SetTimer(Window * window, uint32 period) { host_station->SetTimer(window, period); }
			void VirtualStation::VirtualWindowStation::RequireRefreshRate(Window::RefreshPeriod period) { host_station->RequireRefreshRate(period); }
			Window::RefreshPeriod VirtualStation::VirtualWindowStation::GetRefreshRate(void) { return host_station->GetRefreshRate(); }
			void VirtualStation::VirtualWindowStation::AnimationStateChanged(void) { host_station->AnimationStateChanged(); }
			void VirtualStation::VirtualWindowStation::FocusWindowChanged(void) { host_station->FocusWindowChanged(); }
			void VirtualStation::VirtualWindowStation::RequireRedraw(void) { host_station->RequireRedraw(); }
			void VirtualStation::VirtualWindowStation::DeferredDestroy(Window * window) { host_station->DeferredDestroy(window); }
			void VirtualStation::VirtualWindowStation::DeferredRaiseEvent(Window * window, int ID) { host_station->DeferredRaiseEvent(window, ID); }
			void VirtualStation::VirtualWindowStation::AppendTask(IDispatchTask * task) { host_station->AppendTask(task); }

			VirtualStation::VirtualStation(Window * Parent, WindowStation * Station) : _width(1), _height(1), _station(0),
				_enabled(true), _visible(true), _autosize(false), _render(false), _id(0), _rect(Rectangle::Invalid()), Window(Parent, Station)
			{
				_station = new VirtualWindowStation(this);
				SetDesktopDimensions(Point(_width, _height));
			}
			VirtualStation::VirtualStation(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : _station(0), Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"VirtualStation") throw InvalidArgumentException();
				auto & init = static_cast<Template::Controls::VirtualStation &>(*Template->Properties);
				_rect = init.ControlPosition;
				_width = max(init.Width, 1);
				_height = max(init.Height, 1);
				_autosize = init.Autosize;
				_render = _autosize && init.Render;
				_visible = !init.Invisible;
				_enabled = !init.Disabled;
				_id = init.ID;
				_station = new VirtualWindowStation(this);
				SetDesktopDimensions(Point(_width, _height));
				if (Template->Children.Length()) {
					auto child = Template->Children.FirstElement();
					if (child->Properties->GetTemplateClass() != L"VirtualStationData") throw InvalidArgumentException();
					auto & data = static_cast<Template::Controls::VirtualStationData &>(*child->Properties);
					auto & styles = _station->GetVisualStyles();
					styles.WindowActiveView = data.WindowActiveView;
					styles.WindowInactiveView = data.WindowInactiveView;
					styles.WindowSmallActiveView = data.WindowSmallActiveView;
					styles.WindowSmallInactiveView = data.WindowSmallInactiveView;
					styles.WindowDefaultBackground = data.WindowDefaultBackground;
					styles.MenuShadow = data.MenuShadow;
					styles.MenuBackground = data.MenuBackground;
					styles.MenuArrow = data.MenuArrow;
					styles.WindowCloseButton = data.WindowCloseButton;
					styles.WindowMaximizeButton = data.WindowMaximizeButton;
					styles.WindowMinimizeButton = data.WindowMinimizeButton;
					styles.WindowHelpButton = data.WindowHelpButton;
					styles.WindowSmallCloseButton = data.WindowSmallCloseButton;
					styles.WindowSmallMaximizeButton = data.WindowSmallMaximizeButton;
					styles.WindowSmallMinimizeButton = data.WindowSmallMinimizeButton;
					styles.WindowSmallHelpButton = data.WindowSmallHelpButton;
					styles.WindowFixedBorder = data.WindowFixedBorder;
					styles.WindowSizableBorder = data.WindowSizableBorder;
					styles.WindowCaptionHeight = data.WindowCaptionHeight;
					styles.WindowSmallCaptionHeight = data.WindowSmallCaptionHeight;
					styles.MenuBorder = data.MenuBorder;
					styles.CaretWidth = data.CaretWidth;
				}
			}
			VirtualStation::~VirtualStation(void) { if (_station) _station->DestroyStation(); }
			void VirtualStation::Render(const Box & at)
			{
				if (_render) {
					_station->SetRenderingDevice(GetStation()->GetRenderingDevice());
					_station->Animate();
					_station->Render(Point(at.Left, at.Top));
				}
			}
			void VirtualStation::ResetCache(void)
			{
				if (_render) {
					_station->SetRenderingDevice(GetStation()->GetRenderingDevice());
					_station->ResetCache();
				}
			}
			void VirtualStation::Enable(bool enable) { _enabled = enable; }
			bool VirtualStation::IsEnabled(void) { return _enabled; }
			void VirtualStation::Show(bool visible) { _visible = visible; }
			bool VirtualStation::IsVisible(void) { return _visible; }
			void VirtualStation::SetID(int ID) { _id = ID; }
			int VirtualStation::GetID(void) { return _id; }
			Window * VirtualStation::FindChild(int ID) { if (ID == _id) return this; else return 0; }
			void VirtualStation::SetRectangle(const Rectangle & rect) { _rect = rect; GetParent()->ArrangeChildren(); }
			Rectangle VirtualStation::GetRectangle(void) { return _rect; }
			void VirtualStation::SetPosition(const Box & box) { Window::SetPosition(box); if (_autosize) SetDesktopDimensions(Point(box.Right - box.Left, box.Bottom - box.Top)); }
			void VirtualStation::FocusChanged(bool got_focus) { _station->FocusChanged(got_focus); }
			void VirtualStation::CaptureChanged(bool got_capture) { _station->CaptureChanged(got_capture); }
			void VirtualStation::LeftButtonDown(Point at) { SetFocus(); auto p = OuterToInner(at); if (GetCapture() == this || _station->NativeHitTest(p)) _station->LeftButtonDown(p); }
			void VirtualStation::LeftButtonUp(Point at) { auto p = OuterToInner(at); if (GetCapture() == this || _station->NativeHitTest(p)) _station->LeftButtonUp(p); }
			void VirtualStation::LeftButtonDoubleClick(Point at) { auto p = OuterToInner(at); if (GetCapture() == this || _station->NativeHitTest(p)) _station->LeftButtonDoubleClick(p); }
			void VirtualStation::RightButtonDown(Point at) { SetFocus(); auto p = OuterToInner(at); if (GetCapture() == this || _station->NativeHitTest(p)) _station->RightButtonDown(p); }
			void VirtualStation::RightButtonUp(Point at) { auto p = OuterToInner(at); if (GetCapture() == this || _station->NativeHitTest(p)) _station->RightButtonUp(p); }
			void VirtualStation::RightButtonDoubleClick(Point at) { auto p = OuterToInner(at); if (GetCapture() == this || _station->NativeHitTest(p)) _station->RightButtonDoubleClick(p); }
			void VirtualStation::MouseMove(Point at) { auto p = OuterToInner(at); if (GetCapture() == this || _station->NativeHitTest(p)) _station->MouseMove(p); }
			void VirtualStation::ScrollVertically(double delta) { SetFocus(); _station->ScrollVertically(delta); }
			void VirtualStation::ScrollHorizontally(double delta) { SetFocus(); _station->ScrollHorizontally(delta); }
			bool VirtualStation::KeyDown(int key_code) { return _station->KeyDown(key_code); }
			void VirtualStation::KeyUp(int key_code) { _station->KeyUp(key_code); }
			void VirtualStation::CharDown(uint32 ucs_code) { _station->CharDown(ucs_code); }
			void VirtualStation::SetCursor(Point at) {}
			Window::RefreshPeriod VirtualStation::FocusedRefreshPeriod(void)
			{
				if (_station->IsPlayingAnimation()) return RefreshPeriod::Cinematic;
				auto focus = _station->GetFocus();
				if (focus) return focus->FocusedRefreshPeriod(); else return RefreshPeriod::None;
			}
			string VirtualStation::GetControlClass(void) { return L"VirtualStation"; }
			WindowStation * VirtualStation::GetInnerStation(void) { return _station; }
			Point VirtualStation::GetDesktopDimensions(void) { auto box = _station->GetBox(); return Point(box.Right - box.Left, box.Bottom - box.Top); }
			void VirtualStation::SetDesktopDimensions(Point size)
			{
				auto s = _autosize ? Point(WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top) : size;
				if (s.x < 1) s.x = 1; if (s.y < 1) s.y = 1;
				_width = s.x; _height = s.y;
				_station->SetBox(Box(0, 0, s.x, s.y));
			}
			bool VirtualStation::IsAutosize(void) { return _autosize; }
			void VirtualStation::UseAutosize(bool use) { _autosize = use; if (_autosize) SetDesktopDimensions(Point(0, 0)); else _render = false; }
			bool VirtualStation::IsStandardRendering(void) { return _render; }
			void VirtualStation::UseStandardRendering(bool use) { auto prev_rnd = _render; _render = _autosize && use; if (prev_rnd != _render) ResetCache(); }
			Point VirtualStation::InnerToOuter(Point p)
			{
				auto w = WindowPosition.Right - WindowPosition.Left, h = WindowPosition.Bottom - WindowPosition.Top;
				return Point(p.x * w / _width, p.y * h / _height);
			}
			Point VirtualStation::OuterToInner(Point p)
			{
				auto w = WindowPosition.Right - WindowPosition.Left, h = WindowPosition.Bottom - WindowPosition.Top;
				return Point(p.x * _width / w, p.y * _height / h);
			}
		}
	}
}