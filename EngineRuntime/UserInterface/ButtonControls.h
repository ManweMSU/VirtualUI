#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

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
				virtual void KeyDown(int key_code) override;
				virtual void KeyUp(int key_code) override;

				virtual void SetNormalImage(ITexture * Image);
				virtual ITexture * GetNormalImage(void);
				virtual void SetGrayedImage(ITexture * Image);
				virtual ITexture * GetGrayedImage(void);
			};
		}
	}
}