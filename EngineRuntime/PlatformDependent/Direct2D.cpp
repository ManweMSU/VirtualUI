#include "Direct2D.h"

#include <d2d1_1helper.h>
#include <dwrite.h>

#include <math.h>

#include "ComInterop.h"
#include "Direct3D.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "dxguid.lib")

#undef max

namespace Engine
{
	namespace Direct2D
	{
		using namespace Codec;
		using namespace Streaming;

		class WICTexture;
		class D2DTexture;
		class DWFont;
		class D2DRenderingDevice;

		ID2D1Factory1 * D2DFactory1 = 0;
		ID2D1Factory * D2DFactory = 0;
		IWICImagingFactory * WICFactory = 0;
		IDWriteFactory * DWriteFactory = 0;

		void InitializeFactory(void)
		{
			if (!D2DFactory) {
				if (D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory1) != S_OK) {
					D2DFactory1 = 0;
					if (D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory) != S_OK) {
						D2DFactory = 0;
						throw Exception();
					}
				} else {
					D2DFactory = D2DFactory1;
				}
			}
			if (!WICFactory) {
				if (CoCreateInstance(CLSID_WICImagingFactory1, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&WICFactory)) != S_OK) throw Exception();
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

		class WicCodec : public ICodec
		{
		public:
			virtual void EncodeFrame(Stream * stream, Frame * frame, const string & format) override
			{
				SafePointer<Image> Fake = new Image;
				Fake->Frames.Append(frame);
				EncodeImage(stream, Fake, format);
			}
			virtual void EncodeImage(Stream * stream, Image * image, const string & format) override
			{
				if (!image->Frames.Length()) throw InvalidArgumentException();
				SafePointer<IWICBitmapEncoder> Encoder;
				if (format == L"BMP") {
					if (WICFactory->CreateEncoder(GUID_ContainerFormatBmp, 0, Encoder.InnerRef()) != S_OK) throw Exception();
				} else if (format == L"PNG") {
					if (WICFactory->CreateEncoder(GUID_ContainerFormatPng, 0, Encoder.InnerRef()) != S_OK) throw Exception();
				} else if (format == L"JPG") {
					if (WICFactory->CreateEncoder(GUID_ContainerFormatJpeg, 0, Encoder.InnerRef()) != S_OK) throw Exception();
				} else if (format == L"GIF") {
					if (WICFactory->CreateEncoder(GUID_ContainerFormatGif, 0, Encoder.InnerRef()) != S_OK) throw Exception();
				} else if (format == L"TIF") {
					if (WICFactory->CreateEncoder(GUID_ContainerFormatTiff, 0, Encoder.InnerRef()) != S_OK) throw Exception();
				} else if (format == L"DDS") {
					if (WICFactory->CreateEncoder(GUID_ContainerFormatDds, 0, Encoder.InnerRef()) != S_OK) throw Exception();
				} else throw InvalidArgumentException();
				SafePointer<ComStream> Output = new ComStream(stream);
				if (Encoder->Initialize(Output, WICBitmapEncoderNoCache) != S_OK) throw Exception();
				int max_frame = (format == L"GIF" || format == L"TIF") ? image->Frames.Length() : 1;
				for (int i = 0; i < max_frame; i++) {
					SafePointer<IWICBitmapFrameEncode> Frame;
					SafePointer<IPropertyBag2> Properties;
					if (Encoder->CreateNewFrame(Frame.InnerRef(), Properties.InnerRef()) == S_OK) {
						WICPixelFormatGUID PixelFormat = GUID_WICPixelFormat32bppBGRA;
						Frame->Initialize(Properties);
						Frame->SetSize(image->Frames[i].GetWidth(), image->Frames[i].GetHeight());
						Frame->SetPixelFormat(&PixelFormat);
						Codec::PixelFormat SourceFormat;
						Codec::AlphaMode SourceAlpha = Codec::AlphaMode::Normal;
						bool Deny = false;
						if (PixelFormat == GUID_WICPixelFormat32bppBGRA) {
							SourceFormat = PixelFormat::B8G8R8A8;
						} else if (PixelFormat == GUID_WICPixelFormat32bppPBGRA) {
							SourceFormat = PixelFormat::B8G8R8A8;
							SourceAlpha = Codec::AlphaMode::Premultiplied;
						} else if (PixelFormat == GUID_WICPixelFormat32bppRGBA) {
							SourceFormat = PixelFormat::R8G8B8A8;
						} else if (PixelFormat == GUID_WICPixelFormat32bppPRGBA) {
							SourceFormat = PixelFormat::R8G8B8A8;
							SourceAlpha = Codec::AlphaMode::Premultiplied;
						} else if (PixelFormat == GUID_WICPixelFormat32bppBGR) {
							SourceFormat = PixelFormat::B8G8R8X8;
						} else if (PixelFormat == GUID_WICPixelFormat32bppRGB) {
							SourceFormat = PixelFormat::R8G8B8X8;
						} else if (PixelFormat == GUID_WICPixelFormat24bppBGR) {
							SourceFormat = PixelFormat::B8G8R8;
						} else if (PixelFormat == GUID_WICPixelFormat24bppRGB) {
							SourceFormat = PixelFormat::R8G8B8;
						} else if (PixelFormat == GUID_WICPixelFormat16bppBGR555) {
							SourceFormat = PixelFormat::B5G5R5X1;
						} else if (PixelFormat == GUID_WICPixelFormat16bppBGRA5551) {
							SourceFormat = PixelFormat::B5G5R5A1;
						} else if (PixelFormat == GUID_WICPixelFormat16bppBGR565) {
							SourceFormat = PixelFormat::B5G6R5;
						} else if (PixelFormat == GUID_WICPixelFormat8bppGray) {
							SourceFormat = PixelFormat::R8;
						} else if (PixelFormat == GUID_WICPixelFormat8bppAlpha) {
							SourceFormat = PixelFormat::A8;
						} else if (PixelFormat == GUID_WICPixelFormat8bppIndexed) {
							SourceFormat = PixelFormat::P8;
						} else Deny = true;
						if (!Deny) {
							SafePointer<Engine::Codec::Frame> Conv = image->Frames[i].ConvertFormat(SourceFormat, SourceAlpha, ScanOrigin::TopDown);
							if (IsPalettePixel(SourceFormat)) {
								SafePointer<IWICPalette> Palette;
								WICFactory->CreatePalette(Palette.InnerRef());
								Palette->InitializeCustom(const_cast<WICColor *>(Conv->GetPalette()), Conv->GetPaletteVolume());
								Frame->SetPalette(Palette);
							}
							if (format == L"GIF") {
								SafePointer<IWICMetadataQueryWriter> MetaWriter;
								Frame->GetMetadataQueryWriter(MetaWriter.InnerRef());
								PROPVARIANT Value;
								PropVariantInit(&Value);
								Value.vt = VT_UI2;
								Value.uiVal = Conv->Duration / 10;
								MetaWriter->SetMetadataByName(L"/grctlext/Delay", &Value);
								PropVariantClear(&Value);
							}
							Frame->WritePixels(image->Frames[i].GetHeight(), Conv->GetScanLineLength(), Conv->GetScanLineLength() * Conv->GetHeight(), Conv->GetData());
							Frame->Commit();
						}
					}
				}
				Encoder->Commit();
			}
			virtual Frame * DecodeFrame(Stream * stream) override
			{
				SafePointer<Image> image = DecodeImage(stream);
				if (image) {
					SafePointer<Frame> frame = image->Frames.ElementAt(0);
					frame->Retain();
					frame->Retain();
					return frame;
				} else return 0;
			}
			virtual Image * DecodeImage(Stream * stream) override
			{
				Streaming::ComStream * Stream = new Streaming::ComStream(stream);
				IWICBitmapDecoder * Decoder = 0;
				IWICBitmapFrameDecode * FrameDecoder = 0;
				IWICMetadataQueryReader * Metadata = 0;
				IWICFormatConverter * Converter = 0;		
				SafePointer<Image> Result = new Image;
				try {
					uint32 FrameCount;
					HRESULT r;
					if (r = WICFactory->CreateDecoderFromStream(Stream, 0, WICDecodeMetadataCacheOnDemand, &Decoder) != S_OK) throw IO::FileFormatException();
					if (Decoder->GetFrameCount(&FrameCount) != S_OK) throw IO::FileFormatException();
					for (uint32 i = 0; i < FrameCount; i++) {
						if (Decoder->GetFrame(i, &FrameDecoder) != S_OK) throw IO::FileFormatException();
						uint32 fw, fh;
						if (FrameDecoder->GetSize(&fw, &fh) != S_OK) throw IO::FileFormatException();
						SafePointer<Frame> frame = new Frame(fw, fh, -1, PixelFormat::B8G8R8A8, AlphaMode::Normal, ScanOrigin::TopDown);
						if (FrameDecoder->GetMetadataQueryReader(&Metadata) == S_OK) {
							PROPVARIANT Value;
							PropVariantInit(&Value);
							if (Metadata->GetMetadataByName(L"/grctlext/Delay", &Value) == S_OK) {
								if (Value.vt == VT_UI2) {
									frame->Duration = uint(Value.uiVal) * 10;
								}
							}
							PropVariantClear(&Value);
							Metadata->Release();
							Metadata = 0;
						}
						if (WICFactory->CreateFormatConverter(&Converter) != S_OK) throw Exception();
						if (Converter->Initialize(FrameDecoder, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, 0, 0.0f, WICBitmapPaletteTypeCustom) != S_OK) throw IO::FileFormatException();
						if (Converter->CopyPixels(0, 4 * frame->GetWidth(), 4 * frame->GetWidth() * frame->GetHeight(), frame->GetData()) != S_OK) throw IO::FileFormatException();
						Converter->Release();
						Converter = 0;
						FrameDecoder->Release();
						FrameDecoder = 0;
						Result->Frames.Append(frame);
					}
					Decoder->Release();
					Decoder = 0;
				} catch (...) {
					Stream->Release();
					if (Decoder) Decoder->Release();
					if (FrameDecoder) FrameDecoder->Release();
					if (Metadata) Metadata->Release();
					if (Converter) Converter->Release();
					return 0;
				}
				Stream->Release();
				Result->Retain();
				return Result;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string ExamineData(Stream * stream) override
			{
				uint64 size = stream->Length() - stream->Seek(0, Current);
				if (size < 8) return L"";
				uint64 begin = stream->Seek(0, Current);
				uint64 sign;
				try {
					stream->Read(&sign, sizeof(sign));
					stream->Seek(begin, Begin);
				} catch (...) { return L""; }
				if ((sign & 0xFFFF) == 0x4D42) return L"BMP";
				else if (sign == 0x0A1A0A0D474E5089) return L"PNG";
				else if ((sign & 0xFFFFFF) == 0xFFD8FF) return L"JPG";
				else if ((sign & 0xFFFFFFFFFFFF) == 0x613938464947 || (sign & 0xFFFFFFFFFFFF) == 0x613738464947) return L"GIF";
				else if ((sign & 0xFFFFFFFF) == 0x2A004D4D) return L"TIF";
				else if ((sign & 0xFFFFFFFF) == 0x002A4949) return L"TIF";
				else if ((sign & 0xFFFFFFFF) == 0x20534444) return L"DDS";
				else if ((sign & 0xFFFFFFFF00000000) == 0x7079746600000000) return L"HEIF";
				else return L"";
			}
			virtual bool CanEncode(const string & format) override { return (format == L"BMP" || format == L"PNG" || format == L"JPG" || format == L"GIF" || format == L"TIF" || format == L"DDS"); }
			virtual bool CanDecode(const string & format) override { return (format == L"BMP" || format == L"PNG" || format == L"JPG" || format == L"GIF" || format == L"TIF" || format == L"DDS" || format == L"HEIF"); }
		};

