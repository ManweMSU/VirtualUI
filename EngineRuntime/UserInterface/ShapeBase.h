#pragma once

#include "../EngineBase.h"
#include "../Streaming.h"
#include "../ImageCodec/CodecBase.h"
#include "../Graphics/GraphicsBase.h"
#include "../Math/Vector.h"

namespace Engine
{
	namespace UI
	{
		class Shape : public Object
		{
		public:
			Rectangle Position;

			virtual void Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept = 0;
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
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept override;
			virtual void ClearCache(void) noexcept override;
			virtual Shape * Clone(void) const override;
			virtual string ToString(void) const override;
		};
		class BarShape : public Shape
		{
			SafePointer<Graphics::IColorBrush> _info;
			Array<GradientPoint> _gradient;
			Coordinate _x1, _y1, _x2, _y2;
			Point _p1, _p2;
			int _w, _h;
		public:
			BarShape(const Rectangle & position, const Color & color);
			BarShape(const Rectangle & position, const Array<GradientPoint> & gradient, const Coordinate & x1, const Coordinate & y1, const Coordinate & x2, const Coordinate & y2);
			~BarShape(void) override;
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept override;
			virtual void ClearCache(void) noexcept override;
			virtual Shape * Clone(void) const override;
			virtual string ToString(void) const override;
		};
		class BlurEffectShape : public Shape
		{
			SafePointer<Graphics::IBlurEffectBrush> _info;
			double _power;
		public:
			BlurEffectShape(const Rectangle & position, double power);
			~BlurEffectShape(void) override;
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept override;
			virtual void ClearCache(void) noexcept override;
			virtual Shape * Clone(void) const override;
			virtual string ToString(void) const override;
		};
		class InversionEffectShape : public Shape
		{
			SafePointer<Graphics::IInversionEffectBrush> _info;
		public:
			InversionEffectShape(const Rectangle & position);
			~InversionEffectShape(void) override;
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept override;
			virtual void ClearCache(void) noexcept override;
			virtual Shape * Clone(void) const override;
			virtual string ToString(void) const override;
		};
		class TextureShape : public Shape
		{
		public:
			enum class TextureRenderMode { Stretch = 0, Fit = 1, FillPattern = 2, AsIs = 3 };
		private:
			SafePointer<Graphics::IBitmapBrush> _info;
			SafePointer<Graphics::IBitmap> _bitmap;
			TextureRenderMode _mode;
			Rectangle _from;
			Box _area;
		public:
			TextureShape(const Rectangle & position, Graphics::IBitmap * bitmap, const Rectangle & take_from, TextureRenderMode mode);
			~TextureShape(void) override;
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept override;
			virtual void ClearCache(void) noexcept override;
			virtual Shape * Clone(void) const override;
			virtual string ToString(void) const override;
		};
		class TextShape : public Shape
		{
		public:
			enum TextRenderFlags {
				TextRenderDirect		= 0x0000,
				TextRenderMultiline		= 0x0001,
				TextRenderAllowWordWrap	= 0x0002,
				TextRenderAllowEllipsis	= 0x0004,
				TextRenderAlignLeft		= 0x0000,
				TextRenderAlignCenter	= 0x0010,
				TextRenderAlignRight	= 0x0020,
				TextRenderAlignTop		= 0x0000,
				TextRenderAlignVCenter	= 0x0100,
				TextRenderAlignBottom	= 0x0200,
			};
		private:
			struct _text_atom {
				SafePointer<Graphics::ITextBrush> info;
				string text;
				bool end_line;
				int x, y;
			};

			Array<_text_atom> _atoms;
			SafePointer<Graphics::ITextBrush> _info;
			SafePointer<Graphics::IFont> _font;
			string _text;
			Color _color;
			uint32 _flags, _w, _h;
		public:
			TextShape(const Rectangle & position, const string & text, Graphics::IFont * font, const Color & color, uint32 flags);
			~TextShape(void) override;
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept override;
			virtual void ClearCache(void) noexcept override;
			virtual Shape * Clone(void) const override;
			virtual string ToString(void) const override;
		};
	}
}