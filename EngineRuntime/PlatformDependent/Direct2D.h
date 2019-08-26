#pragma once

#include "../UserInterface/ShapeBase.h"
#include "../UserInterface/Canvas.h"
#include "../Miscellaneous/Dictionary.h"
#include "../ImageCodec/CodecBase.h"

#include <d2d1_1.h>
#include <dwrite.h>
#include <wincodec.h>

namespace Engine
{
	namespace Direct2D
	{
		using namespace ::Engine::UI;

		extern ID2D1Factory1 * D2DFactory1;
		extern ID2D1Factory * D2DFactory;
		extern IWICImagingFactory * WICFactory;
		extern IDWriteFactory * DWriteFactory;

		void InitializeFactory(void);
		void ShutdownFactory(void);

		Engine::Codec::ICodec * CreateWicCodec(void);

		namespace StandaloneDevice
		{
			ITexture * LoadTexture(Streaming::Stream * Source);
			ITexture * LoadTexture(Engine::Codec::Image * Source);
			ITexture * LoadTexture(Engine::Codec::Frame * Source);
			IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout);
		}

		class D2DRenderDevice : public Drawing::ITextureRenderingDevice
		{
			ID2D1DeviceContext * ExtendedTarget;
			ID2D1RenderTarget * Target;
			Array<ID2D1Layer *> Layers;
			Array<Box> Clipping;
			uint32 AnimationTimer;
			uint32 BlinkPeriod;
			uint32 HalfBlinkPeriod;
			Dictionary::ObjectCache<Color, IBarRenderingInfo> BrushCache;
			Dictionary::ObjectCache<double, IBlurEffectRenderingInfo> BlurCache;
			Dictionary::ObjectCache<ITexture *, ITexture> TextureCache;
			SafePointer<IInversionEffectRenderingInfo> InversionInfo;
			SafePointer<IWICBitmap> BitmapTarget;
			int BitmapTargetState;
			int BitmapTargetResX, BitmapTargetResY;
		public:
			D2DRenderDevice(ID2D1DeviceContext * target);
			D2DRenderDevice(ID2D1RenderTarget * target);
			~D2DRenderDevice(void) override;

			ID2D1RenderTarget * GetRenderTarget(void) const noexcept;

			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint>& gradient, double angle) noexcept override;
			virtual IBarRenderingInfo * CreateBarRenderingInfo(Color color) noexcept override;
			virtual IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) noexcept override;
			virtual IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) noexcept override;
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern) noexcept override;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color) noexcept override;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const Color & color) noexcept override;
			virtual ILineRenderingInfo * CreateLineRenderingInfo(const Color & color, bool dotted) noexcept override;

			virtual ITexture * LoadTexture(Streaming::Stream * Source) override;
			virtual ITexture * LoadTexture(Engine::Codec::Image * Source) override;
			virtual ITexture * LoadTexture(Engine::Codec::Frame * Source) override;
			virtual UI::IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override;

			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) noexcept override;
			virtual void RenderTexture(ITextureRenderingInfo * Info, const Box & At) noexcept override;
			virtual void RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip) noexcept override;
			virtual void RenderLine(ILineRenderingInfo * Info, const Box & At) noexcept override;
			virtual void ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) noexcept override;
			virtual void ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink) noexcept override;

			virtual void PushClip(const Box & Rect) noexcept override;
			virtual void PopClip(void) noexcept override;
			virtual void BeginLayer(const Box & Rect, double Opacity) noexcept override;
			virtual void EndLayer(void) noexcept override;

			virtual void SetTimerValue(uint32 time) noexcept override;
			virtual uint32 GetCaretBlinkHalfTime(void) noexcept override;
			virtual void ClearCache(void) noexcept override;

			virtual Drawing::ICanvasRenderingDevice * QueryCanvasDevice(void) noexcept override;
			virtual void DrawPolygon(const Math::Vector2 * points, int count, const Math::Color & color, double width) noexcept override;
			virtual void FillPolygon(const Math::Vector2 * points, int count, const Math::Color & color) noexcept override;
			virtual Drawing::ITextureRenderingDevice * CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept override;
			virtual void BeginDraw(void) noexcept override;
			virtual void EndDraw(void) noexcept override;
			virtual UI::ITexture * GetRenderTargetAsTexture(void) noexcept override;
			virtual Engine::Codec::Frame * GetRenderTargetAsFrame(void) noexcept override;

			static Drawing::ITextureRenderingDevice * CreateD2DCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept;
		};
	}
}