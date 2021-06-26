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
	}
}