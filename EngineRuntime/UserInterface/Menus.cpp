#include "Menus.h"

#include "../PlatformDependent/NativeStation.h"

namespace Engine
{
	namespace UI
	{
		namespace Menus
		{
			namespace ArgumentService
			{
				class MenuArgumentProvider : public IArgumentProvider
				{
				public:
					MenuItem * Owner;
					MenuArgumentProvider(MenuItem * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->Text;
						else if (name == L"RightText") *value = Owner->RightText;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, ITexture ** value) override
					{
						if (name == L"ImageNormal" && Owner->ImageNormal) {
							*value = Owner->ImageNormal;
							(*value)->Retain();
						} else if (name == L"ImageGrayed" && Owner->ImageGrayed) {
							*value = Owner->ImageGrayed;
							(*value)->Retain();
						} else if (name == L"CheckedImageNormal" && Owner->CheckedImageNormal) {
							*value = Owner->CheckedImageNormal;
							(*value)->Retain();
						} else if (name == L"CheckedImageGrayed" && Owner->CheckedImageGrayed) {
							*value = Owner->CheckedImageGrayed;
							(*value)->Retain();
						} else *value = 0;
					}
					virtual void GetArgument(const string & name, IFont ** value) override
					{
						if (name == L"Font" && Owner->Font) {
							*value = Owner->Font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
			}
			namespace MenuService
			{
				class MenuList : public Window
				{
				private:
					SafePointer<Shape> _background;
					SafePointer<Shape> _arrow;
				public:
					ObjectArray<MenuElement> * Elements;
					Array<int> Height;
					int TotalHeight, TotalWidth;
					int Current;
					int Mode;
					int Border;
					MenuList * Submenu;
					int SubmenuIndex;

					MenuList(Window * parent, WindowStation * station) : Window(parent, station), Mode(0), Submenu(0), SubmenuIndex(-1) {}
					void CalculateDimensions(void)
					{
						TotalWidth = 0;
						TotalHeight = 0;
						for (int i = 0; i < Elements->Length(); i++) {
							if (Elements->ElementAt(i)->GetWidth() > TotalWidth) TotalWidth = Elements->ElementAt(i)->GetWidth();
							int height = Elements->ElementAt(i)->GetHeight();
							Height << height;
							TotalHeight += height;
						}
						Current = -1;
						Border = GetStation()->GetVisualStyles().MenuBorder;
						TotalHeight += 2 * Border;
						TotalWidth += 2 * Border;
					}
					void ShowSubmenu()
					{
						CloseSubmenu();
						SubmenuIndex = Current;
						Submenu = GetStation()->CreateWindow<MenuService::MenuList>(GetParent());
						Submenu->Elements = &static_cast<MenuItem *>(Elements->ElementAt(Current))->Children;
						Submenu->CalculateDimensions();
						int y = 0;
						for (int i = 0; i < Current; i++) y += Height[i];
						Box at = WindowPosition;
						Box item = Box(at.Left + Border, at.Top + Border + y, at.Right - Border, at.Top + Border + y + Height[Current]);
						Box desktop = GetStation()->GetBox();
						if (desktop.Right >= item.Right + Submenu->TotalWidth && desktop.Bottom >= item.Top + Submenu->TotalHeight) {
							Submenu->SetPosition(Box(item.Right, item.Top, item.Right + Submenu->TotalWidth, item.Top + Submenu->TotalHeight));
						} else if (desktop.Right >= item.Right + Submenu->TotalWidth) {
							Submenu->SetPosition(Box(item.Right, item.Bottom - Submenu->TotalHeight, item.Right + Submenu->TotalWidth, item.Bottom));
						} else if (desktop.Bottom >= item.Top + Submenu->TotalHeight) {
							Submenu->SetPosition(Box(item.Left - Submenu->TotalWidth, item.Top, item.Left, item.Top + Submenu->TotalHeight));
						} else {
							Submenu->SetPosition(Box(item.Left - Submenu->TotalWidth, item.Bottom - Submenu->TotalHeight, item.Left, item.Bottom));
						}
					}
					void CloseSubmenu(void)
					{
						if (Submenu) {
							SubmenuIndex = -1;
							Submenu->CloseSubmenu();
							Submenu->Destroy();
							Submenu = 0;
						}
					}
					virtual void Render(const Box & at) override
					{
						auto Device = GetStation()->GetRenderingDevice();
						if (!_background && GetStation()->GetVisualStyles().MenuBackground) {
							auto provider = ZeroArgumentProvider();
							_background.SetReference(GetStation()->GetVisualStyles().MenuBackground->Initialize(&provider));
						}
						if (_background) _background->Render(Device, at);
						int y = 0;
						for (int i = 0; i < Elements->Length(); i++) {
							Box item = Box(at.Left + Border, at.Top + Border + y, at.Right - Border, at.Top + Border + y + Height[i]);
							y += Height[i];
							auto element = Elements->ElementAt(i);
							element->Render(item, Current == i || SubmenuIndex == i);
							if (!element->IsSeparator()) {
								if (static_cast<MenuItem *>(element)->Children.Length()) {
									if (!_arrow && GetStation()->GetVisualStyles().MenuArrow) {
										auto provider = ZeroArgumentProvider();
										_arrow.SetReference(GetStation()->GetVisualStyles().MenuArrow->Initialize(&provider));
									}
									if (_arrow) _arrow->Render(Device, item);
								}
							}
						}
					}
					virtual void ResetCache(void) override
					{
						_background.SetReference(0);
						_arrow.SetReference(0);
					}
					virtual void CaptureChanged(bool got_capture) override
					{
						if (!got_capture && Mode == 0) Current = -1;
					}
					virtual void LeftButtonUp(Point at) override
					{
						if (Mode == 0) {
							if (Current >= 0) {
								if (!Elements->ElementAt(Current)->IsSeparator()) {
									auto item = static_cast<MenuItem *>(Elements->ElementAt(Current));
									if (!item->Children.Length() && !item->Disabled) GetParent()->RaiseEvent(item->ID, Event::MenuCommand, 0);
									else if (!item->Disabled) {
										Mode = 1;
										ReleaseCapture();
										ShowSubmenu();
									}
								}
							}
						} else {
							Mode = 0;
							CloseSubmenu();
							MouseMove(at);
						}
					}
					virtual void RightButtonUp(Point at) override { LeftButtonUp(at); }
					virtual void MouseMove(Point at) override
					{
						if (Mode == 0) {
							if (at.x < Border || at.y < Border || at.x >= TotalWidth - Border || at.y >= TotalHeight - Border) {
								ReleaseCapture();
								Current = -1;
							} else {
								SetCapture();
								int y = Border;
								for (int i = 0; i < Elements->Length(); i++) {
									if (at.y < y + Height[i]) { Current = i; break; }
									y += Height[i];
								}
								if (SubmenuIndex != Current) {
									CloseSubmenu();
									if (!Elements->ElementAt(Current)->IsSeparator()) {
										auto item = static_cast<MenuItem *>(Elements->ElementAt(Current));
										if (item->Children.Length() && !item->Disabled) {
											ShowSubmenu();
										}
									}
								}
							}
						}
					}
					virtual string GetControlClass(void) override { return L"MenuList"; }
				};
				class MenuHolder : public ParentWindow
				{
				public:
					SafePointer<Menu> Source;
					SafePointer<Window> Owner;
					SafePointer<Window> Focus;
					MenuList * List;
					bool Final = false;

					MenuHolder(Window * parent, WindowStation * station) : ParentWindow(parent, station) {}
					~MenuHolder(void) override { for (int i = 0; i < Source->Children.Length(); i++) Source->Children[i].Shutdown(); }

					virtual void Render(const Box & at) override
					{
						auto Device = GetStation()->GetRenderingDevice();
						for (int i = 0; i < ChildrenCount(); i++) {
							auto & child = *Child(i);
							Box pos = child.GetPosition();
							Box rect = Box(pos.Left + at.Left, pos.Top + at.Top, pos.Right + at.Left, pos.Bottom + at.Top);
							child.Render(rect);
						}
					}
					virtual void LostExclusiveMode(void) override { if (!Final) { GetStation()->SetFocus(Focus); Window * owner = Owner; Destroy(); owner->PopupMenuCancelled(); } }
					virtual void LeftButtonDown(Point at) override { GetStation()->SetExclusiveWindow(0); }
					virtual void RightButtonDown(Point at) override { GetStation()->SetExclusiveWindow(0); }
					virtual void ResetCache(void) override { for (int i = 0; i < Source->Children.Length(); i++) Source->Children[i].WakeUp(GetStation()->GetRenderingDevice()); Window::ResetCache(); }
					virtual void RaiseEvent(int ID, Event event, Window * sender) override
					{
						GetStation()->SetFocus(Focus);
						Window * owner = Owner;
						Final = true;
						GetStation()->SetExclusiveWindow(0);
						Destroy();
						owner->RaiseEvent(ID, event, sender);
					}
					virtual string GetControlClass(void) override { return L"MenuHolder"; }
				};
			}

			MenuItem::MenuItem(void) : _width(-1), Children(0x10)
			{
				ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer);
			}
			MenuItem::MenuItem(Template::ControlTemplate * Template) : _width(-1), Children(0x10)
			{
				if (Template->Properties->GetTemplateClass() != L"MenuItem") throw InvalidArgumentException();
				static_cast<Template::Controls::MenuItem &>(*this) = static_cast<Template::Controls::MenuItem &>(*Template->Properties);
				for (int i = 0; i < Template->Children.Length(); i++) {
					if (Template->Children[i].Properties->GetTemplateClass() == L"MenuItem") Children.Append(new MenuItem(Template->Children.ElementAt(i)));
					else if (Template->Children[i].Properties->GetTemplateClass() == L"MenuSeparator") Children.Append(new MenuSeparator(Template->Children.ElementAt(i)));
					else throw InvalidArgumentException();
				}
			}
			MenuItem::~MenuItem(void) {}
			int MenuItem::GetHeight(void) const { return Height; }
			int MenuItem::GetWidth(void) const
			{
				if (_width == -1) {
					if (!_device || !Font) return 0;
					SafePointer<ITextRenderingInfo> text = _device->CreateTextRenderingInfo(Font, Text, 0, 0, 0);
					SafePointer<ITextRenderingInfo> right = _device->CreateTextRenderingInfo(Font, RightText, 0, 0, 0);
					int x1, x2, y;
					text->GetExtent(x1, y);
					right->GetExtent(x2, y);
					_width = 3 * Height + x1 + x2;
				}
				return _width;
			}
			void MenuItem::Render(const Box & at, bool highlighted)
			{
				Shape ** _view;
				Template::Shape * View;
				if (Disabled) {
					if (Checked) {
						_view = _disabled_checked.InnerRef();
						View = ViewCheckedDisabled;
					} else {
						_view = _disabled.InnerRef();
						View = ViewDisabled;
					}
				} else {
					if (highlighted) {
						if (Checked) {
							_view = _hot_checked.InnerRef();
							View = ViewCheckedHot;
						} else {
							_view = _hot.InnerRef();
							View = ViewHot;
						}
					} else {
						if (Checked) {
							_view = _normal_checked.InnerRef();
							View = ViewCheckedNormal;
						} else {
							_view = _normal.InnerRef();
							View = ViewNormal;
						}
					}
				}
				if (!(*_view)) {
					if (View) {
						auto provider = ArgumentService::MenuArgumentProvider(this);
						*_view = View->Initialize(&provider);
						(*_view)->Render(_device, at);
					}
				} else (*_view)->Render(_device, at);
			}
			void MenuItem::WakeUp(IRenderingDevice * Device)
			{
				_width = -1;
				_normal.SetReference(0);
				_normal_checked.SetReference(0);
				_disabled.SetReference(0);
				_disabled_checked.SetReference(0);
				_hot.SetReference(0);
				_hot_checked.SetReference(0);
				_device.SetRetain(Device);
				for (int i = 0; i < Children.Length(); i++) Children[i].WakeUp(Device);
			}
			void MenuItem::Shutdown(void)
			{
				_width = -1;
				_device.SetReference(0);
				_normal.SetReference(0);
				_normal_checked.SetReference(0);
				_disabled.SetReference(0);
				_disabled_checked.SetReference(0);
				_hot.SetReference(0);
				_hot_checked.SetReference(0);
				for (int i = 0; i < Children.Length(); i++) Children[i].Shutdown();
			}
			bool MenuItem::IsSeparator(void) const { return false; }
			MenuItem * MenuItem::FindChild(int _ID)
			{
				if (!_ID) return 0;
				if (ID == _ID) return this;
				else for (int i = 0; i < Children.Length(); i++) {
					auto result = Children[i].FindChild(_ID);
					if (result) return result;
				}
				return 0;
			}
			IRenderingDevice * MenuItem::GetRenderingDevice(void) { return _device; }

