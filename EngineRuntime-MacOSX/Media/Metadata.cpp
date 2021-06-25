#include "Metadata.h"

#include "../Interfaces/Socket.h"
#include "../Miscellaneous/DynamicString.h"

using namespace Engine::Streaming;
using namespace Engine::Codec;

namespace Engine
{
	namespace Media
	{
		bool operator==(const MetadataValue & a, const MetadataValue & b) { return MemoryCompare(&a, &b, sizeof(MetadataValue)) == 0; }
		bool MetadataKeyIsString(MetadataKey key)
		{
			if ((static_cast<uint>(key) & 0x000F0000) == 0x00010000) return true;
			else if ((static_cast<uint>(key) & 0x000F0000) == 0x00020000) return true;
			else return false;
		}
		bool MetadataKeyIsMultilineString(MetadataKey key)
		{
			if ((static_cast<uint>(key) & 0x000F0000) == 0x00020000) return true;
			else return false;
		}
		bool MetadataKeyIsInteger(MetadataKey key)
		{
			if ((static_cast<uint>(key) & 0x000F0000) == 0x00030000) return true;
			else return false;
		}
		bool MetadataKeyIsPicture(MetadataKey key)
		{
			if ((static_cast<uint>(key) & 0x000F0000) == 0x00040000) return true;
			else return false;
		}
		MetadataValue::MetadataValue(void) : Number(0) {}
		MetadataValue::MetadataValue(const string & text) : Text(text), Number(0) {}
		MetadataValue::MetadataValue(uint32 number) : Number(number) {}
		MetadataValue::MetadataValue(Codec::Frame * picture, const string & encoder) : Number(0), Text(encoder) { Picture.SetRetain(picture); }
		MetadataValue::operator string(void) const { return Text; }
		MetadataValue::operator uint32(void) const { return Number; }
		MetadataValue::operator Codec::Frame * (void) const { return Picture; }

		namespace FileFormats
		{
			ENGINE_PACKED_STRUCTURE(ID3Header)
				char signature[3];
				uint16 version;
				uint8 flags;
				uint32 size;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(ID3FrameV2)
				char name[3];
				uint8 size_hi;
				uint8 size_md;
				uint8 size_lo;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(ID3FrameV3V4)
				char name[4];
				uint32 size;
				uint16 flags;
			ENGINE_END_PACKED_STRUCTURE
			struct MPEG4Atom
			{
				char type[4];
				SafePointer< SafeArray<MPEG4Atom> > children;
				SafePointer<DataBlock> contents;
			};
			
