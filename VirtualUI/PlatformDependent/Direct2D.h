#pragma once

#include "../ShapeBase.h"
#include "../Miscellaneous/Dictionary.h"

#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>

namespace Engine
{
	namespace Direct2D
	{
		using namespace ::Engine::UI;

		extern ID2D1Factory1 * D2DFactory;
		extern IWICImagingFactory * WICFactory;
		extern IDWriteFactory * DWriteFactory;

		void InitializeFactory(void);
		void ShutdownFactory(void);

		class D2DRenderDevice : public IRenderingDevice
		{
			ID2D1DeviceContext * Target;
			Array<ID2D1Layer *> Layers;
			uint32 AnimationTimer;
			uint32 BlinkPeriod;
			uint32 HalfBlinkPeriod;
			Dictionary::ObjectCache<Color, IBarRenderingInfo> BrushCache;
			Dictionary::ObjectCache<double, IBlurEffectRenderingInfo> BlurCache;
			SafePointer<IInversionEffectRenderingInfo> InversionInfo;
		public:
			D2DRenderDevice(ID2D1DeviceContext * target);
			~D2DRenderDevice(void) override;

			ID2D1RenderTarget * GetRenderTarget(void) const;

			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint>& gradient, double angle) override;
			virtual IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) override;
			virtual IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) override;
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern) override;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color) override;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const Color & color) override;
			virtual ILineRenderingInfo * CreateLineRenderingInfo(const Color & color, bool dotted) override;

			virtual ITexture * LoadTexture(Streaming::Stream * Source) override;
			virtual IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override;

			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) override;
			virtual void RenderTexture(ITextureRenderingInfo * Info, const Box & At) override;
			virtual void RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip) override;
			virtual void RenderLine(ILineRenderingInfo * Info, const Box & At) override;
			virtual void ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) override;
			virtual void ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink) override;

			virtual void PushClip(const Box & Rect) override;
			virtual void PopClip(void) override;
			virtual void BeginLayer(const Box & Rect, double Opacity) override;
			virtual void EndLayer(void) override;

			virtual void SetTimerValue(uint32 time) override;
			virtual void ClearCache(void) override;
		};
	}
}