			MenuSeparator::MenuSeparator(void)
			{
				ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer);
			}
			MenuSeparator::MenuSeparator(Template::ControlTemplate * Template)
			{
				if (Template->Properties->GetTemplateClass() != L"MenuSeparator") throw InvalidArgumentException();
				static_cast<Template::Controls::MenuSeparator &>(*this) = static_cast<Template::Controls::MenuSeparator &>(*Template->Properties);
			}
			MenuSeparator::~MenuSeparator(void) {}
			int MenuSeparator::GetHeight(void) const { return Height; }
			int MenuSeparator::GetWidth(void) const { return 0; }
			void MenuSeparator::Render(const Box & at, bool highlighted)
			{
				if (!_view) {
					if (View) {
						auto provider = ZeroArgumentProvider();
						_view.SetReference(View->Initialize(&provider));
						_view->Render(_device, at);
					}
				} else _view->Render(_device, at);
			}
			void MenuSeparator::WakeUp(IRenderingDevice * Device)
			{
				Shutdown();
				_device.SetRetain(Device);
			}
			void MenuSeparator::Shutdown(void)
			{
				_device.SetReference(0);
				_view.SetReference(0);
			}
			bool MenuSeparator::IsSeparator(void) const { return true; }
			MenuItem * MenuSeparator::FindChild(int ID) { return 0; }
			IRenderingDevice * MenuSeparator::GetRenderingDevice(void) { return _device; }

