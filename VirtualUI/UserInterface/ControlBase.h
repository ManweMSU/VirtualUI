#pragma once

#include "../EngineBase.h"
#include "ShapeBase.h"
#include "Templates.h"

namespace Engine
{
	namespace UI
	{
		class WindowStation;
		class Window;

		class Window : public Object
		{
			friend class WindowStation;
			friend class ParentWindow;
		public:
			enum class DepthOrder { SetFirst = 0, SetLast = 1, MoveUp = 2, MoveDown = 3 };
			enum class Event { Command = 0, DoubleClick = 1, ContextClick = 2, ValueChange = 3 };
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
			virtual void SetID(int ID);
			virtual int GetID(void);
			virtual Window * FindChild(int ID);
			virtual void SetRectangle(const Rectangle & rect);
			virtual Rectangle GetRectangle(void);
			virtual void SetPosition(const Box & box);
			virtual Box GetPosition(void);
			virtual void SetText(const string & text);
			virtual string GetText(void);		
			virtual void RaiseEvent(int ID, Event event);
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
			virtual Window * HitTest(Point at);

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
		};
		class WindowStation : public Object
		{
		private:
			SafePointer<IRenderingDevice> RenderingDevice;
			SafePointer<Window> TopLevelWindow;
			SafePointer<Window> CaptureWindow;
			SafePointer<Window> FocusedWindow;
			Box Position;
			void DeconstructChain(Window * window);
		public:
			WindowStation(void);
			~WindowStation(void) override;
			template <class W> W * CreateWindow(Window * Parent, Template::ControlTemplate * Template)
			{
				SafePointer<W> New = new W(Parent, this, Template);
				Parent->Children.Insert(New, 0);
				return New;
			}
			template <class W> W * CreateWindow(Window * Parent)
			{
				SafePointer<W> New = new W(Parent, this);
				Parent->Children.Insert(New, 0);
				return New;
			}
			template <class W> W * CreateTopLevel(void)
			{
				SafePointer<W> New = new W(0, this);
				if (TopLevelWindow.Inner()) DeconstructChain(TopLevelWindow);
				TopLevelWindow.SetRetain(New);
				return New;
			}
			void DestroyWindow(Window * window);
			void SetBox(const Box & box);
			Box GetBox(void);
			void Render(void);
			void ResetCache(void);
			Window * HitTest(Point at);
			Window * EnabledHitTest(Point at);
			void SetRenderingDevice(IRenderingDevice * Device);
			IRenderingDevice * GetRenderingDevice(void);
			Point CalculateLocalPoint(Window * window, Point global);
			Point CalculateGlobalPoint(Window * window, Point local);
			void GetMouseTarget(Point global, Window ** target, Point * local);

			virtual void SetFocus(Window * window);
			virtual Window * GetFocus(void);
			virtual void SetCapture(Window * window);
			virtual Window * GetCapture(void);
			virtual void ReleaseCapture(void);
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
		};
		class ParentWindow : public Window
		{
		public:
			ParentWindow(Window * parent, WindowStation * station);
			virtual void Render(const Box & at) override;
			virtual void ArrangeChildren(void) override;
			virtual Window * HitTest(Point at) override;
		};
		class TopLevelWindow : public ParentWindow
		{
		private:
			SafePointer<Shape> BackgroundShape;
		public:
			SafePointer<Template::Shape> Background;
			TopLevelWindow(Window * parent, WindowStation * station);
			virtual ~TopLevelWindow(void) override;
			virtual void Render(const Box & at) override;
			virtual void ResetCache(void) override;
			virtual Rectangle GetRectangle(void) override;
			virtual Box GetPosition(void) override;

#pragma message("REMOVE!")
			virtual void RaiseEvent(int ID, Event event) override;
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