		Engine::Codec::ICodec * _WicCodec = 0;
		Engine::Codec::ICodec * CreateWicCodec(void) { if (!_WicCodec) { InitializeFactory(); _WicCodec = new WicCodec(); _WicCodec->Release(); } return _WicCodec; }

		struct BarRenderingInfo : public IBarRenderingInfo
		{
			ID2D1GradientStopCollection * Collection = 0;
			ID2D1SolidColorBrush * Brush = 0;
			double prop_h = 0.0, prop_w = 0.0;
			~BarRenderingInfo(void) override { if (Collection) Collection->Release(); if (Brush) Brush->Release(); }
		};
		struct TextureRenderingInfo : public ITextureRenderingInfo
		{
			SafePointer<D2DTexture> Texture;
			Box From;
			SafePointer<ID2D1BitmapBrush> Brush;
			bool Fill;
			TextureRenderingInfo(void) {}
			~TextureRenderingInfo(void) override {}
		};
		struct TextRenderingInfo : public ITextRenderingInfo
		{
			struct TextRange
			{
				int LeftEdge;
				int RightEdge;
				BarRenderingInfo * Brush;
			};
			DWFont * Font;
			Array<uint32> CharString;
			Array<uint16> GlyphString;
			Array<float> GlyphAdvances;
			Array<uint8> UseAlternative;
			Array<TextRange> Ranges;
			ID2D1PathGeometry * Geometry;
			ID2D1SolidColorBrush * TextBrush;
			ID2D1SolidColorBrush * HighlightBrush;
			D2DRenderingDevice * Device;
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
			SafePointer<BarRenderingInfo> MainBrushInfo;
			SafePointer<BarRenderingInfo> BackBrushInfo;
			SafePointer< ObjectArray<BarRenderingInfo> > ExtraBrushes;

			TextRenderingInfo(void) : CharString(0x40), GlyphString(0x40), GlyphAdvances(0x40), UseAlternative(0x40), Ranges(0x10), hls(-1), hle(-1) {}

			void FillAdvances(void);
			void FillGlyphs(void);
			void BuildGeometry(void);
			float FontUnitsToDIP(int units);
			float AltFontUnitsToDIP(int units);

