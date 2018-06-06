#include "IconCodec.h"

#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Codec
	{
		using namespace Streaming;
		class IconCodec : public Codec
		{
			ENGINE_PACKED_STRUCTURE(WindowsIconHeader)
				uint16 reserved;
				uint16 type;
				uint16 count;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(WindowsIconFrameHeader)
				uint8 width;
				uint8 height;
				uint8 colors;
				uint8 reserved;
				union {
					struct {
						uint16 planes;
						uint16 bpp;
					};
					struct {
						uint16 x_hot_point;
						uint16 y_hot_point;
					};
				};
				uint32 size;
				uint32 offset;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(WindowsBitmapInfoHeader)
				uint32 struct_size;
				int32 width;
				int32 height;
				uint16 planes;
				uint16 bpp;
				uint32 compression;
				uint32 data_size;
				int32 dpm_x;
				int32 dpm_y;
				uint32 color_count;
				uint32 color_used;
			ENGINE_END_PACKED_STRUCTURE
			static uint32 InverseEndianess(uint32 value) { return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value & 0xFF0000) >> 8) | ((value & 0xFF000000) >> 24); }
			static Array<uint8> * AppleRleCompress(const Array<uint8> * data)
			{
				SafePointer< Array<uint8> > Result = new Array<uint8>(0x400);
				int pos = 0;
				while (pos < data->Length()) {
					if (pos < data->Length() - 2 && data->ElementAt(pos + 1) == data->ElementAt(pos) && data->ElementAt(pos + 2) == data->ElementAt(pos)) {
						int ep = pos + 3;
						while (ep - pos < 130 && ep < data->Length() && data->ElementAt(ep) == data->ElementAt(pos)) ep++;
						uint8 command = uint8(ep - pos - 3) | 0x80;
						Result->Append(command);
						Result->Append(data->ElementAt(pos));
						pos = ep;
					} else {
						int ep = pos + 1;
						while (ep - pos < 128 && ep < data->Length() && (ep >= data->Length() - 2 ||
							data->ElementAt(ep) != data->ElementAt(ep + 1) || data->ElementAt(ep) != data->ElementAt(ep + 2))) ep++;
						uint8 command = uint8(ep - pos - 1);
						Result->Append(command);
						for (int i = pos; i < ep; i++) Result->Append(data->ElementAt(i));
						pos = ep;
					}
				}
				Result->Retain();
				return Result;
			}
			static Array<uint8> * AppleRleDecompress(const Array<uint8> * data)
			{
				SafePointer< Array<uint8> > Result = new Array<uint8>(0x800);
				int pos = 0;
				while (pos < data->Length()) {
					uint8 command = data->ElementAt(pos);
					if ((command & 0x80) == 0) {
						pos++;
						int rep = 1 + int(command);
						for (int i = 0; i < rep; i++) {
							if (pos >= data->Length()) break;
							Result->Append(data->ElementAt(pos));
							pos++;
						}
					} else {
						pos++;
						if (pos < data->Length()) {
							uint8 word = data->ElementAt(pos);
							pos++;
							int rep = 3 + int(command & 0x7F);
							for (int i = 0; i < rep; i++) Result->Append(word);
						}
					}
				}
				Result->Retain();
				return Result;
			}
			static void AppleIconWriteFrame(Stream * stream, Frame * frame, const string & section)
			{
				if (section == L"ic11" || section == L"ic12" || section == L"ic13" || section == L"ic14" ||
					section == L"ic07" || section == L"ic08" || section == L"ic09" || section == L"ic10") {
					MemoryStream encoded(0x1000);
					Engine::Codec::EncodeFrame(&encoded, frame, L"PNG");
					uint32 type, size;
					section.Encode(&type, Encoding::ANSI, false);
					size = InverseEndianess(uint32(encoded.Length()) + 8);
					encoded.Seek(0, Begin);
					stream->Write(&type, 4);
					stream->Write(&size, 4);
					encoded.CopyTo(stream);
				} else if (section == L"is32" || section == L"il32") {
					Array<uint8> out_r(0x200);
					Array<uint8> out_g(0x200);
					Array<uint8> out_b(0x200);
					auto format = frame->GetPixelFormat();
					auto alpha = frame->GetAlphaFormat();
					if (IsPalettePixel(format)) {
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 cv = uint8(GetRedChannel(frame->GetPalette()[frame->GetPixel(x, y)], PixelFormat::B8G8R8A8, AlphaFormat::Normal));
							out_r << cv;
						}
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 cv = uint8(GetGreenChannel(frame->GetPalette()[frame->GetPixel(x, y)], PixelFormat::B8G8R8A8, AlphaFormat::Normal));
							out_g << cv;
						}
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 cv = uint8(GetBlueChannel(frame->GetPalette()[frame->GetPixel(x, y)], PixelFormat::B8G8R8A8, AlphaFormat::Normal));
							out_b << cv;
						}
					} else {
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 cv = uint8(GetRedChannel(frame->GetPixel(x, y), format, alpha));
							out_r << cv;
						}
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 cv = uint8(GetGreenChannel(frame->GetPixel(x, y), format, alpha));
							out_g << cv;
						}
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 cv = uint8(GetBlueChannel(frame->GetPixel(x, y), format, alpha));
							out_b << cv;
						}
					}
					SafePointer< Array<uint8> > com_r = AppleRleCompress(&out_r);
					SafePointer< Array<uint8> > com_g = AppleRleCompress(&out_g);
					SafePointer< Array<uint8> > com_b = AppleRleCompress(&out_b);
					uint32 type, size;
					section.Encode(&type, Encoding::ANSI, false);
					size = InverseEndianess(uint32(com_r->Length() + com_g->Length() + com_b->Length()) + 8);
					stream->Write(&type, 4);
					stream->Write(&size, 4);
					stream->Write(com_r->GetBuffer(), com_r->Length());
					stream->Write(com_g->GetBuffer(), com_g->Length());
					stream->Write(com_b->GetBuffer(), com_b->Length());
				} else if (section == L"s8mk" || section == L"l8mk") {
					Array<uint8> out(0x800);
					auto format = frame->GetPixelFormat();
					auto alpha = frame->GetAlphaFormat();
					if (IsPalettePixel(format)) {
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 av = uint8(GetAlphaChannel(frame->GetPalette()[frame->GetPixel(x, y)], PixelFormat::B8G8R8A8, AlphaFormat::Normal));
							out << av;
						}
					} else {
						for (int y = 0; y < frame->GetHeight(); y++) for (int x = 0; x < frame->GetWidth(); x++) {
							uint8 av = uint8(GetAlphaChannel(frame->GetPixel(x, y), format, alpha));
							out << av;
						}
					}
					uint32 type, size;
					section.Encode(&type, Encoding::ANSI, false);
					size = InverseEndianess(uint32(out.Length()) + 8);
					stream->Write(&type, 4);
					stream->Write(&size, 4);
					stream->Write(out.GetBuffer(), out.Length());
				}
			}
		public:
			IconCodec(void) {}
			~IconCodec(void) override {}
			virtual void EncodeFrame(Streaming::Stream * stream, Frame * frame, const string & format) override {}
			virtual void EncodeImage(Streaming::Stream * stream, Image * image, const string & format) override
			{
				if (format == L"ICO" || format == L"CUR") {
					for (int i = 0; i < image->Frames.Length(); i++)
						if (image->Frames[i].GetWidth() > 0x100 || image->Frames[i].GetHeight() > 0x100)
							throw InvalidArgumentException();
					WindowsIconHeader hdr;
					hdr.reserved = 0;
					hdr.type = (format == L"ICO") ? 1 : 2;
					hdr.count = image->Frames.Length();
					Array<WindowsIconFrameHeader> frames(0x10);
					frames.SetLength(image->Frames.Length());
					ZeroMemory(frames.GetBuffer(), frames.Length() * sizeof(WindowsIconFrameHeader));
					auto start = stream->Seek(0, Current);
					stream->Write(&hdr, sizeof(hdr));
					auto pos = stream->Seek(0, Current);
					stream->Write(frames.GetBuffer(), frames.Length() * sizeof(WindowsIconFrameHeader));
					for (int i = 0; i < image->Frames.Length(); i++) {
						frames[i].width = image->Frames[i].GetWidth();
						frames[i].height = image->Frames[i].GetHeight();
						frames[i].colors = 0;
						frames[i].reserved = 0;
						if (format == L"ICO") {
							frames[i].planes = 1;
							frames[i].bpp = 32;
						} else {
							frames[i].x_hot_point = image->Frames[i].HotPointX;
							frames[i].y_hot_point = image->Frames[i].HotPointY;
						}
						frames[i].offset = uint32(stream->Seek(0, Current) - start);
						if (frames[i].width == 0 && frames[i].height == 0) {
							MemoryStream frame(0x1000);
							Engine::Codec::EncodeFrame(&frame, image->Frames.ElementAt(i), L"PNG");
							frame.Seek(0, Begin);
							frame.CopyTo(stream);
						} else {
							SafePointer<Frame> Conv = image->Frames[i].ConvertFormat(FrameFormat(PixelFormat::B8G8R8A8, AlphaFormat::Normal, LineDirection::BottomUp));
							WindowsBitmapInfoHeader bhdr;
							ZeroMemory(&bhdr, sizeof(bhdr));
							bhdr.struct_size = sizeof(bhdr);
							bhdr.width = Conv->GetWidth();
							bhdr.height = Conv->GetHeight();
							bhdr.planes = 1;
							bhdr.bpp = 32;
							bhdr.data_size = Conv->GetHeight() * Conv->GetScanLineLength();
							stream->Write(&bhdr, sizeof(bhdr));
							stream->Write(Conv->GetData(), bhdr.data_size);
						}
						frames[i].size = uint32(stream->Seek(0, Current) - start - frames[i].offset);
					}
					auto end = stream->Seek(0, Current);
					stream->Seek(pos, Begin);
					stream->Write(frames.GetBuffer(), frames.Length() * sizeof(WindowsIconFrameHeader));
					stream->Seek(end, Begin);
				} else if (format == L"ICNS") {
					auto pos = stream->Seek(0, Current);
					uint32 signature = 0x736E6369;
					stream->Write(&signature, 4);
					signature = 0;
					stream->Write(&signature, 4);
					Frame * encode = 0;
					encode = image->GetFramePreciseSize(16, 16);
					if (encode) {
						AppleIconWriteFrame(stream, encode, L"is32");
						AppleIconWriteFrame(stream, encode, L"s8mk");
					}
					encode = image->GetFramePreciseSize(32, 32);
					if (encode) {
						AppleIconWriteFrame(stream, encode, L"ic11");
						AppleIconWriteFrame(stream, encode, L"il32");
						AppleIconWriteFrame(stream, encode, L"l8mk");
					}
					encode = image->GetFramePreciseSize(64, 64);
					if (encode) {
						AppleIconWriteFrame(stream, encode, L"ic12");
					}
					encode = image->GetFramePreciseSize(128, 128);
					if (encode) {
						AppleIconWriteFrame(stream, encode, L"ic07");
					}
					encode = image->GetFramePreciseSize(256, 256);
					if (encode) {
						AppleIconWriteFrame(stream, encode, L"ic13");
						AppleIconWriteFrame(stream, encode, L"ic08");
					}
					encode = image->GetFramePreciseSize(512, 512);
					if (encode) {
						AppleIconWriteFrame(stream, encode, L"ic14");
						AppleIconWriteFrame(stream, encode, L"ic09");
					}
					encode = image->GetFramePreciseSize(1024, 1024);
					if (encode) {
						AppleIconWriteFrame(stream, encode, L"ic10");
					}
					auto end = stream->Seek(0, Current);
					uint32 size = InverseEndianess(uint32(end - pos));
					stream->Seek(pos + 4, Begin);
					stream->Write(&size, 4);
					stream->Seek(end, Begin);
				} else throw InvalidArgumentException();
			}
			virtual Frame * DecodeFrame(Streaming::Stream * stream) override { return 0; }
			virtual Image * DecodeImage(Streaming::Stream * stream) override
			{
				string Type = ExamineData(stream);
				if (!Type) return 0;
				SafePointer<Image> Result = new Image;
				if (Type == L"ICO" || Type == L"CUR") {
					try {
						WindowsIconHeader hdr;
						stream->Read(&hdr, sizeof(hdr));
						for (int i = 0; i < hdr.count; i++) {
							WindowsIconFrameHeader fhdr;
							stream->Read(&fhdr, sizeof(fhdr));
							auto pos = stream->Seek(0, Current);
							stream->Seek(fhdr.offset, Begin);
							uint64 sign;
							stream->Read(&sign, sizeof(sign));
							stream->Seek(fhdr.offset, Begin);
							if (sign == 0x0A1A0A0D474E5089) {
								FragmentStream frame(stream, fhdr.offset, fhdr.size);
								SafePointer<Frame> dec = Engine::Codec::DecodeFrame(&frame);
								if (!dec) throw 0;
								if (hdr.type == 2) {
									dec->HotPointX = fhdr.x_hot_point;
									dec->HotPointY = fhdr.y_hot_point;
									dec->DpiUsage = 1.0;
									if (dec->GetWidth() == 48) dec->DpiUsage = 1.5;
									else if (dec->GetWidth() == 64) dec->DpiUsage = 2.0;
								} else {
									dec->HotPointX = 0;
									dec->HotPointY = 0;
									dec->DpiUsage = 1.0;
								}
								dec->Duration = 0;
								dec->Usage = FrameUsage::ColorMap;
								Result->Frames.Append(dec);
							} else {
								WindowsBitmapInfoHeader bhdr;
								stream->Read(&bhdr, sizeof(bhdr));
								SafePointer<Frame> frame = new Frame(fhdr.width ? fhdr.width : 0x100, fhdr.height ? fhdr.height : 0x100, -1,
									PixelFormat::B8G8R8A8, AlphaFormat::Normal, (bhdr.height > 0) ? LineDirection::BottomUp : LineDirection::TopDown);
								if (hdr.type == 2) {
									frame->HotPointX = fhdr.x_hot_point;
									frame->HotPointY = fhdr.y_hot_point;
									frame->DpiUsage = 1.0;
									if (frame->GetWidth() == 48) frame->DpiUsage = 1.5;
									else if (frame->GetWidth() == 64) frame->DpiUsage = 2.0;
								}
								if (bhdr.bpp == 32) {
									stream->Seek(fhdr.offset + bhdr.struct_size, Begin);
									stream->Read(frame->GetData(), 4 * frame->GetWidth() * frame->GetHeight());
								} else {
									Array<uint32> palette(0x100);
									if (bhdr.bpp <= 8) {
										uint32 dcc = 1;
										for (int j = 0; j < bhdr.bpp; j++) dcc <<= 1;
										palette.SetLength(bhdr.color_used ? bhdr.color_used : dcc);
										stream->Read(palette.GetBuffer(), palette.Length() * 4);
									}
									for (int y = 0; y < frame->GetHeight(); y++) {
										Array<uint8> line(0x100);
										uint32 ll = ((bhdr.bpp * frame->GetWidth() + 31) / 32) * 4;
										line.SetLength(ll);
										stream->Read(line.GetBuffer(), ll);
										for (int x = 0; x < frame->GetWidth(); x++) {
											uint32 byte = (x * bhdr.bpp) / 8;
											uint32 bit = 8 - (x * bhdr.bpp) % 8 - bhdr.bpp;
											if (bhdr.bpp == 24) {
												uint32 v1 = line[byte];
												uint32 v2 = line[byte + 1];
												uint32 v3 = line[byte + 2];
												uint32 cc = 0xFF000000 | v1 | (v2 << 8) | (v3 << 16);
												int cy = (frame->GetLineDirection() == LineDirection::BottomUp) ? (frame->GetHeight() - y - 1) : y;
												frame->SetPixel(x, cy, cc);
											} else {
												uint8 v = line[byte];
												if (bhdr.bpp < 8) v >>= bit;
												if (bhdr.bpp == 4) {
													v &= 0xF;
												} else if (bhdr.bpp == 2) {
													v &= 0x3;
												} else if (bhdr.bpp == 1) {
													v &= 0x1;
												}
												uint32 cc = 0xFF000000 | palette[v];
												int cy = (frame->GetLineDirection() == LineDirection::BottomUp) ? (frame->GetHeight() - y - 1) : y;
												frame->SetPixel(x, cy, cc);
											}
										}
									}
									for (int y = 0; y < frame->GetHeight(); y++) {
										Array<uint8> line(0x100);
										uint32 ll = ((frame->GetWidth() + 31) / 32) * 4;
										line.SetLength(ll);
										stream->Read(line.GetBuffer(), ll);
										for (int x = 0; x < frame->GetWidth(); x++) {
											uint32 byte = x / 8;
											uint32 bit = 7 - x % 8;
											uint8 v = (line[byte] >> bit) & 1;	
											int cy = (frame->GetLineDirection() == LineDirection::BottomUp) ? (frame->GetHeight() - y - 1) : y;
											if (v) frame->SetPixel(x, cy, 0);
										}
									}
								}
								Result->Frames.Append(frame);
							}
							stream->Seek(pos, Begin);
						}
					}
					catch (...) { return 0; }
				} else if (Type == L"ICNS") {
					try {
						uint64 length = stream->Length();
						uint32 read_length;
						stream->Seek(4, Begin);
						stream->Read(&read_length, 4);
						read_length = InverseEndianess(read_length);
						if (read_length > length) return 0;
						uint64 position = stream->Seek(0, Current);
						Dictionary::Dictionary<string, Array<uint8> > Deferred;
						while (position < length) {
							uint8 type[4];
							uint32 frame_length;
							stream->Seek(position, Begin);
							stream->Read(&type, 4);
							stream->Read(&frame_length, 4);
							frame_length = InverseEndianess(frame_length);
							auto last_position = position;
							position += frame_length;
							if (MemoryCompare(type, "icp4", 4) == 0 || MemoryCompare(type, "icp5", 4) == 0 || MemoryCompare(type, "icp6", 4) == 0 ||
								MemoryCompare(type, "ic07", 4) == 0 || MemoryCompare(type, "ic08", 4) == 0 || MemoryCompare(type, "ic09", 4) == 0 ||
								MemoryCompare(type, "ic10", 4) == 0 || MemoryCompare(type, "ic11", 4) == 0 || MemoryCompare(type, "ic12", 4) == 0 ||
								MemoryCompare(type, "ic13", 4) == 0 || MemoryCompare(type, "ic14", 4) == 0) {
								FragmentStream FrameStream(stream, last_position + 8, frame_length - 8);
								SafePointer<Frame> FrameDecoded = Engine::Codec::DecodeFrame(&FrameStream);
								if (FrameDecoded) Result->Frames.Append(FrameDecoded);
							} else if (MemoryCompare(type, "is32", 4) == 0 || MemoryCompare(type, "s8mk", 4) == 0 || MemoryCompare(type, "il32", 4) == 0 ||
								MemoryCompare(type, "l8mk", 4) == 0 || MemoryCompare(type, "ih32", 4) == 0 || MemoryCompare(type, "h8mk", 4) == 0 ||
								MemoryCompare(type, "it32", 4) == 0 || MemoryCompare(type, "t8mk", 4) == 0) {
								SafePointer< Array<uint8> > block = new Array<uint8>(0x100);
								block->SetLength(frame_length - 8);
								stream->Read(block->GetBuffer(), frame_length - 8);
								Deferred.Append(string(type, 4, Encoding::ANSI), block);
							}
						}
						for (int i = 0; i < Deferred.Length(); i++) {
							if (Deferred[i].key == L"is32" || Deferred[i].key == L"il32" || Deferred[i].key == L"ih32" || Deferred[i].key == L"it32") {
								Array<uint8> * color_map = Deferred[i].object;
								int32 side = 0;
								string mask_name = L"";
								if (Deferred[i].key == L"is32") { side = 16; mask_name = L"s8mk"; }
								else if (Deferred[i].key == L"il32") { side = 32; mask_name = L"l8mk"; }
								else if (Deferred[i].key == L"ih32") { side = 48; mask_name = L"h8mk"; }
								else if (Deferred[i].key == L"it32") { side = 128; mask_name = L"t8mk"; }
								Array<uint8> * mask_map = Deferred[mask_name];
								SafePointer< Array<uint8> > color_map_dec;
								color_map_dec.SetReference(AppleRleDecompress(color_map));
								SafePointer<Frame> frame = new Frame(side, side, -1, PixelFormat::B8G8R8A8, AlphaFormat::Normal, LineDirection::TopDown);
								for (int y = 0; y < side; y++) for (int x = 0; x < side; x++) {
									uint32 alpha = mask_map ? mask_map->ElementAt(x + side * y) : 0xFF;
									frame->SetPixel(x, y, (uint32(color_map_dec->ElementAt(x + side * y)) << 16) |
										(uint32(color_map_dec->ElementAt(side * side + x + side * y)) << 8) |
										uint32(color_map_dec->ElementAt(2 * side * side + x + side * y)) |
										(alpha << 24));
								}
								Result->Frames.Append(frame);
							}
						}
					}
					catch (...) { return 0; }
				}
				Result->Retain();
				return Result;
			}
			virtual bool IsImageCodec(void) override { return true; }
			virtual bool IsFrameCodec(void) override { return false; }
			virtual string ExamineData(Streaming::Stream * stream) override
			{
				uint64 size = stream->Length() - stream->Seek(0, Current);
				if (size < 4) return L"";
				uint64 begin = stream->Seek(0, Current);
				uint32 sign;
				try {
					stream->Read(&sign, sizeof(sign));
					stream->Seek(begin, Begin);
				}
				catch (...) { return L""; }
				if (sign == 0x00010000) return L"ICO";
				else if (sign == 0x00020000) return L"CUR";
				else if (sign == 0x736E6369) return L"ICNS";
				else return L"";
			}
			virtual bool CanEncode(const string & format) override { return (format == L"ICO" || format == L"CUR" || format == L"ICNS"); }
			virtual bool CanDecode(const string & format) override { return (format == L"ICO" || format == L"CUR" || format == L"ICNS"); }
		};
		Codec * _IconCodec = 0;
		Codec * CreateIconCodec(void) { if (!_IconCodec) { _IconCodec = new IconCodec(); _IconCodec->Release(); } return _IconCodec; }
	}
}