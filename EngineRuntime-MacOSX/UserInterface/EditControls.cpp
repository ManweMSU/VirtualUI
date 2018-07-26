#include "EditControls.h"

#include "../PlatformDependent/Clipboard.h"
#include "../PlatformDependent/KeyCodes.h"
#include "../Miscellaneous/DynamicString.h"

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
					auto args = ZeroArgumentProvider();
					*back = source->Initialize(&args);
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
			void Edit::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; }
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
				while (_sp > 0 && ((IsAlphabetical(_text[_sp - 1]) || (_text[_sp - 1] >= L'0' && _text[_sp - 1] <= L'9') || (_text[_sp - 1] == L'_')))) _sp--;
				while (_cp < len && ((IsAlphabetical(_text[_cp]) || (_text[_cp - 1] >= L'0' && _text[_cp - 1] <= L'9') || (_text[_cp] == L'_')))) _cp++;
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
			Window::RefreshPeriod Edit::FocusedRefreshPeriod(void) { return RefreshPeriod::CaretBlink; }
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

			MultiLineEdit::MultiLineEdit(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station), _undo(_content, 10), _chars_enabled(0x100), _text_info(0x20)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				ResetCache();
				_content.lines << EditorLine();
				_text_info.Append(0);
			}
			MultiLineEdit::MultiLineEdit(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station), _undo(_content, 10), _chars_enabled(0x100), _text_info(0x20)
			{
				if (Template->Properties->GetTemplateClass() != L"MultiLineEdit") throw InvalidArgumentException();
				static_cast<Template::Controls::MultiLineEdit &>(*this) = static_cast<Template::Controls::MultiLineEdit &>(*Template->Properties);
				_menu.SetReference(ContextMenu ? new Menues::Menu(ContextMenu) : 0);
				ResetCache(); _chars_enabled.SetLength(CharactersEnabled.GetEncodedLength(Encoding::UTF32));
				CharactersEnabled.Encode(_chars_enabled.GetBuffer(), Encoding::UTF32, false);
				SetText(Text);
			}
			MultiLineEdit::~MultiLineEdit(void) {}
			void MultiLineEdit::Render(const Box & at)
			{
				auto device = GetStation()->GetRenderingDevice();
				if (_caret_width < 0) _caret_width = GetStation()->GetVisualStyles().CaretWidth;
				Shape ** back = 0;
				Template::Shape * source = 0;
				UI::Color text_color;
				bool focused = false;
				if (Disabled) {
					source = ViewDisabled;
					back = _disabled.InnerRef();
					text_color = ColorDisabled;
				} else {
					text_color = Color;
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
					auto args = ZeroArgumentProvider();
					*back = source->Initialize(&args);
				}
				if (*back) (*back)->Render(device, at);
				Box field = Box(at.Left + Border, at.Top + Border, at.Right - Border - VerticalScrollSize, at.Bottom - Border - HorizontalScrollSize);		
				device->PushClip(field);
				int caret_x = -1;
				int caret_y = - _fh - _fh;
				if (Font) {
					if (_deferred_update) {
						_vscroll->SetPageSilent(field.Bottom - field.Top);
						_hscroll->SetPageSilent(field.Right - field.Left);
					}
					int vpl = _vscroll->Position / _fh;
					int vph = (_vscroll->Position + _vscroll->Page) / _fh + 1;
					for (int i = max(vpl, 0); i < min(vph, _content.lines.Length()); i++) {
						SafePointer<ITextRenderingInfo> line = device->CreateTextRenderingInfo(Font, _content.lines[i].text, 0, 0, text_color);
						if (_hook) {
							SafePointer< Array<UI::Color> > colors = _hook->GetPalette(this);
							SafePointer< Array<uint8> > indicies = _hook->ColorHighlight(this, _content.lines[i].text, i);
							if (colors && indicies) {
								line->SetCharPalette(*colors);
								line->SetCharColors(*indicies);
							}
						}
						line->SetHighlightColor(SelectionColor);
						_text_info.SetElement(line, i);
						int w, h;
						line->GetExtent(w, h);
						_content.lines[i].width = w;
					}
					if (_deferred_update) {
						int mw = 0;
						for (int i = 0; i < _content.lines.Length(); i++) if (_content.lines[i].width > mw) mw = _content.lines[i].width;
						_vscroll->SetRangeSilent(0, _fh * _content.lines.Length());	
						if (!Disabled) {
							_vscroll->Enable(true);
						} else _vscroll->Enable(false);
						_hscroll->SetRangeSilent(0, mw - 1 + _caret_width);	
						if (!Disabled) {
							_hscroll->Enable(true);
						} else _hscroll->Enable(false);
						_deferred_update = false;
					}
					if (_deferred_scroll) {
						ScrollToCaret();
						_deferred_scroll = false;
					}
					auto sp = min(_content.sp, _content.cp);
					auto ep = max(_content.sp, _content.cp);
					for (int i = max(vpl, 0); i < min(vph, _content.lines.Length()); i++) {
						if (_text_info.ElementAt(i)) {
							if (sp != ep && focused) {
								if (sp.y != ep.y) {
									if (i < sp.y) _text_info[i].HighlightText(-1, -1);
									else if (i == sp.y) _text_info[i].HighlightText(sp.x, _content.lines[i].text.Length());
									else if (i > sp.y && i < ep.y) _text_info[i].HighlightText(0, _content.lines[i].text.Length());
									else if (i == ep.y) _text_info[i].HighlightText(0, ep.x);
									else _text_info[i].HighlightText(-1, -1);
								} else {
									if (sp.y == i) _text_info[i].HighlightText(sp.x, ep.x); else _text_info[i].HighlightText(-1, -1);
								}
							}
							if (_content.cp.y == i) {
								caret_y = i * _fh - _vscroll->Position;
								if (_content.cp.x > 0) caret_x = _text_info[i].EndOfChar(_content.cp.x - 1); else caret_x = 0;
								caret_x -= _hscroll->Position;
							}
							Box rb(field.Left - _hscroll->Position, field.Top - _vscroll->Position + i * _fh, field.Right, 0);
							rb.Bottom = rb.Top + _fh;
							device->RenderText(_text_info.ElementAt(i), rb, false);
						}
					}
				}
				if (focused && caret_y >= -_fh) {
					if (!_inversion) {
						_inversion.SetReference(device->CreateInversionEffectRenderingInfo());
					}
					Box caret_box = Box(at.Left + Border + caret_x, at.Top + Border + caret_y,
						at.Left + Border + caret_x + _caret_width, at.Top + Border + caret_y + _fh);
					device->ApplyInversion(_inversion, caret_box, true);
				}
				device->PopClip();
				ParentWindow::Render(at);
			}
			void MultiLineEdit::ResetCache(void)
			{
				if (_vscroll) _vscroll->Destroy();
				if (_hscroll) _hscroll->Destroy();
				_vscroll = GetStation()->CreateWindow<VerticalScrollBar>(this, this);
				_hscroll = GetStation()->CreateWindow<HorizontalScrollBar>(this, this);
				_vscroll->SetRectangle(Rectangle(
					Coordinate::Right() - Border - VerticalScrollSize, Border,
					Coordinate::Right() - Border, Coordinate::Bottom() - Border - HorizontalScrollSize));
				_hscroll->SetRectangle(Rectangle(
					Border, Coordinate::Bottom() - Border - HorizontalScrollSize,
					Coordinate::Right() - Border - VerticalScrollSize, Coordinate::Bottom() - Border));
				_vscroll->Disabled = _hscroll->Disabled = Disabled;
				_vscroll->Line = Font ? Font->GetWidth() : 1;
				_hscroll->Line = Font ? Font->GetHeight() : 1;
				_deferred_update = true;
				for (int i = 0; i < _text_info.Length(); i++) _text_info.SetElement(0, i);
				_inversion.SetReference(0);
				_normal.SetReference(0);
				_focused.SetReference(0);
				_normal_readonly.SetReference(0);
				_focused_readonly.SetReference(0);
				_disabled.SetReference(0);
				if (Font) {
					_fw = Font->GetWidth();
					_fh = Font->GetHeight();
				}
				ParentWindow::ResetCache();
			}
			void MultiLineEdit::Enable(bool enable)
			{
				Disabled = !enable;
				if (Disabled) { _state = 0; _vscroll->Enable(false); _hscroll->Enable(false); }
				else { _deferred_update = true; }
				for (int i = 0; i < _text_info.Length(); i++) _text_info.SetElement(0, i);
			}
			bool MultiLineEdit::IsEnabled(void) { return !Disabled; }
			void MultiLineEdit::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; }
			bool MultiLineEdit::IsVisible(void) { return !Invisible; }
			bool MultiLineEdit::IsTabStop(void) { return true; }
			void MultiLineEdit::SetID(int _ID) { ID = _ID; }
			int MultiLineEdit::GetID(void) { return ID; }
			void MultiLineEdit::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle MultiLineEdit::GetRectangle(void) { return ControlPosition; }
			void MultiLineEdit::SetPosition(const Box & box) { ParentWindow::SetPosition(box); _deferred_update = true; }
			void MultiLineEdit::SetText(const string & text)
			{
				_undo.RemoveAllVersions();
				_content.lines.Clear();
				_save = true;
				Array<uint32> chars(0x100);
				chars.SetLength(text.GetEncodedLength(Encoding::UTF32));
				text.Encode(chars.GetBuffer(), Encoding::UTF32, false);
				int sp = 0;
				bool nle = false;
				while (sp < chars.Length()) {
					int ep = sp;
					while (ep < chars.Length() && chars[ep] != L'\n') ep++;
					nle = chars[ep] == L'\n';
					int vcc = 0;
					for (int i = sp; i < ep; i++) if (chars[i] >= 0x20 || chars[i] == L'\t') vcc++;
					_content.lines << EditorLine();
					_content.lines.LastElement().text.SetLength(vcc);
					int wp = 0;
					for (int i = sp; i < ep; i++) if (chars[i] >= 0x20 || chars[i] == L'\t') { _content.lines.LastElement().text[wp] = chars[i]; wp++; }
					sp = ep + 1;
				}
				if (nle) _content.lines << EditorLine();
				_text_info.Clear();
				for (int i = 0; i < _content.lines.Length(); i++) _text_info.Append(0);
				_content.cp = _content.sp = EditorCoord();
				_vscroll->Position = 0;
				_hscroll->Position = 0;
				_deferred_update = true;
			}
			string MultiLineEdit::GetText(void)
			{
				DynamicString result;
				for (int i = 0; i < _content.lines.Length(); i++) {
					if (i) result += IO::NewLineChar;
					result += string(_content.lines[i].text.GetBuffer(), _content.lines[i].text.Length(), Encoding::UTF32);
				}
				return result.ToString();
			}
			void MultiLineEdit::RaiseEvent(int ID, Event event, Window * sender)
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
				} else if (ID) GetParent()->RaiseEvent(ID, event, sender);
			}
			void MultiLineEdit::FocusChanged(bool got_focus) { if (!got_focus) { _save = true; } }
			void MultiLineEdit::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; } }
			void MultiLineEdit::LeftButtonDown(Point at)
			{
				if ((at.x >= Border && at.y >= Border &&
					at.x < WindowPosition.Right - WindowPosition.Left - Border - VerticalScrollSize &&
					at.y < WindowPosition.Bottom - WindowPosition.Top - Border - HorizontalScrollSize)) {
					int y = (at.y - Border + _vscroll->Position) / _fh;
					_state = 1;
					SetFocus();
					SetCapture();
					_save = true;
					if (y < 0) {
						_content.cp = EditorCoord();
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					} else if (y >= _content.lines.Length()) {
						_content.cp = EditorCoord(_content.lines.LastElement().text.Length(), _content.lines.Length() - 1);
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					} else if (_text_info.ElementAt(y)) {
						EditorCoord pos = EditorCoord(_text_info[y].TestPosition(at.x - Border + _hscroll->Position), y);
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = pos;
						_content.cp = pos;
					} else {
						_content.cp = EditorCoord();
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					}
					ScrollToCaret();
				}
			}
			void MultiLineEdit::LeftButtonUp(Point at) { if (_state == 1) ReleaseCapture(); }
			void MultiLineEdit::LeftButtonDoubleClick(Point at)
			{
				_content.sp = _content.cp;
				int len = _content.lines[_content.cp.y].text.Length();
				auto & _sp = _content.sp.x;
				auto & _cp = _content.cp.x;
				auto & _text = _content.lines[_content.cp.y].text;
				while (_sp > 0 && ((IsAlphabetical(_text[_sp - 1]) || (_text[_sp - 1] >= L'0' && _text[_sp - 1] <= L'9') || (_text[_sp - 1] == L'_')))) _sp--;
				while (_cp < len && ((IsAlphabetical(_text[_cp]) || (_text[_cp - 1] >= L'0' && _text[_cp - 1] <= L'9') || (_text[_cp] == L'_')))) _cp++;
			}
			void MultiLineEdit::RightButtonDown(Point at)
			{
				if ((at.x >= Border && at.y >= Border &&
					at.x < WindowPosition.Right - WindowPosition.Left - Border - VerticalScrollSize &&
					at.y < WindowPosition.Bottom - WindowPosition.Top - Border - HorizontalScrollSize)) {
					int y = (at.y - Border + _vscroll->Position) / _fh;
					SetFocus();
					_save = true;
					auto sp = min(_content.sp, _content.cp);
					auto ep = max(_content.sp, _content.cp);
					EditorCoord pos;
					if (y < 0) {
						pos = EditorCoord();
					} else if (y >= _content.lines.Length()) {
						pos = EditorCoord(_content.lines.LastElement().text.Length(), _content.lines.Length() - 1);
					} else if (_text_info.ElementAt(y)) {
						pos = EditorCoord(_text_info[y].TestPosition(at.x - Border + _hscroll->Position), y);
					} else {
						pos = EditorCoord();
					}
					if (sp > pos || ep < pos) _content.cp = _content.sp = pos;
					ScrollToCaret();
				}
			}
			void MultiLineEdit::RightButtonUp(Point at)
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
					if (cut) cut->Disabled = ReadOnly || _content.cp == _content.sp;
					if (copy) copy->Disabled = _content.cp == _content.sp;
					if (paste) paste->Disabled = ReadOnly || !Clipboard::IsFormatAvailable(Clipboard::Format::Text);
					if (remove) remove->Disabled = ReadOnly || _content.cp == _content.sp;
					_state = 2;
					_menu->RunPopup(this, pos);
				}
			}
			void MultiLineEdit::MouseMove(Point at)
			{
				if (_state == 1) {
					int y = (at.y - Border + _vscroll->Position) / _fh;
					if (y < 0) {
						_content.cp = EditorCoord();
					} else if (y >= _content.lines.Length()) {
						_content.cp = EditorCoord(_content.lines.LastElement().text.Length(), _content.lines.Length() - 1);
					} else if (_text_info.ElementAt(y)) {
						_content.cp = EditorCoord(_text_info[y].TestPosition(at.x - Border + _hscroll->Position), y);
					}
					ScrollToCaret();
				}
			}
			void MultiLineEdit::ScrollVertically(double delta) { _vscroll->SetScrollerPositionSilent(_vscroll->Position + int(delta * double(_vscroll->Line))); }
			void MultiLineEdit::ScrollHorizontally(double delta) { _hscroll->SetScrollerPositionSilent(_hscroll->Position + int(delta * double(_hscroll->Line))); }
			void MultiLineEdit::KeyDown(int key_code)
			{
				if (key_code == KeyCodes::Back && !ReadOnly && (_content.cp != _content.sp || _content.cp > EditorCoord())) {
					if (_save) {
						_undo.PushCurrentVersion();
						_save = false;
					}
					if (_content.cp == _content.sp) {
						if (_content.sp.x > 0) {
							_content.cp = EditorCoord(_content.sp.x - 1, _content.sp.y);
						} else if (_content.sp.y > 0) {
							_content.cp = EditorCoord(_content.lines[_content.sp.y - 1].text.Length(), _content.sp.y - 1);
						}
					}
					Print(L"");
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Delete && !ReadOnly &&
					(_content.cp != _content.sp || _content.cp < EditorCoord(_content.lines.LastElement().text.Length(), _content.lines.Length() - 1))) {
					if (_save) {
						_undo.PushCurrentVersion();
						_save = false;
					}
					if (_content.cp == _content.sp) {
						if (_content.sp.x < _content.lines[_content.sp.y].text.Length()) {
							_content.cp = EditorCoord(_content.sp.x + 1, _content.sp.y);
						} else if (_content.sp.y < _content.lines.Length() - 1) {
							_content.cp = EditorCoord(0, _content.sp.y + 1);
						}
					}
					Print(L"");
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Left && _content.cp > EditorCoord()) {
					_save = true;
					if (_content.cp.x > 0) {
						_content.cp = EditorCoord(_content.cp.x - 1, _content.cp.y);
					} else if (_content.cp.y > 0) {
						_content.cp = EditorCoord(_content.lines[_content.cp.y - 1].text.Length(), _content.cp.y - 1);
					}
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Right && _content.cp < EditorCoord(_content.lines.LastElement().text.Length(), _content.lines.Length() - 1)) {
					_save = true;
					if (_content.cp.x < _content.lines[_content.cp.y].text.Length()) {
						_content.cp = EditorCoord(_content.cp.x + 1, _content.cp.y);
					} else if (_content.cp.y < _content.lines.Length() - 1) {
						_content.cp = EditorCoord(0, _content.cp.y + 1);
					}
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Escape) {
					_save = true;
					_content.sp = _content.cp;
				} else if (key_code == KeyCodes::Home) {
					_save = true;
					_content.cp.x = 0;
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::End) {
					_save = true;
					_content.cp.x = _content.lines[_content.cp.y].text.Length();
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Up) {
					_save = true;
					int sx = (_content.cp.x > 0) ? _text_info[_content.cp.y].EndOfChar(_content.cp.x - 1) : 0;
					_content.cp.y = max(_content.cp.y - 1, 0);
					_content.cp.x = _text_info[_content.cp.y].TestPosition(sx);
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Down) {
					_save = true;
					int sx = (_content.cp.x > 0) ? _text_info[_content.cp.y].EndOfChar(_content.cp.x - 1) : 0;
					_content.cp.y = min(_content.cp.y + 1, _content.lines.Length() - 1);
					_content.cp.x = _text_info[_content.cp.y].TestPosition(sx);
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::PageUp) {
					_save = true;
					int sx = (_content.cp.x > 0) ? _text_info[_content.cp.y].EndOfChar(_content.cp.x - 1) : 0;
					int dy = _vscroll->Page / _fh;
					_content.cp.y = max(_content.cp.y - dy, 0);
					_content.cp.x = _text_info[_content.cp.y].TestPosition(sx);
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::PageDown) {
					_save = true;
					int sx = (_content.cp.x > 0) ? _text_info[_content.cp.y].EndOfChar(_content.cp.x - 1) : 0;
					int dy = _vscroll->Page / _fh;
					_content.cp.y = min(_content.cp.y + dy, _content.lines.Length() - 1);
					_content.cp.x = _text_info[_content.cp.y].TestPosition(sx);
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _content.sp = _content.cp;
					_deferred_scroll = true;
				} else if (key_code == KeyCodes::Return) {
					_save = true;
					CharDown(L'\n');
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
			void MultiLineEdit::CharDown(uint32 ucs_code)
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
			void MultiLineEdit::PopupMenuCancelled(void) { if (_state == 2) _state = 0; }
			void MultiLineEdit::SetCursor(Point at)
			{
				SystemCursor cursor = SystemCursor::Arrow;
				if ((at.x >= Border && at.y >= Border &&
					at.x < WindowPosition.Right - WindowPosition.Left - Border - VerticalScrollSize &&
					at.y < WindowPosition.Bottom - WindowPosition.Top - Border - HorizontalScrollSize) || _state) cursor = SystemCursor::Beam;
				GetStation()->SetCursor(GetStation()->GetSystemCursor(cursor));
			}
			Window::RefreshPeriod MultiLineEdit::FocusedRefreshPeriod(void) { return RefreshPeriod::CaretBlink; }
			void MultiLineEdit::Undo(void)
			{
				if (!ReadOnly && _undo.CanUndo()) {
					_undo.Undo();
					_save = true;
					_deferred_scroll = true;
					_deferred_update = true;
					for (int i = 0; i < _text_info.Length(); i++) _text_info.SetElement(0, i);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			void MultiLineEdit::Redo(void)
			{
				if (!ReadOnly && _undo.CanRedo()) {
					_undo.Redo();
					_save = true;
					_deferred_scroll = true;
					_deferred_update = true;
					for (int i = 0; i < _text_info.Length(); i++) _text_info.SetElement(0, i);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			void MultiLineEdit::Cut(void)
			{
				if (!ReadOnly && _content.cp != _content.sp) {
					_undo.PushCurrentVersion();
					_save = true;
					Clipboard::SetData(GetSelection());
					Print(L"");
					_deferred_scroll = true;
				}
			}
			void MultiLineEdit::Copy(void)
			{
				if (_content.cp != _content.sp) {
					_save = true;
					Clipboard::SetData(GetSelection());
				}
			}
			void MultiLineEdit::Paste(void)
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
			void MultiLineEdit::Delete(void)
			{
				if (!ReadOnly && _content.cp != _content.sp) {
					_undo.PushCurrentVersion();
					_save = true;
					Print(L"");
					_deferred_scroll = true;
				}
			}
			string MultiLineEdit::GetSelection(void)
			{
				auto sp = min(_content.sp, _content.cp);
				auto ep = max(_content.sp, _content.cp);
				DynamicString result;
				if (sp.y != ep.y) {
					for (int i = sp.y; i <= ep.y; i++) {
						if (i != sp.y) result += IO::NewLineChar;
						if (i == sp.y) {
							result += string(_content.lines[i].text.GetBuffer() + sp.x, _content.lines[i].text.Length() - sp.x, Encoding::UTF32);
						} else if (i == ep.y) {
							result += string(_content.lines[i].text.GetBuffer(), ep.x, Encoding::UTF32);
						} else {
							result += string(_content.lines[i].text.GetBuffer(), _content.lines[i].text.Length(), Encoding::UTF32);
						}
					}
				} else {
					result += string(_content.lines[ep.y].text.GetBuffer() + sp.x, ep.x - sp.x, Encoding::UTF32);
				}
				return result.ToString();
			}
			void MultiLineEdit::SetSelection(Point selection_position, Point caret_position)
			{
				_content.sp.y = max(min(selection_position.y, _content.lines.Length() - 1), 0);
				_content.cp.y = max(min(caret_position.y, _content.lines.Length() - 1), 0);
				_content.sp.x = max(min(selection_position.x, _content.lines[_content.sp.y].text.Length()), 0);
				_content.cp.x = max(min(caret_position.x, _content.lines[_content.cp.y].text.Length()), 0);
			}
			Point MultiLineEdit::GetCaretPosition(void) { return Point(_content.cp.x, _content.cp.y); }
			Point MultiLineEdit::GetSelectionPosition(void) { return Point(_content.sp.x, _content.sp.y); }
			void MultiLineEdit::ScrollToCaret(void)
			{
				if (_text_info.ElementAt(_content.cp.y)) {
					int caret_x = (_content.cp.x > 0) ? _text_info[_content.cp.y].EndOfChar(_content.cp.x - 1) : 0;
					int caret_y = _content.cp.y * _fh;
					if (caret_x < _hscroll->Position) _hscroll->SetScrollerPositionSilent(caret_x);
					else if (caret_x + _caret_width >= _hscroll->Position + _hscroll->Page) _hscroll->SetScrollerPositionSilent(caret_x + _caret_width - _hscroll->Page);
					if (caret_y < _vscroll->Position) _vscroll->SetScrollerPositionSilent(caret_y);
					else if (caret_y + _fh >= _vscroll->Position + _vscroll->Page) _vscroll->SetScrollerPositionSilent(caret_y + _fh - _vscroll->Page);
				}
			}
			void MultiLineEdit::SetCharacterFilter(const string & filter)
			{
				CharactersEnabled = filter;
				_chars_enabled.SetLength(filter.GetEncodedLength(Encoding::UTF32));
				filter.Encode(_chars_enabled.GetBuffer(), Encoding::UTF32, false);
			}
			string MultiLineEdit::GetCharacterFilter(void) { return CharactersEnabled; }
			void MultiLineEdit::SetContextMenu(Menues::Menu * menu) { _menu.SetRetain(menu); }
			Menues::Menu * MultiLineEdit::GetContextMenu(void) { return _menu; }
			void MultiLineEdit::SetHook(IMultiLineEditHook * hook) { _hook = hook; for (int i = 0; i < _text_info.Length(); i++) _text_info.SetElement(0, i); }
			MultiLineEdit::IMultiLineEditHook * MultiLineEdit::GetHook(void) { return _hook; }
			const Array<uint32>& MultiLineEdit::GetRawLine(int line_index) { return _content.lines[line_index].text; }
			int MultiLineEdit::GetLineCount(void) { return _content.lines.Length(); }
			void MultiLineEdit::InvalidateLine(int line_index) { _text_info.SetElement(0, line_index); }
			uint64 MultiLineEdit::GetUserDataForRawLine(int line_index) { return _content.lines[line_index].user_data; }
			void MultiLineEdit::SetUserDataForRawLine(int line_index, uint64 value) { _content.lines[line_index].user_data = value; }
			string MultiLineEdit::FilterInput(const string & input)
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
				if (!conv.Length()) return conv;
				if (_hook) conv = _hook->Filter(this, conv, Point(_content.cp.x, _content.cp.y));
				return conv;
			}
			void MultiLineEdit::Print(const string & text)
			{
				Array<uint32> utf32(0x100);
				utf32.SetLength(text.GetEncodedLength(Encoding::UTF32));
				text.Encode(utf32.GetBuffer(), Encoding::UTF32, false);
				if (_content.cp != _content.sp) {
					auto sp = min(_content.sp, _content.cp);
					auto ep = max(_content.sp, _content.cp);
					if (sp.y == ep.y) {
						int dl = ep.x - sp.x;
						for (int i = ep.x; i < _content.lines[sp.y].text.Length(); i++)
							_content.lines[sp.y].text[i - dl] = _content.lines[sp.y].text[i];
						_content.lines[sp.y].text.SetLength(_content.lines[sp.y].text.Length() - dl);
						_content.lines[sp.y].user_data = 0;
						_content.cp = _content.sp = sp;
						_text_info.SetElement(0, sp.y);
					} else {
						for (int i = ep.y - 1; i > sp.y; i--) {
							_content.lines.Remove(i);
							_text_info.Remove(i);
						}
						ep.y = sp.y + 1;
						_content.lines[sp.y].text.SetLength(sp.x);
						_content.lines[sp.y].user_data = 0;
						_text_info.SetElement(0, sp.y);
						int dl = ep.x;
						for (int i = ep.x; i < _content.lines[ep.y].text.Length(); i++)
							_content.lines[ep.y].text[i - dl] = _content.lines[ep.y].text[i];
						_content.lines[ep.y].text.SetLength(_content.lines[ep.y].text.Length() - dl);
						_content.cp = _content.sp = sp;
						_text_info.SetElement(0, ep.y);
						if (ep.y < _content.lines.Length() - 1 && _hook) _text_info.SetElement(0, ep.y + 1);
						_content.lines[sp.y].text.Append(_content.lines[ep.y].text);
						_content.lines.Remove(ep.y);
						_text_info.Remove(ep.y);
						_content.cp = _content.sp = sp;
					}
				}
				for (int i = 0; i < utf32.Length(); i++) {
					if (utf32[i] >= 0x20 || utf32[i] == L'\t') {
						_content.lines[_content.cp.y].text.Insert(utf32[i], _content.cp.x);
						_content.lines[_content.cp.y].user_data = 0;
						_text_info.SetElement(0, _content.cp.y);
						_content.cp.x++;
						_content.sp.x++;
					} else if (utf32[i] == L'\n') {
						EditorLine ins;
						ins.user_data = 0;
						ins.width = 0;
						ins.text.Append(
							_content.lines[_content.cp.y].text.GetBuffer() + _content.cp.x,
							_content.lines[_content.cp.y].text.Length() - _content.cp.x);
						_content.lines[_content.cp.y].text.SetLength(_content.cp.x);
						_content.lines[_content.cp.y].user_data = 0;
						_content.lines.Insert(ins, _content.cp.y + 1);
						_text_info.SetElement(0, _content.cp.y);
						_text_info.Insert(0, _content.cp.y + 1);
						_content.cp.y++;
						_content.sp.y++;
						_content.cp.x = 0;
						_content.sp.x = 0;
					}
				}
				_deferred_update = true;
				GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}

			string MultiLineEdit::IMultiLineEditHook::Filter(MultiLineEdit * sender, const string & input, Point insert_at) { return input; }
			Array<uint8>* MultiLineEdit::IMultiLineEditHook::ColorHighlight(MultiLineEdit * sender, const Array<uint32>& text, int line) { return 0; }
			Array<UI::Color>* MultiLineEdit::IMultiLineEditHook::GetPalette(MultiLineEdit * sender) { return 0; }

			MultiLineEdit::EditorCoord::EditorCoord(void) {}
			MultiLineEdit::EditorCoord::EditorCoord(int sx, int sy) : x(sx), y(sy) {}

			bool operator==(const MultiLineEdit::EditorCoord & a, const MultiLineEdit::EditorCoord & b) { return a.x == b.x && a.y == b.y; }
			bool operator!=(const MultiLineEdit::EditorCoord & a, const MultiLineEdit::EditorCoord & b) { return a.x != b.x || a.y != b.y; }
			bool operator<(const MultiLineEdit::EditorCoord & a, const MultiLineEdit::EditorCoord & b) { return (a.y < b.y) || (a.y == b.y && a.x < b.x); }
			bool operator>(const MultiLineEdit::EditorCoord & a, const MultiLineEdit::EditorCoord & b) { return (a.y > b.y) || (a.y == b.y && a.x > b.x); }
			bool operator<=(const MultiLineEdit::EditorCoord & a, const MultiLineEdit::EditorCoord & b) { return (a.y < b.y) || (a.y == b.y && a.x <= b.x); }
			bool operator>=(const MultiLineEdit::EditorCoord & a, const MultiLineEdit::EditorCoord & b) { return (a.y > b.y) || (a.y == b.y && a.x >= b.x); }
			bool operator==(const MultiLineEdit::EditorLine & a, const MultiLineEdit::EditorLine & b) { return a.text == b.text; }
			bool operator!=(const MultiLineEdit::EditorLine & a, const MultiLineEdit::EditorLine & b) { return a.text != b.text; }
			bool operator==(const MultiLineEdit::EditorContent & a, const MultiLineEdit::EditorContent & b) { return a.cp == b.cp && a.sp == b.sp && a.lines == b.lines; }
			bool operator!=(const MultiLineEdit::EditorContent & a, const MultiLineEdit::EditorContent & b) { return a.cp != b.cp || a.sp != b.sp || a.lines != b.lines; }
		}
	}
}