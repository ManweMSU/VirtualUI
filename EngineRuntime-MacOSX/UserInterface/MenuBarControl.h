#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class MenuBar : public Control, IMenuLayoutProvider
			{
			private:
				struct _menu_bar_element
				{
					int _id, _pure_width, _width, _org;
					bool _enabled, _flush_right;
					string _text;
					SafePointer<Windows::IMenu> _menu;
					SafePointer<Shape> _view_element_normal;
					SafePointer<Shape> _view_element_hot;
					SafePointer<Shape> _view_element_pressed;
					SafePointer<Shape> _view_element_disabled;

					void _reset(void) noexcept;
				};
			private:
				Rectangle _position;
				int _id, _current, _state, _left_space;
				bool _visible;
				SafePointer<VirtualPopupStyles> _styles;
				Array<_menu_bar_element> _elements;

				void _internal_render(Graphics::I2DDeviceContext * device, const Box & at, int current) noexcept;
				virtual VirtualPopupStyles * ProvideStyles(void) override;
				virtual Box ProvidePrimaryBox(void) override;
				virtual Box ProvideClientBox(void) override;
				virtual Shape * ProvideClientBackground(void) override;
				virtual int ProvidePrimarySubmenu(void) override;
				virtual void RenderMenu(Graphics::I2DDeviceContext * device, const Box & at, int current) override;
				virtual int GetElementCount(void) override;
				virtual Windows::IMenu * GetElementMenu(int index) override;
				virtual Box GetElementBox(int index) override;
			public:
				SafePointer<Shape> ViewBackground;
				SafePointer<Shape> ViewClientEffect;
				SafePointer<Template::Shape> ViewElementNormal;
				SafePointer<Template::Shape> ViewElementHot;
				SafePointer<Template::Shape> ViewElementPressed;
				SafePointer<Template::Shape> ViewElementDisabled;

				MenuBar(void);
				MenuBar(Template::ControlTemplate * Template);
				~MenuBar(void) override;

				virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void RaiseEvent(int ID, ControlEvent event, Control * sender) override;
				virtual void PopupMenuCancelled(void) override;
				virtual string GetControlClass(void) override;

				VirtualPopupStyles & GetVisualStyles(void) noexcept;
				const VirtualPopupStyles & GetVisualStyles(void) const noexcept;
				int GetMenuLeftSpace(void) const noexcept;
				void SetMenuLeftSpace(int space) noexcept;
				int GetMenuElementCount(void) const noexcept;
				void AppendMenuElement(int id, bool enable, const string & text, Windows::IMenu * menu);
				void InsertMenuElement(int at, int id, bool enable, const string & text, Windows::IMenu * menu);
				void RemoveMenuElement(int at);
				void ClearMenuElements(void);
				void SwapMenuElements(int i, int j);
				int FindMenuElement(int id);
				int GetMenuElementID(int at);
				void SetMenuElementID(int at, int id);
				bool IsMenuElementEnabled(int at);
				void EnableMenuElement(int at, bool enable);
				string GetMenuElementText(int at);
				void SetMenuElementText(int at, const string & text);
				Windows::IMenu * GetMenuElementMenu(int at);
				void SetFlushRight(int index_from, bool set);
				void SetMenuElementMenu(int at, Windows::IMenu * menu);
				Windows::IMenuItem * FindMenuItem(int id);
			};
		}
	}
}