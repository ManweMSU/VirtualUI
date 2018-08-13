#include "ControlBase.h"

namespace Engine
{
	namespace UI
	{
		Window::Window(Window * parent, WindowStation * station) : Children(0x10) { Parent.SetRetain(parent); Station.SetRetain(station); WindowPosition = Box(0, 0, 0, 0); }
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
		void Window::LostExclusiveMode(void) {}
		void Window::LeftButtonDown(Point at) {}
		void Window::LeftButtonUp(Point at) {}
		void Window::LeftButtonDoubleClick(Point at) {}
		void Window::RightButtonDown(Point at) {}
		void Window::RightButtonUp(Point at) {}
		void Window::RightButtonDoubleClick(Point at) {}
		void Window::MouseMove(Point at) {}
		void Window::ScrollVertically(double delta) { if (Parent) Parent->ScrollVertically(delta); }
		void Window::ScrollHorizontally(double delta) { if (Parent) Parent->ScrollHorizontally(delta); }
		void Window::KeyDown(int key_code) {}
		void Window::KeyUp(int key_code) {}
		bool Window::TranslateAccelerators(int key_code) { return false; }
		void Window::CharDown(uint32 ucs_code) {}
		void Window::PopupMenuCancelled(void) {}
		void Window::Timer(void) {}
		Window * Window::HitTest(Point at) { return this; }
		void Window::SetCursor(Point at) { Station->SetCursor(Station->GetSystemCursor(SystemCursor::Arrow)); }
		Window::RefreshPeriod Window::FocusedRefreshPeriod(void) { return RefreshPeriod::None; }
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
		Box Window::GetAbsolutePosition(void)
		{
			Box result = WindowPosition;
			Window * current = Parent;
			do {
				result.Left += current->WindowPosition.Left;
				result.Top += current->WindowPosition.Top;
				result.Right += current->WindowPosition.Left;
				result.Bottom += current->WindowPosition.Top;
				current = current->Parent;
			} while (current);
			return result;
		}
		bool Window::IsGeneralizedParent(Window * window)
		{
			if (this == window) return true;
			Window * current = this;
			while (current->Parent) {
				current = current->Parent;
				if (current == window) return true;
			}
			return false;
		}
		bool Window::IsAvailable(void)
		{
			Window * current = this;
			do {
				if (!current->IsEnabled() || !current->IsVisible()) return false;
				current = current->Parent;
			} while (current->Parent);
			return true;
		}
		Window * Window::GetOverlappedParent(void)
		{
			Window * current = this;
			do {
				if (current->IsOverlapped()) return current;
				current = current->Parent;
			} while (true);
			return 0;
		}
		void Window::MoveAnimated(const Rectangle & to, uint32 duration, Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action)
		{
			Station->AnimateWindow(this, to, duration, begin_class, end_class, action);
		}
		void Window::HideAnimated(Animation::SlideSide side, uint32 duration, Animation::AnimationClass begin, Animation::AnimationClass end)
		{
			auto position = GetRectangle();
			auto width = position.Right - position.Left;
			auto height = position.Bottom - position.Top;
			if (side == Animation::SlideSide::Left) {
				MoveAnimated(Rectangle(-width, position.Top, Coordinate(0), position.Bottom), duration, begin,
					end, Animation::AnimationAction::HideWindowKeepPosition);
			} else if (side == Animation::SlideSide::Top) {
				MoveAnimated(Rectangle(position.Left, -height, position.Right, Coordinate(0)), duration, begin,
					end, Animation::AnimationAction::HideWindowKeepPosition);
			} else if (side == Animation::SlideSide::Right) {
				MoveAnimated(Rectangle(Coordinate::Right(), position.Top, Coordinate::Right() + width, position.Bottom), duration, begin,
					end, Animation::AnimationAction::HideWindowKeepPosition);
			} else if (side == Animation::SlideSide::Bottom) {
				MoveAnimated(Rectangle(position.Left, Coordinate::Bottom(), position.Right, Coordinate::Bottom() + height), duration, begin,
					end, Animation::AnimationAction::HideWindowKeepPosition);
			}
		}
		void Window::ShowAnimated(Animation::SlideSide side, uint32 duration, Animation::AnimationClass end, Animation::AnimationClass begin)
		{
			auto position = GetRectangle();
			auto width = position.Right - position.Left;
			auto height = position.Bottom - position.Top;
			if (side == Animation::SlideSide::Left) {
				Station->AnimateWindow(this, Rectangle(-width, position.Top, Coordinate(0), position.Bottom),
					position, duration, begin, end,
					Animation::AnimationAction::ShowWindow);
			} else if (side == Animation::SlideSide::Top) {
				Station->AnimateWindow(this, Rectangle(position.Left, -height, position.Right, Coordinate(0)),
					position, duration, begin, end,
					Animation::AnimationAction::ShowWindow);
			} else if (side == Animation::SlideSide::Right) {
				Station->AnimateWindow(this, Rectangle(Coordinate::Right(), position.Top, Coordinate::Right() + width, position.Bottom),
					position, duration, begin, end,
					Animation::AnimationAction::ShowWindow);
			} else if (side == Animation::SlideSide::Bottom) {
				Station->AnimateWindow(this, Rectangle(position.Left, Coordinate::Bottom(), position.Right, Coordinate::Bottom() + height),
					position, duration, begin, end,
					Animation::AnimationAction::ShowWindow);
			}
		}
		Window * Window::GetNextTabStopControl(void)
		{
			Window * Current = this;
			int ParentIndex = GetIndexAtParent();
			do {
				if (Current->Children.Length() && Current->IsEnabled() && Current->IsVisible()) {
					Current = Current->Children.FirstElement();
					ParentIndex = 0;
				} else if (!Current->IsOverlapped()) {
					int SuperParentIndex = ParentIndex;
					while (!Current->IsOverlapped() && SuperParentIndex == Current->Parent->Children.Length() - 1) {
						Current = Current->Parent;
						SuperParentIndex = Current->GetIndexAtParent();
					}
					if (!Current->IsOverlapped()) {
						ParentIndex = SuperParentIndex + 1;
						Current = Current->Parent->Children.ElementAt(ParentIndex);
					}
				} else return Current;
				if (Current == this) return this;
			} while (!Current->IsTabStop() || !Current->IsEnabled() || !Current->IsVisible());
			return Current;
		}
		Window * Window::GetPreviousTabStopControl(void)
		{
			Window * Current = this;
			int ParentIndex = GetIndexAtParent();
			do {
				if (Current->IsOverlapped()) {
					while (Current->Children.Length() && Current->IsEnabled() && Current->IsVisible()) Current = Current->Children.LastElement();
					ParentIndex = Current->GetIndexAtParent();
				} else {
					if (ParentIndex == 0) {
						Current = Current->Parent;
						ParentIndex = Current->GetIndexAtParent();
					} else {
						ParentIndex--;
						Current = Current->Parent->Children.ElementAt(ParentIndex);
						while (Current->Children.Length() && Current->IsEnabled() && Current->IsVisible()) Current = Current->Children.LastElement();
						ParentIndex = Current->GetIndexAtParent();
					}
				}
				if (Current == this) return this;
			} while (!Current->IsTabStop() || !Current->IsEnabled() || !Current->IsVisible());
			return Current;
		}
		int Window::GetIndexAtParent(void)
		{
			if (!Parent) return -1;
			for (int i = 0; i < Parent->Children.Length(); i++) if (Parent->Children.ElementAt(i) == this) return i;
			return -1;
		}