			uint32 ID3_ReadLength(uint32 length)
			{
				auto i = Network::InverseEndianess(length);
				return (i & 0x7F) | ((i & 0x7F00) >> 1) | ((i & 0x7F0000) >> 2) | ((i & 0x7F000000) >> 3);
			}
			uint32 ID3_WriteLength(uint32 length)
			{
				auto i = (length & 0x7F) | ((length & 0x3F80) << 1) | ((length & 0x1FC000) << 2) | ((length & 0x0FE00000) << 3);
				return Network::InverseEndianess(i);
			}
			DataBlock * ID3_RemoveUnsynchronisation(const DataBlock * block)
			{
				SafePointer<DataBlock> regular = new DataBlock(0x10000);
				for (int i = 0; i < block->Length(); i++) {
					regular->Append(block->ElementAt(i));
					if (i < block->Length() - 1 && block->ElementAt(i) == 0xFF && !block->ElementAt(i + 1)) i++;
				}
				regular->Retain();
				return regular;
			}
			bool ID3_Unsynchronize(DataBlock * block)
			{
				bool unsync = false;
				DataBlock local(0x1000);
				for (int i = 0; i < block->Length(); i++) {
					if (block->ElementAt(i) == 0xFF) {
						if (i + 1 < block->Length() && (block->ElementAt(i + 1) & 0xE0) == 0xE0) {
							local.Append(0xFF);
							local.Append(0x00);
							unsync = true;
						} else if (i + 1 == block->Length() || block->ElementAt(i + 1) == 0) {
							local.Append(0xFF);
							local.Append(0x00);
							unsync = true;
						} else local.Append(block->ElementAt(i));
					} else local.Append(block->ElementAt(i));
				}
				if (unsync) {
					block->SetLength(local.Length());
					MemoryCopy(block->GetBuffer(), local.GetBuffer(), local.Length());
				}
				return unsync;
			}
			string ID3_DecodeString(uint8 enc, uint8 * buffer, uint32 size, uint32 * term)
			{
				if (enc == 0) {
					uint pos = 0;
					while (pos < size && buffer[pos]) pos++;
					if (term) *term = pos + 1;
					return string(buffer, pos, Encoding::ANSI);
				} else if (enc == 1) {
					uint pos = 0;
					while (pos < size - 1 && (buffer[pos] || buffer[pos + 1])) pos += 2;
					if (term) *term = pos + 2;
					if (buffer[0] == 0xFF && buffer[1] == 0xFE) {
						int i = 0;
						while (buffer[i] && buffer[i + 1]) {
							swap(buffer[i], buffer[i + 1]);
							i += 2;
						}
					}
					if (buffer[0] == 0xFE && buffer[1] == 0xFF) {
						return string(buffer + 2, pos / 2 - 1, Encoding::UTF16);
					} else throw InvalidFormatException();
				} else if (enc == 2) {
					uint pos = 0;
					while (pos < size - 1 && (buffer[pos] || buffer[pos + 1])) pos += 2;
					if (term) *term = pos + 2;
					int i = 0;
					while (buffer[i] && buffer[i + 1]) {
						swap(buffer[i], buffer[i + 1]);
						i += 2;
					}
					return string(buffer, pos / 2 - 1, Encoding::UTF16);
				} else if (enc == 3) {
					uint pos = 0;
					while (pos < size && buffer[pos]) pos++;
					if (term) *term = pos + 1;
					return string(buffer, pos, Encoding::UTF8);
				} else throw InvalidFormatException();
			}
			string ID3_DecodeString(uint8 * buffer, uint32 size) { return ID3_DecodeString(buffer[0], buffer + 1, size - 1, 0); }
			void ID3_ReadAttributes(Metadata & metadata, uint32 version, const uint8 * data, uint32 data_size)
			{
				uint32 pos = 0;
				uint32 hdr_size = (version == 2) ? sizeof(ID3FrameV2) : sizeof(ID3FrameV3V4);
				while (pos <= data_size - hdr_size) {
					const ID3FrameV3V4 * hdr_v3v4 = 0;
					const ID3FrameV2 * hdr_v2 = 0;
					uint32 length;
					if (version == 2) {
						hdr_v2 = reinterpret_cast<const ID3FrameV2 *>(data + pos);
						length = (uint32(hdr_v2->size_hi) << 16) | (uint32(hdr_v2->size_md) << 8) | hdr_v2->size_lo;
						if (!hdr_v2->name[0]) break;
					} else {
						hdr_v3v4 = reinterpret_cast<const ID3FrameV3V4 *>(data + pos);
						if (version == 3) length = Network::InverseEndianess(hdr_v3v4->size);
						else length = ID3_ReadLength(hdr_v3v4->size);
						if (!hdr_v3v4->name[0]) break;
					}
					pos += hdr_size;
					bool frame_unsync = false;
					if (hdr_v3v4) {
						if (version == 3) {
							if (hdr_v3v4->flags & 0xC000) break;
							if (hdr_v3v4->flags & 0x2000) { pos++; length--; }
						} else {
							if (hdr_v3v4->flags & 0x0C00) break;
							if (hdr_v3v4->flags & 0x4000) { pos++; length--; }
							if (hdr_v3v4->flags & 0x0200) frame_unsync = true;
							if (hdr_v3v4->flags & 0x0100) { pos += 4; length -= 4; }
						}
					}
					SafePointer<DataBlock> frame = new DataBlock(0x100);
					frame->SetLength(length);
					MemoryCopy(frame->GetBuffer(), data + pos, length);
					if (frame_unsync) frame = ID3_RemoveUnsynchronisation(frame);
					if ((hdr_v2 && hdr_v2->name[0] == 'T') || (hdr_v3v4 && hdr_v3v4->name[0] == 'T')) {
						bool skip = false;
						if (hdr_v2 && MemoryCompare(hdr_v2->name, "TXX", 3) == 0) skip = true;
						if (hdr_v3v4 && MemoryCompare(hdr_v3v4->name, "TXXX", 4) == 0) skip = true;
						if (!skip) {
							auto code = hdr_v3v4 ? string(hdr_v3v4->name, 4, Encoding::ANSI) : string(hdr_v2->name, 3, Encoding::ANSI);
							auto string = ID3_DecodeString(frame->GetBuffer(), frame->Length());
							try {
								if (code == L"TALB" || code == L"TAL") metadata.Append(MetadataKey::Album, string);
								else if (code == L"TBPM" || code == L"TBP") metadata.Append(MetadataKey::BeatsPerMinute, string.ToUInt32());
								else if (code == L"TCOM" || code == L"TCM") metadata.Append(MetadataKey::Composer, string);
								else if (code == L"TCOP" || code == L"TCR") metadata.Append(MetadataKey::Copyright, string);
								else if (code == L"TENC" || code == L"TEN") metadata.Append(MetadataKey::Encoder, string);
								else if (code == L"TIT2" || code == L"TT2") metadata.Append(MetadataKey::Title, string);
								else if (code == L"TIT3" || code == L"TT3") metadata.Append(MetadataKey::Description, string);
								else if (code == L"TYER" || code == L"TYE") metadata.Append(MetadataKey::Year, string);
								else if (code == L"TPE1" || code == L"TP1") metadata.Append(MetadataKey::Artist, string);
								else if (code == L"TPE2" || code == L"TP2") metadata.Append(MetadataKey::AlbumArtist, string);
								else if (code == L"TPUB") metadata.Append(MetadataKey::Publisher, string);
								else if (code == L"TCON" || code == L"TCO") {
									DynamicString str;
									int pos = 0;
									while (string[pos] == L'(') {
										pos++;
										if (string[pos] == L'(') break;
										while (pos < string.Length() && string[pos] != L')') pos++;
										if (pos < string.Length()) pos++;
									}
									while (pos < string.Length()) { str << string[pos]; pos++; }
									if (str.Length()) metadata.Append(MetadataKey::Genre, str.ToString());
								} else if (code == L"TRCK" || code == L"TRK") {
									int index;
									if ((index = string.FindFirst(L'/')) != -1) {
										metadata.Append(MetadataKey::TrackNumber, string.Fragment(0, index).ToUInt32());
										metadata.Append(MetadataKey::TrackCount, string.Fragment(index + 1, -1).ToUInt32());
									} else metadata.Append(MetadataKey::TrackNumber, string.ToUInt32());
								} else if (code == L"TPOS" || code == L"TPA") {
									int index;
									if ((index = string.FindFirst(L'/')) != -1) {
										metadata.Append(MetadataKey::DiskNumber, string.Fragment(0, index).ToUInt32());
										metadata.Append(MetadataKey::DiskCount, string.Fragment(index + 1, -1).ToUInt32());
									} else metadata.Append(MetadataKey::DiskNumber, string.ToUInt32());
								}
							} catch (...) {}
						}
					} else if ((hdr_v2 && MemoryCompare(hdr_v2->name, "COM", 3) == 0) || (hdr_v3v4 && MemoryCompare(hdr_v3v4->name, "COMM", 4) == 0)) {
						uint32 end;
						auto shr = ID3_DecodeString(frame->ElementAt(0), frame->GetBuffer() + 4, frame->Length() - 4, &end);
						auto lng = ID3_DecodeString(frame->ElementAt(0), frame->GetBuffer() + 4 + end, frame->Length() - 4 - end, &end);
						metadata.Append(MetadataKey::Comment, lng);
					} else if ((hdr_v2 && MemoryCompare(hdr_v2->name, "PIC", 3) == 0) || (hdr_v3v4 && MemoryCompare(hdr_v3v4->name, "APIC", 4) == 0)) {
						uint8 enc = frame->ElementAt(0);
						uint32 type_end, desc_end;
						if (version >= 3) ID3_DecodeString(0, frame->GetBuffer() + 1, frame->Length() - 1, &type_end);
						else type_end = 3;
						uint8 pic_type = frame->ElementAt(type_end + 1);
						auto pic_desc = ID3_DecodeString(enc, frame->GetBuffer() + type_end + 2, frame->Length() - 2 - type_end, &desc_end);
						MemoryStream data(0x10000);
						data.Write(frame->GetBuffer() + type_end + 2 + desc_end, frame->Length() - type_end - 2 - desc_end);
						data.Seek(0, Begin);
						string format;
						try {
							SafePointer<Frame> picture = DecodeFrame(&data, &format, 0);
							if (picture) metadata.Append(MetadataKey::Artwork, MetadataValue(picture, format));
						} catch (...) {}
					} else if ((hdr_v2 && MemoryCompare(hdr_v2->name, "ULT", 3) == 0) || (hdr_v3v4 && MemoryCompare(hdr_v3v4->name, "USLT", 4) == 0)) {
						uint32 end;
						auto desc = ID3_DecodeString(frame->ElementAt(0), frame->GetBuffer() + 4, frame->Length() - 4, &end);
						auto lyr = ID3_DecodeString(frame->ElementAt(0), frame->GetBuffer() + 4 + end, frame->Length() - 4 - end, &end);
						metadata.Append(MetadataKey::Lyrics, lyr);
					}
					pos += length;
				}
			}
			Metadata * ID3_Read(Streaming::Stream * stream, const ID3Header & header, uint64 base, string * metadata_format)
			{
				stream->Seek(base + sizeof(header), Begin);
				uint32 version = 0;
				if (header.version == 0x0002) version = 2;
				else if (header.version == 0x0003) version = 3;
				else if (header.version == 0x0004) version = 4;
				else return 0;
				if (version == 2 && (header.flags & 0x40)) return 0;
				auto length = FileFormats::ID3_ReadLength(header.size);
				SafePointer<DataBlock> frames = new DataBlock(0x100);
				frames->SetLength(length);
				stream->Read(frames->GetBuffer(), length);
				if (header.flags & 0x80 && version < 4) frames = FileFormats::ID3_RemoveUnsynchronisation(frames);
				uint32 read_pos = 0;
				if (version >= 3 && (header.flags & 0x40)) {
					if (frames->Length() < 4) throw InvalidFormatException();
					auto ex_length = FileFormats::ID3_ReadLength(*reinterpret_cast<uint32 *>(frames->GetBuffer()));
					if (frames->Length() < int(ex_length)) throw InvalidFormatException();
					read_pos = ex_length;
				}
				SafePointer<Metadata> result = new Metadata(0x10);
				if (metadata_format) *metadata_format = MetadataFormatMPEG3ID3;
				FileFormats::ID3_ReadAttributes(*result, version, frames->GetBuffer() + read_pos, frames->Length() - read_pos);
				result->Retain();
				return result;
			}
			DataBlock * ID3_CreateAttribute(const Metadata * metadata, int index, bool & unsync)
			{
				auto & attr = metadata->ElementByIndex(index);
				SafePointer<DataBlock> result = new DataBlock(0x1000);
				ID3FrameV3V4 frame;
				frame.flags = 0;
				if (MetadataKeyIsString(attr.key)) {
					if (MetadataKeyIsMultilineString(attr.key)) {
						DynamicString output;
						for (int i = 0; i < attr.value.Text.Length(); i++) {
							if (attr.value.Text[i] >= 32 || attr.value.Text[i] == L'\t' || attr.value.Text[i] == L'\n') output << attr.value.Text[i];
						}
						SafePointer<DataBlock> encoded = output.ToString().EncodeSequence(Encoding::UTF8, false);
						auto length = 5 + encoded->Length();
						result->SetLength(length + sizeof(frame));
						ZeroMemory(result->GetBuffer(), sizeof(frame));
						result->ElementAt(sizeof(frame)) = 3;
						result->ElementAt(sizeof(frame) + 1) = 'u';
						result->ElementAt(sizeof(frame) + 2) = 'n';
						result->ElementAt(sizeof(frame) + 3) = 'd';
						result->ElementAt(sizeof(frame) + 4) = 0;
						MemoryCopy(result->GetBuffer() + sizeof(frame) + 5, encoded->GetBuffer(), encoded->Length());
						if (ID3_Unsynchronize(result)) { frame.flags |= 0x0200; unsync = true; }
						frame.size = ID3_WriteLength(result->Length() - sizeof(frame));
						if (attr.key == MetadataKey::Comment) MemoryCopy(frame.name, "COMM", 4);
						else if (attr.key == MetadataKey::Lyrics) MemoryCopy(frame.name, "USLT", 4);
						else { result->SetLength(0); result->Retain(); return result; }
						MemoryCopy(result->GetBuffer(), &frame, sizeof(frame));
					} else {
						DynamicString output;
						for (int i = 0; i < attr.value.Text.Length(); i++) {
							if (attr.value.Text[i] >= 32 || attr.value.Text[i] == 9) output << attr.value.Text[i];
						}
						if (attr.key == MetadataKey::Genre && output[0] == L'(') output.Insert(L"(", 0);
						SafePointer<DataBlock> encoded = output.ToString().EncodeSequence(Encoding::UTF8, false);
						auto length = 1 + encoded->Length();
						result->SetLength(length + sizeof(frame));
						ZeroMemory(result->GetBuffer(), sizeof(frame));
						result->ElementAt(sizeof(frame)) = 3;
						MemoryCopy(result->GetBuffer() + sizeof(frame) + 1, encoded->GetBuffer(), encoded->Length());
						if (ID3_Unsynchronize(result)) { frame.flags |= 0x0200; unsync = true; }
						frame.size = ID3_WriteLength(result->Length() - sizeof(frame));
						if (attr.key == MetadataKey::Album) MemoryCopy(frame.name, "TALB", 4);
						else if (attr.key == MetadataKey::Composer) MemoryCopy(frame.name, "TCOM", 4);
						else if (attr.key == MetadataKey::Copyright) MemoryCopy(frame.name, "TCOP", 4);
						else if (attr.key == MetadataKey::Encoder) MemoryCopy(frame.name, "TENC", 4);
						else if (attr.key == MetadataKey::Title) MemoryCopy(frame.name, "TIT2", 4);
						else if (attr.key == MetadataKey::Description) MemoryCopy(frame.name, "TIT3", 4);
						else if (attr.key == MetadataKey::Year) MemoryCopy(frame.name, "TYER", 4);
						else if (attr.key == MetadataKey::Artist) MemoryCopy(frame.name, "TPE1", 4);
						else if (attr.key == MetadataKey::AlbumArtist) MemoryCopy(frame.name, "TPE2", 4);
						else if (attr.key == MetadataKey::Genre) MemoryCopy(frame.name, "TCON", 4);
						else if (attr.key == MetadataKey::Publisher) MemoryCopy(frame.name, "TPUB", 4);
						else { result->SetLength(0); result->Retain(); return result; }
						MemoryCopy(result->GetBuffer(), &frame, sizeof(frame));
					}
				} else if (MetadataKeyIsInteger(attr.key)) {
					string output;
					if (attr.key == MetadataKey::TrackNumber) {
						MemoryCopy(frame.name, "TRCK", 4);
						output = string(attr.value.Number);
						auto frac = metadata->ElementByKey(MetadataKey::TrackCount);
						if (frac) output += L"/" + string(frac->Number);
					} else if (attr.key == MetadataKey::DiskNumber) {
						MemoryCopy(frame.name, "TPOS", 4);
						output = string(attr.value.Number);
						auto frac = metadata->ElementByKey(MetadataKey::DiskCount);
						if (frac) output += L"/" + string(frac->Number);
					} else if (attr.key == MetadataKey::BeatsPerMinute) {
						MemoryCopy(frame.name, "TBPM", 4);
						output = string(attr.value.Number);
					} else { result->SetLength(0); result->Retain(); return result; }
					SafePointer<DataBlock> encoded = output.EncodeSequence(Encoding::UTF8, false);
					auto length = 1 + encoded->Length();
					frame.size = ID3_WriteLength(length);
					result->SetLength(length + sizeof(frame));
					ZeroMemory(result->GetBuffer(), sizeof(frame));
					result->ElementAt(sizeof(frame)) = 3;
					MemoryCopy(result->GetBuffer() + sizeof(frame) + 1, encoded->GetBuffer(), encoded->Length());
					MemoryCopy(result->GetBuffer(), &frame, sizeof(frame));
				} else if (MetadataKeyIsPicture(attr.key)) {
					if (attr.key == MetadataKey::Artwork && attr.value.Picture) {
						MemoryCopy(frame.name, "APIC", 4);
						MemoryStream data(0x10000);
						string encoder_format, mime_type;
						if (attr.value.Text == ImageFormatPNG) {
							encoder_format = ImageFormatPNG;
							mime_type = L"image/png";
						} else {
							encoder_format = ImageFormatJPEG;
							mime_type = L"image/jpeg";
						}
						try {
							EncodeFrame(&data, attr.value.Picture, encoder_format);
						} catch (...) { result->Retain(); return result; }
						uint len = mime_type.Length();
						uint size = uint(data.Length() + 4 + len);
						result->SetLength(sizeof(frame) + size);
						ZeroMemory(result->GetBuffer(), sizeof(frame));
						result->ElementAt(sizeof(frame)) = 0x00;
						mime_type.Encode(result->GetBuffer() + sizeof(frame) + 1, Encoding::ANSI, true);
						result->ElementAt(sizeof(frame) + len + 2) = 0x03;
						result->ElementAt(sizeof(frame) + len + 3) = 0x00;
						data.Seek(0, Begin);
						data.Read(result->GetBuffer() + sizeof(frame) + len + 4, uint(data.Length()));
						if (ID3_Unsynchronize(result)) { frame.flags |= 0x0200; unsync = true; }
						frame.size = ID3_WriteLength(result->Length() - sizeof(frame));
						MemoryCopy(result->GetBuffer(), &frame, sizeof(frame));
					}
				}
				result->Retain();
				return result;
			}
			DataBlock * ID3_Create(const Metadata * metadata)
			{
				if (!metadata->Length()) return 0;
				SafePointer<DataBlock> result = new DataBlock(0x10000);
				SafePointer<DataBlock> frames = new DataBlock(0x10000);
				ID3Header header;
				result->SetLength(sizeof(header));
				header.signature[0] = 'I'; header.signature[1] = 'D'; header.signature[2] = '3';
				header.flags = 0; header.version = 0x0004;
				bool unsync = false;
				for (int i = 0; i < metadata->Length(); i++) {
					SafePointer<DataBlock> frame = ID3_CreateAttribute(metadata, i, unsync);
					frames->Append(*frame);
				}
				if (unsync) header.flags |= 0x80;
				header.size = ID3_WriteLength(frames->Length());
				result->Append(*frames);
				MemoryCopy(result->GetBuffer(), &header, sizeof(header));
				result->Retain();
				return result;
			}
			void ID3_MPEG3_GetMediaRange(Stream * stream, uint64 & start, uint64 & end)
			{
				stream->Seek(0, Begin);
				ID3Header header;
				stream->Read(&header, sizeof(header));
				if (MemoryCompare(header.signature, "ID3", 3) == 0) {
					if (header.version == 0x0004 && (header.flags & 0x10)) start = 20 + ID3_ReadLength(header.size);
					else start = 10 + ID3_ReadLength(header.size);
				} else start = 0;
				stream->Seek(-10, End);
				stream->Read(&header, sizeof(header));
				if (MemoryCompare(header.signature, "3DI", 3) == 0) {
					if (header.version == 0x0004 && (header.flags & 0x10)) end = stream->Length() - 20 - ID3_ReadLength(header.size);
					else end = stream->Length() - 10 - ID3_ReadLength(header.size);
				} else {
					stream->Seek(-128, End);
					stream->Read(&header, sizeof(header));
					if (MemoryCompare(header.signature, "TAG", 3) == 0) end = stream->Length() - 128;
					else end = stream->Length();
				}
			}
			
