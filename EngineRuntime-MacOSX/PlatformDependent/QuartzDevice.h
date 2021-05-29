#pragma once

#include "../UserInterface/ShapeBase.h"
#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Cocoa
	{
		void * GetCoreImageFromTexture(UI::ITexture * texture);
        class QuartzRenderingDevice : public UI::ITextureRenderingDevice
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

			virtual void GetImplementationInfo(string & tech, uint32 & version) noexcept override;
			virtual uint32 GetFeatureList(void) noexcept override;

			virtual UI::IBarRenderingInfo * CreateBarRenderingInfo(const Array<UI::GradientPoint>& gradient, double angle) noexcept override;
			virtual UI::IBarRenderingInfo * CreateBarRenderingInfo(UI::Color color) noexcept override;
			virtual UI::IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) noexcept override;
			virtual UI::IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) noexcept override;
			virtual UI::ITextureRenderingInfo * CreateTextureRenderingInfo(UI::ITexture * texture, const UI::Box & take_area, bool fill_pattern) noexcept override;
			virtual UI::ITextureRenderingInfo * CreateTextureRenderingInfo(Graphics::ITexture * texture) noexcept override;
			virtual UI::ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const string & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept override;
			virtual UI::ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept override;

			virtual Graphics::ITexture * CreateIntermediateRenderTarget(Graphics::PixelFormat format, int width, int height) override;

			virtual void RenderBar(UI::IBarRenderingInfo * Info, const UI::Box & At) noexcept override;
			virtual void RenderTexture(UI::ITextureRenderingInfo * Info, const UI::Box & At) noexcept override;
			virtual void RenderText(UI::ITextRenderingInfo * Info, const UI::Box & At, bool Clip) noexcept override;
			virtual void ApplyBlur(UI::IBlurEffectRenderingInfo * Info, const UI::Box & At) noexcept override;
			virtual void ApplyInversion(UI::IInversionEffectRenderingInfo * Info, const UI::Box & At, bool Blink) noexcept override;

			virtual void DrawPolygon(const Math::Vector2 * points, int count, UI::Color color, double width) noexcept override;
			virtual void FillPolygon(const Math::Vector2 * points, int count, UI::Color color) noexcept override;

			virtual void PushClip(const UI::Box & Rect) noexcept override;
			virtual void PopClip(void) noexcept override;
			virtual void BeginLayer(const UI::Box & Rect, double Opacity) noexcept override;
			virtual void EndLayer(void) noexcept override;

			virtual void SetTimerValue(uint32 time) noexcept override;
			virtual uint32 GetCaretBlinkHalfTime(void) noexcept override;
			virtual bool CaretShouldBeVisible(void) noexcept override;
			virtual void ClearCache(void) noexcept override;

			virtual UI::ITexture * LoadTexture(Codec::Frame * source) noexcept override;
			virtual UI::IFont * LoadFont(const string & face_name, int height, int weight, bool italic, bool underline, bool strikeout) noexcept override;
			virtual UI::ITextureRenderingDevice * CreateTextureRenderingDevice(int width, int height, UI::Color color) noexcept override;
			virtual UI::ITextureRenderingDevice * CreateTextureRenderingDevice(Codec::Frame * frame) noexcept override;

			virtual void BeginDraw(void) noexcept override;
			virtual void EndDraw(void) noexcept override;
			virtual UI::ITexture * GetRenderTargetAsTexture(void) noexcept override;
			virtual Engine::Codec::Frame * GetRenderTargetAsFrame(void) noexcept override;

			virtual string ToString(void) const override;

			static UI::ITexture * StaticLoadTexture(Codec::Frame * source) noexcept;
			static UI::IFont * StaticLoadFont(const string & face_name, int height, int weight, bool italic, bool underline, bool strikeout) noexcept;
			static UI::ITextureRenderingDevice * StaticCreateTextureRenderingDevice(int width, int height, UI::Color color) noexcept;
			static UI::ITextureRenderingDevice * StaticCreateTextureRenderingDevice(Codec::Frame * frame) noexcept;
        };
    }
}