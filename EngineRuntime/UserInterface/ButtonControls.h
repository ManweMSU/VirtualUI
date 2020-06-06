#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "Menus.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class Button : public Window, public Template::Controls::Button
			{
				SafePointer<Shape> _normal;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _focused;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _pressed;
				int _state;
			public:
				Button(Window * Parent, WindowStation * Station);
				Button(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~Button(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
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

				virtual void SetNormalImage(ITexture * Image);
				virtual ITexture * GetNormalImage(void);
				virtual void SetGrayedImage(ITexture * Image);
				virtual ITexture * GetGrayedImage(void);
			};
			class CheckBox : public Window, public Template::Controls::CheckBox
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
				CheckBox(Window * Parent, WindowStation * Station);
				CheckBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~CheckBox(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
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
			};
			class RadioButton : public Window, public Template::Controls::RadioButton
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
				RadioButton(Window * Parent, WindowStation * Station);
				RadioButton(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~RadioButton(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
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
			};
			class ToolButton : public ParentWindow, public Template::Controls::ToolButton
			{
				friend class ToolButtonPart;
				SafePointer<Shape> _normal;
				SafePointer<Shape> _disabled;
				SafePointer<Shape> _hot;
				SafePointer<Shape> _pressed;
				int _state;
			public:
				ToolButton(Window * Parent, WindowStation * Station);
				ToolButton(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ToolButton(void) override;

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
				virtual void CaptureChanged(bool got_capture) override;
				virtual void MouseMove(Point at) override;
				virtual string GetControlClass(void) override;
			};
			class ToolButtonPart : public Window, public Template::Controls::ToolButtonPart
			{
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
				SafePointer<Menus::Menu> _menu;
				int _state;
			public:
				ToolButtonPart(Window * Parent, WindowStation * Station);
				ToolButtonPart(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ToolButtonPart(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void PopupMenuCancelled(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetNormalImage(ITexture * Image);
				virtual ITexture * GetNormalImage(void);
				virtual void SetGrayedImage(ITexture * Image);
				virtual ITexture * GetGrayedImage(void);
				virtual void SetDropDownMenu(Menus::Menu * Menu);
				virtual Menus::Menu * GetDropDownMenu(void);
			};
		}
	}
}