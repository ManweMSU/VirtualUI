#include "AudioStorage.h"
#include "Audio.h"

#include "../Miscellaneous/DynamicString.h"
#include "../Interfaces/Socket.h"
#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace Media
	{
		namespace Format
		{
			ENGINE_PACKED_STRUCTURE(id3_header)
				char signature[3];
				uint16 version;
				uint8 flags;
				uint32 size;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(id3_frame_v2)
				char name[3];
				uint8 size_hi;
				uint8 size_md;
				uint8 size_lo;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(id3_frame_v3v4)
				char name[4];
				uint32 size;
				uint16 flags;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(id3_header_v1)
				char signature[3];
				char name[30];
				char artist[30];
				char album[30];
				char year[4];
				char comment[30];
				uint8 genre;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mp3_frame_header)
				uint syncword_a : 8;
				uint checksum : 1;
				uint layer : 2;
				uint version : 2;
				uint syncword_b : 3;
				uint user : 1;
				uint padding : 1;
				uint framerate : 2;
				uint bitrate : 4;
				uint emphasis : 2;
				uint original : 1;
				uint copyright : 1;
				uint extension : 2;
				uint channels : 2;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(flac_stream_info)
				uint16 min_block_size;
				uint16 max_block_size;
				uint16 min_frame_size_lo;
				uint8 min_frame_size_hi;
				uint16 max_frame_size_lo;
				uint8 max_frame_size_hi;
				uint16 frame_rate_lo;
				uint8 frame_rate_hi_num_channels_bit_per_sample_lo;
				uint8 bit_per_sample_hi_num_samples_lo;
				uint32 num_samples_hi;
				uint64 hash_lo;
				uint64 hash_hi;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(image_probe_header)
				union {
					struct {
						uint16 start_of_image;
						uint16 application_use_marker;
						uint16 application_use_length;
						uint8 format_identifier[5];
						uint16 format_version;
						uint8 units;
						uint16 width;
						uint16 height;
						uint8 thumb_width;
						uint8 thumb_height;
					} jpeg;
					struct {
						uint8 signature[8];
						uint32 header_chunk_length;
						uint32 header_chunk_type;
						uint32 width;
						uint32 height;
						uint8 color_depth;
						uint8 channel_layout;
						uint8 compression;
						uint8 filter;
						uint8 interlace;
					} png;
				};
			ENGINE_END_PACKED_STRUCTURE
		}

		class EngineMP3ContainerSink : public IMediaContainerSink
		{
			friend class EngineMP3ContainerSource;

			static uint32 _id3_write_length(uint32 length)
			{
				auto i = (length & 0x7F) | ((length & 0x3F80) << 1) | ((length & 0x1FC000) << 2) | ((length & 0x0FE00000) << 3);
				return Network::InverseEndianess(i);
			}
			static bool _id3_unsynchronize(DataBlock * block)
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
			static DataBlock * _id3_create_attribute(const Metadata * metadata, int index, bool & unsync)
			{
				auto & attr = metadata->ElementAt(index)->GetValue();
				SafePointer<DataBlock> result = new DataBlock(0x1000);
				Format::id3_frame_v3v4 frame;
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
						if (_id3_unsynchronize(result)) { frame.flags |= 0x0200; unsync = true; }
						frame.size = _id3_write_length(result->Length() - sizeof(frame));
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
						if (attr.key == MetadataKey::Genre) {
							auto genre_index = metadata->GetElementByKey(MetadataKey::GenreIndex);
							if (genre_index) output.Insert(L"(" + string(genre_index->Number) + L")", 0);
						}
						SafePointer<DataBlock> encoded = output.ToString().EncodeSequence(Encoding::UTF8, false);
						auto length = 1 + encoded->Length();
						result->SetLength(length + sizeof(frame));
						ZeroMemory(result->GetBuffer(), sizeof(frame));
						result->ElementAt(sizeof(frame)) = 3;
						MemoryCopy(result->GetBuffer() + sizeof(frame) + 1, encoded->GetBuffer(), encoded->Length());
						if (_id3_unsynchronize(result)) { frame.flags |= 0x0200; unsync = true; }
						frame.size = _id3_write_length(result->Length() - sizeof(frame));
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
						auto frac = metadata->GetElementByKey(MetadataKey::TrackCount);
						if (frac) output += L"/" + string(frac->Number);
					} else if (attr.key == MetadataKey::DiskNumber) {
						MemoryCopy(frame.name, "TPOS", 4);
						output = string(attr.value.Number);
						auto frac = metadata->GetElementByKey(MetadataKey::DiskCount);
						if (frac) output += L"/" + string(frac->Number);
					} else if (attr.key == MetadataKey::BeatsPerMinute) {
						MemoryCopy(frame.name, "TBPM", 4);
						output = string(attr.value.Number);
					} else { result->SetLength(0); result->Retain(); return result; }
					SafePointer<DataBlock> encoded = output.EncodeSequence(Encoding::UTF8, false);
					auto length = 1 + encoded->Length();
					frame.size = _id3_write_length(length);
					result->SetLength(length + sizeof(frame));
					ZeroMemory(result->GetBuffer(), sizeof(frame));
					result->ElementAt(sizeof(frame)) = 3;
					MemoryCopy(result->GetBuffer() + sizeof(frame) + 1, encoded->GetBuffer(), encoded->Length());
					MemoryCopy(result->GetBuffer(), &frame, sizeof(frame));
				} else if (MetadataKeyIsPicture(attr.key)) {
					if (attr.key == MetadataKey::Artwork && attr.value.Picture) {
						MemoryCopy(frame.name, "APIC", 4);
						string mime_type;
						if (attr.value.Text == Codec::ImageFormatPNG) mime_type = L"image/png";
						else mime_type = L"image/jpeg";
						uint len = mime_type.Length();
						uint size = uint(attr.value.Picture->Length() + 4 + len);
						result->SetLength(sizeof(frame) + size);
						ZeroMemory(result->GetBuffer(), sizeof(frame));
						result->ElementAt(sizeof(frame)) = 0x00;
						mime_type.Encode(result->GetBuffer() + sizeof(frame) + 1, Encoding::ANSI, true);
						result->ElementAt(sizeof(frame) + len + 2) = 0x03;
						result->ElementAt(sizeof(frame) + len + 3) = 0x00;
						MemoryCopy(result->GetBuffer() + sizeof(frame) + len + 4, attr.value.Picture->GetBuffer(), attr.value.Picture->Length());
						if (_id3_unsynchronize(result)) { frame.flags |= 0x0200; unsync = true; }
						frame.size = _id3_write_length(result->Length() - sizeof(frame));
						MemoryCopy(result->GetBuffer(), &frame, sizeof(frame));
					}
				}
				result->Retain();
				return result;
			}
			static DataBlock * _id3_create(const Metadata * metadata)
			{
				if (!metadata->GetRoot()) return 0;
				SafePointer<DataBlock> result = new DataBlock(0x10000);
				SafePointer<DataBlock> frames = new DataBlock(0x10000);
				Format::id3_header header;
				result->SetLength(sizeof(header));
				header.signature[0] = 'I'; header.signature[1] = 'D'; header.signature[2] = '3';
				header.flags = 0; header.version = 0x0004;
				bool unsync = false;
				auto count = metadata->Count();
				for (int i = 0; i < count; i++) {
					SafePointer<DataBlock> frame = _id3_create_attribute(metadata, i, unsync);
					frames->Append(*frame);
				}
				if (unsync) header.flags |= 0x80;
				header.size = _id3_write_length(frames->Length());
				result->Append(*frames);
				MemoryCopy(result->GetBuffer(), &header, sizeof(header));
				result->Retain();
				return result;
			}

			class EngineMP3TrackSink : public IMediaTrackSink
			{
				friend class EngineMP3ContainerSink;

				IMediaContainerSink * _sink;
				SafePointer<IMediaContainerCodec> _codec;
				SafePointer<Streaming::Stream> _stream;
				SafePointer<TrackFormatDesc> _desc;
				bool _sealed;
			public:
				EngineMP3TrackSink(void) : _sink(0), _sealed(false) {}
				virtual ~EngineMP3TrackSink(void) override {}
				virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Sink; }
				virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
				virtual IMediaContainer * GetParentContainer(void) const noexcept override { return _sink; }
				virtual TrackClass GetTrackClass(void) const noexcept override { return TrackClass::Audio; }
				virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept override { return *_desc; }
				virtual string GetTrackName(void) const override { return L""; }
				virtual string GetTrackLanguage(void) const override { return L""; }
				virtual bool IsTrackVisible(void) const noexcept override { return true; }
				virtual bool IsTrackAutoselectable(void) const noexcept override { return true; }
				virtual int GetTrackGroup(void) const noexcept override { return 0; }
				virtual bool SetTrackName(const string & name) noexcept override { return true; }
				virtual bool SetTrackLanguage(const string & language) noexcept override { return true; }
				virtual void MakeTrackVisible(bool make) noexcept override {}
				virtual void MakeTrackAutoselectable(bool make) noexcept override {}
				virtual void SetTrackGroup(int group) noexcept override {}
				virtual bool WritePacket(const PacketBuffer & buffer) noexcept override
				{
					if (_sealed) return false;
					try {
						if (buffer.PacketDataActuallyUsed) _stream->Write(buffer.PacketData->GetBuffer(), buffer.PacketDataActuallyUsed);
						return true;
					} catch (...) { return false; }
				}
				virtual bool UpdateCodecMagic(const DataBlock * data) noexcept override { return true; }
				virtual bool Sealed(void) const noexcept override { return _sealed; }
				virtual bool Finalize(void) noexcept override
				{
					if (_sealed) return false;
					_sealed = true;
					if (_sink->IsAutofinalizable()) return _sink->Finalize();
					else return true;
				}
			};

			SafePointer<IMediaContainerCodec> _codec;
			SafePointer<Streaming::Stream> _stream;
			SafePointer<Metadata> _metadata;
			SafePointer<EngineMP3TrackSink> _track;
			ContainerClassDesc _desc;
			bool _sealed, _autofinalize;
		public:
			EngineMP3ContainerSink(IMediaContainerCodec * codec, Streaming::Stream * stream, const ContainerClassDesc & desc) : _desc(desc), _sealed(false), _autofinalize(false)
			{
				_codec.SetRetain(codec);
				_stream.SetRetain(stream);
			}
			virtual ~EngineMP3ContainerSink(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Sink; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept override { return _desc; }
			virtual Metadata * ReadMetadata(void) const noexcept override { try { return _metadata ? CloneMetadata(_metadata) : 0; } catch (...) { return 0; } }
			virtual int GetTrackCount(void) const noexcept override { return _track ? 1 : 0; }
			virtual IMediaTrack * GetTrack(int index) const noexcept override { if (index) return 0; return _track; }
			virtual void WriteMetadata(const Metadata * metadata) noexcept override { try { _metadata = metadata ? CloneMetadata(metadata) : 0; } catch (...) {} }
			virtual IMediaTrackSink * CreateTrack(const TrackFormatDesc & desc) noexcept override
			{
				if (_sealed || _track) return 0;
				if (desc.GetTrackClass() == TrackClass::Audio && desc.GetTrackCodec() == Audio::AudioFormatMP3) {
					try {
						_track = new EngineMP3TrackSink;
						_track->_sink = this;
						_track->_codec.SetRetain(_codec);
						_track->_stream.SetRetain(_stream);
						_track->_desc = desc.Clone();
						_track->Retain();
						return _track;
					} catch (...) { _track.SetReference(0); return 0; }
				} else return 0;
			}
			virtual void SetAutofinalize(bool set) noexcept override { _autofinalize = set; }
			virtual bool IsAutofinalizable(void) const noexcept override { return _autofinalize; }
			virtual bool Finalize(void) noexcept override
			{
				if (_sealed || !_track || !_track->_sealed) return false;
				try {
					SafePointer<DataBlock> data = _metadata ? _id3_create(_metadata) : 0;
					uint64 relocate = data ? data->Length() : 0;
					uint64 media_size = _stream->Seek(0, Streaming::Current);
					_stream->SetLength(media_size + relocate);
					_stream->RelocateData(0, relocate, media_size);
					if (data) {
						_stream->Seek(0, Streaming::Begin);
						_stream->WriteArray(data);
					}
					_sealed = true;
					return true;
				} catch (...) { return false; }
			}
		};
		class EngineMP3ContainerSource : public IMediaContainerSource
		{
			static uint32 _id3_read_length(uint32 length)
			{
				auto i = Network::InverseEndianess(length);
				return (i & 0x7F) | ((i & 0x7F00) >> 1) | ((i & 0x7F0000) >> 2) | ((i & 0x7F000000) >> 3);
			}
			static DataBlock * _id3_remove_unsynchronisation(const DataBlock * block)
			{
				SafePointer<DataBlock> regular = new DataBlock(0x10000);
				for (int i = 0; i < block->Length(); i++) {
					regular->Append(block->ElementAt(i));
					if (i < block->Length() - 1 && block->ElementAt(i) == 0xFF && !block->ElementAt(i + 1)) i++;
				}
				regular->Retain();
				return regular;
			}
			static string _id3_decode_string(uint8 enc, uint8 * buffer, uint32 size, uint32 * term)
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
					if (buffer[0] == 0xFE && buffer[1] == 0xFF) {
						int i = 0;
						while (i < size - 1 && buffer[i] && buffer[i + 1]) {
							swap(buffer[i], buffer[i + 1]);
							i += 2;
						}
					}
					if (buffer[0] == 0xFF && buffer[1] == 0xFE) {
						return string(buffer + 2, pos / 2 - 1, Encoding::UTF16);
					} else throw InvalidFormatException();
				} else if (enc == 2) {
					uint pos = 0;
					while (pos < size - 1 && (buffer[pos] || buffer[pos + 1])) pos += 2;
					if (term) *term = pos + 2;
					int i = 0;
					while (i < size - 1 && buffer[i] && buffer[i + 1]) {
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
			static string _id3_decode_string(uint8 * buffer, uint32 size) { return _id3_decode_string(buffer[0], buffer + 1, size - 1, 0); }
			static void _id3_read_attributes(Metadata & metadata, uint32 version, const uint8 * data, uint32 data_size)
			{
				uint32 pos = 0;
				uint32 hdr_size = (version == 2) ? sizeof(Format::id3_frame_v2) : sizeof(Format::id3_frame_v3v4);
				while (pos <= data_size - hdr_size) {
					const Format::id3_frame_v3v4 * hdr_v3v4 = 0;
					const Format::id3_frame_v2 * hdr_v2 = 0;
					uint32 length;
					if (version == 2) {
						hdr_v2 = reinterpret_cast<const Format::id3_frame_v2 *>(data + pos);
						length = (uint32(hdr_v2->size_hi) << 16) | (uint32(hdr_v2->size_md) << 8) | hdr_v2->size_lo;
						if (!hdr_v2->name[0]) break;
					} else {
						hdr_v3v4 = reinterpret_cast<const Format::id3_frame_v3v4 *>(data + pos);
						if (version == 3) length = Network::InverseEndianess(hdr_v3v4->size);
						else length = _id3_read_length(hdr_v3v4->size);
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
					if (frame_unsync) frame = _id3_remove_unsynchronisation(frame);
					if ((hdr_v2 && hdr_v2->name[0] == 'T') || (hdr_v3v4 && hdr_v3v4->name[0] == 'T')) {
						bool skip = false;
						if (hdr_v2 && MemoryCompare(hdr_v2->name, "TXX", 3) == 0) skip = true;
						if (hdr_v3v4 && MemoryCompare(hdr_v3v4->name, "TXXX", 4) == 0) skip = true;
						if (!skip) {
							auto code = hdr_v3v4 ? string(hdr_v3v4->name, 4, Encoding::ANSI) : string(hdr_v2->name, 3, Encoding::ANSI);
							auto string = _id3_decode_string(frame->GetBuffer(), frame->Length());
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
										int sp = pos;
										while (pos < string.Length() && string[pos] != L')') pos++;
										try {
											auto genre = string.Fragment(sp, pos - sp).ToUInt32();
											metadata.Append(MetadataKey::GenreIndex, genre);
										} catch (...) {}
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
						auto shr = _id3_decode_string(frame->ElementAt(0), frame->GetBuffer() + 4, frame->Length() - 4, &end);
						auto lng = _id3_decode_string(frame->ElementAt(0), frame->GetBuffer() + 4 + end, frame->Length() - 4 - end, &end);
						metadata.Append(MetadataKey::Comment, lng);
					} else if ((hdr_v2 && MemoryCompare(hdr_v2->name, "PIC", 3) == 0) || (hdr_v3v4 && MemoryCompare(hdr_v3v4->name, "APIC", 4) == 0)) {
						uint8 enc = frame->ElementAt(0);
						uint32 type_end, desc_end;
						string type;
						if (version >= 3) type = _id3_decode_string(0, frame->GetBuffer() + 1, frame->Length() - 1, &type_end);
						else type_end = 3;
						uint8 pic_type = frame->ElementAt(type_end + 1);
						auto pic_desc = _id3_decode_string(enc, frame->GetBuffer() + type_end + 2, frame->Length() - 2 - type_end, &desc_end);
						try {
							string pic_fmt = Codec::ImageFormatJPEG;
							if (type == L"image/png") pic_fmt = Codec::ImageFormatPNG;
							SafePointer<DataBlock> pic_data = new DataBlock(frame->Length() - type_end - 2 - desc_end);
							pic_data->Append(frame->GetBuffer() + type_end + 2 + desc_end, frame->Length() - type_end - 2 - desc_end);
							metadata.Append(MetadataKey::Artwork, MetadataValue(pic_data, pic_fmt));
						} catch (...) {}
					} else if ((hdr_v2 && MemoryCompare(hdr_v2->name, "ULT", 3) == 0) || (hdr_v3v4 && MemoryCompare(hdr_v3v4->name, "USLT", 4) == 0)) {
						uint32 end;
						auto desc = _id3_decode_string(frame->ElementAt(0), frame->GetBuffer() + 4, frame->Length() - 4, &end);
						auto lyr = _id3_decode_string(frame->ElementAt(0), frame->GetBuffer() + 4 + end, frame->Length() - 4 - end, &end);
						metadata.Append(MetadataKey::Lyrics, lyr);
					}
					pos += length;
				}
			}
			static Metadata * _id3_read(Streaming::Stream * stream, const Format::id3_header & header, uint64 base)
			{
				stream->Seek(base + sizeof(header), Streaming::Begin);
				uint32 version = 0;
				if (header.version == 0x0002) version = 2;
				else if (header.version == 0x0003) version = 3;
				else if (header.version == 0x0004) version = 4;
				else return 0;
				if (version == 2 && (header.flags & 0x40)) return 0;
				auto length = _id3_read_length(header.size);
				SafePointer<DataBlock> frames = new DataBlock(0x100);
				frames->SetLength(length);
				stream->Read(frames->GetBuffer(), length);
				if (header.flags & 0x80 && version < 4) frames = _id3_remove_unsynchronisation(frames);
				uint32 read_pos = 0;
				if (version >= 3 && (header.flags & 0x40)) {
					if (frames->Length() < 4) throw InvalidFormatException();
					auto ex_length = _id3_read_length(*reinterpret_cast<uint32 *>(frames->GetBuffer()));
					if (frames->Length() < int(ex_length)) throw InvalidFormatException();
					read_pos = ex_length;
				}
				SafePointer<Metadata> result = new Metadata;
				_id3_read_attributes(*result, version, frames->GetBuffer() + read_pos, frames->Length() - read_pos);
				result->Retain();
				return result;
			}

			struct EngineMP3Packet {
				uint64 origin, first_frame, last_frame;
				uint length;
			};
			class EngineMP3TrackSource : public IMediaTrackSource
			{
			public:
				IMediaContainerSource * _container;
				SafePointer<Streaming::Stream> _stream;
				SafePointer<IMediaContainerCodec> _codec;
				SafePointer<AudioTrackFormatDesc> _track;
				Array<EngineMP3Packet> _packets;
				uint64 _length_frames, _current_frame;
				uint _current_packet;
				Audio::StreamDesc _desc;

				EngineMP3TrackSource(void) : _container(0), _length_frames(0), _current_frame(0), _current_packet(0), _packets(0x10000) {}
				virtual ~EngineMP3TrackSource(void) override {}
				virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
				virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
				virtual IMediaContainer * GetParentContainer(void) const noexcept override { return _container; }
				virtual TrackClass GetTrackClass(void) const noexcept override { return TrackClass::Audio; }
				virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept override { return *_track; }
				virtual string GetTrackName(void) const override { return L""; }
				virtual string GetTrackLanguage(void) const override { return L""; }
				virtual bool IsTrackVisible(void) const noexcept override { return true; }
				virtual bool IsTrackAutoselectable(void) const noexcept override { return true; }
				virtual int GetTrackGroup(void) const noexcept override { return 0; }
				virtual uint64 GetTimeScale(void) const noexcept override { return _desc.FramesPerSecond; }
				virtual uint64 GetDuration(void) const noexcept override { return _length_frames; }
				virtual uint64 GetPosition(void) const noexcept override { return _current_frame; }
				virtual uint64 Seek(uint64 time) noexcept override
				{
					uint32 lt = time;
					int pm = 0, px = _packets.Length() - 1;
					while (px > pm) {
						int pc = (px + pm + 1) / 2;
						if (_packets[pc].first_frame > lt) px = max(pc - 1, 0); else pm = pc;
					}
					_current_packet = pm;
					_current_frame = _packets[pm].first_frame;
					return _current_frame;
				}
				virtual uint64 GetCurrentPacket(void) const noexcept override { return _current_packet; }
				virtual uint64 GetPacketCount(void) const noexcept override { return _packets.Length(); }
				virtual bool ReadPacket(PacketBuffer & buffer) noexcept override
				{
					if (_current_packet < _packets.Length()) {
						try {
							auto & packet = _packets[_current_packet];
							if (!buffer.PacketData || buffer.PacketData->Length() < packet.length) {
								buffer.PacketData = new DataBlock(packet.length);
								buffer.PacketData->SetLength(packet.length);
							}
							buffer.PacketDataActuallyUsed = packet.length;
							_stream->Seek(packet.origin, Streaming::Begin);
							_stream->Read(buffer.PacketData->GetBuffer(), packet.length);
							buffer.PacketIsKey = true;
							buffer.PacketDecodeTime = buffer.PacketRenderTime = packet.first_frame;
							buffer.PacketRenderDuration = packet.last_frame - packet.first_frame;
							_current_packet++;
							_current_frame = _current_packet < _packets.Length() ? _packets[_current_packet].first_frame : _length_frames;
							return true;
						} catch (...) { return false; }
					} else {
						buffer.PacketDataActuallyUsed = 0;
						buffer.PacketIsKey = true;
						buffer.PacketDecodeTime = buffer.PacketRenderTime = GetDuration();
						buffer.PacketRenderDuration = 0;
						return true;
					}
				}
			};

			const int _mpeg1_l1_bps[16] = { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 };
			const int _mpeg1_l2_bps[16] = { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0 };
			const int _mpeg1_l3_bps[16] = { 0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0 };
			const int _mpeg2_l1_bps[16] = { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 0 };
			const int _mpeg2_l2_bps[16] = { 0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0 };

			SafePointer<IMediaContainerCodec> _codec;
			SafePointer<Streaming::Stream> _stream;
			SafePointer<Metadata> _metadata;
			SafePointer<EngineMP3TrackSource> _track;
			ContainerClassDesc _desc;
			uint64 _media_start, _media_end;
		public:
			EngineMP3ContainerSource(IMediaContainerCodec * codec, Streaming::Stream * stream, const ContainerClassDesc & desc)
			{
				_codec.SetRetain(codec);
				_stream.SetRetain(stream);
				_desc = desc;
				Format::id3_header header;
				stream->Read(&header, sizeof(header));
				if (MemoryCompare(header.signature, "ID3", 3) == 0) {
					if (header.version == 0x0004 && (header.flags & 0x10)) _media_start = 20 + _id3_read_length(header.size);
					else _media_start = 10 + _id3_read_length(header.size);
					_metadata = _id3_read(stream, header, 0);
				} else _media_start = 0;
				stream->Seek(-10, Streaming::End);
				stream->Read(&header, sizeof(header));
				if (MemoryCompare(header.signature, "3DI", 3) == 0) {
					if (header.version == 0x0004 && (header.flags & 0x10)) _media_end = stream->Length() - 20 - _id3_read_length(header.size);
					else _media_end = stream->Length() - 10 - _id3_read_length(header.size);
					if (!_metadata) _metadata = _id3_read(stream, header, stream->Length() - 20 - _id3_read_length(header.size));
				} else {
					Format::id3_header_v1 header_v1;
					stream->Seek(-128, Streaming::End);
					stream->Read(&header_v1, sizeof(header_v1));
					if (MemoryCompare(header_v1.signature, "TAG", 3) == 0) {
						_media_end = stream->Length() - 128;
						if (!_metadata) {
							_metadata = new Metadata;
							_metadata->Append(MetadataKey::Title, string(header_v1.name, 30, Encoding::ANSI));
							_metadata->Append(MetadataKey::Artist, string(header_v1.artist, 30, Encoding::ANSI));
							_metadata->Append(MetadataKey::Album, string(header_v1.album, 30, Encoding::ANSI));
							_metadata->Append(MetadataKey::Year, string(header_v1.year, 4, Encoding::ANSI));
							_metadata->Append(MetadataKey::Comment, string(header_v1.comment, 30, Encoding::ANSI));
							_metadata->Append(MetadataKey::GenreIndex, uint32(header_v1.genre));
						}
					} else _media_end = stream->Length();
				}
				_track = new EngineMP3TrackSource;
				_track->_container = this;
				_track->_codec.SetRetain(_codec);
				_track->_stream.SetRetain(_stream);
				_track->_desc.Format = Audio::SampleFormat::Invalid;
				Format::mp3_frame_header frame_header;
				stream->Seek(_media_start, Streaming::Begin);
				stream->Read(&frame_header, sizeof(frame_header));
				uint cc = (frame_header.channels == 3) ? 1 : 2;
				uint cl = (cc == 2) ? (Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight) : Audio::ChannelLayoutCenter;
				_track->_desc.ChannelCount = cc;
				if (frame_header.version == 0) { // MPEG 2.5
					if (frame_header.framerate == 0) _track->_desc.FramesPerSecond = 11025;
					else if (frame_header.framerate == 1) _track->_desc.FramesPerSecond = 12000;
					else if (frame_header.framerate == 2) _track->_desc.FramesPerSecond = 8000;
					else throw InvalidFormatException();
				} else if (frame_header.version == 3) { // MPEG 1
					if (frame_header.framerate == 0) _track->_desc.FramesPerSecond = 44100;
					else if (frame_header.framerate == 1) _track->_desc.FramesPerSecond = 48000;
					else if (frame_header.framerate == 2) _track->_desc.FramesPerSecond = 32000;
					else throw InvalidFormatException();
				} else { // MPEG 2
					if (frame_header.framerate == 0) _track->_desc.FramesPerSecond = 22050;
					else if (frame_header.framerate == 1) _track->_desc.FramesPerSecond = 24000;
					else if (frame_header.framerate == 2) _track->_desc.FramesPerSecond = 16000;
					else throw InvalidFormatException();
				}
				_track->_track = new AudioTrackFormatDesc(Audio::AudioFormatMP3, _track->_desc, cl);
				auto cp = _media_start;
				uint64 frame_counter = 0;
				while (cp < _media_end - 3) {
					stream->Seek(cp, Streaming::Begin);
					stream->Read(&frame_header, sizeof(frame_header));
					if (frame_header.syncword_a != 0xFF || frame_header.syncword_b != 0x07) break;
					uint fpp, bps;
					if (frame_header.version == 0) { // MPEG 2.5
						if (frame_header.layer == 3) { // Layer I
							fpp = 384;
							bps = _mpeg2_l1_bps[frame_header.bitrate];
						} else if (frame_header.layer == 2) { // Layer II
							fpp = 1152;
							bps = _mpeg2_l2_bps[frame_header.bitrate];
						} else if (frame_header.layer == 1) { // Layer III
							fpp = 576;
							bps = _mpeg2_l2_bps[frame_header.bitrate];
						} else break;
					} else if (frame_header.version == 3) { // MPEG 1
						if (frame_header.layer == 3) { // Layer I
							fpp = 384;
							bps = _mpeg1_l1_bps[frame_header.bitrate];
						} else if (frame_header.layer == 2) { // Layer II
							fpp = 1152;
							bps = _mpeg1_l2_bps[frame_header.bitrate];
						} else if (frame_header.layer == 1) { // Layer III
							fpp = 1152;
							bps = _mpeg1_l3_bps[frame_header.bitrate];
						} else break;
					} else { // MPEG 2
						if (frame_header.layer == 3) { // Layer I
							fpp = 384;
							bps = _mpeg2_l1_bps[frame_header.bitrate];
						} else if (frame_header.layer == 2) { // Layer II
							fpp = 1152;
							bps = _mpeg2_l2_bps[frame_header.bitrate];
						} else if (frame_header.layer == 1) { // Layer III
							fpp = 576;
							bps = _mpeg2_l2_bps[frame_header.bitrate];
						} else break;
					}
					if (!bps) break;
					uint sf, sl;
					if (frame_header.layer == 3) { // Layer I
						sf = 12;
						sl = 4;
					} else if (frame_header.layer == 2) { // Layer II
						sf = 144;
						sl = 1;
					} else if (frame_header.layer == 1) { // Layer III
						sf = 144;
						sl = 1;
					} else break;
					uint size = sl * (sf * bps * 1000 / _track->_desc.FramesPerSecond + frame_header.padding);
					EngineMP3Packet packet;
					packet.origin = cp;
					packet.first_frame = frame_counter;
					packet.length = size;
					packet.last_frame = frame_counter + fpp;
					_track->_packets << packet;
					frame_counter += fpp;
					cp += size;
				}
				if (!_track->_packets.Length()) throw InvalidFormatException();
				_track->_length_frames = _track->_packets.LastElement().last_frame;
			}
			virtual ~EngineMP3ContainerSource(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept override { return _desc; }
			virtual Metadata * ReadMetadata(void) const noexcept override { if (_metadata) { try { return CloneMetadata(_metadata); } catch (...) {} } return 0; }
			virtual int GetTrackCount(void) const noexcept override { return 1; }
			virtual IMediaTrack * GetTrack(int index) const noexcept override { if (index) return 0; return _track; }
			virtual IMediaTrackSource * OpenTrack(int index) const noexcept override { if (index) return 0; _track->Retain(); return _track; }
			virtual uint64 GetDuration(void) const noexcept override { return (_track->GetDuration() * 1000 + _track->GetTimeScale() - 1) / _track->GetTimeScale(); }
			virtual bool PatchMetadata(const Metadata * metadata) noexcept override
			{
				try {
					_metadata.SetReference(0);
					if (metadata) _metadata = CloneMetadata(metadata);
					SafePointer<DataBlock> data = _metadata ? EngineMP3ContainerSink::_id3_create(_metadata) : 0;
					uint64 media_new_offset = data ? data->Length() : 0;
					uint64 media_relocation = media_new_offset - _media_start;
					if (media_new_offset > _media_start) {
						_stream->SetLength(_media_end + media_relocation);
						_stream->RelocateData(_media_start, media_new_offset, _media_end - _media_start);
					} else if (media_new_offset < _media_start) {
						_stream->RelocateData(_media_start, media_new_offset, _media_end - _media_start);
						_stream->SetLength(_media_end + media_relocation);
					} else _stream->SetLength(_media_end + media_relocation);
					if (data) {
						_stream->Seek(0, Streaming::Begin);
						_stream->WriteArray(data);
					}
					for (auto & packet : _track->_packets) packet.origin += media_relocation;
					_media_start += media_relocation;
					_media_end += media_relocation;
					return true;
				} catch (...) { return false; }
			}
			virtual bool PatchMetadata(const Metadata * metadata, Streaming::Stream * dest) const noexcept override
			{
				try {
					SafePointer<DataBlock> data = metadata ? EngineMP3ContainerSink::_id3_create(metadata) : 0;
					dest->SetLength(0);
					dest->Seek(0, Streaming::Begin);
					if (data) dest->WriteArray(data);
					_stream->Seek(_media_start, Streaming::Begin);
					_stream->CopyTo(dest, _media_end - _media_start);
					return true;
				} catch (...) { return false; }
			}
		};
		class EngineFLACContainerSource : public IMediaContainerSource
		{
			static void _read_core_metadata(Streaming::Stream * stream, Metadata * metadata)
			{
				uint32 vendor_length, num_tags;
				stream->Read(&vendor_length, 4);
				if (vendor_length) {
					DataBlock vendor(vendor_length);
					vendor.SetLength(vendor_length);
					stream->Read(vendor.GetBuffer(), vendor.Length());
					metadata->Append(MetadataKey::Encoder, string(vendor.GetBuffer(), vendor.Length(), Encoding::UTF8));
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
						if (key == L"TITLE") metadata->Append(MetadataKey::Title, value);
						else if (key == L"ALBUM") metadata->Append(MetadataKey::Album, value);
						else if (key == L"TRACKNUMBER") metadata->Append(MetadataKey::TrackNumber, value.ToUInt32());
						else if (key == L"DISCNUMBER") metadata->Append(MetadataKey::DiskNumber, value.ToUInt32());
						else if (key == L"TRACKTOTAL" || key == L"TOTALTRACKS") metadata->Append(MetadataKey::TrackCount, value.ToUInt32());
						else if (key == L"DISCTOTAL" || key == L"TOTALDISCS") metadata->Append(MetadataKey::DiskCount, value.ToUInt32());
						else if (key == L"ARTIST") metadata->Append(MetadataKey::Artist, value);
						else if (key == L"ALBUMARTIST" || key == L"BAND") metadata->Append(MetadataKey::AlbumArtist, value);
						else if (key == L"COPYRIGHT") metadata->Append(MetadataKey::Copyright, value);
						else if (key == L"DESCRIPTION") metadata->Append(MetadataKey::Description, value);
						else if (key == L"COMMENT") metadata->Append(MetadataKey::Comment, value);
						else if (key == L"GENRE") metadata->Append(MetadataKey::Genre, value);
						else if (key == L"DATE") metadata->Append(MetadataKey::Year, value);
						else if (key == L"COMPOSER") metadata->Append(MetadataKey::Composer, value);
						else if (key == L"LYRICS") metadata->Append(MetadataKey::Lyrics, value);
						else if (key == L"PUBLISHER") metadata->Append(MetadataKey::Publisher, value);
					} catch (...) {}
				}
			}
			static void _read_artwork_metadata(Streaming::Stream * stream, Metadata * metadata)
			{
				DataBlock mime_type(0x10);
				uint32 type, mime_type_length, desc_length;
				stream->Read(&type, 4);
				stream->Read(&mime_type_length, 4);
				mime_type.SetLength(Network::InverseEndianess(mime_type_length));
				stream->Read(mime_type.GetBuffer(), mime_type.Length());
				stream->Read(&desc_length, 4);
				string mime = string(mime_type.GetBuffer(), mime_type.Length(), Encoding::ANSI);
				string format = (mime == L"image/png") ? Codec::ImageFormatPNG : Codec::ImageFormatJPEG;
				stream->Seek(Network::InverseEndianess(desc_length) + 16, Streaming::Current);
				stream->Read(&desc_length, 4);
				desc_length = Network::InverseEndianess(desc_length);
				SafePointer<DataBlock> picture = new DataBlock(desc_length);
				picture->SetLength(desc_length);
				stream->Read(picture->GetBuffer(), desc_length);
				metadata->Append(MetadataKey::Artwork, MetadataValue(picture, format));
			}
			static uint64 _read_ucs_word(const DataBlock & data, int & i)
			{
				if (data.Length() <= i) throw Exception();
				if (data[i] & 0x80) {
					uint64 result;
					int size;
					if ((data[i] & 0xE0) == 0xC0) {
						size = 2;
						result = data[i] & 0x1F;
					} else if ((data[i] & 0xF0) == 0xE0) {
						size = 3;
						result = data[i] & 0x0F;
					} else if ((data[i] & 0xF8) == 0xF0) {
						size = 4;
						result = data[i] & 0x07;
					} else if ((data[i] & 0xFC) == 0xF8) {
						size = 5;
						result = data[i] & 0x03;
					} else if ((data[i] & 0xFE) == 0xFC) {
						size = 6;
						result = data[i] & 0x01;
					} else if ((data[i] & 0xFF) == 0xFE) {
						size = 7;
						result = 0;
					} else throw Exception();
					i++;
					for (int j = 0; j < size - 1; j++) {
						if (data.Length() <= i) throw Exception();
						if ((data[i] & 0xC0) != 0x80) throw Exception();
						result <<= 6;
						result |= uint64(data[i] & 0x3F);
						i++;
					}
					return result;
				} else {
					i++;
					return data[i - 1] & 0x7F;
				}
			}
			static int _first_one(uint16 word)
			{
				int result = 0;
				if (!word) return -1;
				while (word > 1) { word >>= 1; result++; }
				return result;
			}
			static uint8 _crc8(const DataBlock & data, int i, int j, uint16 div)
			{
				int bits_rest = (j - i - 2) * 8;
				int padding = 8;
				int ci = i + 2;
				uint16 block = (uint16(data[i]) << 8) | data[i + 1];
				while (bits_rest || padding || block > 0xFF) {
					if (!(block & 0x8000) && bits_rest) {
						auto src = data[ci];
						auto bi = ((bits_rest - 1) % 8);
						block <<= 1;
						block |= (src >> bi) & 0x01;
						bits_rest--;
						if (bi == 0) ci++;
					} else if (!(block & 0x8000) && padding) {
						block <<= 1;
						padding--;
					} else {
						auto fo_block = _first_one(block);
						auto fo_div = _first_one(div);
						auto div_shifted = div << (fo_block - fo_div);
						block ^= div_shifted;
					}
				}
				return block;
			}

			static DataBlock * _create_header_block(const Format::flac_stream_info & header, bool encode_as_final)
			{
				SafePointer<DataBlock> result = new DataBlock(0x100);
				result->SetLength(8 + sizeof(header));
				MemoryCopy(result->GetBuffer(), "fLaC", 4);
				result->ElementAt(4) = encode_as_final ? 0x80 : 0x00;
				result->ElementAt(5) = 0;
				result->ElementAt(6) = 0;
				result->ElementAt(7) = sizeof(header);
				MemoryCopy(result->GetBuffer() + 8, &header, sizeof(header));
				result->Retain();
				return result;
			}
			static DataBlock * _create_core_metadata_block(const Metadata * metadata, bool encode_as_final)
			{
				if (!metadata) return 0;
				uint num_encoded = 0;
				SafePointer<DataBlock> result = new DataBlock(0x100);
				result->SetLength(4);
				result->ElementAt(0) = encode_as_final ? 0x84 : 0x04;
				auto encoder = metadata->GetElementByKey(MetadataKey::Encoder);
				if (encoder && encoder->Text.Length()) {
					SafePointer<DataBlock> value = encoder->Text.EncodeSequence(Encoding::UTF8, false);
					result->Append(value->Length() & 0xFF);
					result->Append((value->Length() >> 8) & 0xFF);
					result->Append((value->Length() >> 16) & 0xFF);
					result->Append((value->Length() >> 24) & 0xFF);
					result->Append(*value);
					num_encoded++;
				} else for (int i = 0; i < 4; i++) result->Append(0);
				uint num_tags_offset = result->Length();
				uint num_tags = 0;
				for (int i = 0; i < 4; i++) result->Append(0);
				for (auto & tag : metadata->Elements()) {
					string tag_encode;
					if (tag.key == MetadataKey::Album) tag_encode = L"ALBUM=" + tag.value.Text;
					else if (tag.key == MetadataKey::AlbumArtist) tag_encode = L"ALBUMARTIST=" + tag.value.Text;
					else if (tag.key == MetadataKey::Artist) tag_encode = L"ARTIST=" + tag.value.Text;
					else if (tag.key == MetadataKey::Comment) tag_encode = L"COMMENT=" + tag.value.Text;
					else if (tag.key == MetadataKey::Composer) tag_encode = L"COMPOSER=" + tag.value.Text;
					else if (tag.key == MetadataKey::Copyright) tag_encode = L"COPYRIGHT=" + tag.value.Text;
					else if (tag.key == MetadataKey::Description) tag_encode = L"DESCRIPTION=" + tag.value.Text;
					else if (tag.key == MetadataKey::Genre) tag_encode = L"GENRE=" + tag.value.Text;
					else if (tag.key == MetadataKey::Lyrics) tag_encode = L"LYRICS=" + tag.value.Text;
					else if (tag.key == MetadataKey::Publisher) tag_encode = L"PUBLISHER=" + tag.value.Text;
					else if (tag.key == MetadataKey::Title) tag_encode = L"TITLE=" + tag.value.Text;
					else if (tag.key == MetadataKey::Year) tag_encode = L"DATE=" + tag.value.Text;
					else if (tag.key == MetadataKey::TrackNumber) tag_encode = L"TRACKNUMBER=" + string(tag.value.Number);
					else if (tag.key == MetadataKey::DiskNumber) tag_encode = L"DISCNUMBER=" + string(tag.value.Number);
					else if (tag.key == MetadataKey::TrackCount) tag_encode = L"TRACKTOTAL=" + string(tag.value.Number);
					else if (tag.key == MetadataKey::DiskCount) tag_encode = L"DISCTOTAL=" + string(tag.value.Number);
					if (tag_encode.Length()) {
						SafePointer<DataBlock> value = tag_encode.EncodeSequence(Encoding::UTF8, false);
						result->Append(value->Length() & 0xFF);
						result->Append((value->Length() >> 8) & 0xFF);
						result->Append((value->Length() >> 16) & 0xFF);
						result->Append((value->Length() >> 24) & 0xFF);
						result->Append(*value);
						num_encoded++;
						num_tags++;
					}
				}
				MemoryCopy(result->GetBuffer() + num_tags_offset, &num_tags, 4);
				uint length = result->Length() - 4;
				result->ElementAt(1) = uint8(length >> 16);
				result->ElementAt(2) = uint8(length >> 8);
				result->ElementAt(3) = uint8(length);
				if (num_encoded) {
					result->Retain();
					return result;
				} else return 0;
			}
			static DataBlock * _create_artwork_metadata_block(const Metadata * metadata, bool encode_as_final)
			{
				if (!metadata) return 0;
				auto value = metadata->GetElementByKey(MetadataKey::Artwork);
				if (value) {
					SafePointer<DataBlock> result = new DataBlock(0x10000);
					result->SetLength(4);
					result->ElementAt(0) = encode_as_final ? 0x86 : 0x06;
					result->Append(0);
					result->Append(0);
					result->Append(0);
					result->Append(3);
					uint width, height, bpp;
					string mime;
					auto & ihdr = *reinterpret_cast<Format::image_probe_header *>(value->Picture->GetBuffer());
					if (ihdr.jpeg.start_of_image == 0xD8FF && ihdr.jpeg.application_use_marker == 0xE0FF) {
						mime = L"image/jpeg";
						bpp = 24;
						width = height = 0x100;
						for (int i = 2; i < value->Picture->Length() - 1; i++) {
							if (value->Picture->ElementAt(i) == 0xFF && value->Picture->ElementAt(i + 1) == 0xC0) {
								height = (uint(value->Picture->ElementAt(i + 5))) << 8 | value->Picture->ElementAt(i + 6);
								width = (uint(value->Picture->ElementAt(i + 7))) << 8 | value->Picture->ElementAt(i + 8);
								break;
							} else if (value->Picture->ElementAt(i) == 0xFF && value->Picture->ElementAt(i + 1) == 0xC2) {
								height = (uint(value->Picture->ElementAt(i + 5))) << 8 | value->Picture->ElementAt(i + 6);
								width = (uint(value->Picture->ElementAt(i + 7))) << 8 | value->Picture->ElementAt(i + 8);
								break;
							}
						}
					} else if (MemoryCompare(&ihdr.png.signature, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8) == 0) {
						mime = L"image/png";
						width = Network::InverseEndianess(ihdr.png.width);
						height = Network::InverseEndianess(ihdr.png.height);
						bpp = ihdr.png.color_depth;
						if (ihdr.png.channel_layout == 2) bpp *= 3;
						else if (ihdr.png.channel_layout == 4) bpp *= 2;
						else if (ihdr.png.channel_layout == 6) bpp *= 4;
					} else throw Exception();
					SafePointer<DataBlock> mime_enc = mime.EncodeSequence(Encoding::ANSI, false);
					result->Append(uint8(mime_enc->Length() >> 24));
					result->Append(uint8(mime_enc->Length() >> 16));
					result->Append(uint8(mime_enc->Length() >> 8));
					result->Append(uint8(mime_enc->Length()));
					result->Append(*mime_enc);
					for (int i = 0; i < 4; i++) result->Append(0);
					result->Append(uint8(width >> 24));
					result->Append(uint8(width >> 16));
					result->Append(uint8(width >> 8));
					result->Append(uint8(width));
					result->Append(uint8(height >> 24));
					result->Append(uint8(height >> 16));
					result->Append(uint8(height >> 8));
					result->Append(uint8(height));
					result->Append(uint8(bpp >> 24));
					result->Append(uint8(bpp >> 16));
					result->Append(uint8(bpp >> 8));
					result->Append(uint8(bpp));
					for (int i = 0; i < 4; i++) result->Append(0);
					result->Append(uint8(value->Picture->Length() >> 24));
					result->Append(uint8(value->Picture->Length() >> 16));
					result->Append(uint8(value->Picture->Length() >> 8));
					result->Append(uint8(value->Picture->Length()));
					result->Append(*value->Picture);
					uint length = result->Length() - 4;
					result->ElementAt(1) = uint8(length >> 16);
					result->ElementAt(2) = uint8(length >> 8);
					result->ElementAt(3) = uint8(length);
					result->Retain();
					return result;
				} else return 0;
			}

			struct EngineFLACPacket
			{
				uint64 offset;
				uint length;
				uint64 first_frame, last_frame;
			};
			class EngineFLACTrackSource : public IMediaTrackSource
			{
				friend class EngineFLACContainerSource;

				Format::flac_stream_info _info;
				Audio::StreamDesc _desc;
				IMediaContainerSource * _source;
				SafePointer<IMediaContainerCodec> _codec;
				SafePointer<Streaming::Stream> _stream;
				SafePointer<AudioTrackFormatDesc> _track;
				Array<EngineFLACPacket> _packets;
				uint64 _duration, _current_frame;
				uint _current_packet;
			public:
				EngineFLACTrackSource(void) : _source(0), _packets(0x10000), _duration(0), _current_frame(0), _current_packet(0) {}
				virtual ~EngineFLACTrackSource(void) override {}
				virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
				virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
				virtual IMediaContainer * GetParentContainer(void) const noexcept override { return _source; }
				virtual TrackClass GetTrackClass(void) const noexcept override { return TrackClass::Audio; }
				virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept override { return *_track; }
				virtual string GetTrackName(void) const override { return L""; }
				virtual string GetTrackLanguage(void) const override { return L""; }
				virtual bool IsTrackVisible(void) const noexcept override { return true; }
				virtual bool IsTrackAutoselectable(void) const noexcept override { return true; }
				virtual int GetTrackGroup(void) const noexcept override { return 0; }
				virtual uint64 GetTimeScale(void) const noexcept override { return _desc.FramesPerSecond; }
				virtual uint64 GetDuration(void) const noexcept override { return _duration; }
				virtual uint64 GetPosition(void) const noexcept override { return _current_frame; }
				virtual uint64 Seek(uint64 time) noexcept override
				{
					uint32 lt = time;
					int pm = 0, px = _packets.Length() - 1;
					while (px > pm) {
						int pc = (px + pm + 1) / 2;
						if (_packets[pc].first_frame > lt) px = max(pc - 1, 0); else pm = pc;
					}
					_current_packet = pm;
					_current_frame = _packets[pm].first_frame;
					return _current_frame;
				}
				virtual uint64 GetCurrentPacket(void) const noexcept override { return _current_packet; }
				virtual uint64 GetPacketCount(void) const noexcept override { return _packets.Length(); }
				virtual bool ReadPacket(PacketBuffer & buffer) noexcept override
				{
					if (_current_packet < _packets.Length()) {
						try {
							auto & packet = _packets[_current_packet];
							if (!buffer.PacketData || buffer.PacketData->Length() < packet.length) {
								buffer.PacketData = new DataBlock(packet.length);
								buffer.PacketData->SetLength(packet.length);
							}
							buffer.PacketDataActuallyUsed = packet.length;
							_stream->Seek(packet.offset, Streaming::Begin);
							_stream->Read(buffer.PacketData->GetBuffer(), packet.length);
							buffer.PacketIsKey = true;
							buffer.PacketDecodeTime = buffer.PacketRenderTime = packet.first_frame;
							buffer.PacketRenderDuration = packet.last_frame - packet.first_frame;
							_current_packet++;
							_current_frame = _current_packet < _packets.Length() ? _packets[_current_packet].first_frame : _duration;
							return true;
						} catch (...) { return false; }
					} else {
						buffer.PacketDataActuallyUsed = 0;
						buffer.PacketIsKey = true;
						buffer.PacketDecodeTime = buffer.PacketRenderTime = GetDuration();
						buffer.PacketRenderDuration = 0;
						return true;
					}
				}
			};

			const uint _flac_fps_list[12] = { 0, 88200, 176400, 192000, 8000, 16000, 22050, 24000, 32000, 44100, 48000, 96000 };
			const uint _flac_bps_list[8] = { 0, 8, 12, 0xFF, 16, 20, 24, 0xFF };

			SafePointer<IMediaContainerCodec> _codec;
			SafePointer<Streaming::Stream> _stream;
			SafePointer<EngineFLACTrackSource> _track;
			SafePointer<Metadata> _metadata;
			ContainerClassDesc _desc;
			uint64 _media_base, _media_end;
		public:
			EngineFLACContainerSource(IMediaContainerCodec * codec, Streaming::Stream * stream, const ContainerClassDesc & desc)
			{
				_codec.SetRetain(codec);
				_stream.SetRetain(stream);
				_desc = desc;
				bool info_header_initialized = false;
				Format::flac_stream_info info;
				uint32 header = 0;
				while (!(header & 0x00000080)) {
					_stream->Read(&header, 4);
					uint64 current = _stream->Seek(0, Streaming::Current);
					uint32 length = ((header & 0xFF000000) >> 24) | ((header & 0x00FF0000) >> 8) | ((header & 0x0000FF00) << 8);
					if ((header & 0x0000007F) == 0x00) {
						if (length < sizeof(info)) throw InvalidFormatException();
						_stream->Read(&info, sizeof(info));
						info_header_initialized = true;
					} else if ((header & 0x0000007F) == 0x04) {
						if (!_metadata) _metadata = new Metadata;
						_read_core_metadata(_stream, _metadata);
					} else if ((header & 0x0000007F) == 0x06) {
						if (!_metadata) _metadata = new Metadata;
						_read_artwork_metadata(_stream, _metadata);
					}
					stream->Seek(current + length, Streaming::Begin);
				}
				if (!info_header_initialized) throw InvalidFormatException();
				uint bps = ((uint32(info.frame_rate_hi_num_channels_bit_per_sample_lo & 0x01) << 4) |
					((info.bit_per_sample_hi_num_samples_lo & 0xF0) >> 4)) + 1;
				uint64 length = (uint64(info.bit_per_sample_hi_num_samples_lo & 0x0F) << 32) | Network::InverseEndianess(info.num_samples_hi);
				_track = new EngineFLACTrackSource;
				_track->_source = this;
				_track->_codec.SetRetain(_codec);
				_track->_stream.SetRetain(_stream);
				_track->_info = info;
				_track->_desc.FramesPerSecond = (uint32(Network::InverseEndianess(info.frame_rate_lo)) << 4) |
					((info.frame_rate_hi_num_channels_bit_per_sample_lo & 0xF0) >> 4);
				_track->_desc.ChannelCount = ((info.frame_rate_hi_num_channels_bit_per_sample_lo & 0x0E) >> 1) + 1;
				if (bps == 8) _track->_desc.Format = Audio::SampleFormat::S8_snorm;
				else if (bps == 16) _track->_desc.Format = Audio::SampleFormat::S16_snorm;
				else if (bps == 24) _track->_desc.Format = Audio::SampleFormat::S24_snorm;
				else if (bps == 32) _track->_desc.Format = Audio::SampleFormat::S32_snorm;
				else throw InvalidFormatException();
				uint channel_layout = 0;
				if (_track->_desc.ChannelCount == 1) channel_layout = Audio::ChannelLayoutCenter;
				else if (_track->_desc.ChannelCount == 2) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight;
				else if (_track->_desc.ChannelCount == 3) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter;
				else if (_track->_desc.ChannelCount == 4) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutBackLeft |
					Audio::ChannelLayoutBackRight;
				else if (_track->_desc.ChannelCount == 5) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
					Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight;
				else if (_track->_desc.ChannelCount == 6) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
					Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight | Audio::ChannelLayoutLowFrequency;
				else if (_track->_desc.ChannelCount == 7) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
					Audio::ChannelLayoutBackCenter | Audio::ChannelLayoutLowFrequency |
					Audio::ChannelLayoutSideLeft | Audio::ChannelLayoutSideRight;
				else if (_track->_desc.ChannelCount == 8) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
					Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight | Audio::ChannelLayoutLowFrequency |
					Audio::ChannelLayoutSideLeft | Audio::ChannelLayoutSideRight;
				_track->_track = new AudioTrackFormatDesc(Audio::AudioFormatFreeLossless, _track->_desc, channel_layout);
				_media_base = _stream->Seek(0, Streaming::Current);
				_media_end = _stream->Length();
				bool var_fpp = info.max_block_size != info.min_block_size;
				uint max_fpp = Network::InverseEndianess(info.max_block_size);
				uint min_frame = (uint32(Network::InverseEndianess(info.min_frame_size_lo)) << 8) | info.min_frame_size_hi;
				uint max_frame = (uint32(Network::InverseEndianess(info.max_frame_size_lo)) << 8) | info.max_frame_size_hi;
				uint64 cp = _media_base;
				DataBlock buffer(max_frame + 16);
				while (cp <= _media_end - min_frame) {
					buffer.SetLength(min(uint64(max_frame) + 16, _media_end - cp));
					_stream->Seek(cp, Streaming::Begin);
					_stream->Read(buffer.GetBuffer(), buffer.Length());
					bool packet_located = false;
					for (int i = 0; i < buffer.Length() - 3; i++) if (buffer[i] == 0xFF && ((buffer[i + 1] & 0xFE) == 0xF8)) {
						int j = i + 1;
						bool local_var_fpp = buffer[j] & 0x01;
						uint block_size_i = (buffer[j + 1] & 0xF0) >> 4;
						uint frame_rate_i = (buffer[j + 1] & 0x0F);
						uint bps_i = (buffer[j + 2] & 0x0E) >> 1;
						if (buffer[j + 2] & 0x01) continue;
						uint frame_rate;
						uint frames_in_packet;
						uint first_frame_index;
						uint bits_per_channel = _flac_bps_list[bps_i];
						if (bits_per_channel && bits_per_channel != Audio::SampleFormatBitSize(_track->_desc.Format)) continue;
						if (local_var_fpp != var_fpp) continue;
						j += 3;
						try {
							if (var_fpp) {
								first_frame_index = _read_ucs_word(buffer, j);
							} else {
								first_frame_index = _read_ucs_word(buffer, j) * max_fpp;
							}
						} catch (...) { continue; }
						if (block_size_i == 0x01) {
							frames_in_packet = 192;
						} else if (block_size_i >= 0x02 && block_size_i <= 0x05) {
							int p = block_size_i - 2;
							frames_in_packet = 576;
							while (p) { frames_in_packet <<= 1; p--; }
						} else if (block_size_i == 0x06) {
							if (j >= buffer.Length()) continue;
							frames_in_packet = uint32(buffer[j]) + 1;
							j++;
						} else if (block_size_i == 0x07) {
							if (j >= buffer.Length() - 1) continue;
							frames_in_packet = ((uint32(buffer[j]) << 8) | buffer[j + 1]) + 1;
							j += 2;
						} else if (block_size_i >= 0x08 && block_size_i <= 0x0F) {
							int p = block_size_i - 8;
							frames_in_packet = 256;
							while (p) { frames_in_packet <<= 1; p--; }
						} else continue;
						if (frame_rate_i < 12) {
							frame_rate = _flac_fps_list[frame_rate_i];
						} else {
							if (frame_rate_i == 0x0C) {
								if (j >= buffer.Length()) continue;
								frame_rate = uint32(buffer[j]) * 1000;
								j++;
							} else if (frame_rate_i == 0x0D) {
								if (j >= buffer.Length() - 1) continue;
								frame_rate = ((uint32(buffer[j]) << 8) | buffer[j + 1]);
								j += 2;
							} else if (frame_rate_i == 0x0E) {
								if (j >= buffer.Length() - 1) continue;
								frame_rate = ((uint32(buffer[j]) << 8) | buffer[j + 1]) * 10;
								j += 2;
							} else continue;
						}
						if (frame_rate && frame_rate != _track->_desc.FramesPerSecond) continue;
						if (j >= buffer.Length()) continue;
						auto eval_crc = _crc8(buffer, i, j, 0x0107);
						auto crc = buffer[j];
						j++;
						if (crc != eval_crc) continue;
						EngineFLACPacket packet;
						packet.offset = cp + i;
						packet.first_frame = first_frame_index;
						packet.last_frame = first_frame_index + frames_in_packet;
						packet.length = 0;
						_track->_packets << packet;
						cp += j;
						packet_located = true;
						break;
					}
					if (!packet_located) throw Exception();
				}
				for (int i = 0; i < _track->_packets.Length() - 1; i++) {
					_track->_packets[i].length = _track->_packets[i + 1].offset - _track->_packets[i].offset;
				}
				_track->_packets.LastElement().length = _media_end - _track->_packets.LastElement().offset;
				_track->_duration = _track->_packets.LastElement().last_frame;
				SafePointer<DataBlock> magic = new DataBlock(sizeof(info));
				magic->SetLength(sizeof(info));
				MemoryCopy(magic->GetBuffer(), &info, sizeof(info));
				_track->_track->SetCodecMagic(magic);
			}
			virtual ~EngineFLACContainerSource(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept override { return _desc; }
			virtual Metadata * ReadMetadata(void) const noexcept override { try { return _metadata ? CloneMetadata(_metadata) : 0; } catch (...) { return 0; } }
			virtual int GetTrackCount(void) const noexcept override { return 1; }
			virtual IMediaTrack * GetTrack(int index) const noexcept override { if (index) return 0; return _track; }
			virtual IMediaTrackSource * OpenTrack(int index) const noexcept override { if (index) return 0; _track->Retain(); return _track; }
			virtual uint64 GetDuration(void) const noexcept override { return (_track->GetDuration() * 1000 + _track->GetTimeScale() - 1) / _track->GetTimeScale(); }
			virtual bool PatchMetadata(const Metadata * metadata) noexcept override
			{
				try {
					_metadata.SetReference(0);
					if (metadata) _metadata = CloneMetadata(metadata);
					SafePointer<DataBlock> art = _create_artwork_metadata_block(_metadata, true);
					SafePointer<DataBlock> core = _create_core_metadata_block(_metadata, !art);
					SafePointer<DataBlock> hdr = _create_header_block(_track->_info, !art && !core);
					uint64 new_base = 0;
					if (art) new_base += art->Length();
					if (core) new_base += core->Length();
					new_base += hdr->Length();
					uint64 relocate_by = new_base - _media_base;
					if (new_base > _media_base) {
						_stream->SetLength(new_base + _media_end - _media_base);
						_stream->RelocateData(_media_base, new_base, _media_end - _media_base);
					} else if (new_base < _media_base) {
						_stream->RelocateData(_media_base, new_base, _media_end - _media_base);
						_stream->SetLength(new_base + _media_end - _media_base);
					}
					_media_base += relocate_by;
					_media_end += relocate_by;
					for (auto & packet : _track->_packets) packet.offset += relocate_by;
					_stream->Seek(0, Streaming::Begin);
					_stream->WriteArray(hdr);
					if (core) _stream->WriteArray(core);
					if (art) _stream->WriteArray(art);
					return true;
				} catch (...) { return false; }
			}
			virtual bool PatchMetadata(const Metadata * metadata, Streaming::Stream * dest) const noexcept override
			{
				try {
					SafePointer<DataBlock> art = _create_artwork_metadata_block(metadata, true);
					SafePointer<DataBlock> core = _create_core_metadata_block(metadata, !art);
					SafePointer<DataBlock> hdr = _create_header_block(_track->_info, !art && !core);
					dest->SetLength(0);
					dest->Seek(0, Streaming::Begin);
					dest->WriteArray(hdr);
					if (core) dest->WriteArray(core);
					if (art) dest->WriteArray(art);
					_stream->Seek(_media_base, Streaming::Begin);
					_stream->CopyTo(dest, _media_end - _media_base);
					return true;
				} catch (...) { return false; }
			}
		};

		class EngineAudioStorageCodec : public IMediaContainerCodec
		{
			ContainerClassDesc _mp3, _flac;
		public:
			EngineAudioStorageCodec(void)
			{
				_mp3.ContainerFormatIdentifier = ContainerFormatMP3;
				_mp3.FormatCapabilities = ContainerClassCapabilityHoldAudio | ContainerClassCapabilityHoldMetadata;
				_mp3.MaximalTrackCount = 1;
				_mp3.MetadataFormatIdentifier = MetadataFormatID3;
				_flac.ContainerFormatIdentifier = ContainerFormatFreeLossless;
				_flac.FormatCapabilities = ContainerClassCapabilityHoldAudio | ContainerClassCapabilityHoldMetadata;
				_flac.MaximalTrackCount = 1;
				_flac.MetadataFormatIdentifier = MetadataFormatVorbisComment;
			}
			virtual ~EngineAudioStorageCodec(void) override {}
			virtual bool CanEncode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatMP3) {
					if (desc) *desc = _mp3;
					return true;
				} else return false;
			}
			virtual bool CanDecode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatMP3) {
					if (desc) *desc = _mp3;
					return true;
				} else if (format == ContainerFormatFreeLossless) {
					if (desc) *desc = _flac;
					return true;
				} else return false;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(1);
				result->Append(_mp3);
				result->Retain();
				return result;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(2);
				result->Append(_mp3);
				result->Append(_flac);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Engine MPEG-1/2 Part 3 and FLAC Multiplexor"; }
			virtual IMediaContainerSource * OpenContainer(Streaming::Stream * source) noexcept override
			{
				try {
					uint8 sign[4];
					source->Seek(0, Streaming::Begin);
					source->Read(sign, 4);
					if (uint(sign[0]) == 0xFF || MemoryCompare(sign, "ID3", 3) == 0) {
						source->Seek(0, Streaming::Begin);
						return new EngineMP3ContainerSource(this, source, _mp3);
					} else if (MemoryCompare(sign, "fLaC", 4) == 0) {
						return new EngineFLACContainerSource(this, source, _flac);
					} else return 0;
				} catch (...) { return 0; }
			}
			virtual IMediaContainerSink * CreateContainer(Streaming::Stream * dest, const string & format) noexcept override
			{
				try {
					if (format == ContainerFormatMP3) return new EngineMP3ContainerSink(this, dest, _mp3);
					else return 0;
				} catch (...) { return 0; }
			}
		};

		SafePointer<IMediaContainerCodec> _engine_audio_storage_codec;
		IMediaContainerCodec * InitializeAudioStorageCodec(void)
		{
			if (!_engine_audio_storage_codec) {
				_engine_audio_storage_codec = new EngineAudioStorageCodec;
				RegisterCodec(_engine_audio_storage_codec);
			}
			return _engine_audio_storage_codec;
		}
	}
}