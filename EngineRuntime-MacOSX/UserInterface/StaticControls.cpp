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
					virtual void GetArgument(const string & name, ITexture ** value) override
					{
						if (name == L"Texture" && Owner->Image) {
							*value = Owner->Image;
							(*value)->Retain();
						} else *value = 0;
					}
					virtual void GetArgument(const string & name, IFont ** value) override
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
					virtual void GetArgument(const string & name, ITexture ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, IFont ** value) override { *value = 0; }
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
					virtual void GetArgument(const string & name, ITexture ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, IFont ** value) override { *value = 0; }
				};
			}

			Static::Static(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			Static::Static(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"Static") throw InvalidArgumentException();
				static_cast<Template::Controls::Static &>(*this) = static_cast<Template::Controls::Static &>(*Template->Properties);
			}
			Static::~Static(void) {}
			void Static::Render(const Box & at)
			{
				if (!shape) {
					if (View) {
						auto provider = ArgumentService::StaticArgumentProvider(this);
						shape.SetReference(View->Initialize(&provider));
						shape->Render(GetStation()->GetRenderingDevice(), at);
					}
				} else shape->Render(GetStation()->GetRenderingDevice(), at);
			}
			void Static::ResetCache(void) { shape.SetReference(0); }
			void Static::Enable(bool enable) {}
			bool Static::IsEnabled(void) { return false; }
			void Static::Show(bool visible) { Invisible = !visible; }
			bool Static::IsVisible(void) { return !Invisible; }
			void Static::SetID(int _ID) { ID = _ID; }
			int Static::GetID(void) { return ID; }
			Window * Static::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void Static::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				GetParent()->ArrangeChildren();
			}
			Rectangle Static::GetRectangle(void) { return ControlPosition; }
			void Static::SetText(const string & text) { Text = text; ResetCache(); }
			string Static::GetText(void) { return Text; }
			void Static::SetImage(ITexture * image) { Image.SetRetain(image); ResetCache(); }
			ITexture * Static::GetImage(void) { return Image; }

			ProgressBar::ProgressBar(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ProgressBar::ProgressBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"ProgressBar") throw InvalidArgumentException();
				static_cast<Template::Controls::ProgressBar &>(*this) = static_cast<Template::Controls::ProgressBar &>(*Template->Properties);
			}
			ProgressBar::~ProgressBar(void) {}
			void ProgressBar::Render(const Box & at)
			{
				if (!shape) {
					if (View) {
						auto provider = ArgumentService::ProgressBarArgumentProvider(this);
						shape.SetReference(View->Initialize(&provider));
						shape->Render(GetStation()->GetRenderingDevice(), at);
					}
				} else shape->Render(GetStation()->GetRenderingDevice(), at);
			}
			void ProgressBar::ResetCache(void) { shape.SetReference(0); }
			void ProgressBar::Enable(bool enable) {}
			bool ProgressBar::IsEnabled(void) { return false; }
			void ProgressBar::Show(bool visible) { Invisible = !visible; }
			bool ProgressBar::IsVisible(void) { return !Invisible; }
			void ProgressBar::SetID(int _ID) { ID = _ID; }
			int ProgressBar::GetID(void) { return ID; }
			Window * ProgressBar::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void ProgressBar::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				GetParent()->ArrangeChildren();
			}
			Rectangle ProgressBar::GetRectangle(void) { return ControlPosition; }
			void ProgressBar::SetValue(double progress) { Progress = progress; ResetCache(); }
			double ProgressBar::GetValue(void) { return Progress; }

			ColorView::ColorView(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ColorView::ColorView(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"ColorView") throw InvalidArgumentException();
				static_cast<Template::Controls::ColorView &>(*this) = static_cast<Template::Controls::ColorView &>(*Template->Properties);
			}
			ColorView::~ColorView(void) {}
			void ColorView::Render(const Box & at)
			{
				if (!shape) {
					if (View) {
						auto provider = ArgumentService::ColorViewArgumentProvider(this);
						shape.SetReference(View->Initialize(&provider));
						shape->Render(GetStation()->GetRenderingDevice(), at);
					}
				} else shape->Render(GetStation()->GetRenderingDevice(), at);
			}
			void ColorView::ResetCache(void) { shape.SetReference(0); }
			void ColorView::Enable(bool enable) {}
			bool ColorView::IsEnabled(void) { return false; }
			void ColorView::Show(bool visible) { Invisible = !visible; }
			bool ColorView::IsVisible(void) { return !Invisible; }
			void ColorView::SetID(int _ID) { ID = _ID; }
			int ColorView::GetID(void) { return ID; }
			Window * ColorView::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void ColorView::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				GetParent()->ArrangeChildren();
			}
			Rectangle ColorView::GetRectangle(void) { return ControlPosition; }
			void ColorView::SetColor(UI::Color color) { Color = color; ResetCache(); }
			UI::Color ColorView::GetColor(void) { return Color; }
		}
	}
}