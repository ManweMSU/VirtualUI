#include "CombinedControls.h"

#include "../Interfaces/KeyCodes.h"
#include "../Interfaces/Clipboard.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace ArgumentService
			{
				class ComboBoxFrameArgumentProvider : public IArgumentProvider
				{
				public:
					ComboBox * Owner;
					ComboBoxFrameArgumentProvider(ComboBox * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override { *value = L""; }
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font" && Owner->Font) {
							*value = Owner->Font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
				class ComboBoxSimpleArgumentProvider : public IArgumentProvider
				{
				public:
					ComboBox * Owner;
					const string & Text;
					ComboBoxSimpleArgumentProvider(ComboBox * owner, const string & text) : Owner(owner), Text(text) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Text;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font" && Owner->Font) {
							*value = Owner->Font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
				class ComboBoxWrapperArgumentProvider : public IArgumentProvider
				{
				public:
					ComboBox * Owner;
					IArgumentProvider * Inner;
					ComboBoxWrapperArgumentProvider(ComboBox * owner, IArgumentProvider * inner) : Owner(owner), Inner(inner) {}
					virtual void GetArgument(const string & name, int * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, double * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, Color * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, string * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font") {
							*value = Owner->Font;
							if (Owner->Font) (*value)->Retain();
						} else Inner->GetArgument(name, value);
					}
				};
				class TextComboBoxFrameArgumentProvider : public IArgumentProvider
				{
				public:
					TextComboBox * Owner;
					TextComboBoxFrameArgumentProvider(TextComboBox * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override { *value = L""; }
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font" && Owner->Font) {
							*value = Owner->Font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
				class TextComboBoxElementArgumentProvider : public IArgumentProvider
				{
				public:
					TextComboBox * Owner;
					const string & Text;
					TextComboBoxElementArgumentProvider(TextComboBox * owner, const string & text) : Owner(owner), Text(text) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Text;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font" && Owner->Font) {
							*value = Owner->Font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
			}
			void ComboBox::invalidate_viewport(void) { _viewport_element.SetReference(0); }
			void ComboBox::run_drop_down(void)
			{
				if (GetCapture() == this) ReleaseCapture();
				for (int i = 0; i < _elements.Length(); i++) {
					if (_elements[i].ViewNormal) _elements[i].ViewNormal->ClearCache();
					if (_elements[i].ViewDisabled) _elements[i].ViewDisabled->ClearCache();
				}
				auto margins = GetPopupMargins(this);
				int uec = (margins.Top - (Border << 1)) / ElementHeight;
				int dec = (margins.Bottom - (Border << 1)) / ElementHeight;
				bool su = false;
				if (dec < _elements.Length() && uec >= _elements.Length()) su = true;
				if (dec < _elements.Length() && uec < _elements.Length()) {
					if (dec <= uec - 10) su = true;
				}
				int y, h, e;
				if (su) {
					e = min(uec, _elements.Length());
					h = e * ElementHeight + (Border << 1);
					y = -h;
				} else {
					e = min(dec, _elements.Length());
					h = e * ElementHeight + (Border << 1);
					y = ControlBoundaries.Bottom - ControlBoundaries.Top;
				}
				auto tlw = CreatePopup(this, Box(0, y, ControlBoundaries.Right - ControlBoundaries.Left, y + h));
				if (tlw) {
					SafePointer<ComboListBox> list = new ComboListBox(this);
					_list = list.Inner();
					tlw->AddChild(list);
					tlw->ArrangeChildren();
					ShowPopup(tlw, true);
					tlw->GetControlSystem()->SetExclusiveControl(list);
				} else _state = 0;
			}
			ComboBox::ComboBox(void) : _elements(0x10)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
			}
			ComboBox::ComboBox(Template::ControlTemplate * Template) : _elements(0x10)
			{
				if (Template->Properties->GetTemplateClass() != L"ComboBox") throw InvalidArgumentException();
				static_cast<Template::Controls::ComboBox &>(*this) = static_cast<Template::Controls::ComboBox &>(*Template->Properties);
			}
			ComboBox::~ComboBox(void) { if (_list) { _list->_owner = 0; _list->GetControlSystem()->SetExclusiveControl(0); } }
			void ComboBox::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				Shape ** back = 0;
				Template::Shape * temp = 0;
				Box element(ItemPosition, at);
				if (!_viewport_element) {
					if (_current >= 0) {
						if (Disabled) {
							if (_elements[_current].ViewDisabled) _viewport_element.SetReference(_elements[_current].ViewDisabled->Clone());
						} else {
							if (_elements[_current].ViewNormal) _viewport_element.SetReference(_elements[_current].ViewNormal->Clone());
						}
					} else {
						ArgumentService::ComboBoxFrameArgumentProvider provider(this);
						if (Disabled) {
							if (ViewElementDisabledPlaceholder) _viewport_element.SetReference(ViewElementDisabledPlaceholder->Initialize(&provider));
						} else {
							if (ViewElementNormalPlaceholder) _viewport_element.SetReference(ViewElementNormalPlaceholder->Initialize(&provider));
						}
					}
				}
				if (Disabled) {
					back = _view_disabled.InnerRef();
					temp = ViewDisabled.Inner();
				} else {
					if ((_state & 0xF) == 2) {
						back = _view_pressed.InnerRef();
						temp = ViewPressed.Inner();
					} else if ((_state & 0xF) == 1) {
						if (GetFocus() == this) {
							back = _view_hot_focused.InnerRef();
							temp = ViewHotFocused.Inner();
						} else {
							back = _view_hot.InnerRef();
							temp = ViewHot.Inner();
						}
					} else {
						if (GetFocus() == this) {
							back = _view_focused.InnerRef();
							temp = ViewFocused.Inner();
						} else {
							back = _view_normal.InnerRef();
							temp = ViewNormal.Inner();
						}
					}
				}
				if (*back) (*back)->Render(device, at);
				else if (temp) {
					ArgumentService::ComboBoxFrameArgumentProvider provider(this);
					*back = temp->Initialize(&provider);
					(*back)->Render(device, at);
				}
				if (_viewport_element) _viewport_element->Render(device, element);
			}
			void ComboBox::ResetCache(void)
			{
				invalidate_viewport();
				_view_normal.SetReference(0);
				_view_disabled.SetReference(0);
				_view_focused.SetReference(0);
				 _view_hot.SetReference(0);
				_view_hot_focused.SetReference(0);
				_view_pressed.SetReference(0);
			}
			void ComboBox::Enable(bool enable) { Disabled = !enable; invalidate_viewport(); if (!enable) _state = 0; Invalidate(); }
			bool ComboBox::IsEnabled(void) { return !Disabled; }
			void ComboBox::Show(bool visible) { Invisible = !visible; if (!visible) _state = 0; Invalidate(); }
			bool ComboBox::IsVisible(void) { return !Invisible; }
			bool ComboBox::IsTabStop(void) { return true; }
			void ComboBox::SetID(int _ID) { ID = _ID; }
			int ComboBox::GetID(void) { return ID; }
			Control * ComboBox::FindChild(int _ID) { if (ID && ID == _ID) return this; else return 0; }
			void ComboBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle ComboBox::GetRectangle(void) { return ControlPosition; }
			void ComboBox::RaiseEvent(int _ID, ControlEvent event, Control * sender) { if (_ID == 1 && event == ControlEvent::Deferred) GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this); }
			void ComboBox::FocusChanged(bool got_focus) { if (!got_focus && _list && _list->GetFocus() != _list) _list->GetControlSystem()->SetExclusiveControl(0); Invalidate(); }
			void ComboBox::CaptureChanged(bool got_capture) { if (!got_capture && (_state & 0xF) == 1) { _state = 0; Invalidate(); } }
			void ComboBox::LeftButtonDown(Point at)
			{
				SetFocus();
				if ((_state == 1 || _state == 0 || _state & 0x10) && !_list) {
					_state = 2;
					run_drop_down();
					Invalidate();
				}
			}
			void ComboBox::MouseMove(Point at)
			{
				if (_state != 1 && _state != 2) {
					_state = 1;
					SetCapture();
					Invalidate();
				} else if (!IsHovered()) ReleaseCapture();
			}
			bool ComboBox::KeyDown(int key_code)
			{
				if (_state == 2) {
					_list->KeyDown(key_code);
				} else {
					if (key_code == KeyCodes::Space) {
						_state = 2;
						run_drop_down();
						Invalidate();
					} else if (key_code == KeyCodes::Up) {
						if (_current > 0) {
							_current--;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						} else if (_current == -1 && _elements.Length()) {
							_current = 0;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					} else if (key_code == KeyCodes::Down) {
						if (_current == -1 && _elements.Length()) {
							_current = 0;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						} else if (_current != -1 && _current < _elements.Length() - 1) {
							_current++;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					} else if (key_code == KeyCodes::PageUp) {
						if (_current > 0) {
							_current--;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						} else if (_current == -1 && _elements.Length()) {
							_current = 0;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					} else if (key_code == KeyCodes::PageDown) {
						if (_current == -1 && _elements.Length()) {
							_current = 0;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						} else if (_current != -1 && _current < _elements.Length() - 1) {
							_current++;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					} else if (key_code == KeyCodes::Home) {
						if (_elements.Length() && _current) {
							_current = 0;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					} else if (key_code == KeyCodes::End) {
						if (_elements.Length() && _current != _elements.Length() - 1) {
							_current = _elements.Length() - 1;
							invalidate_viewport();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					}
				}
				return true;
			}
			string ComboBox::GetControlClass(void) { return L"ComboBox"; }
			void ComboBox::AddItem(const string & text, void * user) { InsertItem(text, _elements.Length(), user); }
			void ComboBox::AddItem(IArgumentProvider * provider, void * user) { InsertItem(provider, _elements.Length(), user); }
			void ComboBox::AddItem(Reflection::Reflected & object, void * user) { InsertItem(object, _elements.Length(), user); }
			void ComboBox::InsertItem(const string & text, int at, void * user)
			{
				ArgumentService::ComboBoxSimpleArgumentProvider provider(this, text);
				_elements.Insert(Element(), at);
				auto & e = _elements[at];
				e.User = user;
				e.ViewNormal.SetReference(ViewElementNormal->Initialize(&provider));
				e.ViewDisabled.SetReference(ViewElementDisabled->Initialize(&provider));
				if (_current >= at) _current++;
			}
			void ComboBox::InsertItem(IArgumentProvider * provider, int at, void * user)
			{
				ArgumentService::ComboBoxWrapperArgumentProvider wrapper(this, provider);
				_elements.Insert(Element(), at);
				auto & e = _elements[at];
				e.User = user;
				e.ViewNormal.SetReference(ViewElementNormal->Initialize(&wrapper));
				e.ViewDisabled.SetReference(ViewElementDisabled->Initialize(&wrapper));
				if (_current >= at) _current++;
			}
			void ComboBox::InsertItem(Reflection::Reflected & object, int at, void * user)
			{
				ReflectorArgumentProvider provider(&object);
				InsertItem(&provider, at, user);
			}
			void ComboBox::ResetItem(int index, const string & text)
			{
				ArgumentService::ComboBoxSimpleArgumentProvider provider(this, text);
				_elements[index].ViewNormal.SetReference(ViewElementNormal->Initialize(&provider));
				_elements[index].ViewDisabled.SetReference(ViewElementDisabled->Initialize(&provider));
				if (index == _current) Invalidate();
			}
			void ComboBox::ResetItem(int index, IArgumentProvider * provider)
			{
				ArgumentService::ComboBoxWrapperArgumentProvider wrapper(this, provider);
				_elements[index].ViewNormal.SetReference(ViewElementNormal->Initialize(&wrapper));
				_elements[index].ViewDisabled.SetReference(ViewElementDisabled->Initialize(&wrapper));
				if (index == _current) Invalidate();
			}
			void ComboBox::ResetItem(int index, Reflection::Reflected & object)
			{
				ReflectorArgumentProvider provider(&object);
				ResetItem(index, &provider);
			}
			void ComboBox::SwapItems(int i, int j)
			{
				if (_current == i) _current = j;
				else if (_current == j) _current = i;
				_elements.SwapAt(i, j);
			}
			void ComboBox::RemoveItem(int index)
			{
				_elements.Remove(index);
				if (_current == index) { _current = -1; invalidate_viewport(); Invalidate(); }
				else if (_current > index) _current--;
			}
			void ComboBox::ClearItems(void) { _elements.Clear(); _current = -1; invalidate_viewport(); Invalidate(); }
			int ComboBox::ItemCount(void) { return _elements.Length(); }
			void * ComboBox::GetItemUserData(int index) { return _elements[index].User; }
			void ComboBox::SetItemUserData(int index, void * user) { _elements[index].User = user; }
			int ComboBox::GetSelectedIndex(void) { return _current; }
			void ComboBox::SetSelectedIndex(int index) { _current = index; invalidate_viewport(); Invalidate(); }

			void ComboBox::ComboListBox::scroll_to_current(void)
			{
				if (!_owner) return;
				if (_owner->_current >= 0) {
					int min = _owner->ElementHeight * _owner->_current;
					int max = min + _owner->ElementHeight;
					if (_scroll->Position > min) _scroll->SetScrollerPosition(min);
					if (_scroll->Position + _scroll->Page < max) _scroll->SetScrollerPosition(max - _scroll->Page);
				}
			}
			void ComboBox::ComboListBox::move_selection(int to)
			{
				if (!_owner) return;
				int index = max(min(to, _owner->_elements.Length() - 1), 0);
				if (!_owner->_elements.Length()) return;
				if (_owner->_current != index) {
					_owner->_current = index;
					_hot = index;
					scroll_to_current();
					_owner->SubmitEvent(1);
					_owner->invalidate_viewport();
					_owner->Invalidate();
					Invalidate();
				}
			}
			ComboBox::ComboListBox::ComboListBox(ComboBox * Base) : _owner(Base)
			{
				_hot = Base->_current;
				_position = Rectangle::Entire();
				SafePointer<VerticalScrollBar> bar = new VerticalScrollBar(Base);
				_scroll = bar.Inner();
				AddChild(bar);
				_scroll->ControlPosition = Rectangle(Coordinate::Right() - Base->Border - Base->ScrollSize, Base->Border, Coordinate::Right() - Base->Border, Coordinate::Bottom() - Base->Border);
				_scroll->Line = Base->ElementHeight;
				_scroll->Invisible = true;
				int space = Base->_elements.Length() * Base->ElementHeight;
				_scroll->SetRangeSilent(0, space - 1);
			}
			ComboBox::ComboListBox::~ComboListBox(void) { if (_owner) { _owner->_list = 0; _owner->_state = 0; _owner->Invalidate(); _owner->SetFocus(); } }
			void ComboBox::ComboListBox::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				if (!_owner) return;
				if (!_view) {
					if (_owner->ViewDropDownList) {
						ZeroArgumentProvider provider;
						_view.SetReference(_owner->ViewDropDownList->Initialize(&provider));
						_view->Render(device, at);
					}
				} _view->Render(device, at);
				if (!_view_element_hot && _owner->ViewElementHot) {
					ZeroArgumentProvider provider;
					_view_element_hot.SetReference(_owner->ViewElementHot->Initialize(&provider));
				}
				if (!_view_element_selected && _owner->ViewElementSelected) {
					ZeroArgumentProvider provider;
					_view_element_selected.SetReference(_owner->ViewElementSelected->Initialize(&provider));
				}
				Box viewport = Box(at.Left + _owner->Border, at.Top + _owner->Border,
					at.Right - _owner->Border - (_svisible ? _owner->ScrollSize : 0), at.Bottom - _owner->Border);
				device->PushClip(viewport);
				if (_owner->_elements.Length()) {
					int from = max(min(_scroll->Position / _owner->ElementHeight, _owner->_elements.Length() - 1), 0);
					int to = max(min((_scroll->Position + _scroll->Page) / _owner->ElementHeight, _owner->_elements.Length() - 1), 0);
					for (int i = from; i <= to; i++) {
						Box item(viewport.Left, viewport.Top + i * _owner->ElementHeight - _scroll->Position, viewport.Right, 0);
						item.Bottom = item.Top + _owner->ElementHeight;
						if (i == _hot && _view_element_hot) {
							_view_element_hot->Render(device, item);
						} else if (i == _owner->_current && _view_element_selected) {
							_view_element_selected->Render(device, item);
						}
						_owner->_elements[i].ViewNormal->Render(device, item);
					}
				}
				device->PopClip();
				ParentControl::Render(device, at);
			}
			void ComboBox::ComboListBox::ResetCache(void) { if (GetControlSystem()) GetControlSystem()->SetExclusiveControl(0); }
			void ComboBox::ComboListBox::SetRectangle(const Rectangle & rect) { _position = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle ComboBox::ComboListBox::GetRectangle(void) { return _position; }
			void ComboBox::ComboListBox::SetPosition(const Box & box)
			{
				ControlBoundaries = box; ArrangeChildren();
				if (!_owner) return;
				int page = box.Bottom - box.Top - (_owner->Border << 1);
				_scroll->SetPageSilent(page);
				_scroll->SetScrollerPositionSilent(_owner->_current * _owner->ElementHeight - (page >> 1));
				if (page < _owner->_elements.Length() * _owner->ElementHeight) _scroll->Show(_svisible = true);
			}
			void ComboBox::ComboListBox::CaptureChanged(bool got_capture) { if (!got_capture) { _hot = -1; Invalidate(); } }
			void ComboBox::ComboListBox::LostExclusiveMode(void) { if (_owner) _owner->SetFocus(); DestroyPopup(GetParent()); }
			void ComboBox::ComboListBox::LeftButtonUp(Point at)
			{
				if (!_owner) return;
				if (_hot != -1) {
					if (_hot != _owner->_current) {
						_owner->_current = _hot;
						_owner->SubmitEvent(1);
						_owner->invalidate_viewport();
						_owner->Invalidate();
					}
				}
				GetControlSystem()->SetExclusiveControl(0);
			}
			void ComboBox::ComboListBox::MouseMove(Point at)
			{
				if (!_owner) return;
				Box element(_owner->Border, _owner->Border, ControlBoundaries.Right - ControlBoundaries.Left - _owner->Border - (_svisible ? _owner->ScrollSize : 0), ControlBoundaries.Bottom - ControlBoundaries.Top - _owner->Border);
				int oh = _hot;
				if (element.IsInside(at) && IsHovered()) {
					int index = (at.y - _owner->Border + _scroll->Position) / _owner->ElementHeight;
					if (index < 0 || index >= _owner->_elements.Length()) index = -1;
					_hot = index;
				} else {
					_hot = -1;
				}
				if (oh != _hot) Invalidate();
				if (oh == -1 && _hot != -1) SetCapture();
				else if ((oh != -1 || GetCapture() == this) && _hot == -1) ReleaseCapture();
			}
			void ComboBox::ComboListBox::ScrollVertically(double delta)
			{
				if (!_owner) return;
				if (_svisible) {
					_scroll->SetScrollerPosition(_scroll->Position + int(delta * double(_scroll->Line)));
				}
			}
			bool ComboBox::ComboListBox::KeyDown(int key_code)
			{
				if (!_owner) return false;
				int page = max((ControlBoundaries.Bottom - ControlBoundaries.Top - _owner->Border - _owner->Border) / _owner->ElementHeight, 1);
				if (key_code == KeyCodes::Space) {
					GetControlSystem()->SetExclusiveControl(0);
				} else if (key_code == KeyCodes::Down) {
					if (_owner->_current != -1) move_selection(_owner->_current + 1); else move_selection(0);
				} else if (key_code == KeyCodes::Up) {
					if (_owner->_current != -1) move_selection(_owner->_current - 1); else move_selection(0);
				} else if (key_code == KeyCodes::End) {
					move_selection(_owner->_elements.Length() - 1);
				} else if (key_code == KeyCodes::Home) {
					move_selection(0);
				} else if (key_code == KeyCodes::PageDown) {
					if (_owner->_current != -1) move_selection(_owner->_current + page); else move_selection(0);
				} else if (key_code == KeyCodes::PageUp) {
					if (_owner->_current != -1) move_selection(_owner->_current - page); else move_selection(0);
				}
				return true;
			}
			Control * ComboBox::ComboListBox::HitTest(Point at)
			{
				if (!IsEnabled()) return this;
				if (_scroll && _svisible) {
					auto box = _scroll->GetPosition();
					if (box.IsInside(at)) return _scroll->HitTest(Point(at.x - box.Left, at.y - box.Top));
				}
				return this;
			}
			string ComboBox::ComboListBox::GetControlClass(void) { return L"ComboListBox"; }

			void TextComboBox::find_advice(void)
			{
				int advice = -1;
				if (_cp == _text.Length() && _cp && _cp == _sp) {
					string text = GetText();
					for (int i = 0; i < _elements.Length(); i++) {
						if (_elements[i].Length() > text.Length() && string::CompareIgnoreCase(text, _elements[i].Fragment(0, text.Length())) == 0) { advice = i; break; }
					}
				}
				if (advice != _advice) {
					Invalidate();
					_advice = advice;
					_advice_info.SetReference(0);
				}
			}
			void TextComboBox::run_drop_down(void)
			{
				if (GetCapture() == this) ReleaseCapture();
				_save = true;
				auto margins = GetPopupMargins(this);
				int uec = (margins.Top - (Border << 1)) / ElementHeight;
				int dec = (margins.Bottom - (Border << 1)) / ElementHeight;
				bool su = false;
				if (dec < _elements.Length() && uec >= _elements.Length()) su = true;
				if (dec < _elements.Length() && uec < _elements.Length()) {
					if (dec <= uec - 10) su = true;
				}
				int y, h, e;
				if (su) {
					e = min(uec, _elements.Length());
					h = e * ElementHeight + (Border << 1);
					y = -h;
				} else {
					e = min(dec, _elements.Length());
					h = e * ElementHeight + (Border << 1);
					y = ControlBoundaries.Bottom - ControlBoundaries.Top;
				}
				auto tlw = CreatePopup(this, Box(0, y, ControlBoundaries.Right - ControlBoundaries.Left, y + h));
				if (tlw) {
					SafePointer<TextComboListBox> list = new TextComboListBox(this);
					_list = list.Inner();
					tlw->AddChild(list);
					tlw->ArrangeChildren();
					ShowPopup(tlw, true);
					tlw->GetControlSystem()->SetExclusiveControl(list);
				} else _state = 0;
			}
			TextComboBox::TextComboBox(void) : _elements(0x10), _undo(_text), _text(0x100), _chars_enabled(0x100)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
			}
			TextComboBox::TextComboBox(Template::ControlTemplate * Template) : _elements(0x10), _undo(_text), _text(0x100), _chars_enabled(0x100)
			{
				if (Template->Properties->GetTemplateClass() != L"TextComboBox") throw InvalidArgumentException();
				static_cast<Template::Controls::TextComboBox &>(*this) = static_cast<Template::Controls::TextComboBox &>(*Template->Properties);
				Text = Text.NormalizedForm();
				_text.SetLength(Text.GetEncodedLength(Encoding::UTF32));
				Text.Encode(_text.GetBuffer(), Encoding::UTF32, false);
				_chars_enabled.SetLength(CharactersEnabled.GetEncodedLength(Encoding::UTF32));
				CharactersEnabled.Encode(_chars_enabled.GetBuffer(), Encoding::UTF32, false);
				if (ContextMenu) _menu.SetReference(CreateMenu(ContextMenu));
			}
			TextComboBox::~TextComboBox(void) { if (_list) { _list->_owner = 0; _list->GetControlSystem()->SetExclusiveControl(0); } }
			void TextComboBox::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				if (_caret_width < 0) _caret_width = CaretWidth ? CaretWidth : GetControlSystem()->GetCaretWidth();
				Shape ** back = 0;
				Template::Shape * source = 0;
				Engine::Color text_color;
				Engine::Color placeholder_color;
				bool focused = false;
				if (Disabled) {
					source = ViewDisabled;
					back = _view_frame_disabled.InnerRef();
					text_color = ColorDisabled;
					placeholder_color = PlaceholderColorDisabled;
				} else {
					text_color = Color;
					placeholder_color = PlaceholderColor;
					if (GetFocus() == this || _state == 2 || _state == 4) {
						focused = true;
						source = ViewFocused;
						back = _view_frame_focused.InnerRef();
					} else {
						source = ViewNormal;
						back = _view_frame_normal.InnerRef();
					}
				}
				if (!(*back) && source) {
					ArgumentService::TextComboBoxFrameArgumentProvider provider(this);
					*back = source->Initialize(&provider);
				}
				if (*back) (*back)->Render(device, at);
				Box field = Box(TextPosition, at);
				device->PushClip(field);
				int caret = 0;
				if (_text.Length()) {
					if (Font) {
						if (!_text_info) {
							_text_info.SetReference(device->CreateTextBrush(Font, _text.GetBuffer(), _text.Length(), 0, 1, text_color));
						}
						if (!_advice_info && _advice != -1) {
							string residue = _elements[_advice].Fragment(GetText().Length(), -1);
							_advice_info.SetReference(device->CreateTextBrush(Font, residue, 0, 1, AdviceColor));
						}
						if (_deferred_scroll) {
							ScrollToCaret();
							_deferred_scroll = false;
						}
						int sp = min(_sp, _cp);
						int ep = max(_sp, _cp);
						if (_text_info) {
							_text_info->SetHighlightColor(SelectionColor);
							if (sp != ep && focused) _text_info->HighlightText(sp, ep); else _text_info->HighlightText(-1, -1);
							Box text_box(field.Left + _shift, field.Top, field.Right, field.Bottom);
							if (_cp > 0) caret = _text_info->EndOfChar(_cp - 1);
							caret += _shift;
							device->Render(_text_info, text_box, false);
							if (_advice_info && focused) {
								int w, h;
								_text_info->GetExtents(w, h);
								text_box.Left += w;
								device->Render(_advice_info, text_box, false);
							}
						}
					}
				} else {
					if (!_placeholder_info && Placeholder.Length() && PlaceholderFont) {
						_placeholder_info.SetReference(device->CreateTextBrush(PlaceholderFont, Placeholder, 0, 1, placeholder_color));
					}
					if (_placeholder_info && !focused) device->Render(_placeholder_info, field, false);
				}
				if (focused) {
					if (!_inversion) {
						if (CaretColor.a) {
							_inversion.SetReference(device->CreateSolidColorBrush(CaretColor));
							_use_color_caret = true;
						} else {
							_inversion.SetReference(device->CreateInversionEffectBrush());
							_use_color_caret = false;
						}
					}
					Box caret_box = Box(field.Left + caret, field.Top, field.Left + caret + _caret_width, field.Bottom);
					if (_use_color_caret) { if (device->IsCaretVisible()) device->Render(static_cast<Graphics::IColorBrush *>(_inversion.Inner()), caret_box); }
					else device->Render(static_cast<Graphics::IInversionEffectBrush *>(_inversion.Inner()), caret_box, true);
				}
				device->PopClip();
				Shape ** button;
				Template::Shape * button_template;
				Box button_box = Box(ButtonPosition, at);
				if (Disabled) {
					button = _view_button_disabled.InnerRef();
					button_template = ViewButtonDisabled.Inner();
				} else {
					if (_state == 4) {
						button = _view_button_pressed.InnerRef();
						button_template = ViewButtonPressed.Inner();
					} else if (_state == 3) {
						button = _view_button_hot.InnerRef();
						button_template = ViewButtonHot.Inner();
					} else {
						button = _view_button_normal.InnerRef();
						button_template = ViewButtonNormal.Inner();
					}
				}
				if (!(*button) && button_template) {
					ArgumentService::TextComboBoxFrameArgumentProvider provider(this);
					*button = button_template->Initialize(&provider);
				}
				if (*button) (*button)->Render(device, button_box);
			}
			void TextComboBox::ResetCache(void)
			{
				_view_frame_normal.SetReference(0);
				_view_frame_focused.SetReference(0);
				_view_frame_disabled.SetReference(0);
				_view_button_normal.SetReference(0);
				_view_button_hot.SetReference(0);
				_view_button_pressed.SetReference(0);
				_view_button_disabled.SetReference(0);
				_text_info.SetReference(0);
				_advice_info.SetReference(0);
				_placeholder_info.SetReference(0);
				_inversion.SetReference(0);
			}
			void TextComboBox::Enable(bool enable)
			{
				Disabled = !enable;
				if (Disabled) { _state = 0; _shift = 0, _cp = 0, _sp = 0; _advice_info.SetReference(0); _advice = -1; }
				_text_info.SetReference(0);
				_placeholder_info.SetReference(0);
				Invalidate();
			}
			bool TextComboBox::IsEnabled(void) { return !Disabled; }
			void TextComboBox::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; Invalidate(); }
			bool TextComboBox::IsVisible(void) { return !Invisible; }
			bool TextComboBox::IsTabStop(void) { return true; }
			void TextComboBox::SetID(int _ID) { ID = _ID; }
			int TextComboBox::GetID(void) { return ID; }
			Control * TextComboBox::FindChild(int _ID) { if (ID && ID == _ID) return this; else return 0; }
			void TextComboBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle TextComboBox::GetRectangle(void) { return ControlPosition; }
			void TextComboBox::SetText(const string & text)
			{
				_undo.RemoveAllVersions();
				_save = true;
				Text = text.NormalizedForm();
				_text.SetLength(Text.GetEncodedLength(Encoding::UTF32));
				Text.Encode(_text.GetBuffer(), Encoding::UTF32, false);
				_text_info.SetReference(0);
				_advice_info.SetReference(0);
				_shift = 0; _cp = 0; _sp = 0; _advice = -1;
				Invalidate();
			}
			string TextComboBox::GetText(void) { return string(_text.GetBuffer(), _text.Length(), Encoding::UTF32); }
			void TextComboBox::RaiseEvent(int _ID, ControlEvent event, Control * sender)
			{
				if (_state == 2) { _state = 0; Invalidate(); }
				if (event == ControlEvent::MenuCommand) {
					if (_ID == 1001) Undo();
					else if (_ID == 1000) Redo();
					else if (_ID == 1002) Cut();
					else if (_ID == 1003) Copy();
					else if (_ID == 1004) Paste();
					else if (_ID == 1005) Delete();
					else GetParent()->RaiseEvent(_ID, ControlEvent::Command, this);
				} else if (event == ControlEvent::Deferred && _ID == 1) {
					GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
				} else GetParent()->RaiseEvent(_ID, event, sender);
			}
			void TextComboBox::FocusChanged(bool got_focus)
			{
				if (!got_focus) {
					if (_list && _list->GetFocus() != _list) {
						_list->GetControlSystem()->SetExclusiveControl(0);
					} else if (!_list) { _save = true; }
				}
				Invalidate();
			}
			void TextComboBox::CaptureChanged(bool got_capture) { if (!got_capture && _state != 4) { _state = 0; Invalidate(); } }
			void TextComboBox::LeftButtonDown(Point at)
			{
				SetFocus();
				Box my = Box(0, 0, ControlBoundaries.Right - ControlBoundaries.Left, ControlBoundaries.Bottom - ControlBoundaries.Top);
				Box text = Box(TextPosition, my);
				Box button = Box(ButtonPosition, my);
				if (text.IsInside(at)) {
					int ocp = _cp, osp = _sp;
					SetCapture();
					_state = 1;
					_save = true;
					int pos = 0;
					if (_text_info) pos = _text_info->TestPosition(at.x - _shift - text.Left);
					if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = pos;
					_cp = pos;
					ScrollToCaret();
					auto device = GetRenderingDevice();
					if (device) device->SetCaretReferenceTime(GetTimerValue());
					if (ocp != _cp || osp != _sp) find_advice();
					Invalidate();
				} else if (button.IsInside(at) && !_list) { _state = 4; Invalidate(); run_drop_down(); }
			}
			void TextComboBox::LeftButtonUp(Point at) { if (_state == 1) ReleaseCapture(); }
			void TextComboBox::LeftButtonDoubleClick(Point at)
			{
				_sp = _cp;
				int len = _text.Length();
				while (_sp > 0 && ((IsAlphabetical(_text[_sp - 1]) || (_text[_sp - 1] >= L'0' && _text[_sp - 1] <= L'9') || (_text[_sp - 1] == L'_')))) _sp--;
				while (_cp < len && ((IsAlphabetical(_text[_cp]) || (_text[_cp] >= L'0' && _text[_cp] <= L'9') || (_text[_cp] == L'_')))) _cp++;
				find_advice();
				Invalidate();
			}
			void TextComboBox::RightButtonDown(Point at)
			{
				SetFocus();
				Box my = Box(0, 0, ControlBoundaries.Right - ControlBoundaries.Left, ControlBoundaries.Bottom - ControlBoundaries.Top);
				Box text = Box(TextPosition, my);
				if (text.IsInside(at)) {
					_save = true;
					if (_text_info) {
						int sp = min(const_cast<const int &>(_sp), _cp);
						int ep = max(const_cast<const int &>(_sp), _cp);
						int pos = _text_info->TestPosition(at.x - text.Left - _shift);
						if (sp > pos || ep < pos) _cp = _sp = pos;
					} else { _cp = _sp = 0; }
					ScrollToCaret();
					auto device = GetRenderingDevice();
					if (device) device->SetCaretReferenceTime(GetTimerValue());
					find_advice();
					Invalidate();
				}
			}
			void TextComboBox::RightButtonUp(Point at)
			{
				Box my = Box(0, 0, ControlBoundaries.Right - ControlBoundaries.Left, ControlBoundaries.Bottom - ControlBoundaries.Top);
				Box text = Box(TextPosition, my);
				if (_menu && text.IsInside(at)) {
					auto undo = _menu->FindMenuItem(1001);
					auto redo = _menu->FindMenuItem(1000);
					auto cut = _menu->FindMenuItem(1002);
					auto copy = _menu->FindMenuItem(1003);
					auto paste = _menu->FindMenuItem(1004);
					auto remove = _menu->FindMenuItem(1005);
					if (undo) undo->Enable(_undo.CanUndo());
					if (redo) redo->Enable(_undo.CanRedo());
					if (cut) cut->Enable(_cp != _sp);
					if (copy) copy->Enable(_cp != _sp);
					if (paste) paste->Enable(Clipboard::IsFormatAvailable(Clipboard::Format::Text));
					if (remove) remove->Enable(_cp != _sp);
					_state = 2;
					Invalidate();
					RunMenu(_menu, this, at);
				}
			}
			void TextComboBox::MouseMove(Point at)
			{
				Box my = Box(0, 0, ControlBoundaries.Right - ControlBoundaries.Left, ControlBoundaries.Bottom - ControlBoundaries.Top);
				Box text = Box(TextPosition, my);
				Box button = Box(ButtonPosition, my);
				if (_state == 0) {
					if (button.IsInside(at) && IsHovered()) {
						_state = 3;
						SetCapture();
						Invalidate();
					}
				} else if (_state == 3) {
					if (!button.IsInside(at) || !IsHovered()) ReleaseCapture();
				} else if (_state == 1) {
					int ocp = _cp;
					_cp = _text_info ? _text_info->TestPosition(at.x - text.Left - _shift) : 0;
					ScrollToCaret();
					auto device = GetRenderingDevice();
					if (device) device->SetCaretReferenceTime(GetTimerValue());
					if (ocp != _cp) find_advice();
					Invalidate();
				}
			}
			bool TextComboBox::KeyDown(int key_code)
			{
				auto device = GetRenderingDevice();
				if (device) device->SetCaretReferenceTime(GetTimerValue());
				if (_state != 4) {
					if (key_code == KeyCodes::Down) {
						_state = 4;
						run_drop_down();
						return true;
					} else if (key_code == KeyCodes::Back) {
						if (_cp != _sp || _cp > 0) {
							if (_save) {
								_undo.PushCurrentVersion();
								_save = false;
							}
							if (_cp == _sp) _cp = _sp - 1;
							Print(L"");
							_deferred_scroll = true;
						} else Beep();
						return true;
					} else if (key_code == KeyCodes::Delete) {
						if (_cp != _sp || _cp < _text.Length()) {
							if (_save) {
								_undo.PushCurrentVersion();
								_save = false;
							}
							if (_cp == _sp) _cp = _sp + 1;
							Print(L"");
							_deferred_scroll = true;
						} else Beep();
						return true;
					} else if (key_code == KeyCodes::Left) {
						if (_cp > 0) {
							_save = true;
							_cp--;
							if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
							_deferred_scroll = true;
							find_advice();
							Invalidate();
						} else Beep();
						return true;
					} else if (key_code == KeyCodes::Right) {
						if (_cp < _text.Length()) {
							_save = true;
							_cp++;
							if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
							_deferred_scroll = true;
							find_advice();
							Invalidate();
						} else if (_cp == _sp && _cp && _advice != -1) {
							if (_save) {
								_undo.PushCurrentVersion();
								_save = false;
							}
							string residue = _elements[_advice];
							_text.SetLength(residue.GetEncodedLength(Encoding::UTF32));
							residue.Encode(_text.GetBuffer(), Encoding::UTF32, false);
							Text = residue;
							_text_info.SetReference(0);
							_sp = 0;
							_cp = _text.Length();
							_deferred_scroll = true;
							find_advice();
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						} else {
							if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) {
								_save = true;
								_sp = _cp;
								_deferred_scroll = true;
								find_advice();
								Invalidate();
							} else Beep();
						}
						return true;
					} else if (key_code == KeyCodes::Escape) {
						_save = true;
						_sp = _cp;
						find_advice();
						Invalidate();
						return true;
					} else if (key_code == KeyCodes::Home) {
						_save = true;
						_cp = 0;
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
						_deferred_scroll = true;
						find_advice();
						Invalidate();
						return true;
					} else if (key_code == KeyCodes::End) {
						_save = true;
						_cp = _text.Length();
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) _sp = _cp;
						_deferred_scroll = true;
						find_advice();
						Invalidate();
						return true;
					} else if (!Keyboard::IsKeyPressed(KeyCodes::Shift) && Keyboard::IsKeyPressed(KeyCodes::Control) && !Keyboard::IsKeyPressed(KeyCodes::Alternative) && !Keyboard::IsKeyPressed(KeyCodes::System)) {
						if (key_code == KeyCodes::Z) { Undo(); return true; }
						else if (key_code == KeyCodes::X) { Cut(); return true; }
						else if (key_code == KeyCodes::C) { Copy(); return true; }
						else if (key_code == KeyCodes::V) { Paste(); return true; }
						else if (key_code == KeyCodes::Y) { Redo(); return true; }
					}
				} else return _list->KeyDown(key_code);
				return false;
			}
			void TextComboBox::CharDown(uint32 ucs_code)
			{
				auto device = GetRenderingDevice();
				if (device) device->SetCaretReferenceTime(GetTimerValue());
				if (_state != 4) {
					string filtered = FilterInput(string(&ucs_code, 1, Encoding::UTF32));
					if (filtered.Length()) {
						if (_save) {
							_undo.PushCurrentVersion();
							_save = false;
						}
						Print(filtered);
						_deferred_scroll = true;
						find_advice();
					} else Beep();
				}
			}
			void TextComboBox::PopupMenuCancelled(void) { if (_state == 2) { _state = 0; Invalidate(); } }
			void TextComboBox::SetCursor(Point at)
			{
				auto cursor = Windows::SystemCursorClass::Arrow;
				if (Box(TextPosition, Box(0, 0, ControlBoundaries.Right - ControlBoundaries.Left, ControlBoundaries.Bottom - ControlBoundaries.Top)).IsInside(at) || _state == 1)
					cursor = Windows::SystemCursorClass::Beam;
				SelectCursor(cursor);
			}
			bool TextComboBox::IsWindowEventEnabled(Windows::WindowHandler handler)
			{
				if (handler == Windows::WindowHandler::Undo) return _undo.CanUndo();
				else if (handler == Windows::WindowHandler::Redo) return _undo.CanRedo();
				else if (handler == Windows::WindowHandler::Cut) return _cp != _sp;
				else if (handler == Windows::WindowHandler::Copy) return _cp != _sp;
				else if (handler == Windows::WindowHandler::Paste) return Clipboard::IsFormatAvailable(Clipboard::Format::Text);
				else if (handler == Windows::WindowHandler::Delete) return _cp != _sp;
				else if (handler == Windows::WindowHandler::SelectAll) return true;
				else return false;
			}
			void TextComboBox::HandleWindowEvent(Windows::WindowHandler handler)
			{
				if (handler == Windows::WindowHandler::Undo) Undo();
				else if (handler == Windows::WindowHandler::Redo) Redo();
				else if (handler == Windows::WindowHandler::Cut) Cut();
				else if (handler == Windows::WindowHandler::Copy) Copy();
				else if (handler == Windows::WindowHandler::Paste) Paste();
				else if (handler == Windows::WindowHandler::Delete) Delete();
				else if (handler == Windows::WindowHandler::SelectAll) SetSelection(0, _text.Length());
			}
			ControlRefreshPeriod TextComboBox::GetFocusedRefreshPeriod(void) { return ControlRefreshPeriod::CaretBlink; }
			string TextComboBox::GetControlClass(void) { return L"TextComboBox"; }
			void TextComboBox::Undo(void)
			{
				if (_undo.CanUndo()) {
					_undo.Undo();
					_cp = _sp = 0;
					_save = true;
					_deferred_scroll = true;
					_text_info.SetReference(0);
					find_advice();
					Invalidate();
					GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
				} else Beep();
			}
			void TextComboBox::Redo(void)
			{
				if ( _undo.CanRedo()) {
					_undo.Redo();
					_cp = _sp = 0;
					_save = true;
					_deferred_scroll = true;
					_text_info.SetReference(0);
					find_advice();
					Invalidate();
					GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
				} else Beep();
			}
			void TextComboBox::Cut(void)
			{
				if (_cp != _sp) {
					_undo.PushCurrentVersion();
					_save = true;
					Clipboard::SetData(GetSelection());
					Print(L"");
					_deferred_scroll = true;
					find_advice();
				} else Beep();
			}
			void TextComboBox::Copy(void)
			{
				if (_cp != _sp) {
					_save = true;
					Clipboard::SetData(GetSelection());
				} else Beep();
			}
			void TextComboBox::Paste(void)
			{
				string text;
				if (Clipboard::GetData(text)) {
					string filter = FilterInput(text);
					if (filter.Length()) {
						_undo.PushCurrentVersion();
						_save = true;
						Print(filter);
						_deferred_scroll = true;
						find_advice();
					} else Beep();
				} else Beep();
			}
			void TextComboBox::Delete(void)
			{
				if (_cp != _sp) {
					_undo.PushCurrentVersion();
					_save = true;
					Print(L"");
					_deferred_scroll = true;
					find_advice();
				} else Beep();
			}
			string TextComboBox::GetSelection(void)
			{
				int sp = min(const_cast<const int &>(_sp), _cp);
				int ep = max(const_cast<const int &>(_sp), _cp);
				return string(_text.GetBuffer() + sp, ep - sp, Encoding::UTF32);
			}
			void TextComboBox::SetSelection(int selection_position, int caret_position)
			{
				_sp = min(max(selection_position, 0), _text.Length());
				_cp = min(max(caret_position, 0), _text.Length());
				find_advice();
				Invalidate();
			}
			void TextComboBox::ScrollToCaret(void)
			{
				if (_text_info) {
					auto field = Box(TextPosition, Box(0, 0, ControlBoundaries.Right - ControlBoundaries.Left, ControlBoundaries.Bottom - ControlBoundaries.Top));
					int width = field.Right - field.Left;
					int shifted_caret = ((_cp > 0) ? _text_info->EndOfChar(_cp - 1) : 0) + _shift;
					if (shifted_caret < 0) _shift -= shifted_caret;
					else if (shifted_caret + _caret_width >= width) _shift -= shifted_caret + _caret_width - width;
					Invalidate();
				}
			}
			void TextComboBox::SetPlaceholder(const string & text) { Placeholder = text; _placeholder_info.SetReference(0); Invalidate(); }
			string TextComboBox::GetPlaceholder(void) { return Placeholder; }
			void TextComboBox::SetCharacterFilter(const string & filter)
			{
				CharactersEnabled = filter;
				_chars_enabled.SetLength(filter.GetEncodedLength(Encoding::UTF32));
				filter.Encode(_chars_enabled.GetBuffer(), Encoding::UTF32, false);
			}
			string TextComboBox::GetCharacterFilter(void) { return CharactersEnabled; }
			void TextComboBox::SetContextMenu(Windows::IMenu * menu) { _menu.SetRetain(menu); }
			Windows::IMenu * TextComboBox::GetContextMenu(void) { return _menu; }
			string TextComboBox::FilterInput(const string & input)
			{
				string conv = input;
				if (LowerCase) conv = conv.LowerCase();
				else if (UpperCase) conv = conv.UpperCase();
				Array<uint32> utf32(0x100);
				utf32.SetLength(conv.GetEncodedLength(Encoding::UTF32));
				conv.Encode(utf32.GetBuffer(), Encoding::UTF32, false);
				for (int i = 0; i < utf32.Length(); i++) {
					bool enabled = false;
					if (utf32[i] >= 0x20) {
						if (_chars_enabled.Length()) for (int j = 0; j < _chars_enabled.Length(); j++) {
							if (utf32[i] == _chars_enabled[j]) { enabled = true; break; }
						} else enabled = true;
					}
					if (!enabled) {
						utf32.Remove(i);
						i--;
					}
				}
				return string(utf32.GetBuffer(), utf32.Length(), Encoding::UTF32);
			}
			void TextComboBox::Print(const string & text)
			{
				auto norm = text.NormalizedForm();
				Array<uint32> utf32(0x100);
				utf32.SetLength(norm.GetEncodedLength(Encoding::UTF32));
				norm.Encode(utf32.GetBuffer(), Encoding::UTF32, false);
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
				_advice_info.SetReference(0);
				find_advice();
				Invalidate();
				GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
			}
			void TextComboBox::AddItem(const string & text) { _elements.Append(text); }
			void TextComboBox::InsertItem(const string & text, int at) { _elements.Insert(text, at); if (_advice >= at) _advice++; }
			void TextComboBox::SwapItems(int i, int j)
			{
				if (_advice != -1) {
					if (_advice == i) _advice = j;
					else if (_advice == j) _advice = i;
				}
				_elements.SwapAt(i, j);
			}
			void TextComboBox::RemoveItem(int index) { _elements.Remove(index); _advice = -1; _advice_info.SetReference(0); Invalidate(); }
			void TextComboBox::ClearItems(void) { _elements.Clear(); _advice = -1; _advice_info.SetReference(0); Invalidate(); }
			int TextComboBox::ItemCount(void) { return _elements.Length(); }
			void TextComboBox::SetItemText(int index, const string & text) { _elements[index] = text; if (_advice == index) { _advice = -1; _advice_info.SetReference(0); Invalidate(); } }
			string TextComboBox::GetItemText(int index) { return _elements[index]; }

			void TextComboBox::TextComboListBox::scroll_to_current(void)
			{
				if (!_owner) return;
				if (_current >= 0) {
					int min = _owner->ElementHeight * _current;
					int max = min + _owner->ElementHeight;
					if (_scroll->Position > min) _scroll->SetScrollerPosition(min);
					if (_scroll->Position + _scroll->Page < max) _scroll->SetScrollerPosition(max - _scroll->Page);
				}
			}
			void TextComboBox::TextComboListBox::move_selection(int to)
			{
				if (!_owner) return;
				int index = max(min(to, _owner->_elements.Length() - 1), 0);
				if (!_owner->_elements.Length()) return;
				_current = index;
				_hot = index;
				scroll_to_current();
				if (_owner->GetText() != _owner->_elements[_current]) {
					if (_owner->_save) {
						_owner->_undo.PushCurrentVersion();
						_owner->_save = false;
					}
					_owner->Text = _owner->_elements[_current];
					_owner->_text.SetLength(_owner->Text.GetEncodedLength(Encoding::UTF32));
					_owner->Text.Encode(_owner->_text.GetBuffer(), Encoding::UTF32, false);
					_owner->_text_info.SetReference(0);
					_owner->_sp = 0;
					_owner->_cp = _owner->_text.Length();
					_owner->_deferred_scroll = true;
					_owner->find_advice();
					_owner->SubmitEvent(1);
					_owner->Invalidate();
				}
				Invalidate();
			}
			TextComboBox::TextComboListBox::TextComboListBox(TextComboBox * Base) : _elements(0x10), _owner(Base)
			{
				_hot = -1;
				string text = Base->GetText();
				for (int i = 0; i < Base->_elements.Length(); i++) {
					_elements.Append(0);
					if (_hot == -1 && string::CompareIgnoreCase(text, Base->_elements[i]) == 0) { _hot = i; }
				}
				if (_hot == -1) _hot = Base->_advice;
				_current = _hot;
				_position = Rectangle::Entire();
				SafePointer<VerticalScrollBar> bar = new VerticalScrollBar(Base);
				_scroll = bar.Inner();
				AddChild(bar);
				_scroll->ControlPosition = Rectangle(Coordinate::Right() - Base->Border - Base->ScrollSize, Base->Border, Coordinate::Right() - Base->Border, Coordinate::Bottom() - Base->Border);
				_scroll->Line = Base->ElementHeight;
				_scroll->Invisible = true;
				int space = Base->_elements.Length() * Base->ElementHeight;
				_scroll->SetRangeSilent(0, space - 1);
			}
			TextComboBox::TextComboListBox::~TextComboListBox(void) { if (_owner) { _owner->_list = 0; _owner->_state = 0; _owner->Invalidate(); _owner->SetFocus(); } }
			void TextComboBox::TextComboListBox::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				if (!_owner) return;
				if (!_view) {
					if (_owner->ViewDropDownList) {
						ZeroArgumentProvider provider;
						_view.SetReference(_owner->ViewDropDownList->Initialize(&provider));
						_view->Render(device, at);
					}
				} _view->Render(device, at);
				if (!_view_element_hot && _owner->ViewElementHot) {
					ZeroArgumentProvider provider;
					_view_element_hot.SetReference(_owner->ViewElementHot->Initialize(&provider));
				}
				if (!_view_element_selected && _owner->ViewElementSelected) {
					ZeroArgumentProvider provider;
					_view_element_selected.SetReference(_owner->ViewElementSelected->Initialize(&provider));
				}
				Box viewport = Box(at.Left + _owner->Border, at.Top + _owner->Border,
					at.Right - _owner->Border - (_svisible ? _owner->ScrollSize : 0), at.Bottom - _owner->Border);
				device->PushClip(viewport);
				if (_owner->_elements.Length()) {
					int from = max(min(_scroll->Position / _owner->ElementHeight, _owner->_elements.Length() - 1), 0);
					int to = max(min((_scroll->Position + _scroll->Page) / _owner->ElementHeight, _owner->_elements.Length() - 1), 0);
					for (int i = from; i <= to; i++) {
						Box item(viewport.Left, viewport.Top + i * _owner->ElementHeight - _scroll->Position, viewport.Right, 0);
						item.Bottom = item.Top + _owner->ElementHeight;
						if (i == _hot && _view_element_hot) {
							_view_element_hot->Render(device, item);
						} else if (i == _current && _view_element_selected) {
							_view_element_selected->Render(device, item);
						}
						if (_elements.ElementAt(i)) {
							_elements.ElementAt(i)->Render(device, item);
						} else if (_owner->ViewElementNormal) {
							ArgumentService::TextComboBoxElementArgumentProvider provider(_owner, _owner->_elements[i]);
							SafePointer<Shape> shape = _owner->ViewElementNormal->Initialize(&provider);
							_elements.SetElement(shape, i);
							_elements.ElementAt(i)->Render(device, item);
						}
					}
				}
				device->PopClip();
				ParentControl::Render(device, at);
			}
			void TextComboBox::TextComboListBox::ResetCache(void) { if (GetControlSystem()) GetControlSystem()->SetExclusiveControl(0); }
			void TextComboBox::TextComboListBox::SetRectangle(const Rectangle & rect) { _position = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle TextComboBox::TextComboListBox::GetRectangle(void) { return _position; }
			void TextComboBox::TextComboListBox::SetPosition(const Box & box)
			{
				ControlBoundaries = box; ArrangeChildren();
				if (!_owner) return;
				int page = box.Bottom - box.Top - (_owner->Border << 1);
				_scroll->SetPageSilent(page);
				_scroll->SetScrollerPositionSilent(_current * _owner->ElementHeight - (page >> 1));
				if (page < _owner->_elements.Length() * _owner->ElementHeight) _scroll->Show(_svisible = true);
			}
			void TextComboBox::TextComboListBox::CaptureChanged(bool got_capture) { if (!got_capture) { _hot = -1; Invalidate(); } }
			void TextComboBox::TextComboListBox::LostExclusiveMode(void) { if (_owner) _owner->SetFocus(); DestroyPopup(GetParent()); }
			void TextComboBox::TextComboListBox::LeftButtonUp(Point at)
			{
				if (!_owner) return;
				if (_hot != -1) {
					move_selection(_hot);
				}
				GetControlSystem()->SetExclusiveControl(0);
			}
			void TextComboBox::TextComboListBox::MouseMove(Point at)
			{
				if (!_owner) return;
				Box element(_owner->Border, _owner->Border, ControlBoundaries.Right - ControlBoundaries.Left - _owner->Border - (_svisible ? _owner->ScrollSize : 0), ControlBoundaries.Bottom - ControlBoundaries.Top - _owner->Border);
				int oh = _hot;
				if (element.IsInside(at) && IsHovered()) {
					int index = (at.y - _owner->Border + _scroll->Position) / _owner->ElementHeight;
					if (index < 0 || index >= _owner->_elements.Length()) index = -1;
					_hot = index;
				} else {
					_hot = -1;
				}
				if (oh != _hot) Invalidate();
				if (oh == -1 && _hot != -1) SetCapture();
				else if ((oh != -1 || GetCapture() == this) && _hot == -1) ReleaseCapture();
			}
			void TextComboBox::TextComboListBox::ScrollVertically(double delta)
			{
				if (!_owner) return;
				if (_svisible) {
					_scroll->SetScrollerPosition(_scroll->Position + int(delta * double(_scroll->Line)));
				}
			}
			bool TextComboBox::TextComboListBox::KeyDown(int key_code)
			{
				if (!_owner) return false;
				int page = max((ControlBoundaries.Bottom - ControlBoundaries.Top - _owner->Border - _owner->Border) / _owner->ElementHeight, 1);
				if (key_code == KeyCodes::Left) {
					GetControlSystem()->SetExclusiveControl(0);
				} else if (key_code == KeyCodes::Down) {
					if (_current != -1) move_selection(_current + 1); else move_selection(0);
				} else if (key_code == KeyCodes::Up) {
					if (_current != -1) move_selection(_current - 1); else move_selection(0);
				} else if (key_code == KeyCodes::End) {
					move_selection(_owner->_elements.Length() - 1);
				} else if (key_code == KeyCodes::Home) {
					move_selection(0);
				} else if (key_code == KeyCodes::PageDown) {
					if (_current != -1) move_selection(_current + page); else move_selection(0);
				} else if (key_code == KeyCodes::PageUp) {
					if (_current != -1) move_selection(_current - page); else move_selection(0);
				}
				return true;
			}
			Control * TextComboBox::TextComboListBox::HitTest(Point at)
			{
				if (!IsEnabled()) return this;
				if (_scroll && _svisible) {
					auto box = _scroll->GetPosition();
					if (box.IsInside(at)) return _scroll->HitTest(Point(at.x - box.Left, at.y - box.Top));
				}
				return this;
			}
			string TextComboBox::TextComboListBox::GetControlClass(void) { return L"TextComboListBox"; }
		}
	}
}