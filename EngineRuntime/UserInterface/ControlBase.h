#pragma once

#include "../EngineBase.h"
#include "../ImageCodec/CodecBase.h"
#include "ShapeBase.h"
#include "Templates.h"

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
			enum class Event { Command = 0, MenuCommand = 1, DoubleClick = 2, ContextClick = 3, ValueChange = 4 };
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
			virtual void ScrollVertically(int delta);
			virtual void ScrollHorizontally(int delta);
			virtual void KeyDown(int key_code);
			virtual void KeyUp(int key_code);
			virtual void CharDown(uint32 ucs_code);
			virtual void PopupMenuCancelled(void);
			virtual Window * HitTest(Point at);
			virtual void SetCursor(Point at);

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
			template <class W> W * As(void) { return static_cast<W *>(this); }
		};
		class ICursor : public Object {};
		enum class SystemCursor { Null = 0, Arrow = 1, Beam = 2, Link = 3, SizeLeftRight = 4, SizeUpDown = 5, SizeLeftUpRightDown = 6, SizeLeftDownRightUp = 7, SizeAll = 8 };
		class WindowStation : public Object
		{
		public:
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
		private:
			SafePointer<IRenderingDevice> RenderingDevice;
			SafePointer<Engine::UI::TopLevelWindow> TopLevelWindow;
			SafePointer<Window> CaptureWindow;
			SafePointer<Window> FocusedWindow;
			SafePointer<Window> ActiveWindow;
			SafePointer<Window> ExclusiveWindow;
			Box Position;
			VisualStyles Styles;
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
			void SetBox(const Box & box);
			Box GetBox(void);
			void Render(void);
			void ResetCache(void);
			Engine::UI::TopLevelWindow * GetDesktop(void);
			Window * HitTest(Point at);
			Window * EnabledHitTest(Point at);
			void SetRenderingDevice(IRenderingDevice * Device);
			IRenderingDevice * GetRenderingDevice(void);
			Point CalculateLocalPoint(Window * window, Point global);
			Point CalculateGlobalPoint(Window * window, Point local);
			void GetMouseTarget(Point global, Window ** target, Point * local);
			void SetActiveWindow(Window * window);
			Window * GetActiveWindow(void);

			virtual void SetFocus(Window * window);
			virtual Window * GetFocus(void);
			virtual void SetCapture(Window * window);
			virtual Window * GetCapture(void);
			virtual void ReleaseCapture(void);
			virtual void SetExclusiveWindow(Window * window);
			virtual Window * GetExclusiveWindow(void);
			virtual void FocusChanged(bool got_focus);
			virtual void CaptureChanged(bool got_capture);
			virtual void LeftButtonDown(Point at);
			virtual void LeftButtonUp(Point at);
			virtual void LeftButtonDoubleClick(Point at);
			virtual void RightButtonDown(Point at);
			virtual void RightButtonUp(Point at);
			virtual void RightButtonDoubleClick(Point at);
			virtual void MouseMove(Point at);
			virtual void ScrollVertically(int delta);
			virtual void ScrollHorizontally(int delta);
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
	}
}