#include "QuartzDevice.h"

#include "../ImageCodec/CodecBase.h"

#include <ImageIO/ImageIO.h>

namespace Engine
{
	namespace Cocoa
	{
        void QuartzDataRelease(void *info, const void *data, size_t size) { free(info); }
        CGRect QuartzMakeRect(const UI::Box & box, int w, int h, int scale)
        {
            double s = double(scale);
            return CGRectMake(
                double(box.Left) / s, double(h - box.Bottom) / s,
                double(box.Right - box.Left) / s, double(box.Bottom - box.Top) / s
            );
        }
        class QuartzTexture : public UI::ITexture
        {
        public:
            Array<CGImageRef> frames;
            Array<uint32> duration;
			uint32 total_duration;
			int w, h;

            QuartzTexture(void) : frames(0x10), total_duration(0), w(0), h(0) {}
            ~QuartzTexture(void) override { for (int i = 0; i < frames.Length(); i++) CGImageRelease(frames[i]); }
            void Replace(QuartzTexture * object)
            {
                frames = object->frames;
                duration = object->duration;
                total_duration = object->total_duration;
                w = object->w; h = object->h;
                object->frames.Clear();
            }

            virtual int GetWidth(void) const override { return w; }
			virtual int GetHeight(void) const override { return h; }
			virtual bool IsDynamic(void) const override { return false; }
			virtual void Reload(UI::IRenderingDevice * Device, Streaming::Stream * Source) override
            {
                Replace(static_cast<QuartzTexture *>(Device->LoadTexture(Source)));
            }
			virtual void Reload(UI::IRenderingDevice * Device, Codec::Image * Source) override
            {
                Replace(static_cast<QuartzTexture *>(Device->LoadTexture(Source)));
            }
			virtual void Reload(UI::IRenderingDevice * Device, Codec::Frame * Source) override
            {
                Replace(static_cast<QuartzTexture *>(Device->LoadTexture(Source)));
            }
        };

        class QuartzBarRenderingInfo : public UI::IBarRenderingInfo
        {
        public:
            CGGradientRef gradient;
            double r, g, b, a;
            double prop_w, prop_h;
            QuartzBarRenderingInfo(void) { r = g = b = a = prop_w = prop_h = 0.0; gradient = 0; }
            ~QuartzBarRenderingInfo(void) override { if (gradient) CGGradientRelease(gradient); }
        };
        class QuartzInversionRenderingInfo : public UI::IInversionEffectRenderingInfo
        {
        public:
            uint32 pixel;
            CGImageRef white;
            QuartzInversionRenderingInfo(void) { pixel = 0xFFFFFFFF; }
            ~QuartzInversionRenderingInfo(void) override { CGImageRelease(white); }
        };
        class QuartzTextureRenderingInfo : public UI::ITextureRenderingInfo
        {
        public:
            SafePointer<QuartzTexture> fragment;
            bool fill;
            QuartzTextureRenderingInfo(void) {}
            ~QuartzTextureRenderingInfo(void) override {}
        };
        class QuartzLineRenderingInfo : public UI::ILineRenderingInfo
        {
        public:
            double r, g, b, a;
            bool dotted;
            QuartzLineRenderingInfo(void) {}
            ~QuartzLineRenderingInfo(void) override {}
        };

        QuartzRenderingDevice::QuartzRenderingDevice(void) : BrushCache(0x20, Dictionary::ExcludePolicy::ExcludeLeastRefrenced), Clipping(0x10), _animation(0)
        {

        }
        QuartzRenderingDevice::~QuartzRenderingDevice(void) {}

        void * QuartzRenderingDevice::GetContext(void) const { return _context; }
        void QuartzRenderingDevice::SetContext(void * context, int width, int height, int scale)
        {
            _context = context; _width = width; _height = height; _scale = scale;
            CGContextSetInterpolationQuality(reinterpret_cast<CGContextRef>(_context), kCGInterpolationNone);
        }

