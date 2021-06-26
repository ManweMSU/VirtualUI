#include "MenuBarControl.h"

#include "ControlServiceEx.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace ArgumentService
			{
				class MenuBarArgumentProvider : public IArgumentProvider
				{
					string _text;
					SafePointer<Graphics::IFont> _font;
				public:
					MenuBarArgumentProvider(const string & text, Graphics::IFont * font) : _text(text) { _font.SetRetain(font); }
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = _text;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font" && _font) {
							*value = _font.Inner();
							(*value)->Retain();
						} else *value = 0;
					}
				};
			}
			void MenuBar::_internal_render(Graphics::I2DDeviceContext * device, const Box & at, int current) noexcept
			{
				if (ViewBackground) ViewBackground->Render(device, at);
				if (!_styles->MenuItemReferenceFont) return;
				int height = at.Bottom - at.Top;
				bool update_org = false;
				for (int i = 0; i < _elements.Length(); i++) {
					auto & e = _elements[i];
					if (!e._pure_width) {
						int h;
						SafePointer<Graphics::ITextBrush> test = device->CreateTextBrush(_styles->MenuItemReferenceFont, e._text, 0, 0, Color(0, 0, 0));
						if (!test) return;
						test->GetExtents(e._pure_width, h);
						update_org = true;
					}
					if (!e._width) {
						e._width = e._pure_width + height;
						update_org = true;
					}
				}
				if (update_org) {
					int offs = _left_space;
					for (int i = 0; i < _elements.Length(); i++) {
						auto & e = _elements[i];
						if (e._flush_right) {
							offs = at.Right - at.Left - _left_space;
							for (int j = _elements.Length() - 1; j >= i; j--) offs -= _elements[j]._width;
						}
						e._org = offs;
						offs += e._width;
					}
				}
				for (int i = 0; i < _elements.Length(); i++) {
					auto & e = _elements[i];
					auto element_box = Box(at.Left + e._org, at.Top, at.Left + e._org + e._width, at.Bottom);
					SafePointer<Template::Shape> * base = &ViewElementNormal;
					SafePointer<Shape> * shape = &e._view_element_normal;
					if (e._enabled && (e._menu || _state != 1)) {
						if (i == current) {
							if (_state) {
								base = &ViewElementPressed;
								shape = &e._view_element_pressed;
							} else if (ViewElementHot) {
								base = &ViewElementHot;
								shape = &e._view_element_hot;
							}
						}
					} else {
						if (ViewElementDisabled) {
							base = &ViewElementDisabled;
							shape = &e._view_element_disabled;
						}
					}
					if (!(*shape) && *base) {
						ArgumentService::MenuBarArgumentProvider provider(e._text, _styles->MenuItemReferenceFont);
						(*shape) = (*base)->Initialize(&provider);
					}
					if (*shape) (*shape)->Render(device, element_box);
				}
			}
			VirtualPopupStyles * MenuBar::ProvideStyles(void) { return _styles; }
			Box MenuBar::ProvidePrimaryBox(void) { return GetAbsolutePosition(); }
			Box MenuBar::ProvideClientBox(void) { return GetParent()->GetAbsolutePosition(); }
			Shape * MenuBar::ProvideClientBackground(void) { return ViewClientEffect; }
			int MenuBar::ProvidePrimarySubmenu(void) { return _current; }
			void MenuBar::RenderMenu(Graphics::I2DDeviceContext * device, const Box & at, int current) { _internal_render(device, at, current); }
			int MenuBar::GetElementCount(void) { return _elements.Length(); }
			Windows::IMenu * MenuBar::GetElementMenu(int index) { return _elements[index]._enabled ? _elements[index]._menu.Inner() : 0; }
			Box MenuBar::GetElementBox(int index)
			{
				return Box(_elements[index]._org, 0, _elements[index]._org + _elements[index]._width, ControlBoundaries.Bottom - ControlBoundaries.Top);
			}
			MenuBar::MenuBar(void) : _elements(0x10), _id(0), _visible(true), _current(-1), _state(0), _left_space(0)
			{
				_position = Rectangle::Invalid();
				_styles = new VirtualPopupStyles;
				_styles->MenuElementDefaultSize = _styles->MenuSeparatorDefaultSize = 1;
				_styles->MenuBorderWidth = 0;
			}
			MenuBar::MenuBar(Template::ControlTemplate * Template) : _elements(0x10), _current(-1), _state(0)
			{
				if (Template->Properties->GetTemplateClass() != L"MenuBar") throw InvalidArgumentException();
				auto & prop = static_cast<Template::Controls::MenuBar &>(*Template->Properties);
				ZeroArgumentProvider zero;
				_position = prop.ControlPosition;
				_id = prop.ID;
				_left_space = prop.SizeLeftSpace;
				_visible = !prop.Invisible;
				_styles = new VirtualPopupStyles;
				_styles->MenuBorderWidth = prop.SizeMenuBorder;
				_styles->MenuElementDefaultSize = prop.SizeMenuItem;
				_styles->MenuSeparatorDefaultSize = prop.SizeMenuSeparator;
				_styles->PopupShadow = prop.ViewMenuShadow ? prop.ViewMenuShadow->Initialize(&zero) : 0;
				_styles->MenuFrame = prop.ViewMenuFrame ? prop.ViewMenuFrame->Initialize(&zero) : 0;
				_styles->SubmenuArrow = prop.ViewMenuArrow ? prop.ViewMenuArrow->Initialize(&zero) : 0;
				_styles->MenuSeparator = prop.ViewMenuSeparator ? prop.ViewMenuSeparator->Initialize(&zero) : 0;
				_styles->MenuItemReferenceFont = prop.Font;
				_styles->MenuItemNormal = prop.ViewMenuItemNormal;
				_styles->MenuItemHot = prop.ViewMenuItemHot;
				_styles->MenuItemGrayed = prop.ViewMenuItemGrayed;
				_styles->MenuItemNormalChecked = prop.ViewMenuItemNormalChecked;
				_styles->MenuItemHotChecked = prop.ViewMenuItemHotChecked;
				_styles->MenuItemGrayedChecked = prop.ViewMenuItemGrayedChecked;
				ViewBackground = prop.ViewBackground ? prop.ViewBackground->Initialize(&zero) : 0;
				ViewClientEffect = prop.ViewClientEffect ? prop.ViewClientEffect->Initialize(&zero) : 0;
				ViewElementNormal = prop.ViewElementNormal;
				ViewElementHot = prop.ViewElementHot;
				ViewElementPressed = prop.ViewElementPressed;
				ViewElementDisabled = prop.ViewElementDisabled;
				for (auto & c : Template->Children) {
					if (c.Properties->GetTemplateClass() != L"MenuBarElement") throw InvalidArgumentException();
					auto & eprops = static_cast<Template::Controls::MenuBarElement &>(*c.Properties);
					SafePointer<Windows::IMenu> menu = eprops.Menu ? CreateMenu(eprops.Menu) : 0;
					AppendMenuElement(eprops.ID, !eprops.Disabled, eprops.Text, menu);
					if (eprops.FlushRight) SetFlushRight(_elements.Length() - 1, true);
				}
			}
			MenuBar::~MenuBar(void) {}
			void MenuBar::Render(Graphics::I2DDeviceContext * device, const Box & at) { if (_state == 1) return; _internal_render(device, at, _current); }
			void MenuBar::ResetCache(void)
			{
				if (ViewBackground) ViewBackground->ClearCache();
				if (ViewClientEffect) ViewClientEffect->ClearCache();
				if (_styles->MenuFrame) _styles->MenuFrame->ClearCache();
				if (_styles->SubmenuArrow) _styles->SubmenuArrow->ClearCache();
				if (_styles->MenuSeparator) _styles->MenuSeparator->ClearCache();
				if (_styles->PopupShadow) _styles->PopupShadow->ClearCache();
				for (auto & e : _elements) e._reset();
			}
			void MenuBar::Show(bool visible) { _visible = visible; Invalidate(); if (!_visible) { _state = 0; _current = -1; } }
			bool MenuBar::IsVisible(void) { return _visible; }
			void MenuBar::SetID(int ID) { _id = ID; }
			int MenuBar::GetID(void) { return _id; }
			Control * MenuBar::FindChild(int ID) { if (ID && ID == _id) return this; else return 0; }
			void MenuBar::SetRectangle(const Rectangle & rect) { _position = rect; if (GetParent()) { GetParent()->ArrangeChildren(); Invalidate(); } }
			Rectangle MenuBar::GetRectangle(void) { return _position; }
			void MenuBar::SetPosition(const Box & box)
			{
				Control::SetPosition(box);
				if (_state == 1) GetControlSystem()->SubmitTask(CreateFunctionalTask([cs = GetControlSystem(), self = this]() { cs->SetExclusiveControl(0); self->ResetCache(); }));
				for (auto & e : _elements) e._width = 0;
			}
			void MenuBar::CaptureChanged(bool got_capture) { if (!got_capture && _state != 1) { _state = 0; _current = -1; Invalidate(); } }
			void MenuBar::LeftButtonDown(Point at)
			{
				if (_current >= 0 && !_state && _elements[_current]._enabled) {
					if (_elements[_current]._menu) {
						_state = 1;
						RunVirtualMenu(this, this);
					} else {
						_state = 2;
						Invalidate();
					}
				}
			}
			void MenuBar::LeftButtonUp(Point at)
			{
				if (_state == 2) {
					_state = 0;
					Invalidate();
					auto & e = _elements[_current];
					if (IsHovered() && at.x >= e._org && at.x < e._org + e._width) GetParent()->RaiseEvent(e._id, ControlEvent::MenuCommand, this);
				}
			}
			void MenuBar::MouseMove(Point at)
			{
				if (!_state) {
					if (IsHovered()) {
						SetCapture();
						int new_current = -1;
						for (int i = 0; i < _elements.Length(); i++) {
							if (at.x >= _elements[i]._org && at.x < _elements[i]._org + _elements[i]._width) { new_current = i; break; }
						}
						if (new_current != _current) {
							_current = new_current;
							Invalidate();
						}
					} else ReleaseCapture();
				}
			}
			void MenuBar::RaiseEvent(int ID, ControlEvent event, Control * sender) { _state = 0; _current = -1; Invalidate(); GetParent()->RaiseEvent(ID, event, this); }
			void MenuBar::PopupMenuCancelled(void) { _state = 0; _current = -1; Invalidate(); }
			string MenuBar::GetControlClass(void) { return L"MenuBar"; }
			VirtualPopupStyles & MenuBar::GetVisualStyles(void) noexcept { return *_styles; }
			const VirtualPopupStyles & MenuBar::GetVisualStyles(void) const noexcept { return *_styles; }
			int MenuBar::GetMenuLeftSpace(void) const noexcept { return _left_space; }
			void MenuBar::SetMenuLeftSpace(int space) noexcept { _left_space = space; Invalidate(); }
			int MenuBar::GetMenuElementCount(void) const noexcept { return _elements.Length(); }
			void MenuBar::AppendMenuElement(int id, bool enable, const string & text, Windows::IMenu * menu) { InsertMenuElement(_elements.Length(), id, enable, text, menu); }
			void MenuBar::InsertMenuElement(int at, int id, bool enable, const string & text, Windows::IMenu * menu)
			{
				_menu_bar_element element;
				element._width = element._pure_width = element._org = 0;
				element._enabled = enable;
				element._flush_right = false;
				element._id = id;
				element._text = text;
				element._menu.SetRetain(menu);
				_elements.Append(element);
				Invalidate();
			}
			void MenuBar::RemoveMenuElement(int at) { _elements.Remove(at); _current = -1; Invalidate(); }
			void MenuBar::ClearMenuElements(void) { _elements.Clear(); _current = -1; Invalidate(); }
			void MenuBar::SwapMenuElements(int i, int j) { _elements.SwapAt(i, j); Invalidate(); }
			int MenuBar::FindMenuElement(int id) { for (int i = 0; i < _elements.Length(); i++) if (_elements[i]._id == id) return i; return -1; }
			int MenuBar::GetMenuElementID(int at) { return _elements[at]._id; }
			void MenuBar::SetMenuElementID(int at, int id) { _elements[at]._id = id; }
			bool MenuBar::IsMenuElementEnabled(int at) { return _elements[at]._enabled; }
			void MenuBar::EnableMenuElement(int at, bool enable) { _elements[at]._enabled = enable; Invalidate(); }
			string MenuBar::GetMenuElementText(int at) { return _elements[at]._text; }
			void MenuBar::SetMenuElementText(int at, const string & text) { _elements[at]._text = text; _elements[at]._reset(); Invalidate(); }
			Windows::IMenu * MenuBar::GetMenuElementMenu(int at) { return _elements[at]._menu; }
			void MenuBar::SetFlushRight(int index_from, bool set) { _elements[index_from]._flush_right = set; for (auto & e : _elements) e._reset(); Invalidate(); }
			void MenuBar::SetMenuElementMenu(int at, Windows::IMenu * menu) { _elements[at]._menu.SetRetain(menu); }
			Windows::IMenuItem * MenuBar::FindMenuItem(int id)
			{
				for (auto & e : _elements) if (e._menu) {
					auto item = e._menu->FindMenuItem(id);
					if (item) return item;
				}
				return 0;
			}
			void MenuBar::_menu_bar_element::_reset(void) noexcept
			{
				_pure_width = _width = _org = 0;
				_view_element_normal.SetReference(0);
				_view_element_hot.SetReference(0);
				_view_element_pressed.SetReference(0);
				_view_element_disabled.SetReference(0);
			}
		}
	}
}