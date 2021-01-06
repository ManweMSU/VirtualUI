#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class VirtualStation : public Window
			{
				class VirtualWindowStation : public WindowStation
				{
					VirtualStation * host;
					WindowStation * host_station;
				public:
					VirtualWindowStation(VirtualStation * host_window);
					~VirtualWindowStation(void) override;

					virtual void SetFocus(Window * window) override;
					virtual Window * GetFocus(void) override;
					virtual void SetCapture(Window * window) override;
					virtual Window * GetCapture(void) override;
					virtual void ReleaseCapture(void) override;
					virtual void SetExclusiveWindow(Window * window) override;
					virtual Window * GetExclusiveWindow(void) override;
					virtual bool NativeHitTest(const Point & at) override;
					virtual Point GetCursorPos(void) override;
					virtual void SetCursorPos(Point pos) override;
					virtual ICursor * LoadCursor(Streaming::Stream * Source) override;
					virtual ICursor * LoadCursor(Codec::Image * Source) override;
					virtual ICursor * LoadCursor(Codec::Frame * Source) override;
					virtual ICursor * GetSystemCursor(SystemCursor cursor) override;
					virtual void SetSystemCursor(SystemCursor entity, ICursor * cursor) override;
					virtual void SetCursor(ICursor * cursor) override;
					virtual void SetTimer(Window * window, uint32 period) override;
					virtual void RequireRefreshRate(Window::RefreshPeriod period) override;
					virtual Window::RefreshPeriod GetRefreshRate(void) override;
					virtual void AnimationStateChanged(void) override;
					virtual void FocusWindowChanged(void) override;
					virtual void RequireRedraw(void) override;
					virtual void DeferredDestroy(Window * window) override;
					virtual void DeferredRaiseEvent(Window * window, int ID) override;
					virtual void PostJob(Tasks::ThreadJob * job) override;
				};
				
				VirtualWindowStation * _station;
				bool _enabled, _visible;
				bool _autosize, _render;
				int _id;
				Rectangle _rect;
				int _width, _height;
			public:
				VirtualStation(Window * Parent, WindowStation * Station);
				VirtualStation(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~VirtualStation(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void RightButtonDown(Point at) override;
				virtual void RightButtonUp(Point at) override;
				virtual void RightButtonDoubleClick(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void ScrollVertically(double delta) override;
				virtual void ScrollHorizontally(double delta) override;
				virtual bool KeyDown(int key_code) override;
				virtual void KeyUp(int key_code) override;
				virtual void CharDown(uint32 ucs_code) override;
				virtual void SetCursor(Point at) override;
				virtual RefreshPeriod FocusedRefreshPeriod(void) override;
				virtual string GetControlClass(void) override;

				WindowStation * GetInnerStation(void);
				Point GetDesktopDimensions(void);
				void SetDesktopDimensions(Point size);
				bool IsAutosize(void);
				void UseAutosize(bool use);
				bool IsStandardRendering(void);
				void UseStandardRendering(bool use);

				Point InnerToOuter(Point p);
				Point OuterToInner(Point p);
			};
		}
	}
}