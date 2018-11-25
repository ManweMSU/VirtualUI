#pragma once

#include "../EngineBase.h"
#include "../ImageCodec/CodecBase.h"
#include "../Miscellaneous/ThreadPool.h"
#include "ShapeBase.h"
#include "Templates.h"
#include "Animation.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//                   Retain/Release Features
// Window Stations and Windows have different Retain/Release
// policy required. Window Station warranties a life of any
// internal window. So, retain windows only if there is a need
// to use it after station was destroyed. Window Station is
// refrenced by any window it contains, Window is referenced
// by it's children. So Release() will not destroy them.
// Instead of it use:
// on windows: window's Destroy() or station's DestroyWindow()
// methods: they detach window from it's window station and
// destroy parent-child linking, then station automaticly
// releases your window.
// on window stations: DestroyStation() method: it destroys
// all links between owned windows and the station, then calls
// Release().
// Both window and window station are invalid after these calls.
// To create a window, use station's CreateWindow() methods.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Engine
{
	namespace UI
	{
		class WindowStation;
		class Window;
		class TopLevelWindow;

		class Window : public Object
		{
			friend class WindowStation;
			friend class ParentWindow;
		public:
			enum class DepthOrder { SetFirst = 0, SetLast = 1, MoveUp = 2, MoveDown = 3 };
			enum class Event { Command = 0, AcceleratorCommand = 1, MenuCommand = 2, DoubleClick = 3, ContextClick = 4, ValueChange = 5, Deferred = 6 };
			enum class RefreshPeriod { None = 0, CaretBlink = 1, Cinematic = 2 };
		private:
			ObjectArray<Window> Children;
			SafePointer<Window> Parent;
			SafePointer<WindowStation> Station;
		protected:
			Box WindowPosition;
		public:
			Window(Window * parent, WindowStation * station);
			virtual ~Window(void) override;
			virtual void Render(const Box & at);
			virtual void ResetCache(void);
			virtual void ArrangeChildren(void);
			virtual void Enable(bool enable);
			virtual bool IsEnabled(void);
			virtual void Show(bool visible);
			virtual bool IsVisible(void);
			virtual bool IsTabStop(void);
			virtual bool IsOverlapped(void);
			virtual bool IsNeverActive(void);
			virtual void SetID(int ID);
			virtual int GetID(void);
			virtual Window * FindChild(int ID);
			virtual void SetRectangle(const Rectangle & rect);
			virtual Rectangle GetRectangle(void);
			virtual void SetPosition(const Box & box);
			virtual Box GetPosition(void);
			virtual void SetText(const string & text);
			virtual string GetText(void);		
			virtual void RaiseEvent(int ID, Event event, Window * sender);
			virtual void FocusChanged(bool got_focus);
			virtual void CaptureChanged(bool got_capture);
			virtual void LostExclusiveMode(void);
			virtual void LeftButtonDown(Point at);
			virtual void LeftButtonUp(Point at);
			virtual void LeftButtonDoubleClick(Point at);
			virtual void RightButtonDown(Point at);
			virtual void RightButtonUp(Point at);
			virtual void RightButtonDoubleClick(Point at);
			virtual void MouseMove(Point at);
			virtual void ScrollVertically(double delta);
			virtual void ScrollHorizontally(double delta);
			virtual void KeyDown(int key_code);
			virtual void KeyUp(int key_code);
			virtual bool TranslateAccelerators(int key_code);
			virtual void CharDown(uint32 ucs_code);
			virtual void PopupMenuCancelled(void);
			virtual void Timer(void);
			virtual Window * HitTest(Point at);
			virtual void SetCursor(Point at);
			virtual RefreshPeriod FocusedRefreshPeriod(void);

			Window * GetParent(void);
			WindowStation * GetStation(void);
			void SetOrder(DepthOrder order);
			int ChildrenCount(void);
			Window * Child(int index);
			void SetFocus(void);
			Window * GetFocus(void);
			void SetCapture(void);
			Window * GetCapture(void);
			void ReleaseCapture(void);
			void Destroy(void);
			Box GetAbsolutePosition(void);
			bool IsGeneralizedParent(Window * window);
			bool IsAvailable(void);
			Window * GetOverlappedParent(void);
			void MoveAnimated(const Rectangle & to, uint32 duration, Animation::AnimationClass begin_class,
				Animation::AnimationClass end_class, Animation::AnimationAction action);
			void HideAnimated(Animation::SlideSide side, uint32 duration, Animation::AnimationClass begin, Animation::AnimationClass end = Animation::AnimationClass::Hard);
			void ShowAnimated(Animation::SlideSide side, uint32 duration, Animation::AnimationClass end, Animation::AnimationClass begin = Animation::AnimationClass::Hard);
			Window * GetNextTabStopControl(void);
			Window * GetPreviousTabStopControl(void);
			int GetIndexAtParent(void);
			void RequireRedraw(void);
			void DeferredDestroy(void);
			void DeferredRaiseEvent(int ID);
			void PostEvent(int ID);
			template <class W> W * As(void) { return static_cast<W *>(this); }
		};
		class ICursor : public Object {};
		typedef Animation::AnimationState<Window, Rectangle> WindowAnimationState;
		enum class SystemCursor { Null = 0, Arrow = 1, Beam = 2, Link = 3, SizeLeftRight = 4, SizeUpDown = 5, SizeLeftUpRightDown = 6, SizeLeftDownRightUp = 7, SizeAll = 8 };
		class WindowStation : public Object
		{
		public:
			class IDesktopWindowFactory
			{
			public:
				virtual Window * CreateDesktopWindow(WindowStation * Station) = 0;
			};
			struct VisualStyles
			{
				SafePointer<Template::Shape> WindowActiveView; // Argumented with Text, Border, NegBorder, ButtonsWidth, NegButtonsWidth, Caption and NegCaption
				SafePointer<Template::Shape> WindowInactiveView;
				SafePointer<Template::Shape> WindowSmallActiveView;
				SafePointer<Template::Shape> WindowSmallInactiveView;
				SafePointer<Template::Shape> WindowDefaultBackground;
				int WindowFixedBorder = 0;
				int WindowSizableBorder = 0;
				int WindowCaptionHeight = 0;
				int WindowSmallCaptionHeight = 0;
				SafePointer<Template::ControlTemplate> WindowCloseButton; // ToolButtonPart template
				SafePointer<Template::ControlTemplate> WindowMaximizeButton;
				SafePointer<Template::ControlTemplate> WindowMinimizeButton;
				SafePointer<Template::ControlTemplate> WindowHelpButton;
				SafePointer<Template::ControlTemplate> WindowSmallCloseButton;
				SafePointer<Template::ControlTemplate> WindowSmallMaximizeButton;
				SafePointer<Template::ControlTemplate> WindowSmallMinimizeButton;
				SafePointer<Template::ControlTemplate> WindowSmallHelpButton;

				SafePointer<Template::Shape> MenuBackground;
				SafePointer<Template::Shape> MenuArrow;
				int MenuBorder = 0;

				int CaretWidth = 1;
			};
		protected:
			WindowStation(IDesktopWindowFactory * Factory);
		private:
			SafePointer<IRenderingDevice> RenderingDevice;
			SafePointer<Window> TopLevelWindow;
			SafePointer<Window> CaptureWindow;
			SafePointer<Window> FocusedWindow;
			SafePointer<Window> ActiveWindow;
			SafePointer<Window> ExclusiveWindow;
			Box Position;
			VisualStyles Styles;
			Array<WindowAnimationState> Animations;
			bool Works = true;
			void DeconstructChain(Window * window);
		public:
			WindowStation(void);
			~WindowStation(void) override;
			template <class W> W * CreateWindow(Window * Parent, Template::ControlTemplate * Template)
			{
				if (!Parent) Parent = TopLevelWindow;
				SafePointer<W> New = new W(Parent, this, Template);
				Parent->Children.Append(New);
				return New;
			}
			template <class W> W * CreateWindow(Window * Parent)
			{
				if (!Parent) Parent = TopLevelWindow;
				SafePointer<W> New = new W(Parent, this);
				Parent->Children.Append(New);
				return New;
			}
			template <class W, class A> W * CreateWindow(Window * Parent, A a)
			{
				if (!Parent) Parent = TopLevelWindow;
				SafePointer<W> New = new W(Parent, this, a);
				Parent->Children.Append(New);
				return New;
			}
			template <class W, class A1, class A2> W * CreateWindow(Window * Parent, A1 a1, A2 a2)
			{
				if (!Parent) Parent = TopLevelWindow;
				SafePointer<W> New = new W(Parent, this, a1, a2);
				Parent->Children.Append(New);
				return New;
			}
			template <class W, class A1, class A2, class A3> W * CreateWindow(Window * Parent, A1 a1, A2 a2, A3 a3)
			{
				if (!Parent) Parent = TopLevelWindow;
				SafePointer<W> New = new W(Parent, this, a1, a2, a3);
				Parent->Children.Append(New);
				return New;
			}
			void DestroyWindow(Window * window);
			void DestroyStation(void);
			void SetBox(const Box & box);
			Box GetBox(void);
			void Render(void);
			void ResetCache(void);
			Window * GetDesktop(void);
			Window * HitTest(Point at);
			Window * EnabledHitTest(Point at);
			void SetRenderingDevice(IRenderingDevice * Device);
			IRenderingDevice * GetRenderingDevice(void);
			Point CalculateLocalPoint(Window * window, Point global);
			Point CalculateGlobalPoint(Window * window, Point local);
			void GetMouseTarget(Point global, Window ** target, Point * local);
			void SetActiveWindow(Window * window);
			Window * GetActiveWindow(void);
			void AnimateWindow(Window * window, const Rectangle & position, uint32 duration,
				Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action);
			void AnimateWindow(Window * window, const Rectangle & from, const Rectangle & position, uint32 duration,
				Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action);
			void AnimateWindow(Window * window, const Box & from, const Box & position, uint32 duration,
				Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action);
			bool IsPlayingAnimation(void) const;
			void Animate(void);

			virtual void SetFocus(Window * window);
			virtual Window * GetFocus(void);
			virtual void SetCapture(Window * window);
			virtual Window * GetCapture(void);
			virtual void ReleaseCapture(void);
			virtual void SetExclusiveWindow(Window * window);
			virtual Window * GetExclusiveWindow(void);
			virtual void FocusChanged(bool got_focus);
			virtual void CaptureChanged(bool got_capture);
			virtual bool NativeHitTest(const Point & at);
			virtual void LeftButtonDown(Point at);
			virtual void LeftButtonUp(Point at);
			virtual void LeftButtonDoubleClick(Point at);
			virtual void RightButtonDown(Point at);
			virtual void RightButtonUp(Point at);
			virtual void RightButtonDoubleClick(Point at);
			virtual void MouseMove(Point at);
			virtual void ScrollVertically(double delta);
			virtual void ScrollHorizontally(double delta);
			virtual void KeyDown(int key_code);
			virtual void KeyUp(int key_code);
			virtual void CharDown(uint32 ucs_code);
			virtual Point GetCursorPos(void);
			virtual ICursor * LoadCursor(Streaming::Stream * Source);
			virtual ICursor * LoadCursor(Codec::Image * Source);
			virtual ICursor * LoadCursor(Codec::Frame * Source);
			virtual ICursor * GetSystemCursor(SystemCursor cursor);
			virtual void SetSystemCursor(SystemCursor entity, ICursor * cursor);
			virtual void SetCursor(ICursor * cursor);
			virtual void SetTimer(Window * window, uint32 period);
			virtual bool IsNativeStationWrapper(void) const;
			virtual void OnDesktopDestroy(void);
			virtual void RequireRefreshRate(Window::RefreshPeriod period);
			virtual Window::RefreshPeriod GetRefreshRate(void);
			virtual void AnimationStateChanged(void);
			virtual void FocusWindowChanged(void);
			virtual Box GetDesktopBox(void);
			virtual Box GetAbsoluteDesktopBox(const Box & box);
			virtual void RequireRedraw(void);
			virtual void DeferredDestroy(Window * window);
			virtual void DeferredRaiseEvent(Window * window, int ID);
			virtual void PostJob(Tasks::ThreadJob * job);

			VisualStyles & GetVisualStyles(void);
		};
		class ParentWindow : public Window
		{
		public:
			ParentWindow(Window * parent, WindowStation * station);
			virtual Window * FindChild(int ID) override;
			virtual void Render(const Box & at) override;
			virtual void ArrangeChildren(void) override;
			virtual void SetPosition(const Box & box) override;
			virtual Window * HitTest(Point at) override;
		};
		class TopLevelWindow : public ParentWindow
		{
			friend class WindowStation;
		private:
			SafePointer<Shape> BackgroundShape;
			TopLevelWindow(Window * parent, WindowStation * station);
		public:
			SafePointer<Template::Shape> Background;
			virtual ~TopLevelWindow(void) override;
			virtual void Render(const Box & at) override;
			virtual void ResetCache(void) override;
			virtual Rectangle GetRectangle(void) override;
			virtual Box GetPosition(void) override;
			virtual bool IsOverlapped(void) override;
			virtual bool IsNeverActive(void) override;
		};
		class ZeroArgumentProvider : public IArgumentProvider
		{
		public:
			ZeroArgumentProvider(void);
			virtual void GetArgument(const string & name, int * value) override;
			virtual void GetArgument(const string & name, double * value) override;
			virtual void GetArgument(const string & name, Color * value) override;
			virtual void GetArgument(const string & name, string * value) override;
			virtual void GetArgument(const string & name, ITexture ** value) override;
			virtual void GetArgument(const string & name, IFont ** value) override;
		};
		class ReflectorArgumentProvider : public IArgumentProvider
		{
			Reflection::Reflected * Source;
		public:
			ReflectorArgumentProvider(Reflection::Reflected * source);

			virtual void GetArgument(const string & name, int * value) override;
			virtual void GetArgument(const string & name, double * value) override;
			virtual void GetArgument(const string & name, Color * value) override;
			virtual void GetArgument(const string & name, string * value) override;
			virtual void GetArgument(const string & name, ITexture ** value) override;
			virtual void GetArgument(const string & name, IFont ** value) override;
		};
	}
}