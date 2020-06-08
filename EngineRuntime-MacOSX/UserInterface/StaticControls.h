#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class Static : public Window, public Template::Controls::Static
			{
			private:
				SafePointer<Shape> shape;
			public:
				Static(Window * Parent, WindowStation * Station);
				Static(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~Static(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetImage(ITexture * Image);
				virtual ITexture * GetImage(void);
			};
			class ProgressBar : public Window, public Template::Controls::ProgressBar
			{
			private:
				SafePointer<Shape> shape;
			public:
				ProgressBar(Window * Parent, WindowStation * Station);
				ProgressBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ProgressBar(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetValue(double progress);
				virtual double GetValue(void);
			};
			class ColorView : public Window, public Template::Controls::ColorView
			{
			private:
				SafePointer<Shape> shape;
			public:
				ColorView(Window * Parent, WindowStation * Station);
				ColorView(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ColorView(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Window * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetColor(UI::Color color);
				virtual UI::Color GetColor(void);
			};
		}
	}
}