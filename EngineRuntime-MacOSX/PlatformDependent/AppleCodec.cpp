#include "AppleCodec.h"

#include <ImageIO/ImageIO.h>
#include <CoreServices/CoreServices.h>

namespace Engine
{
	namespace Cocoa
	{
        class AppleCodec : public Codec::Codec
        {
        public:
            AppleCodec(void) {}
			~AppleCodec(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Engine::Codec::Frame * frame, const string & format) override
            {
                SafePointer<Engine::Codec::Image> Fake = new Engine::Codec::Image;
				Fake->Frames.Append(frame);
				EncodeImage(stream, Fake, format);
            }
			virtual void EncodeImage(Streaming::Stream * stream, Engine::Codec::Image * image, const string & format) override
			{
				if (!image->Frames.Length()) throw InvalidArgumentException();
				CFStringRef encoder = 0;
				int max_frame = 1;
				if (format == L"BMP") {
					encoder = kUTTypeBMP;
				} else if (format == L"PNG") {
					encoder = kUTTypePNG;
				} else if (format == L"JPG") {
					encoder = kUTTypeJPEG;
				} else if (format == L"GIF") {
					encoder = kUTTypeGIF;
					max_frame = image->Frames.Length();
					for (int i = 1; i < max_frame; i++) {
						if (image->Frames[i].GetWidth() != image->Frames[0].GetWidth() ||
							image->Frames[i].GetHeight() != image->Frames[0].GetHeight()) throw InvalidArgumentException();
					}
				} else if (format == L"TIF") {
					encoder = kUTTypeTIFF;
					max_frame = image->Frames.Length();
				} else throw InvalidArgumentException();
				CFMutableDataRef data = CFDataCreateMutable(0, 0);
				CGImageDestinationRef dest = CGImageDestinationCreateWithData(data, encoder, max_frame, 0);
				if (!dest) {
					CFRelease(data);
					throw Exception();
				}
				ObjectArray<Engine::Codec::Frame> conv(0x10);
				for (int i = 0; i < max_frame; i++) {
					SafePointer<Engine::Codec::Frame> frame = image->Frames[i].ConvertFormat(Engine::Codec::FrameFormat(
						Engine::Codec::PixelFormat::R8G8B8A8, Engine::Codec::AlphaFormat::Normal, Engine::Codec::LineDirection::TopDown));
					conv.Append(frame);
				}
				for (int i = 0; i < max_frame; i++) {
					CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
					CGDataProviderRef provider = CGDataProviderCreateWithData(0, conv[i].GetData(), conv[i].GetScanLineLength() * conv[i].GetHeight(), 0);
					CGImageRef frame = CGImageCreate(conv[i].GetWidth(), conv[i].GetHeight(), 8, 32, conv[i].GetScanLineLength(),
						rgb, kCGImageAlphaLast, provider, 0, false, kCGRenderingIntentDefault);
					CGColorSpaceRelease(rgb);
					CGDataProviderRelease(provider);
					CFDictionaryRef props = 0;
					float compression = 1.0f;
					if (format == L"GIF") {
						double duration = double(conv[i].Duration) / 1000.0;
						CFStringRef gif_keys[1];
						CFTypeRef gif_vals[1];
						gif_keys[0] = kCGImagePropertyGIFDelayTime;
						gif_vals[0] = CFNumberCreate(0, kCFNumberFloat64Type, &duration);
						CFDictionaryRef gif_props = CFDictionaryCreate(0, reinterpret_cast<const void **>(gif_keys), reinterpret_cast<const void **>(gif_vals),
							1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
						CFRelease(gif_vals[0]);
						CFStringRef keys[2];
						CFTypeRef vals[2];
						keys[0] = kCGImageDestinationLossyCompressionQuality;
						vals[0] = CFNumberCreate(0, kCFNumberFloatType, &compression);
						keys[1] = kCGImagePropertyGIFDictionary;
						vals[1] = gif_props;
						props = CFDictionaryCreate(0, reinterpret_cast<const void **>(keys), reinterpret_cast<const void **>(vals),
							2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
						CFRelease(vals[0]);
						CFRelease(vals[1]);
					} else {
						CFStringRef keys[1];
						CFTypeRef vals[1];
						keys[0] = kCGImageDestinationLossyCompressionQuality;
						vals[0] = CFNumberCreate(NULL, kCFNumberFloatType, &compression);
						props = CFDictionaryCreate(0, reinterpret_cast<const void **>(keys), reinterpret_cast<const void **>(vals),
							1, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
						CFRelease(vals[0]);
					}
					CGImageDestinationAddImage(dest, frame, props);
					CFRelease(props);
					CGImageRelease(frame);
				}
				CGImageDestinationFinalize(dest);
				CFRelease(dest);
				stream->Write(CFDataGetBytePtr(data), CFDataGetLength(data));
				CFRelease(data);
			}
			virtual Engine::Codec::Frame * DecodeFrame(Streaming::Stream * stream) override
            {
                SafePointer<Engine::Codec::Image> image = DecodeImage(stream);
				SafePointer<Engine::Codec::Frame> frame = image->Frames.ElementAt(0);
				frame->Retain();
				frame->Retain();
				return frame;
            }
			virtual Engine::Codec::Image * DecodeImage(Streaming::Stream * stream) override
			{
                SafePointer<Engine::Codec::Image> result;
                int length = int(stream->Length() - stream->Seek(0, Streaming::Current));
                Array<uint8> bytes(length);
                bytes.SetLength(length);
                stream->Read(bytes.GetBuffer(), length);
                CFDataRef data = CFDataCreateWithBytesNoCopy(0, bytes.GetBuffer(), length, kCFAllocatorNull);
                CGImageSourceRef source = CGImageSourceCreateWithData(data, 0);
                if (source) {
                    result = new Engine::Codec::Image;
                    int frame_count = CGImageSourceGetCount(source);
                    for (int i = 0; i < frame_count; i++) {
                        CGImageRef frame = CGImageSourceCreateImageAtIndex(source, i, 0);
						if (frame) {
							int width = CGImageGetWidth(frame);
							int height = CGImageGetHeight(frame);
							SafePointer<Engine::Codec::Frame> eframe = new Engine::Codec::Frame(width, height, 4 * width, Engine::Codec::PixelFormat::R8G8B8A8,
								Engine::Codec::AlphaFormat::Premultiplied, Engine::Codec::LineDirection::TopDown);
							ZeroMemory(eframe->GetData(), width * height * 4);
							CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
							CGContextRef context = CGBitmapContextCreate(eframe->GetData(), width, height, 8, eframe->GetScanLineLength(), rgb, kCGImageAlphaPremultipliedLast);
							CGRect rect = CGRectMake(0.0f, 0.0f, float(width), float(height));
							CGContextSetBlendMode(context, kCGBlendModeCopy);
							CGContextDrawImage(context, rect, frame);
							CFDictionaryRef props = CGImageSourceCopyPropertiesAtIndex(source, i, 0);
							CFDictionaryRef gif_props = reinterpret_cast<CFDictionaryRef>(CFDictionaryGetValue(props, kCGImagePropertyGIFDictionary));
							if (gif_props) {
								CFNumberRef val_ref = reinterpret_cast<CFNumberRef>(CFDictionaryGetValue(gif_props, kCGImagePropertyGIFUnclampedDelayTime));
								if (val_ref) {
									double val;
									CFNumberGetValue(val_ref, kCFNumberFloat64Type, &val);
									eframe->Duration = int32(val * 1000.0);
								}
							}
							CFRelease(props);
							CGContextRelease(context);
							CGColorSpaceRelease(rgb);
							result->Frames.Append(eframe);
							CGImageRelease(frame);
						}
                    }
                    CFRelease(source);
                }
                CFRelease(data);
                if (result) result->Retain();
                return result;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				uint64 size = stream->Length() - stream->Seek(0, Streaming::Current);
				if (size < 8) return L"";
				uint64 begin = stream->Seek(0, Streaming::Current);
				uint64 sign;
				try {
					stream->Read(&sign, sizeof(sign));
					stream->Seek(begin, Streaming::Begin);
				} catch (...) { return L""; }
				if ((sign & 0xFFFF) == 0x4D42) return L"BMP";
				else if (sign == 0x0A1A0A0D474E5089) return L"PNG";
				else if ((sign & 0xFFFFFF) == 0xFFD8FF) return L"JPG";
				else if ((sign & 0xFFFFFFFFFFFF) == 0x613938464947) return L"GIF";
				else if ((sign & 0xFFFFFFFF) == 0x2A004D4D) return L"TIF";
				else if ((sign & 0xFFFFFFFF) == 0x002A4949) return L"TIF";
				else if ((sign & 0xFFFFFFFF) == 0x20534444) return L"DDS";
				else if (sign == 0x7079746618000000) return L"HEIF";
				else return L"";
			}
			virtual bool CanEncode(const string & format) override { return (format == L"BMP" || format == L"PNG" || format == L"JPG" || format == L"GIF" || format == L"TIF"); }
			virtual bool CanDecode(const string & format) override { return (format == L"BMP" || format == L"PNG" || format == L"JPG" || format == L"GIF" || format == L"TIF" || format == L"HEIF"); }
        };

        Codec::Codec * _AppleCodec = 0;
        Codec::Codec * CreateAppleCodec(void) { if (!_AppleCodec) { _AppleCodec = new AppleCodec(); _AppleCodec->Release(); } return _AppleCodec; }
    }
}