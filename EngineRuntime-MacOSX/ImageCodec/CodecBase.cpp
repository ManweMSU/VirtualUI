#include "CodecBase.h"

#include <math.h>

namespace Engine
{
	namespace Codec
	{
		FrameFormat::FrameFormat(void) {}
		FrameFormat::FrameFormat(PixelFormat format, AlphaFormat alpha, LineDirection direction) :
			Format(format), Alpha(alpha), Direction(direction) {}

		bool IsPalettePixel(PixelFormat format)
		{
			return (format == PixelFormat::P8 || format == PixelFormat::P4 || format == PixelFormat::P2 || format == PixelFormat::P1);
		}
		uint32 GetPaletteVolume(PixelFormat format)
		{
			uint32 v = 1;
			for (int i = 0; i < int(GetBitsPerPixel(format)); i++) v <<= 1;
			return v;
		}
		uint32 GetBitsPerPixel(PixelFormat format)
		{
			if (format == PixelFormat::B8G8R8A8) return 32;
			else if (format == PixelFormat::B8G8R8U8) return 32;
			else if (format == PixelFormat::R8G8B8A8) return 32;
			else if (format == PixelFormat::R8G8B8U8) return 32;
			else if (format == PixelFormat::B8G8R8) return 24;
			else if (format == PixelFormat::R8G8B8) return 24;
			else if (format == PixelFormat::P8) return 8;
			else if (format == PixelFormat::P4) return 4;
			else if (format == PixelFormat::P2) return 2;
			else if (format == PixelFormat::P1) return 1;
			else return 0;
		}
		uint32 GetRedChannel(uint32 source, PixelFormat source_format, AlphaFormat source_alpha)
		{
			uint32 ev = 0;
			if (source_format == PixelFormat::B8G8R8A8 || source_format == PixelFormat::B8G8R8) ev = (source & 0x00FF0000) >> 16;
			else if (source_format == PixelFormat::R8G8B8A8 || source_format == PixelFormat::R8G8B8) ev = (source & 0x000000FF);
			else if (source_format == PixelFormat::B8G8R8U8) ev = (source & 0x00FF0000) >> 16;
			else if (source_format == PixelFormat::R8G8B8U8) ev = (source & 0x000000FF);
			if (source_alpha == AlphaFormat::Premultiplied) {
				uint32 ac = GetAlphaChannel(source, source_format, source_alpha);
				if (ac) { ev *= 255; ev /= ac; }
			}
			return ev;
		}
		uint32 GetGreenChannel(uint32 source, PixelFormat source_format, AlphaFormat source_alpha)
		{
			uint32 ev = 0;
			if (source_format == PixelFormat::B8G8R8A8 || source_format == PixelFormat::B8G8R8) ev = (source & 0x0000FF00) >> 8;
			else if (source_format == PixelFormat::R8G8B8A8 || source_format == PixelFormat::R8G8B8) ev = (source & 0x0000FF00) >> 8;
			else if (source_format == PixelFormat::B8G8R8U8) ev = (source & 0x0000FF00) >> 8;
			else if (source_format == PixelFormat::R8G8B8U8) ev = (source & 0x0000FF00) >> 8;
			if (source_alpha == AlphaFormat::Premultiplied) {
				uint32 ac = GetAlphaChannel(source, source_format, source_alpha);
				if (ac) { ev *= 255; ev /= ac; }
			}
			return ev;
		}
		uint32 GetBlueChannel(uint32 source, PixelFormat source_format, AlphaFormat source_alpha)
		{
			uint32 ev = 0;
			if (source_format == PixelFormat::B8G8R8A8 || source_format == PixelFormat::B8G8R8) ev = (source & 0x000000FF);
			else if (source_format == PixelFormat::R8G8B8A8 || source_format == PixelFormat::R8G8B8) ev = (source & 0x00FF0000) >> 16;
			else if (source_format == PixelFormat::B8G8R8U8) ev = (source & 0x000000FF);
			else if (source_format == PixelFormat::R8G8B8U8) ev = (source & 0x00FF0000) >> 16;
			if (source_alpha == AlphaFormat::Premultiplied) {
				uint32 ac = GetAlphaChannel(source, source_format, source_alpha);
				if (ac) { ev *= 255; ev /= ac; }
			}
			return ev;
		}
		uint32 GetAlphaChannel(uint32 source, PixelFormat source_format, AlphaFormat source_alpha)
		{
			uint32 ev = 0;
			if (source_format == PixelFormat::B8G8R8A8) ev = (source & 0xFF000000) >> 24;
			else if (source_format == PixelFormat::R8G8B8A8) ev = (source & 0xFF000000) >> 24;
			else if (source_format == PixelFormat::B8G8R8U8 || source_format == PixelFormat::B8G8R8) ev = 0xFF;
			else if (source_format == PixelFormat::R8G8B8U8 || source_format == PixelFormat::R8G8B8) ev = 0xFF;
			return ev;
		}
		uint32 MakePixel(uint32 r, uint32 g, uint32 b, uint32 a, PixelFormat format, AlphaFormat alpha)
		{
			if (alpha == AlphaFormat::Premultiplied) {
				r *= a; r /= 255;
				g *= a; g /= 255;
				b *= a; b /= 255;
			}
			if (format == PixelFormat::B8G8R8A8) {
				return b | (g << 8) | (r << 16) | (a << 24);
			} else if (format == PixelFormat::R8G8B8A8) {
				return r | (g << 8) | (b << 16) | (a << 24);
			} else if (format == PixelFormat::B8G8R8U8 || format == PixelFormat::B8G8R8) {
				return b | (g << 8) | (r << 16);
			} else if (format == PixelFormat::R8G8B8U8 || format == PixelFormat::R8G8B8) {
				return r | (g << 8) | (b << 16);
			} else return 0;
		}
		uint32 ConvertPixel(uint32 source, PixelFormat source_format, AlphaFormat source_alpha, PixelFormat format, AlphaFormat alpha)
		{
			return MakePixel(
				GetRedChannel(source, source_format, source_alpha),
				GetGreenChannel(source, source_format, source_alpha),
				GetBlueChannel(source, source_format, source_alpha),
				GetAlphaChannel(source, source_format, source_alpha),
				format, alpha);
		}

