#pragma once

#include "../EngineBase.h"
#include "../Streaming.h"
#include "../ImageCodec/CodecBase.h"

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

			bool IsValid(void) const;

			static Rectangle Entire();
			static Rectangle Invalid();
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

			operator uint32 (void) const;

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

		class IRenderingDevice;
		class FrameShape;
		class BarShape;

		class IBarRenderingInfo : public Object
		{
		public:
			virtual ~IBarRenderingInfo(void);
		};
		class IBlurEffectRenderingInfo : public Object
		{
		public:
			virtual ~IBlurEffectRenderingInfo(void);
		};
		class IInversionEffectRenderingInfo : public Object
		{
		public:
			virtual ~IInversionEffectRenderingInfo(void);
		};
		class ITextureRenderingInfo : public Object
		{
		public:
			virtual ~ITextureRenderingInfo(void);
		};
		class ITextRenderingInfo : public Object
		{
		public:
			virtual void GetExtent(int & width, int & height) = 0;
			virtual void SetHighlightColor(const Color & color) = 0;
			virtual void HighlightText(int Start, int End) = 0;
			virtual int TestPosition(int point) = 0;
			virtual int EndOfChar(int Index) = 0;
			virtual ~ITextRenderingInfo(void);
		};
		class ILineRenderingInfo : public Object
		{
		public:
			virtual ~ILineRenderingInfo(void);
		};

		class ITexture : public Object
		{
		public:
			virtual int GetWidth(void) const = 0;
			virtual int GetHeight(void) const = 0;
			virtual bool IsDynamic(void) const = 0;
			virtual void Reload(IRenderingDevice * Device, Streaming::Stream * Source) = 0;
			virtual void Reload(IRenderingDevice * Device, Codec::Image * Source) = 0;
			virtual void Reload(IRenderingDevice * Device, Codec::Frame * Source) = 0;
		};
		class IFont : public Object
		{
		public:
			virtual void Reload(IRenderingDevice * Device) = 0;
		};

		class IRenderingDevice : public Object
		{
		public:
			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint> & gradient, double angle) = 0;
			virtual IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) = 0;
			virtual IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) = 0;
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern) = 0;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color) = 0;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const Color & color) = 0;
			virtual ILineRenderingInfo * CreateLineRenderingInfo(const Color & color, bool dotted) = 0;

			virtual ITexture * LoadTexture(Streaming::Stream * Source) = 0;
			virtual ITexture * LoadTexture(Codec::Image * Source) = 0;
			virtual ITexture * LoadTexture(Codec::Frame * Source) = 0;
			virtual IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) = 0;

			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) = 0;
			virtual void RenderTexture(ITextureRenderingInfo * Info, const Box & At) = 0;
			virtual void RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip) = 0;
			virtual void RenderLine(ILineRenderingInfo * Info, const Box & At) = 0;
			virtual void ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) = 0;
			virtual void ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink) = 0;

			virtual void PushClip(const Box & Rect) = 0;
			virtual void PopClip(void) = 0;
			virtual void BeginLayer(const Box & Rect, double Opacity) = 0;
			virtual void EndLayer(void) = 0;
			virtual void SetTimerValue(uint32 time) = 0;
			virtual void ClearCache(void) = 0;
			virtual ~IRenderingDevice(void);
		};

		class Shape : public Object
		{
		public:
			Rectangle Position;
			virtual void Render(IRenderingDevice * Device, const Box & Outer) const = 0;
			virtual void ClearCache(void) = 0;
		};
		class FrameShape : public Shape
		{
		public:
			enum class FrameRenderMode { Normal = 0, Clipping = 1, Layering = 2 };
			ObjectArray<Shape> Children;
			FrameRenderMode RenderMode;
			double Opacity;
			FrameShape(const Rectangle & position);
			FrameShape(const Rectangle & position, FrameRenderMode mode, double opacity = 1.0);
			~FrameShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const override;
			void ClearCache(void) override;
			string ToString(void) const override;
		};
		class BarShape : public Shape
		{
			mutable SafePointer<IBarRenderingInfo> Info;
			Array<GradientPoint> Gradient;
			double GradientAngle;
		public:
			BarShape(const Rectangle & position, const Color & color);
			BarShape(const Rectangle & position, const Array<GradientPoint> & gradient, double angle);
			~BarShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const override;
			void ClearCache(void) override;
			string ToString(void) const override;
		};
		class BlurEffectShape : public Shape
		{
			mutable SafePointer<IBlurEffectRenderingInfo> Info;
			double BlurPower;
		public:
			BlurEffectShape(const Rectangle & position, double power);
			~BlurEffectShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const override;
			void ClearCache(void) override;
			string ToString(void) const override;
		};
		class InversionEffectShape : public Shape
		{
			mutable SafePointer<IInversionEffectRenderingInfo> Info;
		public:
			InversionEffectShape(const Rectangle & position);
			~InversionEffectShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const override;
			void ClearCache(void) override;
			string ToString(void) const override;
		};
		class TextureShape : public Shape
		{
		public:
			enum class TextureRenderMode { Stretch = 0, Fit = 1, FillPattern = 2, AsIs = 3 };
		private:
			mutable SafePointer<ITextureRenderingInfo> Info;
			mutable Box FromBox;
			ITexture * Texture;
			TextureRenderMode Mode;
			Rectangle From;
		public:
			TextureShape(const Rectangle & position, ITexture * texture, const Rectangle & take_from, TextureRenderMode mode);
			~TextureShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const override;
			void ClearCache(void) override;
			string ToString(void) const override;
		};
		class TextShape : public Shape
		{
		public:
			enum class TextHorizontalAlign { Left = 0, Center = 1, Right = 2 };
			enum class TextVerticalAlign { Top = 0, Center = 1, Bottom = 2 };
		private:
			mutable SafePointer<ITextRenderingInfo> Info;
			IFont * Font;
			string Text;
			TextHorizontalAlign halign;
			TextVerticalAlign valign;
			Color TextColor;
		public:
			TextShape(const Rectangle & position, const string & text, IFont * font, const Color & color, TextHorizontalAlign horizontal_align, TextVerticalAlign vertical_align);
			~TextShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const override;
			void ClearCache(void) override;
			string ToString(void) const override;
		};
		class LineShape : public Shape
		{
		private:
			mutable SafePointer<ILineRenderingInfo> Info;
			Color LineColor;
			bool Dotted;
		public:
			LineShape(const Rectangle & position, const Color & color, bool dotted);
			~LineShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const override;
			void ClearCache(void) override;
			string ToString(void) const override;
		};
	}
}