#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "Menues.h"
#include "ScrollableControls.h"

#include "../Miscellaneous/UndoBuffer.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class Edit : public Window, public Template::Controls::Edit
			{
			public:
				class IEditHook
				{
				public:
					virtual string Filter(Edit * sender, const string & input);
					virtual Array<uint8> * ColorHighlight(Edit * sender, const Array<uint32> & text);
					virtual Array<UI::Color> * GetPalette(Edit * sender);
				};
			private:
				SafePointer<Shape> _normal;
				SafePointer<Shape> _focused;
				SafePointer<Shape> _normal_readonly;
				SafePointer<Shape> _focused_readonly;
				SafePointer<Shape> _disabled;
				SafePointer<Menues::Menu> _menu;
				SafePointer<ITextRenderingInfo> _text_info;
				SafePointer<ITextRenderingInfo> _placeholder_info;
				SafePointer<IInversionEffectRenderingInfo> _inversion;
				Array<uint32> _text;
				UndoBuffer< Array<uint32> > _undo;
				uint32 _mask_char = L'*';
				Array<uint32> _chars_enabled;
				int _sp = 0, _cp = 0;
				int _shift = 0;
				int _state = 0;
				int _caret_width = -1;
				bool _save = true;
				bool _deferred_scroll = false;
				IEditHook * _hook = 0;
			public:
				Edit(Window * Parent, WindowStation * Station);
				Edit(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~Edit(void) override;

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
				virtual void PopupMenuCancelled(void) override;
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
				virtual void SetCursor(Point at) override;

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
				void SetPasswordMode(bool hide);
				bool GetPasswordMode(void);
				void SetPasswordChar(uint32 ucs);
				uint32 GetPasswordChar(void);
				void SetHook(IEditHook * hook);
				IEditHook * GetHook(void);
				string FilterInput(const string & input);
				void Print(const string & text);
			};
			class MultiLineEdit : public ParentWindow, public Template::Controls::MultiLineEdit
			{
			public:
				class IMultiLineEditHook
				{
				public:
					virtual string Filter(MultiLineEdit * sender, const string & input, Point insert_at);
					virtual Array<uint8> * ColorHighlight(MultiLineEdit * sender, const Array<uint32> & text, int line);
					virtual Array<UI::Color> * GetPalette(MultiLineEdit * sender);
				};
				struct EditorCoord
				{
					int x = 0, y = 0;

					EditorCoord(void);
					EditorCoord(int sx, int sy);

					bool friend operator == (const EditorCoord & a, const EditorCoord & b);
					bool friend operator != (const EditorCoord & a, const EditorCoord & b);
					bool friend operator < (const EditorCoord & a, const EditorCoord & b);
					bool friend operator > (const EditorCoord & a, const EditorCoord & b);
					bool friend operator <= (const EditorCoord & a, const EditorCoord & b);
					bool friend operator >= (const EditorCoord & a, const EditorCoord & b);
				};
				struct EditorLine
				{
					Array<uint32> text = Array<uint32>(0x20);
					int width = 0;
					uint64 user_data = 0;

					bool friend operator == (const EditorLine & a, const EditorLine & b);
					bool friend operator != (const EditorLine & a, const EditorLine & b);
				};
				struct EditorContent
				{
					Array<EditorLine> lines = Array<EditorLine>(0x20);
					EditorCoord sp, cp;

					bool friend operator == (const EditorContent & a, const EditorContent & b);
					bool friend operator != (const EditorContent & a, const EditorContent & b);
				};
			private:
				VerticalScrollBar * _vscroll = 0;
				HorizontalScrollBar * _hscroll = 0;
				SafePointer<Shape> _normal;
				SafePointer<Shape> _focused;
				SafePointer<Shape> _normal_readonly;
				SafePointer<Shape> _focused_readonly;
				SafePointer<Shape> _disabled;
				SafePointer<Menues::Menu> _menu;
				SafePointer<IInversionEffectRenderingInfo> _inversion;
				EditorContent _content;
				ObjectArray<ITextRenderingInfo> _text_info;
				LimitedUndoBuffer<EditorContent> _undo;
				Array<uint32> _chars_enabled;
				int _state = 0;
				bool _save = true;
				bool _deferred_scroll = false;
				bool _deferred_update = false;
				int _caret_width = -1;
				int _fh = 1, _fw = 1;
				IMultiLineEditHook * _hook = 0;
			public:
				MultiLineEdit(Window * Parent, WindowStation * Station);
				MultiLineEdit(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~MultiLineEdit(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
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
				virtual void ScrollVertically(int delta) override;
				virtual void ScrollHorizontally(int delta) override;
				virtual void KeyDown(int key_code) override;
				virtual void CharDown(uint32 ucs_code) override;
				virtual void PopupMenuCancelled(void) override;
				virtual void SetCursor(Point at) override;

				void Undo(void);
				void Redo(void);
				void Cut(void);
				void Copy(void);
				void Paste(void);
				void Delete(void);
				string GetSelection(void);
				void SetSelection(Point selection_position, Point caret_position);
				Point GetCaretPosition(void);
				Point GetSelectionPosition(void);
				void ScrollToCaret(void);
				void SetCharacterFilter(const string & filter);
				string GetCharacterFilter(void);
				void SetContextMenu(Menues::Menu * menu);
				Menues::Menu * GetContextMenu(void);
				void SetHook(IMultiLineEditHook * hook);
				IMultiLineEditHook * GetHook(void);
				const Array<uint32> & GetRawLine(int line_index);
				int GetLineCount(void);
				void InvalidateLine(int line_index);
				uint64 GetUserDataForRawLine(int line_index);
				void SetUserDataForRawLine(int line_index, uint64 value);
				string FilterInput(const string & input);
				void Print(const string & text);
			};
		}
	}
}