		Frame::Frame(int32 width, int32 height, int32 scan_line_length, PixelFormat format, AlphaFormat alpha, LineDirection direction) :
			Width(width), Height(height), ScanLineLength(scan_line_length), Format(format), Alpha(alpha), Direction(direction), Palette(0x10)
		{
			if (width <= 0 || height <= 0 || scan_line_length < -1 || scan_line_length == 0) throw InvalidArgumentException();
			if (ScanLineLength == -1) ScanLineLength = ((Width * GetBitsPerPixel(format) + 31) / 32) * 4;
			RawData = reinterpret_cast<uint8 *>(malloc(ScanLineLength * Height));
			if (!RawData) throw OutOfMemoryException();
			ZeroMemory(RawData, ScanLineLength * Height);
			if (IsPalettePixel(Format)) {
				Palette.SetLength(Engine::Codec::GetPaletteVolume(Format));
				ZeroMemory(Palette.GetBuffer(), 4 * Palette.Length());
			}
		}
		Frame::~Frame(void) { free(RawData); }
		int32 Frame::GetWidth(void) const { return Width; }
		int32 Frame::GetHeight(void) const { return Height; }
		int32 Frame::GetScanLineLength(void) const { return ScanLineLength; }
		PixelFormat Frame::GetPixelFormat(void) const { return Format; }
		AlphaFormat Frame::GetAlphaFormat(void) const { return Alpha; }
		LineDirection Frame::GetLineDirection(void) const { return Direction; }
		const uint32 * Frame::GetPalette(void) const { return Palette.GetBuffer(); }
		uint32 * Frame::GetPalette(void) { return Palette.GetBuffer(); }
		int Frame::GetPaletteVolume(void) const { return Palette.Length(); }
		void Frame::SetPaletteVolume(int volume)
		{
			if (!IsPalettePixel(Format)) throw InvalidArgumentException();
			if (volume < 1 || volume > 0x100) throw InvalidArgumentException();
			for (int y = 0; y < Height; y++) for (int x = 0; x < Width; x++) {
				if (GetPixel(x, y) >= uint32(volume)) throw InvalidArgumentException();
			}
			Palette.SetLength(volume);
		}
		uint32 Frame::GetPixel(int x, int y) const
		{
			if (Direction == LineDirection::BottomUp) y = Height - y - 1;
			uint32 bpp = GetBitsPerPixel(Format);
			if (bpp == 32) {
				return *reinterpret_cast<const uint32 *>(RawData + ScanLineLength * y + 4 * x);
			} else if (bpp == 24) {
				int base = ScanLineLength * y + 3 * x;
				return RawData[base] | (uint32(RawData[base + 1]) << 8) || (uint32(RawData[base + 2]) << 16);
			} else if (bpp <= 8) {
				uint64 base = uint64(ScanLineLength) * 8 * y + uint64(bpp) * x;
				int byte = int(base / 8);
				int bit = 8 - base % 8 - bpp;
				uint8 b = RawData[byte];
				if (bpp == 8) return b;
				else if (bpp == 4) return (b >> bit) & 0xF;
				else if (bpp == 2) return (b >> bit) & 0x3;
				else if (bpp == 1) return (b >> bit) & 0x1;
				else return 0;
			} else return 0;
		}
		void Frame::SetPixel(int x, int y, uint32 v)
		{
			if (Direction == LineDirection::BottomUp) y = Height - y - 1;
			uint32 bpp = GetBitsPerPixel(Format);
			if (bpp == 32) {
				*reinterpret_cast<uint32 *>(RawData + ScanLineLength * y + 4 * x) = v;
			} else if (bpp == 24) {
				int base = ScanLineLength * y + 3 * x;
				RawData[base] = v & 0xFF;
				RawData[base + 1] = (v & 0xFF00) >> 8;
				RawData[base + 2] = (v & 0xFF0000) >> 16;
			} else if (bpp <= 8) {
				uint64 base = uint64(ScanLineLength) * 8 * y + uint64(bpp) * x;
				int byte = int(base / 8);
				int bit = 8 - base % 8 - bpp;
				if (bpp == 8) RawData[byte] = v;
				else if (bpp == 4) {
					RawData[byte] &= 0xF ^ (0xF << bit);
					RawData[byte] |= v << bit;
				} else if (bpp == 2) {
					RawData[byte] &= 0xF ^ (0x3 << bit);
					RawData[byte] |= v << bit;
				} else if (bpp == 1) {
					RawData[byte] &= 0xF ^ (0x1 << bit);
					RawData[byte] |= v << bit;
				}
			}
		}
		uint8 * Frame::GetData(void) { return RawData; }
		const uint8 * Frame::GetData(void) const { return RawData; }
		uint32 Frame::GetBestPaletteIndex(uint32 color) const
		{
			int32 b = color & 0xFF;
			int32 g = (color >> 8) & 0xFF;
			int32 r = (color >> 16) & 0xFF;
			int32 a = (color >> 24) & 0xFF;
			int32 min_dist = -1;
			int32 min_index = -1;
			for (int i = 0; i < Palette.Length(); i++) {
				int32 pb = Palette[i] & 0xFF;
				int32 pg = (Palette[i] >> 8) & 0xFF;
				int32 pr = (Palette[i] >> 16) & 0xFF;
				int32 pa = (Palette[i] >> 24) & 0xFF;
				int32 dist = (pr - r) * (pr - r) + (pg - g) * (pg - g) + (pb - b) * (pb - b) + (pa - a) * (pa - a);
				if (dist < min_dist || min_index == -1) {
					min_dist = dist;
					min_index = i;
				}
			}
			return min_index;
		}
		Frame * Frame::ConvertFormat(const FrameFormat & new_format) const
		{
			SafePointer<Frame> New = new Frame(Width, Height, -1, new_format.Format, new_format.Alpha, new_format.Direction);
			if (!IsPalettePixel(new_format.Format) && !IsPalettePixel(Format)) {
				if (new_format.Format == Format && new_format.Alpha == Alpha) {
					for (int y = 0; y < Height; y++) for (int x = 0; x < Width; x++) {
						New->SetPixel(x, y, GetPixel(x, y));
					}
				} else {
					for (int y = 0; y < Height; y++) for (int x = 0; x < Width; x++) {
						New->SetPixel(x, y, ConvertPixel(GetPixel(x, y), Format, Alpha, new_format.Format, new_format.Alpha));
					}
				}
			} else if (!IsPalettePixel(new_format.Format)){
				for (int y = 0; y < Height; y++) for (int x = 0; x < Width; x++) {
					New->SetPixel(x, y, ConvertPixel(Palette[GetPixel(x, y)], PixelFormat::B8G8R8A8, AlphaFormat::Normal, new_format.Format, new_format.Alpha));
				}
			} else {
				bool plt = IsPalettePixel(Format);
				if (New->Palette.Length() == 256) {
					New->Palette.Clear();
					Array<uint32> clr_used(0x100);
					for (int y = 0; y < Height; y++) for (int x = 0; x < Width; x++) {
						uint32 src_color = plt ? Palette[GetPixel(x, y)] :
							ConvertPixel(GetPixel(x, y), Format, Alpha, PixelFormat::B8G8R8A8, AlphaFormat::Normal);
						if (clr_used.Length() <= 0x100) {
							bool found = false;
							for (int j = 0; j < clr_used.Length(); j++) {
								if (clr_used[j] == src_color) { found = true; break; }
							}
							if (!found) clr_used << src_color;
						}
					}
					if (clr_used.Length() > 0x100) {
						bool has_transparent = false;
						for (int y = 0; y < Height; y++) for (int x = 0; x < Width; x++) {
							uint32 src_color = plt ? Palette[GetPixel(x, y)] :
								ConvertPixel(GetPixel(x, y), Format, Alpha, PixelFormat::B8G8R8A8, AlphaFormat::Normal);
							if ((src_color & 0xFF000000) != 0xFF000000) has_transparent = true;
						}
						if (has_transparent) New->Palette << 0x00000000;
						for (int r = 0; r < 8; r++) for (int g = 0; g < 8; g++) for (int b = 0; b < 4; b++) {
							if (has_transparent && r == 4 && g == 4 && b == 1) continue;
							New->Palette << (0xFF000000 | (b * 255 / 3) | ((g * 255 / 7) << 8) | ((r * 255 / 7) << 16));
						}
					} else New->Palette = clr_used;
				} else if (New->Palette.Length() == 16) {
					New->Palette.Clear();
					New->Palette << 0xFF000000;
					New->Palette << 0xFF000080;
					New->Palette << 0xFF008000;
					New->Palette << 0xFF008080;
					New->Palette << 0xFF800000;
					New->Palette << 0xFF800080;
					New->Palette << 0xFF808000;
					New->Palette << 0xFFC0C0C0;
					New->Palette << 0xFF808080;
					New->Palette << 0xFF0000FF;
					New->Palette << 0xFF00FF00;
					New->Palette << 0xFF00FFFF;
					New->Palette << 0xFFFF0000;
					New->Palette << 0xFFFF00FF;
					New->Palette << 0xFFFFFF00;
					New->Palette << 0xFFFFFFFF;
				} else if (New->Palette.Length() == 4) {
					New->Palette.Clear();
					New->Palette << 0xFF000000;
					New->Palette << 0xFF808080;
					New->Palette << 0xFFC0C0C0;
					New->Palette << 0xFFFFFFFF;
				} else if (New->Palette.Length() == 2) {
					New->Palette.Clear();
					New->Palette << 0xFF000000;
					New->Palette << 0xFFFFFFFF;
				}
				for (int y = 0; y < Height; y++) for (int x = 0; x < Width; x++) {
					uint32 src_color = plt ? Palette[GetPixel(x, y)] :
						ConvertPixel(GetPixel(x, y), Format, Alpha, PixelFormat::B8G8R8A8, AlphaFormat::Normal);
					uint32 index = New->GetBestPaletteIndex(src_color);
					New->SetPixel(x, y, index);
				}
			}
			New->DpiUsage = DpiUsage;
			New->Duration = Duration;
			New->HotPointX = HotPointX;
			New->HotPointY = HotPointY;
			New->Usage = Usage;
			New->Retain();
			return New;
		}

