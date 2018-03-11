#include "ControlBase.h"

extern Engine::SafePointer<Engine::Streaming::TextWriter> conout;

namespace Engine
{
	namespace UI
	{
		Window::Window(Window * parent, WindowStation * station) { Parent.SetRetain(parent); Station.SetRetain(station); WindowPosition = Box(0, 0, 0, 0); }
		Window::~Window(void) {}
		void Window::Render(const Box & at) {}
		void Window::ResetCache(void) { for (int i = 0; i < Children.Length(); i++) Children[i].ResetCache(); }
		void Window::ArrangeChildren(void) {}
		void Window::Enable(bool enable) {}
		bool Window::IsEnabled(void) { return true; }
		void Window::Show(bool visible) {}
		bool Window::IsVisible(void) { return true; }
		bool Window::IsTabStop(void) { return false; }
		bool Window::IsOverlapped(void) { return false; }
		void Window::SetID(int ID) {}
		int Window::GetID(void) { return 0; }
		Window * Window::FindChild(int ID) { return 0; }
		void Window::SetRectangle(const Rectangle & rect) {}
		Rectangle Window::GetRectangle(void) { return Rectangle::Invalid(); }
		void Window::SetPosition(const Box & box) { WindowPosition = box; }
		Box Window::GetPosition(void) { return WindowPosition; }
		void Window::SetText(const string & text) {}
		string Window::GetText(void) { return L""; }
		void Window::RaiseEvent(int ID, Event event, Window * sender) { if (Parent) Parent->RaiseEvent(ID, event, sender); }
		void Window::FocusChanged(bool got_focus) {}
		void Window::CaptureChanged(bool got_capture) {}
		void Window::LeftButtonDown(Point at) {}
		void Window::LeftButtonUp(Point at) {}
		void Window::LeftButtonDoubleClick(Point at) {}
		void Window::RightButtonDown(Point at) {}
		void Window::RightButtonUp(Point at) {}
		void Window::RightButtonDoubleClick(Point at) {}
		void Window::MouseMove(Point at) {}
		void Window::ScrollVertically(int delta) { if (Parent) Parent->ScrollVertically(delta); }
		void Window::ScrollHorizontally(int delta) { if (Parent) Parent->ScrollHorizontally(delta); }
		void Window::KeyDown(int key_code) {}
		void Window::KeyUp(int key_code) {}
		void Window::CharDown(uint32 ucs_code) {}
		Window * Window::HitTest(Point at) { return this; }
		Window * Window::GetParent(void) { return Parent; }
		WindowStation * Window::GetStation(void) { return Station; }
		void Window::SetOrder(DepthOrder order)
		{
			if (!Parent) return;
			int index = -1;
			for (int i = 0; i < Parent->Children.Length(); i++) if (Parent->Children.ElementAt(i) == this) { index = i; break; }
			if (index == -1) return;
			if (order == DepthOrder::SetFirst) {
				for (int i = index + 1; i < Parent->Children.Length(); i++) Parent->Children.SwapAt(i - 1, i);
			} else if (order == DepthOrder::SetLast) {
				for (int i = index - 1; i >= 0; i--) Parent->Children.SwapAt(i + 1, i);
			} else if (order == DepthOrder::MoveUp) {
				if (index < Parent->Children.Length() - 1) Parent->Children.SwapAt(index, index + 1);
			} else if (order == DepthOrder::MoveDown) {
				if (index > 0) Parent->Children.SwapAt(index, index - 1);
			}
		}
		int Window::ChildrenCount(void) { return Children.Length(); }
		Window * Window::Child(int index)
		{
			if (index < 0 || index >= Children.Length()) return 0;
			return &Children[index];
		}
		void Window::SetFocus(void) { Station->SetFocus(this); }
		Window * Window::GetFocus(void) { return Station->GetFocus(); }
		void Window::SetCapture(void) { Station->SetCapture(this); }
		Window * Window::GetCapture(void) { return Station->GetCapture(); }
		void Window::ReleaseCapture(void) { Station->ReleaseCapture(); }
		void Window::Destroy(void) { Station->DestroyWindow(this); }

