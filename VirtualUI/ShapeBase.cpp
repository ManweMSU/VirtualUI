#include "ShapeBase.h"

namespace Engine
{
	namespace UI
	{
		double Zoom = 0;

		Coordinate::Coordinate(void) {}
		Coordinate::Coordinate(int shift) : Absolute(shift), Anchor(0.0), Zoom(0.0) {}
		Coordinate::Coordinate(int shift, double zoom, double anchor) : Absolute(shift), Anchor(anchor), Zoom(zoom) {}
		Coordinate & Coordinate::operator+=(const Coordinate & a) { *this = *this + a; return *this; }
		Coordinate & Coordinate::operator-=(const Coordinate & a) { *this = *this - a; return *this; }
		Coordinate & Coordinate::operator*=(double a) { *this = *this * a; return *this; }
		Coordinate & Coordinate::operator/=(double a) { *this = *this / a; return *this; }
		Coordinate Coordinate::operator-(void) const { return Coordinate(-Absolute, -Zoom, -Anchor); }
		Coordinate Coordinate::Right() { return Coordinate(0, 0.0, 1.0); }
		Coordinate Coordinate::Bottom() { return Coordinate(0, 0.0, 1.0); }
		Coordinate operator+(const Coordinate & a, const Coordinate & b) { return Coordinate(a.Absolute + b.Absolute, a.Zoom + b.Zoom, a.Anchor + b.Anchor); }
		Coordinate operator-(const Coordinate & a, const Coordinate & b) { return Coordinate(a.Absolute - b.Absolute, a.Zoom - b.Zoom, a.Anchor - b.Anchor); }
		Coordinate operator*(const Coordinate & a, double b) { return Coordinate(int(a.Absolute * b), a.Zoom * b, a.Anchor * b); }
		Coordinate operator*(double b, const Coordinate & a) { return Coordinate(int(a.Absolute * b), a.Zoom * b, a.Anchor * b); }
		Coordinate operator/(const Coordinate & a, double b) { return Coordinate(int(a.Absolute / b), a.Zoom / b, a.Anchor / b); }
		bool operator==(const Coordinate & a, const Coordinate & b) { return a.Absolute == b.Absolute && a.Zoom == b.Zoom && a.Anchor == b.Anchor; }
		bool operator!=(const Coordinate & a, const Coordinate & b) { return a.Absolute != b.Absolute || a.Zoom != b.Zoom || a.Anchor != b.Anchor; }
		bool operator==(const Rectangle & a, const Rectangle & b) { return a.Left == b.Left && a.Top == b.Top && a.Right == b.Right && a.Bottom == b.Bottom; }
		bool operator!=(const Rectangle & a, const Rectangle & b) { return a.Left != b.Left || a.Top != b.Top || a.Right != b.Right || a.Bottom != b.Bottom; }
		bool operator==(const Point & a, const Point & b) { return a.x == b.x && a.y == b.y; }
		bool operator!=(const Point & a, const Point & b) { return a.x != b.x || a.y != b.y; }
		bool operator==(const Box & a, const Box & b) { return a.Left == b.Left && a.Top == b.Top && a.Right == b.Right && a.Bottom == b.Bottom; }
		bool operator!=(const Box & a, const Box & b) { return a.Left != b.Left || a.Top != b.Top || a.Right != b.Right || a.Bottom != b.Bottom; }
		bool operator==(const Color & a, const Color & b) { return a.Value == b.Value; }
		bool operator!=(const Color & a, const Color & b) { return a.Value != b.Value; }
		bool operator==(const GradientPoint & a, const GradientPoint & b) { return a.Color == b.Color && a.Position == b.Position; }
		bool operator!=(const GradientPoint & a, const GradientPoint & b) { return a.Color != b.Color || a.Position != b.Position; }
		Rectangle::Rectangle(void) {}
		Rectangle::Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}
		Point::Point(void) {}
		Point::Point(int X, int Y) : x(X), y(Y) {}
		Box::Box(void) {}
		Box::Box(const Rectangle & source, const Box & outer)
		{
			double Width = outer.Right - outer.Left;
			double Height = outer.Bottom - outer.Top;
			Left = outer.Left + source.Left.Absolute + int(source.Left.Anchor * Width) + int(source.Left.Zoom * Zoom);
			Right = outer.Left + source.Right.Absolute + int(source.Right.Anchor * Width) + int(source.Right.Zoom * Zoom);
			Top = outer.Top + source.Top.Absolute + int(source.Top.Anchor * Height) + int(source.Top.Zoom * Zoom);
			Bottom = outer.Top + source.Bottom.Absolute + int(source.Bottom.Anchor * Height) + int(source.Bottom.Zoom * Zoom);
		}
		Box::Box(int left, int top, int right, int bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}
		bool Box::IsInside(const Point & p) const { return p.x >= Left && p.x < Right && p.y >= Top && p.y < Bottom; }
		Color::Color(void) {}
		Color::Color(uint8 sr, uint8 sg, uint8 sb, uint8 sa) : r(sr), g(sg), b(sb), a(sa) {}
		Color::Color(float sr, float sg, float sb, float sa) : r(max(min(int(sr * 255.0f), 255), 0)), g(max(min(int(sg * 255.0f), 255), 0)), b(max(min(int(sb * 255.0f), 0), 0)), a(max(min(int(sa * 255.0f), 0), 0)) {}
		Color::Color(double sr, double sg, double sb, double sa) : r(max(min(int(sr * 255.0), 255), 0)), g(max(min(int(sg * 255.0), 255), 0)), b(max(min(int(sb * 255.0), 0), 0)), a(max(min(int(sa * 255.0), 0), 0)) {}
		Color::Color(uint32 code) : Value(code) {}
		FrameShape::FrameShape(const Rectangle position) : Children(0x10) { Position = position; }
		FrameShape::~FrameShape(void) {}
		void FrameShape::Render(IRenderingDevice * Device, const Box & Outer)
		{
			Box my(Position, Outer);
			for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(Device, my);
		}
		GradientPoint::GradientPoint(void) {}
		GradientPoint::GradientPoint(const UI::Color & color) : Color(color), Position(0.0) {}
		GradientPoint::GradientPoint(const UI::Color & color, double position) : Color(color), Position(position) {}
		BarShape::BarShape(const Rectangle position, const Color & color) : Info(0), GradientAngle(0.0) { Position = position; Gradient << GradientPoint(color); }
		BarShape::BarShape(const Rectangle position, const Array<GradientPoint>& gradient, double angle) : Info(0), Gradient(gradient), GradientAngle(angle) { Position = position; }
		BarShape::~BarShape(void) { delete Info; }
		void BarShape::Render(IRenderingDevice * Device, const Box & Outer)
		{
			if (!Info) Info = Device->CreateBarRenderingInfo(Gradient, GradientAngle);
			Box my(Position, Outer);
			Device->RenderBar(Info, my);
		}
	}
}