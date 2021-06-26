#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class VerticalScrollBar : public Control, public Template::Controls::VerticalScrollBar
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
				Box GetScrollerBox(const Box & at);
			public:
				VerticalScrollBar(void);
				VerticalScrollBar(Template::ControlTemplate * Template);
				VerticalScrollBar(Template::Controls::Scrollable * Template);
				VerticalScrollBar(Template::Controls::VerticallyScrollable * Template);
				~VerticalScrollBar(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void Timer(void) override;
				virtual string GetControlClass(void) override;

				void SetScrollerPosition(int position);
				void SetPage(int page);
				void SetRange(int range_min, int range_max);
				void SetScrollerPositionSilent(int position);
				void SetPageSilent(int page);
				void SetRangeSilent(int range_min, int range_max);
			};
			class HorizontalScrollBar : public Control, public Template::Controls::HorizontalScrollBar
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
				Box GetScrollerBox(const Box & at);
			public:
				HorizontalScrollBar(void);
				HorizontalScrollBar(Template::ControlTemplate * Template);
				HorizontalScrollBar(Template::Controls::Scrollable * Template);
				~HorizontalScrollBar(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void Timer(void) override;
				virtual string GetControlClass(void) override;

				void SetScrollerPosition(int position);
				void SetPage(int page);
				void SetRange(int range_min, int range_max);
				void SetScrollerPositionSilent(int position);
				void SetPageSilent(int page);
				void SetRangeSilent(int range_min, int range_max);
			};
			class VerticalTrackBar : public Control, public Template::Controls::VerticalTrackBar
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
				VerticalTrackBar(void);
				VerticalTrackBar(Template::ControlTemplate * Template);
				~VerticalTrackBar(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual bool KeyDown(int key_code) override;
				virtual string GetControlClass(void) override;

				void SetTrackerPosition(int position);
				void SetRange(int range_min, int range_max);
				void SetTrackerPositionSilent(int position);
				void SetRangeSilent(int range_min, int range_max);
			};
			class HorizontalTrackBar : public Control, public Template::Controls::HorizontalTrackBar
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
				HorizontalTrackBar(void);
				HorizontalTrackBar(Template::ControlTemplate * Template);
				~HorizontalTrackBar(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual bool KeyDown(int key_code) override;
				virtual string GetControlClass(void) override;

				void SetTrackerPosition(int position);
				void SetRange(int range_min, int range_max);
				void SetTrackerPositionSilent(int position);
				void SetRangeSilent(int range_min, int range_max);
			};
		}
	}
}