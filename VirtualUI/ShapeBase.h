#pragma once

#include "EngineBase.h"
#include "Reflection.h"

namespace Engine
{
	namespace UI
	{
		class Coordinate
		{
		public:
			double Anchor, Zoom;
			int Absolute;

			Coordinate(void);
			Coordinate(int shift);
			Coordinate(int shift, double zoom, double anchor);

			Coordinate friend operator + (const Coordinate & a, const Coordinate & b);
			Coordinate friend operator - (const Coordinate & a, const Coordinate & b);
			Coordinate friend operator * (const Coordinate & a, double b);
			Coordinate friend operator * (double b, const Coordinate & a);
			Coordinate friend operator / (const Coordinate & a, double b);

			Coordinate & operator += (const Coordinate & a);
			Coordinate & operator -= (const Coordinate & a);
			Coordinate & operator *= (double a);
			Coordinate & operator /= (double a);
			Coordinate operator - (void) const;

			bool friend operator == (const Coordinate & a, const Coordinate & b);
			bool friend operator != (const Coordinate & a, const Coordinate & b);

			static Coordinate Right();
			static Coordinate Bottom();
		};
		class Rectangle
		{
		public:
			Coordinate Left, Top, Right, Bottom;

			Rectangle(void);
			Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom);

			bool friend operator == (const Rectangle & a, const Rectangle & b);
			bool friend operator != (const Rectangle & a, const Rectangle & b);
		};
		class Point
		{
		public:
			int x, y;

			Point(void);
			Point(int X, int Y);

			bool friend operator == (const Point & a, const Point & b);
			bool friend operator != (const Point & a, const Point & b);
		};
		class Box
		{
			int Left, Top, Right, Bottom;

			Box(void);
			Box(const Rectangle & source, const Box & outer);
			Box(int left, int top, int right, int bottom);

			bool friend operator == (const Box & a, const Box & b);
			bool friend operator != (const Box & a, const Box & b);

			bool IsInside(const Point & p) const;
		};
		class Color
		{

		};
	}
	namespace Reflection
	{
		using namespace ::Engine::UI;
		__TYPE_REFLECTION(Coordinate);
		__TYPE_REFLECTION(Rectangle);
		__TYPE_REFLECTION(Color);
	}
}