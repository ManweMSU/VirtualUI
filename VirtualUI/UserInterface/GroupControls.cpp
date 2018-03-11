#include "GroupControls.h"

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
		}
	}
}