		Image::Image(void) : Frames(0x10) {}
		Image::~Image(void) {}
		Frame * Image::GetFrameBestSizeFit(int32 w, int32 h) const
		{
			if (!Frames.Length()) return 0;
			int best_i = -1;
			double min_dist = 0.0;
			for (int i = 0; i < Frames.Length(); i++) {
				double dist = sqrt(pow(double(w - Frames[i].GetWidth()), 2.0) + pow(double(h - Frames[i].GetHeight()), 2.0));
				if (dist < min_dist || best_i == -1) {
					best_i = i;
					min_dist = dist;
				}
			}
			return (best_i >= 0) ? Frames.ElementAt(best_i) : 0;
		}
		Frame * Image::GetFrameBestDpiFit(double dpi) const
		{
			if (!Frames.Length()) return 0;
			int best_i = -1;
			double min_dist = 0.0;
			for (int i = 0; i < Frames.Length(); i++) {
				double dist = fabs(dpi - Frames[i].DpiUsage);
				if (dist < min_dist || best_i == -1) {
					best_i = i;
					min_dist = dist;
				}
			}
			return (best_i >= 0) ? Frames.ElementAt(best_i) : 0;
		}
		Frame * Image::GetFrameByUsage(FrameUsage usage) const
		{
			for (int i = 0; i < Frames.Length(); i++) if (Frames[i].Usage == usage) return Frames.ElementAt(i);
			return 0;
		}
		Frame * Image::GetFramePreciseSize(int32 w, int32 h) const
		{
			for (int i = 0; i < Frames.Length(); i++) if (Frames[i].GetWidth() == w && Frames[i].GetHeight() == h) return Frames.ElementAt(i);
			return 0;
		}

