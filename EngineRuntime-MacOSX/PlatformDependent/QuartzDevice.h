#pragma once

#include "../UserInterface/ShapeBase.h"
#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Cocoa
	{
        class QuartzRenderingDevice : public UI::IRenderingDevice
        {
            void * _context;
            int _width, _height, _scale;
            uint32 _animation;

            Dictionary::ObjectCache<UI::Color, UI::IBarRenderingInfo> BrushCache;
			Array<UI::Box> Clipping;
        public:
            QuartzRenderingDevice(void);
			~QuartzRenderingDevice(void) override;

			void * GetContext(void) const;
            void SetContext(void * context, int width, int height, int scale);

			virtual UI::IBarRenderingInfo * CreateBarRenderingInfo(const Array<UI::GradientPoint>& gradient, double angle) override;
			virtual UI::IBarRenderingInfo * CreateBarRenderingInfo(UI::Color color) override;
			virtual UI::IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) override;
			virtual UI::IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) override;
			virtual UI::ITextureRenderingInfo * CreateTextureRenderingInfo(UI::ITexture * texture, const UI::Box & take_area, bool fill_pattern) override;
			virtual UI::ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const string & text, int horizontal_align, int vertical_align, const UI::Color & color) override;
			virtual UI::ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color) override;
			virtual UI::ILineRenderingInfo * CreateLineRenderingInfo(const UI::Color & color, bool dotted) override;

			virtual UI::ITexture * LoadTexture(Streaming::Stream * Source) override;
			virtual UI::ITexture * LoadTexture(Engine::Codec::Image * Source) override;
			virtual UI::ITexture * LoadTexture(Engine::Codec::Frame * Source) override;
			virtual UI::IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override;

			virtual void RenderBar(UI::IBarRenderingInfo * Info, const UI::Box & At) override;
			virtual void RenderTexture(UI::ITextureRenderingInfo * Info, const UI::Box & At) override;
			virtual void RenderText(UI::ITextRenderingInfo * Info, const UI::Box & At, bool Clip) override;
			virtual void RenderLine(UI::ILineRenderingInfo * Info, const UI::Box & At) override;
			virtual void ApplyBlur(UI::IBlurEffectRenderingInfo * Info, const UI::Box & At) override;
			virtual void ApplyInversion(UI::IInversionEffectRenderingInfo * Info, const UI::Box & At, bool Blink) override;

			virtual void PushClip(const UI::Box & Rect) override;
			virtual void PopClip(void) override;
			virtual void BeginLayer(const UI::Box & Rect, double Opacity) override;
			virtual void EndLayer(void) override;

			virtual void SetTimerValue(uint32 time) override;
			virtual void ClearCache(void) override;
        };
    }
}