			virtual void GetExtent(int & width, int & height) noexcept override;
			virtual void SetHighlightColor(const Color & color) noexcept override
			{
				if (HighlightBrush) { HighlightBrush->Release(); HighlightBrush = 0; }
				Device->GetRenderTarget()->CreateSolidColorBrush(D2D1::ColorF(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f), &HighlightBrush);
			}
			virtual void HighlightText(int Start, int End) noexcept override { hls = Start; hle = End; }
			virtual int TestPosition(int point) noexcept override
			{
				if (point < 0) return 0;
				if (point > run_length) return CharString.Length();
				float p = float(point);
				float s = 0.0f;
				for (int i = 0; i < GlyphAdvances.Length(); i++) {
					if (p <= s + GlyphAdvances[i]) {
						if (p < s + GlyphAdvances[i] / 2.0f) return i;
						else return i + 1;
						break;
					}
					s += GlyphAdvances[i];
				}
				return CharString.Length();
			}
			virtual int EndOfChar(int Index) noexcept override
			{
				if (Index < 0) return 0;
				float summ = 0.0f;
				for (int i = 0; i <= Index; i++) summ += GlyphAdvances[i];
				return int(summ);
			}
			virtual void SetCharPalette(const Array<Color> & colors) override
			{
				ExtraBrushes.SetReference(new ObjectArray<BarRenderingInfo>(colors.Length()));
				for (int i = 0; i < colors.Length(); i++) {
					SafePointer<BarRenderingInfo> Brush = reinterpret_cast<BarRenderingInfo *>(Device->CreateBarRenderingInfo(colors[i]));
					ExtraBrushes->Append(Brush);
				}
			}
			virtual void SetCharColors(const Array<uint8> & indicies) override
			{
				Ranges.Clear();
				int cp = 0;
				float summ = 0.0f;
				while (cp < indicies.Length()) {
					float base = summ;
					int ep = cp;
					while (ep < indicies.Length() && indicies[ep] == indicies[cp]) { summ += GlyphAdvances[ep]; ep++; }
					int index = indicies[cp];
					Ranges << TextRange{ int(base), int(summ + 1.0), (index == 0) ? MainBrushInfo.Inner() : ExtraBrushes->ElementAt(index - 1) };
					cp = ep;
				}
			}
			virtual ~TextRenderingInfo(void) override
			{
				if (Geometry) Geometry->Release();
				if (TextBrush) TextBrush->Release();
				if (HighlightBrush) HighlightBrush->Release();
			}
		};
		struct BlurEffectRenderingInfo : public IBlurEffectRenderingInfo
		{
			SafePointer<ID2D1Effect> Effect;
			virtual ~BlurEffectRenderingInfo(void) override {}
		};
		struct InversionEffectRenderingInfo : public IInversionEffectRenderingInfo
		{
			SafePointer<ID2D1Effect> Effect;
			SafePointer<IBarRenderingInfo> WhiteBar;
			SafePointer<IBarRenderingInfo> BlackBar;
			virtual ~InversionEffectRenderingInfo(void) override {}
		};
		class WICTexture : public ITexture
		{
			struct dev_pair { ITexture * dev_tex; IRenderingDevice * dev_ptr; };
		public:
			Array<dev_pair> variants;
			IWICBitmap * bitmap;
			int w, h;
			WICTexture(void) : variants(0x10), bitmap(0) {}
			virtual ~WICTexture(void)
			{
				if (bitmap) bitmap->Release();
				for (int i = 0; i < variants.Length(); i++) {
					variants[i].dev_tex->VersionWasDestroyed(this);
					variants[i].dev_ptr->TextureWasDestroyed(this);
				}
			}
			virtual int GetWidth(void) const noexcept override { return w; }
			virtual int GetHeight(void) const noexcept override { return h; }

			virtual void VersionWasDestroyed(ITexture * texture) noexcept override
			{
				for (int i = 0; i < variants.Length(); i++) if (variants[i].dev_tex == texture) { variants.Remove(i); break; }
			}
			virtual void DeviceWasDestroyed(IRenderingDevice * device) noexcept override
			{
				for (int i = 0; i < variants.Length(); i++) if (variants[i].dev_ptr == device) { variants.Remove(i); break; }
			}
			virtual void AddDeviceVersion(IRenderingDevice * device, ITexture * texture) noexcept override
			{
				for (int i = 0; i < variants.Length(); i++) if (variants[i].dev_ptr == device) return;
				variants << dev_pair{ texture, device };
			}
			virtual bool IsDeviceSpecific(void) const noexcept override { return false; }
			virtual IRenderingDevice * GetParentDevice(void) const noexcept override { return 0; }
			virtual ITexture * GetDeviceVersion(IRenderingDevice * target_device) noexcept override
			{
				if (!target_device) return this;
				for (int i = 0; i < variants.Length(); i++) if (variants[i].dev_ptr == target_device) return variants[i].dev_tex;
				return 0;
			}

			virtual void Reload(Codec::Frame * source) override
			{
				if (bitmap) bitmap->Release();
				SafePointer<WICTexture> new_texture = static_cast<WICTexture *>(StandaloneDevice::LoadTexture(source));
				bitmap = new_texture->bitmap;
				w = new_texture->w; h = new_texture->h;
				new_texture->bitmap = 0;
				for (int i = 0; i < variants.Length(); i++) variants[i].dev_tex->Reload(this);
			}
			virtual void Reload(ITexture * device_independent) override {}
			virtual string ToString(void) const override { return L"Engine.Direct2D.WICTexture"; }
		};
		class D2DTexture : public ITexture
		{
		public:
			WICTexture * base;
			IRenderingDevice * factory_device;
			ID2D1Bitmap * bitmap;
			int w, h;
			D2DTexture(void) : base(0), factory_device(0), bitmap(0) {}
			virtual ~D2DTexture(void)
			{
				if (bitmap) bitmap->Release();
				if (factory_device) factory_device->TextureWasDestroyed(this);
				if (base) base->VersionWasDestroyed(this);
			}
			virtual int GetWidth(void) const noexcept override { return w; }
			virtual int GetHeight(void) const noexcept override { return h; }

			virtual void VersionWasDestroyed(ITexture * texture) noexcept override { if (texture == base) { base = 0; factory_device = 0; } }
			virtual void DeviceWasDestroyed(IRenderingDevice * device) noexcept override { if (device == factory_device) { base = 0; factory_device = 0; } }
			virtual void AddDeviceVersion(IRenderingDevice * device, ITexture * texture) noexcept override {}
			virtual bool IsDeviceSpecific(void) const noexcept override { return true; }
			virtual IRenderingDevice * GetParentDevice(void) const noexcept override { return factory_device; }
			virtual ITexture * GetDeviceVersion(IRenderingDevice * target_device) noexcept override
			{
				if (factory_device && target_device == factory_device) return this;
				else if (!target_device) return base;
				return 0;
			}