			SafeArray<MPEG4Atom> * MPEG4_RestoreAtoms(DataBlock * source, int limit, bool expand = true, int * pos_ptr = 0)
			{
				SafePointer< SafeArray<MPEG4Atom> > atoms = new SafeArray<MPEG4Atom>(0x10);
				int pos = pos_ptr ? *pos_ptr : 0;
				while (pos < limit - 7) {
					atoms->Append(MPEG4Atom());
					uint32 size = Network::InverseEndianess(*reinterpret_cast<uint32 *>(source->GetBuffer() + pos));
					MemoryCopy(atoms->LastElement().type, source->GetBuffer() + pos + 4, 4);
					if (size < 8) { if (pos_ptr) *pos_ptr = pos; atoms->Retain(); return atoms; }
					if (expand) {
						pos += 8;
						atoms->LastElement().children = MPEG4_RestoreAtoms(source, pos + size - 8, false, &pos);
					} else {
						atoms->LastElement().contents = new DataBlock(size - 8);
						atoms->LastElement().contents->Append(source->GetBuffer() + pos + 8, size - 8);
						pos += size;
					}
				}
				if (pos_ptr) *pos_ptr = pos;
				atoms->Retain();
				return atoms;
			}
			void MPEG4_AtomReadDataTrackNumber(MPEG4Atom * root, uint32 & number, uint32 & count)
			{
				number = count = 0;
				for (auto & atom : root->children->Elements()) if (MemoryCompare(atom.type, "data", 4) == 0) {
					number = atom.contents->ElementAt(11);
					count = atom.contents->ElementAt(13);
					return;
				}
			}
			string MPEG4_AtomReadDataString(MPEG4Atom * root)
			{
				for (auto & atom : root->children->Elements()) if (MemoryCompare(atom.type, "data", 4) == 0) {
					return string(atom.contents->GetBuffer() + 8, atom.contents->Length() - 8, Encoding::UTF8);
				}
				return L"";
			}
			uint32 MPEG4_AtomReadDataNumber(MPEG4Atom * root)
			{
				for (auto & atom : root->children->Elements()) if (MemoryCompare(atom.type, "data", 4) == 0) {
					auto size = atom.contents->Length() - 8;
					if (size == 1) return atom.contents->ElementAt(8);
					else if (size == 2) return Network::InverseEndianess(*reinterpret_cast<uint16 *>(atom.contents->GetBuffer() + 8));
					else if (size == 4) return Network::InverseEndianess(*reinterpret_cast<uint32 *>(atom.contents->GetBuffer() + 8));
					else return 0;
				}
				return 0;
			}
			Frame * MPEG4_AtomReadDataPicture(MPEG4Atom * root, string & format)
			{
				for (auto & atom : root->children->Elements()) if (MemoryCompare(atom.type, "data", 4) == 0) {
					auto size = atom.contents->Length() - 8;
					try {
						MemoryStream data(0x10000);
						data.Write(atom.contents->GetBuffer() + 8, size);
						data.Seek(0, Begin);
						return DecodeFrame(&data, &format, 0);
					} catch (...) { return 0; }
				}
				return 0;
			}
			Metadata * MPEG4_ReadMetadata(DataBlock * ilst)
			{
				SafePointer<Metadata> result = new Metadata(0x10);
				SafePointer< SafeArray<MPEG4Atom> > atoms = MPEG4_RestoreAtoms(ilst, ilst->Length());
				for (auto & atom : atoms->Elements()) {
					if (MemoryCompare(atom.type, "trkn", 4) == 0) {
						uint32 no, cnt; MPEG4_AtomReadDataTrackNumber(&atom, no, cnt);
						if (no) result->Append(MetadataKey::TrackNumber, no);
						if (cnt) result->Append(MetadataKey::TrackCount, cnt);
					} else if (MemoryCompare(atom.type, "disk", 4) == 0) {
						uint32 no, cnt; MPEG4_AtomReadDataTrackNumber(&atom, no, cnt);
						if (no) result->Append(MetadataKey::DiskNumber, no);
						if (cnt) result->Append(MetadataKey::DiskCount, cnt);
					} else if (MemoryCompare(atom.type, "\251alb", 4) == 0) result->Append(MetadataKey::Album, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251art", 4) == 0) result->Append(MetadataKey::Artist, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251ART", 4) == 0) result->Append(MetadataKey::Artist, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "aART", 4) == 0) result->Append(MetadataKey::AlbumArtist, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251cmt", 4) == 0) result->Append(MetadataKey::Comment, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251day", 4) == 0) result->Append(MetadataKey::Year, MPEG4_AtomReadDataString(&atom).Fragment(0, 4));
					else if (MemoryCompare(atom.type, "\251nam", 4) == 0) result->Append(MetadataKey::Title, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251gen", 4) == 0) result->Append(MetadataKey::Genre, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251wrt", 4) == 0) result->Append(MetadataKey::Composer, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251too", 4) == 0) result->Append(MetadataKey::Encoder, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "cprt", 4) == 0) result->Append(MetadataKey::Copyright, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "desc", 4) == 0) result->Append(MetadataKey::Description, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "\251lyr", 4) == 0) result->Append(MetadataKey::Lyrics, MPEG4_AtomReadDataString(&atom));
					else if (MemoryCompare(atom.type, "tmpo", 4) == 0) result->Append(MetadataKey::BeatsPerMinute, MPEG4_AtomReadDataNumber(&atom));
					else if (MemoryCompare(atom.type, "covr", 4) == 0) {
						string format;
						SafePointer<Frame> picture = MPEG4_AtomReadDataPicture(&atom, format);
						if (picture) result->Append(MetadataKey::Artwork, MetadataValue(picture, format));
					}
					else int p = 6;
				}
				result->Retain();
				return result;
			}
			void MPEG4_LocateAtom(Stream * stream, uint64 max_offset, const string & atom_type, uint64 & begin, uint64 & end)
			{
				auto index = atom_type.FindFirst(L'/');
				auto atom_find = atom_type.Fragment(0, index);
				auto rest = (index >= 0) ? atom_type.Fragment(index + 1, -1) : L"";
				while (max_offset - stream->Seek(0, Current) >= 8) {
					char type[4];
					uint32 size;
					uint64 full_size, header_size;
					stream->Read(&size, 4);
					stream->Read(&type, 4);
					size = Network::InverseEndianess(size);
					if (size == 1) {
						uint64 size64; stream->Read(&size64, 8);
						full_size = ((size64 & 0x00000000000000FF) << 56) | ((size64 & 0x000000000000FF00) << 40) |
							((size64 & 0x0000000000FF0000) << 24) | ((size64 & 0x00000000FF000000) << 8) | ((size64 & 0x000000FF00000000) >> 8) |
							((size64 & 0x0000FF0000000000) >> 24) | ((size64 & 0x00FF000000000000) >> 40) | ((size64 & 0xFF00000000000000) >> 56);
						header_size = 16;
					} else if (size == 0) { full_size = max_offset - stream->Seek(0, Current) + 8; header_size = 8; }
					else if (size >= 8) { full_size = size; header_size = 8; } else { begin = end = 0; return; }
					if (string(&type, 4, Encoding::ANSI) == atom_find) {
						if (rest.Length()) return MPEG4_LocateAtom(stream, stream->Seek(0, Current) + full_size - header_size, rest, begin, end);
						else {
							begin = stream->Seek(0, Current) - header_size;
							end = begin + full_size;
							return;
						}
					} else stream->Seek(full_size - header_size, Current);
				}
				begin = end = 0;
			}
			DataBlock * MPEG4_ReadAtom(Stream * stream, uint64 max_offset, const string & atom_type)
			{
				auto index = atom_type.FindFirst(L'/');
				auto atom_find = atom_type.Fragment(0, index);
				auto rest = (index >= 0) ? atom_type.Fragment(index + 1, -1) : L"";
				if (atom_find == L"-" && max_offset - stream->Seek(0, Current) >= 4) {
					uint32 data;
					stream->Read(&data, 4);
					if (data) return 0;
					return MPEG4_ReadAtom(stream, max_offset, rest);
				}
				while (max_offset - stream->Seek(0, Current) >= 8) {
					char type[4];
					uint32 size;
					uint64 full_size, header_size;
					stream->Read(&size, 4);
					stream->Read(&type, 4);
					size = Network::InverseEndianess(size);
					if (size == 1) {
						uint64 size64; stream->Read(&size64, 8);
						full_size = ((size64 & 0x00000000000000FF) << 56) | ((size64 & 0x000000000000FF00) << 40) |
							((size64 & 0x0000000000FF0000) << 24) | ((size64 & 0x00000000FF000000) << 8) | ((size64 & 0x000000FF00000000) >> 8) |
							((size64 & 0x0000FF0000000000) >> 24) | ((size64 & 0x00FF000000000000) >> 40) | ((size64 & 0xFF00000000000000) >> 56);
						header_size = 16;
					} else if (size == 0) { full_size = max_offset - stream->Seek(0, Current) + 8; header_size = 8; }
					else if (size >= 8) { full_size = size; header_size = 8; } else return 0;
					if (string(&type, 4, Encoding::ANSI) == atom_find) {
						if (rest.Length()) return MPEG4_ReadAtom(stream, stream->Seek(0, Current) + full_size - header_size, rest);
						else {
							SafePointer<DataBlock> result = new DataBlock(0x10000);
							result->SetLength(int(full_size - header_size));
							stream->Read(result->GetBuffer(), result->Length());
							result->Retain();
							return result;
						}
					} else stream->Seek(full_size - header_size, Current);
				}
				return 0;
			}
			void MPEG4_BeginAtom(DataBlock * block, const string & type, Array<uint32> & stack)
			{
				stack << block->Length();
				block->Append(0); block->Append(0); block->Append(0); block->Append(0);
				block->Append(uint8(type[0])); block->Append(uint8(type[1])); block->Append(uint8(type[2])); block->Append(uint8(type[3]));
			}
			void MPEG4_BeginAtom(Stream * stream, const string & type, Array<uint64> & stack, bool long_atom = false)
			{
				stack << stream->Seek(0, Current);
				uint32 size = 0;
				stream->Write(&size, 4);
				type.Fragment(0, 4).Encode(&size, Encoding::ANSI, false);
				stream->Write(&size, 4);
				if (long_atom) { size = 0; stream->Write(&size, 4); stream->Write(&size, 4); }
			}
			void MPEG4_EndAtom(DataBlock * block, Array<uint32> & stack)
			{
				uint32 atom_start = stack.LastElement();
				stack.RemoveLast();
				*reinterpret_cast<uint32 *>(block->GetBuffer() + atom_start) = Network::InverseEndianess(block->Length() - atom_start);
			}
			void MPEG4_EndAtom(Stream * stream, Array<uint64> & stack, bool long_atom = false)
			{
				uint64 atom_start = stack.LastElement();
				stack.RemoveLast();
				uint64 current = stream->Seek(0, Current);
				uint64 length = current - atom_start;
				stream->Seek(atom_start, Begin);
				if (long_atom) {
					uint32 size = 1;
					stream->Write(&size, 4);
					stream->Seek(4, Current);
					size = Network::InverseEndianess(uint32(length >> 32));
					stream->Write(&size, 4);
					size = Network::InverseEndianess(uint32(length));
					stream->Write(&size, 4);
				} else {
					uint32 size = Network::InverseEndianess(uint32(length));
					stream->Write(&size, 4);
				}
				stream->Seek(current, Begin);
			}
			void MPEG4_WriteStringAttribute(DataBlock * block, Array<uint32> & stack, const string & atom, const string & text)
			{
				MPEG4_BeginAtom(block, atom, stack);
				MPEG4_BeginAtom(block, L"data", stack);
				block->Append(0); block->Append(0); block->Append(0); block->Append(1);
				block->Append(0); block->Append(0); block->Append(0); block->Append(0);
				SafePointer<DataBlock> enc = text.EncodeSequence(Encoding::UTF8, false);
				block->Append(*enc);
				MPEG4_EndAtom(block, stack);
				MPEG4_EndAtom(block, stack);
			}
			DataBlock * MPEG4_CreateMetadata(Metadata * metadata)
			{
				SafePointer<DataBlock> result = new DataBlock(0x10000);
				Array<uint32> stack(0x100);
				MPEG4_BeginAtom(result, L"udta", stack);
				MPEG4_BeginAtom(result, L"meta", stack);
				result->Append(0); result->Append(0); result->Append(0); result->Append(0);
				MPEG4_BeginAtom(result, L"hdlr", stack);
				for (int i = 0; i < 8; i++) result->Append(0);
				result->Append('m'); result->Append('d'); result->Append('i'); result->Append('r');
				result->Append('a'); result->Append('p'); result->Append('p'); result->Append('l');
				for (int i = 0; i < 9; i++) result->Append(0);
				MPEG4_EndAtom(result, stack);
				MPEG4_BeginAtom(result, L"ilst", stack);
				for (auto & meta : metadata->Elements()) {
					if (meta.key == MetadataKey::Title) MPEG4_WriteStringAttribute(result, stack, L"\251nam", meta.value.Text);
					else if (meta.key == MetadataKey::Album) MPEG4_WriteStringAttribute(result, stack, L"\251alb", meta.value.Text);
					else if (meta.key == MetadataKey::Artist) MPEG4_WriteStringAttribute(result, stack, L"\251art", meta.value.Text);
					else if (meta.key == MetadataKey::AlbumArtist) {
						MPEG4_WriteStringAttribute(result, stack, L"aART", meta.value.Text);
						MPEG4_WriteStringAttribute(result, stack, L"\251ART", meta.value.Text);
					} else if (meta.key == MetadataKey::Year) MPEG4_WriteStringAttribute(result, stack, L"\251day", meta.value.Text);
					else if (meta.key == MetadataKey::Genre) MPEG4_WriteStringAttribute(result, stack, L"\251gen", meta.value.Text);
					else if (meta.key == MetadataKey::Composer) MPEG4_WriteStringAttribute(result, stack, L"\251wrt", meta.value.Text);
					else if (meta.key == MetadataKey::Encoder) MPEG4_WriteStringAttribute(result, stack, L"\251too", meta.value.Text);
					else if (meta.key == MetadataKey::Copyright) MPEG4_WriteStringAttribute(result, stack, L"cprt", meta.value.Text);
					else if (meta.key == MetadataKey::Description) MPEG4_WriteStringAttribute(result, stack, L"desc", meta.value.Text);
					else if (meta.key == MetadataKey::Comment) MPEG4_WriteStringAttribute(result, stack, L"\251cmt", meta.value.Text);
					else if (meta.key == MetadataKey::Lyrics) MPEG4_WriteStringAttribute(result, stack, L"\251lyr", meta.value.Text);
					else if (meta.key == MetadataKey::TrackNumber) {
						auto count = metadata->ElementByKey(MetadataKey::TrackCount);
						MPEG4_BeginAtom(result, L"trkn", stack);
						MPEG4_BeginAtom(result, L"data", stack);
						for (int i = 0; i < 11; i++) result->Append(0);
						result->Append(meta.value.Number);
						result->Append(0);
						if (count) result->Append(count->Number); else result->Append(0);
						result->Append(0); result->Append(0);
						MPEG4_EndAtom(result, stack); MPEG4_EndAtom(result, stack);
					} else if (meta.key == MetadataKey::DiskNumber) {
						auto count = metadata->ElementByKey(MetadataKey::DiskCount);
						MPEG4_BeginAtom(result, L"disk", stack);
						MPEG4_BeginAtom(result, L"data", stack);
						for (int i = 0; i < 11; i++) result->Append(0);
						result->Append(meta.value.Number);
						result->Append(0);
						if (count) result->Append(count->Number); else result->Append(0);
						result->Append(0); result->Append(0);
						MPEG4_EndAtom(result, stack); MPEG4_EndAtom(result, stack);
					} else if (meta.key == MetadataKey::BeatsPerMinute) {
						MPEG4_BeginAtom(result, L"tmpo", stack);
						MPEG4_BeginAtom(result, L"data", stack);
						for (int i = 0; i < 3; i++) result->Append(0);
						result->Append(0x15);
						for (int i = 0; i < 4; i++) result->Append(0);
						uint32 write = meta.value.Number;
						if (write > 0xFFFF) {
							result->Append(write >> 24); result->Append(write >> 16);
							result->Append(write >> 8); result->Append(write);
						} else if (write > 0xFF) {
							result->Append(write >> 8); result->Append(write);
						} else result->Append(write);
						MPEG4_EndAtom(result, stack); MPEG4_EndAtom(result, stack);
					} else if (meta.key == MetadataKey::Artwork && meta.value.Picture) {
						bool failed = false;
						MemoryStream data(0x10000);
						uint8 type = 13;
						try {
							string format = ImageFormatJPEG;
							if (meta.value.Text = ImageFormatPNG) { type = 14; format = ImageFormatPNG; }
							EncodeFrame(&data, meta.value.Picture, format);
						} catch (...) { failed = true; }
						if (!failed) {
							MPEG4_BeginAtom(result, L"covr", stack);
							MPEG4_BeginAtom(result, L"data", stack);
							for (int i = 0; i < 3; i++) result->Append(0);
							result->Append(type);
							for (int i = 0; i < 4; i++) result->Append(0);
							result->Append(reinterpret_cast<const uint8 *>(data.GetBuffer()), uint32(data.Length()));
							MPEG4_EndAtom(result, stack); MPEG4_EndAtom(result, stack);
						}
					}
				}
				MPEG4_EndAtom(result, stack); MPEG4_EndAtom(result, stack); MPEG4_EndAtom(result, stack);
				result->Retain();
				return result;
			}
			void MPEG4_PatchMovieAtom(DataBlock & moov, const DataBlock * udta)
			{
				int pos = 0;
				while (moov.Length() - pos >= 8) {
					uint32 size = Network::InverseEndianess(*reinterpret_cast<uint32 *>(moov.GetBuffer() + pos));
					if (MemoryCompare(moov.GetBuffer() + pos + 4, "free", 4) == 0 || MemoryCompare(moov.GetBuffer() + pos + 4, "udta", 4) == 0) {
						for (int i = pos + size; i < moov.Length(); i++) moov[i - size] = moov[i];
						moov.SetLength(moov.Length() - size);
					} else pos += size;
				}
				moov.SetLength(pos);
				if (udta) moov.Append(*udta);
			}
			void MPEG4_FindChunkOffsetAtoms(DataBlock & moov, Array<uint32> & starts, Array<uint32> & ends, int pos, int max_pos)
			{
				while (max_pos - pos >= 8) {
					uint32 size = Network::InverseEndianess(*reinterpret_cast<uint32 *>(moov.GetBuffer() + pos));
					if (MemoryCompare(moov.GetBuffer() + pos + 4, "trak", 4) == 0 ||
						MemoryCompare(moov.GetBuffer() + pos + 4, "mdia", 4) == 0 ||
						MemoryCompare(moov.GetBuffer() + pos + 4, "minf", 4) == 0 ||
						MemoryCompare(moov.GetBuffer() + pos + 4, "stbl", 4) == 0) {
						MPEG4_FindChunkOffsetAtoms(moov, starts, ends, pos + 8, pos + size);
					} else if (MemoryCompare(moov.GetBuffer() + pos + 4, "stco", 4) == 0 ||
						MemoryCompare(moov.GetBuffer() + pos + 4, "co64", 4) == 0) {
						starts << pos;
						ends << (pos + size);
					}
					pos += size;
				}
			}
			void MPEG4_RelocateChunks(DataBlock & moov, int64 delta)
			{
				Array<uint32> starts(0x10), ends(0x10);
				MPEG4_FindChunkOffsetAtoms(moov, starts, ends, 0, moov.Length());
				for (int i = 0; i < starts.Length(); i++) {
					if (MemoryCompare(moov.GetBuffer() + starts[i] + 4, "stco", 4) == 0) {
						uint32 num_entries = Network::InverseEndianess(*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 12));
						if (num_entries) for (uint j = 0; j < num_entries; j++) {
							uint32 offset = Network::InverseEndianess(*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 16 + 4 * j));
							offset += int32(delta);
							*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 16 + 4 * j) = Network::InverseEndianess(offset);
						}
					} else if (MemoryCompare(moov.GetBuffer() + starts[i] + 4, "co64", 4) == 0) {
						uint32 num_entries = Network::InverseEndianess(*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 12));
						if (num_entries) for (uint j = 0; j < num_entries; j++) {
							uint32 offset_hi = Network::InverseEndianess(*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 16 + 8 * j));
							uint32 offset_lo = Network::InverseEndianess(*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 20 + 8 * j));
							uint64 offset = uint64(offset_lo) | (uint64(offset_hi) << 32);
							offset += delta;
							*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 16 + 8 * j) = Network::InverseEndianess(uint32(offset >> 32));
							*reinterpret_cast<uint32 *>(moov.GetBuffer() + starts[i] + 20 + 8 * j) = Network::InverseEndianess(uint32(offset));
						}
					}
				}
			}

			Metadata * FLAC_ReadMetadata(Stream * stream)
			{
				stream->Seek(4, Begin);
				SafePointer<Metadata> result = new Metadata(0x10);
				uint32 header = 0;
				while (!(header & 0x00000080)) {
					stream->Read(&header, 4);
					uint64 current = stream->Seek(0, Current);
					uint32 length = ((header & 0xFF000000) >> 24) | ((header & 0x00FF0000) >> 8) | ((header & 0x0000FF00) << 8);
					if ((header & 0x0000007F) == 0x04) {
						uint32 vendor_length, num_tags;
						stream->Read(&vendor_length, 4);
						if (vendor_length) {
							DataBlock vendor(vendor_length);
							vendor.SetLength(vendor_length);
							stream->Read(vendor.GetBuffer(), vendor.Length());
							result->Append(MetadataKey::Encoder, string(vendor.GetBuffer(), vendor.Length(), Encoding::UTF8));
						}
						stream->Read(&num_tags, 4);
						for (uint i = 0; i < num_tags; i++) {
							uint32 tag_length;
							stream->Read(&tag_length, 4);
							DataBlock contents(tag_length);
							contents.SetLength(tag_length);
							stream->Read(contents.GetBuffer(), contents.Length());
							string tag = string(contents.GetBuffer(), contents.Length(), Encoding::UTF8);
							int index = tag.FindFirst(L'=');
							if (index < 0) continue;
							string key = tag.Fragment(0, index);
							string value = tag.Fragment(index + 1, -1);
							try {
								if (key == L"TITLE") result->Append(MetadataKey::Title, value);
								else if (key == L"ALBUM") result->Append(MetadataKey::Album, value);
								else if (key == L"TRACKNUMBER") result->Append(MetadataKey::TrackNumber, value.ToUInt32());
								else if (key == L"DISCNUMBER") result->Append(MetadataKey::DiskNumber, value.ToUInt32());
								else if (key == L"TRACKTOTAL" || key == L"TOTALTRACKS") result->Append(MetadataKey::TrackCount, value.ToUInt32());
								else if (key == L"DISCTOTAL" || key == L"TOTALDISCS") result->Append(MetadataKey::DiskCount, value.ToUInt32());
								else if (key == L"ARTIST") result->Append(MetadataKey::Artist, value);
								else if (key == L"ALBUMARTIST" || key == L"BAND") result->Append(MetadataKey::AlbumArtist, value);
								else if (key == L"COPYRIGHT") result->Append(MetadataKey::Copyright, value);
								else if (key == L"DESCRIPTION") result->Append(MetadataKey::Description, value);
								else if (key == L"COMMENT") result->Append(MetadataKey::Comment, value);
								else if (key == L"GENRE") result->Append(MetadataKey::Genre, value);
								else if (key == L"DATE") result->Append(MetadataKey::Year, value);
								else if (key == L"COMPOSER") result->Append(MetadataKey::Composer, value);
								else if (key == L"LYRICS") result->Append(MetadataKey::Lyrics, value);
								else if (key == L"PUBLISHER") result->Append(MetadataKey::Publisher, value);
							} catch (...) {}
						}
					} else if ((header & 0x0000007F) == 0x06) {
						uint32 type, mime_type_length, desc_length;
						stream->Read(&type, 4);
						stream->Read(&mime_type_length, 4);
						stream->Seek(Network::InverseEndianess(mime_type_length), Current);
						stream->Read(&desc_length, 4);
						uint32 pic_desc_size = Network::InverseEndianess(mime_type_length) + Network::InverseEndianess(desc_length) + 32;
						try {
							FragmentStream data(stream, current + pic_desc_size, length - pic_desc_size);
							string format;
							SafePointer<Frame> picture = DecodeFrame(&data, &format, 0);
							if (picture) result->Append(MetadataKey::Artwork, MetadataValue(picture, format));
						} catch (...) {}
					}
					stream->Seek(current + length, Begin);
				}
				result->Retain();
				return result;
			}
			
			void RelocateMedia(Stream * stream, uint64 media_start, uint64 media_end, uint64 header_size)
			{
				uint64 media_size = media_end - media_start;
				if (header_size > media_start) {
					stream->SetLength(media_size + header_size);
					DataBlock block(0x10000);
					block.SetLength(0x100000);
					uint64 data_left = media_size;
					while (data_left) {
						uint32 current = uint32(min(data_left, uint64(block.Length())));
						stream->Seek(media_start + data_left - current, Begin);
						stream->Read(block.GetBuffer(), current);
						stream->Seek(header_size + data_left - current, Begin);
						stream->Write(block.GetBuffer(), current);
						data_left -= current;
					}
				} else if (header_size < media_start) {
					DataBlock block(0x10000);
					block.SetLength(0x100000);
					uint64 data_left = media_size;
					while (data_left) {
						uint32 current = uint32(min(data_left, uint64(block.Length())));
						stream->Seek(media_end - data_left, Begin);
						stream->Read(block.GetBuffer(), current);
						stream->Seek(header_size + media_size - data_left, Begin);
						stream->Write(block.GetBuffer(), current);
						data_left -= current;
					}
					stream->SetLength(media_size + header_size);
				} else stream->SetLength(media_end);
			}
		};

		using namespace FileFormats;

		bool EncodeMetadata(Streaming::Stream * stream, Metadata * metadata, string * metadata_format)
		{
			try {
				uint8 sign[8];
				stream->Seek(0, Begin);
				stream->Read(sign, 8);
				if (sign[0] == 0xFF || MemoryCompare(sign, "ID3", 3) == 0) {
					uint64 media_start, media_end;
					ID3_MPEG3_GetMediaRange(stream, media_start, media_end);
					SafePointer<DataBlock> block = metadata ? ID3_Create(metadata) : 0;
					uint32 meta_size = block ? block->Length() : 0;
					RelocateMedia(stream, media_start, media_end, meta_size);
					if (block) {
						stream->Seek(0, Begin);
						stream->WriteArray(block);
					}
					return true;
				} else if (MemoryCompare(sign + 4, "ftyp", 4) == 0) {
					SafePointer<DataBlock> udta = metadata ? MPEG4_CreateMetadata(metadata) : 0;
					stream->Seek(0, Begin);
					SafePointer<DataBlock> ftyp = MPEG4_ReadAtom(stream, stream->Length(), L"ftyp");
					stream->Seek(0, Begin);
					SafePointer<DataBlock> moov = MPEG4_ReadAtom(stream, stream->Length(), L"moov");
					stream->Seek(0, Begin);
					uint64 mdat_begin, mdat_end;
					MPEG4_LocateAtom(stream, stream->Length(), L"mdat", mdat_begin, mdat_end);
					MPEG4_PatchMovieAtom(*moov, udta);
					udta.SetReference(0);
					int64 header_size = moov->Length() + ftyp->Length() + 16;
					MPEG4_RelocateChunks(*moov, header_size - int64(mdat_begin));
					RelocateMedia(stream, mdat_begin, mdat_end, header_size);
					Array<uint64> stack(10);
					stream->Seek(0, Begin);
					MPEG4_BeginAtom(stream, L"ftyp", stack);
					stream->WriteArray(ftyp);
					MPEG4_EndAtom(stream, stack);
					MPEG4_BeginAtom(stream, L"moov", stack);
					stream->WriteArray(moov);
					MPEG4_EndAtom(stream, stack);
					if (metadata_format) *metadata_format = MetadataFormatMPEG4iTunes;
					return true;
				} else return false;
			} catch (...) { return false; }
		}
		bool EncodeMetadata(Streaming::Stream * source, Streaming::Stream * dest, Metadata * metadata, string * metadata_format)
		{
			try {
				uint8 sign[8];
				source->Seek(0, Begin);
				source->Read(sign, 8);
				if (sign[0] == 0xFF || MemoryCompare(sign, "ID3", 3) == 0) {
					uint64 media_start, media_end;
					ID3_MPEG3_GetMediaRange(source, media_start, media_end);
					SafePointer<DataBlock> block = metadata ? ID3_Create(metadata) : 0;
					if (block) dest->WriteArray(block);
					source->Seek(media_start, Begin);
					source->CopyTo(dest, media_end - media_start);
					if (metadata_format) *metadata_format = MetadataFormatMPEG3ID3;
					return true;
				} else if (MemoryCompare(sign + 4, "ftyp", 4) == 0) {
					SafePointer<DataBlock> udta = metadata ? MPEG4_CreateMetadata(metadata) : 0;
					source->Seek(0, Begin);
					SafePointer<DataBlock> ftyp = MPEG4_ReadAtom(source, source->Length(), L"ftyp");
					source->Seek(0, Begin);
					SafePointer<DataBlock> moov = MPEG4_ReadAtom(source, source->Length(), L"moov");
					source->Seek(0, Begin);
					uint64 mdat_begin, mdat_end;
					MPEG4_LocateAtom(source, source->Length(), L"mdat", mdat_begin, mdat_end);
					MPEG4_PatchMovieAtom(*moov, udta);
					udta.SetReference(0);
					int64 header_size = moov->Length() + ftyp->Length() + 16;
					MPEG4_RelocateChunks(*moov, header_size - int64(mdat_begin));
					Array<uint64> stack(10);
					MPEG4_BeginAtom(dest, L"ftyp", stack);
					dest->WriteArray(ftyp);
					MPEG4_EndAtom(dest, stack);
					MPEG4_BeginAtom(dest, L"moov", stack);
					dest->WriteArray(moov);
					MPEG4_EndAtom(dest, stack);
					ftyp.SetReference(0); moov.SetReference(0);
					source->Seek(mdat_begin, Begin);
					source->CopyTo(dest, mdat_end - mdat_begin);
					if (metadata_format) *metadata_format = MetadataFormatMPEG4iTunes;
					return true;
				} else return false;
			} catch (...) { return false; }
		}
		Metadata * DecodeMetadata(Streaming::Stream * stream, string * metadata_format)
		{
			try {
				ID3Header header;
				stream->Seek(0, Begin);
				stream->Read(&header, sizeof(header));
				if (MemoryCompare(header.signature, "ID3", 3) == 0) return ID3_Read(stream, header, 0, metadata_format);
				if (stream->Length() > 10) {
					stream->Seek(-10, End);
					stream->Read(&header, sizeof(header));
					if (MemoryCompare(header.signature, "3DI", 3) == 0) {
						auto size = ID3_ReadLength(header.size);
						return ID3_Read(stream, header, stream->Length() - size - 20, metadata_format);
					}
				}
				char sign[8];
				stream->Seek(0, Begin);
				stream->Read(sign, 8);
				if (MemoryCompare(sign + 4, "ftyp", 4) == 0) {
					stream->Seek(0, Begin);
					SafePointer<DataBlock> meta = MPEG4_ReadAtom(stream, stream->Length(), L"moov/udta/meta/-/ilst");
					if (!meta) return 0;
					if (metadata_format) *metadata_format = MetadataFormatMPEG4iTunes;
					return MPEG4_ReadMetadata(meta);
				}
				if (MemoryCompare(sign, "fLaC", 4) == 0) {
					if (metadata_format) *metadata_format = MetadataFormatFreeLossless;
					return FLAC_ReadMetadata(stream);
				}
				return 0;
			} catch (...) { return 0; }
		}
	}
}