        UI::IBarRenderingInfo * QuartzRenderingDevice::CreateBarRenderingInfo(const Array<UI::GradientPoint>& gradient, double angle)
        {
            if (!gradient.Length()) throw InvalidArgumentException();
            if (gradient.Length() == 1) return CreateBarRenderingInfo(gradient[0].Color);
            SafePointer<QuartzBarRenderingInfo> info = new QuartzBarRenderingInfo;
            CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
            Array<double> clr(4);
            Array<double> pos(4);
            clr.SetLength(gradient.Length() * 4);
            pos.SetLength(gradient.Length());
            for (int i = 0; i < gradient.Length(); i++) {
                clr[i * 4 + 0] = double(gradient[i].Color.r) / 255.0;
                clr[i * 4 + 1] = double(gradient[i].Color.g) / 255.0;
                clr[i * 4 + 2] = double(gradient[i].Color.b) / 255.0;
                clr[i * 4 + 3] = double(gradient[i].Color.a) / 255.0;
                pos[i] = gradient[i].Position;
            }
            info->gradient = CGGradientCreateWithColorComponents(rgb, clr.GetBuffer(), pos.GetBuffer(), gradient.Length());
            info->prop_w = cos(angle);
            info->prop_h = sin(angle);
            CGColorSpaceRelease(rgb);
            info->Retain();
            return info;
        }
        UI::IBarRenderingInfo * QuartzRenderingDevice::CreateBarRenderingInfo(UI::Color color)
        {
            auto CachedInfo = BrushCache.ElementByKey(color);
			if (CachedInfo) {
				CachedInfo->Retain();
				return CachedInfo;
			}
            SafePointer<QuartzBarRenderingInfo> info = new QuartzBarRenderingInfo;
            info->r = double(color.r) / 255.0;
            info->g = double(color.g) / 255.0;
            info->b = double(color.b) / 255.0;
            info->a = double(color.a) / 255.0;
            BrushCache.Append(color, info);
            info->Retain();
            return info;
        }
        UI::IBlurEffectRenderingInfo * QuartzRenderingDevice::CreateBlurEffectRenderingInfo(double power)
        {
            return 0;
        }
        UI::IInversionEffectRenderingInfo * QuartzRenderingDevice::CreateInversionEffectRenderingInfo(void)
        {
            if (InversionCache) return InversionCache;
            SafePointer<QuartzInversionRenderingInfo> info = new QuartzInversionRenderingInfo;
            CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
            CGDataProviderRef data = CGDataProviderCreateWithData(0, &info->pixel, 4, 0);
            info->white = CGImageCreate(1, 1, 8, 32, 4, rgb, kCGImageAlphaLast, data, 0, false, kCGRenderingIntentDefault);
            CGDataProviderRelease(data);
            CGColorSpaceRelease(rgb);
            InversionCache.SetRetain(info);
            info->Retain();
            return info;
        }
        UI::ITextureRenderingInfo * QuartzRenderingDevice::CreateTextureRenderingInfo(UI::ITexture * texture, const UI::Box & take_area, bool fill_pattern)
        {
            auto tex = static_cast<QuartzTexture *>(texture);
            SafePointer<QuartzTextureRenderingInfo> info = new QuartzTextureRenderingInfo;
            info->fill = fill_pattern;
            if (take_area.Left == 0 && take_area.Top == 0 && take_area.Right == texture->GetWidth() && take_area.Bottom == texture->GetHeight()) {
                info->fragment.SetRetain(tex);
            } else {
                info->fragment = new QuartzTexture;
                CGRect rect = CGRectMake(double(take_area.Left), double(take_area.Top),
                    double(take_area.Right - take_area.Left), double(take_area.Bottom - take_area.Top));
                for (int i = 0; i < tex->frames.Length(); i++) {
                    info->fragment->frames << CGImageCreateWithImageInRect(tex->frames[i], rect);
                }
                info->fragment->duration = tex->duration;
                info->fragment->total_duration = tex->total_duration;
                info->fragment->w = take_area.Right - take_area.Left;
                info->fragment->h = take_area.Bottom - take_area.Top;
            }
            info->Retain();
            return info;
        }
        UI::ITextRenderingInfo * QuartzRenderingDevice::CreateTextRenderingInfo(UI::IFont * font, const string & text, int horizontal_align, int vertical_align, const UI::Color & color)
        {
            return 0;
        }
        UI::ITextRenderingInfo * QuartzRenderingDevice::CreateTextRenderingInfo(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color)
        {
            return 0;
        }
        UI::ILineRenderingInfo * QuartzRenderingDevice::CreateLineRenderingInfo(const UI::Color & color, bool dotted)
        {
            SafePointer<QuartzLineRenderingInfo> info = new QuartzLineRenderingInfo;
            info->r = double(color.r) / 255.0;
            info->g = double(color.g) / 255.0;
            info->b = double(color.b) / 255.0;
            info->a = double(color.a) / 255.0;
            info->dotted = dotted;
            info->Retain();
            return info;
        }

