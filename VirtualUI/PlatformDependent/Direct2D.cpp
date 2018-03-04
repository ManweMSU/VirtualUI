#include "Direct2D.h"

#include <d2d1_1helper.h>
#include <dwrite.h>

#include <math.h>

#include "ComInterop.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

namespace Engine
{
	namespace Direct2D
	{
		class D2DTexture;
		class D2DFont;
		class D2DRenderDevice;

		ID2D1Factory1 * D2DFactory = 0;
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
			Array<ID2D1BitmapBrush *> Brushes;
			bool Fill;
			TextureRenderingInfo(void) : Brushes(0x20) {}
			~TextureRenderingInfo(void) override { for (int i = 0; i < Brushes.Length(); i++) Brushes[i]->Release(); }
		};
		struct TextRenderingInfo : public ITextRenderingInfo
		{
			D2DFont * Font;
			Array<uint32> CharString;
			Array<uint16> GlyphString;
			Array<float> GlyphAdvances;
			Array<uint8> UseAlternative;
			ID2D1PathGeometry * Geometry;
			ID2D1SolidColorBrush * TextBrush;
			ID2D1SolidColorBrush * HighlightBrush;
			ID2D1RenderTarget * RenderTarget;
			uint16 NormalSpaceGlyph;
			uint16 AlternativeSpaceGlyph;
			int BaselineOffset;
			int halign, valign;
			int run_length;
			float UnderlineOffset;
			float UnderlineHalfWidth;
			float StrikeoutOffset;
			float StrikeoutHalfWidth;
			int hls, hle;

			TextRenderingInfo(void) : CharString(0x40), GlyphString(0x40), GlyphAdvances(0x40), UseAlternative(0x40), hls(-1), hle(-1) {}

			void FillAdvances(void);
			void FillGlyphs(void);
			void BuildGeometry(void);
			float FontUnitsToDIP(int units);
			float AltFontUnitsToDIP(int units);

			virtual void GetExtent(int & width, int & height) override;
			virtual void SetHighlightColor(const Color & color) override
			{
				if (HighlightBrush) { HighlightBrush->Release(); HighlightBrush = 0; }
				RenderTarget->CreateSolidColorBrush(D2D1::ColorF(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f), &HighlightBrush);
			}
			virtual void HighlightText(int Start, int End) override { hls = Start; hle = End; }
			virtual int TestPosition(int point) override
			{
				if (point < 0) return 0;
				if (point > run_length) return CharString.Length();
				float p = float(point);
				float s = 0.0f;
				for (int i = 0; i < GlyphAdvances.Length(); i++) {
					if (p >= s) {
						if (p < s + GlyphAdvances[i] / 2.0f) return i;
						else return i + 1;
						break;
					}
					s += GlyphAdvances[i];
				}
				return CharString.Length();
			}
			virtual int EndOfChar(int Index) override
			{
				if (Index < 0) return 0;
				float summ = 0;
				for (int i = 0; i <= Index; i++) summ += GlyphAdvances[i];
				return int(summ);
			}
			virtual ~TextRenderingInfo(void) override
			{
				if (Geometry) Geometry->Release();
				if (TextBrush) TextBrush->Release();
				if (HighlightBrush) HighlightBrush->Release();
			}
		};
		struct LineRenderingInfo : public ILineRenderingInfo
		{
			ID2D1SolidColorBrush * Brush;
			ID2D1StrokeStyle * Stroke;
			virtual ~LineRenderingInfo(void) override { if (Brush) Brush->Release(); if (Stroke) Stroke->Release(); }
		};
		class D2DTexture : public ITexture
		{
		public:
			Array<ID2D1Bitmap *> Frames;
			Array<uint32> FrameDuration;
			uint32 TotalDuration;
			int w, h;
			D2DTexture(void) : Frames(0x20) {}
			virtual ~D2DTexture(void)
			{
				for (int i = 0; i < Frames.Length(); i++) Frames[i]->Release();
			}
			virtual int GetWidth(void) const override { return w; }
			virtual int GetHeight(void) const override { return h; }
			virtual bool IsDynamic(void) const override { return false; }
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
		class D2DFont : public IFont
		{
		public:
			IDWriteFontFace * FontFace;
			IDWriteFontFace * AlternativeFace;
			string FontName;
			int Height, Weight;
			int ActualHeight;
			bool Italic, Underline, Strikeout;
			DWRITE_FONT_METRICS FontMetrics;
			DWRITE_FONT_METRICS AltMetrics;
			virtual ~D2DFont(void) override { if (FontFace) FontFace->Release(); if (AlternativeFace) AlternativeFace->Release(); }
			virtual void Reload(IRenderingDevice * Device) override
			{
				if (FontFace) FontFace->Release();
				if (AlternativeFace) AlternativeFace->Release();
				auto New = static_cast<D2DFont *>(Device->LoadFont(FontName, ActualHeight, Weight, Italic, Underline, Strikeout));
				FontFace = New->FontFace;
				FontMetrics = New->FontMetrics;
				AlternativeFace = New->AlternativeFace;
				New->FontFace = 0;
				New->AlternativeFace = 0;
				New->Release();
			}
			virtual string ToString(void) const override { return L"D2DFont"; }
		};

