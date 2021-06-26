#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class Static : public Control, public Template::Controls::Static
			{
			private:
				SafePointer<Shape> shape;
			public:
				Static(void);
				Static(Template::ControlTemplate * Template);
				~Static(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetImage(ITexture * Image);
				virtual ITexture * GetImage(void);
			};
			class ProgressBar : public Control, public Template::Controls::ProgressBar
			{
			private:
				SafePointer<Shape> shape;
			public:
				ProgressBar(void);
				ProgressBar(Template::ControlTemplate * Template);
				~ProgressBar(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetValue(double progress);
				virtual double GetValue(void);
			};
			class ColorView : public Control, public Template::Controls::ColorView
			{
			private:
				SafePointer<Shape> shape;
			public:
				ColorView(void);
				ColorView(Template::ControlTemplate * Template);
				~ColorView(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetColor(UI::Color color);
				virtual UI::Color GetColor(void);
			};
			class ComplexView : public Control, public Template::Controls::ComplexView
			{
			private:
				SafePointer<Shape> shape;
			public:
				ComplexView(void);
				ComplexView(Template::ControlTemplate * Template);
				~ComplexView(void) override;

				virtual void Render(IRenderingDevice * device, const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual Control * FindChild(int ID) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual string GetControlClass(void) override;

				virtual void SetInteger(int value, int index = 0);
				virtual void SetFloat(double value, int index = 0);
				virtual void SetColor(UI::Color value, int index = 0);
				virtual void SetText(const string & value, int index = 0);
				virtual void SetTexture(ITexture * value, int index = 0);
				virtual void SetFont(IFont * value, int index = 0);
				virtual int GetInteger(int index = 0);
				virtual double GetFloat(int index = 0);
				virtual UI::Color GetColor(int index = 0);
				virtual string GetText(int index = 0);
				virtual ITexture * GetTexture(int index = 0);
				virtual IFont * GetFont(int index = 0);
			};
		}
	}
}