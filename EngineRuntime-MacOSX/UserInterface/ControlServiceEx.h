#pragma once

#include "ControlBase.h"

namespace Engine
{
	namespace UI
	{
		class MenuArgumentProvider : public IArgumentProvider
		{
			Windows::IMenuItem * Item;
			Template::Controls::MenuItem * Base;
		public:
			MenuArgumentProvider(Windows::IMenuItem * item, Template::Controls::MenuItem * base);
			virtual void GetArgument(const string & name, int * value) override;
			virtual void GetArgument(const string & name, double * value) override;
			virtual void GetArgument(const string & name, Color * value) override;
			virtual void GetArgument(const string & name, string * value) override;
			virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override;
			virtual void GetArgument(const string & name, Graphics::IFont ** value) override;
		};

		void RunVirtualMenu(Windows::IMenu * menu, Control * for_control, Point at);
		void RunVirtualMenu(Control * for_control, IMenuLayoutProvider * provider);
		Control * RunVirtualPopup(ControlSystem * system, const Box & at);
	}
}