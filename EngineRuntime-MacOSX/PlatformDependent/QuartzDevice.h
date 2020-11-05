#pragma once

#include "../UserInterface/ShapeBase.h"
#include "../UserInterface/Canvas.h"
#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Cocoa
	{
		void * GetCoreImageFromTexture(UI::ITexture * texture);
        class QuartzRenderingDevice : public Drawing::ITextureRenderingDevice
        {
            void * _context;
            int _width, _height, _scale;
            uint32 _animation;

            Dictionary::ObjectCache<UI::Color, UI::IBarRenderingInfo> BrushCache;
			SafePointer<UI::IInversionEffectRenderingInfo> InversionCache;
			Array<UI::Box> Clipping;

			SafePointer<Codec::Frame> BitmapTarget;
			UI::ITextRenderingInfo * CreateTextRenderingInfoRaw(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept;
        public:
            QuartzRenderingDevice(void);
			~QuartzRenderingDevice(void) override;

			void * GetContext(void) const noexcept;
            void SetContext(void * context, int width, int height, int scale) noexcept;

			virtual void TextureWasDestroyed(UI::ITexture * texture) noexcept override;

			virtual UI::IBarRenderingInfo * CreateBarRenderingInfo(const Array<UI::GradientPoint>& gradient, double angle) noexcept override;
			virtual UI::IBarRenderingInfo * CreateBarRenderingInfo(UI::Color color) noexcept override;
			virtual UI::IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) noexcept override;
			virtual UI::IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) noexcept override;
			virtual UI::ITextureRenderingInfo * CreateTextureRenderingInfo(UI::ITexture * texture, const UI::Box & take_area, bool fill_pattern) noexcept override;
			virtual UI::ITextureRenderingInfo * CreateTextureRenderingInfo(Graphics::ITexture * texture) noexcept override;
			virtual UI::ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const string & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept override;
			virtual UI::ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept override;

			virtual UI::ITexture * LoadTexture(Streaming::Stream * Source) override;
			virtual UI::ITexture * LoadTexture(Engine::Codec::Image * Source) override;
			virtual UI::ITexture * LoadTexture(Engine::Codec::Frame * Source) override;
			virtual UI::IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override;
			virtual Graphics::ITexture * CreateIntermediateRenderTarget(Graphics::PixelFormat format, int width, int height) override;

			virtual void RenderBar(UI::IBarRenderingInfo * Info, const UI::Box & At) noexcept override;
			virtual void RenderTexture(UI::ITextureRenderingInfo * Info, const UI::Box & At) noexcept override;
			virtual void RenderText(UI::ITextRenderingInfo * Info, const UI::Box & At, bool Clip) noexcept override;
			virtual void ApplyBlur(UI::IBlurEffectRenderingInfo * Info, const UI::Box & At) noexcept override;
			virtual void ApplyInversion(UI::IInversionEffectRenderingInfo * Info, const UI::Box & At, bool Blink) noexcept override;

			virtual void PushClip(const UI::Box & Rect) noexcept override;
			virtual void PopClip(void) noexcept override;
			virtual void BeginLayer(const UI::Box & Rect, double Opacity) noexcept override;
			virtual void EndLayer(void) noexcept override;

			virtual void SetTimerValue(uint32 time) noexcept override;
			virtual uint32 GetCaretBlinkHalfTime(void) noexcept override;
			virtual bool CaretShouldBeVisible(void) noexcept override;
			virtual void ClearCache(void) noexcept override;

			virtual Drawing::ICanvasRenderingDevice * QueryCanvasDevice(void) noexcept override;
			virtual void DrawPolygon(const Math::Vector2 * points, int count, const Math::Color & color, double width) noexcept override;
			virtual void FillPolygon(const Math::Vector2 * points, int count, const Math::Color & color) noexcept override;
			virtual Drawing::ITextureRenderingDevice * CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept override;
			virtual void BeginDraw(void) noexcept override;
			virtual void EndDraw(void) noexcept override;
			virtual UI::ITexture * GetRenderTargetAsTexture(void) noexcept override;
			virtual Engine::Codec::Frame * GetRenderTargetAsFrame(void) noexcept override;

			virtual string ToString(void) const override;

			static Drawing::ITextureRenderingDevice * CreateQuartzCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept;
        };
    }
}