#include "Templates.h"

namespace Engine
{
	namespace UI
	{
		ZeroArgumentProvider::ZeroArgumentProvider(void) {}
		void ZeroArgumentProvider::GetArgument(const string & name, int * value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, double * value) { *value = 0.0; }
		void ZeroArgumentProvider::GetArgument(const string & name, Color * value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, string * value) { *value = L""; }
		void ZeroArgumentProvider::GetArgument(const string & name, Graphics::IBitmap ** value) { *value = 0; }
		void ZeroArgumentProvider::GetArgument(const string & name, Graphics::IFont ** value) { *value = 0; }
		ReflectorArgumentProvider::ReflectorArgumentProvider(Reflection::Reflected * source) : Source(source) {}
		void ReflectorArgumentProvider::GetArgument(const string & name, int * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Integer) *value = prop.Get<int>();
			else if (prop.Type == Reflection::PropertyType::Double) *value = int(prop.Get<double>());
			*value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, double * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Integer) *value = double(prop.Get<int>());
			else if (prop.Type == Reflection::PropertyType::Double) *value = prop.Get<double>();
			else *value = 0.0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, Color * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Color) *value = prop.Get<Color>();
			else *value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, string * value)
		{
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::String) *value = prop.Get<string>();
			else *value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, Graphics::IBitmap ** value)
		{
			Graphics::IBitmap * object = 0;
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Texture) object = prop.Get<SafePointer<Graphics::IBitmap>>();
			if (object) {
				*value = object;
				object->Retain();
			} else *value = 0;
		}
		void ReflectorArgumentProvider::GetArgument(const string & name, Graphics::IFont ** value)
		{
			Graphics::IFont * object = 0;
			auto prop = Source->GetProperty(name);
			if (prop.Type == Reflection::PropertyType::Font) object = prop.Get<SafePointer<Graphics::IFont>>();
			if (object) {
				*value = object;
				object->Retain();
			} else *value = 0;
		}

		namespace Template
		{
			ExpressionArgumentProvider::ExpressionArgumentProvider(IArgumentProvider * provider) : Source(provider) {}
			int64 ExpressionArgumentProvider::GetInteger(const string & name)
			{
				int value;
				Source->GetArgument(name, &value);
				return value;
			}
			double ExpressionArgumentProvider::GetDouble(const string & name)
			{
				double value;
				Source->GetArgument(name, &value);
				return value;
			}
			template <> int ResolveExpression<int>(const string & expression, IArgumentProvider * provider)
			{
				auto argp = ExpressionArgumentProvider(provider);
				return int(Syntax::Math::CalculateExpressionInteger(expression, &argp));
			}
			template <> double ResolveExpression<double>(const string & expression, IArgumentProvider * provider)
			{
				auto argp = ExpressionArgumentProvider(provider);
				return Syntax::Math::CalculateExpressionDouble(expression, &argp);
			}
			Coordinate::Coordinate(void) {}
			Coordinate::Coordinate(const Engine::UI::Coordinate & src) : Absolute(src.Absolute), Zoom(src.Zoom), Anchor(src.Anchor) {}
			Coordinate::Coordinate(const BasicTemplate<int>& shift, const BasicTemplate<double>& zoom, const BasicTemplate<double>& anchor) : Absolute(shift), Zoom(zoom), Anchor(anchor) {}
			bool Coordinate::IsDefined(void) const { return Absolute.IsDefined() && Zoom.IsDefined() && Anchor.IsDefined(); }
			Engine::UI::Coordinate Coordinate::Initialize(IArgumentProvider * provider) const { return Engine::UI::Coordinate(Absolute.Initialize(provider), Zoom.Initialize(provider), Anchor.Initialize(provider)); }
			Rectangle::Rectangle(void) {}
			Rectangle::Rectangle(const Engine::UI::Rectangle & src) : Left(src.Left), Top(src.Top), Right(src.Right), Bottom(src.Bottom) {}
			Rectangle::Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}
			bool Rectangle::IsDefined(void) const { return Left.IsDefined() && Top.IsDefined() && Right.IsDefined() && Bottom.IsDefined(); }
			Engine::UI::Rectangle Rectangle::Initialize(IArgumentProvider * provider) const { return Engine::UI::Rectangle(Left.Initialize(provider), Top.Initialize(provider), Right.Initialize(provider), Bottom.Initialize(provider)); }
			GradientPoint::GradientPoint(void) {}
			GradientPoint::GradientPoint(const Engine::GradientPoint & gradient) : PointColor(gradient.Value), Position(gradient.Position) {}
			GradientPoint::GradientPoint(const BasicTemplate<Color>& color, const BasicTemplate<double>& position) : PointColor(color), Position(position) {}
			bool GradientPoint::IsDefined(void) const { return PointColor.IsDefined() && Position.IsDefined(); }
			Engine::GradientPoint GradientPoint::Initialize(IArgumentProvider * provider) const { return Engine::GradientPoint(PointColor.Initialize(provider), Position.Initialize(provider)); }

			BarShape::BarShape(void) : Gradient(0x4), X1(0), Y1(0), X2(0), Y2(0) { Position = Engine::UI::Rectangle::Entire(); }
			BarShape::~BarShape(void) {}
			string BarShape::ToString(void) const { return L"Templates::BarShape"; }
			bool BarShape::IsDefined(void) const
			{
				for (int i = 0; i < Gradient.Length(); i++) if (!Gradient[i].IsDefined()) return false;
				return X1.IsDefined() && X2.IsDefined() && Y1.IsDefined() && Y2.IsDefined() && Position.IsDefined();
			}
			Engine::UI::Shape * BarShape::Initialize(IArgumentProvider * provider) const
			{
				Array<Engine::GradientPoint> init(1);
				init.SetLength(Gradient.Length());
				for (int i = 0; i < Gradient.Length(); i++) init[i] = Gradient[i].Initialize(provider);
				return new Engine::UI::BarShape(Position.Initialize(provider), init, X1.Initialize(provider), Y1.Initialize(provider), X2.Initialize(provider), Y2.Initialize(provider));
			}
			BlurEffectShape::BlurEffectShape(void) : BlurPower(5.0) { Position = Engine::UI::Rectangle::Entire(); }
			BlurEffectShape::~BlurEffectShape(void) {}
			string BlurEffectShape::ToString(void) const { return L"Templates::BlurEffectShape"; }
			bool BlurEffectShape::IsDefined(void) const { return BlurPower.IsDefined() && Position.IsDefined(); }
			Engine::UI::Shape * BlurEffectShape::Initialize(IArgumentProvider * provider) const
			{
				return new Engine::UI::BlurEffectShape(Position.Initialize(provider), BlurPower.Initialize(provider));
			}
			InversionEffectShape::InversionEffectShape(void) { Position = Engine::UI::Rectangle::Entire(); }
			InversionEffectShape::~InversionEffectShape(void) {}
			string InversionEffectShape::ToString(void) const { return L"Templates::InversionEffectShape"; }
			bool InversionEffectShape::IsDefined(void) const { return Position.IsDefined(); }
			Engine::UI::Shape * InversionEffectShape::Initialize(IArgumentProvider * provider) const
			{
				return new Engine::UI::InversionEffectShape(Position.Initialize(provider));
			}
			TextureShape::TextureShape(void) : RenderMode(Engine::UI::TextureShape::TextureRenderMode::Stretch) { Position = Engine::UI::Rectangle::Entire(); From = Engine::UI::Rectangle::Entire(); }
			TextureShape::~TextureShape(void) {}
			string TextureShape::ToString(void) const { return L"Templates::TextureShape"; }
			bool TextureShape::IsDefined(void) const
			{
				return Texture.IsDefined() && Position.IsDefined() && From.IsDefined();
			}
			Engine::UI::Shape * TextureShape::Initialize(IArgumentProvider * provider) const
			{
				SafePointer<Graphics::IBitmap> Query = Texture.Initialize(provider);
				return new Engine::UI::TextureShape(Position.Initialize(provider), Query, From.Initialize(provider), RenderMode);
			}
			TextShape::TextShape(void) : Flags(0), TextColor(Engine::Color(0)) { Position = Engine::UI::Rectangle::Entire(); }
			TextShape::~TextShape(void) {}
			string TextShape::ToString(void) const { return L"Templates::TextShape"; }
			bool TextShape::IsDefined(void) const
			{
				return Position.IsDefined() && TextColor.IsDefined() && Text.IsDefined() && Font.IsDefined();
			}
			Engine::UI::Shape * TextShape::Initialize(IArgumentProvider * provider) const
			{
				SafePointer<Graphics::IFont> Query = Font.Initialize(provider);
				return new Engine::UI::TextShape(Position.Initialize(provider), Text.Initialize(provider), Query, TextColor.Initialize(provider), Flags);
			}
			FrameShape::FrameShape(void) : Children(0x4), RenderMode(Engine::UI::FrameShape::FrameRenderMode::Normal), Opacity(1.0) { Position = Engine::UI::Rectangle::Entire(); }
			FrameShape::~FrameShape(void) {}
			string FrameShape::ToString(void) const { return L"Templates::FrameShape"; }
			bool FrameShape::IsDefined(void) const
			{
				for (int i = 0; i < Children.Length(); i++) if (!Children[i].IsDefined()) return false;
				return Position.IsDefined() && Opacity.IsDefined();
			}
			Engine::UI::Shape * FrameShape::Initialize(IArgumentProvider * provider) const
			{
				SafePointer<Engine::UI::FrameShape> New = new Engine::UI::FrameShape(Position.Initialize(provider), RenderMode, Opacity.Initialize(provider));
				for (int i = 0; i < Children.Length(); i++) {
					SafePointer<Engine::UI::Shape> child = Children[i].Initialize(provider);
					New->Children.Append(child);
				}
				New->Retain();
				return New;
			}
			ControlTemplate::ControlTemplate(ControlReflectedBase * properties) : Properties(properties) {}
			ControlTemplate::~ControlTemplate(void) { delete Properties; }
			string ControlTemplate::ToString(void) const { return Properties->GetTemplateClass(); }
			ControlReflectedBase::~ControlReflectedBase(void) {}
		}
		InterfaceTemplate::InterfaceTemplate(void) {}
		InterfaceTemplate::~InterfaceTemplate(void) {}
		string InterfaceTemplate::ToString(void) const { return L"InterfaceTemplate"; }
	}
}