			Menu::Menu(void) : Children(0x10) {}
			Menu::Menu(Template::ControlTemplate * MenuTemplate) : Children(0x10)
			{
				if (MenuTemplate->Properties->GetTemplateClass() != L"PopupMenu") throw InvalidArgumentException();
				for (int i = 0; i < MenuTemplate->Children.Length(); i++) {
					if (MenuTemplate->Children[i].Properties->GetTemplateClass() == L"MenuItem") Children.Append(new MenuItem(MenuTemplate->Children.ElementAt(i)));
					else if (MenuTemplate->Children[i].Properties->GetTemplateClass() == L"MenuSeparator") Children.Append(new MenuSeparator(MenuTemplate->Children.ElementAt(i)));
					else throw InvalidArgumentException();
				}
			}
			Menu::~Menu(void) {}
			MenuItem * Menu::FindChild(int ID)
			{
				if (ID == 0) return 0;
				for (int i = 0; i < Children.Length(); i++) {
					auto result = Children[i].FindChild(ID);
					if (result) return result;
				}
				return 0;
			}
			void Menu::CheckRange(int RangeMin, int RangeMax, int Element)
			{
				for (int i = RangeMin; i <= RangeMax; i++) {
					auto element = FindChild(i);
					if (element) element->Checked = Element == i;
				}
			}
			void Menu::RunPopup(Window * owner, Point at)
			{
				if (!owner) throw InvalidArgumentException();
				auto station = owner->GetStation();
				if (station->IsNativeStationWrapper()) {
					int command = NativeWindows::RunMenuPopup(this, owner, at);
					if (command) owner->RaiseEvent(command, Window::Event::MenuCommand, 0);
					else owner->PopupMenuCancelled();
				} else {
					auto holder = station->CreateWindow<MenuService::MenuHolder>(0);
					Box desktop = station->GetBox();
					holder->SetPosition(desktop);
					holder->Owner.SetRetain(owner);
					holder->Source.SetRetain(this);
					for (int i = 0; i < Children.Length(); i++) Children[i].WakeUp(owner->GetStation()->GetRenderingDevice());
					station->SetExclusiveWindow(holder);
					holder->Focus.SetRetain(station->GetFocus());
					station->SetFocus(holder);
					auto menu = station->CreateWindow<MenuService::MenuList>(holder);
					menu->Elements = &Children;
					menu->CalculateDimensions();
					if (desktop.Right >= at.x + menu->TotalWidth && desktop.Bottom >= at.y + menu->TotalHeight) {
						menu->SetPosition(Box(at.x, at.y, at.x + menu->TotalWidth, at.y + menu->TotalHeight));
					} else if (desktop.Right >= at.x + menu->TotalWidth) {
						menu->SetPosition(Box(at.x, at.y - menu->TotalHeight + 1, at.x + menu->TotalWidth, at.y + 1));
					} else if (desktop.Bottom >= at.y + menu->TotalHeight) {
						menu->SetPosition(Box(at.x - menu->TotalWidth + 1, at.y, at.x + 1, at.y + menu->TotalHeight));
					} else {
						menu->SetPosition(Box(at.x - menu->TotalWidth + 1, at.y - menu->TotalHeight + 1, at.x + 1, at.y + 1));
					}
				}
			}
			void Menu::RunPopup(Window * owner)
			{
				if (!owner) throw InvalidArgumentException();
				RunPopup(owner, owner->GetStation()->GetCursorPos());
			}
		}
	}
}