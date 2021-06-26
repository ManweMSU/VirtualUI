#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "ScrollableControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class ControlGroup : public ParentControl, public Template::Controls::ControlGroup
			{
			private:
				SafePointer<Shape> _background;
			public:
				ControlGroup(void);
				ControlGroup(Template::ControlTemplate * Template, IControlFactory * Factory = 0);
				~ControlGroup(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;

				void SetInnerControls(Template::ControlTemplate * Template, IControlFactory * Factory = 0);
			};
			class RadioButtonGroup : public ParentControl, public Template::Controls::RadioButtonGroup
			{
			private:
				SafePointer<Shape> _background;
			public:
				RadioButtonGroup(void);
				RadioButtonGroup(Template::ControlTemplate * Template);
				~RadioButtonGroup(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;

				void CheckRadioButton(Control * window);
				void CheckRadioButton(int ID);
				int GetCheckedButton(void);
			};
			class ScrollBoxVirtual : public ParentControl
			{
			public:
				ScrollBoxVirtual(void);
				~ScrollBoxVirtual(void) override;

				virtual void SetPosition(const Box & box) override;
				virtual void ArrangeChildren(void) override;
				virtual string GetControlClass(void) override;
			};
			class ScrollBox : public ParentControl, public Template::Controls::ScrollBox
			{
				ScrollBoxVirtual * _virtual = 0;
				VerticalScrollBar * _vertical = 0;
				HorizontalScrollBar * _horizontal = 0;
				SafePointer<Shape> _background;
				bool _show_vs = false;
				bool _show_hs = false;
			public:
				ScrollBox(void);
				ScrollBox(Template::ControlTemplate * Template, IControlFactory * Factory = 0);
				~ScrollBox(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
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
				virtual void RaiseEvent(int ID, ControlEvent event, Control * sender) override;
				virtual void ScrollVertically(double delta) override;
				virtual void ScrollHorizontally(double delta) override;
				virtual Control * HitTest(Point at) override;
				virtual string GetControlClass(void) override;

				Control * GetVirtualGroup(void);
			};
			class SplitBoxPart : public ParentControl, public Template::Controls::SplitBoxPart
			{
			public:
				int Size;
				SplitBoxPart(void);
				SplitBoxPart(Template::ControlTemplate * Template, IControlFactory * Factory = 0);
				~SplitBoxPart(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;
			};
			class VerticalSplitBox : public ParentControl, public Template::Controls::VerticalSplitBox
			{
				SafePointer<Shape> _splitter_normal;
				SafePointer<Shape> _splitter_hot;
				SafePointer<Shape> _splitter_pressed;
				int _state = 0, _part = -1, _mouse = 0;
				bool _initialized = false;
			public:
				VerticalSplitBox(void);
				VerticalSplitBox(Template::ControlTemplate * Template, IControlFactory * Factory = 0);
				~VerticalSplitBox(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
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
				virtual string GetControlClass(void) override;
			};
			class HorizontalSplitBox : public ParentControl, public Template::Controls::HorizontalSplitBox
			{
				SafePointer<Shape> _splitter_normal;
				SafePointer<Shape> _splitter_hot;
				SafePointer<Shape> _splitter_pressed;
				int _state = 0, _part = -1, _mouse = 0;
				bool _initialized = false;
			public:
				HorizontalSplitBox(void);
				HorizontalSplitBox(Template::ControlTemplate * Template, IControlFactory * Factory = 0);
				~HorizontalSplitBox(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
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
				virtual string GetControlClass(void) override;
			};
			class BookmarkView : public ParentControl, public Template::Controls::BookmarkView
			{
				struct Bookmark
				{
					int ID;
					string Text;
					Control * Group;
					SafePointer<Shape> _view_normal;
					SafePointer<Shape> _view_hot;
					SafePointer<Shape> _view_active;
					SafePointer<Shape> _view_active_focused;
					int _width;
				};
				Array<Bookmark> _bookmarks;
				int _active = -1;
				int _hot = -1;
				int _shift = 0;
				SafePointer<Shape> _view_background;
			public:
				BookmarkView(void);
				BookmarkView(Template::ControlTemplate * Template, IControlFactory * Factory = 0);
				~BookmarkView(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual bool KeyDown(int key_code) override;
				virtual string GetControlClass(void) override;

				int FindBookmark(int ID);
				int GetBookmarkCount(void);
				int GetBookmarkID(int index);
				string GetBookmarkText(int index);
				int GetBookmarkWidth(int index);
				Control * GetBookmarkView(int index);
				void SetBookmarkID(int index, int ID);
				void SetBookmarkText(int index, const string & text);
				void SetBookmarkWidth(int index, int width);
				void OrderBookmark(int index, int new_index);
				void RemoveBookmark(int index);
				void AddBookmark(int width, int ID, const string & text);
				void AddBookmark(int width, int ID, const string & text, Template::ControlTemplate * view_template, IControlFactory * factory = 0);
				int GetActiveBookmark(void);
				void ActivateBookmark(int index);
			};
		}
	}
}