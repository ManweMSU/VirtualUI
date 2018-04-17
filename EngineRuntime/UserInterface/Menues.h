#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Menues
		{
			class MenuElement;
			class MenuItem;
			class MenuSeparator;

			class MenuElement : public Object
			{
			public:
				virtual int GetHeight(void) const = 0;
				virtual int GetWidth(void) const = 0;
				virtual void Render(const Box & at, bool highlighted) = 0;
				virtual void WakeUp(IRenderingDevice * Device) = 0;
				virtual void Shutdown(void) = 0;
				virtual bool IsSeparator(void) const = 0;
				virtual MenuItem * FindChild(int ID) = 0;
				virtual IRenderingDevice * GetRenderingDevice(void) = 0;
			};
			class MenuItem : public MenuElement, public Template::Controls::MenuItem
			{
			private:
				SafePointer<IRenderingDevice> _device;
				SafePointer<Shape> _normal;
				SafePointer<Shape> _normal_checked;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _disabled_checked;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _hot_checked;
				mutable int _width;
			public:
				ObjectArray<MenuElement> Children;

				MenuItem(void);
				MenuItem(Template::ControlTemplate * Template);
				~MenuItem(void) override;

				virtual int GetHeight(void) const override;
				virtual int GetWidth(void) const override;
				virtual void Render(const Box & at, bool highlighted) override;
				virtual void WakeUp(IRenderingDevice * Device) override;
				virtual void Shutdown(void) override;
				virtual bool IsSeparator(void) const override;
				virtual MenuItem * FindChild(int ID) override;
				virtual IRenderingDevice * GetRenderingDevice(void) override;
			};
			class MenuSeparator : public MenuElement, public Template::Controls::MenuSeparator
			{
			private:
				SafePointer<IRenderingDevice> _device;
				SafePointer<Shape> _view;
			public:
				MenuSeparator(void);
				MenuSeparator(Template::ControlTemplate * Template);
				~MenuSeparator(void) override;

				virtual int GetHeight(void) const override;
				virtual int GetWidth(void) const override;
				virtual void Render(const Box & at, bool highlighted) override;
				virtual void WakeUp(IRenderingDevice * Device) override;
				virtual void Shutdown(void) override;
				virtual bool IsSeparator(void) const override;
				virtual MenuItem * FindChild(int ID) override;
				virtual IRenderingDevice * GetRenderingDevice(void) override;
			};
			class Menu : public Object
			{
			public:
				ObjectArray<MenuElement> Children;

				Menu(void);
				Menu(Template::ControlTemplate * MenuTemplate);
				~Menu(void) override;

				MenuItem * FindChild(int ID);
				void CheckRange(int RangeMin, int RangeMax, int Element);

				void RunPopup(Window * owner, Point at);
			};
		}
	}
}