#include "ButtonControls.h"

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
			}

			Button::Button(Window * Parent, WindowStation * Station) : Window(Parent, Station), _state(0) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			Button::Button(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station), _state(0)
			{
				if (Template->Properties->GetTemplateClass() != L"Button") throw InvalidArgumentException();
				static_cast<Template::Controls::Button &>(*this) = static_cast<Template::Controls::Button &>(*Template->Properties);
			}
			Button::~Button(void) {}
			void Button::Render(const Box & at)
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
					*shape = temp->Initialize(&ArgumentService::ButtonArgumentProvider(this));
				}
				if (*shape) (*shape)->Render(GetStation()->GetRenderingDevice(), at);
			}
			void Button::ResetCache(void)
			{
				_normal.SetReference(0);
				_disabled.SetReference(0);
				_focused.SetReference(0);
				_hot.SetReference(0);
				_pressed.SetReference(0);
			}
			void Button::Enable(bool enable) { Disabled = !enable; }
			bool Button::IsEnabled(void) { return !Disabled; }
			void Button::Show(bool visible) { Invisible = !visible; }
			bool Button::IsVisible(void) { return !Invisible; }
			bool Button::IsTabStop(void) { return true; }
			void Button::SetID(int _ID) { ID = _ID; }
			int Button::GetID(void) { return ID; }
			Window * Button::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void Button::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle Button::GetRectangle(void) { return ControlPosition; }
			void Button::SetText(const string & text) { Text = text; ResetCache(); }
			string Button::GetText(void) { return Text; }
			void Button::SetNormalImage(ITexture * Image) { ImageNormal.SetRetain(Image); ResetCache(); }
			ITexture * Button::GetNormalImage(void) { return ImageNormal; }
			void Button::SetGrayedImage(ITexture * Image) { ImageGrayed.SetRetain(Image); ResetCache(); }
			ITexture * Button::GetGrayedImage(void) { return ImageGrayed; }
			void Button::FocusChanged(bool got_focus) { if (!got_focus && _state == 0x12) _state = 0; }
			void Button::CaptureChanged(bool got_capture)
			{
				if (!got_capture) _state = 0;
			}
			void Button::LeftButtonDown(Point at)
			{
				SetFocus();
				if (_state == 1 || (_state & 0x10)) {
					_state = 2;
				}
			}
			void Button::LeftButtonUp(Point at)
			{
				if (_state == 2) {
					ReleaseCapture();
					if (GetStation()->HitTest(GetStation()->GetCursorPos()) == this) {
						GetParent()->RaiseEvent(ID, Event::Command, this);
					}
				} else ReleaseCapture();
			}
			void Button::MouseMove(Point at)
			{
				if (_state == 0) {
					_state = 1;
					SetCapture();
				} else if (_state == 1) {
					SetCapture();
					if (GetStation()->HitTest(GetStation()->GetCursorPos()) != this) {
						_state = 0;
						ReleaseCapture();
					}
				}
			}
			void Button::KeyDown(int key_code)
			{
				if (key_code == L' ') {
					if (_state != 0x2 && _state != 0x12) {
						_state = 0x12;
					}
				}
			}
			void Button::KeyUp(int key_code)
			{
				if (key_code == L' ') {
					if (_state == 0x12) {
						_state = 0x0;
						GetParent()->RaiseEvent(ID, Event::Command, this);
					}
				}
			}
		}
	}
}