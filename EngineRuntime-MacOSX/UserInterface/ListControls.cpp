#include "ListControls.h"

#include "../PlatformDependent/KeyCodes.h"
#include "OverlappedWindows.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace ArgumentService
			{
				class ListBoxSimpleArgumentProvider : public IArgumentProvider
				{
				public:
					ListBox * Owner;
					const string & Text;
					ListBoxSimpleArgumentProvider(ListBox * owner, const string & text) : Owner(owner), Text(text) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Text;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, ITexture ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, IFont ** value) override
					{
						if (name == L"Font" && Owner->Font) {
							*value = Owner->Font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
				class ListBoxWrapperArgumentProvider : public IArgumentProvider
				{
				public:
					ListBox * Owner;
					IArgumentProvider * Inner;
					ListBoxWrapperArgumentProvider(ListBox * owner, IArgumentProvider * inner) : Owner(owner), Inner(inner) {}
					virtual void GetArgument(const string & name, int * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, double * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, Color * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, string * value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, ITexture ** value) override { Inner->GetArgument(name, value); }
					virtual void GetArgument(const string & name, IFont ** value) override
					{
						if (name == L"Font") {
							*value = Owner->Font;
							if (Owner->Font) (*value)->Retain();
						} else Inner->GetArgument(name, value);
					}
				};
			}
			void ListBox::reset_scroll_ranges(void)
			{
				int page = WindowPosition.Bottom - WindowPosition.Top - Border - Border;
				int hpage = WindowPosition.Right - WindowPosition.Left - Border - Border - (_svisible ? ScrollSize : 0);
				int lines_max;
				if (Tiled) {
					int epl = max(hpage / ElementHeight, 1);
					lines_max = (_elements.Length() / epl + ((_elements.Length() % epl) ? 1 : 0)) * ElementHeight - 1;
				} else {
					lines_max = ElementHeight * _elements.Length() - 1;
				}
				_scroll->SetRange(0, lines_max);
				_scroll->Show(_svisible = (lines_max >= page));
			}
			void ListBox::scroll_to_current(void)
			{
				if (_current >= 0) {
					if (Tiled) {
						int hpage = WindowPosition.Right - WindowPosition.Left - Border - Border - (_svisible ? ScrollSize : 0);
						int epl = max(hpage / ElementHeight, 1);
						int min = ElementHeight * (_current / epl);
						int max = min + ElementHeight;
						if (_scroll->Position > min) _scroll->SetScrollerPosition(min);
						if (_scroll->Position + _scroll->Page < max) _scroll->SetScrollerPosition(max - _scroll->Page);
					} else {
						int min = ElementHeight * _current;
						int max = min + ElementHeight;
						if (_scroll->Position > min) _scroll->SetScrollerPosition(min);
						if (_scroll->Position + _scroll->Page < max) _scroll->SetScrollerPosition(max - _scroll->Page);
					}
				}
			}
			void ListBox::select(int index, bool down)
			{
				if (MultiChoose) {
					if (Keyboard::IsKeyPressed(KeyCodes::Control) && down) {
						if (index != -1) {
							if (_elements[index].Selected) {
								_elements[index].Selected = false;
								if (_current == index) {
									_current = -1;
									for (int i = 0; i < _elements.Length(); i++) if (_elements[i].Selected) { _current = i; break; }
									if (_editor) { _editor->Destroy(); _editor = 0; }
								}
								GetParent()->RaiseEvent(ID, Event::ValueChange, this);
							} else {
								_elements[index].Selected = true;
								_current = index;
								if (_editor) { _editor->Destroy(); _editor = 0; }
								scroll_to_current();
								GetParent()->RaiseEvent(ID, Event::ValueChange, this);
							}
						}
					} else if (Keyboard::IsKeyPressed(KeyCodes::Shift) && down) {
						if (index != -1) {
							if (_current == -1) {
								_current = index;
								_elements[_current].Selected = true;
								scroll_to_current();
								GetParent()->RaiseEvent(ID, Event::ValueChange, this);
							} else {
								if (_current != index) {
									for (int i = min(_current, index); i <= max(_current, index); i++) _elements[i].Selected = true;
									_current = index;
									if (_editor) { _editor->Destroy(); _editor = 0; }
									scroll_to_current();
									GetParent()->RaiseEvent(ID, Event::ValueChange, this);
								}
							}
						}
					} else if (down) {
						if (index != -1) {
							if (_elements[index].Selected) {
								if (_current != index) {
									_current = index;
									if (_editor) { _editor->Destroy(); _editor = 0; }
									scroll_to_current();
									GetParent()->RaiseEvent(ID, Event::ValueChange, this);
								}
							} else {
								_current = index;
								for (int i = 0; i < _elements.Length(); i++) _elements[i].Selected = i == _current;
								if (_editor) { _editor->Destroy(); _editor = 0; }
								scroll_to_current();
								GetParent()->RaiseEvent(ID, Event::ValueChange, this);
							}
						}
					}
				} else {
					if (Keyboard::IsKeyPressed(KeyCodes::Control) && down) {
						if (_current == index && index != -1) {
							_elements[_current].Selected = false;
							_current = -1;
							if (_editor) { _editor->Destroy(); _editor = 0; }
							GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						}
					} else {
						if (_current != index && index != -1) {
							if (_current != -1) _elements[_current].Selected = false;
							_current = index;
							_elements[_current].Selected = true;
							if (_editor) { _editor->Destroy(); _editor = 0; }
							scroll_to_current();
							GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						}
					}
				}
			}
			void ListBox::move_selection(int to)
			{
				int index = max(min(to, _elements.Length() - 1), 0);
				if (!_elements.Length()) return;
				if (_current != index) {
					if (MultiChoose) {
						if (Keyboard::IsKeyPressed(KeyCodes::Shift) && _current != -1) {
							for (int i = min(_current, index); i <= max(_current, index); i++) _elements[i].Selected = true;
							_current = index;
							if (_editor) { _editor->Destroy(); _editor = 0; }
							scroll_to_current();
							GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						} else {
							_current = index;
							for (int i = 0; i < _elements.Length(); i++) _elements[i].Selected = i == _current;
							if (_editor) { _editor->Destroy(); _editor = 0; }
							scroll_to_current();
							GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						}
					} else {
						if (_current != -1) _elements[_current].Selected = false;
						_current = index;
						_elements[_current].Selected = true;
						if (_editor) { _editor->Destroy(); _editor = 0; }
						scroll_to_current();
						GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					}
				}
			}
			ListBox::ListBox(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station), _elements(0x10)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				ResetCache();
			}
			ListBox::ListBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station), _elements(0x10)
			{
				if (Template->Properties->GetTemplateClass() != L"ListBox") throw InvalidArgumentException();
				static_cast<Template::Controls::ListBox &>(*this) = static_cast<Template::Controls::ListBox &>(*Template->Properties);
				ResetCache();
			}
			ListBox::~ListBox(void) {}
			void ListBox::Render(const Box & at)
			{
				auto device = GetStation()->GetRenderingDevice();
				Shape ** back = 0;
				Template::Shape * temp = 0;
				if (Disabled) {
					back = _view_disabled.InnerRef();
					temp = ViewDisabled.Inner();
				} else {
					if (GetFocus() == this) {
						back = _view_focused.InnerRef();
						temp = ViewFocused.Inner();
					} else {
						back = _view_normal.InnerRef();
						temp = ViewNormal.Inner();
					}
				}
				if (*back) {
					(*back)->Render(device, at);
				} else if (temp) {
					ZeroArgumentProvider provider;
					*back = temp->Initialize(&provider);
					(*back)->Render(device, at);
				}
				if (!_view_element_hot && ViewElementHot) {
					ZeroArgumentProvider provider;
					_view_element_hot.SetReference(ViewElementHot->Initialize(&provider));
				}
				if (!_view_element_selected && ViewElementSelected) {
					ZeroArgumentProvider provider;
					_view_element_selected.SetReference(ViewElementSelected->Initialize(&provider));
				}
				Box viewport = Box(at.Left + Border, at.Top + Border, at.Right - Border, at.Bottom - Border);
				device->PushClip(viewport);
				int width = at.Right - at.Left - Border - Border - (_svisible ? ScrollSize : 0);
				int height = at.Bottom - at.Top - Border - Border;
				if (_elements.Length()) {
					if (Tiled) {
						int epl = max(width / ElementHeight, 1);
						int from = max(min((_scroll->Position / ElementHeight) * epl, _elements.Length() - 1), 0);
						int to = max(min((_scroll->Position + _scroll->Page) / ElementHeight * epl + epl, _elements.Length() - 1), 0);
						if (Disabled) {
							for (int i = from; i <= to; i++) {
								if (i == _current && _editor) continue;
								int x = i % epl, y = i / epl;
								Box item(at.Left + Border + x * ElementHeight, at.Top + Border + y * ElementHeight - _scroll->Position, 0, 0);
								item.Right = item.Left + ElementHeight; item.Bottom = item.Top + ElementHeight;
								_elements[i].ViewDisabled->Render(device, item);
							}
						} else {
							for (int i = from; i <= to; i++) {
								int x = i % epl, y = i / epl;
								Box item(at.Left + Border + x * ElementHeight, at.Top + Border + y * ElementHeight - _scroll->Position, 0, 0);
								item.Right = item.Left + ElementHeight; item.Bottom = item.Top + ElementHeight;
								if (i == _hot && _view_element_hot) {
									_view_element_hot->Render(device, item);
								} else if (_elements[i].Selected && _view_element_selected) {
									_view_element_selected->Render(device, item);
								}
								if (i == _current && _editor) continue;
								_elements[i].ViewNormal->Render(device, item);
							}
						}
					} else {
						int from = max(min(_scroll->Position / ElementHeight, _elements.Length() - 1), 0);
						int to = max(min((_scroll->Position + _scroll->Page) / ElementHeight, _elements.Length() - 1), 0);
						if (Disabled) {
							for (int i = from; i <= to; i++) {
								if (i == _current && _editor) continue;
								Box item(at.Left + Border, at.Top + Border + i * ElementHeight - _scroll->Position,
									at.Left + Border + width, 0);
								item.Bottom = item.Top + ElementHeight;
								_elements[i].ViewDisabled->Render(device, item);
							}
						} else {
							for (int i = from; i <= to; i++) {
								Box item(at.Left + Border, at.Top + Border + i * ElementHeight - _scroll->Position,
									at.Left + Border + width, 0);
								item.Bottom = item.Top + ElementHeight;
								if (i == _hot && _view_element_hot) {
									_view_element_hot->Render(device, item);
								} else if (_elements[i].Selected && _view_element_selected) {
									_view_element_selected->Render(device, item);
								}
								if (i == _current && _editor) continue;
								_elements[i].ViewNormal->Render(device, item);
							}
						}
					}
				}
				if (_editor && _editor->IsVisible()) {
					Box pos = _editor->GetPosition();
					Box rect = Box(pos.Left + at.Left, pos.Top + at.Top, pos.Right + at.Left, pos.Bottom + at.Top);
					device->PushClip(rect);
					_editor->Render(rect);
					device->PopClip();
				}
				device->PopClip();
				if (_scroll && _svisible) {
					Box pos = _scroll->GetPosition();
					Box rect = Box(pos.Left + at.Left, pos.Top + at.Top, pos.Right + at.Left, pos.Bottom + at.Top);
					device->PushClip(rect);
					_scroll->Render(rect);
					device->PopClip();
				}
			}
			void ListBox::ResetCache(void)
			{
				int pos = (_scroll) ? _scroll->Position : 0;
				if (_editor) _editor->ResetCache();
				if (_scroll) _scroll->Destroy();
				int page = WindowPosition.Bottom - WindowPosition.Top - Border - Border;
				int hpage = WindowPosition.Right - WindowPosition.Left - Border - Border - (_svisible ? ScrollSize : 0);
				int lines_max;
				if (Tiled) {
					int epl = max(hpage / ElementHeight, 1);
					lines_max = (_elements.Length() / epl + ((_elements.Length() % epl) ? 1 : 0)) * ElementHeight - 1;
				} else {
					lines_max = ElementHeight * _elements.Length() - 1;
				}
				_scroll = GetStation()->CreateWindow<VerticalScrollBar>(this, this);
				_scroll->SetRectangle(Rectangle(Coordinate::Right() - ScrollSize - Border, Border,
					Coordinate::Right() - Border, Coordinate::Bottom() - Border));
				_scroll->Disabled = Disabled;
				_scroll->Line = ElementHeight;
				_scroll->SetPageSilent(page);
				_scroll->SetRangeSilent(0, lines_max);
				_scroll->SetScrollerPositionSilent(pos);
				_scroll->Invisible = !(_svisible = (lines_max >= page));
				for (int i = 0; i < _elements.Length(); i++) {
					_elements[i].ViewNormal->ClearCache();
					_elements[i].ViewDisabled->ClearCache();
				}
				_view_normal.SetReference(0);
				_view_disabled.SetReference(0);
				_view_focused.SetReference(0);
				_view_element_hot.SetReference(0);
				_view_element_selected.SetReference(0);
			}
			void ListBox::ArrangeChildren(void)
			{
				Box inner = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				if (_scroll) {
					auto rect = _scroll->GetRectangle();
					if (rect.IsValid()) _scroll->SetPosition(Box(rect, inner));
				}
				if (_editor) {
					auto rect = _editor->GetRectangle();
					if (Tiled) {
						int hpage = WindowPosition.Right - WindowPosition.Left - Border - Border - (_svisible ? ScrollSize : 0);
						int epl = max(hpage / ElementHeight, 1);
						int x = _current % epl, y = _current / epl;
						auto item = Box(Border + x * ElementHeight, Border + y * ElementHeight - _scroll->Position,
							Border + (x + 1) * ElementHeight, Border + (y + 1) * ElementHeight - _scroll->Position);
						_editor->SetPosition(Box(rect, item));
					} else {
						auto item = Box(Border, Border + ElementHeight * _current - _scroll->Position,
							inner.Right - Border - (_svisible ? ScrollSize : 0), Border + ElementHeight * (_current + 1) - _scroll->Position);
						_editor->SetPosition(Box(rect, item));
					}
				}
			}
			void ListBox::Enable(bool enable) { Disabled = !enable; _hot = -1; if (_scroll) _scroll->Enable(enable); }
			bool ListBox::IsEnabled(void) { return !Disabled; }
			void ListBox::Show(bool visible) { Invisible = !visible; _hot = -1; }
			bool ListBox::IsVisible(void) { return !Invisible; }
			bool ListBox::IsTabStop(void) { return true; }
			void ListBox::SetID(int _ID) { ID = _ID; }
			int ListBox::GetID(void) { return ID; }
			void ListBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle ListBox::GetRectangle(void) { return ControlPosition; }
			void ListBox::SetPosition(const Box & box)
			{
				WindowPosition = box;
				int page = WindowPosition.Bottom - WindowPosition.Top - Border - Border;
				int hpage = WindowPosition.Right - WindowPosition.Left - Border - Border - (_svisible ? ScrollSize : 0);
				int lines_max;
				if (Tiled) {
					int epl = max(hpage / ElementHeight, 1);
					lines_max = (_elements.Length() / epl + ((_elements.Length() % epl) ? 1 : 0)) * ElementHeight - 1;
				} else {
					lines_max = ElementHeight * _elements.Length() - 1;
				}
				_scroll->SetRange(0, lines_max);
				_scroll->SetPage(page);
				_scroll->Show(_svisible = (lines_max >= page));
				ArrangeChildren();
			}
			void ListBox::RaiseEvent(int ID, Event event, Window * sender)
			{
				if (sender == _scroll) {
					if (_editor) ArrangeChildren();
				} else GetParent()->RaiseEvent(ID, event, sender);
			}
			void ListBox::CaptureChanged(bool got_capture) { if (!got_capture) _hot = -1; }
			void ListBox::LeftButtonDown(Point at) { SetFocus(); select(_hot, true); }
			void ListBox::LeftButtonUp(Point at) { select(_hot, false); }
			void ListBox::LeftButtonDoubleClick(Point at) { if (_hot != -1) GetParent()->RaiseEvent(ID, Event::DoubleClick, this); }
			void ListBox::RightButtonDown(Point at) { SetFocus(); select(_hot, true); }
			void ListBox::RightButtonUp(Point at) { select(_hot, false); GetParent()->RaiseEvent(ID, Event::ContextClick, this); }
			void ListBox::MouseMove(Point at)
			{
				Box element(Border, Border,
					WindowPosition.Right - WindowPosition.Left - Border - (_svisible ? ScrollSize : 0),
					WindowPosition.Bottom - WindowPosition.Top - Border);
				int oh = _hot;
				if (element.IsInside(at) && GetStation()->HitTest(GetStation()->GetCursorPos()) == this) {
					int index = (at.y - Border + _scroll->Position) / ElementHeight;
					if (Tiled) {
						int epl = max((element.Right - element.Left) / ElementHeight, 1);
						int x = (at.x - Border) / ElementHeight;
						if (x >= 0 && x < epl) {
							index = index * epl + x;
						} else index = -1;
					}
					if (index < 0 || index >= _elements.Length()) index = -1;
					_hot = index;
				} else {
					_hot = -1;
				}
				if (oh == -1 && _hot != -1) SetCapture();
				else if (oh != -1 && _hot == -1) ReleaseCapture();
			}
			void ListBox::ScrollVertically(double delta)
			{
				if (_svisible) {
					_scroll->SetScrollerPosition(_scroll->Position + int(delta * double(_scroll->Line)));
				}
			}
			void ListBox::KeyDown(int key_code)
			{
				if (Tiled) {
					int page = max((WindowPosition.Bottom - WindowPosition.Top - Border - Border) / ElementHeight, 1);
					int hpage = WindowPosition.Right - WindowPosition.Left - Border - Border - (_svisible ? ScrollSize : 0);
					int epl = max(hpage / ElementHeight, 1);
					if (key_code == KeyCodes::Right) {
						if (_current != -1) move_selection(_current + 1); else move_selection(0);
					} else if (key_code == KeyCodes::Left) {
						if (_current != -1) move_selection(_current - 1); else move_selection(0);
					} else if (key_code == KeyCodes::Down) {
						if (_current != -1) move_selection(_current + epl); else move_selection(0);
					} else if (key_code == KeyCodes::Up) {
						if (_current != -1) move_selection(_current - epl); else move_selection(0);
					} else if (key_code == KeyCodes::End) {
						move_selection(_elements.Length() - 1);
					} else if (key_code == KeyCodes::Home) {
						move_selection(0);
					} else if (key_code == KeyCodes::PageDown) {
						if (_current != -1) move_selection(_current + page * epl); else move_selection(0);
					} else if (key_code == KeyCodes::PageUp) {
						if (_current != -1) move_selection(_current - page * epl); else move_selection(0);
					}
				} else {
					int page = max((WindowPosition.Bottom - WindowPosition.Top - Border - Border) / ElementHeight, 1);
					if (key_code == KeyCodes::Down) {
						if (_current != -1) move_selection(_current + 1); else move_selection(0);
					} else if (key_code == KeyCodes::Up) {
						if (_current != -1) move_selection(_current - 1); else move_selection(0);
					} else if (key_code == KeyCodes::End) {
						move_selection(_elements.Length() - 1);
					} else if (key_code == KeyCodes::Home) {
						move_selection(0);
					} else if (key_code == KeyCodes::PageDown) {
						if (_current != -1) move_selection(_current + page); else move_selection(0);
					} else if (key_code == KeyCodes::PageUp) {
						if (_current != -1) move_selection(_current - page); else move_selection(0);
					}
				}
			}
			Window * ListBox::HitTest(Point at)
			{
				if (!IsEnabled()) return this;
				if (_scroll && _svisible) {
					auto box = _scroll->GetPosition();
					if (box.IsInside(at)) return _scroll->HitTest(Point(at.x - box.Left, at.y - box.Top));
				}
				if (_editor && at.x >= Border && at.y >= Border && at.x < WindowPosition.Right - WindowPosition.Left - Border &&
					at.y < WindowPosition.Bottom - WindowPosition.Top - Border) {
					if (!_editor->IsVisible()) return this;
					auto box = _editor->GetPosition();
					if (box.IsInside(at)) return _editor->HitTest(Point(at.x - box.Left, at.y - box.Top));
				}
				return this;
			}
			void ListBox::AddItem(const string & text, void * user) { InsertItem(text, _elements.Length(), user); }
			void ListBox::AddItem(IArgumentProvider * provider, void * user) { InsertItem(provider, _elements.Length(), user); }
			void ListBox::AddItem(Reflection::Reflected & object, void * user) { InsertItem(object, _elements.Length(), user); }
			void ListBox::InsertItem(const string & text, int at, void * user)
			{
				ArgumentService::ListBoxSimpleArgumentProvider provider(this, text);
				_elements.Insert(Element(), at);
				auto & e = _elements[at];
				e.User = user;
				e.ViewNormal.SetReference(ViewElementNormal->Initialize(&provider));
				e.ViewDisabled.SetReference(ViewElementDisabled->Initialize(&provider));
				e.Selected = false;
				reset_scroll_ranges();
				if (_current >= at) {
					_current++;
					if (_editor) ArrangeChildren();
				}
			}
			void ListBox::InsertItem(IArgumentProvider * provider, int at, void * user)
			{
				ArgumentService::ListBoxWrapperArgumentProvider wrapper(this, provider);
				_elements.Insert(Element(), at);
				auto & e = _elements[at];
				e.User = user;
				e.ViewNormal.SetReference(ViewElementNormal->Initialize(&wrapper));
				e.ViewDisabled.SetReference(ViewElementDisabled->Initialize(&wrapper));
				e.Selected = false;
				reset_scroll_ranges();
				if (_current >= at) {
					_current++;
					if (_editor) ArrangeChildren();
				}
			}
			void ListBox::InsertItem(Reflection::Reflected & object, int at, void * user)
			{
				ReflectorArgumentProvider provider(&object);
				InsertItem(&provider, at, user);
			}
			void ListBox::ResetItem(int index, const string & text)
			{
				ArgumentService::ListBoxSimpleArgumentProvider provider(this, text);
				_elements[index].ViewNormal.SetReference(ViewElementNormal->Initialize(&provider));
				_elements[index].ViewDisabled.SetReference(ViewElementDisabled->Initialize(&provider));
			}
			void ListBox::ResetItem(int index, IArgumentProvider * provider)
			{
				ArgumentService::ListBoxWrapperArgumentProvider wrapper(this, provider);
				_elements[index].ViewNormal.SetReference(ViewElementNormal->Initialize(&wrapper));
				_elements[index].ViewDisabled.SetReference(ViewElementDisabled->Initialize(&wrapper));
			}
			void ListBox::ResetItem(int index, Reflection::Reflected & object)
			{
				ReflectorArgumentProvider provider(&object);
				ResetItem(index, &provider);
			}
			void ListBox::SwapItems(int i, int j)
			{
				if (_current == i) { _current = j; if (_editor) ArrangeChildren(); }
				else if (_current == j) { _current = i; if (_editor) ArrangeChildren(); }
				_elements.SwapAt(i, j);
			}
			void ListBox::RemoveItem(int index)
			{
				_elements.Remove(index);
				reset_scroll_ranges();
				if (_current == index) {
					_current = -1;
					if (_editor) { _editor->Destroy(); _editor = 0; }
				} else if (_current > index) {
					_current--;
					if (_editor) ArrangeChildren();
				}
				_hot = -1;
			}
			void ListBox::ClearItems(void)
			{
				_elements.Clear();
				if (_editor) { _editor->Destroy(); _editor = 0; }
				_current = -1; _hot = -1;
				reset_scroll_ranges();
			}
			int ListBox::ItemCount(void) { return _elements.Length(); }
			void * ListBox::GetItemUserData(int index) { return _elements[index].User; }
			void ListBox::SetItemUserData(int index, void * user) { _elements[index].User = user; }
			int ListBox::GetSelectedIndex(void) { return _current; }
			void ListBox::SetSelectedIndex(int index, bool scroll_to_view)
			{
				if (_current != index && _editor) { _editor->Destroy(); _editor = 0; }
				if (MultiChoose) {
					_current = index;
					for (int i = 0; i < _elements.Length(); i++) _elements[i].Selected = i == _current;
				} else {
					if (_current != -1) _elements[_current].Selected = false;
					_current = index;
					if (_current != -1) _elements[_current].Selected = true;
				}
				if (scroll_to_view && _current >= 0) scroll_to_current();
			}
			bool ListBox::IsItemSelected(int index) { return _elements[index].Selected; }
			void ListBox::SelectItem(int index, bool select)
			{
				if (MultiChoose) {
					_elements[index].Selected = select;
					if (!select && index == _current) {
						_current = -1;
						if (_editor) { _editor->Destroy(); _editor = 0; }
						for (int i = 0; i < _elements.Length(); i++) if (_elements[i].Selected) { _current = i; break; }
					}
				} else {
					if (select) SetSelectedIndex(index);
					else if (index == _current) SetSelectedIndex(-1);
				}
			}
			Window * ListBox::CreateEmbeddedEditor(Template::ControlTemplate * Template, const Rectangle & Position)
			{
				CloseEmbeddedEditor();
				auto group = GetStation()->CreateWindow<ControlGroup>(this);
				try {
					group->ControlPosition = Position;
					Constructor::ConstructChildren(group, Template);
				} catch (...) { group->Destroy(); throw; }
				_editor = group;
				ArrangeChildren();
				return group;
			}
			Window * ListBox::GetEmbeddedEditor(void) { return _editor; }
			void ListBox::CloseEmbeddedEditor(void) { if (_editor) { _editor->Destroy(); _editor = 0; } }
		}
	}
}