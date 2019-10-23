#include "CocoaInterop.h"

namespace Engine
{
    namespace Cocoa
    {
        void QuartzDataRelease(void *info, const void *data, size_t size);
        NSString * CocoaString(const string & str) __attribute((ns_returns_retained))
        {
            Array<uint16> utf16(0x100);
            utf16.SetLength(str.GetEncodedLength(Encoding::UTF16));
            str.Encode(utf16.GetBuffer(), Encoding::UTF16, false);
            NSString * New = [[NSString alloc] initWithBytes: utf16.GetBuffer() length: utf16.Length() * 2 encoding: NSUTF16LittleEndianStringEncoding];
            return New;
        }
        string EngineString(NSString * str)
        {
            Array<uint16> utf16(0x100);
            utf16.SetLength([str length]);
            [str getBytes: (utf16.GetBuffer()) maxLength: 2 * utf16.Length() usedLength: NULL encoding: NSUTF16LittleEndianStringEncoding
                options: 0 range: NSMakeRange(0, utf16.Length()) remainingRange: NULL];
            return string(utf16.GetBuffer(), utf16.Length(), Engine::Encoding::UTF16);
        }
        NSImage * CocoaImage(Codec::Frame * frame) __attribute((ns_returns_retained))
        {
            SafePointer<Codec::Frame> src;
            if (frame->GetLineDirection() == Codec::LineDirection::TopDown && frame->GetPixelFormat() == Codec::PixelFormat::R8G8B8A8) {
                src.SetRetain(frame);
            } else {
                src = frame->ConvertFormat(Codec::FrameFormat(Codec::PixelFormat::R8G8B8A8, frame->GetAlphaFormat(), Codec::LineDirection::TopDown));
            }
            int data_len = src->GetScanLineLength() * src->GetHeight();
            uint8 * data = reinterpret_cast<uint8 *>(malloc(data_len));
            if (!data) throw OutOfMemoryException();
            MemoryCopy(data, src->GetData(), data_len);
            CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
            CGDataProviderRef provider = CGDataProviderCreateWithData(data, data, data_len, QuartzDataRelease);
            CGImageRef img = CGImageCreate(src->GetWidth(), src->GetHeight(), 8, 32, src->GetScanLineLength(), rgb,
                (src->GetAlphaFormat() == Codec::AlphaFormat::Premultiplied) ? kCGImageAlphaPremultipliedLast : kCGImageAlphaLast,
                provider, 0, false, kCGRenderingIntentDefault);
            CGColorSpaceRelease(rgb);
            CGDataProviderRelease(provider);
            if (!img) throw Exception();
            NSImage * result = [[NSImage alloc] initWithCGImage: img size: NSZeroSize];
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
            // NSSize size = [image size];
            // int width = size.width;
            // int height = size.height;
            // SafePointer<Codec::Frame> result = new Engine::Codec::Frame(width, height, 4 * width, Codec::PixelFormat::R8G8B8A8,
            //     Codec::AlphaFormat::Premultiplied, Codec::LineDirection::TopDown);
            // ZeroMemory(result->GetData(), width * height * 4);
            // CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
			// CGContextRef context = CGBitmapContextCreate(result->GetData(), width, height, 8, result->GetScanLineLength(), rgb, kCGImageAlphaPremultipliedLast);

            // CGContextRelease(context);
			// CGColorSpaceRelease(rgb);
            // if (result) result->Retain();
            // return result;

							
							
			// 				CGRect rect = CGRectMake(0.0f, 0.0f, float(width), float(height));
			// 				CGContextSetBlendMode(context, kCGBlendModeCopy);
			// 				CGContextDrawImage(context, rect, frame);
							
			// 				result->Frames.Append(eframe);
			// 				CGImageRelease(frame);
        }
    }
}