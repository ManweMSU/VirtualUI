#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "OverlappedWindows.h"
#include "ScrollableControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class ControlGroup : public ParentWindow, public Template::Controls::ControlGroup
			{
			private:
				SafePointer<Shape> _background;
			public:
				ControlGroup(Window * Parent, WindowStation * Station);
				ControlGroup(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ControlGroup(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;

				void SetInnerControls(Template::ControlTemplate * Template);
			};
			class RadioButtonGroup : public ParentWindow, public Template::Controls::RadioButtonGroup
			{
			private:
				SafePointer<Shape> _background;
			public:
				RadioButtonGroup(Window * Parent, WindowStation * Station);
				RadioButtonGroup(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~RadioButtonGroup(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;

				void CheckRadioButton(Window * window);
				void CheckRadioButton(int ID);
				int GetCheckedButton(void);
			};
			class ScrollBoxVirtual : public ParentWindow
			{
			public:
				ScrollBoxVirtual(Window * Parent, WindowStation * Station);
				~ScrollBoxVirtual(void) override;

				virtual void SetPosition(const Box & box) override;
				virtual void ArrangeChildren(void) override;
			};
			class ScrollBox : public ParentWindow, public Template::Controls::ScrollBox
			{
				ScrollBoxVirtual * _virtual = 0;
				VerticalScrollBar * _vertical = 0;
				HorizontalScrollBar * _horizontal = 0;
				SafePointer<Shape> _background;
				bool _show_vs = false;
				bool _show_hs = false;
			public:
				ScrollBox(Window * Parent, WindowStation * Station);
				ScrollBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ScrollBox(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void ScrollVertically(double delta) override;
				virtual void ScrollHorizontally(double delta) override;
				virtual Window * HitTest(Point at) override;

				Window * GetVirtualGroup(void);
			};
			class SplitBoxPart : public ParentWindow, public Template::Controls::SplitBoxPart
			{
			public:
				int Size;
				SplitBoxPart(Window * Parent, WindowStation * Station);
				SplitBoxPart(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~SplitBoxPart(void) override;

				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
			};
			class VerticalSplitBox : public ParentWindow, public Template::Controls::VerticalSplitBox
			{
				SafePointer<Shape> _splitter_normal;
				SafePointer<Shape> _splitter_hot;
				SafePointer<Shape> _splitter_pressed;
				int _state = 0, _part = -1, _mouse = 0;
				bool _initialized = false;
			public:
				VerticalSplitBox(Window * Parent, WindowStation * Station);
				VerticalSplitBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~VerticalSplitBox(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void SetCursor(Point at) override;
			};
			class HorizontalSplitBox : public ParentWindow, public Template::Controls::HorizontalSplitBox
			{
				SafePointer<Shape> _splitter_normal;
				SafePointer<Shape> _splitter_hot;
				SafePointer<Shape> _splitter_pressed;
				int _state = 0, _part = -1, _mouse = 0;
				bool _initialized = false;
			public:
				HorizontalSplitBox(Window * Parent, WindowStation * Station);
				HorizontalSplitBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~HorizontalSplitBox(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void SetCursor(Point at) override;
			};
		}
	}
}