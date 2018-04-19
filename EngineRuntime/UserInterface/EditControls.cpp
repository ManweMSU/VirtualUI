#include "EditControls.h"

#include "../PlatformDependent/Clipboard.h"
#include "../PlatformDependent/KeyCodes.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			Edit::Edit(Window * Parent, WindowStation * Station) : Window(Parent, Station), _undo(_text), _text(0x100), _chars_enabled(0x100)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
			}
			Edit::Edit(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station), _undo(_text), _text(0x100), _chars_enabled(0x100)
			{
				if (Template->Properties->GetTemplateClass() != L"Edit") throw InvalidArgumentException();
				static_cast<Template::Controls::Edit &>(*this) = static_cast<Template::Controls::Edit &>(*Template->Properties);
				_text.SetLength(Text.GetEncodedLength(Encoding::UTF32));
				Text.Encode(_text.GetBuffer(), Encoding::UTF32, false);
				_chars_enabled.SetLength(CharactersEnabled.GetEncodedLength(Encoding::UTF32));
				CharactersEnabled.Encode(_chars_enabled.GetBuffer(), Encoding::UTF32, false);
				SafePointer< Array<uint8> > mask = PasswordCharacter.EncodeSequence(Encoding::UTF32, true);
				MemoryCopy(&_mask_char, mask->GetBuffer(), sizeof(uint32));
				_menu.SetReference(ContextMenu ? new Menues::Menu(ContextMenu) : 0);
			}
			Edit::~Edit(void) {}
			void Edit::Render(const Box & at)
			{
				auto device = GetStation()->GetRenderingDevice();
				if (_caret_width < 0) _caret_width = GetStation()->GetVisualStyles().CaretWidth;
				Shape ** back = 0;
				Template::Shape * source = 0;
				UI::Color text_color;
				UI::Color placeholder_color;
				bool focused = false;
				if (Disabled) {
					source = ViewDisabled;
					back = _disabled.InnerRef();
					text_color = ColorDisabled;
					placeholder_color = PlaceholderColorDisabled;
				} else {
					text_color = Color;
					placeholder_color = PlaceholderColor;
					if (GetFocus() == this || _state == 2) {
						focused = true;
						if (ReadOnly) {
							source = ViewFocusedReadOnly;
							back = _focused_readonly.InnerRef();
						} else {
							source = ViewFocused;
							back = _focused.InnerRef();
						}
					} else {
						if (ReadOnly) {
							source = ViewNormalReadOnly;
							back = _normal_readonly.InnerRef();
						} else {
							source = ViewNormal;
							back = _normal.InnerRef();
						}
					}
				}
				if (!(*back) && source) {
					*back = source->Initialize(&ZeroArgumentProvider());
				}
				if (*back) (*back)->Render(device, at);
				Box field = Box(at.Left + Border + LeftSpace, at.Top + Border, at.Right - Border, at.Bottom - Border);
				device->PushClip(field);
				int caret = 0;
				if (_text.Length()) {
					if (Font) {
						if (!_text_info) {
							if (Password) {
								Array<uint32> masked(0x100);
								masked.SetLength(_text.Length());
								for (int i = 0; i < masked.Length(); i++) masked[i] = _mask_char;
								_text_info.SetReference(device->CreateTextRenderingInfo(Font, masked, 0, 1, text_color));
							} else {
								_text_info.SetReference(device->CreateTextRenderingInfo(Font, _text, 0, 1, text_color));
								if (_hook) {
									SafePointer< Array<UI::Color> > colors = _hook->GetPalette(this);
									SafePointer< Array<uint8> > indicies = _hook->ColorHighlight(this, _text);
									if (colors && indicies) {
										_text_info->SetCharPalette(*colors);
										_text_info->SetCharColors(*indicies);
									}
								}
							}
						}
						if (_deferred_scroll) {
							ScrollToCaret();
							_deferred_scroll = false;
						}
						int sp = min(const_cast<const int &>(_sp), _cp);
						int ep = max(const_cast<const int &>(_sp), _cp);
						if (_text_info) {
							_text_info->SetHighlightColor(SelectionColor);
							if (sp != ep && focused) _text_info->HighlightText(sp, ep); else _text_info->HighlightText(-1, -1);
							field.Left += _shift;
							field.Right += _shift;
							if (_cp > 0) caret = _text_info->EndOfChar(_cp - 1);
							caret += _shift;
							device->RenderText(_text_info, field, false);
						}
					}
				} else {
					if (Placeholder.Length() && PlaceholderFont && !_placeholder_info) {
						_placeholder_info.SetReference(device->CreateTextRenderingInfo(PlaceholderFont, Placeholder, 0, 1, placeholder_color));
					}
					if (_placeholder_info && !focused) device->RenderText(_placeholder_info, field, false);
				}
				if (focused) {
					if (!_inversion) {
						_inversion.SetReference(device->CreateInversionEffectRenderingInfo());
					}
					Box caret_box = Box(at.Left + Border + LeftSpace + caret, at.Top + Border,
						at.Left + Border + LeftSpace + caret + _caret_width, at.Bottom - Border);
					device->ApplyInversion(_inversion, caret_box, true);
				}
				device->PopClip();
			}
			void Edit::ResetCache(void)
			{
				_normal.SetReference(0);
				_normal_readonly.SetReference(0);
				_focused.SetReference(0);
				_focused_readonly.SetReference(0);
				_disabled.SetReference(0);
				_text_info.SetReference(0);
				_placeholder_info.SetReference(0);
				_inversion.SetReference(0);
			}
			void Edit::Enable(bool enable)
			{
				Disabled = !enable;
				if (Disabled) { _state = 0; _shift = 0, _cp = 0, _sp = 0; }
				_text_info.SetReference(0);
				_placeholder_info.SetReference(0);
			}
			bool Edit::IsEnabled(void) { return !Disabled; }
			void Edit::Show(bool visible) { Invisible = !visible; if (Disabled) _state = 0; }
			bool Edit::IsVisible(void) { return !Invisible; }
			bool Edit::IsTabStop(void) { return true; }
			void Edit::SetID(int _ID) { ID = _ID; }
			int Edit::GetID(void) { return ID; }
			Window * Edit::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void Edit::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle Edit::GetRectangle(void) { return ControlPosition; }
			void Edit::SetText(const string & text)
			{
				_undo.RemoveAllVersions();
				_save = true;
				Text = text;
				_text.SetLength(Text.GetEncodedLength(Encoding::UTF32));
				Text.Encode(_text.GetBuffer(), Encoding::UTF32, false);
				_text_info.SetReference(0);
				_shift = 0; _cp = 0; _sp = 0;
			}
			string Edit::GetText(void)
			{
				Text = string(_text.GetBuffer(), _text.Length(), Encoding::UTF32);
				return Text;
			}
			void Edit::RaiseEvent(int ID, Event event, Window * sender)
			{
				if (_state == 2) _state = 0;
				if (event == Event::MenuCommand) {
					if (ID == 1001) Undo();
					else if (ID == 1000) Redo();
					else if (ID == 1002) Cut();
					else if (ID == 1003) Copy();
					else if (ID == 1004) Paste();
					else if (ID == 1005) Delete();
					else GetParent()->RaiseEvent(ID, Event::Command, this);
				} else GetParent()->RaiseEvent(ID, event, sender);
			}
			void Edit::PopupMenuCancelled(void) { if (_state == 2) _state = 0; }
			void Edit::FocusChanged(bool got_focus) { if (!got_focus) { _save = true; } }
			void Edit::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; } }
			void Edit::LeftButtonDown(Point at)
			{
				if ((at.x >= Border && at.y >= Border &&
					at.x < WindowPosition.Right - WindowPosition.Left - Border &&
					at.y < WindowPosition.Bottom - WindowPosition.Top - Border)) {
					if (_text_info) {
						_state = 1;
						SetFocus();
						SetCapture();
						_save = true;
						int pos = _text_info->TestPosition(at.x - Border - LeftSpace - _shift);
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = pos;
						_cp = pos;
						ScrollToCaret();
					} else {
						_state = 1;
						SetFocus();
						SetCapture();
						_save = true;
						_cp = 0;
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
						ScrollToCaret();
					}
				}
			}
			void Edit::LeftButtonUp(Point at) { if (_state == 1) ReleaseCapture(); }
			void Edit::LeftButtonDoubleClick(Point at)
			{
				_sp = _cp;
				int len = _text.Length();
				while ((_sp > 0) && (IsAlphabetical(_text[_sp - 1]) || (_text[_sp - 1] >= L'0' && _text[_sp - 1] <= L'9') || (_text[_sp - 1] == L'_'))) _sp--;
				while ((_cp < len) && (IsAlphabetical(_text[_cp]) || (_text[_sp - 1] >= L'0' && _text[_sp - 1] <= L'9') || (_text[_cp] == L'_'))) _cp++;
			}
			void Edit::RightButtonDown(Point at)
			{
				if ((at.x >= Border && at.y >= Border &&
					at.x < WindowPosition.Right - WindowPosition.Left - Border &&
					at.y < WindowPosition.Bottom - WindowPosition.Top - Border)) {
					if (_text_info) {
						SetFocus();
						_save = true;
						int sp = min(const_cast<const int &>(_sp), _cp);
						int ep = max(const_cast<const int &>(_sp), _cp);
						int pos = _text_info->TestPosition(at.x - Border - LeftSpace - _shift);
						if (sp > pos || ep < pos) _cp = _sp = pos;
						ScrollToCaret();
					} else {
						SetFocus();
						_save = true;
						_cp = _sp = 0;
						ScrollToCaret();
					}
				}
			}
			void Edit::RightButtonUp(Point at)
			{
				if (_menu) {
					auto pos = GetStation()->GetCursorPos();
					auto undo = _menu->FindChild(1001);
					auto redo = _menu->FindChild(1000);
					auto cut = _menu->FindChild(1002);
					auto copy = _menu->FindChild(1003);
					auto paste = _menu->FindChild(1004);
					auto remove = _menu->FindChild(1005);
					if (undo) undo->Disabled = ReadOnly || !_undo.CanUndo();
					if (redo) redo->Disabled = ReadOnly || !_undo.CanRedo();
					if (cut) cut->Disabled = ReadOnly || _cp == _sp;
					if (copy) copy->Disabled = _cp == _sp;
					if (paste) paste->Disabled = ReadOnly || !Clipboard::IsFormatAvailable(Clipboard::Format::Text);
					if (remove) remove->Disabled = ReadOnly || _cp == _sp;
					_state = 2;
					_menu->RunPopup(this, pos);
				}
			}
			void Edit::MouseMove(Point at)
			{
				if (_state == 1) {
					_cp = _text_info ? _text_info->TestPosition(at.x - Border - LeftSpace - _shift) : 0;
					ScrollToCaret();
				}
			}
			void Edit::KeyDown(int key_code)
			{
				if (key_code == KeyCodes::Back && !ReadOnly && (_cp != _sp || _cp > 0)) {
					if (_save) {
						_undo.PushCurrentVersion();
						_save = false;
					}
					if (_cp == _sp) _cp = _sp - 1;
					Print(L"");
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Delete && !ReadOnly && (_cp != _sp || _cp < _text.Length())) {
					if (_save) {
						_undo.PushCurrentVersion();
						_save = false;
					}
					if (_cp == _sp) _cp = _sp + 1;
					Print(L"");
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Left && _cp > 0) {
					_save = true;
					_cp--;
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Right && _cp < _text.Length()) {
					_save = true;
					_cp++;
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Escape) {
					_save = true;
					_sp = _cp;
				} else if (key_code == KeyCodes::Home) {
					_save = true;
					_cp = 0;
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::End) {
					_save = true;
					_cp = _text.Length();
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
					_deferred_scroll = true;
				} else if (!Keyboard::IsKeyPressed(KeyCodes::Shift) &&
					Keyboard::IsKeyPressed(KeyCodes::Control) &&
					!Keyboard::IsKeyPressed(KeyCodes::Alternative) &&
					!Keyboard::IsKeyPressed(KeyCodes::System)) {
					if (key_code == KeyCodes::Z) Undo();
					else if (key_code == KeyCodes::X) Cut();
					else if (key_code == KeyCodes::C) Copy();
					else if (key_code == KeyCodes::V) Paste();
					else if (key_code == KeyCodes::Y) Redo();
				}
			}
			void Edit::CharDown(uint32 ucs_code)
			{
				if (!ReadOnly) {
					string filtered = FilterInput(string(&ucs_code, 1, Encoding::UTF32));
					if (filtered.Length()) {
						if (_save) {
							_undo.PushCurrentVersion();
							_save = false;
						}
						Print(filtered);
						_deferred_scroll = true;
					}
				}
			}
			void Edit::SetCursor(Point at)
			{
				SystemCursor cursor = SystemCursor::Arrow;
				if ((at.x >= Border && at.y >= Border && at.x < WindowPosition.Right - WindowPosition.Left - Border &&
					at.y < WindowPosition.Bottom - WindowPosition.Top - Border) || _state) cursor = SystemCursor::Beam;
				GetStation()->SetCursor(GetStation()->GetSystemCursor(cursor));
			}
			void Edit::Undo(void)
			{
				if (!ReadOnly && _undo.CanUndo()) {
					_undo.Undo();
					_cp = _sp = 0;
					_save = true;
					_deferred_scroll = true;
					_text_info.SetReference(0);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			void Edit::Redo(void)
			{
				if (!ReadOnly && _undo.CanRedo()) {
					_undo.Redo();
					_cp = _sp = 0;
					_save = true;
					_deferred_scroll = true;
					_text_info.SetReference(0);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			void Edit::Cut(void)
			{
				if (!ReadOnly && _cp != _sp) {
					_undo.PushCurrentVersion();
					_save = true;
					Clipboard::SetData(GetSelection());
					Print(L"");
					_deferred_scroll = true;
				}
			}
			void Edit::Copy(void)
			{
				if (_cp != _sp) {
					_save = true;
					Clipboard::SetData(GetSelection());
				}
			}
			void Edit::Paste(void)
			{
				if (!ReadOnly) {
					string text;
					if (Clipboard::GetData(text)) {
						string filter = FilterInput(text);
						if (filter.Length()) {
							_undo.PushCurrentVersion();
							_save = true;
							Print(filter);
							_deferred_scroll = true;
						}
					}
				}
			}
			void Edit::Delete(void)
			{
				if (!ReadOnly && _cp != _sp) {
					_undo.PushCurrentVersion();
					_save = true;
					Print(L"");
					_deferred_scroll = true;
				}
			}
			string Edit::GetSelection(void)
			{
				int sp = min(const_cast<const int &>(_sp), _cp);
				int ep = max(const_cast<const int &>(_sp), _cp);
				return string(_text.GetBuffer() + sp, ep - sp, Encoding::UTF32);
			}
			void Edit::SetSelection(int selection_position, int caret_position)
			{
				_sp = min(max(selection_position, 0), _text.Length());
				_cp = min(max(caret_position, 0), _text.Length());
			}
			void Edit::ScrollToCaret(void)
			{
				if (_text_info) {
					int width = WindowPosition.Right - WindowPosition.Left - 2 * Border - LeftSpace;
					int shifted_caret = ((_cp > 0) ? _text_info->EndOfChar(_cp - 1) : 0) + _shift;
					if (shifted_caret < 0) _shift -= shifted_caret;
					else if (shifted_caret + _caret_width >= width) _shift -= shifted_caret + _caret_width - width;
				}
			}
			void Edit::SetPlaceholder(const string & text) { Placeholder = text; _placeholder_info.SetReference(0); }
			string Edit::GetPlaceholder(void) { return Placeholder; }
			void Edit::SetCharacterFilter(const string & filter)
			{
				CharactersEnabled = filter;
				_chars_enabled.SetLength(filter.GetEncodedLength(Encoding::UTF32));
				filter.Encode(_chars_enabled.GetBuffer(), Encoding::UTF32, false);
			}
			string Edit::GetCharacterFilter(void) { return CharactersEnabled; }
			void Edit::SetContextMenu(Menues::Menu * menu) { _menu.SetRetain(menu); }
			Menues::Menu * Edit::GetContextMenu(void) { return _menu; }
			void Edit::SetPasswordMode(bool hide) { Password = hide; _text_info.SetReference(0); }
			bool Edit::GetPasswordMode(void) { return Password; }
			void Edit::SetPasswordChar(uint32 ucs)
			{
				_mask_char = ucs;
				PasswordCharacter = string(&_mask_char, 1, Encoding::UTF32);
				_text_info.SetReference(0);
			}
			uint32 Edit::GetPasswordChar(void) { return _mask_char; }
			void Edit::SetHook(IEditHook * hook) { _hook = hook; _text_info.SetReference(0); }
			Edit::IEditHook * Edit::GetHook(void) { return _hook; }
			string Edit::FilterInput(const string & input)
			{
				string conv = input;
				if (LowerCase) conv = conv.LowerCase();
				else if (UpperCase) conv = conv.UpperCase();
				if (_chars_enabled.Length()) {
					Array<uint32> utf32(0x100);
					utf32.SetLength(conv.GetEncodedLength(Encoding::UTF32));
					conv.Encode(utf32.GetBuffer(), Encoding::UTF32, false);
					for (int i = 0; i < utf32.Length(); i++) {
						bool enabled = false;
						if (utf32[i] >= 0x20) {
							for (int j = 0; j < _chars_enabled.Length(); j++) {
								if (utf32[i] == _chars_enabled[j]) { enabled = true; break; }
							}
						}
						if (!enabled) {
							utf32.Remove(i);
							i--;
						}
					}
					conv = string(utf32.GetBuffer(), utf32.Length(), Encoding::UTF32);
				}
				if (_hook) conv = _hook->Filter(this, conv);
				return conv;
			}
			void Edit::Print(const string & text)
			{
				Array<uint32> utf32(0x100);
				utf32.SetLength(text.GetEncodedLength(Encoding::UTF32));
				text.Encode(utf32.GetBuffer(), Encoding::UTF32, false);
				if (_cp != _sp) {
					int sp = min(const_cast<const int &>(_sp), _cp);
					int ep = max(const_cast<const int &>(_sp), _cp);
					int dl = ep - sp;
					for (int i = ep; i < _text.Length(); i++) _text[i - dl] = _text[i];
					_text.SetLength(_text.Length() - dl);
					_cp = _sp = sp;
				}
				int len = _text.Length();
				_text.SetLength(_text.Length() + utf32.Length());
				for (int i = len - 1; i >= _cp; i--) _text[i + utf32.Length()] = _text[i];
				for (int i = 0; i < utf32.Length(); i++) _text[i + _cp] = utf32[i];
				_cp += utf32.Length(); _sp = _cp;
				_text_info.SetReference(0);
				GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}
			string Edit::IEditHook::Filter(Edit * sender, const string & input) { return input; }
			Array<uint8> * Edit::IEditHook::ColorHighlight(Edit * sender, const Array<uint32>& text) { return 0; }
			Array<UI::Color>* Edit::IEditHook::GetPalette(Edit * sender) { return 0; }
		}
	}
}