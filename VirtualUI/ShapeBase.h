#pragma once

#include "EngineBase.h"
#include "Reflection.h"

namespace Engine
{
	namespace UI
	{
		extern double Zoom;

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

			static Rectangle Entire();
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
		public:
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
		public:
			union {
				struct { uint8 r, g, b, a; };
				uint32 Value;
			};

			Color(void);
			Color(uint8 sr, uint8 sg, uint8 sb, uint8 sa = 0xFF);
			Color(int sr, int sg, int sb, int sa = 0xFF);
			Color(float sr, float sg, float sb, float sa = 1.0);
			Color(double sr, double sg, double sb, double sa = 1.0);
			Color(uint32 code);

			bool friend operator == (const Color & a, const Color & b);
			bool friend operator != (const Color & a, const Color & b);
		};
		class GradientPoint
		{
		public:
			Color Color;
			double Position;

			GradientPoint(void);
			GradientPoint(const UI::Color & color);
			GradientPoint(const UI::Color & color, double position);

			bool friend operator == (const GradientPoint & a, const GradientPoint & b);
			bool friend operator != (const GradientPoint & a, const GradientPoint & b);
		};

		class FrameShape;
		class BarShape;

		class IBarRenderingInfo
		{
		public:
			virtual ~IBarRenderingInfo(void);
		};
		class ITextureRenderingInfo
		{
		public:
#pragma message ("INTERFACE NOT DEFINED, DEFINE IT!")
		};
		class ITextRenderingInfo
		{
		public:
#pragma message ("INTERFACE NOT DEFINED, DEFINE IT!")
		};
		class IRenderingDevice
		{
		public:
			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint> & gradient, double angle) = 0;
			//virtual ITextureRenderingInfo * CreateTextureRenderingInfo(/* неведомый */) = 0;
			//virtual ITextRenderingInfo * CreateTextRenderingInfo(/* неведомый */) = 0;
			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) = 0;

			virtual void PushClip(const Box & Rect) = 0;
			virtual void PopClip(void) = 0;
			virtual void BeginLayer(const Box & Rect, double Opacity) = 0;
			virtual void EndLayer(void) = 0;
			virtual ~IRenderingDevice(void);
#pragma message ("INTERFACE IS NOT COMPLETE!")
		};

		class Shape : public Object
		{
		public:
			Rectangle Position;
			virtual void Render(IRenderingDevice * Device, const Box & Outer) = 0;
		};
		class FrameShape : public Shape
		{
		public:
			enum class FrameRenderMode { Normal = 0, Clipping = 1, Layering = 2 };
			ObjectArray<Shape> Children;
			FrameRenderMode RenderMode;
			double Opacity;
			FrameShape(const Rectangle position);
			FrameShape(const Rectangle position, FrameRenderMode mode, double opacity = 1.0);
			~FrameShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) override;
		};
		class BarShape : public Shape
		{
			IBarRenderingInfo * Info;
			Array<GradientPoint> Gradient;
			double GradientAngle;
		public:
			BarShape(const Rectangle position, const Color & color);
			BarShape(const Rectangle position, const Array<GradientPoint> & gradient, double angle);
			~BarShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) override;
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