		void WindowStation::DeconstructChain(Window * window)
		{
			window->Parent.SetReference(0);
			window->Station.SetReference(0);
			for (int i = 0; i < window->Children.Length(); i++) DeconstructChain(&window->Children[i]);
			if (CaptureWindow.Inner() == window) ReleaseCapture();
			if (FocusedWindow.Inner() == window) SetFocus(TopLevelWindow);
		}
		WindowStation::WindowStation(IDesktopWindowFactory * Factory) : Position(0, 0, 0, 0), Animations(0x10)
		{
			TopLevelWindow.SetReference(Factory->CreateDesktopWindow(this));
			ActiveWindow.SetRetain(TopLevelWindow);
		}
		WindowStation::WindowStation(void) : Position(0, 0, 0, 0), Animations(0x10)
		{
			TopLevelWindow.SetReference(new Engine::UI::TopLevelWindow(0, this));
			ActiveWindow.SetRetain(TopLevelWindow);
		}
		WindowStation::~WindowStation(void) {}
		void WindowStation::DestroyWindow(Window * window)
		{
			SetTimer(window, 0);
			if (!window->Parent) {
				if (window == TopLevelWindow.Inner()) OnDesktopDestroy();
				return;
			}
			SafePointer<Window> Parent = window->Parent;
			Parent->Retain();
			DeconstructChain(window);
			for (int i = 0; i < Parent->Children.Length(); i++) if (Parent->Children.ElementAt(i) == window) {
				Parent->Children.Remove(i);
				break;
			}
		}
		void WindowStation::DestroyStation(void)
		{
			if (TopLevelWindow.Inner()) DeconstructChain(TopLevelWindow);
			this->Release();
		}
		void WindowStation::SetBox(const Box & box) { Position = box; if (TopLevelWindow) { TopLevelWindow->WindowPosition = box; TopLevelWindow->ArrangeChildren(); } }
		Box WindowStation::GetBox(void) { return Position; }
		void WindowStation::Render(void)
		{
			uint32 time = GetTimerValue();
			for (int i = 0; i < Animations.Length(); i++) {
				if (Animations[i].IsOver(time)) {
					Animations[i].Target->SetRectangle(Animations[i].GetFrame(Animations[i].EndTime));
					if (Animations[i].Action == Animation::AnimationAction::HideWindow || Animations[i].Action == Animation::AnimationAction::HideWindowKeepPosition) {
						Animations[i].Target->Show(false);
						if (Animations[i].Action == Animation::AnimationAction::HideWindowKeepPosition) Animations[i].Target->SetRectangle(Animations[i].BeginState);
					}
					Animations.Remove(i); i--;
					if (Animations.Length() == 0) AnimationStateChanged();
				} else {
					Animations[i].Target->SetRectangle(Animations[i].GetFrame(time));
				}
			}
			if (TopLevelWindow.Inner()) TopLevelWindow->Render(Position);
		}
		void WindowStation::ResetCache(void) { if (TopLevelWindow.Inner()) TopLevelWindow->ResetCache(); }
		Window * WindowStation::GetDesktop(void) { return TopLevelWindow; }
		Window * WindowStation::HitTest(Point at) { if (TopLevelWindow.Inner() && NativeHitTest(at)) return TopLevelWindow->HitTest(at); else return 0; }
		Window * WindowStation::EnabledHitTest(Point at)
		{
			Window * target = TopLevelWindow->HitTest(at);
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
			if (ExclusiveWindow) if (!Target->IsGeneralizedParent(ExclusiveWindow)) Target = 0;
			*target = Target; *local = CalculateLocalPoint(Target, global);
		}
		void WindowStation::SetActiveWindow(Window * window) { if (ActiveWindow.Inner() != window) ActiveWindow.SetRetain(window); if (ActiveWindow) ActiveWindow->SetOrder(Window::DepthOrder::SetFirst); }
		Window * WindowStation::GetActiveWindow(void) { return ActiveWindow; }
		void WindowStation::AnimateWindow(Window * window, const Rectangle & position, uint32 duration, Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action)
		{
			for (int i = 0; i < Animations.Length(); i++) if (Animations[i].Target == window) return;
			if (action == Animation::AnimationAction::ShowWindow) window->Show(true);
			Animations << WindowAnimationState(window, window->GetRectangle(), position, GetTimerValue(), duration,
				begin_class, end_class, action);
			if (Animations.Length() == 1) AnimationStateChanged();
		}
		void WindowStation::AnimateWindow(Window * window, const Rectangle & from, const Rectangle & position, uint32 duration, Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action)
		{
			for (int i = 0; i < Animations.Length(); i++) if (Animations[i].Target == window) return;
			window->SetRectangle(from);
			if (action == Animation::AnimationAction::ShowWindow) window->Show(true);
			Animations << WindowAnimationState(window, window->GetRectangle(), position, GetTimerValue(), duration,
				begin_class, end_class, action);
			if (Animations.Length() == 1) AnimationStateChanged();
		}
		bool WindowStation::IsPlayingAnimation(void) const { return Animations.Length() != 0; }
		void WindowStation::SetFocus(Window * window)
		{
			if (window == FocusedWindow.Inner()) return;
			if (FocusedWindow.Inner()) FocusedWindow->FocusChanged(false);
			FocusedWindow.SetRetain(window);
			if (FocusedWindow.Inner()) FocusedWindow->FocusChanged(true);
			FocusWindowChanged();
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
		void WindowStation::SetExclusiveWindow(Window * window) { if (ExclusiveWindow) ExclusiveWindow->LostExclusiveMode(); ExclusiveWindow.SetRetain(window); MouseMove(GetCursorPos()); }
		Window * WindowStation::GetExclusiveWindow(void) { return ExclusiveWindow; }
		void WindowStation::FocusChanged(bool got_focus)
		{
			if (!got_focus) {
				if (FocusedWindow.Inner()) FocusedWindow->FocusChanged(false);
				FocusedWindow.SetReference(0);
				FocusWindowChanged();
			}
		}
		void WindowStation::CaptureChanged(bool got_capture)
		{
			if (!got_capture) {
				if (CaptureWindow.Inner()) {
					CaptureWindow->CaptureChanged(false);
					CaptureWindow.SetReference(0);
				}
				if (ExclusiveWindow.Inner()) {
					ExclusiveWindow->LostExclusiveMode();
					ExclusiveWindow.SetReference(0);
				}
			}
		}
		bool WindowStation::NativeHitTest(const Point & at) { return true; }
		void WindowStation::LeftButtonDown(Point at)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			Window * Target; Point At;
			if (CaptureWindow) {
				Target = CaptureWindow;
				At = CalculateLocalPoint(CaptureWindow, at);
			} else {
				GetMouseTarget(at, &Target, &At);
			}
			if (Target) {
				if (!ExclusiveWindow) {
					Window * Parent = Target->GetOverlappedParent();
					if (Parent != TopLevelWindow.Inner() && ActiveWindow.Inner() != Parent) {
						SetActiveWindow(Parent);
						SetFocus(0);
					}
				}
				Target->LeftButtonDown(At);
			} else if (ExclusiveWindow) SetExclusiveWindow(0);
		}
		void WindowStation::LeftButtonUp(Point at)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			if (CaptureWindow) CaptureWindow->LeftButtonUp(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->LeftButtonUp(At);
			}
		}
		void WindowStation::LeftButtonDoubleClick(Point at)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			if (CaptureWindow) CaptureWindow->LeftButtonDoubleClick(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->LeftButtonDoubleClick(At);
			}
		}
		void WindowStation::RightButtonDown(Point at)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			Window * Target; Point At;
			if (CaptureWindow) {
				Target = CaptureWindow;
				At = CalculateLocalPoint(CaptureWindow, at);
			} else {
				GetMouseTarget(at, &Target, &At);
			}
			if (Target) {
				if (!ExclusiveWindow) {
					Window * Parent = Target->GetOverlappedParent();
					if (Parent != TopLevelWindow.Inner() && ActiveWindow.Inner() != Parent) {
						SetActiveWindow(Parent);
						SetFocus(0);
					}
				}
				Target->RightButtonDown(At);
			} else if (ExclusiveWindow) SetExclusiveWindow(0);
		}
		void WindowStation::RightButtonUp(Point at)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			if (CaptureWindow) CaptureWindow->RightButtonUp(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->RightButtonUp(At);
			}
		}
		void WindowStation::RightButtonDoubleClick(Point at)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			if (CaptureWindow) CaptureWindow->RightButtonDoubleClick(CalculateLocalPoint(CaptureWindow, at));
			else {
				Window * Target; Point At;
				GetMouseTarget(at, &Target, &At);
				if (Target) Target->RightButtonDoubleClick(At);
			}
		}
		void WindowStation::MouseMove(Point at)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			Window * Target; Point At;
			if (CaptureWindow) {
				Target = CaptureWindow;
				At = CalculateLocalPoint(CaptureWindow, at);
			} else {
				GetMouseTarget(at, &Target, &At);
			}
			if (Target) {
				Target->SetCursor(At);
				Target->MouseMove(At);
			}
		}
		void WindowStation::ScrollVertically(double delta)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			if (CaptureWindow) CaptureWindow->ScrollVertically(delta);
			else {
				Window * Target = EnabledHitTest(GetCursorPos());
				if (Target) Target->ScrollVertically(delta);
			}
		}
		void WindowStation::ScrollHorizontally(double delta)
		{
			if (CaptureWindow && !CaptureWindow->IsAvailable()) SetCapture(0);
			if (CaptureWindow) CaptureWindow->ScrollHorizontally(delta);
			else {
				Window * Target = EnabledHitTest(GetCursorPos());
				if (Target) Target->ScrollHorizontally(delta);
			}
		}
		void WindowStation::KeyDown(int key_code)
		{
			if (ExclusiveWindow || !ActiveWindow->TranslateAccelerators(key_code)) {
				if (FocusedWindow && !FocusedWindow->IsAvailable()) SetFocus(0);
				if (FocusedWindow) FocusedWindow->KeyDown(key_code);
			}
		}
		void WindowStation::KeyUp(int key_code) { if (FocusedWindow && !FocusedWindow->IsAvailable()) SetFocus(0); if (FocusedWindow) FocusedWindow->KeyUp(key_code); }
		void WindowStation::CharDown(uint32 ucs_code) { if (FocusedWindow && !FocusedWindow->IsAvailable()) SetFocus(0); if (FocusedWindow) FocusedWindow->CharDown(ucs_code); }
		Point WindowStation::GetCursorPos(void) { return Point(0, 0); }
		ICursor * WindowStation::LoadCursor(Streaming::Stream * Source) { return 0; }
		ICursor * WindowStation::LoadCursor(Codec::Image * Source) { return 0; }
		ICursor * WindowStation::LoadCursor(Codec::Frame * Source) { return 0; }
		ICursor * WindowStation::GetSystemCursor(SystemCursor cursor) { return 0; }
		void WindowStation::SetSystemCursor(SystemCursor entity, ICursor * cursor) {}
		void WindowStation::SetCursor(ICursor * cursor) {}
		void WindowStation::SetTimer(Window * window, uint32 period) {}
		bool WindowStation::IsNativeStationWrapper(void) const { return false; }
		void WindowStation::OnDesktopDestroy(void) {}
		void WindowStation::RequireRefreshRate(Window::RefreshPeriod period) {}
		Window::RefreshPeriod WindowStation::GetRefreshRate(void) { return Window::RefreshPeriod::None; }
		void WindowStation::AnimationStateChanged(void) {}
		void WindowStation::FocusWindowChanged(void) {}
		WindowStation::VisualStyles & WindowStation::GetVisualStyles(void) { return Styles; }

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
			Box inner = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
			for (int i = 0; i < Children.Length(); i++) {	
				auto rect = Children[i].GetRectangle();
				if (rect.IsValid()) {
					Children[i].SetPosition(Box(rect, inner));
				}
			}
		}
		void ParentWindow::SetPosition(const Box & box) { Window::SetPosition(box); ArrangeChildren(); }
		Window * ParentWindow::HitTest(Point at)
		{
			if (!IsEnabled()) return this;
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
				auto args = ZeroArgumentProvider();
				BackgroundShape.SetReference(Background->Initialize(&args));
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

		ZeroArgumentProvider::ZeroArgumentProvider(void) {}
		void ZeroArgumentProvider::GetArgument(const string & name, int * value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, double * value) { *value = 0.0; }
		void ZeroArgumentProvider::GetArgument(const string & name, Color * value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, string * value) { *value = L""; }
		void ZeroArgumentProvider::GetArgument(const string & name, ITexture ** value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, IFont ** value) { *value = 0; }
		ReflectorArgumentProvider::ReflectorArgumentProvider(Reflection::Reflected * source) : Source(source) {}
		void ReflectorArgumentProvider::GetArgument(const string & name, int * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Integer) *value = prop.Get<int>();
			else if (prop.Type == Reflection::PropertyType::Double) *value = int(prop.Get<double>());
			*value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, double * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Integer) *value = double(prop.Get<int>());
			else if (prop.Type == Reflection::PropertyType::Double) *value = prop.Get<double>();
			else *value = 0.0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, Color * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Color) *value = prop.Get<Color>();
			else *value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, string * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::String) *value = prop.Get<string>();
			else *value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, ITexture ** value)
		{
			ITexture * object = 0;
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Texture) object = prop.Get<SafePointer<ITexture>>();
			if (object) {
				*value = object;
				object->Retain();
			} else *value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, IFont ** value)
		{
			IFont * object = 0;
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Font) object = prop.Get<SafePointer<IFont>>();
			if (object) {
				*value = object;
				object->Retain();
			} else *value = 0;
		}
	}
}