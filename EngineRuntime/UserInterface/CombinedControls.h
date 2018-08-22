#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "ScrollableControls.h"
#include "Menues.h"
#include "../Miscellaneous/UndoBuffer.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class ComboBox : public Window, public Template::Controls::ComboBox
			{
				friend class ComboListBox;
			private:
				struct Element
				{
					SafePointer<Shape> ViewNormal;
					SafePointer<Shape> ViewDisabled;
					void * User;
				};
				class ComboListBox : public ParentWindow
				{
					friend class ComboBox;
				private:
					ComboBox * _owner = 0;
					VerticalScrollBar * _scroll = 0;
					bool _svisible = false;
					int _hot = -1;
					SafePointer<Shape> _view;
					SafePointer<Shape> _view_element_hot;
					SafePointer<Shape> _view_element_selected;
					Rectangle _position;

					void scroll_to_current(void);
					void move_selection(int to);
				public:
					ComboListBox(Window * Parent, WindowStation * Station, ComboBox * Base);
					~ComboListBox(void) override;

					virtual void Render(const Box & at) override;
					virtual void ResetCache(void) override;
					virtual void SetRectangle(const Rectangle & rect) override;
					virtual Rectangle GetRectangle(void) override;
					virtual void SetPosition(const Box & box) override;
					virtual void CaptureChanged(bool got_capture) override;
					virtual void LostExclusiveMode(void) override;
					virtual void LeftButtonUp(Point at) override;
					virtual void MouseMove(Point at) override;
					virtual void ScrollVertically(double delta) override;
					virtual void KeyDown(int key_code) override;
					virtual Window * HitTest(Point at) override;
				};

				ComboListBox * _list = 0;
				Array<Element> _elements;
				int _current = -1;
				SafePointer<Shape> _viewport_element;
				int _state = 0;
				SafePointer<Shape> _view_normal;
				SafePointer<Shape> _view_disabled;
				SafePointer<Shape> _view_focused;
				SafePointer<Shape> _view_hot;
				SafePointer<Shape> _view_hot_focused;
				SafePointer<Shape> _view_pressed;

				void invalidate_viewport(void);
				void run_drop_down(void);
			public:
				ComboBox(Window * Parent, WindowStation * Station);
				ComboBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ComboBox(void) override;

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
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void KeyDown(int key_code) override;

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
				void SetSelectedIndex(int index);
			};
			class TextComboBox : public Window, public Template::Controls::TextComboBox
			{
				friend class TextComboListBox;
			private:
				class TextComboListBox : public ParentWindow
				{
					friend class TextComboBox;
				private:
					TextComboBox * _owner = 0;
					VerticalScrollBar * _scroll = 0;
					bool _svisible = false;
					int _current = -1, _hot = -1;
					SafePointer<Shape> _view;
					SafePointer<Shape> _view_element_hot;
					SafePointer<Shape> _view_element_selected;
					Rectangle _position;
					ObjectArray<Shape> _elements;

					void scroll_to_current(void);
					void move_selection(int to);
				public:
					TextComboListBox(Window * Parent, WindowStation * Station, TextComboBox * Base);
					~TextComboListBox(void) override;

					virtual void Render(const Box & at) override;
					virtual void ResetCache(void) override;
					virtual void SetRectangle(const Rectangle & rect) override;
					virtual Rectangle GetRectangle(void) override;
					virtual void SetPosition(const Box & box) override;
					virtual void CaptureChanged(bool got_capture) override;
					virtual void LostExclusiveMode(void) override;
					virtual void LeftButtonUp(Point at) override;
					virtual void MouseMove(Point at) override;
					virtual void ScrollVertically(double delta) override;
					virtual void KeyDown(int key_code) override;
					virtual Window * HitTest(Point at) override;
				};

				TextComboListBox * _list = 0;
				Array<string> _elements;
				int _state = 0;
				SafePointer<Shape> _view_frame_normal;
				SafePointer<Shape> _view_frame_focused;
				SafePointer<Shape> _view_frame_disabled;
				SafePointer<Shape> _view_button_normal;
				SafePointer<Shape> _view_button_hot;
				SafePointer<Shape> _view_button_pressed;
				SafePointer<Shape> _view_button_disabled;
				SafePointer<Menues::Menu> _menu;
				SafePointer<ITextRenderingInfo> _text_info;
				SafePointer<ITextRenderingInfo> _advice_info;
				SafePointer<ITextRenderingInfo> _placeholder_info;
				SafePointer<IInversionEffectRenderingInfo> _inversion;
				Array<uint32> _text;
				UndoBuffer< Array<uint32> > _undo;
				int _advice = -1;
				Array<uint32> _chars_enabled;
				int _sp = 0, _cp = 0;
				int _shift = 0;
				int _caret_width = -1;
				bool _save = true;
				bool _deferred_scroll = false;

				void find_advice(void);
				void run_drop_down(void);
			public:
				TextComboBox(Window * Parent, WindowStation * Station);
				TextComboBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~TextComboBox(void) override;

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
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void RightButtonDown(Point at) override;
				virtual void RightButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void KeyDown(int key_code) override;
				virtual void CharDown(uint32 ucs_code) override;
				virtual void PopupMenuCancelled(void) override;
				virtual void SetCursor(Point at) override;
				virtual RefreshPeriod FocusedRefreshPeriod(void) override;

				void Undo(void);
				void Redo(void);
				void Cut(void);
				void Copy(void);
				void Paste(void);
				void Delete(void);
				string GetSelection(void);
				void SetSelection(int selection_position, int caret_position);
				void ScrollToCaret(void);
				void SetPlaceholder(const string & text);
				string GetPlaceholder(void);
				void SetCharacterFilter(const string & filter);
				string GetCharacterFilter(void);
				void SetContextMenu(Menues::Menu * menu);
				Menues::Menu * GetContextMenu(void);
				string FilterInput(const string & input);
				void Print(const string & text);

				void AddItem(const string & text);
				void InsertItem(const string & text, int at);
				void SwapItems(int i, int j);
				void RemoveItem(int index);
				void ClearItems(void);
				int ItemCount(void);
				void SetItemText(int index, const string & text);
				string GetItemText(int index);
			};
		}
	}
}