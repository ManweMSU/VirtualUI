#include "StaticControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace ArgumentService
			{
				class StaticArgumentProvider : public IArgumentProvider
				{
				public:
					Static * Owner;
					StaticArgumentProvider(Static * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->Text;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override
					{
						if (name == L"Texture" && Owner->Image) {
							*value = Owner->Image;
							(*value)->Retain();
						} else *value = 0;
					}
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font" && Owner->Font) {
							*value = Owner->Font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
				class ProgressBarArgumentProvider : public IArgumentProvider
				{
				public:
					ProgressBar * Owner;
					ProgressBarArgumentProvider(ProgressBar * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override
					{
						if (name == L"Progress") *value = int(Owner->Progress);
						else *value = 0;
					}
					virtual void GetArgument(const string & name, double * value) override
					{
						if (name == L"Progress") *value = Owner->Progress;
						else *value = 0.0;
					}
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override { *value = L""; }
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override { *value = 0; }
				};
				class ColorViewArgumentProvider : public IArgumentProvider
				{
				public:
					ColorView * Owner;
					ColorViewArgumentProvider(ColorView * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override
					{
						if (name == L"Color") *value = Owner->Color;
						else *value = 0;
					}
					virtual void GetArgument(const string & name, string * value) override { *value = L""; }
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override { *value = 0; }
				};
				class ComplexViewArgumentProvider: public IArgumentProvider
				{
				public:
					ComplexView * Owner;
					ComplexViewArgumentProvider(ComplexView * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override
					{
						if (name == L"Integer") *value = Owner->Integer;
						else if (name == L"Integer2") *value = Owner->Integer2;
						else if (name == L"Float") *value = int(Owner->Float);
						else if (name == L"Float2") *value = int(Owner->Float2);
						else *value = 0;
					}
					virtual void GetArgument(const string & name, double * value) override
					{
						if (name == L"Integer") *value = double(Owner->Integer);
						else if (name == L"Integer2") *value = double(Owner->Integer2);
						else if (name == L"Float") *value = Owner->Float;
						else if (name == L"Float2") *value = Owner->Float2;
						else *value = 0.0;
					}
					virtual void GetArgument(const string & name, Color * value) override
					{
						if (name == L"Color") *value = Owner->Color;
						else if (name == L"Color2") *value = Owner->Color2;
						else *value = 0;
					}
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->Text;
						else if (name == L"Text2") *value = Owner->Text2;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, Graphics::IBitmap ** value) override
					{
						if (name == L"Image") *value = Owner->Image;
						else if (name == L"Image2") *value = Owner->Image2;
						else *value = 0;
						if (*value) (*value)->Retain();
					}
					virtual void GetArgument(const string & name, Graphics::IFont ** value) override
					{
						if (name == L"Font") *value = Owner->Font;
						else if (name == L"Font2") *value = Owner->Font2;
						else *value = 0;
						if (*value) (*value)->Retain();
					}
				};
			}

			Static::Static(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			Static::Static(Template::ControlTemplate * Template)
			{
				if (Template->Properties->GetTemplateClass() != L"Static") throw InvalidArgumentException();
				static_cast<Template::Controls::Static &>(*this) = static_cast<Template::Controls::Static &>(*Template->Properties);
			}
			Static::~Static(void) {}
			void Static::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				if (!shape) {
					if (View) {
						auto provider = ArgumentService::StaticArgumentProvider(this);
						shape.SetReference(View->Initialize(&provider));
						shape->Render(device, at);
					}
				} else shape->Render(device, at);
			}
			void Static::ResetCache(void) { shape.SetReference(0); }
			void Static::Enable(bool enable) {}
			bool Static::IsEnabled(void) { return false; }
			void Static::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool Static::IsVisible(void) { return !Invisible; }
			void Static::SetID(int _ID) { ID = _ID; }
			int Static::GetID(void) { return ID; }
			Control * Static::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void Static::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				if (GetParent()) GetParent()->ArrangeChildren();
				Invalidate();
			}
			Rectangle Static::GetRectangle(void) { return ControlPosition; }
			void Static::SetText(const string & text) { Text = text; ResetCache(); Invalidate(); }
			string Static::GetText(void) { return Text; }
			string Static::GetControlClass(void) { return L"Static"; }
			void Static::SetImage(Graphics::IBitmap * image) { Image.SetRetain(image); ResetCache(); Invalidate(); }
			Graphics::IBitmap * Static::GetImage(void) { return Image; }

			ProgressBar::ProgressBar(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ProgressBar::ProgressBar(Template::ControlTemplate * Template)
			{
				if (Template->Properties->GetTemplateClass() != L"ProgressBar") throw InvalidArgumentException();
				static_cast<Template::Controls::ProgressBar &>(*this) = static_cast<Template::Controls::ProgressBar &>(*Template->Properties);
			}
			ProgressBar::~ProgressBar(void) {}
			void ProgressBar::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				if (!shape) {
					if (View) {
						auto provider = ArgumentService::ProgressBarArgumentProvider(this);
						shape.SetReference(View->Initialize(&provider));
						shape->Render(device, at);
					}
				} else shape->Render(device, at);
			}
			void ProgressBar::ResetCache(void) { shape.SetReference(0); }
			void ProgressBar::Enable(bool enable) {}
			bool ProgressBar::IsEnabled(void) { return false; }
			void ProgressBar::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool ProgressBar::IsVisible(void) { return !Invisible; }
			void ProgressBar::SetID(int _ID) { ID = _ID; }
			int ProgressBar::GetID(void) { return ID; }
			Control * ProgressBar::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void ProgressBar::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				if (GetParent()) GetParent()->ArrangeChildren();
				Invalidate();
			}
			Rectangle ProgressBar::GetRectangle(void) { return ControlPosition; }
			string ProgressBar::GetControlClass(void) { return L"ProgressBar"; }
			void ProgressBar::SetValue(double progress) { Progress = progress; ResetCache(); Invalidate(); }
			double ProgressBar::GetValue(void) { return Progress; }

			ColorView::ColorView(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ColorView::ColorView(Template::ControlTemplate * Template)
			{
				if (Template->Properties->GetTemplateClass() != L"ColorView") throw InvalidArgumentException();
				static_cast<Template::Controls::ColorView &>(*this) = static_cast<Template::Controls::ColorView &>(*Template->Properties);
			}
			ColorView::~ColorView(void) {}
			void ColorView::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				if (!shape) {
					if (View) {
						auto provider = ArgumentService::ColorViewArgumentProvider(this);
						shape.SetReference(View->Initialize(&provider));
						shape->Render(device, at);
					}
				} else shape->Render(device, at);
			}
			void ColorView::ResetCache(void) { shape.SetReference(0); }
			void ColorView::Enable(bool enable) {}
			bool ColorView::IsEnabled(void) { return false; }
			void ColorView::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool ColorView::IsVisible(void) { return !Invisible; }
			void ColorView::SetID(int _ID) { ID = _ID; }
			int ColorView::GetID(void) { return ID; }
			Control * ColorView::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void ColorView::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				if (GetParent()) GetParent()->ArrangeChildren();
				Invalidate();
			}
			Rectangle ColorView::GetRectangle(void) { return ControlPosition; }
			string ColorView::GetControlClass(void) { return L"ColorView"; }
			void ColorView::SetColor(Engine::Color color) { Color = color; ResetCache(); Invalidate(); }
			Color ColorView::GetColor(void) { return Color; }

			ComplexView::ComplexView(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ComplexView::ComplexView(Template::ControlTemplate * Template)
			{
				if (Template->Properties->GetTemplateClass() != L"ComplexView") throw InvalidArgumentException();
				static_cast<Template::Controls::ComplexView &>(*this) = static_cast<Template::Controls::ComplexView &>(*Template->Properties);
			}
			ComplexView::~ComplexView(void) {}
			void ComplexView::Render(Graphics::I2DDeviceContext * device, const Box & at)
			{
				if (!shape) {
					if (View) {
						auto provider = ArgumentService::ComplexViewArgumentProvider(this);
						shape.SetReference(View->Initialize(&provider));
						shape->Render(device, at);
					}
				} else shape->Render(device, at);
			}
			void ComplexView::ResetCache(void) { shape.SetReference(0); }
			void ComplexView::Enable(bool enable) {}
			bool ComplexView::IsEnabled(void) { return false; }
			void ComplexView::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool ComplexView::IsVisible(void) { return !Invisible; }
			void ComplexView::SetID(int _ID) { ID = _ID; }
			int ComplexView::GetID(void) { return ID; }
			Control * ComplexView::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void ComplexView::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				if (GetParent()) GetParent()->ArrangeChildren();
				Invalidate();
			}
			Rectangle ComplexView::GetRectangle(void) { return ControlPosition; }
			string ComplexView::GetControlClass(void) { return L"ComplexView"; }
			void ComplexView::SetInteger(int value, int index) { if (index == 0) Integer = value; else if (index == 1) Integer2 = value; ResetCache(); Invalidate(); }
			void ComplexView::SetFloat(double value, int index) { if (index == 0) Float = value; else if (index == 1) Float2 = value; ResetCache(); Invalidate(); }
			void ComplexView::SetColor(Engine::Color value, int index) { if (index == 0) Color = value; else if (index == 1) Color2 = value; ResetCache(); Invalidate(); }
			void ComplexView::SetText(const string & value, int index) { if (index == 0) Text = value; else if (index == 1) Text2 = value; ResetCache(); Invalidate(); }
			void ComplexView::SetTexture(Graphics::IBitmap * value, int index) { if (index == 0) Image.SetRetain(value); else if (index == 1) Image2.SetRetain(value); ResetCache(); Invalidate(); }
			void ComplexView::SetFont(Graphics::IFont * value, int index) { if (index == 0) Font.SetRetain(value); else if (index == 1) Font2.SetRetain(value); ResetCache(); Invalidate(); }
			int ComplexView::GetInteger(int index) { if (index == 0) return Integer; else if (index == 1) return Integer2; else return 0; }
			double ComplexView::GetFloat(int index) { if (index == 0) return Float; else if (index == 1) return Float2; else return 0.0; }
			Engine::Color ComplexView::GetColor(int index) { if (index == 0) return Color; else if (index == 1) return Color2; else return 0; }
			string ComplexView::GetText(int index) { if (index == 0) return Text; else if (index == 1) return Text2; else return L""; }
			Graphics::IBitmap * ComplexView::GetTexture(int index) { if (index == 0) return Image; else if (index == 1) return Image2; else return 0; }
			Graphics::IFont * ComplexView::GetFont(int index) { if (index == 0) return Font; else if (index == 1) return Font2; else return 0; }
		}
	}
}