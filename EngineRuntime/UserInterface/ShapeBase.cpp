#include "ShapeBase.h"
#include "Templates.h"
#include "../Math/Color.h"

namespace Engine
{
	namespace UI
	{
		double Zoom = 1.0;

		Coordinate::Coordinate(void) noexcept {}
		Coordinate::Coordinate(int shift) noexcept : Absolute(shift), Anchor(0.0), Zoom(0.0) {}
		Coordinate::Coordinate(int shift, double zoom, double anchor) noexcept : Absolute(shift), Anchor(anchor), Zoom(zoom) {}
		Coordinate & Coordinate::operator+=(const Coordinate & a) noexcept { *this = *this + a; return *this; }
		Coordinate & Coordinate::operator-=(const Coordinate & a) noexcept { *this = *this - a; return *this; }
		Coordinate & Coordinate::operator*=(double a) noexcept { *this = *this * a; return *this; }
		Coordinate & Coordinate::operator/=(double a) noexcept { *this = *this / a; return *this; }
		Coordinate Coordinate::operator-(void) const noexcept  { return Coordinate(-Absolute, -Zoom, -Anchor); }
		Coordinate Coordinate::Right() noexcept { return Coordinate(0, 0.0, 1.0); }
		Coordinate Coordinate::Bottom() noexcept { return Coordinate(0, 0.0, 1.0); }
		Coordinate operator+(const Coordinate & a, const Coordinate & b) noexcept { return Coordinate(a.Absolute + b.Absolute, a.Zoom + b.Zoom, a.Anchor + b.Anchor); }
		Coordinate operator-(const Coordinate & a, const Coordinate & b) noexcept { return Coordinate(a.Absolute - b.Absolute, a.Zoom - b.Zoom, a.Anchor - b.Anchor); }
		Coordinate operator*(const Coordinate & a, double b) noexcept { return Coordinate(int(a.Absolute * b), a.Zoom * b, a.Anchor * b); }
		Coordinate operator*(double b, const Coordinate & a) noexcept { return Coordinate(int(a.Absolute * b), a.Zoom * b, a.Anchor * b); }
		Coordinate operator/(const Coordinate & a, double b) noexcept { return Coordinate(int(a.Absolute / b), a.Zoom / b, a.Anchor / b); }
		bool operator==(const Coordinate & a, const Coordinate & b) noexcept { return a.Absolute == b.Absolute && a.Zoom == b.Zoom && a.Anchor == b.Anchor; }
		bool operator!=(const Coordinate & a, const Coordinate & b) noexcept { return a.Absolute != b.Absolute || a.Zoom != b.Zoom || a.Anchor != b.Anchor; }
		Rectangle operator+(const Rectangle & a, const Rectangle & b) noexcept { Rectangle Result = a; Result += b; return Result; }
		Rectangle operator-(const Rectangle & a, const Rectangle & b) noexcept { Rectangle Result = a; Result -= b; return Result; }
		Rectangle operator*(const Rectangle & a, double b) noexcept { Rectangle Result = a; Result *= b; return Result; }
		Rectangle operator*(double b, const Rectangle & a) noexcept { Rectangle Result = a; Result *= b; return Result; }
		Rectangle operator/(const Rectangle & a, double b) noexcept { Rectangle Result = a; Result /= b; return Result; }
		bool operator==(const Rectangle & a, const Rectangle & b) noexcept { return a.Left == b.Left && a.Top == b.Top && a.Right == b.Right && a.Bottom == b.Bottom; }
		bool operator!=(const Rectangle & a, const Rectangle & b) noexcept { return a.Left != b.Left || a.Top != b.Top || a.Right != b.Right || a.Bottom != b.Bottom; }
		bool operator==(const Point & a, const Point & b) noexcept { return a.x == b.x && a.y == b.y; }
		bool operator!=(const Point & a, const Point & b) noexcept { return a.x != b.x || a.y != b.y; }
		bool operator==(const Box & a, const Box & b) noexcept { return a.Left == b.Left && a.Top == b.Top && a.Right == b.Right && a.Bottom == b.Bottom; }
		bool operator!=(const Box & a, const Box & b) noexcept { return a.Left != b.Left || a.Top != b.Top || a.Right != b.Right || a.Bottom != b.Bottom; }
		bool operator==(const Color & a, const Color & b) noexcept { return a.Value == b.Value; }
		bool operator!=(const Color & a, const Color & b) noexcept { return a.Value != b.Value; }
		bool operator==(const GradientPoint & a, const GradientPoint & b) noexcept { return a.Color == b.Color && a.Position == b.Position; }
		bool operator!=(const GradientPoint & a, const GradientPoint & b) noexcept { return a.Color != b.Color || a.Position != b.Position; }
		Rectangle::Rectangle(void) noexcept {}
		Rectangle::Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom) noexcept : Left(left), Top(top), Right(right), Bottom(bottom) {}
		Rectangle & Rectangle::operator+=(const Rectangle & a) noexcept { Left += a.Left; Top += a.Top; Right += a.Right; Bottom += a.Bottom; return *this; }
		Rectangle & Rectangle::operator-=(const Rectangle & a) noexcept { Left -= a.Left; Top -= a.Top; Right -= a.Right; Bottom -= a.Bottom; return *this; }
		Rectangle & Rectangle::operator*=(double a) noexcept { Left *= a; Top *= a; Right *= a; Bottom *= a; return *this; }
		Rectangle & Rectangle::operator/=(double a) noexcept { Left /= a; Top /= a; Right /= a; Bottom /= a; return *this; }
		Rectangle Rectangle::operator-(void) const noexcept { return Rectangle(-Left, -Top, -Right, -Bottom); }
		bool Rectangle::IsValid(void) const noexcept { return Left.Anchor == Left.Anchor; }
		Rectangle Rectangle::Entire() noexcept { return Rectangle(0, 0, Coordinate::Right(), Coordinate::Bottom()); }
		Rectangle Rectangle::Invalid() noexcept { double z = 0.0, nan = z / z; return Rectangle(Coordinate(0, 0.0, nan), Coordinate(0, 0.0, nan), Coordinate(0, 0.0, nan), Coordinate(0, 0.0, nan)); }
		Point::Point(void) noexcept {}
		Point::Point(int X, int Y) noexcept : x(X), y(Y) {}
		Box::Box(void) noexcept {}
		Box::Box(const Rectangle & source, const Box & outer) noexcept
		{
			double Width = outer.Right - outer.Left;
			double Height = outer.Bottom - outer.Top;
			Left = outer.Left + source.Left.Absolute + int(source.Left.Anchor * Width) + int(source.Left.Zoom * Zoom);
			Right = outer.Left + source.Right.Absolute + int(source.Right.Anchor * Width) + int(source.Right.Zoom * Zoom);
			Top = outer.Top + source.Top.Absolute + int(source.Top.Anchor * Height) + int(source.Top.Zoom * Zoom);
			Bottom = outer.Top + source.Bottom.Absolute + int(source.Bottom.Anchor * Height) + int(source.Bottom.Zoom * Zoom);
		}
		Box::Box(int left, int top, int right, int bottom) noexcept : Left(left), Top(top), Right(right), Bottom(bottom) {}
		bool Box::IsInside(const Point & p) const noexcept { return p.x >= Left && p.x < Right && p.y >= Top && p.y < Bottom; }
		Box Box::Intersect(const Box & a, const Box & b) noexcept
		{
			return Box(max(a.Left, b.Left), max(a.Top, b.Top), min(a.Right, b.Right), min(a.Bottom, b.Bottom));
		}
		Color::Color(void) noexcept {}
		Color::Color(uint8 sr, uint8 sg, uint8 sb, uint8 sa) noexcept : r(sr), g(sg), b(sb), a(sa) {}
		Color::Color(int sr, int sg, int sb, int sa) noexcept : r(sr), g(sg), b(sb), a(sa) {}
		Color::Color(float sr, float sg, float sb, float sa) noexcept : r(max(min(int(sr * 255.0f), 255), 0)), g(max(min(int(sg * 255.0f), 255), 0)), b(max(min(int(sb * 255.0f), 255), 0)), a(max(min(int(sa * 255.0f), 255), 0)) {}
		Color::Color(double sr, double sg, double sb, double sa) noexcept : r(max(min(int(sr * 255.0), 255), 0)), g(max(min(int(sg * 255.0), 255), 0)), b(max(min(int(sb * 255.0), 255), 0)), a(max(min(int(sa * 255.0), 255), 0)) {}
		Color::Color(uint32 code) noexcept : Value(code) {}
		Color::operator uint32(void) const noexcept { return Value; }
		FrameShape::FrameShape(const Rectangle & position) : Children(0x10), RenderMode(FrameRenderMode::Normal), Opacity(1.0) { Position = position; }
		FrameShape::FrameShape(const Rectangle & position, FrameRenderMode mode, double opacity) : Children(0x10), RenderMode(mode), Opacity(opacity) { Position = position; }
		FrameShape::~FrameShape(void) {}
		void FrameShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			if (RenderMode == FrameRenderMode::Normal) {
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(Device, my);
			} else if (RenderMode == FrameRenderMode::Clipping) {
				Device->PushClip(my);
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(Device, my);
				Device->PopClip();
			} else if (RenderMode == FrameRenderMode::Layering) {
				Device->BeginLayer(my, Opacity);
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(Device, my);
				Device->EndLayer();
			}
		}
		void FrameShape::ClearCache(void) noexcept { for (int i = Children.Length() - 1; i >= 0; i--) Children[i].ClearCache(); }
		Shape * FrameShape::Clone(void) const
		{
			SafePointer<FrameShape> clone = new FrameShape(Position, RenderMode, Opacity);
			for (int i = 0; i < Children.Length(); i++) {
				SafePointer<Shape> child = Children[i].Clone();
				clone->Children.Append(child);
			}
			clone->Retain();
			return clone;
		}
		string FrameShape::ToString(void) const { return L"FrameShape"; }
		GradientPoint::GradientPoint(void) noexcept {}
		GradientPoint::GradientPoint(const UI::Color & color) noexcept : Color(color), Position(0.0) {}
		GradientPoint::GradientPoint(const UI::Color & color, double position) noexcept : Color(color), Position(position) {}
		BarShape::BarShape(const Rectangle & position, const Color & color) : GradientAngle(0.0) { Position = position; Gradient << GradientPoint(color); }
		BarShape::BarShape(const Rectangle & position, const Array<GradientPoint>& gradient, double angle) : Gradient(gradient), GradientAngle(angle) { Position = position; }
		BarShape::~BarShape(void) {}
		void BarShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateBarRenderingInfo(Gradient, GradientAngle));
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->RenderBar(Info, my);
		}
		void BarShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * BarShape::Clone(void) const { return new BarShape(Position, Gradient, GradientAngle); }
		string BarShape::ToString(void) const { return L"BarShape"; }
		Drawing::ICanvasRenderingDevice * IRenderingDevice::QueryCanvasDevice(void) noexcept { return 0; }
		IRenderingDevice::~IRenderingDevice(void) {}
		IBarRenderingInfo::~IBarRenderingInfo(void) {}
		IBlurEffectRenderingInfo::~IBlurEffectRenderingInfo(void) {}
		BlurEffectShape::BlurEffectShape(const Rectangle & position, double power) : BlurPower(power) { Position = position; }
		BlurEffectShape::~BlurEffectShape(void) {}
		void BlurEffectShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateBlurEffectRenderingInfo(BlurPower));
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->ApplyBlur(Info, my);
		}
		void BlurEffectShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * BlurEffectShape::Clone(void) const { return new BlurEffectShape(Position, BlurPower); }
		string BlurEffectShape::ToString(void) const { return L"BlurEffectShape"; }
		ITextureRenderingInfo::~ITextureRenderingInfo(void) {}
		TextureShape::TextureShape(const Rectangle & position, ITexture * texture, const Rectangle & take_from, TextureRenderMode mode) : Texture(texture), From(take_from), Mode(mode) { Position = position; if (Texture) Texture->Retain(); }
		TextureShape::~TextureShape(void) { if (Texture) Texture->Release(); }
		void TextureShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (Texture) {
				if (!Info) {
					FromBox = Box(From, Box(0, 0, Texture->GetWidth(), Texture->GetHeight()));
					Info.SetReference(Device->CreateTextureRenderingInfo(Texture, FromBox, Mode == TextureRenderMode::FillPattern));
				}
				Box to(Position, Outer);
				if (to.Right < to.Left || to.Bottom < to.Top) return;
				if (Mode == TextureRenderMode::Fit) {
					double ta = double(to.Right - to.Left) / double(to.Bottom - to.Top);
					double fa = double(FromBox.Right - FromBox.Left) / double(FromBox.Bottom - FromBox.Top);
					if (ta > fa) {
						int adjx = int(double(to.Bottom - to.Top) * fa);
						int xc = (to.Right + to.Left) >> 1;
						to.Left = xc - (adjx >> 1);
						to.Right = xc + (adjx >> 1);
					} else if (fa > ta) {
						int adjy = int(double(to.Right - to.Left) / fa);
						int yc = (to.Bottom + to.Top) >> 1;
						to.Top = yc - (adjy >> 1);
						to.Bottom = yc + (adjy >> 1);
					}
				} else if (Mode == TextureRenderMode::AsIs) {
					int sx = FromBox.Right - FromBox.Left;
					int sy = FromBox.Bottom - FromBox.Top;
					if (to.Right - to.Left < sx || to.Bottom - to.Top < sy) return;
					to.Right = to.Left = (to.Right + to.Left) / 2;
					to.Bottom = to.Top = (to.Bottom + to.Top) / 2;
					int rx = sx / 2, ry = sy / 2;
					to.Left -= rx; to.Top -= ry;
					to.Right += sx - rx; to.Bottom += sy - ry;
				}
				Device->RenderTexture(Info, to);
			}
		}
		void TextureShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * TextureShape::Clone(void) const { return new TextureShape(Position, Texture, From, Mode); }
		string TextureShape::ToString(void) const { return L"TextureShape"; }
		ITextRenderingInfo::~ITextRenderingInfo(void) {}
		TextShape::TextShape(const Rectangle & position, const string & text, IFont * font, const Color & color, TextHorizontalAlign horizontal_align, TextVerticalAlign vertical_align) :
			Text(text), Font(font), TextColor(color), halign(horizontal_align), valign(vertical_align) { Position = position; if (Font) Font->Retain(); }
		TextShape::~TextShape(void) { if (Font) Font->Release(); }
		void TextShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateTextRenderingInfo(Font, Text, int(halign), int(valign), TextColor));
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->RenderText(Info, my, true);
		}
		void TextShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * TextShape::Clone(void) const { return new TextShape(Position, Text, Font, TextColor, halign, valign); }
		string TextShape::ToString(void) const { return L"TextShape"; }
		ILineRenderingInfo::~ILineRenderingInfo(void) {}
		LineShape::LineShape(const Rectangle & position, const Color & color, bool dotted) : LineColor(color), Dotted(dotted) { Position = position; }
		LineShape::~LineShape(void) {}
		void LineShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateLineRenderingInfo(LineColor, Dotted));
			Box my(Position, Outer);
			Device->RenderLine(Info, my);
		}
		void LineShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * LineShape::Clone(void) const { return new LineShape(Position, LineColor, Dotted); }
		string LineShape::ToString(void) const { return L"LineShape"; }
		IInversionEffectRenderingInfo::~IInversionEffectRenderingInfo(void) {}
		InversionEffectShape::InversionEffectShape(const Rectangle & position) { Position = position; }
		InversionEffectShape::~InversionEffectShape(void) {}
		void InversionEffectShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateInversionEffectRenderingInfo());
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->ApplyInversion(Info, my, false);
		}
		void InversionEffectShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * InversionEffectShape::Clone(void) const { return new InversionEffectShape(Position); }
		string InversionEffectShape::ToString(void) const { return L"InversionEffectShape"; }
	}
}