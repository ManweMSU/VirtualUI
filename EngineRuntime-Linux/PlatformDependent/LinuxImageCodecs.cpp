#include "LinuxImageCodecs.h"
#include "../ImageCodec/CodecBase.h"

#include <libpng16/png.h>
#include <tiffio.h>
#include <jpeglib.h>
#include <gif_lib.h>

namespace Engine
{
	namespace Linux
	{
		class EngineBitmapCodec : public Codec::ICodec
		{
			ENGINE_PACKED_STRUCTURE(REFCOLOR_XYZ)
				uint32 x, y, z;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(BITMAPFILEHEADER)
				uint16 sign;
				uint32 file_size;
				uint16 reserved0, reserved1;
				uint32 pixels_offset;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(BITMAPINFOHEADER)
				uint32 struct_size;
				int32 width;
				int32 height;
				uint16 planes;
				uint16 bpp;
				uint32 compression;
				uint32 pixels_size;
				uint32 xppm;
				uint32 yppm;
				uint32 plt_colors_used;
				uint32 plt_colors_imp;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(BITMAPINFOHEADER_V4)
				BITMAPINFOHEADER base_header;
				uint32 mask_red;
				uint32 mask_green;
				uint32 mask_blue;
				uint32 mask_alpha;
				uint32 cs_type;
				REFCOLOR_XYZ ref_red;
				REFCOLOR_XYZ ref_green;
				REFCOLOR_XYZ ref_blue;
				uint32 gamma_red;
				uint32 gamma_green;
				uint32 gamma_blue;
			ENGINE_END_PACKED_STRUCTURE
			static uint32 _extract_channel(uint32 pixel_dword, uint32 mask)
			{
				if (!mask) return 0;
				pixel_dword &= mask;
				while (mask & 0xFFFFFF00) {
					pixel_dword >>= 1;
					mask >>= 1;
				}
				while (!(mask & 0x80)) {
					pixel_dword <<= 1;
					mask <<= 1;
				}
				while (mask != 0xFF) {
					uint32 test = 0x80;
					int repl = 0;
					while (mask & test) {
						test >>= 1;
						repl++;
					}
					pixel_dword |= pixel_dword >> repl;
					mask |= mask >> repl;
				}
				return pixel_dword;
			}
		public:
			EngineBitmapCodec(void) {}
			virtual ~EngineBitmapCodec(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Codec::Frame * frame, const string & format) override
			{
				if (!frame || format != Codec::ImageFormatDIB) throw InvalidArgumentException();
				auto pxf = frame->GetPixelFormat();
				if (pxf != Codec::PixelFormat::P1 && pxf != Codec::PixelFormat::P2 && pxf != Codec::PixelFormat::P4 && pxf != Codec::PixelFormat::P8 &&
					pxf != Codec::PixelFormat::B8G8R8 && pxf != Codec::PixelFormat::B8G8R8X8 && pxf != Codec::PixelFormat::B8G8R8A8) {
					auto bpp = Codec::GetBitsPerPixel(pxf);
					if (bpp > 25 || (uint32(frame->GetPixelFormat()) & 0x00000F00)) {
						if (uint32(frame->GetPixelFormat()) & 0x00000F00) pxf = Codec::PixelFormat::B8G8R8A8;
						else pxf = Codec::PixelFormat::B8G8R8X8;
					} else if (bpp > 16) pxf = Codec::PixelFormat::B8G8R8;
					else if (bpp > 8) pxf = Codec::PixelFormat::B5G5R5X1;
					else if (bpp > 4) pxf = Codec::PixelFormat::P8;
					else if (bpp > 2) pxf = Codec::PixelFormat::P4;
					else if (bpp > 1) pxf = Codec::PixelFormat::P2;
					else pxf = Codec::PixelFormat::P1;
				}
				SafePointer<Codec::Frame> conv = frame->ConvertFormat(pxf, Codec::AlphaMode::Straight, Codec::ScanOrigin::BottomUp);
				if (pxf == Codec::PixelFormat::P1 || pxf == Codec::PixelFormat::P2 || pxf == Codec::PixelFormat::P4 || pxf == Codec::PixelFormat::P8) {
					for (int i = 0; i < conv->GetPaletteVolume(); i++) conv->GetPalette()[i] &= 0xFFFFFF;
					int plt_size = 4 * conv->GetPaletteVolume();
					int plt_max = 1 << Codec::GetBitsPerPixel(pxf);
					BITMAPFILEHEADER fhdr;
					BITMAPINFOHEADER hdr;
					MemoryCopy(&fhdr.sign, "BM", 2);
					fhdr.reserved0 = fhdr.reserved1 = 0;
					fhdr.pixels_offset = sizeof(fhdr) + sizeof(hdr) + plt_size;
					fhdr.file_size = fhdr.pixels_offset + conv->GetHeight() * conv->GetScanLineLength();
					hdr.struct_size = sizeof(hdr);
					hdr.width = conv->GetWidth();
					hdr.height = conv->GetHeight();
					hdr.planes = 1;
					hdr.bpp = Codec::GetBitsPerPixel(pxf);
					hdr.compression = 0;
					hdr.pixels_size = conv->GetHeight() * conv->GetScanLineLength();
					hdr.xppm = hdr.yppm = hdr.plt_colors_imp = 0;
					hdr.plt_colors_used = (plt_size / 4 < plt_max) ? (plt_size / 4) : 0;
					stream->Write(&fhdr, sizeof(fhdr));
					stream->Write(&hdr, sizeof(hdr));
					stream->Write(conv->GetPalette(), plt_size);
					stream->Write(conv->GetData(), hdr.pixels_size);
				} else if (pxf == Codec::PixelFormat::B5G5R5X1 || pxf == Codec::PixelFormat::B8G8R8 || pxf == Codec::PixelFormat::B8G8R8X8) {
					BITMAPFILEHEADER fhdr;
					BITMAPINFOHEADER hdr;
					MemoryCopy(&fhdr.sign, "BM", 2);
					fhdr.reserved0 = fhdr.reserved1 = 0;
					fhdr.pixels_offset = sizeof(fhdr) + sizeof(hdr);
					fhdr.file_size = fhdr.pixels_offset + conv->GetHeight() * conv->GetScanLineLength();
					hdr.struct_size = sizeof(hdr);
					hdr.width = conv->GetWidth();
					hdr.height = conv->GetHeight();
					hdr.planes = 1;
					hdr.bpp = Codec::GetBitsPerPixel(pxf);
					hdr.compression = 0;
					hdr.pixels_size = conv->GetHeight() * conv->GetScanLineLength();
					hdr.xppm = hdr.yppm = hdr.plt_colors_used = hdr.plt_colors_imp = 0;
					stream->Write(&fhdr, sizeof(fhdr));
					stream->Write(&hdr, sizeof(hdr));
					stream->Write(conv->GetData(), hdr.pixels_size);
				} else if (pxf == Codec::PixelFormat::B8G8R8A8) {
					BITMAPFILEHEADER fhdr;
					BITMAPINFOHEADER_V4 hdr;
					ZeroMemory(&hdr, sizeof(hdr));
					MemoryCopy(&fhdr.sign, "BM", 2);
					fhdr.reserved0 = fhdr.reserved1 = 0;
					fhdr.pixels_offset = sizeof(fhdr) + sizeof(hdr) + 12;
					fhdr.file_size = fhdr.pixels_offset + conv->GetHeight() * conv->GetScanLineLength();
					hdr.base_header.struct_size = sizeof(hdr);
					hdr.base_header.width = conv->GetWidth();
					hdr.base_header.height = conv->GetHeight();
					hdr.base_header.planes = 1;
					hdr.base_header.bpp = Codec::GetBitsPerPixel(pxf);
					hdr.base_header.compression = 3;
					hdr.base_header.pixels_size = conv->GetHeight() * conv->GetScanLineLength();
					hdr.base_header.plt_colors_used = 3;
					hdr.mask_alpha = 0xFF000000;
					hdr.mask_red = 0x00FF0000;
					hdr.mask_green = 0x0000FF00;
					hdr.mask_blue = 0x000000FF;
					stream->Write(&fhdr, sizeof(fhdr));
					stream->Write(&hdr, sizeof(hdr));
					stream->Write(&hdr.mask_red, 12);
					stream->Write(conv->GetData(), hdr.base_header.pixels_size);
				} else throw InvalidFormatException();
			}
			virtual void EncodeImage(Streaming::Stream * stream, Codec::Image * image, const string & format) override
			{
				if (!image || !image->Frames.Length()) throw InvalidArgumentException();
				EncodeFrame(stream, image->Frames.FirstElement(), format);
			}
			virtual Codec::Frame * DecodeFrame(Streaming::Stream * stream) override
			{
				BITMAPFILEHEADER fhdr;
				stream->Read(&fhdr, sizeof(fhdr));
				if (fhdr.file_size < sizeof(fhdr)) throw InvalidFormatException();
				if (fhdr.pixels_offset < sizeof(fhdr)) throw InvalidFormatException();
				SafePointer<DataBlock> data = new DataBlock(1);
				data->SetLength(fhdr.file_size - sizeof(fhdr));
				stream->Read(data->GetBuffer(), data->Length());
				if (data->Length() < sizeof(BITMAPINFOHEADER)) throw InvalidFormatException();
				BITMAPINFOHEADER * hdr = reinterpret_cast<BITMAPINFOHEADER *>(data->GetBuffer());
				uint32 pixels_offs = fhdr.pixels_offset - sizeof(fhdr);
				uint32 palette_offs = hdr->struct_size;
				if (hdr->struct_size < sizeof(BITMAPINFOHEADER)) throw InvalidFormatException();
				if (hdr->compression == 3) {
					uint32 r_mask = 0, g_mask = 0, b_mask = 0, a_mask = 0;
					if (hdr->struct_size >= sizeof(BITMAPINFOHEADER_V4)) {
						if (hdr->struct_size > data->Length()) throw InvalidFormatException();
						BITMAPINFOHEADER_V4 * hdr_v4 = reinterpret_cast<BITMAPINFOHEADER_V4 *>(data->GetBuffer());
						r_mask = hdr_v4->mask_red;
						g_mask = hdr_v4->mask_green;
						b_mask = hdr_v4->mask_blue;
						a_mask = hdr_v4->mask_alpha;
					} else {
						if (palette_offs + 12 > data->Length()) throw InvalidFormatException();
						r_mask = *reinterpret_cast<uint32 *>(data->GetBuffer() + palette_offs + 0);
						g_mask = *reinterpret_cast<uint32 *>(data->GetBuffer() + palette_offs + 4);
						b_mask = *reinterpret_cast<uint32 *>(data->GetBuffer() + palette_offs + 8);
					}
					if (hdr->bpp != 32 && hdr->bpp != 16) throw InvalidFormatException();
					uint32 width = hdr->width;
					uint32 height = hdr->height > 0 ? hdr->height : -hdr->height;
					Codec::ScanOrigin org = hdr->height > 0 ? Codec::ScanOrigin::BottomUp : Codec::ScanOrigin::TopDown;
					Codec::PixelFormat pxf;
					uint32 src_stride = width * hdr->bpp / 8;
					if (src_stride & 3) src_stride += 2;
					if (a_mask) pxf = Codec::PixelFormat::B8G8R8A8;
					else if (hdr->bpp == 32) pxf = Codec::PixelFormat::B8G8R8X8;
					else pxf = Codec::PixelFormat::B5G5R5X1;
					if (!width || !height) throw InvalidFormatException();
					if (hdr->planes != 1) throw InvalidFormatException();
					if (!hdr->pixels_size) hdr->pixels_size = src_stride * height;
					if (hdr->pixels_size != src_stride * height) throw InvalidFormatException();
					if (hdr->pixels_size + pixels_offs > data->Length()) throw InvalidFormatException();
					SafePointer<Codec::Frame> surface = new Codec::Frame(width, height, pxf, Codec::AlphaMode::Straight, org);
					for (int y = 0; y < height; y++) for (int x = 0; x < width; x++) {
						uint32 pixel;
						if (hdr->bpp == 32) pixel = *reinterpret_cast<uint32 *>(data->GetBuffer() + pixels_offs + x * 4 + y * src_stride);
						else pixel = *reinterpret_cast<uint16 *>(data->GetBuffer() + pixels_offs + x * 2 + y * src_stride);
						auto r = _extract_channel(pixel, r_mask);
						auto g = _extract_channel(pixel, g_mask);
						auto b = _extract_channel(pixel, b_mask);
						auto a = _extract_channel(pixel, a_mask);
						if (pxf == Codec::PixelFormat::B5G5R5X1) {
							auto & pixel = *reinterpret_cast<uint16 *>(surface->GetData() + x * 2 + y * surface->GetScanLineLength());
							pixel = ((r >> 3) << 10) | ((g >> 3) << 5) | (b >> 3);
						} else {
							auto & pixel = *reinterpret_cast<uint32 *>(surface->GetData() + x * 4 + y * surface->GetScanLineLength());
							pixel = (a << 24) | (r << 16) | (g << 8) | b;
						}
					}
					surface->Retain();
					return surface;
				} else if (hdr->compression == 0 || hdr->compression == 1 || hdr->compression == 2) {
					uint32 width = hdr->width;
					uint32 height = hdr->height > 0 ? hdr->height : -hdr->height;
					Codec::ScanOrigin org = hdr->height > 0 ? Codec::ScanOrigin::BottomUp : Codec::ScanOrigin::TopDown;
					Codec::PixelFormat pxf;
					if (!width || !height) throw InvalidFormatException();
					if (hdr->bpp == 32) pxf = Codec::PixelFormat::B8G8R8X8;
					else if (hdr->bpp == 24) pxf = Codec::PixelFormat::B8G8R8;
					else if (hdr->bpp == 16) pxf = Codec::PixelFormat::B5G5R5X1;
					else if (hdr->bpp == 8) pxf = Codec::PixelFormat::P8;
					else if (hdr->bpp == 4) pxf = Codec::PixelFormat::P4;
					else if (hdr->bpp == 2) pxf = Codec::PixelFormat::P2;
					else if (hdr->bpp == 1) pxf = Codec::PixelFormat::P1;
					else throw InvalidFormatException();
					if (hdr->planes != 1) throw InvalidFormatException();
					SafePointer<Codec::Frame> surface = new Codec::Frame(width, height, pxf, Codec::AlphaMode::Straight, org);
					if (Codec::IsPalettePixel(pxf)) {
						int plt_vol = 1 << hdr->bpp;
						if (hdr->plt_colors_used && hdr->plt_colors_used < plt_vol) plt_vol = hdr->plt_colors_used;
						if (palette_offs + plt_vol * 4 > data->Length())
						surface->SetPaletteVolume(plt_vol);
						for (int i = 0; i < plt_vol; i++) {
							auto quad = *reinterpret_cast<uint32 *>(data->GetBuffer() + palette_offs + 4 * i);
							auto & clr = surface->GetPalette()[i];
							clr = 0xFF000000 | quad;
						}
						if (hdr->compression == 2) {
							if (hdr->bpp != 4) throw InvalidFormatException();
							if (hdr->pixels_size + pixels_offs > data->Length()) throw InvalidFormatException();
							int x = 0, y = 0, rp = 0;
							auto bytes = data->GetBuffer() + pixels_offs;
							while (rp + 2 <= hdr->pixels_size) {
								if (y >= height) break;
								auto repcnt = bytes[rp];
								auto word = bytes[rp + 1];
								rp += 2;
								if (repcnt) {
									for (int i = 0; i < int(repcnt); i++) {
										if (x >= width) { x = 0; y++; }
										if (y < height) {
											if (i & 1) surface->SetPixel(x, height - 1 - y, word & 0xF);
											else surface->SetPixel(x, height - 1 - y, word >> 4);
										}
										x++;
									}
								} else {
									if (word == 0) {
										x = 0;
										y++;
									} else if (word == 1) {
										break;
									} else if (word == 2) {
										if (rp + 2 <= hdr->pixels_size) {
											int dx = bytes[rp];
											int dy = bytes[rp + 1];
											rp += 2;
											x += dx;
											y += dy;
										} else break;
									} else {
										for (int i = 0; i < int(word); i++) {
											if (rp < hdr->pixels_size) {
												uint8 px = bytes[rp];
												if (i & 1) px &= 0xF; else px >>= 4;
											 	if (x >= width) { x = 0; y++; }
											 	if (y < height) surface->SetPixel(x, height - 1 - y, px);
											 	x++;
											 	if (i & 1) rp++;
											} else break;
										}
										if (word & 1) rp++;
										if ((word & 3) > 0 && (word & 3) < 3) rp++;
									}
								}
							}
						} else if (hdr->compression == 1) {
							if (hdr->bpp != 8) throw InvalidFormatException();
							if (hdr->pixels_size + pixels_offs > data->Length()) throw InvalidFormatException();
							int x = 0, y = 0, rp = 0;
							auto bytes = data->GetBuffer() + pixels_offs;
							while (rp + 2 <= hdr->pixels_size) {
								if (y >= height) break;
								auto repcnt = bytes[rp];
								auto word = bytes[rp + 1];
								rp += 2;
								if (repcnt) {
									for (int i = 0; i < int(repcnt); i++) {
										if (x >= width) { x = 0; y++; }
										if (y < height) surface->SetPixel(x, height - 1 - y, word);
										x++;
									}
								} else {
									if (word == 0) {
										x = 0;
										y++;
									} else if (word == 1) {
										break;
									} else if (word == 2) {
										if (rp + 2 <= hdr->pixels_size) {
											int dx = bytes[rp];
											int dy = bytes[rp + 1];
											rp += 2;
											x += dx;
											y += dy;
										} else break;
									} else {
										for (int i = 0; i < int(word); i++) {
											if (rp < hdr->pixels_size) {
												if (x >= width) { x = 0; y++; }
												if (y < height) surface->SetPixel(x, height - 1 - y, bytes[rp]);
												x++;
												rp++;
											} else break;
										}
										if (word & 1) rp++;
									}
								}
							}
						} else {
							if (!hdr->pixels_size) hdr->pixels_size = surface->GetScanLineLength() * surface->GetHeight();
							if (hdr->pixels_size != surface->GetScanLineLength() * surface->GetHeight()) throw InvalidFormatException();
							if (hdr->pixels_size + pixels_offs > data->Length()) throw InvalidFormatException();
							MemoryCopy(surface->GetData(), data->GetBuffer() + pixels_offs, surface->GetScanLineLength() * surface->GetHeight());
						}
					} else {
						if (!hdr->pixels_size) hdr->pixels_size = surface->GetScanLineLength() * surface->GetHeight();
						if (hdr->pixels_size != surface->GetScanLineLength() * surface->GetHeight()) throw InvalidFormatException();
						if (hdr->pixels_size + pixels_offs > data->Length()) throw InvalidFormatException();
						MemoryCopy(surface->GetData(), data->GetBuffer() + pixels_offs, surface->GetScanLineLength() * surface->GetHeight());
					}
					surface->Retain();
					return surface;
				} else throw InvalidFormatException();
			}
			virtual Codec::Image * DecodeImage(Streaming::Stream * stream) override
			{
				SafePointer<Codec::Frame> frame = DecodeFrame(stream);
				if (!frame) return 0;
				SafePointer<Codec::Image> image = new Codec::Image;
				image->Frames.Append(frame);
				image->Retain();
				return image;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string GetCodecName(void) override { return L"Engine Bitmap Codec"; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				try {
					uint16 sign = 0;
					stream->Seek(0, Streaming::Begin);
					stream->Read(&sign, 2);
					stream->Seek(0, Streaming::Begin);
					if (sign == 0x4D42) return Codec::ImageFormatDIB; else return L"";
				} catch (...) { return L""; }
			}
			virtual bool CanEncode(const string & format) override { return format == Codec::ImageFormatDIB; }
			virtual bool CanDecode(const string & format) override { return format == Codec::ImageFormatDIB; }
		};
		class LibpngCodec : public Codec::ICodec
		{
			static void _stream_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(png_get_io_ptr(png_ptr));
					stream->Read(data, length);
				} catch (...) { png_error(png_ptr, ""); }
			}
			static void _stream_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(png_get_io_ptr(png_ptr));
					stream->Write(data, length);
				} catch (...) { png_error(png_ptr, ""); }
			}
			static void _stream_flush_data(png_structp png_ptr)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(png_get_io_ptr(png_ptr));
					stream->Flush();
				} catch (...) { png_error(png_ptr, ""); }
			}
			static void _png_error_callback(png_structp png_ptr, png_const_charp error) {}
			static void _png_warning_callback(png_structp png_ptr, png_const_charp error) {}
		public:
			LibpngCodec(void) {}
			virtual ~LibpngCodec(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Codec::Frame * frame, const string & format) override
			{
				if (!frame || format != Codec::ImageFormatPNG) throw InvalidArgumentException();
				SafePointer<Codec::Frame> converted;
				Array<uint8 *> scanlines(0x100);
				Codec::PixelFormat pixel_format;
				int png_color_type;
				int png_transform = PNG_TRANSFORM_IDENTITY;
				if (uint32(frame->GetPixelFormat()) & 0x00000F00) {
					png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
					pixel_format = Codec::PixelFormat::R8G8B8A8;
				} else {
					png_color_type = PNG_COLOR_TYPE_RGB;
					pixel_format = Codec::PixelFormat::R8G8B8;
				}
				if (frame->GetPixelFormat() == pixel_format && frame->GetAlphaMode() == Codec::AlphaMode::Straight) {
					converted.SetRetain(frame);
				} else {
					converted = frame->ConvertFormat(pixel_format, Codec::AlphaMode::Straight);
				}
				scanlines.SetLength(converted->GetHeight());
				int stride = converted->GetScanLineLength();
				if (converted->GetScanOrigin() == Codec::ScanOrigin::TopDown) {
					for (int i = 0; i < scanlines.Length(); i++) {
						scanlines[i] = converted->GetData() + (stride * i);
					}
				} else {
					for (int i = 0; i < scanlines.Length(); i++) {
						scanlines[i] = converted->GetData() + (stride * (scanlines.Length() - i - 1));
					}
				}
				png_structp session = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, _png_error_callback, _png_warning_callback);
				if (!session) throw OutOfMemoryException();
				png_infop info = png_create_info_struct(session);
				if (!info) {
					png_destroy_write_struct(&session, 0);
					throw OutOfMemoryException();
				}
				if (setjmp(png_jmpbuf(session))) {
					png_destroy_write_struct(&session, &info);
					throw Exception();
				}
				png_set_write_fn(session, stream, _stream_write_data, _stream_flush_data);
				png_set_IHDR(session, info, frame->GetWidth(), frame->GetHeight(), 8, png_color_type, PNG_INTERLACE_NONE,
					PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
				png_set_rows(session, info, scanlines.GetBuffer());
				png_write_png(session, info, png_transform, 0);
				png_destroy_write_struct(&session, &info);
			}
			virtual void EncodeImage(Streaming::Stream * stream, Codec::Image * image, const string & format) override
			{
				if (!image || !image->Frames.Length()) throw InvalidArgumentException();
				EncodeFrame(stream, image->Frames.FirstElement(), format);
			}
			virtual Codec::Frame * DecodeFrame(Streaming::Stream * stream) override
			{
				stream->Seek(0, Streaming::Begin);
				SafePointer<Codec::Frame> framebuffer;
				png_structp session = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, _png_error_callback, _png_warning_callback);
				if (!session) throw OutOfMemoryException();
				png_infop info = png_create_info_struct(session);
				if (!info) {
					png_destroy_read_struct(&session, 0, 0);
					throw OutOfMemoryException();
				}
				png_infop end = png_create_info_struct(session);
				if (!end) {
					png_destroy_read_struct(&session, &info, 0);
					throw OutOfMemoryException();
				}
				if (setjmp(png_jmpbuf(session))) {
					png_destroy_read_struct(&session, &info, &end);
					throw InvalidFormatException();
				}
				png_set_read_fn(session, stream, _stream_read_data);
				png_read_png(session, info, PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_GRAY_TO_RGB, 0);
				auto rows = png_get_rows(session, info);
				auto width = png_get_image_width(session, info);
				auto height = png_get_image_height(session, info);
				auto color_type = png_get_color_type(session, info);
				auto bps = png_get_bit_depth(session, info);
				int valid_scan;
				try {
					if (bps == 8 && color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
						framebuffer = new Codec::Frame(width, height, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
						valid_scan = 4 * width;
					} else if (bps == 8 && color_type == PNG_COLOR_TYPE_RGB) {
						framebuffer = new Codec::Frame(width, height, Codec::PixelFormat::R8G8B8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
						valid_scan = 3 * width;
					} else throw InvalidFormatException();
				} catch (...) {
					png_destroy_read_struct(&session, &info, &end);
					throw;
				}
				int stride = framebuffer->GetScanLineLength();
				for (int y = 0; y < height; y++) MemoryCopy(framebuffer->GetData() + (y * stride), rows[y], valid_scan);
				png_destroy_read_struct(&session, &info, &end);
				framebuffer->Retain();
				return framebuffer;
			}
			virtual Codec::Image * DecodeImage(Streaming::Stream * stream) override
			{
				SafePointer<Codec::Frame> frame = DecodeFrame(stream);
				if (!frame) return 0;
				SafePointer<Codec::Image> image = new Codec::Image;
				image->Frames.Append(frame);
				image->Retain();
				return image;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string GetCodecName(void) override { return L"libpng Codec"; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				try {
					uint8 sign[8];
					stream->Seek(0, Streaming::Begin);
					stream->Read(&sign, 8);
					stream->Seek(0, Streaming::Begin);
					if (!png_sig_cmp(sign, 0, 8)) return Codec::ImageFormatPNG; else return L"";
				} catch (...) { return L""; }
			}
			virtual bool CanEncode(const string & format) override { return format == Codec::ImageFormatPNG; }
			virtual bool CanDecode(const string & format) override { return format == Codec::ImageFormatPNG; }
		};
		class LibjpegCodec : public Codec::ICodec
		{
			struct _error_handler {
				jpeg_error_mgr mgr;
				jmp_buf jump;

				static void _handle_error(j_common_ptr cinfo)
				{
					auto hdlr = reinterpret_cast<_error_handler *>(cinfo->err);
					longjmp(hdlr->jump, 1);
				}
				static void _handle_zero(j_common_ptr cinfo) {}
			};
		public:
			LibjpegCodec(void) {}
			virtual ~LibjpegCodec(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Codec::Frame * frame, const string & format) override
			{
				if (!frame || format != Codec::ImageFormatJPEG) throw InvalidArgumentException();
				SafePointer<Codec::Frame> converted;
				uint8 * output_data = 0;
				unsigned long output_data_size = 0;
				if (frame->GetPixelFormat() == Codec::PixelFormat::R8G8B8 && frame->GetScanOrigin() == Codec::ScanOrigin::TopDown) {
					converted.SetRetain(frame);
				} else {
					converted = frame->ConvertFormat(Codec::PixelFormat::R8G8B8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
				}
				_error_handler hdlr;
				jpeg_compress_struct com;
				com.err = jpeg_std_error(&hdlr.mgr);
				hdlr.mgr.error_exit = _error_handler::_handle_error;
				hdlr.mgr.output_message = _error_handler::_handle_zero;
				if (setjmp(hdlr.jump)) {
					jpeg_destroy_compress(&com);
					if (output_data) free(output_data);
					throw Exception();
				}
				jpeg_create_compress(&com);
				jpeg_mem_dest(&com, &output_data, &output_data_size);
				com.image_width = converted->GetWidth();
				com.image_height = converted->GetHeight();
				com.input_components = 3;
				com.in_color_space = JCS_RGB;
				jpeg_set_defaults(&com);
				jpeg_start_compress(&com, TRUE);
				for (int i = 0; i < converted->GetHeight(); i++) {
					JSAMPROW org = converted->GetData() + i * converted->GetScanLineLength();
					jpeg_write_scanlines(&com, &org, 1);
				}
				jpeg_finish_compress(&com);
				jpeg_destroy_compress(&com);
				if (output_data) {
					try {
						stream->Write(output_data, output_data_size);
					} catch (...) {
						free(output_data);
						throw;
					}
					free(output_data);
				}
			}
			virtual void EncodeImage(Streaming::Stream * stream, Codec::Image * image, const string & format) override
			{
				if (!image || !image->Frames.Length()) throw InvalidArgumentException();
				EncodeFrame(stream, image->Frames.FirstElement(), format);
			}
			virtual Codec::Frame * DecodeFrame(Streaming::Stream * stream) override
			{
				SafePointer<DataBlock> data = stream->ReadAll();
				SafePointer<Codec::Frame> framebuffer;
				_error_handler hdlr;
				jpeg_decompress_struct decom;
				decom.err = jpeg_std_error(&hdlr.mgr);
				hdlr.mgr.error_exit = _error_handler::_handle_error;
				hdlr.mgr.output_message = _error_handler::_handle_zero;
				if (setjmp(hdlr.jump)) {
					jpeg_destroy_decompress(&decom);
					throw InvalidFormatException();
				}
				jpeg_create_decompress(&decom);
				jpeg_mem_src(&decom, data->GetBuffer(), data->Length());
				jpeg_read_header(&decom, TRUE);
				decom.out_color_space = JCS_RGB;
				decom.quantize_colors = FALSE;
				jpeg_start_decompress(&decom);
				int scanline, stride, rows_read;
				try {
					if (decom.output_components == 1) {
						scanline = decom.output_width;
						framebuffer = new Codec::Frame(decom.output_width, decom.output_height, Codec::PixelFormat::R8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
					} else if (decom.output_components == 3) {
						scanline = decom.output_width * 3;
						framebuffer = new Codec::Frame(decom.output_width, decom.output_height, Codec::PixelFormat::R8G8B8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
					} else throw InvalidFormatException();
					stride = framebuffer->GetScanLineLength();
					rows_read = 0;
				} catch (...) {
					jpeg_destroy_decompress(&decom);
					throw;
				}
				while (rows_read < framebuffer->GetHeight()) {
					JSAMPROW org = framebuffer->GetData() + stride * rows_read;
					auto read_now = jpeg_read_scanlines(&decom, &org, 1);
					rows_read += read_now;
				}
				jpeg_finish_decompress(&decom);
				jpeg_destroy_decompress(&decom);
				framebuffer->Retain();
				return framebuffer;
			}
			virtual Codec::Image * DecodeImage(Streaming::Stream * stream) override
			{
				SafePointer<Codec::Frame> frame = DecodeFrame(stream);
				if (!frame) return 0;
				SafePointer<Codec::Image> image = new Codec::Image;
				image->Frames.Append(frame);
				image->Retain();
				return image;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string GetCodecName(void) override { return L"libjpeg Codec"; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				try {
					uint32 sign = 0;
					stream->Seek(0, Streaming::Begin);
					stream->Read(&sign, 3);
					stream->Seek(0, Streaming::Begin);
					if (sign == 0xFFD8FF) return Codec::ImageFormatJPEG; else return L"";
				} catch (...) { return L""; }
			}
			virtual bool CanEncode(const string & format) override { return format == Codec::ImageFormatJPEG; }
			virtual bool CanDecode(const string & format) override { return format == Codec::ImageFormatJPEG; }
		};
		class LibtiffCodec : public Codec::ICodec
		{
			static tsize_t _stream_read(thandle_t user, tdata_t pdata, tsize_t size)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(user);
					stream->Read(pdata, size);
					return size;
				} catch (IO::FileReadEndOfFileException & e) {
					return e.DataRead;
				} catch (...) {
					return -1;
				}
			}
			static tsize_t _stream_write(thandle_t user, tdata_t pdata, tsize_t size)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(user);
					stream->Write(pdata, size);
					return size;
				} catch (...) {
					return -1;
				}
			}
			static toff_t _stream_seek(thandle_t user, toff_t offset, int method)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(user);
					if (method == SEEK_SET) return stream->Seek(offset, Streaming::Begin);
					else if (method == SEEK_CUR) return stream->Seek(offset, Streaming::Current);
					else if (method == SEEK_END) return stream->Seek(offset, Streaming::End);
					else return -1;
				} catch (...) {
					return -1;
				}
			}
			static int _stream_close(thandle_t user) { return 0; }
			static toff_t _stream_length(thandle_t user)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(user);
					return stream->Length();
				} catch (...) {
					return -1;
				}
			}
			static int _stream_map(thandle_t user, tdata_t * ppdata, toff_t * plength)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(user);
					auto size = stream->Length();
					auto mem = malloc(size);
					if (!mem) return -1;
					try {
						auto org = stream->Seek(0, Streaming::Current);
						stream->Seek(0, Streaming::Begin);
						stream->Read(mem, size);
						stream->Seek(org, Streaming::Begin);
					} catch (...) { free(mem); return -1; }
					*ppdata = mem;
					*plength = size;
					return 0;
				} catch (...) {
					return -1;
				}
			}
			static void _stream_unmap(thandle_t user, tdata_t pdata, toff_t length) { free(pdata); }
		public:
			LibtiffCodec(void) {}
			virtual ~LibtiffCodec(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Codec::Frame * frame, const string & format) override
			{
				if (!frame) throw InvalidArgumentException();
				SafePointer<Codec::Image> image = new Codec::Image;
				image->Frames.Append(frame);
				EncodeImage(stream, image, format);
			}
			virtual void EncodeImage(Streaming::Stream * stream, Codec::Image * image, const string & format) override
			{
				if (!image || !image->Frames.Length() || format != Codec::ImageFormatTIFF) throw InvalidArgumentException();
				TIFFSetErrorHandler(0);
				TIFFSetWarningHandler(0);
				auto tiff = TIFFClientOpen("", "w", stream, _stream_read, _stream_write, _stream_seek, _stream_close, _stream_length, _stream_map, _stream_unmap);
				if (tiff) {
					try {
						int npage = 1;
						for (auto & frame : image->Frames) {
							int num_channels;
							int alpha_format = EXTRASAMPLE_ASSOCALPHA;
							SafePointer<Codec::Frame> conv;
							Codec::PixelFormat pixel_format;
							if (uint32(frame.GetPixelFormat()) & 0x00000F00) {
								num_channels = 4;
								pixel_format = Codec::PixelFormat::R8G8B8A8;
							} else {
								num_channels = 3;
								pixel_format = Codec::PixelFormat::R8G8B8;
							}
							if (frame.GetPixelFormat() == pixel_format && frame.GetAlphaMode() == Codec::AlphaMode::Straight && frame.GetScanOrigin() == Codec::ScanOrigin::TopDown) {
								conv.SetRetain(&frame);
							} else {
								conv = frame.ConvertFormat(pixel_format, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
							}
							if (!TIFFSetField(tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, frame.GetWidth())) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, frame.GetHeight())) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_LZW)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, num_channels)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_PAGENUMBER, npage)) throw Exception();
							if (num_channels > 3) if (!TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &alpha_format)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT)) throw Exception();
							if (!TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) throw Exception();
							for (int i = 0; i < conv->GetHeight(); i++) {
								if (TIFFWriteScanline(tiff, conv->GetData() + i * conv->GetScanLineLength(), i) != 1) throw Exception();
							}
							if (!TIFFWriteDirectory(tiff)) throw Exception();
							npage++;
						}
					} catch (...) {
						TIFFClose(tiff);
						throw;
					}
					TIFFClose(tiff);
				} else throw OutOfMemoryException();
			}
			virtual Codec::Frame * DecodeFrame(Streaming::Stream * stream) override
			{
				SafePointer<Codec::Image> image = DecodeImage(stream);
				if (!image || !image->Frames.Length()) return 0;
				SafePointer<Codec::Frame> frame;
				frame.SetRetain(image->Frames.FirstElement());
				frame->Retain();
				return frame;
			}
			virtual Codec::Image * DecodeImage(Streaming::Stream * stream) override
			{
				TIFFSetErrorHandler(0);
				TIFFSetWarningHandler(0);
				auto tiff = TIFFClientOpen("", "r", stream, _stream_read, _stream_write, _stream_seek, _stream_close, _stream_length, _stream_map, _stream_unmap);
				if (tiff) {
					SafePointer<Codec::Image> result;
					try {
						result = new Codec::Image;
						do {
							uint32 width, height;
							TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
							TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
							SafePointer<Codec::Frame> frame = new Codec::Frame(width, height, 4 * width, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Straight, Codec::ScanOrigin::BottomUp);
							if (TIFFReadRGBAImage(tiff, width, height, reinterpret_cast<uint32 *>(frame->GetData()))) result->Frames.Append(frame);
						} while (TIFFReadDirectory(tiff));
					} catch (...) {
						TIFFClose(tiff);
						throw;
					}
					TIFFClose(tiff);
					if (!result || !result->Frames.Length()) throw InvalidFormatException();
					result->Retain();
					return result;
				} else throw InvalidFormatException();
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string GetCodecName(void) override { return L"libtiff Codec"; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				try {
					uint32 sign = 0;
					stream->Seek(0, Streaming::Begin);
					stream->Read(&sign, 4);
					stream->Seek(0, Streaming::Begin);
					if (sign == 0x2A004D4D || sign == 0x002A4949) return Codec::ImageFormatTIFF; else return L"";
				} catch (...) { return L""; }
			}
			virtual bool CanEncode(const string & format) override { return format == Codec::ImageFormatTIFF; }
			virtual bool CanDecode(const string & format) override { return format == Codec::ImageFormatTIFF; }
		};
		class LibgifCodec : public Codec::ICodec
		{
			static int _stream_input(GifFileType * file, GifByteType * data, int size)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(file->UserData);
					stream->Read(data, size);
					return size;
				} catch (IO::FileReadEndOfFileException & e) {
					return e.DataRead;
				} catch (...) {
					return -1;
				}
			}
			static int _stream_output(GifFileType * file, const GifByteType * data, int size)
			{
				try {
					auto stream = reinterpret_cast<Streaming::Stream *>(file->UserData);
					stream->Write(data, size);
					return size;
				} catch (...) {
					return -1;
				}
			}
		public:
			LibgifCodec(void) {}
			virtual ~LibgifCodec(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Codec::Frame * frame, const string & format) override
			{
				if (!frame) throw InvalidArgumentException();
				SafePointer<Codec::Image> image = new Codec::Image;
				image->Frames.Append(frame);
				EncodeImage(stream, image, format);
			}
			virtual void EncodeImage(Streaming::Stream * stream, Codec::Image * image, const string & format) override
			{
				if (!image || !image->Frames.Length() || format != Codec::ImageFormatGIF) throw InvalidArgumentException();
				auto gif = EGifOpen(stream, _stream_output, 0);
				if (!gif) throw OutOfMemoryException();
				auto & f0 = image->Frames[0];
				if (EGifPutScreenDesc(gif, f0.GetWidth(), f0.GetHeight(), 8, 0, 0) != GIF_OK) {
					EGifCloseFile(gif, 0);
					throw Exception();
				}
				try {
					Array<GifColorType> plt(0x100);
					plt.SetLength(0x100);
					ZeroMemory(plt.GetBuffer(), plt.Length() * sizeof(GifColorType));
					ColorMapObject color_map;
					color_map.SortFlag = false;
					color_map.Colors = plt.GetBuffer();
					GraphicsControlBlock gc_block;
					gc_block.DisposalMode = DISPOSE_BACKGROUND;
					gc_block.UserInputFlag = false;
					GifByteType gc_block_data[0x100];
					for (auto & f : image->Frames) {
						if (f.GetWidth() != f0.GetWidth() || f.GetHeight() != f0.GetHeight()) continue;
						SafePointer<Codec::Frame> conv = f.ConvertFormat(Codec::PixelFormat::P8, Codec::ScanOrigin::TopDown);
						int color_count = conv->GetPaletteVolume();
						int transparent = -1;
						for (int i = 0; i < color_count; i++) {
							auto pc = conv->ReadPalette(i);
							if ((pc >> 24) < 128) {
								transparent = i;
								plt[i].Red = plt[i].Green = plt[i].Blue = 0;
							} else {
								plt[i].Red = (pc & 0xFF);
								plt[i].Green = (pc & 0xFF00) >> 8;
								plt[i].Blue = (pc & 0xFF0000) >> 16;
							}
						}
						color_map.ColorCount = color_count;
						color_map.BitsPerPixel = 0;
						while (color_count > (1 << color_map.BitsPerPixel)) color_map.BitsPerPixel++;
						gc_block.TransparentColor = transparent;
						gc_block.DelayTime = f.Duration / 10;
						int gc_size = EGifGCBToExtension(&gc_block, gc_block_data);
						if (EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, gc_size, gc_block_data) != GIF_OK) throw Exception();
						if (EGifPutImageDesc(gif, 0, 0, f0.GetWidth(), f0.GetHeight(), false, &color_map) != GIF_OK) throw Exception();
						for (int i = 0; i < conv->GetHeight(); i++) {
							if (EGifPutLine(gif, conv->GetData() + i * conv->GetScanLineLength(), conv->GetWidth()) != GIF_OK) throw Exception();
						}
					}
				} catch (...) {
					EGifCloseFile(gif, 0);
					throw Exception();
				}
				if (EGifCloseFile(gif, 0) != GIF_OK) throw Exception();
			}
			virtual Codec::Frame * DecodeFrame(Streaming::Stream * stream) override
			{
				SafePointer<Codec::Image> image = DecodeImage(stream);
				if (!image || !image->Frames.Length()) return 0;
				SafePointer<Codec::Frame> frame;
				frame.SetRetain(image->Frames.FirstElement());
				frame->Retain();
				return frame;
			}
			virtual Codec::Image * DecodeImage(Streaming::Stream * stream) override
			{
				SafePointer<Codec::Image> result = new Codec::Image;
				auto gif = DGifOpen(stream, _stream_input, 0);
				if (!gif) throw OutOfMemoryException();
				try {
					if (DGifSlurp(gif) != GIF_OK) throw InvalidFormatException();
					Array<uint32> plt_global(0x100), plt_local(0x100);
					plt_global.SetLength(0x100);
					plt_local.SetLength(0x100);
					ZeroMemory(plt_global.GetBuffer(), 0x400);
					ZeroMemory(plt_local.GetBuffer(), 0x400);
					if (gif->SColorMap) {
						for (int i = 0; i < gif->SColorMap->ColorCount; i++) {
							uint32 clr = 0xFF000000;
							clr |= int(gif->SColorMap->Colors[i].Blue);
							clr |= int(gif->SColorMap->Colors[i].Green) << 8;
							clr |= int(gif->SColorMap->Colors[i].Red) << 16;
							plt_global[i] = clr;
						}
						if (gif->SBackGroundColor >= 0 && gif->SBackGroundColor < 0x100) plt_global[gif->SBackGroundColor] = 0;
					}
					SafePointer<Codec::Frame> prev_frame;
					int image_blt_mode = 0;
					for (int i = 0; i < gif->ImageCount; i++) {
						auto & img = gif->SavedImages[i];
						int local_transparent = -1;
						int image_next_blt_mode = 0;
						int animation_time = 0;
						bool use_local_plt = false;
						for (int j = 0; j < img.ExtensionBlockCount; j++) {
							auto & ext = img.ExtensionBlocks[j];
							if (ext.Function == GRAPHICS_EXT_FUNC_CODE) {
								GraphicsControlBlock control;
								DGifExtensionToGCB(ext.ByteCount, ext.Bytes, &control);
								local_transparent = control.TransparentColor;
								image_next_blt_mode = (control.DisposalMode < 2) ? 1 : 0;
								animation_time = control.DelayTime * 10;
								break;
							}
						}
						if (img.ImageDesc.ColorMap) {
							for (int j = 0; j < img.ImageDesc.ColorMap->ColorCount; j++) {
								uint32 clr = 0xFF000000;
								clr |= int(img.ImageDesc.ColorMap->Colors[j].Blue);
								clr |= int(img.ImageDesc.ColorMap->Colors[j].Green) << 8;
								clr |= int(img.ImageDesc.ColorMap->Colors[j].Red) << 16;
								plt_local[j] = clr;
							}
							if (local_transparent >= 0 && local_transparent < 0x100) plt_local[local_transparent] = 0;
							use_local_plt = true;
						} else use_local_plt = false;
						SafePointer<Codec::Frame> current = new Codec::Frame(img.ImageDesc.Width, img.ImageDesc.Height, img.ImageDesc.Width, Codec::PixelFormat::P8, Codec::ScanOrigin::TopDown);
						current->SetPaletteVolume(0x100);
						MemoryCopy(current->GetData(), img.RasterBits, img.ImageDesc.Width * img.ImageDesc.Height);
						MemoryCopy(current->GetPalette(), use_local_plt ? plt_local.GetBuffer() : plt_global.GetBuffer(), 0x400);
						if (image_blt_mode && prev_frame) {
							prev_frame->Duration = animation_time;
							for (int y = 0; y < prev_frame->GetHeight(); y++) for (int x = 0; x < prev_frame->GetWidth(); x++) {
								if (x >= img.ImageDesc.Left && y >= img.ImageDesc.Top && x < img.ImageDesc.Left + img.ImageDesc.Width && y < img.ImageDesc.Top + img.ImageDesc.Height) {
									auto src = current->ReadPixel(x - img.ImageDesc.Left, y - img.ImageDesc.Top);
									if (src & 0xFF000000) prev_frame->WritePixel(x, y, src);
								}
							}
							result->Frames.Append(prev_frame);
						} else if (img.ImageDesc.Width != gif->SWidth || img.ImageDesc.Height != gif->SHeight || img.ImageDesc.Top || img.ImageDesc.Left) {
							SafePointer<Codec::Frame> sum = new Codec::Frame(gif->SWidth, gif->SHeight, -1, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
							sum->Duration = animation_time;
							for (int y = 0; y < sum->GetHeight(); y++) for (int x = 0; x < sum->GetWidth(); x++) {
								if (x >= img.ImageDesc.Left && y >= img.ImageDesc.Top && x < img.ImageDesc.Left + img.ImageDesc.Width && y < img.ImageDesc.Top + img.ImageDesc.Height) {
									sum->WritePixel(x, y, current->ReadPixel(x - img.ImageDesc.Left, y - img.ImageDesc.Top));
								}
							}
							result->Frames.Append(sum);
						} else {
							current->Duration = animation_time;
							result->Frames.Append(current);
						}
						if (image_next_blt_mode) {
							prev_frame = result->Frames.LastElement()->ConvertFormat(Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
						} else prev_frame.SetReference(0);
						image_blt_mode = image_next_blt_mode;
					}
				} catch (...) {
					DGifCloseFile(gif, 0);
					throw Exception();
				}
				if (DGifCloseFile(gif, 0) != GIF_OK) throw Exception();
				result->Retain();
				return result;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return true; }
			virtual string GetCodecName(void) override { return L"libgif Codec"; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				try {
					uint64 sign = 0;
					stream->Seek(0, Streaming::Begin);
					stream->Read(&sign, 6);
					stream->Seek(0, Streaming::Begin);
					if (sign == 0x613938464947 || sign == 0x613738464947) return Codec::ImageFormatGIF; else return L"";
				} catch (...) { return L""; }
			}
			virtual bool CanEncode(const string & format) override { return format == Codec::ImageFormatGIF; }
			virtual bool CanDecode(const string & format) override { return format == Codec::ImageFormatGIF; }
		};

		EngineBitmapCodec * _bitmap_codec = 0;
		LibpngCodec * _libpng_codec = 0;
		LibjpegCodec * _libjpeg_codec = 0;
		LibtiffCodec * _libtiff_codec = 0;
		LibgifCodec * _libgif_codec = 0;

		void InitLinuxStandardCodecs(void)
		{
			if (!_bitmap_codec) {
				SafePointer<EngineBitmapCodec> codec = new EngineBitmapCodec;
				_bitmap_codec = codec.Inner();
			}
			if (!_libpng_codec) {
				SafePointer<LibpngCodec> codec = new LibpngCodec;
				_libpng_codec = codec.Inner();
			}
			if (!_libjpeg_codec) {
				SafePointer<LibjpegCodec> codec = new LibjpegCodec;
				_libjpeg_codec = codec.Inner();
			}
			if (!_libtiff_codec) {
				SafePointer<LibtiffCodec> codec = new LibtiffCodec;
				_libtiff_codec = codec.Inner();
			}
			if (!_libgif_codec) {
				SafePointer<LibgifCodec> codec = new LibgifCodec;
				_libgif_codec = codec.Inner();
			}
		}
	}
}