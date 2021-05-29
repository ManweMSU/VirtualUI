#include "QuartzDevice.h"

#include "../ImageCodec/CodecBase.h"
#include "../Math/Color.h"

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
			struct dev_pair { UI::ITexture * dev_tex; UI::IRenderingDevice * dev_ptr; };
		public:
			Array<dev_pair> variants;
			CGImageRef bitmap;
			int w, h;

			QuartzTexture(void) : variants(0x10), bitmap(0), w(0), h(0) {}
			~QuartzTexture(void) override
			{
				if (bitmap) CGImageRelease(bitmap);
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
			virtual void DeviceWasDestroyed(UI::IRenderingDevice * device) noexcept override
			{
				for (int i = 0; i < variants.Length(); i++) if (variants[i].dev_ptr == device) { variants.Remove(i); break; }
			}
			virtual void AddDeviceVersion(UI::IRenderingDevice * device, ITexture * texture) noexcept override
			{
				for (int i = 0; i < variants.Length(); i++) if (variants[i].dev_ptr == device) return;
				variants << dev_pair{ texture, device };
			}
			virtual bool IsDeviceSpecific(void) const noexcept override { return false; }
			virtual UI::IRenderingDevice * GetParentDevice(void) const noexcept override { return 0; }
			virtual UI::ITexture * GetDeviceVersion(UI::IRenderingDevice * target_device) noexcept override
			{
				if (!target_device) return this;
				for (int i = 0; i < variants.Length(); i++) if (variants[i].dev_ptr == target_device) return variants[i].dev_tex;
				return 0;
			}
			virtual void Reload(Codec::Frame * source) override
			{
				if (bitmap) CGImageRelease(bitmap);
				SafePointer<QuartzRenderingDevice> local_device = new QuartzRenderingDevice;
				SafePointer<QuartzTexture> new_texture = static_cast<QuartzTexture *>(local_device->LoadTexture(source));
				bitmap = new_texture->bitmap;
				w = new_texture->w; h = new_texture->h;
				new_texture->bitmap = 0;
				for (int i = 0; i < variants.Length(); i++) variants[i].dev_tex->Reload(this);
			}
			virtual void Reload(ITexture * device_independent) override {}
			virtual string ToString(void) const override { return L"Engine.Cocoa.QuartzTexture"; }
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
			virtual int GetLineSpacing(void) const noexcept override { return CTFontGetAscent(font) + CTFontGetDescent(font) + CTFontGetLeading(font); }
			virtual int GetBaselineOffset(void) const noexcept override { return CTFontGetAscent(font); }
			virtual string ToString(void) const override { return L"Engine.Cocoa.CoreTextFont"; }
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
				CTFontRef FontCT;
			};
			SafePointer<CoreTextFont> font;
			Array<CGGlyph> GlyphString;
			Array<CGFloat> GlyphAdvances;
			Array<CGPoint> GlyphPositions;
			Array<CTFontRef> FontsCT;
			Array<bool> Surrogate;
			Array<int> CharEnds32;
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

			CoreTextRenderingInfo(void) : GlyphString(0x40), GlyphAdvances(0x40), GlyphPositions(0x40), ExtraBrushes(0x40), FontsCT(0x40), Surrogate(0x40), CharEnds32(0x40), Ranges(0x40) {}
			~CoreTextRenderingInfo(void) override {}

			int TranslatePositionUtf32ToUtf16(int pos) { int p32 = 0, p16 = 0; while (p32 < pos) { p32++; p16++; if (Surrogate[p16]) p16++; } return p16; }
			int TranslatePositionUtf16ToUtf32(int pos) { int s = 0; for (int i = 0; i < pos; i++) if (!Surrogate[i]) s++; return s; }
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
				if (point > run_length) return TranslatePositionUtf16ToUtf32(GlyphString.Length());
				double p = double(point);
				double s = 0.0;
				for (int i = 0; i < GlyphAdvances.Length(); i++) {
					if (p <= s + GlyphAdvances[i]) {
						if (p < s + GlyphAdvances[i] / 2.0) return TranslatePositionUtf16ToUtf32(i);
						else return TranslatePositionUtf16ToUtf32(i + 1);
						break;
					}
					s += GlyphAdvances[i];
				}
				return TranslatePositionUtf16ToUtf32(GlyphString.Length());
			}
			int EndOfChar16(int Index) noexcept
			{
				if (Index < 0) return 0;
				double summ = 0.0;
				for (int i = 0; i <= Index; i++) summ += GlyphAdvances[i];
				return int(summ);
			}
			virtual int EndOfChar(int Index) noexcept override
			{
				if (Index < 0) return 0;
				return CharEnds32[Index];
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
				SafePointer< Array<uint8> > ind;
				if (indicies) ind = new Array<uint8>(*indicies); else ind = 0;
				if (ind) for (int i = 0; i < Surrogate.Length(); i++) if (Surrogate[i]) ind->Insert(ind->ElementAt(i - 1), i);
				Ranges.Clear();
				int cp = 0;
				while (cp < FontsCT.Length()) {
					int ep = cp;
					while (ep < FontsCT.Length() && FontsCT[ep] == FontsCT[cp] && (!ind || ind->ElementAt(ep) == ind->ElementAt(cp))) ep++;
					int index = ind ? ind->ElementAt(cp) : 0;
					CTFontRef font_ref_ct = FontsCT[cp];
					Ranges << TextRange{ cp, ep, (index == 0) ? TextColor : ExtraBrushes[index - 1], font_ref_ct };
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

		QuartzRenderingDevice::QuartzRenderingDevice(void) : BrushCache(0x20, Dictionary::ExcludePolicy::ExcludeLeastRefrenced), Clipping(0x20), _animation(0) {}
		QuartzRenderingDevice::~QuartzRenderingDevice(void) { if (BitmapTarget) CGContextRelease(reinterpret_cast<CGContextRef>(_context)); }

		void * QuartzRenderingDevice::GetContext(void) const noexcept { return _context; }
		void QuartzRenderingDevice::SetContext(void * context, int width, int height, int scale) noexcept
		{
			_context = context; _width = width; _height = height; _scale = scale;
			CGContextSetInterpolationQuality(reinterpret_cast<CGContextRef>(_context), kCGInterpolationNone);
		}

		void QuartzRenderingDevice::TextureWasDestroyed(UI::ITexture * texture) noexcept {}
		void QuartzRenderingDevice::GetImplementationInfo(string & tech, uint32 & version) noexcept { tech = L"Quartz"; version = 1; }
		uint32 QuartzRenderingDevice::GetFeatureList(void) noexcept
		{
			uint32 result = UI::RenderingDeviceFeatureInversionCapable | UI::RenderingDeviceFeaturePolygonCapable | UI::RenderingDeviceFeatureLayersCapable;
			if (BitmapTarget) result |= UI::RenderingDeviceFeatureTextureTarget;
			return result;
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
			info->prop_w = Math::cos(angle);
			info->prop_h = Math::sin(angle);
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
			if (!texture) return 0;
			auto tex = static_cast<QuartzTexture *>(texture);
			SafePointer<QuartzTextureRenderingInfo> info = new QuartzTextureRenderingInfo;
			info->fill = fill_pattern;
			if (take_area.Left == 0 && take_area.Top == 0 && take_area.Right == texture->GetWidth() && take_area.Bottom == texture->GetHeight()) {
				info->fragment.SetRetain(tex);
			} else {
				info->fragment = new QuartzTexture;
				CGRect rect = CGRectMake(double(take_area.Left), double(take_area.Top),
					double(take_area.Right - take_area.Left), double(take_area.Bottom - take_area.Top));
				info->fragment->bitmap = CGImageCreateWithImageInRect(tex->bitmap, rect);
				info->fragment->w = take_area.Right - take_area.Left;
				info->fragment->h = take_area.Bottom - take_area.Top;
			}
			info->Retain();
			return info;
		}
		UI::ITextureRenderingInfo * QuartzRenderingDevice::CreateTextureRenderingInfo(Graphics::ITexture * texture) noexcept { return 0; }
		UI::ITextRenderingInfo * QuartzRenderingDevice::CreateTextRenderingInfo(UI::IFont * font, const string & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept
		{
			if (!font) return 0;
			string copy = text.NormalizedForm();
			Array<uint32> chars(copy.Length());
			chars.SetLength(copy.Length());
			copy.Encode(chars.GetBuffer(), Encoding::UTF32, false);
			return CreateTextRenderingInfoRaw(font, chars, horizontal_align, vertical_align, color);
		}
		UI::ITextRenderingInfo * QuartzRenderingDevice::CreateTextRenderingInfo(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept
		{
			if (!font) return 0;
			return CreateTextRenderingInfo(font, string(text.GetBuffer(), text.Length(), Encoding::UTF32), horizontal_align, vertical_align, color);
		}
		UI::ITextRenderingInfo * QuartzRenderingDevice::CreateTextRenderingInfoRaw(UI::IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const UI::Color & color) noexcept
		{
			if (!font) return 0;
			SafePointer<CoreTextRenderingInfo> info = new CoreTextRenderingInfo;
			if (!text.Length()) {
				info->run_length = 0;
				info->font.SetRetain(reinterpret_cast<CoreTextFont *>(font));
				info->Retain();
				return info;
			}
			info->CharEnds32.SetLength(text.Length());
			Array<uint16> utf16(0x100);
			Array<bool> alt(0x100);
			Array<CGGlyph> alt_glyph(0x100);
			utf16.SetLength(MeasureSequenceLength(text.GetBuffer(), text.Length(), Encoding::UTF32, Encoding::UTF16));
			info->Surrogate.SetLength(utf16.Length());
			alt.SetLength(utf16.Length());
			alt_glyph.SetLength(utf16.Length());
			ZeroMemory(alt.GetBuffer(), utf16.Length());
			ConvertEncoding(utf16.GetBuffer(), text.GetBuffer(), text.Length(), Encoding::UTF32, Encoding::UTF16);
			for (int i = 0; i < utf16.Length(); i++) info->Surrogate[i] = (utf16[i] >= 0xDC00 && utf16[i] < 0xE000);
			info->font.SetRetain(reinterpret_cast<CoreTextFont *>(font));
			info->GlyphString.SetLength(utf16.Length());
			CTFontGetGlyphsForCharacters(info->font->font, utf16.GetBuffer(), info->GlyphString.GetBuffer(), utf16.Length());
			CTFontGetGlyphsForCharacters(info->font->alt_font, utf16.GetBuffer(), alt_glyph.GetBuffer(), utf16.Length());
			CGGlyph space, alt_space, question; UniChar spc = 32, que = '?';
			CTFontGetGlyphsForCharacters(info->font->font, &spc, &space, 1);
			CTFontGetGlyphsForCharacters(info->font->font, &que, &question, 1);
			CTFontGetGlyphsForCharacters(info->font->alt_font, &spc, &alt_space, 1);
			for (int i = 0; i < utf16.Length(); i++) {
				if (utf16[i] == L'\t') {
					info->GlyphString[i] = 0;
					alt_glyph[i] = 0;
				} else {
					if (!info->GlyphString[i]) {
						if (alt_glyph[i]) {
							info->GlyphString[i] = alt_glyph[i];
							alt[i] = true;
						} else {
							if (info->Surrogate[i]) {
								alt[i] = alt[i - 1];
								info->GlyphString[i] = alt[i] ? alt_space : space;
							} else {
								info->GlyphString[i] = question;
							}
						}
					}
				}
			}
			info->GlyphAdvances.SetLength(info->GlyphString.Length());
			info->GlyphPositions.SetLength(info->GlyphString.Length());
			Array<CGSize> adv(0x10);
			adv.SetLength(info->GlyphString.Length());
			info->FontsCT.SetLength(info->GlyphString.Length());
			double v = 0.0;
			for (int i = 0; i < adv.Length(); i++) {
				if (alt[i]) {
					if (!info->Surrogate[i]) {
						CTFontGetAdvancesForGlyphs(info->font->alt_font, kCTFontOrientationHorizontal, info->GlyphString.GetBuffer() + i, adv.GetBuffer() + i, 1);
						v += adv[i].width;
					} else adv[i].width = 0.0;
					info->FontsCT[i] = info->font->alt_font;
				} else {
					if (info->GlyphString[i]) {
						if (!info->Surrogate[i]) {
							CTFontGetAdvancesForGlyphs(info->font->font, kCTFontOrientationHorizontal, info->GlyphString.GetBuffer() + i, adv.GetBuffer() + i, 1);
							v += adv[i].width;
						} else adv[i].width = 0.0;
					} else if (!info->Surrogate[i]) {
						info->GlyphString[i] = space;
						int w = int(int64(v) + 4 * info->font->width) / int64(max(int(4 * info->font->width), 1));
						double lv = v;
						v = double(4 * info->font->width * w);
						adv[i].width = v - lv;
					} else {
						info->GlyphString[i] = space;
					}
					info->FontsCT[i] = info->font->font;
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
			for (int i = 0; i < info->CharEnds32.Length(); i++) info->CharEnds32[i] = info->EndOfChar16(info->TranslatePositionUtf32ToUtf16(i));
			info->ResetRanges(0);
			info->Retain();
			return info;
		}

		Graphics::ITexture * QuartzRenderingDevice::CreateIntermediateRenderTarget(Graphics::PixelFormat format, int width, int height) { return 0; }

		void QuartzRenderingDevice::RenderBar(UI::IBarRenderingInfo * Info, const UI::Box & At) noexcept
		{
			if (!Info) return;
			auto info = static_cast<QuartzBarRenderingInfo *>(Info);
			if (info->gradient) {
				int w = At.Right - At.Left, h = At.Bottom - At.Top;
				if (!w || !h) return;
				CGPoint s, e;
				double cx = double(At.Right + At.Left) / 2.0;
				double cy = double(At.Top + At.Bottom) / 2.0;
				double rx = double(w) / 2.0;
				double ry = double(h) / 2.0;
				double mx = max(abs(info->prop_w), abs(info->prop_h));
				double boundary_sx = info->prop_w / mx;
				double boundary_sy = info->prop_h / mx;
				s.x = cx - rx * boundary_sx;
				s.y = cy + ry * boundary_sy;
				e.x = cx + rx * boundary_sx;
				e.y = cy - ry * boundary_sy;
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
			if (info->fill) {
				double s = double(_scale);
				PushClip(At);
				CGContextDrawTiledImage(reinterpret_cast<CGContextRef>(_context), CGRectMake(0.0f, 0.0f, double(info->fragment->w) / s, double(info->fragment->h) / s), info->fragment->bitmap);
				PopClip();
			} else {
				CGContextDrawImage(reinterpret_cast<CGContextRef>(_context), QuartzMakeRect(At, _width, _height, _scale), info->fragment->bitmap);
			}
		}
		void QuartzRenderingDevice::RenderText(UI::ITextRenderingInfo * Info, const UI::Box & At, bool Clip) noexcept
		{
			auto info = reinterpret_cast<CoreTextRenderingInfo *>(Info);
			if (!Info || !info->GlyphAdvances.Length()) return;
			CGRect at = QuartzMakeRect(At, _width, _height, _scale);
			if (Clip) {
				CGContextSaveGState(reinterpret_cast<CGContextRef>(_context));
				CGContextClipToRect(reinterpret_cast<CGContextRef>(_context), at);
			}
			int width, height;
			Info->GetExtent(width, height);
			int shift_x = 0;
			int shift_y = (info->font->height - info->font->zoomed_height) / 2 / double(_scale);
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
				CGContextSetFontSize(reinterpret_cast<CGContextRef>(_context), info->font->zoomed_height);
				CGContextSetTextDrawingMode(reinterpret_cast<CGContextRef>(_context), kCGTextFill);
				CGContextSetTextPosition(reinterpret_cast<CGContextRef>(_context), 0.0, 0.0);
				CGContextSetTextMatrix(reinterpret_cast<CGContextRef>(_context), trans);
				CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context),
					info->Ranges[i].Brush.r, info->Ranges[i].Brush.g, info->Ranges[i].Brush.b, info->Ranges[i].Brush.a);
				CTFontDrawGlyphs(info->Ranges[i].FontCT, info->GlyphString.GetBuffer() + info->Ranges[i].LeftEdge, info->GlyphPositions.GetBuffer() + info->Ranges[i].LeftEdge,
					info->Ranges[i].RightEdge - info->Ranges[i].LeftEdge, reinterpret_cast<CGContextRef>(_context));
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

		void QuartzRenderingDevice::DrawPolygon(const Math::Vector2 * points, int count, UI::Color color, double width) noexcept
		{
			CGContextSetRGBStrokeColor(reinterpret_cast<CGContextRef>(_context), color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
			CGContextSetLineWidth(reinterpret_cast<CGContextRef>(_context), width / double(_scale));
			CGContextSetLineJoin(reinterpret_cast<CGContextRef>(_context), kCGLineJoinRound);
			CGContextBeginPath(reinterpret_cast<CGContextRef>(_context));
			if (count) {
				CGContextMoveToPoint(reinterpret_cast<CGContextRef>(_context), points[0].x / _scale, (double(_height) - points[0].y) / _scale);
				for (int i = 1; i < count; i++) CGContextAddLineToPoint(reinterpret_cast<CGContextRef>(_context), points[i].x / double(_scale), (double(_height) - points[i].y) / double(_scale));
			}
			CGContextStrokePath(reinterpret_cast<CGContextRef>(_context));
		}
		void QuartzRenderingDevice::FillPolygon(const Math::Vector2 * points, int count, UI::Color color) noexcept
		{
			CGContextSetRGBFillColor(reinterpret_cast<CGContextRef>(_context), color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
			CGContextBeginPath(reinterpret_cast<CGContextRef>(_context));
			if (count) {
				CGContextMoveToPoint(reinterpret_cast<CGContextRef>(_context), points[0].x / _scale, (double(_height) - points[0].y) / _scale);
				for (int i = 1; i < count; i++) CGContextAddLineToPoint(reinterpret_cast<CGContextRef>(_context), points[i].x / double(_scale), (double(_height) - points[i].y) / double(_scale));
			}
			CGContextFillPath(reinterpret_cast<CGContextRef>(_context));
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
		bool QuartzRenderingDevice::CaretShouldBeVisible(void) noexcept { return _animation % 1000 < 500; }
		void QuartzRenderingDevice::ClearCache(void) noexcept
		{
			BrushCache.Clear();
			InversionCache.SetReference(0);
		}

		UI::ITexture * QuartzRenderingDevice::LoadTexture(Codec::Frame * source) noexcept
		{
			return StaticLoadTexture(source);
		}
		UI::IFont * QuartzRenderingDevice::LoadFont(const string & face_name, int height, int weight, bool italic, bool underline, bool strikeout) noexcept
		{
			return StaticLoadFont(face_name, height, weight, italic, underline, strikeout);
		}
		UI::ITextureRenderingDevice * QuartzRenderingDevice::CreateTextureRenderingDevice(int width, int height, UI::Color color) noexcept
		{
			return StaticCreateTextureRenderingDevice(width, height, color);
		}
		UI::ITextureRenderingDevice * QuartzRenderingDevice::CreateTextureRenderingDevice(Codec::Frame * frame) noexcept
		{
			return StaticCreateTextureRenderingDevice(frame);
		}

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
		string QuartzRenderingDevice::ToString(void) const { return L"Engine.Cocoa.QuartzRenderingDevice"; }

		UI::ITexture * QuartzRenderingDevice::StaticLoadTexture(Codec::Frame * source) noexcept
		{
			SafePointer<QuartzTexture> result = new (std::nothrow) QuartzTexture;
			if (!result) return 0;
			result->w = source->GetWidth();
			result->h = source->GetHeight();
			SafePointer<Codec::Frame> converted;
			try {
				if (source->GetScanOrigin() == Codec::ScanOrigin::TopDown && source->GetPixelFormat() == Codec::PixelFormat::R8G8B8A8) {
					converted.SetRetain(source);
				} else {
					converted = source->ConvertFormat(Codec::PixelFormat::R8G8B8A8, source->GetAlphaMode(), Codec::ScanOrigin::TopDown);
				}
			} catch (...) { return 0; }
			int data_len = converted->GetScanLineLength() * converted->GetHeight();
			uint8 * data = reinterpret_cast<uint8 *>(malloc(data_len));
			if (!data) return 0;
			MemoryCopy(data, converted->GetData(), data_len);
			CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
			CGDataProviderRef provider = CGDataProviderCreateWithData(data, data, data_len, QuartzDataRelease);
			CGImageRef frame = CGImageCreate(result->w, result->h, 8, 32, converted->GetScanLineLength(), rgb,
				(converted->GetAlphaMode() == Codec::AlphaMode::Premultiplied) ? kCGImageAlphaPremultipliedLast : kCGImageAlphaLast,
				provider, 0, false, kCGRenderingIntentDefault);
			CGColorSpaceRelease(rgb);
			CGDataProviderRelease(provider);
			if (!frame) return 0;
			result->bitmap = frame;
			result->Retain();
			return result;
		}
		UI::IFont * QuartzRenderingDevice::StaticLoadFont(const string & face_name, int height, int weight, bool italic, bool underline, bool strikeout) noexcept
		{
			try {
				UI::IFont * font = new CoreTextFont(face_name, double(height), 1.0, weight, italic, underline, strikeout);
				return font;
			} catch (...) { return 0; }
		}
		UI::ITextureRenderingDevice * QuartzRenderingDevice::StaticCreateTextureRenderingDevice(int width, int height, UI::Color color) noexcept
		{
			SafePointer<Codec::Frame> target = new Codec::Frame(width, height, width * 4, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Premultiplied, Codec::ScanOrigin::TopDown);
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
		UI::ITextureRenderingDevice * QuartzRenderingDevice::StaticCreateTextureRenderingDevice(Codec::Frame * frame) noexcept
		{
			SafePointer<Codec::Frame> target = frame->ConvertFormat(Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Premultiplied, Codec::ScanOrigin::TopDown);
			int width = frame->GetWidth();
			int height = frame->GetHeight();
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
		void * GetCoreImageFromTexture(UI::ITexture * texture) { return static_cast<QuartzTexture *>(texture)->bitmap; }
	}
}