		void WindowStation::DeconstructChain(Window * window)
		{
			window->Parent.SetReference(0);
			window->Station.SetReference(0);
			for (int i = 0; i < window->Children.Length(); i++) DeconstructChain(&window->Children[i]);
			if (CaptureWindow.Inner() == window) ReleaseCapture();
			if (FocusedWindow.Inner() == window) SetFocus(TopLevelWindow);
		}
		WindowStation::WindowStation(void) : Position(0, 0, 0, 0) { TopLevelWindow.SetReference(new Engine::UI::TopLevelWindow(0, this)); }
		WindowStation::~WindowStation(void) { if (TopLevelWindow.Inner()) DeconstructChain(TopLevelWindow); }
		void WindowStation::DestroyWindow(Window * window)
		{
			if (!window->Parent.Inner()) return;
			DeconstructChain(window);
			for (int i = 0; i < window->Parent->Children.Length(); i++) if (window->Parent->Children.ElementAt(i) == window) {
				window->Parent->Children.Remove(i);
				break;
			}
		}
		void WindowStation::SetBox(const Box & box) { Position = box; if (TopLevelWindow) { TopLevelWindow->WindowPosition = box; TopLevelWindow->ArrangeChildren(); } }
		Box WindowStation::GetBox(void) { return Position; }
		void WindowStation::Render(void) { if (TopLevelWindow.Inner()) TopLevelWindow->Render(Position); }
		void WindowStation::ResetCache(void) { if (TopLevelWindow.Inner()) TopLevelWindow->ResetCache(); }
		Engine::UI::TopLevelWindow * WindowStation::GetDesktop(void) { return TopLevelWindow; }
		Window * WindowStation::HitTest(Point at) { if (TopLevelWindow.Inner()) return TopLevelWindow->HitTest(at); else return 0; }
		Window * WindowStation::EnabledHitTest(Point at)
		{
			Window * target = (TopLevelWindow.Inner()) ? TopLevelWindow->HitTest(at) : 0;
			while (target && !target->IsEnabled()) target = target->Parent;
			return target;
		}
		void WindowStation::SetRenderingDevice(IRenderingDevice * Device) { RenderingDevice.SetRetain(Device); }
		IRenderingDevice * WindowStation::GetRenderingDevice(void) { return RenderingDevice; }
		Point WindowStation::CalculateLocalPoint(Window * window, Point global)
		{
			Point result = global;
			Window * current = window;
			while (current) {
				result.x -= current->WindowPosition.Left;
				result.y -= current->WindowPosition.Top;
				current = current->Parent;
			}
			return result;
		}
		Point WindowStation::CalculateGlobalPoint(Window * window, Point local)
		{
			Point result = local;
			Window * current = window;
			while (current) {
				result.x += current->WindowPosition.Left;
				result.y += current->WindowPosition.Top;
				current = current->Parent;
			}
			return result;
		}
		void WindowStation::GetMouseTarget(Point global, Window ** target, Point * local)
		{
			Window * Target = EnabledHitTest(global);
			*target = Target; *local = CalculateLocalPoint(Target, global);
		}
		void WindowStation::SetFocus(Window * window)
		{
			if (window == FocusedWindow.Inner()) return;
			if (FocusedWindow.Inner()) FocusedWindow->FocusChanged(false);
			FocusedWindow.SetRetain(window);
			if (FocusedWindow.Inner()) FocusedWindow->FocusChanged(true);
		}
		Window * WindowStation::GetFocus(void)
		{
			return FocusedWindow;
		}
		void WindowStation::SetCapture(Window * window)
		{
			if (window == CaptureWindow.Inner()) return;
			if (CaptureWindow.Inner()) CaptureWindow->CaptureChanged(false);
			CaptureWindow.SetRetain(window);
			if (CaptureWindow.Inner()) {
				CaptureWindow->CaptureChanged(true);
			}
			MouseMove(GetCursorPos());
		}
		Window * WindowStation::GetCapture(void) { return CaptureWindow; }
		void WindowStation::ReleaseCapture(void) { SetCapture(0); }
		void WindowStation::FocusChanged(bool got_focus)
		{
			if (!got_focus) {
				if (FocusedWindow.Inner()) FocusedWindow->FocusChanged(false);
				FocusedWindow.SetReference(0);
			}
		}
		void WindowStation::CaptureChanged(bool got_capture)
		{
			if (!got_capture) {
				if (CaptureWindow.Inner()) CaptureWindow->CaptureChanged(false);
				CaptureWindow.SetReference(0);
			}
		}
		void WindowStation::LeftButtonDown(Point at)
		{
			if (CaptureWindow) CaptureWindow->LeftButtonDown(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->LeftButtonDown(At);
			}
		}
		void WindowStation::LeftButtonUp(Point at)
		{
			if (CaptureWindow) CaptureWindow->LeftButtonUp(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->LeftButtonUp(At);
			}
		}
		void WindowStation::LeftButtonDoubleClick(Point at)
		{
			if (CaptureWindow) CaptureWindow->LeftButtonDoubleClick(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->LeftButtonDoubleClick(At);
			}
		}
		void WindowStation::RightButtonDown(Point at)
		{
			if (CaptureWindow) CaptureWindow->RightButtonDown(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->RightButtonDown(At);
			}
		}
		void WindowStation::RightButtonUp(Point at)
		{
			if (CaptureWindow) CaptureWindow->RightButtonUp(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->RightButtonUp(At);
			}
		}
		void WindowStation::RightButtonDoubleClick(Point at)
		{
			if (CaptureWindow) CaptureWindow->RightButtonDoubleClick(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->RightButtonDoubleClick(At);
			}
		}
		void WindowStation::MouseMove(Point at)
		{
			if (CaptureWindow) CaptureWindow->MouseMove(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->MouseMove(At);
			}
		}
		void WindowStation::ScrollVertically(int delta)
		{
			if (CaptureWindow) CaptureWindow->ScrollVertically(delta);
			else {
				Window * Target = EnabledHitTest(GetCursorPos());
				if (Target) Target->ScrollVertically(delta);
			}
		}
		void WindowStation::ScrollHorizontally(int delta)
		{
			if (CaptureWindow) CaptureWindow->ScrollHorizontally(delta);
			else {
				Window * Target = EnabledHitTest(GetCursorPos());
				if (Target) Target->ScrollHorizontally(delta);
			}
		}

