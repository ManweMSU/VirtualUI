#include "GraphicsBase.h"

#include "../Interfaces/SystemGraphics.h"

namespace Engine
{
	namespace UI
	{
		double CurrentScaleFactor = 1.0;

		Coordinate::Coordinate(void) noexcept {}
		Coordinate::Coordinate(int shift) noexcept : Absolute(shift), Anchor(0.0), Zoom(0.0) {}
		Coordinate::Coordinate(int shift, double zoom, double anchor) noexcept : Absolute(shift), Anchor(anchor), Zoom(zoom) {}
		Coordinate & Coordinate::operator+=(const Coordinate & a) noexcept { *this = *this + a; return *this; }
		Coordinate & Coordinate::operator-=(const Coordinate & a) noexcept { *this = *this - a; return *this; }
		Coordinate & Coordinate::operator*=(double a) noexcept { *this = *this * a; return *this; }
		Coordinate & Coordinate::operator/=(double a) noexcept { *this = *this / a; return *this; }
		Coordinate Coordinate::operator-(void) const noexcept { return Coordinate(-Absolute, -Zoom, -Anchor); }
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
			Left = outer.Left + source.Left.Absolute + int(source.Left.Anchor * Width) + int(source.Left.Zoom * CurrentScaleFactor);
			Right = outer.Left + source.Right.Absolute + int(source.Right.Anchor * Width) + int(source.Right.Zoom * CurrentScaleFactor);
			Top = outer.Top + source.Top.Absolute + int(source.Top.Anchor * Height) + int(source.Top.Zoom * CurrentScaleFactor);
			Bottom = outer.Top + source.Bottom.Absolute + int(source.Bottom.Anchor * Height) + int(source.Bottom.Zoom * CurrentScaleFactor);
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

		IObjectFactory * CreateObjectFactory(void) { return Graphics::CreateSystemDrawingObjectFactory(); }
	}
}