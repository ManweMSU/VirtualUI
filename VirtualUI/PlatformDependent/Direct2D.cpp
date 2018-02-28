#include "Direct2D.h"

#include <d2d1_1helper.h>
#include <dwrite.h>

#include <math.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Engine
{
	namespace Direct2D
	{
		ID2D1Factory * Factory = 0;

		struct BarRenderingInfo : public IBarRenderingInfo
		{
			ID2D1GradientStopCollection * Collection = 0;
			ID2D1SolidColorBrush * Brush = 0;
			double prop_h = 0.0, prop_w = 0.0;
			~BarRenderingInfo(void) override { if (Collection) Collection->Release(); if (Brush) Brush->Release(); }
		};

		void InitializeFactory(void)
		{
			if (!Factory) {
				if (D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &Factory) != S_OK) throw Exception();
			}
		}

		D2DRenderDevice::D2DRenderDevice(ID2D1RenderTarget * target) : Target(target), Layers(0x10) { Target->AddRef(); }
		D2DRenderDevice::~D2DRenderDevice(void) { Target->Release(); }
		ID2D1RenderTarget * D2DRenderDevice::GetRenderTarget(void) const { return Target; }
		IBarRenderingInfo * D2DRenderDevice::CreateBarRenderingInfo(const Array<GradientPoint>& gradient, double angle)
		{
			if (gradient.Length() > 1) {
				D2D1_GRADIENT_STOP * stop = new (std::nothrow) D2D1_GRADIENT_STOP[gradient.Length()];
				if (!stop) { throw OutOfMemoryException(); }
				for (int i = 0; i < gradient.Length(); i++) {
					stop[i].color = D2D1::ColorF(gradient[i].Color.r / 255.0f, gradient[i].Color.g / 255.0f, gradient[i].Color.b / 255.0f, gradient[i].Color.a / 255.0f);
					stop[i].position = float(gradient[i].Position);
				}
				ID2D1GradientStopCollection * Collection;
				if (Target->CreateGradientStopCollection(stop, gradient.Length(), &Collection) != S_OK) { delete[] stop; throw Exception(); }
				BarRenderingInfo * Info = new (std::nothrow) BarRenderingInfo;
				if (!Info) { delete[] stop; Collection->Release(); throw OutOfMemoryException(); }
				Info->Collection = Collection;
				Info->prop_w = cos(angle), Info->prop_h = sin(angle);
				return Info;
			} else {
				BarRenderingInfo * Info = new (std::nothrow) BarRenderingInfo;
				if (!Info) { throw OutOfMemoryException(); }
				ID2D1SolidColorBrush * Brush;
				if (Target->CreateSolidColorBrush(D2D1::ColorF(gradient[0].Color.r / 255.0f, gradient[0].Color.g / 255.0f, gradient[0].Color.b / 255.0f, gradient[0].Color.a / 255.0f), &Brush) != S_OK) { delete Info; throw Exception(); }
				Info->Brush = Brush;
				return Info;
			}
			
		}
		IBlurEffectRenderingInfo * D2DRenderDevice::CreateBlurEffectRenderingInfo(double power)
		{
			throw Exception();
#pragma message ("METHOD NOT IMPLEMENTED, IMPLEMENT IT!")
		}
		void D2DRenderDevice::RenderBar(IBarRenderingInfo * Info, const Box & At)
		{
			auto info = reinterpret_cast<BarRenderingInfo *>(Info);
			if (info->Brush) {
				Target->FillRectangle(D2D1::RectF(float(At.Left), float(At.Top), float(At.Right), float(At.Bottom)), info->Brush);
			} else {
				int w = At.Right - At.Left, h = At.Bottom - At.Top;
				if (!w || !h) return;
				D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES bp;
				if (fabs(info->prop_w) > fabs(info->prop_h)) {
					double aspect = info->prop_h / info->prop_w;
					if (fabs(aspect) > double(h) / double(w)) {
						int dx = int(h / fabs(aspect) * sgn(info->prop_w));
						bp.startPoint.x = float(At.Left + (w - dx) / 2);
						bp.startPoint.y = float((info->prop_h > 0.0) ? At.Bottom : At.Top);
						bp.endPoint.x = float(At.Left + (w + dx) / 2);
						bp.endPoint.y = float((info->prop_h > 0.0) ? At.Top : At.Bottom);
					} else {
						int dy = int(w * fabs(aspect) * sgn(info->prop_h));
						bp.startPoint.x = float((info->prop_w > 0.0) ? At.Left : At.Right);
						bp.startPoint.y = float(At.Top + (h + dy) / 2);
						bp.endPoint.x = float((info->prop_w > 0.0) ? At.Right : At.Left);
						bp.endPoint.y = float(At.Top + (h - dy) / 2);
					}
				} else {
					double aspect = info->prop_w / info->prop_h;
					if (fabs(aspect) > double(w) / double(h)) {
						int dy = int(w / fabs(aspect) * sgn(info->prop_h));
						bp.startPoint.x = float((info->prop_w > 0.0) ? At.Left : At.Right);
						bp.startPoint.y = float(At.Top + (h + dy) / 2);
						bp.endPoint.x = float((info->prop_w > 0.0) ? At.Right : At.Left);
						bp.endPoint.y = float(At.Top + (h - dy) / 2);
					} else {
						int dx = int(h * fabs(aspect) * sgn(info->prop_w));
						bp.startPoint.x = float(At.Left + (w - dx) / 2);
						bp.startPoint.y = float((info->prop_h > 0.0) ? At.Bottom : At.Top);
						bp.endPoint.x = float(At.Left + (w + dx) / 2);
						bp.endPoint.y = float((info->prop_h > 0.0) ? At.Top : At.Bottom);
					}
				}
				ID2D1LinearGradientBrush * Brush;
				if (Target->CreateLinearGradientBrush(bp, info->Collection, &Brush) == S_OK) {
					Target->FillRectangle(D2D1::RectF(float(At.Left), float(At.Top), float(At.Right), float(At.Bottom)), Brush);
					Brush->Release();
				}
			}
		}
		void D2DRenderDevice::ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At)
		{
			throw Exception();
#pragma message ("METHOD NOT IMPLEMENTED, IMPLEMENT IT!")
		}
		void D2DRenderDevice::PushClip(const Box & Rect) { Target->PushAxisAlignedClip(D2D1::RectF(float(Rect.Left), float(Rect.Top), float(Rect.Right), float(Rect.Bottom)), D2D1_ANTIALIAS_MODE_ALIASED); }
		void D2DRenderDevice::PopClip(void) { Target->PopAxisAlignedClip(); }
		void D2DRenderDevice::BeginLayer(const Box & Rect, double Opacity)
		{
			ID2D1Layer * Layer;
			if (Target->CreateLayer(0, &Layer) != S_OK) throw Exception();
			Layers << Layer;
			D2D1_LAYER_PARAMETERS lp;
			lp.contentBounds = D2D1::RectF(float(Rect.Left), float(Rect.Top), float(Rect.Right), float(Rect.Bottom));
			lp.geometricMask = 0;
			lp.maskAntialiasMode = D2D1_ANTIALIAS_MODE_ALIASED;
			lp.maskTransform = D2D1::IdentityMatrix();
			lp.opacity = float(Opacity);
			lp.opacityBrush = 0;
			lp.layerOptions = D2D1_LAYER_OPTIONS_NONE;
			Target->PushLayer(&lp, Layer);
		}
		void D2DRenderDevice::EndLayer(void)
		{
			Target->PopLayer();
			Layers.LastElement()->Release();
			Layers.RemoveLast();
		}
	}
}


