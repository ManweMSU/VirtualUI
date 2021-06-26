#include "ButtonControls.h"

#include "GroupControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace ArgumentService
			{
				class ButtonArgumentProvider : public IArgumentProvider
				{
				public:
					Button * Owner;
					ButtonArgumentProvider(Button * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->Text;
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
				class CheckBoxArgumentProvider : public IArgumentProvider
				{
				public:
					CheckBox * Owner;
					CheckBoxArgumentProvider(CheckBox * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->Text;
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
				class RadioButtonArgumentProvider : public IArgumentProvider
				{
				public:
					RadioButton * Owner;
					RadioButtonArgumentProvider(RadioButton * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->Text;
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
				class ToolButtonPartArgumentProvider : public IArgumentProvider
				{
				public:
					ToolButtonPart * Owner;
					ToolButtonPartArgumentProvider(ToolButtonPart * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->Text;
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

			Button::Button(void) : _state(0) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			Button::Button(Template::ControlTemplate * Template) : _state(0)
			{
				if (Template->Properties->GetTemplateClass() != L"Button") throw InvalidArgumentException();
				static_cast<Template::Controls::Button &>(*this) = static_cast<Template::Controls::Button &>(*Template->Properties);
			}
			Button::~Button(void) {}
			void Button::Render(IRenderingDevice * device, const Box & at)
			{
				Shape ** shape = 0;
				Template::Shape * temp = 0;
				if (IsEnabled()) {
					if ((_state & 0xF) == 2) {
						shape = _pressed.InnerRef();
						temp = ViewPressed.Inner();
					} else if ((_state & 0xF) == 1) {
						shape = _hot.InnerRef();
						temp = ViewHot.Inner();
					} else {
						if (GetFocus() == this) {
							shape = _focused.InnerRef();
							temp = ViewFocused.Inner();
						} else {
							shape = _normal.InnerRef();
							temp = ViewNormal.Inner();
						}
					}
				} else {
					shape = _disabled.InnerRef();
					temp = ViewDisabled.Inner();
				}
				if (!(*shape) && temp) {
					auto args = ArgumentService::ButtonArgumentProvider(this);
					*shape = temp->Initialize(&args);
				}
				if (*shape) (*shape)->Render(device, at);
			}
			void Button::ResetCache(void)
			{
				_normal.SetReference(0);
				_disabled.SetReference(0);
				_focused.SetReference(0);
				_hot.SetReference(0);
				_pressed.SetReference(0);
			}
			void Button::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; Invalidate(); }
			bool Button::IsEnabled(void) { return !Disabled; }
			void Button::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; Invalidate(); }
			bool Button::IsVisible(void) { return !Invisible; }
			bool Button::IsTabStop(void) { return true; }
			void Button::SetID(int _ID) { ID = _ID; }
			int Button::GetID(void) { return ID; }
			Control * Button::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void Button::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle Button::GetRectangle(void) { return ControlPosition; }
			void Button::SetText(const string & text) { Text = text; ResetCache(); Invalidate(); }
			string Button::GetText(void) { return Text; }
			void Button::SetNormalImage(ITexture * Image) { ImageNormal.SetRetain(Image); ResetCache(); Invalidate(); }
			ITexture * Button::GetNormalImage(void) { return ImageNormal; }
			void Button::SetGrayedImage(ITexture * Image) { ImageGrayed.SetRetain(Image); ResetCache(); Invalidate(); }
			ITexture * Button::GetGrayedImage(void) { return ImageGrayed; }
			void Button::FocusChanged(bool got_focus) { if (!got_focus && _state == 0x12) _state = 0; Invalidate(); }
			void Button::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; Invalidate(); } }
			void Button::LeftButtonDown(Point at)
			{
				SetFocus();
				if (_state == 1 || _state == 0 || (_state & 0x10)) {
					SetCapture();
					_state = 2;
					Invalidate();
				}
			}
			void Button::LeftButtonUp(Point at)
			{
				if (_state == 2) {
					if (IsHovered()) {
						_state = 1;
						Invalidate();
						GetParent()->RaiseEvent(ID, ControlEvent::Command, this);
					} else ReleaseCapture();
				} else ReleaseCapture();
			}
			void Button::MouseMove(Point at)
			{
				if (_state == 0) {
					if (IsHovered()) {
						_state = 1;
						SetCapture();
						Invalidate();
					}
				} else if (_state == 1) {
					if (!IsHovered()) {
						_state = 0;
						ReleaseCapture();
					}
				}
			}
			bool Button::KeyDown(int key_code)
			{
				if (key_code == L' ') {
					if (_state != 0x2 && _state != 0x12) {
						_state = 0x12;
						Invalidate();
					}
					return true;
				}
				return false;
			}
			void Button::KeyUp(int key_code)
			{
				if (key_code == L' ') {
					if (_state == 0x12) {
						_state = 0x0;
						Invalidate();
						GetParent()->RaiseEvent(ID, ControlEvent::Command, this);
					}
				}
			}
			string Button::GetControlClass(void) { return L"Button"; }

			CheckBox::CheckBox(void) : _state(0) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			CheckBox::CheckBox(Template::ControlTemplate * Template) : _state(0)
			{
				if (Template->Properties->GetTemplateClass() != L"CheckBox") throw InvalidArgumentException();
				static_cast<Template::Controls::CheckBox &>(*this) = static_cast<Template::Controls::CheckBox &>(*Template->Properties);
			}
			CheckBox::~CheckBox(void) {}
			void CheckBox::Render(IRenderingDevice * device, const Box & at)
			{
				Shape ** shape = 0;
				Template::Shape * temp = 0;
				if (IsEnabled()) {
					if ((_state & 0xF) == 2) {
						if (Checked) {
							shape = _pressed_checked.InnerRef();
							temp = ViewPressedChecked.Inner();
						} else {
							shape = _pressed.InnerRef();
							temp = ViewPressed.Inner();
						}
					} else if ((_state & 0xF) == 1) {
						if (Checked) {
							shape = _hot_checked.InnerRef();
							temp = ViewHotChecked.Inner();
						} else {
							shape = _hot.InnerRef();
							temp = ViewHot.Inner();
						}
					} else {
						if (GetFocus() == this) {
							if (Checked) {
								shape = _focused_checked.InnerRef();
								temp = ViewFocusedChecked.Inner();
							} else {
								shape = _focused.InnerRef();
								temp = ViewFocused.Inner();
							}
						} else {
							if (Checked) {
								shape = _normal_checked.InnerRef();
								temp = ViewNormalChecked.Inner();
							} else {
								shape = _normal.InnerRef();
								temp = ViewNormal.Inner();
							}
						}
					}
				} else {
					if (Checked) {
						shape = _disabled_checked.InnerRef();
						temp = ViewDisabledChecked.Inner();
					} else {
						shape = _disabled.InnerRef();
						temp = ViewDisabled.Inner();
					}
				}
				if (!(*shape) && temp) {
					auto args = ArgumentService::CheckBoxArgumentProvider(this);
					*shape = temp->Initialize(&args);
				}
				if (*shape) (*shape)->Render(device, at);
			}
			void CheckBox::ResetCache(void)
			{
				_normal.SetReference(0);
				_disabled.SetReference(0);
				_focused.SetReference(0);
				_hot.SetReference(0);
				_pressed.SetReference(0);
				_normal_checked.SetReference(0);
				_disabled_checked.SetReference(0);
				_focused_checked.SetReference(0);
				_hot_checked.SetReference(0);
				_pressed_checked.SetReference(0);
			}
			void CheckBox::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; Invalidate(); }
			bool CheckBox::IsEnabled(void) { return !Disabled; }
			void CheckBox::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; Invalidate(); }
			bool CheckBox::IsVisible(void) { return !Invisible; }
			bool CheckBox::IsTabStop(void) { return true; }
			void CheckBox::SetID(int _ID) { ID = _ID; }
			int CheckBox::GetID(void) { return ID; }
			Control * CheckBox::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void CheckBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle CheckBox::GetRectangle(void) { return ControlPosition; }
			void CheckBox::SetText(const string & text) { Text = text; ResetCache(); Invalidate(); }
			string CheckBox::GetText(void) { return Text; }
			void CheckBox::FocusChanged(bool got_focus) { if (!got_focus && _state == 0x12) _state = 0; Invalidate(); }
			void CheckBox::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; Invalidate(); } }
			void CheckBox::LeftButtonDown(Point at)
			{
				SetFocus();
				if (_state == 1 || _state == 0 || (_state & 0x10)) {
					SetCapture();
					_state = 2;
					Invalidate();
				}
			}
			void CheckBox::LeftButtonUp(Point at)
			{
				if (_state == 2) {
					if (IsHovered()) {
						_state = 1;
						Checked ^= true;
						Invalidate();
						GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
					} else ReleaseCapture();
				} else ReleaseCapture();
			}
			void CheckBox::MouseMove(Point at)
			{
				if (_state == 0) {
					if (IsHovered()) {
						_state = 1;
						SetCapture();
						Invalidate();
					}
				} else if (_state == 1) {
					if (!IsHovered()) {
						_state = 0;
						ReleaseCapture();
					}
				}
			}
			bool CheckBox::KeyDown(int key_code)
			{
				if (key_code == L' ') {
					if (_state != 0x2 && _state != 0x12) {
						_state = 0x12;
						Invalidate();
					}
					return true;
				}
				return false;
			}
			void CheckBox::KeyUp(int key_code)
			{
				if (key_code == L' ') {
					if (_state == 0x12) {
						_state = 0x0;
						Checked ^= true;
						Invalidate();
						GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
					}
				}
			}
			string CheckBox::GetControlClass(void) { return L"CheckBox"; }
			void CheckBox::Check(bool check) { if (check != Checked) Invalidate(); Checked = check; }
			bool CheckBox::IsChecked(void) { return Checked; }

			RadioButton::RadioButton(void) : _state(0) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			RadioButton::RadioButton(Template::ControlTemplate * Template) : _state(0)
			{
				if (Template->Properties->GetTemplateClass() != L"RadioButton") throw InvalidArgumentException();
				static_cast<Template::Controls::RadioButton &>(*this) = static_cast<Template::Controls::RadioButton &>(*Template->Properties);
			}
			RadioButton::~RadioButton(void) {}
			void RadioButton::Render(IRenderingDevice * device, const Box & at)
			{
				Shape ** shape = 0;
				Template::Shape * temp = 0;
				if (IsEnabled()) {
					if ((_state & 0xF) == 2) {
						if (Checked) {
							shape = _pressed_checked.InnerRef();
							temp = ViewPressedChecked.Inner();
						} else {
							shape = _pressed.InnerRef();
							temp = ViewPressed.Inner();
						}
					} else if ((_state & 0xF) == 1) {
						if (Checked) {
							shape = _hot_checked.InnerRef();
							temp = ViewHotChecked.Inner();
						} else {
							shape = _hot.InnerRef();
							temp = ViewHot.Inner();
						}
					} else {
						if (GetFocus() == this) {
							if (Checked) {
								shape = _focused_checked.InnerRef();
								temp = ViewFocusedChecked.Inner();
							} else {
								shape = _focused.InnerRef();
								temp = ViewFocused.Inner();
							}
						} else {
							if (Checked) {
								shape = _normal_checked.InnerRef();
								temp = ViewNormalChecked.Inner();
							} else {
								shape = _normal.InnerRef();
								temp = ViewNormal.Inner();
							}
						}
					}
				} else {
					if (Checked) {
						shape = _disabled_checked.InnerRef();
						temp = ViewDisabledChecked.Inner();
					} else {
						shape = _disabled.InnerRef();
						temp = ViewDisabled.Inner();
					}
				}
				if (!(*shape) && temp) {
					auto args = ArgumentService::RadioButtonArgumentProvider(this);
					*shape = temp->Initialize(&args);
				}
				if (*shape) (*shape)->Render(device, at);
			}
			void RadioButton::ResetCache(void)
			{
				_normal.SetReference(0);
				_disabled.SetReference(0);
				_focused.SetReference(0);
				_hot.SetReference(0);
				_pressed.SetReference(0);
				_normal_checked.SetReference(0);
				_disabled_checked.SetReference(0);
				_focused_checked.SetReference(0);
				_hot_checked.SetReference(0);
				_pressed_checked.SetReference(0);
			}
			void RadioButton::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; Invalidate(); }
			bool RadioButton::IsEnabled(void) { return !Disabled; }
			void RadioButton::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; Invalidate(); }
			bool RadioButton::IsVisible(void) { return !Invisible; }
			bool RadioButton::IsTabStop(void) { return true; }
			void RadioButton::SetID(int _ID) { ID = _ID; }
			int RadioButton::GetID(void) { return ID; }
			Control * RadioButton::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void RadioButton::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle RadioButton::GetRectangle(void) { return ControlPosition; }
			void RadioButton::SetText(const string & text) { Text = text; ResetCache(); Invalidate(); }
			string RadioButton::GetText(void) { return Text; }
			void RadioButton::FocusChanged(bool got_focus) { if (!got_focus && _state == 0x12) _state = 0; Invalidate(); }
			void RadioButton::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; Invalidate(); } }
			void RadioButton::LeftButtonDown(Point at)
			{
				SetFocus();
				if (_state == 1 || _state == 0 || (_state & 0x10)) {
					SetCapture();
					_state = 2;
					Invalidate();
				}
			}
			void RadioButton::LeftButtonUp(Point at)
			{
				if (_state == 2) {
					if (IsHovered()) {
						_state = 1;
						Invalidate();
						if (!Checked) {
							static_cast<RadioButtonGroup *>(GetParent())->CheckRadioButton(this);
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					} else ReleaseCapture();
				} else ReleaseCapture();
			}
			void RadioButton::MouseMove(Point at)
			{
				if (_state == 0) {
					if (IsHovered()) {
						_state = 1;
						SetCapture();
						Invalidate();
					}
				} else if (_state == 1) {
					if (!IsHovered()) {
						_state = 0;
						ReleaseCapture();
					}
				}
			}
			bool RadioButton::KeyDown(int key_code)
			{
				if (key_code == L' ') {
					if (_state != 0x2 && _state != 0x12) {
						_state = 0x12;
						Invalidate();
					}
					return true;
				}
				return false;
			}
			void RadioButton::KeyUp(int key_code)
			{
				if (key_code == L' ') {
					if (_state == 0x12) {
						_state = 0x0;
						Invalidate();
						if (!Checked) {
							static_cast<RadioButtonGroup *>(GetParent())->CheckRadioButton(this);
							GetParent()->RaiseEvent(ID, ControlEvent::ValueChange, this);
						}
					}
				}
			}
			string RadioButton::GetControlClass(void) { return L"RadioButton"; }
			void RadioButton::Check(bool check) { if (GetParent()) static_cast<RadioButtonGroup *>(GetParent())->CheckRadioButton(this); else Checked = check; }
			bool RadioButton::IsChecked(void) { return Checked; }

			ToolButton::ToolButton(void) : _state(0) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ToolButton::ToolButton(Template::ControlTemplate * Template) : _state(0)
			{
				if (Template->Properties->GetTemplateClass() != L"ToolButton") throw InvalidArgumentException();
				static_cast<Template::Controls::ToolButton &>(*this) = static_cast<Template::Controls::ToolButton &>(*Template->Properties);
				for (int i = 0; i < Template->Children.Length(); i++) {
					SafePointer<Control> child = new ToolButtonPart(&Template->Children[i]);
					AddChild(child);
				}
			}
			ToolButton::~ToolButton(void) {}
			void ToolButton::Render(IRenderingDevice * device, const Box & at)
			{
				Shape ** shape = 0;
				Template::Shape * temp = 0;
				if (IsEnabled()) {
					if ((_state & 0xF) == 2) {
						shape = _pressed.InnerRef();
						temp = ViewPressed.Inner();
					} else if ((_state & 0xF) == 1) {
						shape = _hot.InnerRef();
						temp = ViewHot.Inner();
					} else {
						shape = _normal.InnerRef();
						temp = ViewNormal.Inner();
					}
				} else {
					shape = _disabled.InnerRef();
					temp = ViewDisabled.Inner();
				}
				if (!(*shape) && temp) {
					auto args = ZeroArgumentProvider();
					*shape = temp->Initialize(&args);
				}
				if (*shape) (*shape)->Render(device, at);
				ParentControl::Render(device, at);
			}
			void ToolButton::ResetCache(void)
			{
				_normal.SetReference(0);
				_disabled.SetReference(0);
				_hot.SetReference(0);
				_pressed.SetReference(0);
				ParentControl::ResetCache();
			}
			void ToolButton::Enable(bool enable)
			{
				Disabled = !enable;
				if (Disabled) {
					_state = 0;
					for (int i = 0; i < ChildrenCount(); i++) static_cast<ToolButtonPart *>(Child(i))->_state = 0;
				}
				Invalidate();
			}
			bool ToolButton::IsEnabled(void) { return !Disabled; }
			void ToolButton::Show(bool visible)
			{
				Invisible = !visible;
				if (Invisible) {
					_state = 0;
					for (int i = 0; i < ChildrenCount(); i++) static_cast<ToolButtonPart *>(Child(i))->_state = 0;
				}
				Invalidate();
			}
			bool ToolButton::IsVisible(void) { return !Invisible; }
			void ToolButton::SetID(int _ID) { ID = _ID; }
			int ToolButton::GetID(void) { return ID; }
			void ToolButton::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle ToolButton::GetRectangle(void) { return ControlPosition; }
			void ToolButton::CaptureChanged(bool got_capture) { if (!got_capture) { if (_state) Invalidate(); _state = 0; } }
			void ToolButton::MouseMove(Point at)
			{
				if (_state == 0) {
					if (IsHovered()) { _state = 1; SetCapture(); Invalidate(); }
				} else if (_state == 1) {
					if (!IsHovered()) { ReleaseCapture(); }
				}
			}
			string ToolButton::GetControlClass(void) { return L"ToolButton"; }

			ToolButtonPart::ToolButtonPart(void) : _callback(0), _state(0) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ToolButtonPart::ToolButtonPart(Template::ControlTemplate * Template) : _callback(0), _state(0)
			{
				if (Template->Properties->GetTemplateClass() != L"ToolButtonPart") throw InvalidArgumentException();
				static_cast<Template::Controls::ToolButtonPart &>(*this) = static_cast<Template::Controls::ToolButtonPart &>(*Template->Properties);
				if (DropDownMenu) _menu.SetReference(CreateMenu(DropDownMenu));
			}
			ToolButtonPart::~ToolButtonPart(void) {}
			void ToolButtonPart::Render(IRenderingDevice * device, const Box & at)
			{
				int pstate = static_cast<ToolButton *>(GetParent())->_state & 0xF;
				Shape ** shape = 0;
				Template::Shape * temp = 0;
				if (IsEnabled() && GetParent()->IsEnabled()) {
					if ((_state & 0xF) == 2) {
						if (Checked) {
							shape = _pressed_checked.InnerRef();
							temp = ViewCheckedPressed.Inner();
						} else {
							shape = _pressed.InnerRef();
							temp = ViewPressed.Inner();
						}
					} else if ((_state & 0xF) == 1) {
						if (Checked) {
							shape = _hot_checked.InnerRef();
							temp = ViewCheckedHot.Inner();
						} else {
							shape = _hot.InnerRef();
							temp = ViewHot.Inner();
						}
					} else {
						if (pstate) {
							if (Checked) {
								shape = _normal_semihot_checked.InnerRef();
								temp = ViewCheckedFramedNormal.Inner();
							} else {
								shape = _normal_semihot.InnerRef();
								temp = ViewFramedNormal.Inner();
							}
						} else {
							if (Checked) {
								shape = _normal_checked.InnerRef();
								temp = ViewCheckedNormal.Inner();
							} else {
								shape = _normal.InnerRef();
								temp = ViewNormal.Inner();
							}
						}
					}
				} else {
					if (pstate) {
						if (Checked) {
							shape = _disabled_semihot_checked.InnerRef();
							temp = ViewCheckedFramedDisabled.Inner();
						} else {
							shape = _disabled_semihot.InnerRef();
							temp = ViewFramedDisabled.Inner();
						}
					} else {
						if (Checked) {
							shape = _disabled_checked.InnerRef();
							temp = ViewCheckedDisabled.Inner();
						} else {
							shape = _disabled.InnerRef();
							temp = ViewDisabled.Inner();
						}
					}
				}
				if (!(*shape) && temp) {
					auto args = ArgumentService::ToolButtonPartArgumentProvider(this);
					*shape = temp->Initialize(&args);
				}
				if (*shape) (*shape)->Render(device, at);
			}
			void ToolButtonPart::ResetCache(void)
			{
				_normal.SetReference(0);
				_disabled.SetReference(0);
				_normal_semihot.SetReference(0);
				_disabled_semihot.SetReference(0);
				_hot.SetReference(0);
				_pressed.SetReference(0);
				_normal_checked.SetReference(0);
				_disabled_checked.SetReference(0);
				_normal_semihot_checked.SetReference(0);
				_disabled_semihot_checked.SetReference(0);
				_hot_checked.SetReference(0);
				_pressed_checked.SetReference(0);
			}
			void ToolButtonPart::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; Invalidate(); }
			bool ToolButtonPart::IsEnabled(void) { return !Disabled; }
			void ToolButtonPart::SetID(int _ID) { ID = _ID; }
			int ToolButtonPart::GetID(void) { return ID; }
			Control * ToolButtonPart::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void ToolButtonPart::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle ToolButtonPart::GetRectangle(void) { return ControlPosition; }
			void ToolButtonPart::SetText(const string & text) { Text = text; ResetCache(); Invalidate(); }
			string ToolButtonPart::GetText(void) { return Text; }
			void ToolButtonPart::RaiseEvent(int ID, ControlEvent event, Control * sender)
			{
				if (event == ControlEvent::MenuCommand) {
					_state = 0;
					static_cast<ToolButton *>(GetParent())->_state = 0;
					Invalidate();
					GetParent()->RaiseEvent(ID, ControlEvent::Command, this);
				}
			}
			void ToolButtonPart::CaptureChanged(bool got_capture)
			{
				if (!got_capture) {
					if (!(_state & 0xF0)) {
						if (_state) Invalidate();
						_state = 0;
						static_cast<ToolButton *>(GetParent())->_state = 0;
					}
				}
			}
			void ToolButtonPart::LeftButtonDown(Point at)
			{
				if (_state == 1 || _state == 0) {
					_state = 2;
					SetCapture();
					static_cast<ToolButton *>(GetParent())->_state = 0xF2;
					Invalidate();
				}
			}
			void ToolButtonPart::LeftButtonUp(Point at)
			{
				if (_state == 2) {
					if (IsHovered()) {
						if (_callback) {
							auto my = GetParent()->GetAbsolutePosition();
							auto top_left = Point(my.Left, my.Bottom);
							if (_callback->RunDropDown(this, top_left)) {
								_state = 0xF2;
								ReleaseCapture();
								static_cast<ToolButton *>(GetParent())->_state = 0xF2;
							} else {
								_state = 1;
								static_cast<ToolButton *>(GetParent())->_state = 0xF1;
								Invalidate();
							}
						} else if (_menu) {
							auto parent = GetParent()->GetPosition();
							_state = 0xF2;
							ReleaseCapture();
							static_cast<ToolButton *>(GetParent())->_state = 0xF2;
							RunMenu(_menu, this, Point(-ControlBoundaries.Left, parent.Bottom - parent.Top - ControlBoundaries.Top));
						} else {
							_state = 1;
							static_cast<ToolButton *>(GetParent())->_state = 0xF1;
							Invalidate();
							GetParent()->RaiseEvent(ID, ControlEvent::Command, this);
						}
					} else ReleaseCapture();
				} else ReleaseCapture();
			}
			void ToolButtonPart::MouseMove(Point at)
			{
				if (_state == 0) {
					if (IsHovered()) {
						_state = 1;
						static_cast<ToolButton *>(GetParent())->_state = 0xF1;
						SetCapture();
						Invalidate();
					}
				} else if (_state == 1) {
					if (!IsHovered()) {
						_state = 0;
						static_cast<ToolButton *>(GetParent())->_state = 0;
						ReleaseCapture();
						Invalidate();
					}
				}
			}
			void ToolButtonPart::PopupMenuCancelled(void) { _state = 0; static_cast<ToolButton *>(GetParent())->_state = 0; Invalidate(); }
			string ToolButtonPart::GetControlClass(void) { return L"ToolButtonPart"; }
			void ToolButtonPart::SetNormalImage(ITexture * Image) { ImageNormal.SetRetain(Image); ResetCache(); Invalidate(); }
			ITexture * ToolButtonPart::GetNormalImage(void) { return ImageNormal; }
			void ToolButtonPart::SetGrayedImage(ITexture * Image) { ImageGrayed.SetRetain(Image); ResetCache(); Invalidate(); }
			ITexture * ToolButtonPart::GetGrayedImage(void) { return ImageGrayed; }
			void ToolButtonPart::SetDropDownMenu(Windows::IMenu * Menu) { _menu.SetRetain(Menu); }
			Windows::IMenu * ToolButtonPart::GetDropDownMenu(void) { return _menu; }
			void ToolButtonPart::SetDropDownCallback(IToolButtonPartCustomDropDown * callback) { _callback = callback; }
			ToolButtonPart::IToolButtonPartCustomDropDown * ToolButtonPart::GetDropDownCallback(void) { return _callback; }
			void ToolButtonPart::CustomDropDownClosed(void) { _state = 0; static_cast<ToolButton *>(GetParent())->_state = 0; Invalidate(); }
			void ToolButtonPart::Check(bool check) { if (check != Checked) Invalidate(); Checked = check; }
			bool ToolButtonPart::IsChecked(void) { return Checked; }
		}
	}
}