        UI::ITexture * QuartzRenderingDevice::LoadTexture(Streaming::Stream * Source)
        {
            SafePointer<Codec::Image> image = Codec::DecodeImage(Source);
            return LoadTexture(image);
        }
        UI::ITexture * QuartzRenderingDevice::LoadTexture(Codec::Image * Source)
        {
            if (!Source->Frames.Length()) throw InvalidArgumentException();
            for (int i = 1; i < Source->Frames.Length(); i++)
                if (Source->Frames[i].GetWidth() != Source->Frames[0].GetWidth() ||
                    Source->Frames[i].GetHeight() != Source->Frames[0].GetHeight()) throw InvalidArgumentException();
            SafePointer<QuartzTexture> result = new QuartzTexture;
            result->w = Source->Frames[0].GetWidth();
            result->h = Source->Frames[0].GetHeight();
            for (int i = 0; i < Source->Frames.Length(); i++) {
                SafePointer<Codec::Frame> source;
                if (Source->Frames[i].GetLineDirection() == Codec::LineDirection::TopDown &&
                    Source->Frames[i].GetPixelFormat() == Codec::PixelFormat::R8G8B8A8) {
                    source.SetRetain(Source->Frames.ElementAt(i));
                } else {
                    source = Source->Frames[i].ConvertFormat(Codec::FrameFormat(
                        Codec::PixelFormat::R8G8B8A8, Source->Frames[i].GetAlphaFormat(), Codec::LineDirection::TopDown));
                }
                int data_len = 4 * source->GetScanLineLength() * source->GetHeight();
                uint8 * data = reinterpret_cast<uint8 *>(malloc(data_len));
                if (!data) throw OutOfMemoryException();
                MemoryCopy(data, source->GetData(), data_len);
                CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
                CGDataProviderRef provider = CGDataProviderCreateWithData(data, data, data_len, QuartzDataRelease);
                CGImageRef frame = CGImageCreate(result->w, result->h, 8, 32, source->GetScanLineLength(), rgb,
                    (source->GetAlphaFormat() == Codec::AlphaFormat::Premultiplied) ? kCGImageAlphaPremultipliedLast : kCGImageAlphaLast,
                    provider, 0, false, kCGRenderingIntentDefault);
                CGColorSpaceRelease(rgb);
                CGDataProviderRelease(provider);
                if (frame) result->frames << frame; else throw Exception();
                result->duration << Source->Frames[i].Duration;
            }
            result->total_duration = result->duration[0];
			for (int i = 1; i < result->duration.Length(); i++) {
				auto v = result->duration[i];
				result->duration[i] = result->total_duration;
				result->total_duration += v;
			}
            result->Retain();
            return result;
        }
        UI::ITexture * QuartzRenderingDevice::LoadTexture(Codec::Frame * Source)
        {
            Codec::Image fake;
            fake.Frames.Append(Source);
            return LoadTexture(&fake);
        }
        UI::IFont * QuartzRenderingDevice::LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout)
        {
            return 0;
        }