			virtual void Reload(Codec::Frame * source) override { SafePointer<ITexture> base = StandaloneDevice::LoadTexture(source); Reload(base); }
			virtual void Reload(ITexture * device_independent) override
			{
				WICTexture * source = static_cast<WICTexture *>(device_independent);
				w = source->w; h = source->h;
				SafePointer<ID2D1Bitmap> texture;
				auto device = static_cast<D2DRenderingDevice *>(factory_device);
				auto render_target = device->GetRenderTarget();
				if (render_target->CreateBitmapFromWicBitmap(source->bitmap, texture.InnerRef()) != S_OK) throw Exception();
				texture->AddRef();
				bitmap = texture;
			}
			virtual string ToString(void) const override { return L"Engine.Direct2D.D2DTexture"; }
		};
		class DWFont : public IFont
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
			virtual ~DWFont(void) override { if (FontFace) FontFace->Release(); if (AlternativeFace) AlternativeFace->Release(); }
			virtual int GetWidth(void) const noexcept override
			{
				uint16 SpaceGlyph;
				uint32 Space = 0x20;
				FontFace->GetGlyphIndicesW(&Space, 1, &SpaceGlyph);
				DWRITE_GLYPH_METRICS SpaceMetrics;
				FontFace->GetGdiCompatibleGlyphMetrics(float(Height), 1.0f, 0, TRUE, &SpaceGlyph, 1, &SpaceMetrics);
				return int(float(SpaceMetrics.advanceWidth) * float(Height) / float(FontMetrics.designUnitsPerEm));
			}
			virtual int GetHeight(void) const noexcept override { return ActualHeight; }
			virtual string ToString(void) const override { return L"Engine.Direct2D.DWFont"; }
		};

