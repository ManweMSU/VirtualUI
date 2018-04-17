#include "Templates.h"

namespace Engine
{
	namespace UI
	{
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
				return int(Syntax::Math::CalculateExpressionInteger(expression, &ExpressionArgumentProvider(provider)));
			}
			template <> double ResolveExpression<double>(const string & expression, IArgumentProvider * provider)
			{
				return Syntax::Math::CalculateExpressionDouble(expression, &ExpressionArgumentProvider(provider));
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
			GradientPoint::GradientPoint(const Engine::UI::GradientPoint & gradient) : PointColor(gradient.Color), Position(gradient.Position) {}
			GradientPoint::GradientPoint(const BasicTemplate<Color>& color, const BasicTemplate<double>& position) : PointColor(color), Position(position) {}
			bool GradientPoint::IsDefined(void) const { return PointColor.IsDefined() && Position.IsDefined(); }
			Engine::UI::GradientPoint GradientPoint::Initialize(IArgumentProvider * provider) const { return Engine::UI::GradientPoint(PointColor.Initialize(provider), Position.Initialize(provider)); }

			BarShape::BarShape(void) : Gradient(0x4), GradientAngle(0.0) { Position = Engine::UI::Rectangle::Entire(); }
			BarShape::~BarShape(void) {}
			string BarShape::ToString(void) const { return L"Templates::BarShape"; }
			bool BarShape::IsDefined(void) const
			{
				for (int i = 0; i < Gradient.Length(); i++) if (!Gradient[i].IsDefined()) return false;
				return GradientAngle.IsDefined() && Position.IsDefined();
			}
			Engine::UI::Shape * BarShape::Initialize(IArgumentProvider * provider) const
			{
				Array<Engine::UI::GradientPoint> init(1);
				init.SetLength(Gradient.Length());
				for (int i = 0; i < Gradient.Length(); i++) init[i] = Gradient[i].Initialize(provider);
				return new Engine::UI::BarShape(Position.Initialize(provider), init, GradientAngle.Initialize(provider));
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
				SafePointer<ITexture> Query = Texture.Initialize(provider);
				return new Engine::UI::TextureShape(Position.Initialize(provider), Query, From.Initialize(provider), RenderMode);
			}
			TextShape::TextShape(void) : HorizontalAlign(Engine::UI::TextShape::TextHorizontalAlign::Left), VerticalAlign(Engine::UI::TextShape::TextVerticalAlign::Top), TextColor(Engine::UI::Color(0)) { Position = Engine::UI::Rectangle::Entire(); }
			TextShape::~TextShape(void) {}
			string TextShape::ToString(void) const { return L"Templates::TextShape"; }
			bool TextShape::IsDefined(void) const
			{
				return Position.IsDefined() && TextColor.IsDefined() && Text.IsDefined() && Font.IsDefined();
			}
			Engine::UI::Shape * TextShape::Initialize(IArgumentProvider * provider) const
			{
				SafePointer<IFont> Query = Font.Initialize(provider);
				return new Engine::UI::TextShape(Position.Initialize(provider), Text.Initialize(provider), Query, TextColor.Initialize(provider), HorizontalAlign, VerticalAlign);
			}
			LineShape::LineShape(void) : Dotted(false), LineColor(Engine::UI::Color(0)) { Position = Engine::UI::Rectangle::Entire(); }
			LineShape::~LineShape(void) {}
			string LineShape::ToString(void) const { return L"Templates::LineShape"; }
			bool LineShape::IsDefined(void) const
			{
				return Position.IsDefined() && LineColor.IsDefined();
			}
			Engine::UI::Shape * LineShape::Initialize(IArgumentProvider * provider) const
			{
				return new Engine::UI::LineShape(Position.Initialize(provider), LineColor.Initialize(provider), Dotted);
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
				for (int i = 0; i < Children.Length(); i++) New->Children << Children[i].Initialize(provider);
				New->Retain();
				return New;
			}
			ControlTemplate::ControlTemplate(ControlReflectedBase * properties) : Properties(properties) {}
			ControlTemplate::~ControlTemplate(void) { delete Properties; }
			string ControlTemplate::ToString(void) const { return Properties->GetTemplateClass(); }
			ControlReflectedBase::~ControlReflectedBase(void) {}
		}
		InterfaceTemplate::InterfaceTemplate(void) : Texture(0x20), Font(0x10), Application(0x20), Dialog(0x10) {}
		InterfaceTemplate::~InterfaceTemplate(void) {}
		string InterfaceTemplate::ToString(void) const { return L"InterfaceTemplate"; }
	}
}