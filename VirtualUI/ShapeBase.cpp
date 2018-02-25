#include "ShapeBase.h"

namespace Engine
{
	namespace UI
	{
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
		Coordinate operator*(const Coordinate & a, double b) { return Coordinate(a.Absolute * b, a.Zoom * b, a.Anchor * b); }
		Coordinate operator*(double b, const Coordinate & a) { return Coordinate(a.Absolute * b, a.Zoom * b, a.Anchor * b); }
		Coordinate operator/(const Coordinate & a, double b) { return Coordinate(a.Absolute / b, a.Zoom / b, a.Anchor / b); }
		bool operator==(const Coordinate & a, const Coordinate & b) { return a.Absolute == b.Absolute && a.Zoom == b.Zoom && a.Anchor == b.Anchor; }
		bool operator!=(const Coordinate & a, const Coordinate & b) { return a.Absolute != b.Absolute || a.Zoom != b.Zoom || a.Anchor != b.Anchor; }
		bool operator==(const Rectangle & a, const Rectangle & b) { return a.Left == b.Left && a.Top == b.Top && a.Right == b.Right && a.Bottom == b.Bottom; }
		bool operator!=(const Rectangle & a, const Rectangle & b) { return a.Left != b.Left || a.Top != b.Top || a.Right != b.Right || a.Bottom != b.Bottom; }
		bool operator==(const Point & a, const Point & b) { return a.x == b.x && a.y == b.y; }
		bool operator!=(const Point & a, const Point & b) { return a.x != b.x || a.y != b.y; }
		bool operator==(const Box & a, const Box & b) { return a.Left == b.Left && a.Top == b.Top && a.Right == b.Right && a.Bottom == b.Bottom; }
		bool operator!=(const Box & a, const Box & b) { return a.Left != b.Left || a.Top != b.Top || a.Right != b.Right || a.Bottom != b.Bottom; }
		Rectangle::Rectangle(void) {}
		Rectangle::Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}
		Point::Point(void) {}
		Point::Point(int X, int Y) : x(X), y(Y) {}
		Box::Box(void) {}
		Box::Box(const Rectangle & source, const Box & outer)
		{
			throw Exception();
#pragma message ("METHOD NOT IMPLEMENTED, IMPLEMENT IT!")

			/*double Width = Outer.Right - Outer.Left;
			double Height = Outer.Bottom - Outer.Top;
			Left = Outer.Left + Src.Left.Shift + int(Src.Left.WidthMul * Width) + int(Src.Left.ZoomMul * Properties.UIZoom);
			Right = Outer.Left + Src.Right.Shift + int(Src.Right.WidthMul * Width) + int(Src.Right.ZoomMul * Properties.UIZoom);
			Top = Outer.Top + Src.Top.Shift + int(Src.Top.WidthMul * Height) + int(Src.Top.ZoomMul * Properties.UIZoom);
			Bottom = Outer.Top + Src.Bottom.Shift + int(Src.Bottom.WidthMul * Height) + int(Src.Bottom.ZoomMul * Properties.UIZoom);*/
		}
		Box::Box(int left, int top, int right, int bottom) : Left(left), Top(top), Right(right), Bottom(bottom) {}
		bool Box::IsInside(const Point & p) const { return p.x >= Left && p.x < Right && p.y >= Top && p.y < Bottom; }
	}
}