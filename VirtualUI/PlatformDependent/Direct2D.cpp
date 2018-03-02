#include "Direct2D.h"

#include <d2d1_1helper.h>
#include <dwrite.h>

#include <math.h>

#include "ComStreamWrapper.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Engine
{
	namespace Direct2D
	{
		class D2DTexture;

		ID2D1Factory * D2DFactory = 0;
		IWICImagingFactory * WICFactory = 0;
		IDWriteFactory * DWriteFactory = 0;

		void InitializeFactory(void)
		{
			if (!D2DFactory) {
				if (D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory) != S_OK) throw Exception();
			}
			if (!WICFactory) {
				if (CoCreateInstance(CLSID_WICImagingFactory, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&WICFactory)) != S_OK) throw Exception();
			}
			if (!DWriteFactory) {
				if (DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&DWriteFactory)) != S_OK) throw Exception();
			}
		}
		void ShutdownFactory(void)
		{
			if (D2DFactory) { D2DFactory->Release(); D2DFactory = 0; }
			if (WICFactory) { WICFactory->Release(); WICFactory = 0; }
			if (DWriteFactory) { DWriteFactory->Release(); DWriteFactory = 0; }
		}

		struct BarRenderingInfo : public IBarRenderingInfo
		{
			ID2D1GradientStopCollection * Collection = 0;
			ID2D1SolidColorBrush * Brush = 0;
			double prop_h = 0.0, prop_w = 0.0;
			~BarRenderingInfo(void) override { if (Collection) Collection->Release(); if (Brush) Brush->Release(); }
		};
		struct TextureRenderingInfo : public ITextureRenderingInfo
		{
			D2DTexture * Texture;
			Box From;
			ID2D1BitmapBrush * Brush;
			bool Fill;
			virtual ~TextureRenderingInfo(void) { if (Brush) Brush->Release(); }
		};
		class D2DTexture : public ITexture
		{
		public:
			Array<ID2D1Bitmap *> Frames;
			Array<uint32> FrameDuration;
			uint32 TotalDuration;
			int w, h;
			virtual ~D2DTexture(void)
			{
				for (int i = 0; i < Frames.Length(); i++) Frames[i]->Release();
			}
			virtual int GetWidth(void) const override { return w; }
			virtual int GetHeight(void) const override { return h; }
			virtual bool IsDynamic(void) const override { return Frames.Length() > 1; }
			virtual void Reload(IRenderingDevice * Device, Streaming::Stream * Source) override
			{
				for (int i = 0; i < Frames.Length(); i++) Frames[i]->Release();
				auto New = static_cast<D2DTexture *>(Device->LoadTexture(Source));
				Frames = New->Frames;
				FrameDuration = New->FrameDuration;
				TotalDuration = New->TotalDuration;
				w = New->w;
				h = New->h;
				New->Frames.Clear();
				New->Release();
			}
			virtual string ToString(void) const override { return L"D2DTexture"; }
		};

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
				delete[] stop;
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
		ITextureRenderingInfo * D2DRenderDevice::CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern)
		{
			TextureRenderingInfo * Info = new (std::nothrow) TextureRenderingInfo;
			Info->Texture = static_cast<D2DTexture *>(texture);
			Info->From = take_area;
			Info->Fill = fill_pattern;
			Info->Brush = 0;
			if (fill_pattern && !texture->IsDynamic()) {
				ID2D1Bitmap * Fragment;
				if (take_area.Left == 0 && take_area.Top == 0 && take_area.Right == Info->Texture->w && take_area.Bottom == Info->Texture->h) {
					Fragment = Info->Texture->Frames[0];
					Fragment->AddRef();
				} else {
					if (Target->CreateBitmap(D2D1::SizeU(uint(take_area.Right - take_area.Left), uint(take_area.Bottom - take_area.Top)),
						D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f), &Fragment) != S_OK) { delete Info; throw Exception(); }
					if (Fragment->CopyFromBitmap(&D2D1::Point2U(0, 0), Info->Texture->Frames[0], &D2D1::RectU(take_area.Left, take_area.Top, take_area.Right, take_area.Bottom)) != S_OK)
					{ Fragment->Release(); delete Info; throw Exception(); }
				}
				if (Target->CreateBitmapBrush(Fragment, D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR), &Info->Brush) != S_OK)
				{ Fragment->Release(); delete Info; throw Exception(); }
				Fragment->Release();
			}
			return Info;
		}
		ITexture * D2DRenderDevice::LoadTexture(Streaming::Stream * Source)
		{
			Streaming::ComStream * Stream = new Streaming::ComStream(Source);
			IWICBitmapDecoder * Decoder = 0;
			IWICBitmapFrameDecode * FrameDecoder = 0;
			IWICMetadataQueryReader * Metadata = 0;
			IWICFormatConverter * Converter = 0;
			D2DTexture * Texture = new (std::nothrow) D2DTexture;
			if (!Texture) { Stream->Release(); throw OutOfMemoryException(); }
			try {
				uint32 FrameCount;
				HRESULT r;
				if (r = WICFactory->CreateDecoderFromStream(Stream, 0, WICDecodeMetadataCacheOnDemand, &Decoder) != S_OK) throw IO::FileFormatException();
				if (Decoder->GetFrameCount(&FrameCount) != S_OK) throw IO::FileFormatException();
				Texture->w = Texture->h = -1;
				for (uint32 i = 0; i < FrameCount; i++) {
					if (Decoder->GetFrame(i, &FrameDecoder) != S_OK) throw IO::FileFormatException();
					uint32 fw, fh;
					if (FrameDecoder->GetSize(&fw, &fh) != S_OK) throw IO::FileFormatException();
					if (Texture->w != -1 && Texture->w != fw) throw IO::FileFormatException();
					if (Texture->h != -1 && Texture->h != fh) throw IO::FileFormatException();
					Texture->w = int(fw); Texture->h = int(fh);
					if (FrameDecoder->GetMetadataQueryReader(&Metadata) == S_OK) {
						PROPVARIANT Value;
						PropVariantInit(&Value);
						if (Metadata->GetMetadataByName(L"/grctlext/Delay", &Value) == S_OK) {
							if (Value.vt == VT_UI2) {
								Texture->FrameDuration << uint(Value.uiVal) * 10;
							} else Texture->FrameDuration << 0;
						} else Texture->FrameDuration << 0;
						PropVariantClear(&Value);
						Metadata->Release();
						Metadata = 0;
					} else Texture->FrameDuration << 0;
					if (WICFactory->CreateFormatConverter(&Converter) != S_OK) throw Exception();
					if (Converter->Initialize(FrameDecoder, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, 0, 0.0f, WICBitmapPaletteTypeCustom) != S_OK) throw IO::FileFormatException();
					ID2D1Bitmap * Frame;
					if (Target->CreateBitmapFromWicBitmap(Converter, &Frame) != S_OK) throw IO::FileFormatException();
					try { Texture->Frames << Frame; }
					catch (...) { Frame->Release(); throw; }
					Converter->Release();
					Converter = 0;
					FrameDecoder->Release();
					FrameDecoder = 0;
				}
				Decoder->Release();
				Decoder = 0;
			}
			catch (...) {
				Stream->Release();
				if (Decoder) Decoder->Release();
				if (FrameDecoder) FrameDecoder->Release();
				if (Metadata) Metadata->Release();
				if (Converter) Converter->Release();
				if (Texture) Texture->Release();
				throw;
			}
			Stream->Release();
			Texture->TotalDuration = Texture->FrameDuration[0];
			for (int i = 1; i < Texture->FrameDuration.Length(); i++) {
				auto v = Texture->FrameDuration[i];
				Texture->FrameDuration[i] = Texture->TotalDuration;
				Texture->TotalDuration += v;
			}
			return Texture;
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
		void D2DRenderDevice::RenderTexture(ITextureRenderingInfo * Info, const Box & At)
		{
			auto info = static_cast<TextureRenderingInfo *>(Info);
			int frame = 0;
			if (info->Texture->Frames.Length() > 1) frame = BinarySearchLE(info->Texture->FrameDuration, AnimationTimer % info->Texture->TotalDuration);
			D2D1_RECT_F To = D2D1::RectF(float(At.Left), float(At.Top), float(At.Right), float(At.Bottom));
			if (info->Fill) {
				if (info->Brush) {
					Target->FillRectangle(To, info->Brush);
				} else {
					ID2D1Bitmap * Fragment = 0;
					if (info->From.Left == 0 && info->From.Top == 0 && info->From.Right == info->Texture->w && info->From.Bottom == info->Texture->h) {
						Fragment = info->Texture->Frames[frame];
						Fragment->AddRef();
					} else {
						if (Target->CreateBitmap(D2D1::SizeU(uint(info->From.Right - info->From.Left), uint(info->From.Bottom - info->From.Top)),
							D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f), &Fragment) == S_OK) {
							if (Fragment->CopyFromBitmap(&D2D1::Point2U(0, 0), info->Texture->Frames[frame], &D2D1::RectU(info->From.Left, info->From.Top, info->From.Right, info->From.Bottom)) != S_OK)
							{
								Fragment->Release(); Fragment = 0;
							}
						}
					}
					if (Fragment) {
						ID2D1BitmapBrush * Brush;
						if (Target->CreateBitmapBrush(Fragment, D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR), &Brush) == S_OK)
						{
							Target->FillRectangle(To, Brush);
							Brush->Release();
						}
						Fragment->Release();
					}
				}
			} else {
				D2D1_RECT_F From = D2D1::RectF(float(info->From.Left), float(info->From.Top), float(info->From.Right), float(info->From.Bottom));
				Target->DrawBitmap(info->Texture->Frames[frame], To, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, From);
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
		void D2DRenderDevice::SetTimerValue(uint32 time) { AnimationTimer = time; }
	}
}