		D2DRenderDevice::D2DRenderDevice(ID2D1DeviceContext * target) : Target(target), Layers(0x10) { Target->AddRef(); BlinkPeriod = GetCaretBlinkTime(); }
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
		IInversionEffectRenderingInfo * D2DRenderDevice::CreateInversionEffectRenderingInfo(void)
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
			if (fill_pattern && !texture->IsDynamic()) {
				for (int i = 0; i < Info->Texture->Frames.Length(); i++) {
					ID2D1Bitmap * Fragment;
					if (take_area.Left == 0 && take_area.Top == 0 && take_area.Right == Info->Texture->w && take_area.Bottom == Info->Texture->h) {
						Fragment = Info->Texture->Frames[i];
						Fragment->AddRef();
					} else {
						if (Target->CreateBitmap(D2D1::SizeU(uint(take_area.Right - take_area.Left), uint(take_area.Bottom - take_area.Top)),
							D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f), &Fragment) != S_OK) {
							delete Info; throw Exception();
						}
						if (Fragment->CopyFromBitmap(&D2D1::Point2U(0, 0), Info->Texture->Frames[i], &D2D1::RectU(take_area.Left, take_area.Top, take_area.Right, take_area.Bottom)) != S_OK)
						{
							Fragment->Release(); delete Info; throw Exception();
						}
					}
					ID2D1BitmapBrush * Brush;
					if (Target->CreateBitmapBrush(Fragment, D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR), &Brush) != S_OK)
					{
						Fragment->Release(); delete Info; throw Exception();
					}
					Fragment->Release();
					try { Info->Brushes << Brush; }
					catch (...) { Brush->Release(); delete Info; throw; }
				}
			}
			return Info;
		}
		ITextRenderingInfo * D2DRenderDevice::CreateTextRenderingInfo(IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color)
		{
			auto Info = new TextRenderingInfo;
			try {
				Info->halign = horizontal_align;
				Info->valign = vertical_align;
				Info->Font = static_cast<D2DFont *>(font);
				Info->RenderTarget = Target;
				Info->HighlightBrush = 0;
				Info->TextBrush = 0;
				Info->Geometry = 0;
				if (Target->CreateSolidColorBrush(D2D1::ColorF(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f), &Info->TextBrush) != S_OK) throw Exception();
				Info->CharString.SetLength(text.GetEncodedLength(Encoding::UTF32));
				text.Encode(Info->CharString, Encoding::UTF32, false);
				Info->FillGlyphs();
				Info->FillAdvances();
				Info->BuildGeometry();
			}
			catch (...) {
				delete Info;
			}
			return Info;
		}
		ITextRenderingInfo * D2DRenderDevice::CreateTextRenderingInfo(IFont * font, const Array<uint32>& text, int horizontal_align, int vertical_align, const Color & color)
		{
			auto Info = new TextRenderingInfo;
			try {
				Info->halign = horizontal_align;
				Info->valign = vertical_align;
				Info->Font = static_cast<D2DFont *>(font);
				Info->RenderTarget = Target;
				Info->HighlightBrush = 0;
				Info->TextBrush = 0;
				Info->Geometry = 0;
				if (Target->CreateSolidColorBrush(D2D1::ColorF(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f), &Info->TextBrush) != S_OK) throw Exception();
				Info->CharString = text;
				Info->FillGlyphs();
				Info->FillAdvances();
				Info->BuildGeometry();
			} catch (...) {
				delete Info;
			}
			return Info;
		}
		ILineRenderingInfo * D2DRenderDevice::CreateLineRenderingInfo(const Color & color, bool dotted)
		{

			LineRenderingInfo * Info = new (std::nothrow) LineRenderingInfo;
			if (!Info) throw OutOfMemoryException();
			Info->Brush = 0;
			Info->Stroke = 0;
			Target->CreateSolidColorBrush(D2D1::ColorF(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f), &Info->Brush);
			if (dotted) {
				float len[] = { 1.0f, 1.0f };
				D2DFactory->CreateStrokeStyle(D2D1::StrokeStyleProperties(D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_CAP_STYLE_FLAT, D2D1_LINE_JOIN_MITER, 10.0f, D2D1_DASH_STYLE_CUSTOM, 0.5f), len, 2, &Info->Stroke);
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
		IFont * D2DRenderDevice::LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout)
		{
			D2DFont * Font = new D2DFont;
			Font->FontFace = 0;
			Font->FontName = FaceName;
			Font->ActualHeight = Height;
			Font->Height = int(float(Height) * 72.0f / 96.0f);
			Font->Weight = Weight;
			Font->Italic = IsItalic;
			Font->Underline = IsUnderline;
			Font->Strikeout = IsStrikeout;
			IDWriteFontCollection * Collection = 0;
			IDWriteFontFamily * FontFamily = 0;
			IDWriteFont * FontSource = 0;
			try {
				if (DWriteFactory->GetSystemFontCollection(&Collection) != S_OK) throw Exception();
				uint32 Index;
				BOOL Exists;
				if (Collection->FindFamilyName(FaceName, &Index, &Exists) != S_OK) throw Exception();
				if (!Exists) throw Exception();
				if (Collection->GetFontFamily(Index, &FontFamily) != S_OK) throw Exception();
				if (FontFamily->GetFirstMatchingFont(DWRITE_FONT_WEIGHT(Weight), DWRITE_FONT_STRETCH_NORMAL, IsItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, &FontSource) != S_OK) throw Exception();
				if (FontSource->CreateFontFace(&Font->FontFace) != S_OK) throw Exception();
				if (Font->FontFace->GetGdiCompatibleMetrics(float(Height), 1.0f, 0, &Font->FontMetrics) != S_OK) throw Exception();
				FontSource->Release();
				FontSource = 0;
				FontFamily->Release();
				FontFamily = 0;
				if (Collection->FindFamilyName(L"Segoe UI Emoji", &Index, &Exists) == S_OK) {
					if (Exists) {
						if (Collection->GetFontFamily(Index, &FontFamily) == S_OK) {
							if (FontFamily->GetFirstMatchingFont(DWRITE_FONT_WEIGHT(Weight), DWRITE_FONT_STRETCH_NORMAL, IsItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL, &FontSource) == S_OK) {
								FontSource->CreateFontFace(&Font->AlternativeFace);
								if (Font->AlternativeFace) Font->AlternativeFace->GetGdiCompatibleMetrics(float(Height), 1.0f, 0, &Font->AltMetrics);
								FontSource->Release();
								FontSource = 0;
							}
							FontFamily->Release();
							FontFamily = 0;
						}
					}
				}
				Collection->Release();
				Collection = 0;
			}
			catch (...) {
				Font->Release();
				if (Collection) Collection->Release();
				if (FontFamily) FontFamily->Release();
				if (FontSource) FontSource->Release();
				throw;
			}
			return Font;
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
				Target->FillRectangle(To, info->Brushes[frame]);
			} else {
				D2D1_RECT_F From = D2D1::RectF(float(info->From.Left), float(info->From.Top), float(info->From.Right), float(info->From.Bottom));
				Target->DrawBitmap(info->Texture->Frames[frame], To, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, From);
			}
		}
		void D2DRenderDevice::RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip)
		{
			auto info = static_cast<TextRenderingInfo *>(Info);
			if (Clip) PushClip(At);
			int width, height;
			Info->GetExtent(width, height);
			D2D1_MATRIX_3X2_F Transform;
			Target->GetTransform(&Transform);
			int shift_x = At.Left;
			int shift_y = At.Top + info->BaselineOffset;
			if (info->halign == 1) shift_x += (At.Right - At.Left - width) / 2;
			else if (info->halign == 2) shift_x += (At.Right - At.Left - width);
			if (info->valign == 1) shift_y += (At.Bottom - At.Top - height) / 2;
			else if (info->valign == 2) shift_y += (At.Bottom - At.Top - height);
			if (info->hls != -1 && info->HighlightBrush) {
				int start = info->EndOfChar(info->hls - 1);
				int end = info->EndOfChar(info->hle - 1);
				D2D1_RECT_F Rect = D2D1::RectF(float(shift_x + start), float(At.Top), float(shift_x + end), float(At.Bottom));
				Target->FillRectangle(Rect, info->HighlightBrush);
			}
			Target->SetTransform(D2D1::Matrix3x2F::Translation(D2D1::SizeF(float(shift_x), float(shift_y))));
			Target->FillGeometry(info->Geometry, info->TextBrush, 0);
			if (info->Font->Underline) {
				Target->FillRectangle(D2D1::RectF(0.0f, info->UnderlineOffset - info->UnderlineHalfWidth, float(info->run_length), info->UnderlineOffset + info->UnderlineHalfWidth), info->TextBrush);
			}
			if (info->Font->Strikeout) {
				Target->FillRectangle(D2D1::RectF(0.0f, info->StrikeoutOffset - info->StrikeoutHalfWidth, float(info->run_length), info->StrikeoutOffset + info->StrikeoutHalfWidth), info->TextBrush);
			}
			Target->SetTransform(Transform);
			if (Clip) PopClip();
		}
		void D2DRenderDevice::RenderLine(ILineRenderingInfo * Info, const Box & At)
		{
			auto info = reinterpret_cast<LineRenderingInfo *>(Info);
			if (info->Brush) {
				D2D1_POINT_2F Start = D2D1::Point2F(float(At.Left) + 0.5f, float(At.Top) + 0.5f);
				D2D1_POINT_2F End = D2D1::Point2F(float(At.Right) + 0.5f, float(At.Bottom) + 0.5f);
				Target->DrawLine(Start, End, info->Brush, 1.0f, info->Stroke);
			}
		}
		void D2DRenderDevice::ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At)
		{
			throw Exception();
#pragma message ("METHOD NOT IMPLEMENTED, IMPLEMENT IT!")
		}
		void D2DRenderDevice::ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink)
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

		void TextRenderingInfo::FillAdvances(void)
		{
			GlyphAdvances.SetLength(GlyphString.Length());
			Array<DWRITE_GLYPH_METRICS> Metrics(0x40);
			Metrics.SetLength(GlyphString.Length());
			if (Font->FontFace->GetGdiCompatibleGlyphMetrics(float(Font->Height), 1.0f, 0, TRUE, GlyphString, GlyphString.Length(), Metrics) != S_OK) throw Exception();
			if (Font->AlternativeFace) {
				for (int i = 0; i < GlyphString.Length(); i++) if (UseAlternative[i]) {
					Font->AlternativeFace->GetGdiCompatibleGlyphMetrics(float(Font->Height), 1.0f, 0, TRUE, GlyphString.GetBuffer() + i, 1, Metrics.GetBuffer() + i);
				}
			}
			for (int i = 0; i < GlyphString.Length(); i++) GlyphAdvances[i] = (UseAlternative[i]) ? AltFontUnitsToDIP(Metrics[i].advanceWidth) : FontUnitsToDIP(Metrics[i].advanceWidth);
			BaselineOffset = int(FontUnitsToDIP(Font->FontMetrics.ascent));
			float rl = 0.0f;
			for (int i = 0; i < GlyphString.Length(); i++) rl += GlyphAdvances[i];
			run_length = int(rl);
		}
		void TextRenderingInfo::FillGlyphs(void)
		{
			uint32 Space = 0x20;
			Font->FontFace->GetGlyphIndicesW(&Space, 1, &NormalSpaceGlyph);
			Font->AlternativeFace->GetGlyphIndicesW(&Space, 1, &AlternativeSpaceGlyph);
			GlyphString.SetLength(CharString.Length());
			if (Font->FontFace->GetGlyphIndicesW(CharString, CharString.Length(), GlyphString) != S_OK) throw Exception();
			UseAlternative.SetLength(CharString.Length());
			for (int i = 0; i < UseAlternative.Length(); i++) {
				if (!GlyphString[i]) {
					if (Font->AlternativeFace && Font->AlternativeFace->GetGlyphIndicesW(CharString.GetBuffer() + i, 1, GlyphString.GetBuffer() + i) == S_OK && GlyphString[i]) UseAlternative[i] = true;
					else UseAlternative[i] = false;
				} else UseAlternative[i] = false;
			}
		}
		void TextRenderingInfo::BuildGeometry(void)
		{
			if (Geometry) { Geometry->Release(); Geometry = 0; }
			ID2D1GeometrySink * Sink;
			if (D2DFactory->CreatePathGeometry(&Geometry) != S_OK) throw Exception();
			if (Geometry->Open(&Sink) != S_OK) { Geometry->Release(); Geometry = 0; throw Exception(); }
			Array<uint16> Glyph = GlyphString;
			for (int i = 0; i < Glyph.Length(); i++) if (UseAlternative[i]) Glyph[i] = NormalSpaceGlyph;
			if (Font->FontFace->GetGlyphRunOutline(float(Font->Height), Glyph, GlyphAdvances, 0, GlyphString.Length(), FALSE, FALSE, static_cast<ID2D1SimplifiedGeometrySink*>(Sink)) != S_OK)
			{ Sink->Close(); Sink->Release(); Geometry->Release(); Geometry = 0; throw Exception(); }
			if (Font->AlternativeFace) {
				for (int i = 0; i < Glyph.Length(); i++) Glyph[i] = (UseAlternative[i]) ? GlyphString[i] : AlternativeSpaceGlyph;
				Font->AlternativeFace->GetGlyphRunOutline(float(Font->Height), Glyph, GlyphAdvances, 0, GlyphString.Length(), FALSE, FALSE, static_cast<ID2D1SimplifiedGeometrySink*>(Sink));
			}
			Sink->Close();
			Sink->Release();
			UnderlineOffset = -FontUnitsToDIP(int16(Font->FontMetrics.underlinePosition));
			UnderlineHalfWidth = FontUnitsToDIP(Font->FontMetrics.underlineThickness) / 2.0f;
			StrikeoutOffset = -FontUnitsToDIP(int16(Font->FontMetrics.strikethroughPosition));
			StrikeoutHalfWidth = FontUnitsToDIP(Font->FontMetrics.strikethroughThickness) / 2.0f;
		}
		float TextRenderingInfo::FontUnitsToDIP(int units) { return float(units) * float(Font->Height) / float(Font->FontMetrics.designUnitsPerEm); }
		float TextRenderingInfo::AltFontUnitsToDIP(int units) { return float(units) * float(Font->Height) / float(Font->AltMetrics.designUnitsPerEm); }
		void TextRenderingInfo::GetExtent(int & width, int & height) { width = run_length; height = Font->ActualHeight; }
	}
}