		void WindowStation::KeyDown(int key_code) { if (FocusedWindow) FocusedWindow->KeyDown(key_code); }
		void WindowStation::KeyUp(int key_code) { if (FocusedWindow) FocusedWindow->KeyUp(key_code); }
		void WindowStation::CharDown(uint32 ucs_code) { if (FocusedWindow) FocusedWindow->CharDown(ucs_code); }
		Point WindowStation::GetCursorPos(void) { return Point(0, 0); }

		ParentWindow::ParentWindow(Window * parent, WindowStation * station) : Window(parent, station) {}
		Window * ParentWindow::FindChild(int ID)
		{
			if (!ID) return 0;
			if (ID == GetID()) return this;
			for (int i = 0; i < Children.Length(); i++) {
				auto result = Children[i].FindChild(ID);
				if (result) return result;
			}
			return 0;
		}
		void ParentWindow::Render(const Box & at)
		{
			auto Device = Station->GetRenderingDevice();
			for (int i = 0; i < Children.Length(); i++) {
				auto & child = Children[i];
				if (!child.IsVisible()) continue;
				Box rect = Box(child.WindowPosition.Left + at.Left, child.WindowPosition.Top + at.Top, child.WindowPosition.Right + at.Left, child.WindowPosition.Bottom + at.Top);
				Device->PushClip(rect);
				child.Render(rect);
				Device->PopClip();
			}
		}
		void ParentWindow::ArrangeChildren(void)
		{
			for (int i = 0; i < Children.Length(); i++) {
				Box inner = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				auto rect = Children[i].GetRectangle();
				if (rect.IsValid()) {
					Children[i].SetPosition(Box(rect, inner));
				}
			}
		}
		void ParentWindow::SetPosition(const Box & box) { Window::SetPosition(box); ArrangeChildren(); }
		Window * ParentWindow::HitTest(Point at)
		{
			for (int i = Children.Length() - 1; i >= 0; i--) {
				if (!Children[i].IsVisible()) continue;
				auto & box = Children[i].WindowPosition;
				if (box.IsInside(at)) {
					return Children[i].HitTest(Point(at.x - box.Left, at.y - box.Top));
				}
			}
			return this;
		}

		TopLevelWindow::TopLevelWindow(Window * parent, WindowStation * station) : ParentWindow(parent, station) {}
		TopLevelWindow::~TopLevelWindow(void) {}
		void TopLevelWindow::Render(const Box & at)
		{
			if (!BackgroundShape && Background) {
				BackgroundShape.SetReference(Background->Initialize(&ZeroArgumentProvider()));
			}
			if (BackgroundShape) {
				BackgroundShape->Render(GetStation()->GetRenderingDevice(), at);
			}
			ParentWindow::Render(at);
		}
		void TopLevelWindow::ResetCache(void) { BackgroundShape.SetReference(0); }
		Rectangle TopLevelWindow::GetRectangle(void) { return Rectangle::Entire(); }
		Box TopLevelWindow::GetPosition(void) { return GetStation()->GetBox(); }

		bool TopLevelWindow::IsOverlapped(void) { return true; }

		void TopLevelWindow::RaiseEvent(int ID, Event event, Window * sender)
		{
			(*conout) << L"DEBUG: Event with ID = " << ID << L", sender = " << string(static_cast<handle>(sender)) << IO::NewLineChar;
		}

		ZeroArgumentProvider::ZeroArgumentProvider(void) {}
		void ZeroArgumentProvider::GetArgument(const string & name, int * value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, double * value) { *value = 0.0; }
		void ZeroArgumentProvider::GetArgument(const string & name, Color * value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, string * value) { *value = L""; }
		void ZeroArgumentProvider::GetArgument(const string & name, ITexture ** value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, IFont ** value) { *value = 0; }
	}
}