		ObjectArray<Codec> Codecs(0x10);

		Codec * FindCoder(const string & format)
		{
			for (int i = 0; i < Codecs.Length(); i++) if (Codecs[i].CanEncode(format)) return Codecs.ElementAt(i);
			return 0;
		}
		Codec * FindDecoder(const string & format)
		{
			for (int i = 0; i < Codecs.Length(); i++) if (Codecs[i].CanDecode(format)) return Codecs.ElementAt(i);
			return 0;
		}
		Codec::Codec(void) { Codecs.Append(this); }
		Codec::~Codec(void) {}
		void EncodeFrame(Streaming::Stream * stream, Frame * frame, const string & format)
		{
			for (int i = 0; i < Codecs.Length(); i++) {
				if (Codecs[i].CanEncode(format) && Codecs[i].IsFrameCodec()) Codecs[i].EncodeFrame(stream, frame, format);
			}
		}
		void EncodeImage(Streaming::Stream * stream, Image * image, const string & format)
		{
			for (int i = 0; i < Codecs.Length(); i++) {
				if (Codecs[i].CanEncode(format) && Codecs[i].IsImageCodec()) Codecs[i].EncodeImage(stream, image, format);
			}
		}
		Frame * DecodeFrame(Streaming::Stream * stream)
		{
			for (int i = 0; i < Codecs.Length(); i++) {
				if (Codecs[i].IsFrameCodec() && Codecs[i].ExamineData(stream).Length()) {
					return Codecs[i].DecodeFrame(stream);
				}
			}
			return 0;
		}
		Image * DecodeImage(Streaming::Stream * stream)
		{
			for (int i = 0; i < Codecs.Length(); i++) {
				if (Codecs[i].IsImageCodec() && Codecs[i].ExamineData(stream).Length()) {
					return Codecs[i].DecodeImage(stream);
				}
			}
			return 0;
		}
	}
}