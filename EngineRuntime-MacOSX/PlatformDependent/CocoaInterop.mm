#include "CocoaInterop.h"
#include "CocoaInterop2.h"

namespace Engine
{
	namespace Cocoa
	{
		void FreeData(void *info, const void *data, size_t size) { free(info); }
		NSString * CocoaString(const string & str) __attribute((ns_returns_retained))
		{
			Array<uint16> utf16(0x100);
			utf16.SetLength(str.GetEncodedLength(Encoding::UTF16));
			str.Encode(utf16.GetBuffer(), Encoding::UTF16, false);
			NSString * New = [[NSString alloc] initWithBytes: utf16.GetBuffer() length: utf16.Length() * 2 encoding: NSUTF16LittleEndianStringEncoding];
			return New;
		}
		string EngineString(CFStringRef str) { return EngineString(reinterpret_cast<NSString *>(str)); }
		string EngineString(NSString * str)
		{
			Array<uint16> utf16(0x100);
			utf16.SetLength([str length]);
			[str getBytes: (utf16.GetBuffer()) maxLength: 2 * utf16.Length() usedLength: NULL encoding: NSUTF16LittleEndianStringEncoding
				options: 0 range: NSMakeRange(0, utf16.Length()) remainingRange: NULL];
			return string(utf16.GetBuffer(), utf16.Length(), Engine::Encoding::UTF16);
		}
		CGImageRef CocoaCoreImage(Codec::Frame * frame)
		{
			SafePointer<Codec::Frame> src;
			if (frame->GetScanOrigin() == Codec::ScanOrigin::TopDown && frame->GetPixelFormat() == Codec::PixelFormat::R8G8B8A8) {
				src.SetRetain(frame);
			} else {
				src = frame->ConvertFormat(Codec::PixelFormat::R8G8B8A8, frame->GetAlphaMode(), Codec::ScanOrigin::TopDown);
			}
			int data_len = src->GetScanLineLength() * src->GetHeight();
			uint8 * data = reinterpret_cast<uint8 *>(malloc(data_len));
			if (!data) throw OutOfMemoryException();
			MemoryCopy(data, src->GetData(), data_len);
			CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
			CGDataProviderRef provider = CGDataProviderCreateWithData(data, data, data_len, FreeData);
			CGImageRef img = CGImageCreate(src->GetWidth(), src->GetHeight(), 8, 32, src->GetScanLineLength(), rgb,
				(src->GetAlphaMode() == Codec::AlphaMode::Premultiplied) ? kCGImageAlphaPremultipliedLast : kCGImageAlphaLast,
				provider, 0, false, kCGRenderingIntentDefault);
			CGColorSpaceRelease(rgb);
			CGDataProviderRelease(provider);
			return img;
		}
		NSImage * CocoaImage(Codec::Frame * frame, double scale_factor) __attribute((ns_returns_retained))
		{
			auto img = CocoaCoreImage(frame);
			if (!img) throw Exception();
			CGSize image_size = CGSizeMake(double(frame->GetWidth()) / scale_factor, double(frame->GetHeight()) / scale_factor);
			NSImage * result = [[NSImage alloc] initWithCGImage: img size: image_size];
			CGImageRelease(img);
			return result;
		}
		Codec::Frame * EngineImage(NSImage * image)
		{
			NSData * data = [image TIFFRepresentation];
			uint64 size = [data length];
			const void * bytes = [data bytes];
			Streaming::MemoryStream stream(bytes, size);
			return Codec::DecodeFrame(&stream);
		}
	}
}