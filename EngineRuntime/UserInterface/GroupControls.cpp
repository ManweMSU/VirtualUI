#include "GroupControls.h"

#include "ButtonControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			ControlGroup::ControlGroup(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ControlGroup::ControlGroup(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"ControlGroup") throw InvalidArgumentException();
				static_cast<Template::Controls::ControlGroup &>(*this) = static_cast<Template::Controls::ControlGroup &>(*Template->Properties);
				Constructor::ConstructChildren(this, Template);
			}
			ControlGroup::~ControlGroup(void) {}
			void ControlGroup::Render(const Box & at)
			{
				if (!_background && Background) _background.SetReference(Background->Initialize(&ZeroArgumentProvider()));
				if (_background) _background->Render(GetStation()->GetRenderingDevice(), at);
				ParentWindow::Render(at);
			}
			void ControlGroup::ResetCache(void) { _background.SetReference(0); ParentWindow::ResetCache(); }
			void ControlGroup::Enable(bool enable) {}
			bool ControlGroup::IsEnabled(void) { return true; }
			void ControlGroup::Show(bool visible) { Invisible = !visible; }
			bool ControlGroup::IsVisible(void) { return !Invisible; }
			void ControlGroup::SetID(int _ID) { ID = _ID; }
			int ControlGroup::GetID(void) { return ID; }
			void ControlGroup::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle ControlGroup::GetRectangle(void) { return ControlPosition; }

			RadioButtonGroup::RadioButtonGroup(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			RadioButtonGroup::RadioButtonGroup(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"RadioButtonGroup") throw InvalidArgumentException();
				static_cast<Template::Controls::RadioButtonGroup &>(*this) = static_cast<Template::Controls::RadioButtonGroup &>(*Template->Properties);
				for (int i = 0; i < Template->Children.Length(); i++) {
					Station->CreateWindow<RadioButton>(this, &Template->Children[i]);
				}
			}
			RadioButtonGroup::~RadioButtonGroup(void) {}
			void RadioButtonGroup::Render(const Box & at)
			{
				if (!_background && Background) _background.SetReference(Background->Initialize(&ZeroArgumentProvider()));
				if (_background) _background->Render(GetStation()->GetRenderingDevice(), at);
				ParentWindow::Render(at);
			}
			void RadioButtonGroup::ResetCache(void) { _background.SetReference(0); ParentWindow::ResetCache(); }
			void RadioButtonGroup::Enable(bool enable) { Disabled = !enable; }
			bool RadioButtonGroup::IsEnabled(void) { return !Disabled; }
			void RadioButtonGroup::Show(bool visible) { Invisible = !visible; }
			bool RadioButtonGroup::IsVisible(void) { return !Invisible; }
			void RadioButtonGroup::SetID(int _ID) { ID = _ID; }
			int RadioButtonGroup::GetID(void) { return ID; }
			void RadioButtonGroup::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle RadioButtonGroup::GetRectangle(void) { return ControlPosition; }
			void RadioButtonGroup::CheckRadioButton(Window * window) { for (int i = 0; i < ChildrenCount(); i++) static_cast<RadioButton *>(Child(i))->Checked = window == Child(i); }
			void RadioButtonGroup::CheckRadioButton(int ID) { for (int i = 0; i < ChildrenCount(); i++) static_cast<RadioButton *>(Child(i))->Checked = ID == Child(i)->GetID(); }
			int RadioButtonGroup::GetCheckedButton(void)
			{
				for (int i = 0; i < ChildrenCount(); i++) if (static_cast<RadioButton *>(Child(i))->Checked) return Child(i)->GetID();
				return 0;
			}
		}
	}
}