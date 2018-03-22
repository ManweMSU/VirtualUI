#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class VerticalScrollBar : public Window, public Template::Controls::VerticalScrollBar
			{
				SafePointer<Shape> _up_normal;
				SafePointer<Shape> _up_hot;
				SafePointer<Shape> _up_pressed;
				SafePointer<Shape> _up_disabled;
				SafePointer<Shape> _down_normal;
				SafePointer<Shape> _down_hot;
				SafePointer<Shape> _down_pressed;
				SafePointer<Shape> _down_disabled;
				SafePointer<Shape> _scroller_normal;
				SafePointer<Shape> _scroller_hot;
				SafePointer<Shape> _scroller_pressed;
				SafePointer<Shape> _scroller_disabled;
				SafePointer<Shape> _bar_normal;
				SafePointer<Shape> _bar_disabled;
				int _state = 0, _part = 0, _mpos = 0, _ipos = 0, _sd = 0;
				uint32 _lasttime, _period;
				Box GetScrollerBox(const Box & at);
			public:
				VerticalScrollBar(Window * Parent, WindowStation * Station);
				VerticalScrollBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~VerticalScrollBar(void) override;

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
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;

				void SetScrollerPosition(int position);
				void SetPage(int page);
				void SetRange(int range_min, int range_max);
			};
			class HorizontalScrollBar : public Window, public Template::Controls::HorizontalScrollBar
			{
				SafePointer<Shape> _left_normal;
				SafePointer<Shape> _left_hot;
				SafePointer<Shape> _left_pressed;
				SafePointer<Shape> _left_disabled;
				SafePointer<Shape> _right_normal;
				SafePointer<Shape> _right_hot;
				SafePointer<Shape> _right_pressed;
				SafePointer<Shape> _right_disabled;
				SafePointer<Shape> _scroller_normal;
				SafePointer<Shape> _scroller_hot;
				SafePointer<Shape> _scroller_pressed;
				SafePointer<Shape> _scroller_disabled;
				SafePointer<Shape> _bar_normal;
				SafePointer<Shape> _bar_disabled;
				int _state = 0, _part = 0, _mpos = 0, _ipos = 0, _sd = 0;
				uint32 _lasttime, _period;
				Box GetScrollerBox(const Box & at);
			public:
				HorizontalScrollBar(Window * Parent, WindowStation * Station);
				HorizontalScrollBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~HorizontalScrollBar(void) override;

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
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;

				void SetScrollerPosition(int position);
				void SetPage(int page);
				void SetRange(int range_min, int range_max);
			};
			class VerticalTrackBar : public Window, public Template::Controls::VerticalTrackBar
			{
				SafePointer<Shape> _tracker_normal;
				SafePointer<Shape> _tracker_focused;
				SafePointer<Shape> _tracker_hot;
				SafePointer<Shape> _tracker_pressed;
				SafePointer<Shape> _tracker_disabled;
				SafePointer<Shape> _bar_normal;
				SafePointer<Shape> _bar_disabled;
				int _state = 0, _mouse = 0;
				Box GetTrackerPosition(const Box & at);
				int GetTrackerShift(const Box & at);
				int MouseToTracker(const Box & at, int mouse);
			public:
				VerticalTrackBar(Window * Parent, WindowStation * Station);
				VerticalTrackBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~VerticalTrackBar(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void KeyDown(int key_code) override;

				void SetTrackerPosition(int position);
				void SetRange(int range_min, int range_max);
			};
			class HorizontalTrackBar : public Window, public Template::Controls::HorizontalTrackBar
			{
				SafePointer<Shape> _tracker_normal;
				SafePointer<Shape> _tracker_focused;
				SafePointer<Shape> _tracker_hot;
				SafePointer<Shape> _tracker_pressed;
				SafePointer<Shape> _tracker_disabled;
				SafePointer<Shape> _bar_normal;
				SafePointer<Shape> _bar_disabled;
				int _state = 0, _mouse = 0;
				Box GetTrackerPosition(const Box & at);
				int GetTrackerShift(const Box & at);
				int MouseToTracker(const Box & at, int mouse);
			public:
				HorizontalTrackBar(Window * Parent, WindowStation * Station);
				HorizontalTrackBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~HorizontalTrackBar(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void KeyDown(int key_code) override;

				void SetTrackerPosition(int position);
				void SetRange(int range_min, int range_max);
			};
		}
	}
}