		namespace StandaloneDevice
		{
			ITexture * LoadTexture(Streaming::Stream * Source)
			{
				SafePointer<Image> image = Engine::Codec::DecodeImage(Source);
				if (!image) throw InvalidFormatException();
				return LoadTexture(image);
			}
			ITexture * LoadTexture(Engine::Codec::Image * Source)
			{
				if (!Source->Frames.Length()) throw InvalidArgumentException();
				return LoadTexture(Source->GetFrameBestDpiFit(Zoom));
			}
			ITexture * LoadTexture(Engine::Codec::Frame * Source)
			{
				SafePointer<IWICBitmap> Bitmap;
				if (WICFactory->CreateBitmap(Source->GetWidth(), Source->GetHeight(), GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnDemand, Bitmap.InnerRef()) != S_OK) throw Exception();
				SafePointer<Frame> conv = Source->ConvertFormat(PixelFormat::B8G8R8A8, Codec::AlphaMode::Premultiplied, ScanOrigin::TopDown);
				IWICBitmapLock * Lock;
				Bitmap->Lock(0, WICBitmapLockRead | WICBitmapLockWrite, &Lock);
				UINT len;
				uint8 * data;
				Lock->GetDataPointer(&len, &data);
				MemoryCopy(data, conv->GetData(), intptr(conv->GetScanLineLength()) * intptr(conv->GetHeight()));
				Lock->Release();
				WICTexture * Texture = new (std::nothrow) WICTexture;
				if (!Texture) { throw OutOfMemoryException(); }
				try {
					Bitmap->AddRef();
					Texture->bitmap = Bitmap;
					Texture->w = Source->GetWidth();
					Texture->h = Source->GetHeight();
				} catch (...) {
					if (Texture) Texture->Release();
					throw;
				}
				return Texture;
			}
			IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout)
			{
				DWFont * Font = new DWFont;
				Font->AlternativeFace = 0;
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
				} catch (...) {
					Font->Release();
					if (Collection) Collection->Release();
					if (FontFamily) FontFamily->Release();
					if (FontSource) FontSource->Release();
					throw;
				}
				return Font;
			}
		}

		D2DRenderingDevice::D2DRenderingDevice(ID2D1DeviceContext * target) :
			ExtendedTarget(target), Target(target), Layers(0x10), Clipping(0x20), BrushCache(0x100, Dictionary::ExcludePolicy::ExcludeLeastRefrenced),
			BlurCache(0x10, Dictionary::ExcludePolicy::ExcludeLeastRefrenced),
			TextureCache(0x100), BitmapTargetState(0), BitmapTargetResX(0), BitmapTargetResY(0), ParentWrappedDevice(0)
		{ Target->AddRef(); HalfBlinkPeriod = GetCaretBlinkTime(); BlinkPeriod = HalfBlinkPeriod * 2; }
		D2DRenderingDevice::D2DRenderingDevice(ID2D1RenderTarget * target) :
			ExtendedTarget(0), Target(target), Layers(0x10), Clipping(0x20), BrushCache(0x100, Dictionary::ExcludePolicy::ExcludeLeastRefrenced),
			BlurCache(0x10, Dictionary::ExcludePolicy::ExcludeLeastRefrenced),
			TextureCache(0x100), BitmapTargetState(0), BitmapTargetResX(0), BitmapTargetResY(0), ParentWrappedDevice(0)
		{ Target->AddRef(); HalfBlinkPeriod = GetCaretBlinkTime(); BlinkPeriod = HalfBlinkPeriod * 2; }
		D2DRenderingDevice::~D2DRenderingDevice(void)
		{
			if (Target) Target->Release();
			for (int i = 0; i < TextureCache.Length(); i++) {
				TextureCache[i].base->DeviceWasDestroyed(this);
				TextureCache[i].spec->DeviceWasDestroyed(this);
			}
		}
		ID2D1RenderTarget * D2DRenderingDevice::GetRenderTarget(void) const noexcept { return Target; }
		void D2DRenderingDevice::UpdateRenderTarget(ID2D1RenderTarget * target) noexcept
		{
			if (Target) { Target->Release(); Target = 0; }
			if (target) { Target = target; Target->AddRef(); }
		}
		void D2DRenderingDevice::SetParentWrappedDevice(Graphics::IDevice * device) noexcept { ParentWrappedDevice = device; }
		void D2DRenderingDevice::TextureWasDestroyed(ITexture * texture) noexcept
		{
			for (int i = 0; i < TextureCache.Length(); i++) {
				if (TextureCache[i].base == texture || TextureCache[i].spec == texture) {
					TextureCache.Remove(i);
					break;
				}
			}
		}
		string D2DRenderingDevice::ToString(void) const { return L"Engine.Direct2D.D2DRenderingDevice"; }
		IBarRenderingInfo * D2DRenderingDevice::CreateBarRenderingInfo(const Array<GradientPoint>& gradient, double angle) noexcept
		{
			try {
				if (gradient.Length() > 1) {
					D2D1_GRADIENT_STOP * stop = new (std::nothrow) D2D1_GRADIENT_STOP[gradient.Length()];
					if (!stop) { return 0; }
					for (int i = 0; i < gradient.Length(); i++) {
						stop[i].color = D2D1::ColorF(gradient[i].Color.r / 255.0f, gradient[i].Color.g / 255.0f, gradient[i].Color.b / 255.0f, gradient[i].Color.a / 255.0f);
						stop[i].position = float(gradient[i].Position);
					}
					ID2D1GradientStopCollection * Collection;
					if (Target->CreateGradientStopCollection(stop, gradient.Length(), &Collection) != S_OK) { delete[] stop; return 0; }
					BarRenderingInfo * Info = new (std::nothrow) BarRenderingInfo;
					if (!Info) { delete[] stop; Collection->Release(); return 0; }
					Info->Collection = Collection;
					Info->prop_w = cos(angle), Info->prop_h = sin(angle);
					delete[] stop;
					return Info;
				} else {
					auto CachedInfo = BrushCache.ElementByKey(gradient[0].Color);
					if (CachedInfo) {
						CachedInfo->Retain();
						return CachedInfo;
					}
					BarRenderingInfo * Info = new (std::nothrow) BarRenderingInfo;
					if (!Info) { return 0; }
					ID2D1SolidColorBrush * Brush;
					if (Target->CreateSolidColorBrush(D2D1::ColorF(gradient[0].Color.r / 255.0f, gradient[0].Color.g / 255.0f, gradient[0].Color.b / 255.0f, gradient[0].Color.a / 255.0f), &Brush) != S_OK) { Info->Release(); return 0; }
					Info->Brush = Brush;
					BrushCache.Append(gradient[0].Color, Info);
					return Info;
				}
			} catch (...) { return 0; }
		}
		IBarRenderingInfo * D2DRenderingDevice::CreateBarRenderingInfo(Color color) noexcept
		{
			try {
				auto CachedInfo = BrushCache.ElementByKey(color);
				if (CachedInfo) {
					CachedInfo->Retain();
					return CachedInfo;
				}
				BarRenderingInfo * Info = new (std::nothrow) BarRenderingInfo;
				if (!Info) { return 0; }
				ID2D1SolidColorBrush * Brush;
				if (Target->CreateSolidColorBrush(D2D1::ColorF(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f), &Brush) != S_OK) { Info->Release(); return 0; }
				Info->Brush = Brush;
				BrushCache.Append(color, Info);
				return Info;
			} catch (...) { return 0; }
		}
		IBlurEffectRenderingInfo * D2DRenderingDevice::CreateBlurEffectRenderingInfo(double power) noexcept
		{
			try {
				auto CachedInfo = BlurCache.ElementByKey(power);
				if (CachedInfo) {
					CachedInfo->Retain();
					return CachedInfo;
				}
				auto Info = new (std::nothrow) BlurEffectRenderingInfo;
				if (!Info) return 0;
				if (ExtendedTarget) {
					if (ExtendedTarget->CreateEffect(CLSID_D2D1GaussianBlur, Info->Effect.InnerRef()) != S_OK) { Info->Release(); return 0; }
					Info->Effect->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, float(power));
					Info->Effect->SetValue(D2D1_GAUSSIANBLUR_PROP_OPTIMIZATION, D2D1_GAUSSIANBLUR_OPTIMIZATION_SPEED);
					Info->Effect->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD);
				}
				BlurCache.Append(power, Info);
				return Info;
			} catch (...) { return 0; }
		}
		IInversionEffectRenderingInfo * D2DRenderingDevice::CreateInversionEffectRenderingInfo(void) noexcept
		{
			if (!InversionInfo.Inner()) {
				auto Info = new (std::nothrow) InversionEffectRenderingInfo;
				if (!Info) return 0;
				if (ExtendedTarget) {
					if (ExtendedTarget->CreateEffect(CLSID_D2D1ColorMatrix, Info->Effect.InnerRef()) != S_OK) { Info->Release(); return 0; }
					Info->Effect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, D2D1::Matrix5x4F(
						-1.0f, 0.0f, 0.0f, 0.0f,
						0.0f, -1.0f, 0.0f, 0.0f,
						0.0f, 0.0f, -1.0f, 0.0f,
						0.0f, 0.0f, 0.0f, 1.0f,
						1.0f, 1.0f, 1.0f, 0.0f
					));
				} else {
					Info->BlackBar.SetReference(CreateBarRenderingInfo(UI::Color(0, 0, 0)));
					Info->WhiteBar.SetReference(CreateBarRenderingInfo(UI::Color(255, 255, 255)));
				}
				InversionInfo.SetReference(Info);
			}
			InversionInfo->Retain();
			return InversionInfo;
		}
		ITextureRenderingInfo * D2DRenderingDevice::CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern) noexcept
		{
			if (!texture) return 0;
			try {
				SafePointer<D2DTexture> device_texture;
				if (texture->IsDeviceSpecific()) {
					if (texture->GetParentDevice() == this) device_texture.SetRetain(static_cast<D2DTexture *>(texture));
					else return 0;
				} else {
					ITexture * cached = texture->GetDeviceVersion(this);
					if (!cached) {
						device_texture.SetReference(new (std::nothrow) D2DTexture);
						if (!device_texture) return 0;
						texture->AddDeviceVersion(this, device_texture);
						TextureCache << tex_pair{ texture, device_texture };
						device_texture->base = static_cast<WICTexture *>(texture);
						device_texture->factory_device = this;
						device_texture->Reload(texture);
					} else device_texture.SetRetain(static_cast<D2DTexture *>(cached));
				}
				auto * Info = new (std::nothrow) TextureRenderingInfo;
				if (!Info) return 0;
				Info->Texture = device_texture;
				Info->From = take_area;
				Info->Fill = fill_pattern;
				if (fill_pattern) {
					SafePointer<ID2D1Bitmap> fragment;
					if (take_area.Left == 0 && take_area.Top == 0 && take_area.Right == Info->Texture->w && take_area.Bottom == Info->Texture->h) {
						fragment.SetReference(Info->Texture->bitmap);
						fragment->AddRef();
					} else {
						if (Target->CreateBitmap(D2D1::SizeU(uint(take_area.Right - take_area.Left), uint(take_area.Bottom - take_area.Top)),
							D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f), fragment.InnerRef()) != S_OK) {
							Info->Release(); throw Exception();
						}
						if (fragment->CopyFromBitmap(&D2D1::Point2U(0, 0), Info->Texture->bitmap, &D2D1::RectU(take_area.Left, take_area.Top, take_area.Right, take_area.Bottom)) != S_OK)
						{ Info->Release(); throw Exception(); }
					}
					ID2D1BitmapBrush * Brush;
					if (Target->CreateBitmapBrush(fragment, D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR), &Brush) != S_OK)
					{ Info->Release(); throw Exception(); }
					Info->Brush.SetReference(Brush);
				}
				return Info;
			} catch (...) { return 0; }
		}
		ITextureRenderingInfo * D2DRenderingDevice::CreateTextureRenderingInfo(Graphics::ITexture * texture) noexcept
		{
			if (!texture) return 0;
			if (texture->GetParentDevice() != ParentWrappedDevice) return 0;
			if (!(texture->GetResourceUsage() & Graphics::ResourceUsageShaderRead)) return 0;
			if (texture->GetTextureType() != Graphics::TextureType::Type2D) return 0;
			if (texture->GetPixelFormat() != Graphics::PixelFormat::B8G8R8A8_unorm) return 0;
			if (texture->GetMipmapCount() != 1) return 0;
			auto resource = static_cast<ID3D11Texture2D *>(Direct3D::QueryInnerObject(texture));
			IDXGISurface * surface = 0;
			if (resource->QueryInterface(__uuidof(IDXGISurface), reinterpret_cast<void **>(&surface)) != S_OK) return 0;
			D2D1_BITMAP_PROPERTIES props;
			props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
			props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
			props.dpiX = props.dpiY = 0.0f;
			ID2D1Bitmap * bitmap = 0;
			if (Target->CreateSharedBitmap(__uuidof(IDXGISurface), surface, &props, &bitmap) != S_OK) {
				surface->Release();
				return 0;
			}
			surface->Release();
			SafePointer<D2DTexture> cover = new (std::nothrow) D2DTexture;
			if (!cover) {
				bitmap->Release();
				return 0;
			}
			cover->bitmap = bitmap;
			cover->w = texture->GetWidth();
			cover->h = texture->GetHeight();
			auto info = new (std::nothrow) TextureRenderingInfo;
			if (!info) return 0;
			info->Texture = cover;
			info->From = UI::Box(0, 0, cover->w, cover->h);
			info->Fill = false;
			return info;
		}
		ITextRenderingInfo * D2DRenderingDevice::CreateTextRenderingInfo(IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color) noexcept
		{
			if (!font) return 0;
			TextRenderingInfo * Info = 0;
			try {
				Info = new TextRenderingInfo;
				Array<GradientPoint> array(1);
				array << GradientPoint(color, 0.0);
				Info->halign = horizontal_align;
				Info->valign = vertical_align;
				Info->Font = static_cast<DWFont *>(font);
				Info->Device = this;
				Info->HighlightBrush = 0;
				Info->TextBrush = 0;
				Info->Geometry = 0;
				Info->run_length = 0;
				Info->MainBrushInfo.SetReference(static_cast<BarRenderingInfo *>(CreateBarRenderingInfo(array, 0.0)));
				Info->MainBrushInfo->Retain();
				Info->TextBrush = Info->MainBrushInfo->Brush;
				Info->TextBrush->AddRef();
				if (text.Length()) {
					Info->CharString.SetLength(text.GetEncodedLength(Encoding::UTF32));
					text.Encode(Info->CharString, Encoding::UTF32, false);
					Info->FillGlyphs();
					Info->FillAdvances();
					Info->BuildGeometry();
				}
			}
			catch (...) {
				Info->Release();
				return 0;
			}
			return Info;
		}
		ITextRenderingInfo * D2DRenderingDevice::CreateTextRenderingInfo(IFont * font, const Array<uint32>& text, int horizontal_align, int vertical_align, const Color & color) noexcept
		{
			if (!font) return 0;
			TextRenderingInfo * Info = 0; 
			try {
				Info = new TextRenderingInfo;
				Array<GradientPoint> array(1);
				array << GradientPoint(color, 0.0);
				Info->halign = horizontal_align;
				Info->valign = vertical_align;
				Info->Font = static_cast<DWFont *>(font);
				Info->Device = this;
				Info->HighlightBrush = 0;
				Info->TextBrush = 0;
				Info->Geometry = 0;
				Info->run_length = 0;
				Info->MainBrushInfo.SetReference(static_cast<BarRenderingInfo *>(CreateBarRenderingInfo(array, 0.0)));
				Info->MainBrushInfo->Retain();
				Info->TextBrush = Info->MainBrushInfo->Brush;
				Info->TextBrush->AddRef();
				if (text.Length()) {
					Info->CharString = text;
					Info->FillGlyphs();
					Info->FillAdvances();
					Info->BuildGeometry();
				}
			} catch (...) {
				Info->Release();
				return 0;
			}
			return Info;
		}
		ITexture * D2DRenderingDevice::LoadTexture(Streaming::Stream * Source) { return StandaloneDevice::LoadTexture(Source); }
		ITexture * D2DRenderingDevice::LoadTexture(Engine::Codec::Image * Source) { return StandaloneDevice::LoadTexture(Source); }
		ITexture * D2DRenderingDevice::LoadTexture(Engine::Codec::Frame * Source) { return StandaloneDevice::LoadTexture(Source); }
		IFont * D2DRenderingDevice::LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) { return StandaloneDevice::LoadFont(FaceName, Height, Weight, IsItalic, IsUnderline, IsStrikeout); }
		Graphics::ITexture * D2DRenderingDevice::CreateIntermediateRenderTarget(Graphics::PixelFormat format, int width, int height)
		{
			if (!ParentWrappedDevice) return 0;
			if (width <= 0 || height <= 0) throw InvalidArgumentException();
			if (format != Graphics::PixelFormat::B8G8R8A8_unorm) throw InvalidArgumentException();
			Graphics::TextureDesc desc;
			desc.Type = Graphics::TextureType::Type2D;
			desc.Format = Graphics::PixelFormat::B8G8R8A8_unorm;
			desc.Width = width;
			desc.Height = height;
			desc.DepthOrArraySize = 1;
			desc.MipmapCount = 1;
			desc.Usage = Graphics::ResourceUsageRenderTarget | Graphics::ResourceUsageShaderRead;
			desc.MemoryPool = Graphics::ResourceMemoryPool::Default;
			return ParentWrappedDevice->CreateTexture(desc);
		}
		void D2DRenderingDevice::RenderBar(IBarRenderingInfo * Info, const Box & At) noexcept
		{
			if (!Info) return;
			auto info = reinterpret_cast<BarRenderingInfo *>(Info);
			if (info->Brush) {
				Target->FillRectangle(D2D1::RectF(float(At.Left), float(At.Top), float(At.Right), float(At.Bottom)), info->Brush);
			} else {
				int w = At.Right - At.Left, h = At.Bottom - At.Top;
				if (!w || !h) return;
				D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES bp;
				double cx = double(At.Right + At.Left) / 2.0;
				double cy = double(At.Top + At.Bottom) / 2.0;
				double rx = double(w) / 2.0;
				double ry = double(h) / 2.0;
				double mx = max(abs(info->prop_w), abs(info->prop_h));
				double boundary_sx = info->prop_w / mx;
				double boundary_sy = info->prop_h / mx;
				bp.startPoint.x = float(cx - rx * boundary_sx);
				bp.startPoint.y = float(cy + ry * boundary_sy);
				bp.endPoint.x = float(cx + rx * boundary_sx);
				bp.endPoint.y = float(cy - ry * boundary_sy);
				ID2D1LinearGradientBrush * Brush;
				if (Target->CreateLinearGradientBrush(bp, info->Collection, &Brush) == S_OK) {
					Target->FillRectangle(D2D1::RectF(float(At.Left), float(At.Top), float(At.Right), float(At.Bottom)), Brush);
					Brush->Release();
				}
			}
		}
		void D2DRenderingDevice::RenderTexture(ITextureRenderingInfo * Info, const Box & At) noexcept
		{
			if (!Info) return;
			auto info = static_cast<TextureRenderingInfo *>(Info);
			D2D1_RECT_F To = D2D1::RectF(float(At.Left), float(At.Top), float(At.Right), float(At.Bottom));
			if (info->Fill) {
				Target->FillRectangle(To, info->Brush);
			} else {
				D2D1_RECT_F From = D2D1::RectF(float(info->From.Left), float(info->From.Top), float(info->From.Right), float(info->From.Bottom));
				Target->DrawBitmap(info->Texture->bitmap, To, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, From);
			}
		}
		void D2DRenderingDevice::RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip) noexcept
		{
			if (!Info) return;
			auto info = static_cast<TextRenderingInfo *>(Info);
			if (!info->GlyphAdvances.Length()) return;
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
			if (info->Ranges.Length()) {
				for (int i = 0; i < info->Ranges.Length(); i++) {
					Target->PushAxisAlignedClip(
						D2D1::RectF(float(info->Ranges[i].LeftEdge), float(At.Top - shift_y), float(info->Ranges[i].RightEdge), float(At.Bottom - shift_y)),
						D2D1_ANTIALIAS_MODE_ALIASED);
					Target->FillGeometry(info->Geometry, info->Ranges[i].Brush->Brush, 0);
					Target->PopAxisAlignedClip();
				}
			} else {
				Target->FillGeometry(info->Geometry, info->TextBrush, 0);
			}
			if (info->Font->Underline) {
				Target->FillRectangle(D2D1::RectF(0.0f, info->UnderlineOffset - info->UnderlineHalfWidth, float(info->run_length), info->UnderlineOffset + info->UnderlineHalfWidth), info->TextBrush);
			}
			if (info->Font->Strikeout) {
				Target->FillRectangle(D2D1::RectF(0.0f, info->StrikeoutOffset - info->StrikeoutHalfWidth, float(info->run_length), info->StrikeoutOffset + info->StrikeoutHalfWidth), info->TextBrush);
			}
			Target->SetTransform(Transform);
			if (Clip) PopClip();
		}
		void D2DRenderingDevice::ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) noexcept
		{
			if (Layers.Length() || !Info) return;
			auto info = static_cast<BlurEffectRenderingInfo *>(Info);
			if (info->Effect) {
				SafePointer<ID2D1Bitmap> Fragment;
				Box Corrected = At;
				if (Corrected.Left < 0) Corrected.Left = 0;
				if (Corrected.Top < 0) Corrected.Top = 0;
				if (Target->CreateBitmap(D2D1::SizeU(Corrected.Right - Corrected.Left, Corrected.Bottom - Corrected.Top), D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f), Fragment.InnerRef()) == S_OK) {
					for (int i = 0; i < Clipping.Length(); i++) Target->PopAxisAlignedClip();
					Fragment->CopyFromRenderTarget(&D2D1::Point2U(0, 0), Target, &D2D1::RectU(Corrected.Left, Corrected.Top, Corrected.Right, Corrected.Bottom));
					for (int i = 0; i < Clipping.Length(); i++) Target->PushAxisAlignedClip(D2D1::RectF(float(Clipping[i].Left), float(Clipping[i].Top), float(Clipping[i].Right), float(Clipping[i].Bottom)), D2D1_ANTIALIAS_MODE_ALIASED);
					info->Effect->SetInput(0, Fragment);
					ExtendedTarget->DrawImage(info->Effect, D2D1::Point2F(float(Corrected.Left), float(Corrected.Top)), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2D1_COMPOSITE_MODE_SOURCE_OVER);
				}
			}
		}
		void D2DRenderingDevice::ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink) noexcept
		{
			if (Layers.Length() || !Info) return;
			auto info = static_cast<InversionEffectRenderingInfo *>(Info);
			if (info->Effect) {
				if (!Blink || (AnimationTimer % BlinkPeriod) < HalfBlinkPeriod) {
					SafePointer<ID2D1Bitmap> Fragment;
					Box Corrected = At;
					if (Corrected.Left < 0) Corrected.Left = 0;
					if (Corrected.Top < 0) Corrected.Top = 0;
					if (Target->CreateBitmap(D2D1::SizeU(Corrected.Right - Corrected.Left, Corrected.Bottom - Corrected.Top), D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f), Fragment.InnerRef()) == S_OK) {
						for (int i = 0; i < Clipping.Length(); i++) Target->PopAxisAlignedClip();
						Fragment->CopyFromRenderTarget(&D2D1::Point2U(0, 0), Target, &D2D1::RectU(Corrected.Left, Corrected.Top, Corrected.Right, Corrected.Bottom));
						for (int i = 0; i < Clipping.Length(); i++) Target->PushAxisAlignedClip(D2D1::RectF(float(Clipping[i].Left), float(Clipping[i].Top), float(Clipping[i].Right), float(Clipping[i].Bottom)), D2D1_ANTIALIAS_MODE_ALIASED);
						info->Effect->SetInput(0, Fragment);
						ExtendedTarget->DrawImage(info->Effect, D2D1::Point2F(float(Corrected.Left), float(Corrected.Top)), D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR, D2D1_COMPOSITE_MODE_SOURCE_OVER);
					}
				}
			} else {
				if (!Blink || (AnimationTimer % BlinkPeriod) < HalfBlinkPeriod) {
					RenderBar(info->BlackBar, At);
				} else {
					RenderBar(info->WhiteBar, At);
				}
			}
		}
		void D2DRenderingDevice::PushClip(const Box & Rect) noexcept
		{
			try {
				Clipping << Rect;
				Target->PushAxisAlignedClip(D2D1::RectF(float(Rect.Left), float(Rect.Top), float(Rect.Right), float(Rect.Bottom)), D2D1_ANTIALIAS_MODE_ALIASED);
			} catch (...) {}
		}
		void D2DRenderingDevice::PopClip(void) noexcept
		{
			Clipping.RemoveLast();
			Target->PopAxisAlignedClip();
		}
		void D2DRenderingDevice::BeginLayer(const Box & Rect, double Opacity) noexcept
		{
			try {
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
			} catch (...) {}
		}
		void D2DRenderingDevice::EndLayer(void) noexcept
		{
			Target->PopLayer();
			Layers.LastElement()->Release();
			Layers.RemoveLast();
		}
		void D2DRenderingDevice::SetTimerValue(uint32 time) noexcept { AnimationTimer = time; }
		uint32 D2DRenderingDevice::GetCaretBlinkHalfTime(void) noexcept { return HalfBlinkPeriod; }
		bool D2DRenderingDevice::CaretShouldBeVisible(void) noexcept { return (AnimationTimer % BlinkPeriod) < HalfBlinkPeriod; }
		void D2DRenderingDevice::ClearCache(void) noexcept { InversionInfo.SetReference(0); BrushCache.Clear(); BlurCache.Clear(); }
		Drawing::ICanvasRenderingDevice * D2DRenderingDevice::QueryCanvasDevice(void) noexcept { return this; }
		void D2DRenderingDevice::DrawPolygon(const Math::Vector2 * points, int count, const Math::Color & color, double width) noexcept
		{
			SafePointer<ID2D1PathGeometry> geometry;
			SafePointer<ID2D1SolidColorBrush> brush;
			if (D2DFactory->CreatePathGeometry(geometry.InnerRef()) != S_OK) return;
			if (count) {
				SafePointer<ID2D1GeometrySink> sink;
				if (geometry->Open(sink.InnerRef()) != S_OK) return;
				sink->BeginFigure(D2D1::Point2F(float(points[0].x), float(points[0].y)), D2D1_FIGURE_BEGIN_FILLED);
				for (int i = 1; i < count; i++) sink->AddLine(D2D1::Point2F(float(points[i].x), float(points[i].y)));
				sink->EndFigure(D2D1_FIGURE_END_OPEN);
				sink->Close();
			}
			if (Target->CreateSolidColorBrush(D2D1::ColorF(float(color.x), float(color.y), float(color.z), float(color.w)), brush.InnerRef()) != S_OK) return;
			Target->DrawGeometry(geometry, brush, float(width));
		}
		void D2DRenderingDevice::FillPolygon(const Math::Vector2 * points, int count, const Math::Color & color) noexcept
		{
			SafePointer<ID2D1PathGeometry> geometry;
			SafePointer<ID2D1SolidColorBrush> brush;
			if (D2DFactory->CreatePathGeometry(geometry.InnerRef()) != S_OK) return;
			if (count) {
				SafePointer<ID2D1GeometrySink> sink;
				if (geometry->Open(sink.InnerRef()) != S_OK) return;
				sink->BeginFigure(D2D1::Point2F(float(points[0].x), float(points[0].y)), D2D1_FIGURE_BEGIN_FILLED);
				for (int i = 1; i < count; i++) sink->AddLine(D2D1::Point2F(float(points[i].x), float(points[i].y)));
				sink->EndFigure(D2D1_FIGURE_END_CLOSED);
				sink->Close();
			}
			if (Target->CreateSolidColorBrush(D2D1::ColorF(float(color.x), float(color.y), float(color.z), float(color.w)), brush.InnerRef()) != S_OK) return;
			Target->FillGeometry(geometry, brush);
		}
		Drawing::ITextureRenderingDevice * D2DRenderingDevice::CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept { return CreateD2DCompatibleTextureRenderingDevice(width, height, color); }
		Drawing::ITextureRenderingDevice * D2DRenderingDevice::CreateD2DCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept
		{
			SafePointer<IWICBitmap> Bitmap;
			Array<UI::Color> Pixels(width);
			Pixels.SetLength(width * height);
			{
				Math::Color prem = color;
				prem.x *= prem.w;
				prem.y *= prem.w;
				prem.z *= prem.w;
				swap(prem.x, prem.z);
				UI::Color pixel = prem;
				for (int i = 0; i < Pixels.Length(); i++) Pixels[i] = pixel;
			}
			if (WICFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppPBGRA, 4 * width, 4 * width * height, reinterpret_cast<LPBYTE>(Pixels.GetBuffer()), Bitmap.InnerRef()) != S_OK) return 0;
			SafePointer<ID2D1RenderTarget> RenderTarget;
			D2D1_RENDER_TARGET_PROPERTIES props;
			props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
			props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
			props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
			props.dpiX = 0.0f;
			props.dpiY = 0.0f;
			props.usage = D2D1_RENDER_TARGET_USAGE_NONE;
			props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
			if (D2DFactory->CreateWicBitmapRenderTarget(Bitmap, props, RenderTarget.InnerRef()) != S_OK) return 0;
			SafePointer<D2DRenderingDevice> Device = new D2DRenderingDevice(RenderTarget);
			Device->BitmapTarget.SetReference(Bitmap);
			Device->BitmapTarget->AddRef();
			Device->BitmapTargetResX = width;
			Device->BitmapTargetResY = height;
			Device->Retain();
			return Device;
		}
		void D2DRenderingDevice::BeginDraw(void) noexcept
		{
			if (BitmapTarget && BitmapTargetState == 0) {
				Target->BeginDraw();
				BitmapTargetState = 1;
			}
		}
		void D2DRenderingDevice::EndDraw(void) noexcept
		{
			if (BitmapTarget && BitmapTargetState == 1) {
				Target->EndDraw();
				BitmapTargetState = 0;
			}
		}
		UI::ITexture * D2DRenderingDevice::GetRenderTargetAsTexture(void) noexcept
		{
			if (!BitmapTarget || BitmapTargetState) return 0;
			SafePointer<WICTexture> result = new (std::nothrow) WICTexture;
			if (!result) return 0;
			result->bitmap = BitmapTarget;
			result->w = BitmapTargetResX;
			result->h = BitmapTargetResY;
			result->Retain();
			BitmapTarget->AddRef();
			return result;
		}
		Engine::Codec::Frame * D2DRenderingDevice::GetRenderTargetAsFrame(void) noexcept
		{
			if (!BitmapTarget || BitmapTargetState) return 0;
			SafePointer<Engine::Codec::Frame> result = new Engine::Codec::Frame(BitmapTargetResX, BitmapTargetResY, BitmapTargetResX * 4, PixelFormat::B8G8R8A8, AlphaMode::Premultiplied, ScanOrigin::TopDown);
			SafePointer<IWICBitmapLock> lock;
			if (BitmapTarget->Lock(0, WICBitmapLockRead, lock.InnerRef()) == S_OK) {
				UINT size;
				WICInProcPointer ptr;
				lock->GetDataPointer(&size, &ptr);
				MemoryCopy(result->GetData(), ptr, min(size, uint(4 * BitmapTargetResX * BitmapTargetResY)));
			}
			result->Retain();
			return result;
		}

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
			int tab_width = Font->GetWidth() * 4;
			for (int i = 0; i < GlyphString.Length(); i++) {
				if (CharString[i] == L'\t') {
					int64 align_pos = ((int64(rl) + tab_width) / tab_width) * tab_width;
					GlyphAdvances[i] = float(align_pos) - rl;
					rl = float(align_pos);
				} else {
					rl += GlyphAdvances[i];
				}
			}
			run_length = int(rl);
		}
		void TextRenderingInfo::FillGlyphs(void)
		{
			uint32 Space = 0x20;
			Font->FontFace->GetGlyphIndicesW(&Space, 1, &NormalSpaceGlyph);
			if (Font->AlternativeFace) Font->AlternativeFace->GetGlyphIndicesW(&Space, 1, &AlternativeSpaceGlyph);
			GlyphString.SetLength(CharString.Length());
			if (Font->FontFace->GetGlyphIndicesW(CharString, CharString.Length(), GlyphString) != S_OK) throw Exception();
			UseAlternative.SetLength(CharString.Length());
			for (int i = 0; i < UseAlternative.Length(); i++) {
				if (CharString[i] == L'\t') {
					GlyphString[i] = NormalSpaceGlyph;
					UseAlternative[i] = false;
				} else {
					if (!GlyphString[i]) {
						if (Font->AlternativeFace && Font->AlternativeFace->GetGlyphIndicesW(CharString.GetBuffer() + i, 1, GlyphString.GetBuffer() + i) == S_OK && GlyphString[i]) UseAlternative[i] = true;
						else UseAlternative[i] = false;
					} else UseAlternative[i] = false;
				}
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
		void TextRenderingInfo::GetExtent(int & width, int & height) noexcept { width = run_length; height = Font->ActualHeight; }
	}
}