        void QuartzRenderingDevice::RenderBar(UI::IBarRenderingInfo * Info, const UI::Box & At)
        {
            auto info = static_cast<QuartzBarRenderingInfo *>(Info);
            if (info->gradient) {
                int w = At.Right - At.Left, h = At.Bottom - At.Top;
				if (!w || !h) return;
				CGPoint s, e;
				if (fabs(info->prop_w) > fabs(info->prop_h)) {
					double aspect = info->prop_h / info->prop_w;
					if (fabs(aspect) > double(h) / double(w)) {
						int dx = int(h / fabs(aspect) * sgn(info->prop_w));
						s.x = float(At.Left + (w - dx) / 2);
						s.y = float((info->prop_h > 0.0) ? At.Bottom : At.Top);
						e.x = float(At.Left + (w + dx) / 2);
						e.y = float((info->prop_h > 0.0) ? At.Top : At.Bottom);
					} else {
						int dy = int(w * fabs(aspect) * sgn(info->prop_h));
						s.x = float((info->prop_w > 0.0) ? At.Left : At.Right);
						s.y = float(At.Top + (h + dy) / 2);
						e.x = float((info->prop_w > 0.0) ? At.Right : At.Left);
						e.y = float(At.Top + (h - dy) / 2);
					}
				} else {
					double aspect = info->prop_w / info->prop_h;
					if (fabs(aspect) > double(w) / double(h)) {
						int dy = int(w / fabs(aspect) * sgn(info->prop_h));
						s.x = float((info->prop_w > 0.0) ? At.Left : At.Right);
						s.y = float(At.Top + (h + dy) / 2);
						e.x = float((info->prop_w > 0.0) ? At.Right : At.Left);
						e.y = float(At.Top + (h - dy) / 2);
					} else {
						int dx = int(h * fabs(aspect) * sgn(info->prop_w));
						s.x = float(At.Left + (w - dx) / 2);
						s.y = float((info->prop_h > 0.0) ? At.Bottom : At.Top);
						e.x = float(At.Left + (w + dx) / 2);
						e.y = float((info->prop_h > 0.0) ? At.Top : At.Bottom);
					}
				}
                double z = double(_scale);
                s.y = _height - s.y;
                e.y = _height - e.y;
                s.x /= z; s.y /= z; e.x /= z; e.y /= z;
                PushClip(At);
				CGContextDrawLinearGradient(reinterpret_cast<CGContextRef>(_context), info->gradient, s, e,
                    kCGGradientDrawsBeforeStartLocation | kCGGradientDrawsAfterEndLocation);
                PopClip();
            } else {
                CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context), info->r, info->g, info->b, info->a);
                CGContextFillRect(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(At, _width, _height, _scale));
            }
        }
        void QuartzRenderingDevice::RenderTexture(UI::ITextureRenderingInfo * Info, const UI::Box & At)
        {
            auto info = static_cast<QuartzTextureRenderingInfo *>(Info);
			int frame = 0;
			if (info->fragment->frames.Length() > 1) frame = max(BinarySearchLE(info->fragment->duration, _animation % info->fragment->total_duration), 0);
            if (info->fill) {
                double s = double(_scale);
                PushClip(At);
                CGContextDrawTiledImage(reinterpret_cast<CGContextRef>(_context), CGRectMake(0.0f, 0.0f, double(info->fragment->w) / s, double(info->fragment->h) / s), info->fragment->frames[frame]);
                PopClip();
            } else {
                CGContextDrawImage(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(At, _width, _height, _scale), info->fragment->frames[frame]);
            }
        }
        void QuartzRenderingDevice::RenderText(UI::ITextRenderingInfo * Info, const UI::Box & At, bool Clip)
        {

        }
        void QuartzRenderingDevice::RenderLine(UI::ILineRenderingInfo * Info, const UI::Box & At)
        {
            auto info = static_cast<QuartzLineRenderingInfo *>(Info);
            double s = double(_scale);
            CGPoint p[2];
            p[0].x = double(At.Left + 0.5) / s;
            p[0].y = double(_height - At.Top - 0.5) / s;
            p[1].x = double(At.Right + 0.5) / s;
            p[1].y = double(_height - At.Bottom - 0.5) / s;
            if (info->dotted) {
                double l[2] = { 1.0 / s, 1.0 / s };
                CGContextSetLineDash(reinterpret_cast<CGContextRef>(_context), 0.5 / s, l, 2);
            } else {
                CGContextSetLineDash(reinterpret_cast<CGContextRef>(_context), 0.0, 0, 0);
            }
            CGContextSetRGBStrokeColor(reinterpret_cast<CGContextRef>(_context), info->r, info->g, info->b, info->a);
            CGContextSetLineWidth(reinterpret_cast<CGContextRef>(_context), 1.0 / s);
            CGContextStrokeLineSegments(reinterpret_cast<CGContextRef>(_context), p, 2);
        }
        void QuartzRenderingDevice::ApplyBlur(UI::IBlurEffectRenderingInfo * Info, const UI::Box & At)
        {

        }
        void QuartzRenderingDevice::ApplyInversion(UI::IInversionEffectRenderingInfo * Info, const UI::Box & At, bool Blink)
        {
            auto info = static_cast<QuartzInversionRenderingInfo *>(Info);
            if (!Blink || (_animation % 1000 < 500)) {
                CGContextSetBlendMode(reinterpret_cast<CGContextRef>(_context), kCGBlendModeDifference);
                CGContextDrawImage(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(At, _width, _height, _scale), info->white);
                CGContextSetBlendMode(reinterpret_cast<CGContextRef>(_context), kCGBlendModeNormal);
            }
        }

        void QuartzRenderingDevice::PushClip(const UI::Box & Rect)
        {
            UI::Box clip = Clipping.Length() ? UI::Box::Intersect(Rect, Clipping.LastElement()) : Rect;
            Clipping << clip;
            CGContextSaveGState(reinterpret_cast<CGContextRef>(_context));
            CGContextClipToRect(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(Rect, _width, _height, _scale));
        }
        void QuartzRenderingDevice::PopClip(void)
        {
            Clipping.RemoveLast();
            CGContextRestoreGState(reinterpret_cast<CGContextRef>(_context));
        }
        void QuartzRenderingDevice::BeginLayer(const UI::Box & Rect, double Opacity)
        {

        }
        void QuartzRenderingDevice::EndLayer(void)
        {

        }

        void QuartzRenderingDevice::SetTimerValue(uint32 time) { _animation = time; }
        void QuartzRenderingDevice::ClearCache(void)
        {
            BrushCache.Clear();
            InversionCache.SetReference(0);
        }
    }
}