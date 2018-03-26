#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "Menues.h"

#include "../Miscellaneous/UndoBuffer.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class Edit : public Window, public Template::Controls::Edit
			{
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
				void SetCharacterFilther(const string & filther);
				string GetCharacterFilther(void);
				void SetContextMenu(Menues::Menu * menu);
				Menues::Menu * GetContextMenu(void);
				string FiltherInput(const string & input);
				void Print(const string & text);
			};
		}
	}
}