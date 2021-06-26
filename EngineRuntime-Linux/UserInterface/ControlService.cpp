#include "ControlService.h"
#include "ControlServiceEx.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Accelerators
		{
			AcceleratorCommand::AcceleratorCommand(void) : KeyCode(0), Shift(false), Control(false), Alternative(false), CommandID(0), SystemCommand(false) {}
			AcceleratorCommand::AcceleratorCommand(int invoke_command, uint on_key, bool control, bool shift, bool alternative) :
				CommandID(invoke_command), KeyCode(on_key), Control(control), Shift(shift), Alternative(alternative), SystemCommand(false) {}
			AcceleratorCommand::AcceleratorCommand(AcceleratorSystemCommand command, uint on_key, bool control, bool shift, bool alternative) :
				CommandID(static_cast<int>(command)), KeyCode(on_key), Control(control), Shift(shift), Alternative(alternative), SystemCommand(true) {}
		}
		VirtualPopupStyles::VirtualPopupStyles(void) { MenuBorderWidth = 0; MenuElementDefaultSize = MenuSeparatorDefaultSize = 1; }
		VirtualPopupStyles::~VirtualPopupStyles(void) {}

		MenuArgumentProvider::MenuArgumentProvider(Windows::IMenuItem * item, Template::Controls::MenuItem * base) : Item(item), Base(base) {}
		void MenuArgumentProvider::GetArgument(const string & name, int * value) { *value = 0; }
		void MenuArgumentProvider::GetArgument(const string & name, double * value) { *value = 0.0; }
		void MenuArgumentProvider::GetArgument(const string & name, Color * value) { *value = 0; }
		void MenuArgumentProvider::GetArgument(const string & name, string * value)
		{
			if (name == L"Text") *value = Item->GetText();
			else if (name == L"RightText") *value = Item->GetSideText();
			else *value = L"";
		}
		void MenuArgumentProvider::GetArgument(const string & name, ITexture ** value)
		{
			if (name == L"ImageNormal" && Base->ImageNormal) {
				*value = Base->ImageNormal;
				(*value)->Retain();
			} else if (name == L"ImageGrayed" && Base->ImageGrayed) {
				*value = Base->ImageGrayed;
				(*value)->Retain();
			} else if (name == L"CheckedImageNormal" && Base->CheckedImageNormal) {
				*value = Base->CheckedImageNormal;
				(*value)->Retain();
			} else if (name == L"CheckedImageGrayed" && Base->CheckedImageGrayed) {
				*value = Base->CheckedImageGrayed;
				(*value)->Retain();
			} else *value = 0;
		}
		void MenuArgumentProvider::GetArgument(const string & name, IFont ** value)
		{
			if (name == L"Font" && Base->Font) {
				*value = Base->Font;
				(*value)->Retain();
			} else *value = 0;
		}

		class VirtualMenuCallback : public Windows::IMenuItemCallback, public Object
		{
			SafePointer<VirtualPopupStyles> _styles;
			SafePointer<Windows::IMenuItem> _item;
			SafePointer<Shape> _normal, _hot, _grayed, _normal_checked, _hot_checked, _grayed_checked, _arrow;
			Point _size;
		public:
			VirtualMenuCallback(VirtualPopupStyles * styles, Windows::IMenuItem * item)
			{
				_styles.SetRetain(styles);
				_item.SetRetain(item);
				_size = Point(-1, -1);
			}
			virtual ~VirtualMenuCallback(void) override {}
			virtual UI::Point MeasureMenuItem(Windows::IMenuItem * item, UI::IRenderingDevice * device) override
			{
				if (_size.x < 0) {
					if (_item->IsSeparator()) {
						_size = Point(1, _styles->MenuSeparatorDefaultSize);
					} else {
						SafePointer<ITextRenderingInfo> info = device->CreateTextRenderingInfo(_styles->MenuItemReferenceFont, _item->GetText() + _item->GetSideText(), 0, 0, 0);
						int w, h;
						info->GetExtent(w, h);
						_size = Point(w + 3 * _styles->MenuElementDefaultSize, _styles->MenuElementDefaultSize);
					}
				}
				return _size;
			}
			virtual void RenderMenuItem(Windows::IMenuItem * item, UI::IRenderingDevice * device, const UI::Box & at, bool hot_state) override
			{
				Shape ** shape;
				if (_item->IsSeparator()) {
					shape = _normal.InnerRef();
					if (!*shape && _styles->MenuSeparator.Inner()) {
						*shape = _styles->MenuSeparator->Clone();
					}
				} else {
					Template::Shape ** src;
					if (_item->IsChecked()) {
						if (_item->IsEnabled()) {
							if (hot_state) {
								src = _styles->MenuItemHotChecked.InnerRef();
								shape = _hot_checked.InnerRef();
							} else {
								src = _styles->MenuItemNormalChecked.InnerRef();
								shape = _normal_checked.InnerRef();
							}
						} else {
							src = _styles->MenuItemGrayedChecked.InnerRef();
							shape = _grayed_checked.InnerRef();
						}
					} else {
						if (_item->IsEnabled()) {
							if (hot_state) {
								src = _styles->MenuItemHot.InnerRef();
								shape = _hot.InnerRef();
							} else {
								src = _styles->MenuItemNormal.InnerRef();
								shape = _normal.InnerRef();
							}
						} else {
							src = _styles->MenuItemGrayed.InnerRef();
							shape = _grayed.InnerRef();
						}
					}
					if (!*shape && *src) {
						Template::Controls::MenuItem item;
						Reflection::PropertyZeroInitializer zero;
						item.EnumerateProperties(zero);
						item.Checked = _item->IsChecked();
						item.Disabled = !_item->IsEnabled();
						item.Font = _styles->MenuItemReferenceFont;
						item.Height = _styles->MenuElementDefaultSize;
						item.RightText = _item->GetSideText();
						item.Text = _item->GetText();
						auto provider = MenuArgumentProvider(_item, &item);
						*shape = (*src)->Initialize(&provider);
					}
				}
				if (*shape) {
					(*shape)->Render(device, at);
				}
			}
			virtual void MenuClosed(Windows::IMenuItem * item) override {}
		};
		class MenuList : public Control
		{
		private:
			SafePointer<Shape> _background;
			SafePointer<Shape> _shadow;
			SafePointer<Shape> _arrow;
		public:
			SafePointer<Windows::IMenu> Menu;
			ObjectArray<VirtualMenuCallback> Callbacks;
			Array<int> Heights;
			int TotalHeight, TotalWidth;
			int Current;
			int Mode;
			int Border;
			MenuList * Submenu;
			int SubmenuIndex;

			MenuList(void) : TotalHeight(0), TotalWidth(0), Current(0), Mode(0), Border(0), Submenu(0), SubmenuIndex(-1), Callbacks(0x20), Heights(0x20) {}
			virtual ~MenuList(void) override
			{
				for (int i = 0; i < Callbacks.Length(); i++) {
					auto item = Menu->ElementAt(i);
					if (Callbacks.ElementAt(i)) Callbacks[i].MenuClosed(item);
					else item->GetCallback()->MenuClosed(item);
				}
			}
			void Initialize(void)
			{
				auto device = GetRenderingDevice();
				TotalWidth = 0;
				TotalHeight = 0;
				for (int i = 0; i < Menu->Length(); i++) {
					auto menu_item = Menu->ElementAt(i);
					Windows::IMenuItemCallback * callback;
					if (menu_item->GetCallback()) {
						callback = menu_item->GetCallback();
						Callbacks.Append(0);
					} else {
						SafePointer<VirtualMenuCallback> vmc = new VirtualMenuCallback(GetControlSystem()->GetVirtualPopupStyles(), menu_item);
						callback = vmc;
						Callbacks.Append(vmc);
					}
					auto size = callback->MeasureMenuItem(menu_item, device);
					if (size.x > TotalWidth) TotalWidth = size.x;
					Heights << size.y;
					TotalHeight += size.y;
				}
				Current = -1;
				Border = GetControlSystem()->GetVirtualPopupStyles()->MenuBorderWidth;
				TotalHeight += 2 * Border;
				TotalWidth += 2 * Border;
			}
			void ShowSubmenu()
			{
				CloseSubmenu();
				SafePointer<MenuList> submenu = new MenuList;
				GetParent()->AddChild(submenu);
				SubmenuIndex = Current;
				Submenu = submenu;
				Submenu->Menu.SetRetain(Menu->ElementAt(Current)->GetSubmenu());
				Submenu->Initialize();
				int y = 0;
				for (int i = 0; i < Current; i++) y += Heights[i];
				Box at = ControlBoundaries;
				Box item = Box(at.Left + Border, at.Top + Border + y, at.Right - Border, at.Top + Border + y + Heights[Current]);
				Box desktop = GetControlSystem()->GetRootControl()->GetPosition();
				if (desktop.Right >= item.Right + Submenu->TotalWidth && desktop.Bottom >= item.Top + Submenu->TotalHeight) {
					Submenu->SetPosition(Box(item.Right, item.Top, item.Right + Submenu->TotalWidth, item.Top + Submenu->TotalHeight));
				} else if (desktop.Right >= item.Right + Submenu->TotalWidth) {
					Submenu->SetPosition(Box(item.Right, item.Bottom - Submenu->TotalHeight, item.Right + Submenu->TotalWidth, item.Bottom));
				} else if (desktop.Bottom >= item.Top + Submenu->TotalHeight) {
					Submenu->SetPosition(Box(item.Left - Submenu->TotalWidth, item.Top, item.Left, item.Top + Submenu->TotalHeight));
				} else {
					Submenu->SetPosition(Box(item.Left - Submenu->TotalWidth, item.Bottom - Submenu->TotalHeight, item.Left, item.Bottom));
				}
				Invalidate();
			}
			void CloseSubmenu(void)
			{
				if (Submenu) {
					SubmenuIndex = -1;
					Submenu->CloseSubmenu();
					Submenu->RemoveFromParent();
					Submenu = 0;
					Invalidate();
				}
			}
			void RenderShadow(IRenderingDevice * device, const Box & at)
			{
				if (!_shadow && GetControlSystem()->GetVirtualPopupStyles()->PopupShadow) {
					_shadow = GetControlSystem()->GetVirtualPopupStyles()->PopupShadow->Clone();
				}
				if (_shadow) _shadow->Render(device, at);
			}
			virtual void Render(IRenderingDevice * device, const Box & at) override
			{
				if (!_background && GetControlSystem()->GetVirtualPopupStyles()->MenuFrame) {
					_background = GetControlSystem()->GetVirtualPopupStyles()->MenuFrame->Clone();
				}
				if (_background) _background->Render(device, at);
				int y = 0;
				for (int i = 0; i < Menu->Length(); i++) {
					Box item = Box(at.Left + Border, at.Top + Border + y, at.Right - Border, at.Top + Border + y + Heights[i]);
					y += Heights[i];
					auto element = Menu->ElementAt(i);
					auto callback = element->GetCallback();
					if (!callback) callback = Callbacks.ElementAt(i);
					callback->RenderMenuItem(element, device, item, Current == i || SubmenuIndex == i);
					if (!element->IsSeparator()) {
						if (element->GetSubmenu()) {
							if (!_arrow && GetControlSystem()->GetVirtualPopupStyles()->SubmenuArrow) {
								_arrow = GetControlSystem()->GetVirtualPopupStyles()->SubmenuArrow->Clone();
							}
							if (_arrow) _arrow->Render(device, item);
						}
					}
				}
			}
			virtual void ResetCache(void) override
			{
				_background.SetReference(0);
				_shadow.SetReference(0);
				_arrow.SetReference(0);
			}
			virtual void CaptureChanged(bool got_capture) override
			{
				if (!got_capture && Mode == 0) {
					Current = -1;
					Invalidate();
				}
			}
			virtual void LeftButtonUp(Point at) override
			{
				if (Mode == 0) {
					if (Current >= 0) {
						if (!Menu->ElementAt(Current)->IsSeparator()) {
							auto item = Menu->ElementAt(Current);
							if (!item->GetSubmenu() && item->IsEnabled()) GetParent()->RaiseEvent(item->GetID(), ControlEvent::MenuCommand, 0);
							else if (item->IsEnabled()) {
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
						Invalidate();
						Current = -1;
					} else {
						SetCapture();
						int y = Border;
						for (int i = 0; i < Menu->Length(); i++) {
							if (at.y < y + Heights[i]) { Current = i; Invalidate(); break; }
							y += Heights[i];
						}
						if (SubmenuIndex != Current) {
							CloseSubmenu();
							if (!Menu->ElementAt(Current)->IsSeparator()) {
								auto item = Menu->ElementAt(Current);
								if (item->GetSubmenu() && item->IsEnabled()) {
									ShowSubmenu();
								}
							}
						}
					}
				}
			}
			virtual string GetControlClass(void) override { return L"MenuList"; }
		};
		class MenuHolder : public ParentControl
		{
		public:
			SafePointer<Windows::IMenu> Source;
			SafePointer<Control> Owner;
			SafePointer<Control> Focus;
			MenuList * List;
			bool Final;

			MenuHolder(void) : List(0), Final(false) {}
			virtual ~MenuHolder(void) override {}
			virtual void Render(IRenderingDevice * device, const Box & at) override
			{
				for (int i = 0; i < ChildrenCount(); i++) {
					auto & child = *Child(i);
					Box pos = child.GetPosition();
					Box rect = Box(pos.Left + at.Left, pos.Top + at.Top, pos.Right + at.Left, pos.Bottom + at.Top);
					static_cast<MenuList &>(child).RenderShadow(device, rect);
				}
				for (int i = 0; i < ChildrenCount(); i++) {
					auto & child = *Child(i);
					Box pos = child.GetPosition();
					Box rect = Box(pos.Left + at.Left, pos.Top + at.Top, pos.Right + at.Left, pos.Bottom + at.Top);
					child.Render(device, rect);
				}
			}
			virtual void LostExclusiveMode(void) override
			{
				if (!Final) {
					Final = true;
					auto system = GetControlSystem();
					system->SetFocus(Focus);
					Control * owner = Owner;
					RemoveFromParent();
					system->Invalidate();
					owner->PopupMenuCancelled();
				}
			}
			virtual void LeftButtonDown(Point at) override { if (GetControlSystem()) GetControlSystem()->SetExclusiveControl(0); }
			virtual void RightButtonDown(Point at) override { if (GetControlSystem()) GetControlSystem()->SetExclusiveControl(0); }
			virtual void ResetCache(void) override { if (GetControlSystem()) GetControlSystem()->SetExclusiveControl(0); }
			virtual void RaiseEvent(int ID, ControlEvent event, Control * sender) override
			{
				auto system = GetControlSystem();
				system->SetFocus(Focus);
				Control * owner = Owner;
				Final = true;
				system->SetExclusiveControl(0);
				RemoveFromParent();
				system->Invalidate();
				owner->RaiseEvent(ID, event, sender);
			}
			virtual string GetControlClass(void) override { return L"MenuHolder"; }
		};
		class PopupHolder : public ParentControl
		{
			SafePointer<Shape> _shadow;
			bool _visible;
		public:
			PopupHolder(void) : _visible(false) {}
			virtual ~PopupHolder(void) override {}
			virtual void Render(IRenderingDevice * device, const Box & at) override
			{
				if (!_shadow && GetControlSystem()->GetVirtualPopupStyles()->PopupShadow) {
					_shadow = GetControlSystem()->GetVirtualPopupStyles()->PopupShadow->Clone();
				}
				if (_shadow) _shadow->Render(device, at);
				ParentControl::Render(device, at);
			}
			virtual void ResetCache(void) override { _shadow.SetReference(0); ParentControl::ResetCache(); }
			virtual void Show(bool visible) override { _visible = visible; Invalidate(); }
			virtual bool IsVisible(void) override { return _visible; }
			virtual string GetControlClass(void) override { return L"PopupHolder"; }
		};

		void RunVirtualMenu(Windows::IMenu * menu, Control * for_control, Point at)
		{
			auto system = for_control->GetControlSystem();
			SafePointer<MenuHolder> holder = new MenuHolder;
			auto desktop = system->GetRootControl()->GetPosition();
			system->GetRootControl()->AddChild(holder);
			holder->SetOrder(ControlDepthOrder::SetFirst);
			holder->SetPosition(desktop);
			holder->Owner.SetRetain(for_control);
			holder->Source.SetRetain(menu);
			system->SetExclusiveControl(holder);
			holder->Focus.SetRetain(system->GetFocus());
			system->SetFocus(holder);
			SafePointer<MenuList> list = new MenuList;
			holder->AddChild(list);
			list->Menu.SetRetain(menu);
			list->Initialize();
			at = system->ConvertControlToClient(for_control, at);
			if (desktop.Right >= at.x + list->TotalWidth && desktop.Bottom >= at.y + list->TotalHeight) {
				list->SetPosition(Box(at.x, at.y, at.x + list->TotalWidth, at.y + list->TotalHeight));
			} else if (desktop.Right >= at.x + list->TotalWidth) {
				list->SetPosition(Box(at.x, at.y - list->TotalHeight + 1, at.x + list->TotalWidth, at.y + 1));
			} else if (desktop.Bottom >= at.y + list->TotalHeight) {
				list->SetPosition(Box(at.x - list->TotalWidth + 1, at.y, at.x + 1, at.y + list->TotalHeight));
			} else {
				list->SetPosition(Box(at.x - list->TotalWidth + 1, at.y - list->TotalHeight + 1, at.x + 1, at.y + 1));
			}
			holder->Invalidate();
		}
		Control * RunVirtualPopup(ControlSystem * system, const Box & at)
		{
			SafePointer<PopupHolder> holder = new PopupHolder;
			system->GetRootControl()->AddChild(holder);
			holder->SetOrder(ControlDepthOrder::SetFirst);
			holder->SetPosition(at);
			holder->Invalidate();
			return holder;
		}
	}
}