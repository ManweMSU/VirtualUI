#include "QuartzDevice.h"

#include "../ImageCodec/CodecBase.h"

#include <ImageIO/ImageIO.h>
#include <CoreText/CoreText.h>

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

            virtual int GetWidth(void) const noexcept override { return w; }
			virtual int GetHeight(void) const noexcept override { return h; }
			virtual bool IsDynamic(void) const noexcept override { return false; }
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
        class CoreTextFont : public UI::IFont
        {
        public:
            CTFontRef font;
            CTFontRef alt_font;
            bool underline;
            bool strikeout;
            double height;
            double zoomed_height;
            double width;
            double scale;

            CoreTextFont(const string & FaceName, double Height, double Scale, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout)
            {
                CFStringRef font_name = CFStringCreateWithBytes(kCFAllocatorDefault, reinterpret_cast<const uint8 *>(static_cast<const widechar *>(FaceName)),
                    FaceName.Length() * 4, kCFStringEncodingUTF32LE, false);
                CFStringRef alt_font_name = CFStringCreateWithCString(kCFAllocatorDefault, "Apple Color Emoji", kCFStringEncodingASCII);
                if (!font_name) throw Exception();
                zoomed_height = Height * 0.75;
                CTFontRef base = CTFontCreateWithName(font_name, zoomed_height, 0);
                alt_font = CTFontCreateWithName(alt_font_name, zoomed_height, 0);
                CFRelease(font_name);
                CFRelease(alt_font_name);
                uint32 flags = 0;
                if (Weight >= 500) flags |= kCTFontBoldTrait;
                if (IsItalic) flags |= kCTFontItalicTrait;
                font = CTFontCreateCopyWithSymbolicTraits(base, 0.0, 0, flags, kCTFontItalicTrait | kCTFontBoldTrait);
                if (font) CFRelease(base); else font = base;
                underline = IsUnderline;
                strikeout = IsStrikeout;
                height = Height;
                scale = Scale;
                UniChar space = 32;
                CGGlyph glyph;
                CGSize adv;
                CTFontGetGlyphsForCharacters(font, &space, &glyph, 1);
                CTFontGetAdvancesForGlyphs(font, kCTFontOrientationHorizontal, &glyph, &adv, 1);
                width = adv.width;
            }
            ~CoreTextFont(void) override
            {
                CFRelease(font);
                CFRelease(alt_font);
            }

            virtual int GetWidth(void) const noexcept override { return width; }
			virtual int GetHeight(void) const noexcept override { return int(height * scale); }
			virtual void Reload(UI::IRenderingDevice * Device) override {}
        };
        
        class CoreTextRenderingInfo : public UI::ITextRenderingInfo
        {
        public:
            struct Color { double r, g, b, a; };
            struct TextRange
			{
				int LeftEdge;
				int RightEdge;
				Color Brush;
                CGFontRef Font;
			};
            SafePointer<CoreTextFont> font;
			Array<CGGlyph> GlyphString;
			Array<CGFloat> GlyphAdvances;
            Array<CGPoint> GlyphPositions;
            Array<CGFontRef> Fonts;
            CGFontRef base_font;
            CGFontRef alt_font;
			Array<TextRange> Ranges;
            Color TextColor;
            Color BackColor;
			int BaselineOffset;
			int halign, valign;
			int run_length;
			double UnderlineOffset;
			double UnderlineHalfWidth;
			double StrikeoutOffset;
			double StrikeoutHalfWidth;
			int hls, hle;
            Array<Color> ExtraBrushes;

            CoreTextRenderingInfo(void) : GlyphString(0x40), GlyphAdvances(0x40), GlyphPositions(0x40), ExtraBrushes(0x40), Fonts(0x40) {}
            ~CoreTextRenderingInfo(void) override
            {
                if (base_font) CGFontRelease(base_font);
                if (alt_font) CGFontRelease(alt_font);
            }

            virtual void GetExtent(int & width, int & height) noexcept override { width = run_length; height = int(font->height); }
			virtual void SetHighlightColor(const UI::Color & color) noexcept override
			{
                BackColor.r = double(color.r) / 255.0;
                BackColor.g = double(color.g) / 255.0;
                BackColor.b = double(color.b) / 255.0;
                BackColor.a = double(color.a) / 255.0;
			}
			virtual void HighlightText(int Start, int End) noexcept override { hls = Start; hle = End; }
			virtual int TestPosition(int point) noexcept override
			{
				if (point < 0) return 0;
				if (point > run_length) return GlyphString.Length();
				double p = double(point);
				double s = 0.0;
				for (int i = 0; i < GlyphAdvances.Length(); i++) {
					if (p <= s + GlyphAdvances[i]) {
						if (p < s + GlyphAdvances[i] / 2.0) return i;
						else return i + 1;
						break;
					}
					s += GlyphAdvances[i];
				}
				return GlyphString.Length();
			}
			virtual int EndOfChar(int Index) noexcept override
			{
				if (Index < 0) return 0;
				double summ = 0.0;
				for (int i = 0; i <= Index; i++) summ += GlyphAdvances[i];
				return int(summ);
			}
			virtual void SetCharPalette(const Array<UI::Color> & colors) override
			{
                ExtraBrushes.Clear();
                for (int i = 0; i < colors.Length(); i++) {
                    ExtraBrushes << Color { double(colors[i].r) / 255.0, double(colors[i].g) / 255.0, double(colors[i].b) / 255.0, double(colors[i].a) / 255.0 };
                }
			}
            void ResetRanges(const Array<uint8> * indicies)
            {
                Ranges.Clear();
				int cp = 0;
				while (cp < Fonts.Length()) {
					int ep = cp;
					while (ep < Fonts.Length() && Fonts[ep] == Fonts[cp] && (!indicies || indicies->ElementAt(ep) == indicies->ElementAt(cp))) ep++;
					int index = indicies ? indicies->ElementAt(cp) : 0;
                    CGFontRef font_ref = Fonts[cp];
					Ranges << TextRange{ cp, ep, (index == 0) ? TextColor : ExtraBrushes[index - 1], font_ref };
					cp = ep;
				}
            }
			virtual void SetCharColors(const Array<uint8> & indicies) override { ResetRanges(&indicies); }
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

        QuartzRenderingDevice::QuartzRenderingDevice(void) : BrushCache(0x20, Dictionary::ExcludePolicy::ExcludeLeastRefrenced), Clipping(0x10), _animation(0) {}
        QuartzRenderingDevice::~QuartzRenderingDevice(void) { if (BitmapTarget) CGContextRelease(reinterpret_cast<CGContextRef>(_context)); }

        void * QuartzRenderingDevice::GetContext(void) const noexcept { return _context; }
        void QuartzRenderingDevice::SetContext(void * context, int width, int height, int scale) noexcept
        {
            _context = context; _width = width; _height = height; _scale = scale;
            CGContextSetInterpolationQuality(reinterpret_cast<CGContextRef>(_context), kCGInterpolationNone);
        }

        UI::IBarRenderingInfo * QuartzRenderingDevice::CreateBarRenderingInfo(const Array<UI::GradientPoint>& gradient, double angle) noexcept
        {
            if (!gradient.Length()) return 0;
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
        UI::IBarRenderingInfo * QuartzRenderingDevice::CreateBarRenderingInfo(UI::Color color) noexcept
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
        UI::IBlurEffectRenderingInfo * QuartzRenderingDevice::CreateBlurEffectRenderingInfo(double power) noexcept { return 0; }
        UI::IInversionEffectRenderingInfo * QuartzRenderingDevice::CreateInversionEffectRenderingInfo(void) noexcept
        {
            if (InversionCache) {
                InversionCache->Retain();
                return InversionCache;
            }
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
        UI::ITextureRenderingInfo * QuartzRenderingDevice::CreateTextureRenderingInfo(UI::ITexture * texture, const UI::Box & take_area, bool fill_pattern) noexcept
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
        UI::ITextRenderingInfo * QuartzRenderingDevice::CreateTextRenderingInfo(UI::IFont * font, const string & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept
        {
            SafePointer<CoreTextRenderingInfo> info = new CoreTextRenderingInfo;
            if (!text.Length()) {
                info->base_font = 0;
                info->alt_font = 0;
                info->run_length = 0;
                info->font.SetRetain(reinterpret_cast<CoreTextFont *>(font));
                info->Retain();
                return info;
            }
            Array<uint16> utf16(0x100);
            Array<bool> alt(0x100);
            Array<CGGlyph> alt_glyph(0x100);
            utf16.SetLength(text.GetEncodedLength(Encoding::UTF16));
            alt.SetLength(utf16.Length());
            alt_glyph.SetLength(utf16.Length());
            ZeroMemory(alt.GetBuffer(), utf16.Length());
            text.Encode(utf16.GetBuffer(), Encoding::UTF16, false);
            info->font.SetRetain(reinterpret_cast<CoreTextFont *>(font));
            info->GlyphString.SetLength(utf16.Length());
            info->base_font = CTFontCopyGraphicsFont(info->font->font, 0);
            info->alt_font = CTFontCopyGraphicsFont(info->font->alt_font, 0);
            CTFontGetGlyphsForCharacters(info->font->font, utf16.GetBuffer(), info->GlyphString.GetBuffer(), utf16.Length());
            CTFontGetGlyphsForCharacters(info->font->alt_font, utf16.GetBuffer(), alt_glyph.GetBuffer(), utf16.Length());
            CGGlyph space, question; UniChar spc = 32, que = '?';
            CTFontGetGlyphsForCharacters(info->font->font, &spc, &space, 1);
            CTFontGetGlyphsForCharacters(info->font->font, &que, &question, 1);
            for (int i = utf16.Length() - 1; i >= 0; i--) {
                if (utf16[i] == L'\t') {
                    info->GlyphString[i] = 0;
                    alt_glyph[i] = 0;
                } else {
                    if (!info->GlyphString[i]) {
                        if (alt_glyph[i]) {
                            info->GlyphString[i] = alt_glyph[i];
                            alt[i] = true;
                        } else {
                            info->GlyphString[i] = question;
                        }
                    }
                }
            }
            info->GlyphAdvances.SetLength(info->GlyphString.Length());
            info->GlyphPositions.SetLength(info->GlyphString.Length());
            Array<CGSize> adv(0x10);
            adv.SetLength(info->GlyphString.Length());
            info->Fonts.SetLength(info->GlyphString.Length());
            double v = 0.0;
            for (int i = 0; i < adv.Length(); i++) {
                if (alt[i]) {
                    CTFontGetAdvancesForGlyphs(info->font->alt_font, kCTFontOrientationHorizontal, info->GlyphString.GetBuffer() + i, adv.GetBuffer() + i, 1);
                    v += adv[i].width;
                    info->Fonts[i] = info->alt_font;
                } else {
                    if (info->GlyphString[i]) {
                        CTFontGetAdvancesForGlyphs(info->font->font, kCTFontOrientationHorizontal, info->GlyphString.GetBuffer() + i, adv.GetBuffer() + i, 1);
                        v += adv[i].width;
                    } else {
                        info->GlyphString[i] = space;
                        int w = int(int64(v) + 4 * info->font->width) / int64(4 * info->font->width);
                        double lv = v;
                        v = double(4 * info->font->width * w);
                        adv[i].width = v - lv;
                    }
                    info->Fonts[i] = info->base_font;
                }
            }
            for (int i = 0; i < adv.Length(); i++) info->GlyphAdvances[i] = adv[i].width;
            if (adv.Length()) {
                info->GlyphPositions[0].x = 0.0;
                info->GlyphPositions[0].y = 0.0;
                for (int i = 1; i < adv.Length(); i++) {
                    info->GlyphPositions[i].x = info->GlyphPositions[i - 1].x + adv[i - 1].width;
                    info->GlyphPositions[i].y = 0.0;
                }
            }
            info->halign = horizontal_align;
            info->valign = vertical_align;
            info->run_length = int(info->GlyphPositions.LastElement().x + info->GlyphAdvances.LastElement());
            info->BaselineOffset = int(CTFontGetDescent(reinterpret_cast<CoreTextFont *>(font)->font));
            info->UnderlineOffset = CTFontGetUnderlinePosition(reinterpret_cast<CoreTextFont *>(font)->font);
            info->UnderlineHalfWidth = CTFontGetUnderlineThickness(reinterpret_cast<CoreTextFont *>(font)->font) / 2.0;
            info->StrikeoutOffset = (CTFontGetAscent(reinterpret_cast<CoreTextFont *>(font)->font) - CTFontGetDescent(reinterpret_cast<CoreTextFont *>(font)->font)) / 2.0;
            info->StrikeoutHalfWidth = info->UnderlineHalfWidth;
            info->TextColor.r = double(color.r) / 255.0;
            info->TextColor.g = double(color.g) / 255.0;
            info->TextColor.b = double(color.b) / 255.0;
            info->TextColor.a = double(color.a) / 255.0;
            info->hls = info->hle = -1;
            info->ResetRanges(0);
            info->Retain();
            return info;
        }
        UI::ITextRenderingInfo * QuartzRenderingDevice::CreateTextRenderingInfo(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept
        {
            SafePointer<CoreTextRenderingInfo> info = new CoreTextRenderingInfo;
            if (!text.Length()) {
                info->base_font = 0;
                info->alt_font = 0;
                info->run_length = 0;
                info->font.SetRetain(reinterpret_cast<CoreTextFont *>(font));
                info->Retain();
                return info;
            }
            Array<uint16> utf16(0x100);
            Array<bool> alt(0x100);
            Array<CGGlyph> alt_glyph(0x100);
            utf16.SetLength(MeasureSequenceLength(text.GetBuffer(), text.Length(), Encoding::UTF32, Encoding::UTF16));
            alt.SetLength(utf16.Length());
            alt_glyph.SetLength(utf16.Length());
            ZeroMemory(alt.GetBuffer(), utf16.Length());
            ConvertEncoding(utf16.GetBuffer(), text.GetBuffer(), text.Length(), Encoding::UTF32, Encoding::UTF16);
            info->font.SetRetain(reinterpret_cast<CoreTextFont *>(font));
            info->GlyphString.SetLength(utf16.Length());
            info->base_font = CTFontCopyGraphicsFont(info->font->font, 0);
            info->alt_font = CTFontCopyGraphicsFont(info->font->alt_font, 0);
            CTFontGetGlyphsForCharacters(info->font->font, utf16.GetBuffer(), info->GlyphString.GetBuffer(), utf16.Length());
            CTFontGetGlyphsForCharacters(info->font->alt_font, utf16.GetBuffer(), alt_glyph.GetBuffer(), utf16.Length());
            CGGlyph space, question; UniChar spc = 32, que = '?';
            CTFontGetGlyphsForCharacters(info->font->font, &spc, &space, 1);
            CTFontGetGlyphsForCharacters(info->font->font, &que, &question, 1);
            for (int i = utf16.Length() - 1; i >= 0; i--) {
                if (utf16[i] == L'\t') {
                    info->GlyphString[i] = 0;
                    alt_glyph[i] = 0;
                } else {
                    if (!info->GlyphString[i]) {
                        if (alt_glyph[i]) {
                            info->GlyphString[i] = alt_glyph[i];
                            alt[i] = true;
                        } else {
                            info->GlyphString[i] = question;
                        }
                    }
                }
            }
            info->GlyphAdvances.SetLength(info->GlyphString.Length());
            info->GlyphPositions.SetLength(info->GlyphString.Length());
            Array<CGSize> adv(0x10);
            adv.SetLength(info->GlyphString.Length());
            info->Fonts.SetLength(info->GlyphString.Length());
            double v = 0.0;
            for (int i = 0; i < adv.Length(); i++) {
                if (alt[i]) {
                    CTFontGetAdvancesForGlyphs(info->font->alt_font, kCTFontOrientationHorizontal, info->GlyphString.GetBuffer() + i, adv.GetBuffer() + i, 1);
                    v += adv[i].width;
                    info->Fonts[i] = info->alt_font;
                } else {
                    if (info->GlyphString[i]) {
                        CTFontGetAdvancesForGlyphs(info->font->font, kCTFontOrientationHorizontal, info->GlyphString.GetBuffer() + i, adv.GetBuffer() + i, 1);
                        v += adv[i].width;
                    } else {
                        info->GlyphString[i] = space;
                        int w = int(int64(v) + 4 * info->font->width) / int64(4 * info->font->width);
                        double lv = v;
                        v = double(4 * info->font->width * w);
                        adv[i].width = v - lv;
                    }
                    info->Fonts[i] = info->base_font;
                }
            }
            for (int i = 0; i < adv.Length(); i++) info->GlyphAdvances[i] = adv[i].width;
            if (adv.Length()) {
                info->GlyphPositions[0].x = 0.0;
                info->GlyphPositions[0].y = 0.0;
                for (int i = 1; i < adv.Length(); i++) {
                    info->GlyphPositions[i].x = info->GlyphPositions[i - 1].x + adv[i - 1].width;
                    info->GlyphPositions[i].y = 0.0;
                }
            }
            info->halign = horizontal_align;
            info->valign = vertical_align;
            info->run_length = int(info->GlyphPositions.LastElement().x + info->GlyphAdvances.LastElement());
            info->BaselineOffset = int(CTFontGetDescent(reinterpret_cast<CoreTextFont *>(font)->font));
            info->UnderlineOffset = CTFontGetUnderlinePosition(reinterpret_cast<CoreTextFont *>(font)->font);
            info->UnderlineHalfWidth = CTFontGetUnderlineThickness(reinterpret_cast<CoreTextFont *>(font)->font) / 2.0;
            info->StrikeoutOffset = (CTFontGetAscent(reinterpret_cast<CoreTextFont *>(font)->font) - CTFontGetDescent(reinterpret_cast<CoreTextFont *>(font)->font)) / 2.0;
            info->StrikeoutHalfWidth = info->UnderlineHalfWidth;
            info->TextColor.r = double(color.r) / 255.0;
            info->TextColor.g = double(color.g) / 255.0;
            info->TextColor.b = double(color.b) / 255.0;
            info->TextColor.a = double(color.a) / 255.0;
            info->hls = info->hle = -1;
            info->ResetRanges(0);
            info->Retain();
            return info;
        }
        UI::ILineRenderingInfo * QuartzRenderingDevice::CreateLineRenderingInfo(const UI::Color & color, bool dotted) noexcept
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
                int data_len = source->GetScanLineLength() * source->GetHeight();
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
            try {
                UI::IFont * font = new CoreTextFont(FaceName, double(Height), 1.0, Weight, IsItalic, IsUnderline, IsStrikeout);
                return font;
            } catch (...) { return 0; }
        }

        void QuartzRenderingDevice::RenderBar(UI::IBarRenderingInfo * Info, const UI::Box & At) noexcept
        {
            if (!Info) return;
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
        void QuartzRenderingDevice::RenderTexture(UI::ITextureRenderingInfo * Info, const UI::Box & At) noexcept
        {
            if (!Info) return;
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
        void QuartzRenderingDevice::RenderText(UI::ITextRenderingInfo * Info, const UI::Box & At, bool Clip) noexcept
        {
            auto info = reinterpret_cast<CoreTextRenderingInfo *>(Info);
            if (!info->GlyphAdvances.Length() || !Info) return;
            if (!info->base_font) return;
            CGRect at = QuartzMakeRect(At, _width, _height, _scale);
            if (Clip) {
                CGContextSaveGState(reinterpret_cast<CGContextRef>(_context));
                CGContextClipToRect(reinterpret_cast<CGContextRef>(_context), at);
            }
			int width, height;
			Info->GetExtent(width, height);
			int shift_x = 0;
			int shift_y = (info->font->height - info->font->zoomed_height) / double(_scale);
			if (info->halign == 1) shift_x += (At.Right - At.Left - width) / 2;
			else if (info->halign == 2) shift_x += (At.Right - At.Left - width);
			if (info->valign == 1) shift_y += (At.Bottom - At.Top - height) / 2;
			else if (info->valign == 0) shift_y += (At.Bottom - At.Top - height);
            CGAffineTransform trans;
            trans.a = trans.d = 1.0 / double(_scale); trans.b = trans.c = 0.0;
            trans.tx = at.origin.x + double(shift_x) / double(_scale);
            trans.ty = at.origin.y + double(shift_y + info->BaselineOffset) / double(_scale);
			if (info->hls != -1) {
				double start = double(info->EndOfChar(info->hls - 1));
				double end = double(info->EndOfChar(info->hle - 1));
                CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context), info->BackColor.r, info->BackColor.g, info->BackColor.b, info->BackColor.a);
                CGContextFillRect(reinterpret_cast<CGContextRef>(_context), CGRectMake(trans.tx + start / double(_scale),
                    at.origin.y, (end - start) / double(_scale), at.size.height));
			}
            for (int i = 0; i < info->Ranges.Length(); i++) {
                CGContextSetFont(reinterpret_cast<CGContextRef>(_context), info->Ranges[i].Font);
                CGContextSetFontSize(reinterpret_cast<CGContextRef>(_context), info->font->zoomed_height);
                CGContextSetTextDrawingMode(reinterpret_cast<CGContextRef>(_context), kCGTextFill);
                CGContextSetTextPosition(reinterpret_cast<CGContextRef>(_context), 0.0, 0.0);
                CGContextSetTextMatrix(reinterpret_cast<CGContextRef>(_context), trans);
                CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context),
                    info->Ranges[i].Brush.r, info->Ranges[i].Brush.g, info->Ranges[i].Brush.b, info->Ranges[i].Brush.a);
                CGContextShowGlyphsAtPositions(reinterpret_cast<CGContextRef>(_context),
                    info->GlyphString.GetBuffer() + info->Ranges[i].LeftEdge, info->GlyphPositions.GetBuffer() + info->Ranges[i].LeftEdge,
                    info->Ranges[i].RightEdge - info->Ranges[i].LeftEdge);
            }
			if (info->font->underline) {
                CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context), info->TextColor.r, info->TextColor.g, info->TextColor.b, info->TextColor.a);
                CGContextFillRect(reinterpret_cast<CGContextRef>(_context), CGRectMake(trans.tx, trans.ty + (info->UnderlineOffset - info->UnderlineHalfWidth) / double(_scale),
                    double(info->run_length) / double(_scale), 2.0 * info->UnderlineHalfWidth / double(_scale)));
			}
			if (info->font->strikeout) {
                CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context), info->TextColor.r, info->TextColor.g, info->TextColor.b, info->TextColor.a);
                CGContextFillRect(reinterpret_cast<CGContextRef>(_context), CGRectMake(trans.tx, trans.ty + (info->StrikeoutOffset - info->StrikeoutHalfWidth) / double(_scale),
                    double(info->run_length) / double(_scale), 2.0 * info->StrikeoutHalfWidth / double(_scale)));
			}
            if (Clip) CGContextRestoreGState(reinterpret_cast<CGContextRef>(_context));
        }
        void QuartzRenderingDevice::RenderLine(UI::ILineRenderingInfo * Info, const UI::Box & At) noexcept
        {
            if (!Info) return;
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
        void QuartzRenderingDevice::ApplyBlur(UI::IBlurEffectRenderingInfo * Info, const UI::Box & At) noexcept {}
        void QuartzRenderingDevice::ApplyInversion(UI::IInversionEffectRenderingInfo * Info, const UI::Box & At, bool Blink) noexcept
        {
            if (!Info) return;
            auto info = static_cast<QuartzInversionRenderingInfo *>(Info);
            if (!Blink || (_animation % 1000 < 500)) {
                CGContextSetBlendMode(reinterpret_cast<CGContextRef>(_context), kCGBlendModeDifference);
                CGContextDrawImage(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(At, _width, _height, _scale), info->white);
                CGContextSetBlendMode(reinterpret_cast<CGContextRef>(_context), kCGBlendModeNormal);
            }
        }

        void QuartzRenderingDevice::PushClip(const UI::Box & Rect) noexcept
        {
            UI::Box clip = Clipping.Length() ? UI::Box::Intersect(Rect, Clipping.LastElement()) : Rect;
            Clipping << clip;
            CGContextSaveGState(reinterpret_cast<CGContextRef>(_context));
            CGContextClipToRect(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(Rect, _width, _height, _scale));
        }
        void QuartzRenderingDevice::PopClip(void) noexcept
        {
            Clipping.RemoveLast();
            CGContextRestoreGState(reinterpret_cast<CGContextRef>(_context));
        }
        void QuartzRenderingDevice::BeginLayer(const UI::Box & Rect, double Opacity) noexcept
        {
            CGContextSaveGState(reinterpret_cast<CGContextRef>(_context));
            CGContextClipToRect(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(Rect, _width, _height, _scale));
            CGContextSetAlpha(reinterpret_cast<CGContextRef>(_context), CGFloat(Opacity));
            CGContextBeginTransparencyLayer(reinterpret_cast<CGContextRef>(_context), 0);
        }
        void QuartzRenderingDevice::EndLayer(void) noexcept
        {
            CGContextEndTransparencyLayer(reinterpret_cast<CGContextRef>(_context));
            CGContextRestoreGState(reinterpret_cast<CGContextRef>(_context));
        }

        void QuartzRenderingDevice::SetTimerValue(uint32 time) noexcept { _animation = time; }
        uint32 QuartzRenderingDevice::GetCaretBlinkHalfTime(void) noexcept { return 500; }
        void QuartzRenderingDevice::ClearCache(void) noexcept
        {
            BrushCache.Clear();
            InversionCache.SetReference(0);
        }

        Drawing::ICanvasRenderingDevice * QuartzRenderingDevice::QueryCanvasDevice(void) noexcept { return this; }
        void QuartzRenderingDevice::DrawPolygon(const Math::Vector2 * points, int count, const Math::Color & color, double width) noexcept
        {
            CGContextSetRGBStrokeColor(reinterpret_cast<CGContextRef>(_context), color.x, color.y, color.z, color.w);
            CGContextSetLineWidth(reinterpret_cast<CGContextRef>(_context), width / double(_scale));
            CGContextSetLineJoin(reinterpret_cast<CGContextRef>(_context), kCGLineJoinRound);
            CGContextBeginPath(reinterpret_cast<CGContextRef>(_context));
            if (count) {
                CGContextMoveToPoint(reinterpret_cast<CGContextRef>(_context), points[0].x / _scale, (double(_height) - points[0].y) / _scale);
                for (int i = 1; i < count; i++) CGContextAddLineToPoint(reinterpret_cast<CGContextRef>(_context), points[i].x / double(_scale), (double(_height) - points[i].y) / double(_scale));
            }
            CGContextStrokePath(reinterpret_cast<CGContextRef>(_context));
        }
        void QuartzRenderingDevice::FillPolygon(const Math::Vector2 * points, int count, const Math::Color & color) noexcept
        {
            CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context), color.x, color.y, color.z, color.w);
            CGContextBeginPath(reinterpret_cast<CGContextRef>(_context));
            if (count) {
                CGContextMoveToPoint(reinterpret_cast<CGContextRef>(_context), points[0].x / _scale, (double(_height) - points[0].y) / _scale);
                for (int i = 1; i < count; i++) CGContextAddLineToPoint(reinterpret_cast<CGContextRef>(_context), points[i].x / double(_scale), (double(_height) - points[i].y) / double(_scale));
            }
            CGContextFillPath(reinterpret_cast<CGContextRef>(_context));
        }
        Drawing::ITextureRenderingDevice * QuartzRenderingDevice::CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept { return CreateQuartzCompatibleTextureRenderingDevice(width, height, color); }
        void QuartzRenderingDevice::BeginDraw(void) noexcept {}
        void QuartzRenderingDevice::EndDraw(void) noexcept { CGContextFlush(reinterpret_cast<CGContextRef>(_context)); }
        UI::ITexture * QuartzRenderingDevice::GetRenderTargetAsTexture(void) noexcept
        {
            if (!BitmapTarget) return 0;
            SafePointer<Engine::Codec::Frame> frame = GetRenderTargetAsFrame();
            return LoadTexture(frame);
        }
        Engine::Codec::Frame * QuartzRenderingDevice::GetRenderTargetAsFrame(void) noexcept
        {
            if (!BitmapTarget) return 0;
            BitmapTarget->Retain();
            return BitmapTarget;
        }
        Drawing::ITextureRenderingDevice * QuartzRenderingDevice::CreateQuartzCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept
        {
            SafePointer<Codec::Frame> target = new Codec::Frame(width, height, width * 4, Codec::PixelFormat::R8G8B8A8, Codec::AlphaFormat::Premultiplied, Codec::LineDirection::TopDown);
            {
                Math::Color prem = color;
                prem.x *= prem.w;
                prem.y *= prem.w;
                prem.z *= prem.w;
                UI::Color pixel = prem;
                UI::Color * data = reinterpret_cast<UI::Color *>(target->GetData());
                for (int i = 0; i < width * height; i++) data[i] = pixel;
            }
            CGColorSpaceRef clr = CGColorSpaceCreateDeviceRGB();
            if (!clr) return 0;
            CGContextRef context = CGBitmapContextCreateWithData(target->GetData(), width, height, 8, width * 4, clr, kCGImageAlphaPremultipliedLast, 0, 0);
            if (!context) {
                CGColorSpaceRelease(clr);
                return 0;
            }
            CGColorSpaceRelease(clr);
            SafePointer<QuartzRenderingDevice> device = new QuartzRenderingDevice;
            device->SetContext(context, width, height, 1);
            device->BitmapTarget.SetRetain(target);
            device->Retain();
            return device;
        }
        void * GetCoreImageFromTexture(UI::ITexture * texture) { return static_cast<QuartzTexture *>(texture)->frames.FirstElement(); }
    }
}