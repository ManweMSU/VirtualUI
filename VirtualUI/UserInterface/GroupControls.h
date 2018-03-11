#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class ControlGroup : public ParentWindow, public Template::Controls::ControlGroup
			{
			private:
				SafePointer<Shape> _background;
			public:
				ControlGroup(Window * Parent, WindowStation * Station);
				ControlGroup(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ControlGroup(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
			};
		}
	}
}