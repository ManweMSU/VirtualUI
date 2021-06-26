#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class Button : public Control, public Template::Controls::Button
			{
				SafePointer<Shape> _normal;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _focused;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _pressed;
				int _state;
			public:
				Button(void);
				Button(Template::ControlTemplate * Template);
				~Button(void) override;

				virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual bool KeyDown(int key_code) override;
				virtual void KeyUp(int key_code) override;
				virtual string GetControlClass(void) override;

				virtual void SetNormalImage(Graphics::IBitmap * Image);
				virtual Graphics::IBitmap * GetNormalImage(void);
				virtual void SetGrayedImage(Graphics::IBitmap * Image);
				virtual Graphics::IBitmap * GetGrayedImage(void);
			};
			class CheckBox : public Control, public Template::Controls::CheckBox
			{
				SafePointer<Shape> _normal;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _focused;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _pressed;
				SafePointer<Shape> _normal_checked;
				SafePointer<Shape> _disabled_checked;
				SafePointer<Shape> _focused_checked;
				SafePointer<Shape> _hot_checked;
				SafePointer<Shape> _pressed_checked;
				int _state;
			public:
				CheckBox(void);
				CheckBox(Template::ControlTemplate * Template);
				~CheckBox(void) override;

				virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual bool KeyDown(int key_code) override;
				virtual void KeyUp(int key_code) override;
				virtual string GetControlClass(void) override;

				virtual void Check(bool check);
				virtual bool IsChecked(void);
			};
			class RadioButton : public Control, public Template::Controls::RadioButton
			{
				SafePointer<Shape> _normal;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _focused;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _pressed;
				SafePointer<Shape> _normal_checked;
				SafePointer<Shape> _disabled_checked;
				SafePointer<Shape> _focused_checked;
				SafePointer<Shape> _hot_checked;
				SafePointer<Shape> _pressed_checked;
				int _state;
			public:
				RadioButton(void);
				RadioButton(Template::ControlTemplate * Template);
				~RadioButton(void) override;

				virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual bool KeyDown(int key_code) override;
				virtual void KeyUp(int key_code) override;
				virtual string GetControlClass(void) override;

				virtual void Check(bool check);
				virtual bool IsChecked(void);
			};
			class ToolButton : public ParentControl, public Template::Controls::ToolButton
			{
				friend class ToolButtonPart;
				SafePointer<Shape> _normal;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _pressed;
				int _state;
			public:
				ToolButton(void);
				ToolButton(Template::ControlTemplate * Template);
				~ToolButton(void) override;

				virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void MouseMove(Point at) override;
				virtual string GetControlClass(void) override;
			};
			class ToolButtonPart : public Control, public Template::Controls::ToolButtonPart
			{
			public:
				class IToolButtonPartCustomDropDown
				{
				public:
					virtual bool RunDropDown(ToolButtonPart * sender, Point top_left) = 0;
				};
			private:
				friend class ToolButton;
				SafePointer<Shape> _normal;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _normal_semihot;
				SafePointer<Shape> _disabled_semihot;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _pressed;
				SafePointer<Shape> _normal_checked;
				SafePointer<Shape> _disabled_checked;
				SafePointer<Shape> _normal_semihot_checked;
				SafePointer<Shape> _disabled_semihot_checked;
				SafePointer<Shape> _hot_checked;
				SafePointer<Shape> _pressed_checked;
				SafePointer<Windows::IMenu> _menu;
				IToolButtonPartCustomDropDown * _callback;
				int _state;
			public:
				ToolButtonPart(void);
				ToolButtonPart(Template::ControlTemplate * Template);
				~ToolButtonPart(void) override;

				virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void RaiseEvent(int ID, ControlEvent event, Control * sender) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void PopupMenuCancelled(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetNormalImage(Graphics::IBitmap * Image);
				virtual Graphics::IBitmap * GetNormalImage(void);
				virtual void SetGrayedImage(Graphics::IBitmap * Image);
				virtual Graphics::IBitmap * GetGrayedImage(void);
				virtual void SetDropDownMenu(Windows::IMenu * Menu);
				virtual Windows::IMenu * GetDropDownMenu(void);
				virtual void SetDropDownCallback(IToolButtonPartCustomDropDown * callback);
				virtual IToolButtonPartCustomDropDown * GetDropDownCallback(void);
				virtual void CustomDropDownClosed(void);

				virtual void Check(bool check);
				virtual bool IsChecked(void);
			};
		}
	}
}