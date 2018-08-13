#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "GroupControls.h"
#include "ScrollableControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class ListBox : public ParentWindow, public Template::Controls::ListBox
			{
			private:
				struct Element
				{
					SafePointer<Shape> ViewNormal;
					SafePointer<Shape> ViewDisabled;
					void * User;
					bool Selected;
				};
				VerticalScrollBar * _scroll = 0;
				ControlGroup * _editor = 0;
				bool _svisible = false;
				Array<Element> _elements;
				int _current = -1;
				int _hot = -1;
				SafePointer<Shape> _view_normal;
				SafePointer<Shape> _view_disabled;
				SafePointer<Shape> _view_focused;
				SafePointer<Shape> _view_element_hot;
				SafePointer<Shape> _view_element_selected;

				void reset_scroll_ranges(void);
				void scroll_to_current(void);
				void select(int index, bool down);
				void move_selection(int to);
			public:
				ListBox(Window * Parent, WindowStation * Station);
				ListBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ListBox(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void RightButtonDown(Point at) override;
				virtual void RightButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void ScrollVertically(double delta) override;
				virtual void KeyDown(int key_code) override;
				virtual Window * HitTest(Point at) override;

				void AddItem(const string & text, void * user = 0);
				void AddItem(IArgumentProvider * provider, void * user = 0);
				void AddItem(Reflection::Reflected & object, void * user = 0);
				void InsertItem(const string & text, int at, void * user = 0);
				void InsertItem(IArgumentProvider * provider, int at, void * user = 0);
				void InsertItem(Reflection::Reflected & object, int at, void * user = 0);
				void ResetItem(int index, const string & text);
				void ResetItem(int index, IArgumentProvider * provider);
				void ResetItem(int index, Reflection::Reflected & object);
				void SwapItems(int i, int j);
				void RemoveItem(int index);
				void ClearItems(void);
				int ItemCount(void);
				void * GetItemUserData(int index);
				void SetItemUserData(int index, void * user);
				int GetSelectedIndex(void);
				void SetSelectedIndex(int index, bool scroll_to_view = false);
				bool IsItemSelected(int index);
				void SelectItem(int index, bool select);
				Window * CreateEmbeddedEditor(Template::ControlTemplate * Template, const Rectangle & Position = Rectangle::Entire());
				Window * GetEmbeddedEditor(void);
				void CloseEmbeddedEditor(void);
			};
		}
	}
}