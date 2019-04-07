#pragma once

#include "../EngineBase.h"
#include "../Streaming.h"
#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace Drawing { class ICanvasRenderingDevice; }
	namespace UI
	{
		extern double Zoom;

		class Coordinate
		{
		public:
			double Anchor, Zoom;
			int Absolute;

			Coordinate(void) noexcept;
			Coordinate(int shift) noexcept;
			Coordinate(int shift, double zoom, double anchor) noexcept;

			Coordinate friend operator + (const Coordinate & a, const Coordinate & b) noexcept;
			Coordinate friend operator - (const Coordinate & a, const Coordinate & b) noexcept;
			Coordinate friend operator * (const Coordinate & a, double b) noexcept;
			Coordinate friend operator * (double b, const Coordinate & a) noexcept;
			Coordinate friend operator / (const Coordinate & a, double b) noexcept;

			Coordinate & operator += (const Coordinate & a) noexcept;
			Coordinate & operator -= (const Coordinate & a) noexcept;
			Coordinate & operator *= (double a) noexcept;
			Coordinate & operator /= (double a) noexcept;
			Coordinate operator - (void) const noexcept;

			bool friend operator == (const Coordinate & a, const Coordinate & b) noexcept;
			bool friend operator != (const Coordinate & a, const Coordinate & b) noexcept;

			static Coordinate Right() noexcept;
			static Coordinate Bottom() noexcept;
		};
		class Rectangle
		{
		public:
			Coordinate Left, Top, Right, Bottom;

			Rectangle(void) noexcept;
			Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom) noexcept;

			Rectangle friend operator + (const Rectangle & a, const Rectangle & b) noexcept;
			Rectangle friend operator - (const Rectangle & a, const Rectangle & b) noexcept;
			Rectangle friend operator * (const Rectangle & a, double b) noexcept;
			Rectangle friend operator * (double b, const Rectangle & a) noexcept;
			Rectangle friend operator / (const Rectangle & a, double b) noexcept;

			Rectangle & operator += (const Rectangle & a) noexcept;
			Rectangle & operator -= (const Rectangle & a) noexcept;
			Rectangle & operator *= (double a) noexcept;
			Rectangle & operator /= (double a) noexcept;
			Rectangle operator - (void) const noexcept;

			bool friend operator == (const Rectangle & a, const Rectangle & b) noexcept;
			bool friend operator != (const Rectangle & a, const Rectangle & b) noexcept;

			bool IsValid(void) const noexcept;

			static Rectangle Entire() noexcept;
			static Rectangle Invalid() noexcept;
		};
		class Point
		{
		public:
			int x, y;

			Point(void) noexcept;
			Point(int X, int Y) noexcept;

			bool friend operator == (const Point & a, const Point & b) noexcept;
			bool friend operator != (const Point & a, const Point & b) noexcept;
		};
		class Box
		{
		public:
			int Left, Top, Right, Bottom;

			Box(void) noexcept;
			Box(const Rectangle & source, const Box & outer) noexcept;
			Box(int left, int top, int right, int bottom) noexcept;

			bool friend operator == (const Box & a, const Box & b) noexcept;
			bool friend operator != (const Box & a, const Box & b) noexcept;

			bool IsInside(const Point & p) const noexcept;

			static Box Intersect(const Box & a, const Box & b) noexcept;
		};
		class Color
		{
		public:
			union {
				struct { uint8 r, g, b, a; };
				uint32 Value;
			};

			Color(void) noexcept;
			Color(uint8 sr, uint8 sg, uint8 sb, uint8 sa = 0xFF) noexcept;
			Color(int sr, int sg, int sb, int sa = 0xFF) noexcept;
			Color(float sr, float sg, float sb, float sa = 1.0) noexcept;
			Color(double sr, double sg, double sb, double sa = 1.0) noexcept;
			Color(uint32 code) noexcept;

			operator uint32 (void) const noexcept;

			bool friend operator == (const Color & a, const Color & b) noexcept;
			bool friend operator != (const Color & a, const Color & b) noexcept;
		};
		class GradientPoint
		{
		public:
			Color Color;
			double Position;

			GradientPoint(void) noexcept;
			GradientPoint(const UI::Color & color) noexcept;
			GradientPoint(const UI::Color & color, double position) noexcept;

			bool friend operator == (const GradientPoint & a, const GradientPoint & b) noexcept;
			bool friend operator != (const GradientPoint & a, const GradientPoint & b) noexcept;
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
			virtual void GetExtent(int & width, int & height) noexcept = 0;
			virtual void SetHighlightColor(const Color & color) noexcept = 0;
			virtual void HighlightText(int Start, int End) noexcept = 0;
			virtual int TestPosition(int point) noexcept = 0;
			virtual int EndOfChar(int Index) noexcept = 0;
			virtual void SetCharPalette(const Array<Color> & colors) = 0;
			virtual void SetCharColors(const Array<uint8> & indicies) = 0;
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
			virtual int GetWidth(void) const noexcept = 0;
			virtual int GetHeight(void) const noexcept = 0;
			virtual bool IsDynamic(void) const noexcept = 0;
			virtual void Reload(IRenderingDevice * Device, Streaming::Stream * Source) = 0;
			virtual void Reload(IRenderingDevice * Device, Codec::Image * Source) = 0;
			virtual void Reload(IRenderingDevice * Device, Codec::Frame * Source) = 0;
		};
		class IFont : public Object
		{
		public:
			virtual int GetWidth(void) const noexcept = 0;
			virtual int GetHeight(void) const noexcept = 0;
			virtual void Reload(IRenderingDevice * Device) = 0;
		};
		class IRenderingDevice : public Object
		{
		public:
			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint> & gradient, double angle) noexcept = 0;
			virtual IBarRenderingInfo * CreateBarRenderingInfo(Color color) noexcept = 0;
			virtual IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) noexcept = 0;
			virtual IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) noexcept = 0;
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern) noexcept = 0;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color) noexcept  = 0;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const Color & color) noexcept = 0;
			virtual ILineRenderingInfo * CreateLineRenderingInfo(const Color & color, bool dotted) noexcept = 0;

			virtual ITexture * LoadTexture(Streaming::Stream * Source) = 0;
			virtual ITexture * LoadTexture(Codec::Image * Source) = 0;
			virtual ITexture * LoadTexture(Codec::Frame * Source) = 0;
			virtual IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) = 0;

			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) noexcept = 0;
			virtual void RenderTexture(ITextureRenderingInfo * Info, const Box & At) noexcept = 0;
			virtual void RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip) noexcept = 0;
			virtual void RenderLine(ILineRenderingInfo * Info, const Box & At) noexcept = 0;
			virtual void ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) noexcept = 0;
			virtual void ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink) noexcept = 0;

			virtual void PushClip(const Box & Rect) noexcept = 0;
			virtual void PopClip(void) noexcept = 0;
			virtual void BeginLayer(const Box & Rect, double Opacity) noexcept = 0;
			virtual void EndLayer(void) noexcept = 0;
			virtual void SetTimerValue(uint32 time) noexcept = 0;
			virtual uint32 GetCaretBlinkHalfTime(void) noexcept = 0;
			virtual void ClearCache(void) noexcept = 0;
			virtual Drawing::ICanvasRenderingDevice * QueryCanvasDevice(void) noexcept;
			virtual ~IRenderingDevice(void);
		};

		class Shape : public Object
		{
		public:
			Rectangle Position;
			virtual void Render(IRenderingDevice * Device, const Box & Outer) const noexcept = 0;
			virtual void ClearCache(void) noexcept = 0;
			virtual Shape * Clone(void) const = 0;
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
			void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override;
			void ClearCache(void) noexcept override;
			Shape * Clone(void) const override;
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
			void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override;
			void ClearCache(void) noexcept override;
			Shape * Clone(void) const override;
			string ToString(void) const override;
		};
		class BlurEffectShape : public Shape
		{
			mutable SafePointer<IBlurEffectRenderingInfo> Info;
			double BlurPower;
		public:
			BlurEffectShape(const Rectangle & position, double power);
			~BlurEffectShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override;
			void ClearCache(void) noexcept override;
			Shape * Clone(void) const override;
			string ToString(void) const override;
		};
		class InversionEffectShape : public Shape
		{
			mutable SafePointer<IInversionEffectRenderingInfo> Info;
		public:
			InversionEffectShape(const Rectangle & position);
			~InversionEffectShape(void) override;
			void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override;
			void ClearCache(void) noexcept override;
			Shape * Clone(void) const override;
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
			void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override;
			void ClearCache(void) noexcept override;
			Shape * Clone(void) const override;
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
			void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override;
			void ClearCache(void) noexcept override;
			Shape * Clone(void) const override;
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
			void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override;
			void ClearCache(void) noexcept override;
			Shape * Clone(void) const override;
			string ToString(void) const override;
		};
	}
}