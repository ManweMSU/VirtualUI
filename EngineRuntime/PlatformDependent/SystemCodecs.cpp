#include "SystemCodecs.h"
#include "ComInterop.h"

using namespace Engine::Streaming;
using namespace Engine::Codec;

namespace Engine
{
	namespace WIC
	{
		IWICImagingFactory * WICFactory = 0;
		ICodec * _wic_codec = 0;

		class WICodec : public ICodec
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
			virtual string GetCodecName(void) override { return L"Windows Imaging Component"; }
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

		ICodec * CreateWICodec(void)
		{
			if (!WICFactory) {
				if (CoCreateInstance(CLSID_WICImagingFactory1, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&WICFactory)) != S_OK) throw Exception();
			}
			if (!_wic_codec) { _wic_codec = new WICodec; _wic_codec->Release(); } return _wic_codec;
		}
	}
}