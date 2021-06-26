#include "MPEG4.h"

#include "../Interfaces/Socket.h"

#include "Audio.h"
#include "Video.h"
#include "Subtitles.h"

namespace Engine
{
	namespace Media
	{
		namespace Format
		{
			ENGINE_PACKED_STRUCTURE(mpeg4_mvhd)
				uint32 flag_version;
				uint32 ctime;
				uint32 mtime;
				uint32 time_scale;
				uint32 duration;
				uint32 preferred_rate;
				uint16 preferred_volume;
				uint8 reserved[10];
				uint8 matrix[36];
				uint32 preview_time;
				uint32 preview_duration;
				uint32 poster_time;
				uint32 selection_time;
				uint32 selection_duration;
				uint32 current_time;
				uint32 next_track_id;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_tkhd)
				uint32 flag_version;
				uint32 ctime;
				uint32 mtime;
				uint32 track_id;
				uint32 reserved_0;
				uint32 duration;
				uint32 reserved_1[2];
				uint16 layer;
				uint16 group;
				uint16 volume;
				uint16 reserved_2;
				uint8 matrix[36];
				uint32 width;
				uint32 height;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_mdhd)
				uint32 flag_version;
				uint32 ctime;
				uint32 mtime;
				uint32 time_scale;
				uint32 duration;
				uint16 language;
				uint16 quality;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_vmhd)
				uint32 flag_version;
				uint16 graphics_mode;
				uint8 opcolor[6];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_smhd)
				uint32 flag_version;
				uint16 balance;
				uint16 reserved;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_gmin)
				uint32 flag_version;
				uint16 graphics_mode;
				uint8 opcolor[6];
				uint16 balance;
				uint16 reserved;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_hdlr)
				uint32 flag_version;
				uint8 com_type[4];
				uint8 com_subtype[4];
				uint32 com_manufacturer;
				uint32 com_flags;
				uint32 com_flags_mask;
				uint8 com_name[1];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stsd)
				uint32 flag_version;
				uint32 record_number;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_sample_desc)
				uint32 size;
				uint8 fourcc[4];
				uint8 reserved[6];
				uint16 data_ref_id;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_audio_sample_desc)
				uint32 size;
				uint8 fourcc[4];
				uint8 reserved[6];
				uint16 data_ref_id;
				// Audio specific fields
				uint16 version;
				uint16 revision;
				uint8 codec_vendor[4];
				uint16 num_channels;
				uint16 bits_per_channel;
				uint16 compression_id_unused;
				uint16 packet_size_unused;
				uint16 sample_rate_int;
				uint16 sample_rate_frac;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_video_sample_desc)
				uint32 size;
				uint8 fourcc[4];
				uint8 reserved[6];
				uint16 data_ref_id;
				// Video specific fields
				uint16 version;
				uint16 revision;
				uint8 codec_vendor[4];
				uint32 temporal_quality;
				uint32 spatial_quality;
				uint16 width;
				uint16 height;
				uint32 horz_dpi;
				uint32 vert_dpi;
				uint32 data_size;
				uint16 frames_per_packet;
				uint8 codec_name[32];
				uint16 color_depth;
				uint16 color_table;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_subtitle_sample_desc)
				uint32 size;
				uint8 fourcc[4];
				uint8 reserved[6];
				uint16 data_ref_id;
				// Subtitle specific fields
				uint32 display_flags;
				uint8 reserved_0; // set to 1
				uint8 reserved_1; // set to -1
				uint32 reserved_2; // set to 0
				uint16 text_box_top;
				uint16 text_box_left;
				uint16 text_box_bottom;
				uint16 text_box_right;
				uint32 reserved_3; // set to 0
				uint16 font_id;
				uint8 font_face;
				uint8 font_size;
				uint32 font_color;
				uint32 font_table_size; // set to 18
				uint32 font_table_fourcc; // set to 'ftab'
				uint16 font_table_font_count; // set to 1
				uint16 font_table_font_id; // set to 1
				uint8 font_table_font_name_length; // set to 5;
				uint8 font_table_font_name[5]; // set to "Serif"
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stts_record)
				uint32 num_samples;
				uint32 duration;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stts)
				uint32 flag_version;
				uint32 record_number;
				mpeg4_stts_record records[0];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stsc_record)
				uint32 first_chunk;
				uint32 num_packets;
				uint32 sample_desc;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stsc)
				uint32 flag_version;
				uint32 record_number;
				mpeg4_stsc_record records[0];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stco)
				uint32 flag_version;
				uint32 record_number;
				uint32 offsets[0];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_co64)
				uint32 flag_version;
				uint32 record_number;
				uint64 offsets[0];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stsz)
				uint32 flag_version;
				uint32 common_size;
				uint32 record_number;
				uint32 sizes[0];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_stss)
				uint32 flag_version;
				uint32 record_number;
				uint32 index[0];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_ctts_record)
				uint32 sample_count;
				uint32 composition_offset;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(mpeg4_ctts)
				uint32 flag_version;
				uint32 record_number;
				mpeg4_ctts_record records[0];
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(alac_spec_config)
				uint32 frames_per_packet;
				uint8 com_version; // set to 0
				uint8 bits_per_sample; // 8, 16, 24 or 32
				uint8 bp; // set to 40
				uint8 mb; // set to 10
				uint8 kb; // set to 14
				uint8 channel_count;
				uint16 max_run; // set to 255
				uint32 max_packet_size;
				uint32 ave_bit_rate;
				uint32 frames_per_second;
			ENGINE_END_PACKED_STRUCTURE
			ENGINE_PACKED_STRUCTURE(alac_channel_layout)
				uint32 channel_layout_ext_size; // set to 24
				uint32 channel_layout_id; // set to 'chan'
				uint32 flag_version; // set to 0
				uint32 channel_layout_tag;
				uint32 reserved_0; // set to 0
				uint32 reserved_1; // set to 0
			ENGINE_END_PACKED_STRUCTURE
		}
		uint MakeFourCC(const string & fourcc)
		{
			uint result = 0;
			fourcc.Fragment(0, 4).Encode(&result, Encoding::ANSI, false);
			return result;
		}
		struct Atom : public Object
		{
			string fourcc;
			SafePointer<DataBlock> data;
			ObjectArray<Atom> children;
			uint64 offset;
			uint64 length;

			Atom(void) : children(0x10) {}
			Atom(const string & fcc) : children(0x10), fourcc(fcc), offset(0), length(0) {}
			Atom(const string & fcc, int size) : children(0x10), fourcc(fcc), offset(0), length(0) { data = new DataBlock(size); data->SetLength(size); }
			static bool IsParentAtom(const string & fcc)
			{
				return fcc == L"moov" || fcc == L"trak" || fcc == L"tref" || fcc == L"mdia" || fcc == L"minf" || fcc == L"stbl" || fcc == L"udta" || fcc == L"ilst";
			}
			static Atom * ReadAtom(Streaming::Stream * from, uint64 upper_bound, bool top_level_atom = false)
			{
				SafePointer<Atom> result = new Atom;
				try {
					uint64 origin = from->Seek(0, Streaming::Current);
					if (upper_bound - origin < 8) return 0;
					uint32 size_primary;
					uint64 size;
					uint32 four_cc;
					uint32 hdr_size = 8;
					from->Read(&size_primary, 4);
					from->Read(&four_cc, 4);
					result->fourcc = string(&four_cc, 4, Encoding::ANSI);
					size_primary = Network::InverseEndianess(size_primary);
					if (size_primary == 0) {
						size = upper_bound - origin;
					} else if (size_primary == 1) {
						if (upper_bound - origin < 16) return 0;
						uint64 size_long;
						from->Read(&size_long, 8);
						size = ((size_long & 0x00000000000000FF) << 56) | ((size_long & 0x000000000000FF00) << 40) |
							((size_long & 0x0000000000FF0000) << 24) | ((size_long & 0x00000000FF000000) << 8) | ((size_long & 0x000000FF00000000) >> 8) |
							((size_long & 0x0000FF0000000000) >> 24) | ((size_long & 0x00FF000000000000) >> 40) | ((size_long & 0xFF00000000000000) >> 56);
						hdr_size = 16;
					} else if (size_primary >= 8) {
						size = size_primary;
					} else throw InvalidFormatException();
					if (top_level_atom && result->fourcc != L"ftyp" && result->fourcc != L"moov") {
						result->offset = origin;
						result->length = size;
						from->Seek(origin + size, Streaming::Begin);
					} else {
						if (size >= 0x100000000) throw InvalidFormatException();
						result->offset = result->length = 0;
						if (IsParentAtom(result->fourcc)) {
							Atom * child;
							while (child = ReadAtom(from, origin + size)) {
								try { result->children.Append(child); } catch (...) {}
								child->Release();
							}
							from->Seek(origin + size, Streaming::Begin);
						} else {
							result->data = from->ReadBlock(size - hdr_size);
						}
					}
				} catch (IO::FileReadEndOfFileException & e) {
					return 0;
				}
				result->Retain();
				return result;
			}
			static Atom * ReadAtom(DataBlock * from, int range_from, int range_up_to, int & length, bool expand = false)
			{
				SafePointer<Atom> result = new Atom;
				try {
					if (range_up_to - range_from < 8) return 0;
					uint32 size_primary;
					uint64 size;
					uint32 four_cc;
					uint32 hdr_size = 8;
					MemoryCopy(&size_primary, from->GetBuffer() + range_from, 4);
					MemoryCopy(&four_cc, from->GetBuffer() + range_from + 4, 4);
					result->fourcc = string(&four_cc, 4, Encoding::ANSI);
					size_primary = Network::InverseEndianess(size_primary);
					if (size_primary == 0) {
						size = range_up_to - range_from;
					} else if (size_primary == 1) {
						if (range_up_to - range_from < 16) return 0;
						uint64 size_long;
						MemoryCopy(&size_long, from->GetBuffer() + range_from + 8, 8);
						size = ((size_long & 0x00000000000000FF) << 56) | ((size_long & 0x000000000000FF00) << 40) |
							((size_long & 0x0000000000FF0000) << 24) | ((size_long & 0x00000000FF000000) << 8) | ((size_long & 0x000000FF00000000) >> 8) |
							((size_long & 0x0000FF0000000000) >> 24) | ((size_long & 0x00FF000000000000) >> 40) | ((size_long & 0xFF00000000000000) >> 56);
						hdr_size = 16;
					} else if (size_primary >= 8) {
						size = size_primary;
					} else throw InvalidFormatException();
					if (size >= 0x100000000) throw InvalidFormatException();
					result->offset = result->length = 0;
					if (IsParentAtom(result->fourcc) || expand) {
						Atom * child;
						int com_length = 0;
						int length;
						bool xc = result->fourcc == L"ilst";
						while (child = ReadAtom(from, range_from + hdr_size + com_length, range_from + size, length, xc)) {
							try { result->children.Append(child); } catch (...) {}
							child->Release();
							com_length += length;
						}
					} else {
						result->data = new DataBlock(size - hdr_size);
						result->data->SetLength(size - hdr_size);
						MemoryCopy(result->data->GetBuffer(), from->GetBuffer() + range_from + hdr_size, size - hdr_size);
					}
					length = size;
				} catch (IO::FileReadEndOfFileException & e) {
					return 0;
				}
				result->Retain();
				return result;
			}
			Atom * iTunesMetadataAtom(void)
			{
				if (data && data->Length() >= 4 && data->ElementAt(0) == 0 && data->ElementAt(1) == 0 && data->ElementAt(2) == 0 && data->ElementAt(3) == 0) {
					SafePointer<Atom> result = new Atom;
					Atom * atom;
					int offset = 4, length;
					while (atom = ReadAtom(data, offset, data->Length(), length)) {
						try { result->children.Append(atom); } catch (...) {}
						atom->Release();
						offset += length;
					}
					result->Retain();
					return result;
				} else throw InvalidFormatException();
			}
			Atom * FindChild(const string & fcc)
			{
				if (fourcc == fcc) return this;
				for (auto & a : children) {
					auto c = a.FindChild(fcc);
					if (c) return c;
				}
				return 0;
			}
			Atom * FindPrimaryChild(const string & fcc) { for (auto & a : children) if (a.fourcc == fcc) return &a; return 0; }
			Atom * Clone(void) const
			{
				SafePointer<Atom> result = new Atom(fourcc);
				result->offset = result->length = 0;
				if (data) result->data = new DataBlock(*data);
				for (auto & c : children) {
					SafePointer<Atom> cc = c.Clone();
					result->children.Append(cc);
				}
				result->Retain();
				return result;
			}
			int Length(void) const
			{
				if (!data) {
					int length = 8;
					for (auto & child : children) length += child.Length();
					return length;
				} else return 8 + data->Length();
			}
			void WriteToBuffer(DataBlock * buffer) const
			{
				uint length = Length();
				length = Network::InverseEndianess(length);
				int base = buffer->Length();
				buffer->SetLength(base + 8);
				MemoryCopy(buffer->GetBuffer() + base, &length, 4);
				length = MakeFourCC(fourcc);
				MemoryCopy(buffer->GetBuffer() + base + 4, &length, 4);
				if (!data) {
					for (auto & child : children) child.WriteToBuffer(buffer);
				} else buffer->Append(*data);
			}
			void WriteToFile(Streaming::Stream * stream) const
			{
				uint length = Length();
				length = Network::InverseEndianess(length);
				stream->Write(&length, 4);
				length = MakeFourCC(fourcc);
				stream->Write(&length, 4);
				if (!data) {
					for (auto & child : children) child.WriteToFile(stream);
				} else stream->WriteArray(data);
			}
			void Write(const void * buffer, int length)
			{
				if (!data) data = new DataBlock(0x100);
				int wp = data->Length();
				data->SetLength(wp + length);
				MemoryCopy(data->GetBuffer() + wp, buffer, length);
			}
			void Write(const Atom * atom)
			{
				DataBlock buffer(0x10000);
				atom->WriteToBuffer(&buffer);
				Write(buffer.GetBuffer(), buffer.Length());
			}
			void ChunkTableRelocate(int64 shift_by)
			{
				if (fourcc == L"stco") {
					auto & table = Interpret<Format::mpeg4_stco>();
					uint len = Network::InverseEndianess(table.record_number);
					for (uint i = 0; i < len; i++) {
						auto offs = Network::InverseEndianess(table.offsets[i]);
						offs += int32(shift_by);
						table.offsets[i] = Network::InverseEndianess(offs);
					}
				} else if (fourcc == L"co64") {
					auto & table = Interpret<Format::mpeg4_co64>();
					uint len = Network::InverseEndianess(table.record_number);
					for (uint i = 0; i < len; i++) {
						uint64 offs = table.offsets[i];
						offs = ((offs & 0x00000000000000FF) << 56) | ((offs & 0x000000000000FF00) << 40) |
							((offs & 0x0000000000FF0000) << 24) | ((offs & 0x00000000FF000000) << 8) | ((offs & 0x000000FF00000000) >> 8) |
							((offs & 0x0000FF0000000000) >> 24) | ((offs & 0x00FF000000000000) >> 40) | ((offs & 0xFF00000000000000) >> 56);
						offs += int32(shift_by);
						offs = ((offs & 0x00000000000000FF) << 56) | ((offs & 0x000000000000FF00) << 40) |
							((offs & 0x0000000000FF0000) << 24) | ((offs & 0x00000000FF000000) << 8) | ((offs & 0x000000FF00000000) >> 8) |
							((offs & 0x0000FF0000000000) >> 24) | ((offs & 0x00FF000000000000) >> 40) | ((offs & 0xFF00000000000000) >> 56);
						table.offsets[i] = offs;
					}
				} else for (auto & c : children) c.ChunkTableRelocate(shift_by);
			}
			template <class T> T & Interpret(void) { return *reinterpret_cast<T *>(data->GetBuffer()); }
		};

		void iTunesWriteStringAttribute(Atom * outer, const string & name, const string & text)
		{
			SafePointer<Atom> holder = new Atom(name);
			SafePointer<Atom> data = new Atom(L"data");
			uint dword = 0x01000000;
			data->Write(&dword, 4);
			dword = 0;
			data->Write(&dword, 4);
			SafePointer<DataBlock> enc = text.EncodeSequence(Encoding::UTF8, false);
			data->Write(enc->GetBuffer(), enc->Length());
			holder->children.Append(data);
			outer->children.Append(holder);
		}
		void iTunesWritePairedAttribute(Atom * outer, const string & name, uint first, uint second)
		{
			SafePointer<Atom> holder = new Atom(name);
			SafePointer<Atom> data = new Atom(L"data");
			uint zero = 0;
			data->Write(&zero, 4);
			data->Write(&zero, 4);
			data->Write(&zero, 3);
			data->Write(&first, 1);
			data->Write(&zero, 1);
			data->Write(&second, 1);
			data->Write(&zero, 2);
			holder->children.Append(data);
			outer->children.Append(holder);
		}
		void iTunesWriteNumericAttribute(Atom * outer, const string & name, uint number, uint code)
		{
			SafePointer<Atom> holder = new Atom(name);
			SafePointer<Atom> data = new Atom(L"data");
			uint dword = code;
			data->Write(&dword, 4);
			dword = 0;
			data->Write(&dword, 4);
			auto inverted = Network::InverseEndianess(number);
			auto p_inv = reinterpret_cast<uint8 *>(&inverted);
			if (number > 0xFFFF) data->Write(p_inv, 4);
			else if (number > 0xFF) data->Write(p_inv + 2, 2);
			else data->Write(p_inv + 3, 1);
			holder->children.Append(data);
			outer->children.Append(holder);
		}
		void iTunesWritePictureAttribute(Atom * outer, const string & name, const DataBlock * blob, uint code)
		{
			SafePointer<Atom> holder = new Atom(name);
			SafePointer<Atom> data = new Atom(L"data");
			uint dword = code;
			data->Write(&dword, 4);
			dword = 0;
			data->Write(&dword, 4);
			data->Write(blob->GetBuffer(), blob->Length());
			holder->children.Append(data);
			outer->children.Append(holder);
		}
		Atom * MakeMetadataAtom(const Metadata * metadata)
		{
			SafePointer<Atom> udta = new Atom(L"udta");
			SafePointer<Atom> meta = new Atom(L"meta");
			udta->children.Append(meta);
			uint zero = 0;
			meta->Write(&zero, 4);
			SafePointer<Atom> hdlr = new Atom(L"hdlr");
			hdlr->Write(&zero, 4);
			hdlr->Write(&zero, 4);
			hdlr->Write("mdirappl", 8);
			hdlr->Write(&zero, 4);
			hdlr->Write(&zero, 4);
			hdlr->Write(&zero, 1);
			meta->Write(hdlr);
			SafePointer<Atom> ilst = new Atom(L"ilst");
			for (auto & meta : metadata->Elements()) {
				if (meta.key == MetadataKey::Title) iTunesWriteStringAttribute(ilst, L"\251nam", meta.value.Text);
				else if (meta.key == MetadataKey::Album) iTunesWriteStringAttribute(ilst, L"\251alb", meta.value.Text);
				else if (meta.key == MetadataKey::Artist) iTunesWriteStringAttribute(ilst, L"\251art", meta.value.Text);
				else if (meta.key == MetadataKey::AlbumArtist) {
					iTunesWriteStringAttribute(ilst, L"aART", meta.value.Text);
					iTunesWriteStringAttribute(ilst, L"\251ART", meta.value.Text);
				} else if (meta.key == MetadataKey::Year) iTunesWriteStringAttribute(ilst, L"\251day", meta.value.Text);
				else if (meta.key == MetadataKey::Genre) iTunesWriteStringAttribute(ilst, L"\251gen", meta.value.Text);
				else if (meta.key == MetadataKey::Composer) iTunesWriteStringAttribute(ilst, L"\251wrt", meta.value.Text);
				else if (meta.key == MetadataKey::Encoder) iTunesWriteStringAttribute(ilst, L"\251too", meta.value.Text);
				else if (meta.key == MetadataKey::Copyright) iTunesWriteStringAttribute(ilst, L"cprt", meta.value.Text);
				else if (meta.key == MetadataKey::Description) iTunesWriteStringAttribute(ilst, L"desc", meta.value.Text);
				else if (meta.key == MetadataKey::Comment) iTunesWriteStringAttribute(ilst, L"\251cmt", meta.value.Text);
				else if (meta.key == MetadataKey::Lyrics) iTunesWriteStringAttribute(ilst, L"\251lyr", meta.value.Text);
				else if (meta.key == MetadataKey::TrackNumber) {
					auto count = metadata->GetElementByKey(MetadataKey::TrackCount);
					iTunesWritePairedAttribute(ilst, L"trkn", meta.value.Number, count ? count->Number : 0);
				} else if (meta.key == MetadataKey::DiskNumber) {
					auto count = metadata->GetElementByKey(MetadataKey::DiskCount);
					iTunesWritePairedAttribute(ilst, L"disk", meta.value.Number, count ? count->Number : 0);
				} else if (meta.key == MetadataKey::BeatsPerMinute) iTunesWriteNumericAttribute(ilst, L"tmpo", meta.value.Number, 0x15000000);
				else if (meta.key == MetadataKey::GenreIndex) iTunesWriteNumericAttribute(ilst, L"gnre", meta.value.Number + 1, 0);
				else if (meta.key == MetadataKey::Artwork && meta.value.Picture) {
					uint format_code = 0;
					if (meta.value.Text == Codec::ImageFormatPNG) format_code = 0x0E000000;
					else if (meta.value.Text == Codec::ImageFormatJPEG) format_code = 0x0D000000;
					if (format_code) iTunesWritePictureAttribute(ilst, L"covr", meta.value.Picture, format_code);
				}
			}
			meta->Write(ilst);
			udta->Retain();
			return udta;
		}

		class MPEG4TrackSink : public IMediaTrackSink
		{
			friend class MPEG4Sink;

			struct packet_desc {
				uint64 file_offset;
				uint32 file_size;
				uint32 flags;
				uint32 decode_time;
				uint32 render_time;
				uint32 render_duration;
			};

			IMediaContainerSink * _parent;
			Array<packet_desc> _packets;
			SafePointer<IMediaContainerCodec> _parent_codec;
			SafePointer<TrackFormatDesc> _desc;
			SafePointer<Streaming::Stream> _stream;
			SafePointer<Atom> stbl, minf, mdia, hdlr, trak;
			string _name, _language;
			bool _visible, _autoselectable;
			bool _sealed;
			int _group;
			int _track_no;
			uint _track_time_scale;
			uint _track_time_duration;

			void _write(DataBlock & buffer, const void * data, int length)
			{
				auto base = buffer.Length();
				buffer.SetLength(base + length);
				MemoryCopy(buffer.GetBuffer() + base, data, length);
			}
			void _make_stsd_for_audio(Atom * stsd)
			{
				auto & ad = _desc->As<AudioTrackFormatDesc>();
				_track_time_scale = ad.GetStreamDescriptor().FramesPerSecond;
				Format::mpeg4_stsd stsd_hdr;
				stsd_hdr.flag_version = 0;
				stsd_hdr.record_number = Network::InverseEndianess(uint32(1));
				stsd->Write(&stsd_hdr, sizeof(stsd_hdr));
				Format::mpeg4_audio_sample_desc asd;
				if (ad.GetTrackCodec() == Audio::AudioFormatAAC) {
					if (!ad.GetCodecMagic()) throw InvalidFormatException();
					uint64 data_size = 0;
					uint64 data_time = 0;
					uint64 max_bitrate = 0;
					for (auto & packet : _packets) {
						uint64 local_bitrate = uint64(packet.file_size) * 8 * _track_time_scale / packet.render_duration;
						if (local_bitrate > max_bitrate) max_bitrate = local_bitrate;
						data_size += uint64(packet.file_size) * 8;
						data_time += packet.render_duration;
					}
					uint64 ave_bitrate = data_size * _track_time_scale / data_time;
					auto asc_len = ad.GetCodecMagic()->Length();
					DataBlock esds(0x100);
					uint write = 0x80808003;
					_write(esds, &write, 4);
					write = 32 + asc_len;
					_write(esds, &write, 1);
					write = Network::InverseEndianess(uint16(_track_no));
					_write(esds, &write, 2);
					write = 0;
					_write(esds, &write, 1);
					write = 0x80808004;
					_write(esds, &write, 4);
					write = 18 + asc_len;
					_write(esds, &write, 1);
					write = 0x1540;
					_write(esds, &write, 2);
					write = 0;
					_write(esds, &write, 3);
					write = Network::InverseEndianess(uint(max_bitrate));
					_write(esds, &write, 4);
					write = Network::InverseEndianess(uint(ave_bitrate));
					_write(esds, &write, 4);
					write = 0x80808005;
					_write(esds, &write, 4);
					write = asc_len;
					_write(esds, &write, 1);
					esds.Append(*ad.GetCodecMagic());
					write = 0x80808006;
					_write(esds, &write, 4);
					write = 0x0201;
					_write(esds, &write, 2);
					ZeroMemory(&asd, sizeof(asd));
					asd.size = Network::InverseEndianess(uint(sizeof(asd) + 12 + esds.Length()));
					MemoryCopy(&asd.fourcc, "mp4a", 4);
					asd.data_ref_id = 0x0100;
					asd.num_channels = ad.GetStreamDescriptor().ChannelCount > 1 ? 0x0200 : 0x0100;
					asd.bits_per_channel = Network::InverseEndianess(uint16(16));
					asd.compression_id_unused = 0;
					asd.packet_size_unused = 0;
					asd.sample_rate_int = Network::InverseEndianess(uint16(ad.GetStreamDescriptor().FramesPerSecond));
					asd.sample_rate_frac = 0;
					stsd->Write(&asd, sizeof(asd));
					write = Network::InverseEndianess(uint(12 + esds.Length()));
					stsd->Write(&write, 4);
					write = MakeFourCC(L"esds");
					stsd->Write(&write, 4);
					write = 0;
					stsd->Write(&write, 4);
					stsd->Write(esds.GetBuffer(), esds.Length());
				} else if (ad.GetTrackCodec() == Audio::AudioFormatAppleLossless) {
					if (!ad.GetCodecMagic()) throw InvalidFormatException();
					ZeroMemory(&asd, sizeof(asd));
					asd.size = Network::InverseEndianess(uint(sizeof(asd) + 12 + ad.GetCodecMagic()->Length()));
					MemoryCopy(&asd.fourcc, "alac", 4);
					asd.data_ref_id = 0x0100;
					asd.num_channels = Network::InverseEndianess(uint16(ad.GetStreamDescriptor().ChannelCount));
					asd.bits_per_channel = Network::InverseEndianess(uint16(Audio::SampleFormatBitSize(ad.GetStreamDescriptor().Format)));
					asd.sample_rate_int = Network::InverseEndianess(uint16(ad.GetStreamDescriptor().FramesPerSecond));
					stsd->Write(&asd, sizeof(asd));
					uint write = Network::InverseEndianess(uint(12 + ad.GetCodecMagic()->Length()));
					stsd->Write(&write, 4);
					write = MakeFourCC(L"alac");
					stsd->Write(&write, 4);
					write = 0;
					stsd->Write(&write, 4);
					stsd->Write(ad.GetCodecMagic()->GetBuffer(), ad.GetCodecMagic()->Length());
				} else throw InvalidFormatException();
			}
			void _make_stsd_for_video(Atom * stsd)
			{
				auto & vd = _desc->As<VideoTrackFormatDesc>();
				_track_time_scale = vd.GetFrameRateNumerator();
				Format::mpeg4_stsd stsd_hdr;
				stsd_hdr.flag_version = 0;
				stsd_hdr.record_number = Network::InverseEndianess(uint32(1));
				stsd->Write(&stsd_hdr, sizeof(stsd_hdr));
				Format::mpeg4_video_sample_desc vsd;
				if (vd.GetTrackCodec() == Video::VideoFormatH264) {
					if (!vd.GetCodecMagic()) throw InvalidFormatException();
					vsd.size = Network::InverseEndianess(uint(sizeof(vsd) + 8 + vd.GetCodecMagic()->Length() + 16));
					vsd.fourcc[0] = 'a'; vsd.fourcc[1] = 'v'; vsd.fourcc[2] = 'c'; vsd.fourcc[3] = '1';
					for (int i = 0; i < 6; i++) vsd.reserved[i] = 0;
					vsd.data_ref_id = Network::InverseEndianess(uint16(1));
					vsd.version = 0;
					vsd.revision = 0;
					vsd.codec_vendor[0] = 0; vsd.codec_vendor[1] = 0; vsd.codec_vendor[2] = 0; vsd.codec_vendor[3] = 0;
					vsd.temporal_quality = 0;
					vsd.spatial_quality = 0;
					vsd.width = Network::InverseEndianess(uint16(vd.GetWidth()));
					vsd.height = Network::InverseEndianess(uint16(vd.GetHeight()));
					vsd.horz_dpi = vsd.vert_dpi = 0x00004800;
					vsd.data_size = 0;
					vsd.frames_per_packet = Network::InverseEndianess(uint16(1));
					ZeroMemory(&vsd.codec_name, sizeof(vsd.codec_name));
					vsd.color_depth = Network::InverseEndianess(uint16(24));
					vsd.color_table = 0xFFFF;
					stsd->Write(&vsd, sizeof(vsd));
					uint avcC_size = Network::InverseEndianess(uint(8 + vd.GetCodecMagic()->Length()));
					uint avcC_fourcc = MakeFourCC(L"avcC");
					stsd->Write(&avcC_size, 4);
					stsd->Write(&avcC_fourcc, 4);
					stsd->Write(vd.GetCodecMagic()->GetBuffer(), vd.GetCodecMagic()->Length());
					uint pasp_size = Network::InverseEndianess(uint(16));
					uint pasp_fourcc = MakeFourCC(L"pasp");
					uint pasp_horz = Network::InverseEndianess(uint(vd.GetPixelAspectHorizontal()));
					uint pasp_vert = Network::InverseEndianess(uint(vd.GetPixelAspectVertical()));
					stsd->Write(&pasp_size, 4);
					stsd->Write(&pasp_fourcc, 4);
					stsd->Write(&pasp_horz, 4);
					stsd->Write(&pasp_vert, 4);
				} else if (vd.GetTrackCodec() == Video::VideoFormatRGB) {
					vsd.size = Network::InverseEndianess(uint(sizeof(vsd) + 16));
					vsd.fourcc[0] = 'r'; vsd.fourcc[1] = 'a'; vsd.fourcc[2] = 'w'; vsd.fourcc[3] = ' ';
					for (int i = 0; i < 6; i++) vsd.reserved[i] = 0;
					vsd.data_ref_id = Network::InverseEndianess(uint16(1));
					vsd.version = 0x0100;
					vsd.revision = 0x0100;
					vsd.codec_vendor[0] = 'a'; vsd.codec_vendor[1] = 'p'; vsd.codec_vendor[2] = 'p'; vsd.codec_vendor[3] = 'l';
					vsd.temporal_quality = 0;
					vsd.spatial_quality = Network::InverseEndianess(uint32(1024));
					vsd.width = Network::InverseEndianess(uint16(vd.GetWidth()));
					vsd.height = Network::InverseEndianess(uint16(vd.GetHeight()));
					vsd.horz_dpi = vsd.vert_dpi = 0x00004800;
					vsd.data_size = 0;
					vsd.frames_per_packet = Network::InverseEndianess(uint16(1));
					ZeroMemory(&vsd.codec_name, sizeof(vsd.codec_name));
					vsd.color_depth = Network::InverseEndianess(uint16(24));
					vsd.color_table = 0xFFFF;
					stsd->Write(&vsd, sizeof(vsd));
					uint atom_size = Network::InverseEndianess(uint(16));
					uint atom_fourcc = MakeFourCC(L"pasp");
					uint pasp_horz = Network::InverseEndianess(uint(vd.GetPixelAspectHorizontal()));
					uint pasp_vert = Network::InverseEndianess(uint(vd.GetPixelAspectVertical()));
					stsd->Write(&atom_size, 4);
					stsd->Write(&atom_fourcc, 4);
					stsd->Write(&pasp_horz, 4);
					stsd->Write(&pasp_vert, 4);
				}
			}
			void _make_stsd_for_subtitles(Atom * stsd)
			{
				auto & sd = _desc->As<SubtitleTrackFormatDesc>();
				_track_time_scale = sd.GetTimeScale();
				Format::mpeg4_stsd stsd_hdr;
				stsd_hdr.flag_version = 0;
				stsd_hdr.record_number = Network::InverseEndianess(uint32(1));
				stsd->Write(&stsd_hdr, sizeof(stsd_hdr));
				Format::mpeg4_subtitle_sample_desc ssd;
				if (sd.GetTrackCodec() == Subtitles::SubtitleFormat3GPPTimedText) {
					ZeroMemory(&ssd, sizeof(ssd));
					ssd.size = Network::InverseEndianess(uint(sizeof(ssd)));
					MemoryCopy(&ssd.fourcc, "tx3g", 4);
					ssd.data_ref_id = 0x0100;
					if (sd.GetFlags() & Subtitles::SubtitleFlagSampleForced) ssd.display_flags |= 0x00000040;
					if (sd.GetFlags() & Subtitles::SubtitleFlagAllForced) ssd.display_flags |= 0x00000080;
					ssd.reserved_0 = 0x01;
					ssd.reserved_1 = 0xFF;
					ssd.font_id = 0x0100;
					ssd.font_color = 0xFFFFFFFF;
					ssd.font_table_size = Network::InverseEndianess(uint(18));
					MemoryCopy(&ssd.font_table_fourcc, "ftab", 4);
					ssd.font_table_font_count = 0x0100;
					ssd.font_table_font_id = 0x0100;
					ssd.font_table_font_name_length = 5;
					MemoryCopy(&ssd.font_table_font_name, "Serif", 5);
					stsd->Write(&ssd, sizeof(ssd));
				}
			}
			Atom * _make_sample_handler(void)
			{
				SafePointer<Atom> stsd = new Atom(L"stsd");
				if (_desc->GetTrackClass() == TrackClass::Audio) {
					_make_stsd_for_audio(stsd);
				} else if (_desc->GetTrackClass() == TrackClass::Video) {
					_make_stsd_for_video(stsd);
				} else if (_desc->GetTrackClass() == TrackClass::Subtitles) {
					_make_stsd_for_subtitles(stsd);
				}
				_track_time_duration = 0;
				for (auto & p : _packets) _track_time_duration += p.render_duration;
				stsd->Retain();
				return stsd;
			}
			void _init_stbl(void)
			{
				stbl = new Atom(L"stbl");
				int min_render_offs = _packets[0].render_time - _packets[0].decode_time;
				bool uniform_size = true;
				bool has_shifts = false;
				bool has_non_key_frames = false;
				int num_key_frames = 0;
				for (auto & p : _packets) {
					int render_offs = p.render_time - p.decode_time;
					if (p.file_size != _packets[0].file_size) uniform_size = false;
					if (render_offs) has_shifts = true;
					if (render_offs < min_render_offs) min_render_offs = render_offs;
					if (!p.flags) has_non_key_frames = true;
					else num_key_frames++;
				}
				SafePointer<Atom> stsd = _make_sample_handler();
				stbl->children.Append(stsd);
				SafePointer<Atom> stts = new Atom(L"stts", 8 + 8 * _packets.Length());
				auto & stts_hdr = stts->Interpret<Format::mpeg4_stts>();
				uint stts_num_entries = 0;
				int stts_fp = 0;
				while (stts_fp < _packets.Length()) {
					int lp = stts_fp + 1;
					while (lp < _packets.Length() && _packets[lp].render_duration == _packets[stts_fp].render_duration) lp++;
					stts_hdr.records[stts_num_entries].num_samples = Network::InverseEndianess(uint(lp - stts_fp));
					stts_hdr.records[stts_num_entries].duration = Network::InverseEndianess(_packets[stts_fp].render_duration);
					stts_num_entries++;
					stts_fp = lp;
				}
				stts_hdr.flag_version = 0;
				stts_hdr.record_number = Network::InverseEndianess(stts_num_entries);
				stts->data->SetLength(8 + 8 * stts_num_entries);
				stbl->children.Append(stts);
				SafePointer<Atom> stsz = new Atom(L"stsz", uniform_size ? 12 : (12 + 4 * _packets.Length()));
				auto & stsz_hdr = stsz->Interpret<Format::mpeg4_stsz>();
				stsz_hdr.flag_version = 0;
				if (uniform_size) {
					stsz_hdr.common_size = Network::InverseEndianess(_packets[0].file_size);
					stsz_hdr.record_number = 0;
				} else {
					stsz_hdr.common_size = 0;
					stsz_hdr.record_number = Network::InverseEndianess(uint(_packets.Length()));
					for (int i = 0; i < _packets.Length(); i++) {
						stsz_hdr.sizes[i] = Network::InverseEndianess(_packets[i].file_size);
					}
				}
				stbl->children.Append(stsz);
				SafePointer<Atom> stsc = new Atom(L"stsc", 20);
				auto & stsc_hdr = stsc->Interpret<Format::mpeg4_stsc>();
				stsc_hdr.flag_version = 0;
				stsc_hdr.record_number = Network::InverseEndianess(uint(1));
				stsc_hdr.records[0].first_chunk = stsc_hdr.records[0].num_packets = stsc_hdr.records[0].sample_desc = Network::InverseEndianess(uint(1));
				stbl->children.Append(stsc);
				if (_packets.LastElement().file_offset <= 0xD0000000) {
					SafePointer<Atom> stco = new Atom(L"stco", 8 + 4 * _packets.Length());
					auto & stco_hdr = stco->Interpret<Format::mpeg4_stco>();
					stco_hdr.flag_version = 0;
					stco_hdr.record_number = Network::InverseEndianess(uint(_packets.Length()));
					for (int i = 0; i < _packets.Length(); i++) {
						stco_hdr.offsets[i] = Network::InverseEndianess(uint(_packets[i].file_offset));
					}
					stbl->children.Append(stco);
				} else {
					SafePointer<Atom> co64 = new Atom(L"co64", 8 + 8 * _packets.Length());
					auto & co64_hdr = co64->Interpret<Format::mpeg4_co64>();
					co64_hdr.flag_version = 0;
					co64_hdr.record_number = Network::InverseEndianess(uint(_packets.Length()));
					for (int i = 0; i < _packets.Length(); i++) {
						auto offs = _packets[i].file_offset;
						co64_hdr.offsets[i] = ((offs & 0x00000000000000FF) << 56) | ((offs & 0x000000000000FF00) << 40) |
							((offs & 0x0000000000FF0000) << 24) | ((offs & 0x00000000FF000000) << 8) | ((offs & 0x000000FF00000000) >> 8) |
							((offs & 0x0000FF0000000000) >> 24) | ((offs & 0x00FF000000000000) >> 40) | ((offs & 0xFF00000000000000) >> 56);
					}
					stbl->children.Append(co64);
				}
				if (has_non_key_frames) {
					SafePointer<Atom> stss = new Atom(L"stss", 8 + 4 * num_key_frames);
					auto & stss_hdr = stss->Interpret<Format::mpeg4_stss>();
					stss_hdr.flag_version = 0;
					stss_hdr.record_number = Network::InverseEndianess(uint(num_key_frames));
					int p = 0;
					for (int i = 0; i < _packets.Length(); i++) if (_packets[i].flags) {
						stss_hdr.index[p] = Network::InverseEndianess(uint(i + 1));
						p++;
					}
					stbl->children.Append(stss);
				}
				if (has_shifts) {
					uint num_records = 0;
					SafePointer<Atom> ctts = new Atom(L"ctts", 8 + 8 * _packets.Length());
					auto & ctts_hdr = ctts->Interpret<Format::mpeg4_ctts>();
					int fp = 0;
					while (fp < _packets.Length()) {
						int lp = fp + 1;
						auto fp_offs = _packets[fp].render_time - _packets[fp].decode_time;
						while (lp < _packets.Length() && _packets[lp].render_time - _packets[lp].decode_time == fp_offs) lp++;
						ctts_hdr.records[num_records].sample_count = Network::InverseEndianess(uint(lp - fp));
						ctts_hdr.records[num_records].composition_offset = Network::InverseEndianess(uint(fp_offs - min_render_offs));
						num_records++;
						fp = lp;
					}
					ctts_hdr.flag_version = 0;
					ctts_hdr.record_number = Network::InverseEndianess(num_records);
					ctts->data->SetLength(8 + 8 * num_records);
					stbl->children.Append(ctts);
				}
			}
			void _init_minf(void)
			{
				minf = new Atom(L"minf");
				hdlr = new Atom(L"hdlr", sizeof(Format::mpeg4_hdlr));
				auto & hdlr_hdr = hdlr->Interpret<Format::mpeg4_hdlr>();
				ZeroMemory(&hdlr_hdr, sizeof(hdlr_hdr));
				if (_desc->GetTrackClass() == TrackClass::Audio) {
					auto & ad = _desc->As<AudioTrackFormatDesc>();
					SafePointer<Atom> smhd = new Atom(L"smhd", sizeof(Format::mpeg4_smhd));
					auto & smhd_hdr = smhd->Interpret<Format::mpeg4_smhd>();
					smhd_hdr.flag_version = 0;
					smhd_hdr.balance = 0;
					smhd_hdr.reserved = 0;
					minf->children.Append(smhd);
					MemoryCopy(&hdlr_hdr.com_type, "mhlr", 4);
					MemoryCopy(&hdlr_hdr.com_subtype, "soun", 4);
				} else if (_desc->GetTrackClass() == TrackClass::Video) {
					auto & vd = _desc->As<VideoTrackFormatDesc>();
					SafePointer<Atom> vmhd = new Atom(L"vmhd", sizeof(Format::mpeg4_vmhd));
					auto & vmhd_hdr = vmhd->Interpret<Format::mpeg4_vmhd>();
					vmhd_hdr.flag_version = 0x01000000;
					vmhd_hdr.graphics_mode = 0;
					ZeroMemory(&vmhd_hdr.opcolor, sizeof(vmhd_hdr.opcolor));
					minf->children.Append(vmhd);
					MemoryCopy(&hdlr_hdr.com_type, "mhlr", 4);
					MemoryCopy(&hdlr_hdr.com_subtype, "vide", 4);
				} else if (_desc->GetTrackClass() == TrackClass::Subtitles) {
					auto & sd = _desc->As<SubtitleTrackFormatDesc>();
					SafePointer<Atom> nmhd = new Atom(L"nmhd");
					uint write = 0;
					nmhd->Write(&write, 4);
					minf->children.Append(nmhd);
					MemoryCopy(&hdlr_hdr.com_type, "mhlr", 4);
					MemoryCopy(&hdlr_hdr.com_subtype, "sbtl", 4);
				}
				SafePointer<Atom> dinf = new Atom(L"dinf");
				SafePointer<Atom> dref = new Atom(L"dref");
				SafePointer<Atom> data_hdlr = new Atom(L"hdlr", sizeof(Format::mpeg4_hdlr));
				auto & data_hdlr_hdr = data_hdlr->Interpret<Format::mpeg4_hdlr>();
				ZeroMemory(&data_hdlr_hdr, sizeof(data_hdlr_hdr));
				MemoryCopy(&data_hdlr_hdr.com_type, "dhlr", 4);
				MemoryCopy(&data_hdlr_hdr.com_subtype, "url ", 4);
				uint dword = 0;
				dref->Write(&dword, 4);
				dword = 0x01000000;
				dref->Write(&dword, 4);
				dword = 0x0C000000;
				dref->Write(&dword, 4);
				dword = MakeFourCC(L"url ");
				dref->Write(&dword, 4);
				dword = 0x01000000;
				dref->Write(&dword, 4);
				dinf->children.Append(dref);
				minf->children.Append(data_hdlr);
				minf->children.Append(dinf);
				if (stbl) minf->children.Append(stbl);
			}
			void _init_mdia(void)
			{
				mdia = new Atom(L"mdia");
				SafePointer<Atom> mdhd = new Atom(L"mdhd", sizeof(Format::mpeg4_mdhd));
				auto & mdhd_hdr = mdhd->Interpret<Format::mpeg4_mdhd>();
				mdhd_hdr.flag_version = 0;
				mdhd_hdr.ctime = mdhd_hdr.mtime = 0;
				mdhd_hdr.time_scale = Network::InverseEndianess(_track_time_scale);
				mdhd_hdr.duration = Network::InverseEndianess(_track_time_duration);
				mdhd_hdr.quality = 0;
				mdhd_hdr.language = 0;
				if (_language.Length() > 2) {
					uint16 lang = 0;
					lang |= ((_language[0] - 0x60) & 0x1F) << 10;
					lang |= ((_language[1] - 0x60) & 0x1F) << 5;
					lang |= ((_language[2] - 0x60) & 0x1F);
					mdhd_hdr.language = Network::InverseEndianess(lang);
				}
				mdia->children.Append(mdhd);
				mdia->children.Append(hdlr);
				mdia->children.Append(minf);
			}
			void _preinit_trak(void)
			{
				trak = new Atom(L"trak");
				if (!_visible) {
					SafePointer<Atom> txas = new Atom(L"txas");
					trak->children.Append(txas);
				}
				trak->children.Append(mdia);
				if (_name.Length()) {
					uint16 any_lang_id = 21956;
					SafePointer<Atom> udta = new Atom(L"udta");
					SafePointer<Atom> name = new Atom(L"name");
					SafePointer<Atom> titl = new Atom(L"titl");
					udta->children.Append(name);
					udta->children.Append(titl);
					trak->children.Append(udta);
					name->data = _name.EncodeSequence(Encoding::UTF8, false);
					titl->data = new DataBlock(0x100);
					titl->data->Append(0);
					titl->data->Append(0);
					titl->data->Append(0);
					titl->data->Append(0);
					titl->data->Append((any_lang_id & 0xFF00) >> 8);
					titl->data->Append(any_lang_id & 0xFF);
					titl->data->Append(*name->data);
					titl->data->Append(0);
				}
			}
		public:
			MPEG4TrackSink(Streaming::Stream * stream, const TrackFormatDesc & desc, IMediaContainerSink * parent) : _parent(parent), _packets(0x1000)
			{
				_stream.SetRetain(stream);
				_parent_codec.SetRetain(_parent->GetParentCodec());
				_visible = _autoselectable = true;
				_sealed = false;
				_group = 0;
				if (desc.GetTrackClass() == TrackClass::Audio) {
					if (desc.GetTrackCodec() != Audio::AudioFormatAAC && desc.GetTrackCodec() != Audio::AudioFormatAppleLossless) throw InvalidFormatException();
					auto & ad = desc.As<AudioTrackFormatDesc>();
					_desc = new AudioTrackFormatDesc(desc.GetTrackCodec(), ad.GetStreamDescriptor(), ad.GetChannelLayout());
				} else if (desc.GetTrackClass() == TrackClass::Video) {
					if (desc.GetTrackCodec() != Video::VideoFormatH264 && desc.GetTrackCodec() != Video::VideoFormatRGB) throw InvalidFormatException();
					auto & vd = desc.As<VideoTrackFormatDesc>();
					_desc = new VideoTrackFormatDesc(desc.GetTrackCodec(), vd.GetWidth(), vd.GetHeight(),
						vd.GetFrameRateNumerator(), vd.GetFrameRateDenominator(), vd.GetPixelAspectHorizontal(), vd.GetPixelAspectVertical());
				} else if (desc.GetTrackClass() == TrackClass::Subtitles) {
					if (desc.GetTrackCodec() != Subtitles::SubtitleFormat3GPPTimedText) throw InvalidFormatException();
					auto & sd = desc.As<SubtitleTrackFormatDesc>();
					_desc = new SubtitleTrackFormatDesc(desc.GetTrackCodec(), sd.GetTimeScale(), sd.GetFlags());
				} else throw InvalidFormatException();
				if (desc.GetCodecMagic()) _desc->SetCodecMagic(desc.GetCodecMagic());
			}
			virtual ~MPEG4TrackSink(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Sink; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _parent_codec; }
			virtual IMediaContainer * GetParentContainer(void) const noexcept override { return _parent; }
			virtual TrackClass GetTrackClass(void) const noexcept override { return _desc->GetTrackClass(); }
			virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept override { return *_desc; }
			virtual string GetTrackName(void) const override { return _name; }
			virtual string GetTrackLanguage(void) const override { return _language; }
			virtual bool IsTrackVisible(void) const noexcept override { return _visible; }
			virtual bool IsTrackAutoselectable(void) const noexcept override { return _autoselectable; }
			virtual int GetTrackGroup(void) const noexcept override { return _group; }
			virtual bool SetTrackName(const string & name) noexcept override { try { _name = name; return true; } catch (...) { return false; } }
			virtual bool SetTrackLanguage(const string & language) noexcept override { try { _language = language; return true; } catch (...) { return false; } }
			virtual void MakeTrackVisible(bool make) noexcept override { _visible = make; }
			virtual void MakeTrackAutoselectable(bool make) noexcept override { _autoselectable = make; }
			virtual void SetTrackGroup(int group) noexcept override { _group = group; }
			virtual bool WritePacket(const PacketBuffer & buffer) noexcept override
			{
				if (_sealed) return false;
				if (buffer.PacketDataActuallyUsed) {
					try {
						packet_desc packet;
						packet.file_offset = _stream->Seek(0, Streaming::Current);
						packet.file_size = buffer.PacketDataActuallyUsed;
						packet.flags = buffer.PacketIsKey ? 0x01 : 0x00;
						packet.decode_time = buffer.PacketDecodeTime;
						packet.render_time = buffer.PacketRenderTime;
						packet.render_duration = buffer.PacketRenderDuration;
						_packets.Append(packet);
						try {
							_stream->Write(buffer.PacketData->GetBuffer(), buffer.PacketDataActuallyUsed);
						} catch (...) { _packets.RemoveLast(); throw; }
						return true;
					} catch (...) { return false; }
				} else return true;
			}
			virtual bool UpdateCodecMagic(const DataBlock * data) noexcept override { try { _desc->SetCodecMagic(data); return true; } catch (...) { return false; } }
			virtual bool Sealed(void) const noexcept override { return _sealed; }
			virtual bool Finalize(void) noexcept override
			{
				if (_sealed || !_packets.Length()) return false;
				try {
					if (_packets.Length()) _init_stbl();
					_init_minf();
					_init_mdia();
					_preinit_trak();
				} catch (...) { return false; }
				_sealed = true;
				if (_parent->IsAutofinalizable()) {
					bool can_finalize = true;
					for (int i = 0; i < _parent->GetTrackCount(); i++) {
						auto track = static_cast<IMediaTrackSink *>(_parent->GetTrack(i));
						if (!track->Sealed()) { can_finalize = false; break; }
					}
					if (can_finalize) return _parent->Finalize();
				}
				return true;
			}
		};
		class MPEG4Sink : public IMediaContainerSink
		{
			ContainerClassDesc _desc;
			SafePointer<Streaming::Stream> _stream;
			SafePointer<IMediaContainerCodec> _parent;
			SafePointer<Metadata> _metadata;
			ObjectArray<MPEG4TrackSink> _tracks;
			SafePointer<Atom> ftyp, moov;
			bool _sealed, _autofin;
		public:
			MPEG4Sink(Streaming::Stream * dest, IMediaContainerCodec * codec, const ContainerClassDesc & desc) : _tracks(0x10), _sealed(false), _autofin(false)
			{
				_stream.SetRetain(dest);
				_parent.SetRetain(codec);
				_desc = desc;
				_stream->SetLength(0);
				_stream->Seek(0, Streaming::Begin);
			}
			virtual ~MPEG4Sink(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Sink; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _parent; }
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept override { return _desc; }
			virtual Metadata * ReadMetadata(void) const noexcept override { if (_metadata) { try { return CloneMetadata(_metadata); } catch (...) { return 0; } } else return 0; }
			virtual int GetTrackCount(void) const noexcept override { return _tracks.Length(); }
			virtual IMediaTrack * GetTrack(int index) const noexcept override { return _tracks.ElementAt(index); }
			virtual void WriteMetadata(const Metadata * metadata) noexcept override { try { _metadata = metadata ? CloneMetadata(metadata) : 0; } catch (...) {} }
			virtual IMediaTrackSink * CreateTrack(const TrackFormatDesc & desc) noexcept override
			{
				if (_sealed) return 0;
				try {
					SafePointer<MPEG4TrackSink> track = new MPEG4TrackSink(_stream, desc, this);
					_tracks.Append(track);
					track->_track_no = _tracks.Length();
					track->Retain();
					return track;
				} catch (...) { return 0; }
			}
			virtual void SetAutofinalize(bool set) noexcept override { _autofin = set; }
			virtual bool IsAutofinalizable(void) const noexcept override { return _autofin; }
			virtual bool Finalize(void) noexcept override
			{
				if (_sealed) return false;
				for (auto & track : _tracks) if (!track._sealed) return false;
				try {
					ftyp = new Atom(L"ftyp", 24);
					MemoryCopy(ftyp->data->GetBuffer(), "mp42", 4);
					MemoryCopy(ftyp->data->GetBuffer() + 4, "\0\0\2\0", 4);
					MemoryCopy(ftyp->data->GetBuffer() + 8, "isom", 4);
					MemoryCopy(ftyp->data->GetBuffer() + 12, "iso2", 4);
					MemoryCopy(ftyp->data->GetBuffer() + 16, "avc1", 4);
					MemoryCopy(ftyp->data->GetBuffer() + 20, "mp41", 4);
					moov = new Atom(L"moov");
					SafePointer<Atom> mvhd = new Atom(L"mvhd", sizeof(Format::mpeg4_mvhd));
					auto & mvhd_hdr = mvhd->Interpret<Format::mpeg4_mvhd>();
					uint common_time_scale = 1000;
					uint common_duration = 0;
					uint ref_subtitle_width = 0;
					uint ref_subtitle_height = 0;
					for (auto & track : _tracks) if (track.GetTrackClass() == TrackClass::Video) {
						auto & vd = track.GetFormatDescriptor().As<VideoTrackFormatDesc>();
						ref_subtitle_width = vd.GetWidth();
						ref_subtitle_height = vd.GetHeight();
					}
					for (int i = 0; i < _tracks.Length(); i++) {
						auto & track = _tracks[i];
						auto duration = (uint64(track._track_time_duration) * common_time_scale + track._track_time_scale - 1) / track._track_time_scale;
						if (duration > common_duration) common_duration = duration;
						SafePointer<Atom> tkhd = new Atom(L"tkhd", sizeof(Format::mpeg4_tkhd));
						auto & tkhd_hdr = tkhd->Interpret<Format::mpeg4_tkhd>();
						ZeroMemory(&tkhd_hdr, sizeof(tkhd_hdr));
						tkhd_hdr.flag_version = track._visible ? 0x03000000 : 0x02000000;
						tkhd_hdr.ctime = tkhd_hdr.mtime = 0;
						tkhd_hdr.track_id = Network::InverseEndianess(uint(i + 1));
						tkhd_hdr.duration = Network::InverseEndianess(uint(duration));
						tkhd_hdr.layer = 0;
						tkhd_hdr.group = Network::InverseEndianess(uint16(track._group));
						tkhd_hdr.volume = 0;
						tkhd_hdr.matrix[1] = 0x01;
						tkhd_hdr.matrix[17] = 0x01;
						tkhd_hdr.matrix[32] = 0x40;
						tkhd_hdr.width = tkhd_hdr.height = 0;
						if (track._desc->GetTrackClass() == TrackClass::Audio) {
							tkhd_hdr.volume = 0x0001;
						} else if (track._desc->GetTrackClass() == TrackClass::Video) {
							auto & vd = track._desc->As<VideoTrackFormatDesc>();
							tkhd_hdr.width = Network::InverseEndianess(uint16(vd.GetWidth()));
							tkhd_hdr.height = Network::InverseEndianess(uint16(vd.GetHeight()));
						} else if (track._desc->GetTrackClass() == TrackClass::Subtitles) {
							tkhd_hdr.width = Network::InverseEndianess(uint16(ref_subtitle_width));
							tkhd_hdr.height = Network::InverseEndianess(uint16(ref_subtitle_height * 0.15));
							if (track.stbl) {
								auto & ssd = *reinterpret_cast<Format::mpeg4_subtitle_sample_desc *>(track.stbl->FindPrimaryChild(L"stsd")->data->GetBuffer() + 8);
								ssd.text_box_right = tkhd_hdr.width;
								ssd.text_box_bottom = tkhd_hdr.height;
								ssd.font_size = ref_subtitle_height * 0.05;
							}
						}
						track.trak->children.Insert(tkhd, 0);
						SafePointer<Atom> edts = new Atom(L"edts");
						SafePointer<Atom> elst = new Atom(L"elst");
						uint version = 0;
						uint edit_count = Network::InverseEndianess(uint(1));
						uint edit_duration = Network::InverseEndianess(uint(duration));
						uint edit_time = 0;
						uint edit_rate = 0x00000100;
						elst->Write(&version, 4);
						elst->Write(&edit_count, 4);
						elst->Write(&edit_duration, 4);
						elst->Write(&edit_time, 4);
						elst->Write(&edit_rate, 4);
						edts->children.Append(elst);
						track.trak->children.Append(edts);
					}
					mvhd_hdr.flag_version = 0;
					mvhd_hdr.ctime = mvhd_hdr.mtime = 0;
					mvhd_hdr.time_scale = Network::InverseEndianess(common_time_scale);
					mvhd_hdr.duration = Network::InverseEndianess(common_duration);
					mvhd_hdr.preferred_rate = 0x00000100;
					mvhd_hdr.preferred_volume = 0x0001;
					ZeroMemory(&mvhd_hdr.reserved, sizeof(mvhd_hdr.reserved));
					ZeroMemory(&mvhd_hdr.matrix, sizeof(mvhd_hdr.matrix));
					mvhd_hdr.matrix[1] = 0x01;
					mvhd_hdr.matrix[17] = 0x01;
					mvhd_hdr.matrix[32] = 0x40;
					mvhd_hdr.preview_time = 0;
					mvhd_hdr.preview_duration = 0;
					mvhd_hdr.poster_time = 0;
					mvhd_hdr.selection_time = 0;
					mvhd_hdr.selection_duration = 0;
					mvhd_hdr.current_time = 0;
					mvhd_hdr.next_track_id = Network::InverseEndianess(uint(_tracks.Length() + 1));
					moov->children.Append(mvhd);
					for (auto & track : _tracks) moov->children.Append(track.trak);
					if (_metadata) {
						SafePointer<Atom> udta = MakeMetadataAtom(_metadata);
						moov->children.Append(udta);
					}
					int ftyp_length = ftyp->Length();
					int moov_length = moov->Length();
					uint64 packet_data_length = _stream->Length();
					bool long_mdat = packet_data_length > 0xC0000000;
					int relocation_offset = ftyp_length + moov_length + (long_mdat ? 16 : 8);
					_stream->SetLength(relocation_offset + packet_data_length);
					_stream->RelocateData(0, relocation_offset, packet_data_length);
					moov->ChunkTableRelocate(relocation_offset);
					_stream->Seek(0, Streaming::Begin);
					ftyp->WriteToFile(_stream);
					moov->WriteToFile(_stream);
					if (long_mdat) {
						uint64 length = packet_data_length + 16;
						length = ((length & 0x00000000000000FF) << 56) | ((length & 0x000000000000FF00) << 40) |
							((length & 0x0000000000FF0000) << 24) | ((length & 0x00000000FF000000) << 8) | ((length & 0x000000FF00000000) >> 8) |
							((length & 0x0000FF0000000000) >> 24) | ((length & 0x00FF000000000000) >> 40) | ((length & 0xFF00000000000000) >> 56);
						uint write = Network::InverseEndianess(uint(1));
						_stream->Write(&write, 4);
						write = MakeFourCC(L"mdat");
						_stream->Write(&write, 4);
						_stream->Write(&length, 8);
					} else {
						uint length = packet_data_length + 8;
						length = Network::InverseEndianess(length);
						_stream->Write(&length, 4);
						length = MakeFourCC(L"mdat");
						_stream->Write(&length, 4);
					}
					_sealed = true;
					return true;
				} catch (...) { return false; }
			}
		};
		
		class MPEG4TrackSource : public IMediaTrackSource
		{
			friend class MPEG4Source;

			struct packet_desc {
				uint64 file_offset;
				uint32 file_size;
				uint32 flags;
				uint32 decode_time;
				uint32 render_time;
				uint32 render_duration;
			};

			SafePointer<Streaming::Stream> _stream;
			SafePointer<IMediaContainerCodec> _codec;
			SafePointer<Atom> _trak;
			SafePointer<TrackFormatDesc> _desc;
			IMediaContainerSource * _source;
			uint32 _common_time_scale, _common_time_duration;
			uint32 _track_time_scale, _track_time_duration;
			bool _visible, _autoselectable;
			int _group, _id;
			string _name, _language;
			SafePointer<DataBlock> _codec_usage_data;
			int _current_packet;
			uint64 _current_time_local_scale;
			Array<packet_desc> _packets;

			static DataBlock * _parse_esds_for_tag(const uint8 * data, int length, uint32 tag)
			{
				for (int i = 0; i < length - 4; i++) {
					auto f = *reinterpret_cast<const uint32 *>(data + i);
					if (f == tag) {
						int tl = data[i + 4];
						SafePointer<DataBlock> result = new DataBlock(tl);
						for (int j = 0; j < tl; j++) result->Append(data[i + j + 5]);
						result->Retain();
						return result;
					}
				}
				return 0;
			}
			static uint _read_acs(const DataBlock & acs, int & cp, int amount) noexcept
			{
				uint result = 0;
				while (amount) {
					int byte = cp / 8, bit = cp % 8;
					int bits_read = min(amount, 8 - bit);
					int mask = 0;
					for (int i = 0; i < bits_read; i++) mask |= (1 << i);
					result <<= bits_read;
					result |= (acs[byte] >> (8 - bit - bits_read)) & mask;
					amount -= bits_read;
					cp += bits_read;
				}
				return result;
			}
			static void _load_stsd_extra_atom(Atom * stsd, int & read_pos, string & four_cc, DataBlock & buffer)
			{
				auto & input = *stsd->data;
				buffer.Clear();
				if (read_pos + 8 <= input.Length()) {
					uint length, fcc;
					MemoryCopy(&length, input.GetBuffer() + read_pos, 4);
					MemoryCopy(&fcc, input.GetBuffer() + read_pos + 4, 4);
					length = Network::InverseEndianess(length);
					four_cc = string(&fcc, 4, Encoding::ANSI);
					read_pos += 8;
					length -= 8;
					if (read_pos + length <= input.Length()) {
						buffer.SetLength(length);
						MemoryCopy(buffer.GetBuffer(), input.GetBuffer() + read_pos, length);
					} else four_cc = L"";
					read_pos += length;
				} else four_cc = L"";
			}
			void _load_audio_desc(Atom * stsd)
			{
				auto & sd = *reinterpret_cast<Format::mpeg4_audio_sample_desc *>(stsd->data->GetBuffer() + 8);
				string codec;
				Audio::StreamDesc desc;
				string cfcc = string(sd.fourcc, 4, Encoding::ANSI);
				if (cfcc == L"mp4a") {
					desc.Format = Audio::SampleFormat::S16_snorm;
					desc.FramesPerSecond = _track_time_scale;
					desc.ChannelCount = Network::InverseEndianess(sd.num_channels);
					uint channel_layout = 0;
					codec = Audio::AudioFormatAAC;
					int rp = 8 + sizeof(sd);
					while (true) {
						DataBlock ex_atom(0x100);
						string ex_fourcc;
						_load_stsd_extra_atom(stsd, rp, ex_fourcc, ex_atom);
						if (!ex_fourcc.Length()) break;
						if (ex_fourcc == L"esds") {
							SafePointer<DataBlock> acs = _parse_esds_for_tag(ex_atom.GetBuffer() + 4, ex_atom.Length() - 4, 0x80808005);
							if (acs) {
								int cp = 0;
								auto obj_type = _read_acs(*acs, cp, 5);
								if (obj_type == 31) _read_acs(*acs, cp, 6);
								auto rate = _read_acs(*acs, cp, 4);
								if (rate == 15) _read_acs(*acs, cp, 24);
								auto cc = _read_acs(*acs, cp, 4);
								if (cc) {
									if (cc == 7) desc.ChannelCount = 8;
									else if (cc > 7) throw InvalidFormatException();
									else desc.ChannelCount = cc;
									if (desc.ChannelCount == 1) channel_layout = Audio::ChannelLayoutCenter;
									else {
										channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight;
										if (desc.ChannelCount > 2) channel_layout |= Audio::ChannelLayoutCenter;
										if (desc.ChannelCount == 4) channel_layout |= Audio::ChannelLayoutBackCenter;
										else if (desc.ChannelCount > 4) {
											channel_layout |= Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight;
											if (desc.ChannelCount > 5) channel_layout |= Audio::ChannelLayoutLowFrequency;
											if (desc.ChannelCount == 8) channel_layout |= Audio::ChannelLayoutSideLeft | Audio::ChannelLayoutSideRight;
										}
									}
								} else throw InvalidFormatException();
								_codec_usage_data = acs;
							}
						}
					}
					_desc = new AudioTrackFormatDesc(codec, desc, channel_layout);
				} else if (cfcc == L"alac") {
					desc.Format = Audio::SampleFormat::Invalid;
					desc.FramesPerSecond = _track_time_scale;
					desc.ChannelCount = Network::InverseEndianess(sd.num_channels);
					codec = Audio::AudioFormatAppleLossless;
					int rp = 8 + sizeof(sd);
					uint channel_layout = 0;
					while (true) {
						DataBlock ex_atom(0x100);
						string ex_fourcc;
						_load_stsd_extra_atom(stsd, rp, ex_fourcc, ex_atom);
						if (!ex_fourcc.Length()) break;
						if (ex_fourcc == L"alac") {
							if (ex_atom.Length() >= sizeof(Format::alac_spec_config) + 4) {
								auto & alac_header = *reinterpret_cast<Format::alac_spec_config *>(ex_atom.GetBuffer() + 4);
								desc.ChannelCount = alac_header.channel_count;
								desc.FramesPerSecond = Network::InverseEndianess(alac_header.frames_per_second);
								if (alac_header.bits_per_sample == 16) desc.Format = Audio::SampleFormat::S16_snorm;
								else if (alac_header.bits_per_sample == 24) desc.Format = Audio::SampleFormat::S24_snorm;
								else if (alac_header.bits_per_sample == 32) desc.Format = Audio::SampleFormat::S32_snorm;
								else desc.Format = Audio::SampleFormat::Invalid;
								if (ex_atom.Length() >= sizeof(Format::alac_spec_config) + sizeof(Format::alac_channel_layout) + 4) {
									auto & alac_cl = *reinterpret_cast<Format::alac_channel_layout *>(ex_atom.GetBuffer() + 4 + sizeof(Format::alac_spec_config));
									uint tag = Network::InverseEndianess(alac_cl.channel_layout_tag);
									if (tag == ((100 << 16) | 1)) channel_layout = Audio::ChannelLayoutCenter;
									else if (tag == ((101 << 16) | 2)) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight;
									else if (tag == ((113 << 16) | 3)) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter;
									else if (tag == ((116 << 16) | 4)) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
										Audio::ChannelLayoutBackCenter;
									else if (tag == ((120 << 16) | 5)) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
										Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight;
									else if (tag == ((124 << 16) | 6)) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
										Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight | Audio::ChannelLayoutLowFrequency;
									else if (tag == ((142 << 16) | 7)) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
										Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight | Audio::ChannelLayoutBackCenter | Audio::ChannelLayoutLowFrequency;
									else if (tag == ((127 << 16) | 8)) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight | Audio::ChannelLayoutCenter |
										Audio::ChannelLayoutBackLeft | Audio::ChannelLayoutBackRight | Audio::ChannelLayoutSideLeft | Audio::ChannelLayoutSideRight |
										Audio::ChannelLayoutLowFrequency;
								} else {
									if (desc.ChannelCount == 1) channel_layout = Audio::ChannelLayoutCenter;
									else if (desc.ChannelCount == 2) channel_layout = Audio::ChannelLayoutLeft | Audio::ChannelLayoutRight;
								}
								_codec_usage_data = new DataBlock(ex_atom.Length() - 4);
								_codec_usage_data->SetLength(ex_atom.Length() - 4);
								MemoryCopy(_codec_usage_data->GetBuffer(), ex_atom.GetBuffer() + 4, _codec_usage_data->Length());
							}
						}
					}
					_desc = new AudioTrackFormatDesc(codec, desc, channel_layout);
				} else throw InvalidFormatException();
			}
			void _load_video_desc(Atom * stsd, Format::mpeg4_stts & stts_table)
			{
				auto & sd = *reinterpret_cast<Format::mpeg4_video_sample_desc *>(stsd->data->GetBuffer() + 8);
				string codec;
				string cfcc = string(sd.fourcc, 4, Encoding::ANSI);
				if (cfcc == L"raw ") {
					auto bpp = Network::InverseEndianess(sd.color_depth);
					if (bpp == 24) codec = Video::VideoFormatRGB;
					else throw InvalidFormatException();
				} else if (cfcc == L"avc1") {
					codec = Video::VideoFormatH264;
				} else throw InvalidFormatException();
				uint rate_num = _track_time_scale;
				uint rate_den = Network::InverseEndianess(stts_table.record_number) == 1 ? Network::InverseEndianess(stts_table.records[0].duration) : 0;
				uint pxw = 1;
				uint pxh = 1;
				int size = Network::InverseEndianess(sd.size);
				int offset = sizeof(sd);
				while (size - offset >= 8) {
					uint32 ex_size = Network::InverseEndianess(*reinterpret_cast<uint32 *>(stsd->data->GetBuffer() + 8 + offset));
					string ex_fourcc = string(stsd->data->GetBuffer() + 12 + offset, 4, Encoding::ANSI);
					if (ex_fourcc == L"pasp" && size - offset >= 16) {
						pxw = Network::InverseEndianess(*reinterpret_cast<uint32 *>(stsd->data->GetBuffer() + 16 + offset));
						pxh = Network::InverseEndianess(*reinterpret_cast<uint32 *>(stsd->data->GetBuffer() + 20 + offset));
					} else if (ex_fourcc == L"esds" && size - offset >= 12) {
						int len = ex_size - 12;
						_codec_usage_data = new DataBlock(len);
						_codec_usage_data->SetLength(len);
						MemoryCopy(_codec_usage_data->GetBuffer(), stsd->data->GetBuffer() + 20 + offset, len);
					} else if (ex_fourcc == L"avcC" && size - offset >= 8) {
						int len = ex_size - 8;
						_codec_usage_data = new DataBlock(len);
						_codec_usage_data->SetLength(len);
						MemoryCopy(_codec_usage_data->GetBuffer(), stsd->data->GetBuffer() + 16 + offset, len);
					}
					offset += ex_size;
				}
				_desc = new VideoTrackFormatDesc(codec, Network::InverseEndianess(sd.width), Network::InverseEndianess(sd.height), rate_num, rate_den, pxw, pxh);
			}
			void _load_subtitle_desc(Atom * stsd)
			{
				auto & sd = *reinterpret_cast<Format::mpeg4_subtitle_sample_desc *>(stsd->data->GetBuffer() + 8);
				string codec;
				string cfcc = string(sd.fourcc, 4, Encoding::ANSI);
				uint flags = 0;
				if (cfcc == L"tx3g") {
					codec = Subtitles::SubtitleFormat3GPPTimedText;
					if (sd.display_flags & 0x00000040) flags |= Subtitles::SubtitleFlagSampleForced;
					if (sd.display_flags & 0x00000080) flags |= Subtitles::SubtitleFlagAllForced;
				} else throw InvalidFormatException();
				_desc = new SubtitleTrackFormatDesc(codec, _track_time_scale, flags);
			}
			void _load_packet_layout(Atom * stbl)
			{
				auto stts = stbl->FindPrimaryChild(L"stts");
				if (!stts) throw InvalidFormatException();
				auto stco = stbl->FindPrimaryChild(L"stco");
				auto co64 = stbl->FindPrimaryChild(L"co64");
				if (!stco && !co64) throw InvalidFormatException();
				auto stsc = stbl->FindPrimaryChild(L"stsc");
				if (!stsc) throw InvalidFormatException();
				auto stsz = stbl->FindPrimaryChild(L"stsz");
				if (!stsz) throw InvalidFormatException();
				auto stss = stbl->FindPrimaryChild(L"stss");
				auto sdtp = stbl->FindPrimaryChild(L"sdtp");
				auto ctts = stbl->FindPrimaryChild(L"ctts");
				int chunk_count;
				if (stco) chunk_count = Network::InverseEndianess(stco->Interpret<Format::mpeg4_stco>().record_number);
				else chunk_count = Network::InverseEndianess(co64->Interpret<Format::mpeg4_co64>().record_number);
				auto & stts_table = stts->Interpret<Format::mpeg4_stts>();
				auto & stsc_table = stsc->Interpret<Format::mpeg4_stsc>();
				auto & stsz_table = stsz->Interpret<Format::mpeg4_stsz>();
				int stsc_index = 0;
				for (int chunk = 0; chunk < chunk_count; chunk++) {
					if (stsc_index + 1 < int(Network::InverseEndianess(stsc_table.record_number))) {
						int next_chunk = Network::InverseEndianess(stsc_table.records[stsc_index + 1].first_chunk);
						if (chunk + 1 >= next_chunk) stsc_index++;
					}
					uint64 chunk_org;
					if (stco) chunk_org = Network::InverseEndianess(stco->Interpret<Format::mpeg4_stco>().offsets[chunk]);
					else {
						chunk_org = co64->Interpret<Format::mpeg4_co64>().offsets[chunk];
						chunk_org = ((chunk_org & 0x00000000000000FF) << 56) | ((chunk_org & 0x000000000000FF00) << 40) |
							((chunk_org & 0x0000000000FF0000) << 24) | ((chunk_org & 0x00000000FF000000) << 8) | ((chunk_org & 0x000000FF00000000) >> 8) |
							((chunk_org & 0x0000FF0000000000) >> 24) | ((chunk_org & 0x00FF000000000000) >> 40) | ((chunk_org & 0xFF00000000000000) >> 56);
					}
					auto & rec = stsc_table.records[stsc_index];
					int num_packets = Network::InverseEndianess(rec.num_packets);
					for (int p = 0; p < num_packets; p++) {
						_packets << packet_desc();
						auto & pr = _packets.LastElement();
						auto pi = _packets.Length() - 1;
						pr.file_offset = chunk_org;
						pr.file_size = Network::InverseEndianess(stsz_table.common_size ? stsz_table.common_size : stsz_table.sizes[pi]);
						chunk_org += pr.file_size;
						pr.flags = pr.decode_time = pr.render_time = pr.render_duration = 0;
					}
				}
				uint32 dec_time = 0;
				int le = Network::InverseEndianess(stts_table.record_number);
				int bs = 0;
				for (int i = 0; i < le; i++) {
					auto & rec = stts_table.records[i];
					uint32 time = Network::InverseEndianess(rec.duration);
					int ns = Network::InverseEndianess(rec.num_samples);
					for (int s = 0; s < ns; s++) {
						_packets[bs + s].render_duration = time;
						_packets[bs + s].decode_time = dec_time;
						dec_time += time;
					}
					bs += ns;
				}
				if (stss) {
					auto & stss_table = stss->Interpret<Format::mpeg4_stss>();
					int ne = Network::InverseEndianess(stss_table.record_number);
					for (int i = 0; i < ne; i++) {
						int index = Network::InverseEndianess(stss_table.index[i]);
						_packets[index - 1].flags = 1;
					}
				} else for (auto & p : _packets) p.flags = 1;
				if (ctts) {
					auto & ctts_table = ctts->Interpret<Format::mpeg4_ctts>();
					int ne = Network::InverseEndianess(ctts_table.record_number);
					int pi = 0;
					int32 min_rt = 0;
					for (int i = 0; i < ne; i++) {
						int ns = Network::InverseEndianess(ctts_table.records[i].sample_count);
						int32 offs = Network::InverseEndianess(ctts_table.records[i].composition_offset);
						for (int j = 0; j < ns; j++) {
							_packets[pi + j].render_time = _packets[pi + j].decode_time + offs;
							if (!i && !j) min_rt = _packets[0].render_time;
							else if (int32(_packets[pi + j].render_time) < min_rt) min_rt = _packets[pi + j].render_time;
						}
						pi += ns;
					}
					for (auto & p : _packets) p.render_time -= min_rt;
				} else for (auto & p : _packets) p.render_time = p.decode_time;
			}
			void _load_track_data(Atom * mdia)
			{
				auto mdhd = mdia->FindPrimaryChild(L"mdhd");
				if (!mdhd) throw InvalidFormatException();
				auto & media_header = mdhd->Interpret<Format::mpeg4_mdhd>();
				_track_time_scale = Network::InverseEndianess(media_header.time_scale);
				_track_time_duration = Network::InverseEndianess(media_header.duration);
				auto lang = Network::InverseEndianess(media_header.language);
				if (lang >= 0x400 && lang < 0x7FFF) {
					uint8 str[3];
					str[2] = (lang & 0x001F) + 0x60;
					str[1] = ((lang & 0x03E0) >> 5) + 0x60;
					str[0] = ((lang & 0x7C00) >> 10) + 0x60;
					_language = string(str, 3, Encoding::ANSI);
				}
				auto hdlr = mdia->FindPrimaryChild(L"hdlr");
				auto minf = mdia->FindPrimaryChild(L"minf");
				if (!hdlr || !minf) throw InvalidFormatException();
				auto & handler = hdlr->Interpret<Format::mpeg4_hdlr>();
				TrackClass track_class;
				if (MemoryCompare(handler.com_subtype, "soun", 4) == 0) {
					track_class = TrackClass::Audio;
				} else if (MemoryCompare(handler.com_subtype, "vide", 4) == 0) {
					track_class = TrackClass::Video;
				} else if (MemoryCompare(handler.com_subtype, "subt", 4) == 0 || MemoryCompare(handler.com_subtype, "sbtl", 4) == 0) {
					track_class = TrackClass::Subtitles;
				} else throw InvalidFormatException();
				auto stbl = minf->FindPrimaryChild(L"stbl");
				if (!stbl) throw InvalidFormatException();
				_load_packet_layout(stbl);
				auto stsd = stbl->FindPrimaryChild(L"stsd");
				if (!stsd) throw InvalidFormatException();
				auto stts = stbl->FindPrimaryChild(L"stts");
				auto & stsd_hdr = stsd->Interpret<Format::mpeg4_stsd>();
				auto & stts_table = stts->Interpret<Format::mpeg4_stts>();
				auto num_entries = Network::InverseEndianess(stsd_hdr.record_number);
				if (num_entries != 1) throw InvalidFormatException();
				if (track_class == TrackClass::Audio) {
					_load_audio_desc(stsd);
				} else if (track_class == TrackClass::Video) {
					_load_video_desc(stsd, stts_table);
				} else if (track_class == TrackClass::Subtitles) {
					_load_subtitle_desc(stsd);
				}
				if (!_desc) throw InvalidFormatException();
				_desc->SetCodecMagic(_codec_usage_data);
			}
			void _relocate_packets(int64 shift_by) { for (auto & p : _packets) p.file_offset += shift_by; }
		public:
			MPEG4TrackSource(Streaming::Stream * stream, IMediaContainerSource * source, Atom * trak, uint32 common_time_scale) : _packets(0x10000)
			{
				_current_packet = 0;
				_current_time_local_scale = 0;
				_stream.SetRetain(stream);
				_trak.SetRetain(trak);
				_codec.SetRetain(source->GetParentCodec());
				_source = source;
				_common_time_scale = common_time_scale;
				auto tkhd = trak->FindPrimaryChild(L"tkhd");
				if (!tkhd) throw InvalidFormatException();
				auto & header = tkhd->Interpret<Format::mpeg4_tkhd>();
				if (header.flag_version & 0x01000000) _visible = true; else _visible = false;
				_id = Network::InverseEndianess(header.track_id);
				_group = Network::InverseEndianess(header.group);
				_common_time_duration = Network::InverseEndianess(header.duration);
				if (trak->FindPrimaryChild(L"txas")) _autoselectable = false; else _autoselectable = true;
				auto udta = trak->FindPrimaryChild(L"udta");
				if (udta) {
					auto name = udta->FindPrimaryChild(L"name");
					if (name) _name = string(name->data->GetBuffer(), name->data->Length(), Encoding::UTF8);
				}
				auto mdia = trak->FindPrimaryChild(L"mdia");
				if (!mdia) throw InvalidFormatException();
				_load_track_data(mdia);
			}
			virtual ~MPEG4TrackSource(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
			virtual IMediaContainer * GetParentContainer(void) const noexcept override { return _source; }
			virtual TrackClass GetTrackClass(void) const noexcept override { return _desc->GetTrackClass(); }
			virtual const TrackFormatDesc & GetFormatDescriptor(void) const noexcept override { return *_desc; }
			virtual string GetTrackName(void) const override { return _name; }
			virtual string GetTrackLanguage(void) const override { return _language; }
			virtual bool IsTrackVisible(void) const noexcept override { return _visible; }
			virtual bool IsTrackAutoselectable(void) const noexcept override { return _autoselectable; }
			virtual int GetTrackGroup(void) const noexcept override { return _group; }
			virtual uint64 GetTimeScale(void) const noexcept override { return _track_time_scale; }
			virtual uint64 GetDuration(void) const noexcept override { return _track_time_duration; }
			virtual uint64 GetPosition(void) const noexcept override { return _current_time_local_scale; }
			virtual uint64 Seek(uint64 time) noexcept override
			{
				uint32 lt = time;
				int pm = 0, px = _packets.Length() - 1;
				while (px > pm) {
					int pc = (px + pm + 1) / 2;
					if (_packets[pc].decode_time > lt) px = max(pc - 1, 0); else pm = pc;
				}
				while (pm > 0 && !_packets[pm].flags) pm--;
				_current_packet = pm;
				_current_time_local_scale = _packets[pm].decode_time;
				return _current_time_local_scale;
			}
			virtual uint64 GetCurrentPacket(void) const noexcept override { return _current_packet; }
			virtual uint64 GetPacketCount(void) const noexcept override { return _packets.Length(); }
			virtual bool ReadPacket(PacketBuffer & buffer) noexcept override
			{
				if (_current_packet < _packets.Length()) {
					try {
						auto & packet = _packets[_current_packet];
						if (!buffer.PacketData || buffer.PacketData->Length() < packet.file_size) {
							buffer.PacketData = new DataBlock(packet.file_size);
							buffer.PacketData->SetLength(packet.file_size);
						}
						buffer.PacketDataActuallyUsed = packet.file_size;
						_stream->Seek(packet.file_offset, Streaming::Begin);
						_stream->Read(buffer.PacketData->GetBuffer(), packet.file_size);
						buffer.PacketIsKey = packet.flags != 0;
						buffer.PacketDecodeTime = packet.decode_time;
						buffer.PacketRenderTime = packet.render_time;
						buffer.PacketRenderDuration = packet.render_duration;
						_current_packet++;
						_current_time_local_scale = _current_packet < _packets.Length() ? _packets[_current_packet].decode_time : _track_time_duration;
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
		class MPEG4Source : public IMediaContainerSource
		{
			SafePointer<Streaming::Stream> _source;
			SafePointer<IMediaContainerCodec> _codec;
			ContainerClassDesc _desc;
			SafePointer<Atom> _ftyp, _moov, _mdat;
			SafePointer<Atom> _itunes_md_holder;
			ObjectArray<MPEG4TrackSource> _tracks;
			uint32 _media_time_unit;
			uint32 _duration;

			static void _read_track_number(Atom * root, uint32 & number, uint32 & count)
			{
				number = count = 0;
				for (auto & atom : root->children) if (atom.fourcc == L"data") {
					number = atom.data->ElementAt(11);
					count = atom.data->ElementAt(13);
					return;
				}
			}
			static string _read_string(Atom * root)
			{
				for (auto & atom : root->children) if (atom.fourcc == L"data") {
					return string(atom.data->GetBuffer() + 8, atom.data->Length() - 8, Encoding::UTF8);
				}
				return L"";
			}
			static uint32 _read_number(Atom * root)
			{
				for (auto & atom : root->children) if (atom.fourcc == L"data") {
					auto size = atom.data->Length() - 8;
					if (size == 1) return atom.data->ElementAt(8);
					else if (size == 2) return Network::InverseEndianess(*reinterpret_cast<uint16 *>(atom.data->GetBuffer() + 8));
					else if (size == 4) return Network::InverseEndianess(*reinterpret_cast<uint32 *>(atom.data->GetBuffer() + 8));
					else return 0;
				}
				return 0;
			}
			static DataBlock * _read_picture(Atom * root, string & format)
			{
				for (auto & atom : root->children) if (atom.fourcc == L"data") {
					auto size = atom.data->Length() - 8;
					SafePointer<DataBlock> result = new DataBlock(size);
					result->SetLength(size);
					MemoryCopy(result->GetBuffer(), atom.data->GetBuffer() + 8, size);
					format = atom.data->ElementAt(3) == 13 ? Codec::ImageFormatJPEG : Codec::ImageFormatPNG;
					result->Retain();
					return result;
				}
				return 0;
			}
			static Metadata * _read_metadata(Atom * ilst)
			{
				SafePointer<Metadata> result = new Metadata;
				for (auto & atom : ilst->children) {
					if (atom.fourcc == L"trkn") {
						uint32 no, cnt; _read_track_number(&atom, no, cnt);
						if (no) result->Append(MetadataKey::TrackNumber, no);
						if (cnt) result->Append(MetadataKey::TrackCount, cnt);
					} else if (atom.fourcc == L"disk") {
						uint32 no, cnt; _read_track_number(&atom, no, cnt);
						if (no) result->Append(MetadataKey::DiskNumber, no);
						if (cnt) result->Append(MetadataKey::DiskCount, cnt);
					} else if (atom.fourcc == L"\251alb") result->Append(MetadataKey::Album, _read_string(&atom));
					else if (atom.fourcc == L"\251art") result->Append(MetadataKey::Artist, _read_string(&atom));
					else if (atom.fourcc == L"\251ART") result->Append(MetadataKey::Artist, _read_string(&atom));
					else if (atom.fourcc == L"aART") result->Append(MetadataKey::AlbumArtist, _read_string(&atom));
					else if (atom.fourcc == L"\251cmt") result->Append(MetadataKey::Comment, _read_string(&atom));
					else if (atom.fourcc == L"\251day") result->Append(MetadataKey::Year, _read_string(&atom).Fragment(0, 4));
					else if (atom.fourcc == L"\251nam") result->Append(MetadataKey::Title, _read_string(&atom));
					else if (atom.fourcc == L"\251gen") result->Append(MetadataKey::Genre, _read_string(&atom));
					else if (atom.fourcc == L"\251wrt") result->Append(MetadataKey::Composer, _read_string(&atom));
					else if (atom.fourcc == L"\251too") result->Append(MetadataKey::Encoder, _read_string(&atom));
					else if (atom.fourcc == L"cprt") result->Append(MetadataKey::Copyright, _read_string(&atom));
					else if (atom.fourcc == L"desc") result->Append(MetadataKey::Description, _read_string(&atom));
					else if (atom.fourcc == L"\251lyr") result->Append(MetadataKey::Lyrics, _read_string(&atom));
					else if (atom.fourcc == L"tmpo") result->Append(MetadataKey::BeatsPerMinute, _read_number(&atom));
					else if (atom.fourcc == L"gnre") result->Append(MetadataKey::GenreIndex, _read_number(&atom) - 1);
					else if (atom.fourcc == L"covr") {
						string format;
						SafePointer<DataBlock> picture = _read_picture(&atom, format);
						if (picture) result->Append(MetadataKey::Artwork, MetadataValue(picture, format));
					} else int p = 6;
				}
				result->Retain();
				return result;
			}
		public:
			MPEG4Source(Streaming::Stream * source, IMediaContainerCodec * codec, const ContainerClassDesc & desc) : _desc(desc)
			{
				_source.SetRetain(source);
				_codec.SetRetain(codec);
				Atom * atom;
				_source->Seek(0, Streaming::Begin);
				auto size = _source->Length();
				while (atom = Atom::ReadAtom(_source, size, true)) {
					if (atom->fourcc == L"ftyp" && !_ftyp) _ftyp.SetRetain(atom);
					else if (atom->fourcc == L"moov" && !_moov) _moov.SetRetain(atom);
					else if (atom->fourcc == L"mdat" && !_mdat) _mdat.SetRetain(atom);
					atom->Release();
				}
				if (!_ftyp || !_moov || !_mdat) throw InvalidFormatException();
				auto mvhd = _moov->FindPrimaryChild(L"mvhd");
				auto & header = mvhd->Interpret<Format::mpeg4_mvhd>();
				_media_time_unit = Network::InverseEndianess(header.time_scale);
				_duration = Network::InverseEndianess(header.duration);
				for (auto & a : _moov->children) if (a.fourcc == L"trak") {
					try {
						SafePointer<MPEG4TrackSource> track = new MPEG4TrackSource(_source, this, &a, _media_time_unit);
						_tracks.Append(track);
					} catch (...) {}
					if (!_tracks.Length()) throw InvalidFormatException();
				}
				auto udta = _moov->FindPrimaryChild(L"udta");
				auto meta = udta ? udta->FindPrimaryChild(L"meta") : 0;
				if (meta) _itunes_md_holder.SetRetain(meta);
			}
			virtual ~MPEG4Source(void) override {}
			virtual ContainerObjectType GetContainerObjectType(void) const noexcept override { return ContainerObjectType::Source; }
			virtual IMediaContainerCodec * GetParentCodec(void) const noexcept override { return _codec; }
			virtual const ContainerClassDesc & GetFormatDescriptor(void) const noexcept override { return _desc; }
			virtual Metadata * ReadMetadata(void) const noexcept override
			{
				if (_itunes_md_holder) {
					try {
						SafePointer<Atom> atom = _itunes_md_holder->iTunesMetadataAtom();
						auto ilst = atom->FindPrimaryChild(L"ilst");
						if (!ilst) return 0;
						return _read_metadata(ilst);
					} catch (...) { return 0; }
				} else return 0;
			}
			virtual int GetTrackCount(void) const noexcept override { return _tracks.Length(); }
			virtual IMediaTrack * GetTrack(int index) const noexcept override
			{
				if (index < 0 || index >= _tracks.Length()) return 0;
				return _tracks.ElementAt(index);
			}
			virtual IMediaTrackSource * OpenTrack(int index) const noexcept override
			{
				if (index < 0 || index >= _tracks.Length()) return 0;
				_tracks.ElementAt(index)->Retain();
				return _tracks.ElementAt(index);
			}
			virtual uint64 GetDuration(void) const noexcept override { return uint64(_duration) * 1000 / uint64(_media_time_unit); }
			virtual bool PatchMetadata(const Metadata * metadata) noexcept override
			{
				try {
					for (int i = 0; i < _moov->children.Length(); i++) if (_moov->children[i].fourcc == L"udta") {
						_moov->children.Remove(i);
						break;
					}
					if (metadata) {
						SafePointer<Atom> udta = MakeMetadataAtom(metadata);
						_moov->children.Append(udta);
					}
					uint64 moov_length = _moov->Length();
					uint64 ftyp_length = _ftyp->Length();
					uint64 mdat_offset_prev = _mdat->offset;
					uint64 mdat_offset_new = ftyp_length + moov_length;
					uint64 mdat_size = _mdat->length;
					uint64 relocate_by = mdat_offset_new - mdat_offset_prev;
					if (mdat_offset_new > mdat_offset_prev) {
						_source->SetLength(mdat_offset_new + mdat_size);
						_source->RelocateData(mdat_offset_prev, mdat_offset_new, mdat_size);
					} else if (mdat_offset_new < mdat_offset_prev) {
						_source->RelocateData(mdat_offset_prev, mdat_offset_new, mdat_size);
						_source->SetLength(mdat_offset_new + mdat_size);
					} else _source->SetLength(mdat_offset_new + mdat_size);
					for (auto & t : _tracks) t._relocate_packets(relocate_by);
					_mdat->offset = mdat_offset_new;
					_moov->ChunkTableRelocate(relocate_by);
					_source->Seek(0, Streaming::Begin);
					_ftyp->WriteToFile(_source);
					_moov->WriteToFile(_source);
					auto udta = _moov->FindPrimaryChild(L"udta");
					auto meta = udta ? udta->FindPrimaryChild(L"meta") : 0;
					if (meta) _itunes_md_holder.SetRetain(meta);
					else _itunes_md_holder.SetReference(0);
					return true;
				} catch (...) { return false; }
			}
			virtual bool PatchMetadata(const Metadata * metadata, Streaming::Stream * dest) const noexcept override
			{
				try {
					if (dest == _source.Inner()) return false;
					dest->SetLength(0);
					dest->Seek(0, Streaming::Begin);
					SafePointer<Atom> moov = _moov->Clone();
					for (int i = 0; i < moov->children.Length(); i++) if (moov->children[i].fourcc == L"udta") {
						moov->children.Remove(i);
						break;
					}
					if (metadata) {
						SafePointer<Atom> udta = MakeMetadataAtom(metadata);
						moov->children.Append(udta);
					}
					uint64 moov_length = moov->Length();
					uint64 ftyp_length = _ftyp->Length();
					uint64 mdat_offset_prev = _mdat->offset;
					uint64 mdat_offset_new = ftyp_length + moov_length;
					uint64 mdat_size = _mdat->length;
					uint64 relocate_by = mdat_offset_new - mdat_offset_prev;
					moov->ChunkTableRelocate(relocate_by);
					_ftyp->WriteToFile(dest);
					moov->WriteToFile(dest);
					_source->Seek(mdat_offset_prev, Streaming::Begin);
					_source->CopyTo(dest, mdat_size);
					return true;
				} catch (...) { return false; }
			}
		};

		class TX3GEncoder : public Subtitles::ISubtitleEncoder
		{
			SafePointer<Subtitles::ISubtitleCodec> _parent;
			Subtitles::SubtitleDesc _subt_desc;
			SafePointer<SubtitleTrackFormatDesc> _track_desc;
			uint _current_time;
			Array<PacketBuffer> _output;
		public:
			TX3GEncoder(Subtitles::ISubtitleCodec * codec, const string & format, const Subtitles::SubtitleDesc & desc) : _output(0x40)
			{
				if (format != Subtitles::SubtitleFormat3GPPTimedText) throw InvalidFormatException();
				_parent.SetRetain(codec);
				_subt_desc = desc;
				_track_desc = new SubtitleTrackFormatDesc(Subtitles::SubtitleFormat3GPPTimedText, _subt_desc.TimeScale, _subt_desc.Flags);
				_current_time = 0;
			}
			virtual ~TX3GEncoder(void) override {}
			virtual const Subtitles::SubtitleDesc & GetObjectDescriptor(void) const noexcept override { return _subt_desc; }
			virtual Subtitles::ISubtitleCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return Subtitles::SubtitleFormat3GPPTimedText; }
			virtual bool Reset(void) noexcept override { _current_time = 0; _output.Clear(); return true; }
			virtual int GetPendingPacketsCount(void) const noexcept override { return _output.Length(); }
			virtual int GetPendingSamplesCount(void) const noexcept override { return 0; }
			virtual const Media::SubtitleTrackFormatDesc & GetEncodedDescriptor(void) const noexcept override { return *_track_desc; }
			virtual const DataBlock * GetCodecMagic(void) noexcept override { return 0; }
			virtual bool SupplySample(const Subtitles::SubtitleSample & sample) noexcept override
			{
				try {
					if (sample.TimePresent > _current_time) {
						PacketBuffer buffer;
						buffer.PacketData = new DataBlock(2);
						buffer.PacketData->Append(0);
						buffer.PacketData->Append(0);
						buffer.PacketDataActuallyUsed = 2;
						buffer.PacketIsKey = true;
						buffer.PacketDecodeTime = buffer.PacketRenderTime = _current_time;
						buffer.PacketRenderDuration = sample.TimePresent - _current_time;
						_output << buffer;
						_current_time = sample.TimePresent;
					}
					PacketBuffer buffer;
					buffer.PacketData = new DataBlock(0x80);
					uint16 length = sample.Text.GetEncodedLength(Encoding::UTF8);
					buffer.PacketData->Append(length >> 8);
					buffer.PacketData->Append(length & 0xFF);
					buffer.PacketData->SetLength(2 + length);
					sample.Text.Encode(buffer.PacketData->GetBuffer() + 2, Encoding::UTF8, false);
					if (((sample.Flags & Subtitles::SubtitleFlagSampleForced) && (_subt_desc.Flags & Subtitles::SubtitleFlagSampleForced)) ||
						(_subt_desc.Flags & Subtitles::SubtitleFlagAllForced)) {
						for (int i = 0; i < 3; i++) buffer.PacketData->Append(0);
						buffer.PacketData->Append(8);
						buffer.PacketData->Append('f');
						buffer.PacketData->Append('r');
						buffer.PacketData->Append('c');
						buffer.PacketData->Append('d');
					}
					if (sample.Flags & Subtitles::SubtitleFlagAllowWrap) {
						for (int i = 0; i < 3; i++) buffer.PacketData->Append(0);
						buffer.PacketData->Append(9);
						buffer.PacketData->Append('t');
						buffer.PacketData->Append('w');
						buffer.PacketData->Append('r');
						buffer.PacketData->Append('p');
						buffer.PacketData->Append(1);
					}
					buffer.PacketDataActuallyUsed = buffer.PacketData->Length();
					buffer.PacketIsKey = true;
					buffer.PacketDecodeTime = buffer.PacketRenderTime = _current_time;
					buffer.PacketRenderDuration = sample.Duration;
					_output << buffer;
					_current_time += sample.Duration;
					return true;
				} catch (...) { return false; }
			}
			virtual bool SupplyEndOfStream(void) noexcept override
			{
				try {
					PacketBuffer buffer;
					buffer.PacketDataActuallyUsed = buffer.PacketRenderDuration = 0;
					buffer.PacketIsKey = true;
					buffer.PacketRenderTime = buffer.PacketDecodeTime = _current_time;
					_output << buffer;
					return true;
				} catch (...) { return false; }
			}
			virtual bool ReadPacket(Media::PacketBuffer & packet) noexcept override { if (!_output.Length()) return false; packet = _output.FirstElement(); _output.RemoveFirst(); return true; }
		};
		class TX3GDecoder : public Subtitles::ISubtitleDecoder
		{
			SafePointer<Subtitles::ISubtitleCodec> _parent;
			Subtitles::SubtitleDesc _subt_desc;
			Array<Subtitles::SubtitleSample> _output;
		public:
			TX3GDecoder(Subtitles::ISubtitleCodec * codec, const Media::TrackFormatDesc & format) : _output(0x20)
			{
				if (format.GetTrackClass() != TrackClass::Subtitles) throw InvalidFormatException();
				if (format.GetTrackCodec() != Subtitles::SubtitleFormat3GPPTimedText) throw InvalidFormatException();
				_parent.SetRetain(codec);
				auto & sd = format.As<SubtitleTrackFormatDesc>();
				_subt_desc.TimeScale = sd.GetTimeScale();
				_subt_desc.Flags = sd.GetFlags();
			}
			virtual ~TX3GDecoder(void) override {}
			virtual const Subtitles::SubtitleDesc & GetObjectDescriptor(void) const noexcept override { return _subt_desc; }
			virtual Subtitles::ISubtitleCodec * GetParentCodec(void) const override { return _parent; }
			virtual string GetEncodedFormat(void) const override { return Subtitles::SubtitleFormat3GPPTimedText; }
			virtual bool Reset(void) noexcept override { _output.Clear(); return true; }
			virtual int GetPendingPacketsCount(void) const noexcept override { return 0; }
			virtual int GetPendingSamplesCount(void) const noexcept override { return _output.Length(); }
			virtual bool SupplyPacket(const Media::PacketBuffer & packet) noexcept override
			{
				try {
					if (packet.PacketDataActuallyUsed) {
						if (packet.PacketDataActuallyUsed < 2) return false;
						uint length = (uint(packet.PacketData->ElementAt(0)) << 8) | packet.PacketData->ElementAt(1);
						if (!length) return true;
						if (packet.PacketDataActuallyUsed < 2 + length) return false;
						Subtitles::SubtitleSample result;
						if (length >= 2 && packet.PacketData->ElementAt(2) == 0xFF && packet.PacketData->ElementAt(3) == 0xFE) {
							result.Text = string(packet.PacketData->GetBuffer() + 4, length - 2, Encoding::UTF16);
						} else if (length >= 2 && packet.PacketData->ElementAt(2) == 0xFE && packet.PacketData->ElementAt(3) == 0xFF) {
							Array<uint8> reversed(0x100);
							for (uint i = 0; i < length - 2; i++) reversed << packet.PacketData->ElementAt(4 + i);
							for (uint i = 0; i < reversed.Length() - 1; i += 2) swap(reversed[i], reversed[i + 1]);
							result.Text = string(reversed.GetBuffer(), length - 2, Encoding::UTF16);
						} else {
							result.Text = string(packet.PacketData->GetBuffer() + 2, length, Encoding::UTF8);
						}
						result.Flags = 0;
						if (_subt_desc.Flags & Subtitles::SubtitleFlagAllForced) result.Flags |= Subtitles::SubtitleFlagSampleForced;
						int ext_read_pos = 2 + length;
						while (ext_read_pos + 8 <= packet.PacketDataActuallyUsed) {
							auto ext_len = Network::InverseEndianess(*reinterpret_cast<const uint32 *>(packet.PacketData->GetBuffer() + ext_read_pos));
							auto fcc = string(packet.PacketData->GetBuffer() + ext_read_pos + 4, 4, Encoding::ANSI);
							if (fcc == L"frcd") {
								result.Flags |= Subtitles::SubtitleFlagSampleForced;
							} else if (fcc == L"twrp" && ext_read_pos + 9 <= packet.PacketDataActuallyUsed) {
								if (packet.PacketData->ElementAt(ext_read_pos + 8)) result.Flags |= Subtitles::SubtitleFlagAllowWrap;
							}
							ext_read_pos += ext_len;
						}
						result.TimeScale = _subt_desc.TimeScale;
						result.TimePresent = packet.PacketRenderTime;
						result.Duration = packet.PacketRenderDuration;
						_output << result;
					}
					return true;
				} catch (...) { return false; }
			}
			virtual bool ReadSample(Subtitles::SubtitleSample & sample) noexcept override { if (!_output.Length()) return false; try { sample = _output.FirstElement(); _output.RemoveFirst(); return true; } catch (...) { return false; } }
		};

		class EngineMPEG4Codec : public IMediaContainerCodec
		{
			ContainerClassDesc mpeg4;
		public:
			EngineMPEG4Codec(void)
			{
				mpeg4.ContainerFormatIdentifier = ContainerFormatMPEG4;
				mpeg4.MetadataFormatIdentifier = MetadataFormatiTunes;
				mpeg4.FormatCapabilities = ContainerClassCapabilityHoldAudio | ContainerClassCapabilityHoldVideo | ContainerClassCapabilityHoldSubtitles |
					ContainerClassCapabilityHoldMetadata | ContainerClassCapabilityInterleavedTracks | ContainerClassCapabilityKeyFrames;
				mpeg4.MaximalTrackCount = 0;
			}
			virtual ~EngineMPEG4Codec(void) override {}
			virtual bool CanEncode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatMPEG4) {
					if (desc) *desc = mpeg4;
					return true;
				} else return false;
			}
			virtual bool CanDecode(const string & format, ContainerClassDesc * desc = 0) const noexcept override
			{
				if (format == ContainerFormatMPEG4) {
					if (desc) *desc = mpeg4;
					return true;
				} else return false;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(0x1);
				result->Append(mpeg4);
				result->Retain();
				return result;
			}
			virtual Array<ContainerClassDesc> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<ContainerClassDesc> > result = new Array<ContainerClassDesc>(0x1);
				result->Append(mpeg4);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Engine MPEG-4 Multiplexor"; }
			virtual IMediaContainerSource * OpenContainer(Streaming::Stream * source) noexcept override
			{
				try {
					uint8 sign[8];
					source->Seek(0, Streaming::Begin);
					source->Read(sign, 8);
					if (MemoryCompare(sign + 4, "ftyp", 4) == 0) return new MPEG4Source(source, this, mpeg4);
					else return 0;
				} catch (...) { return 0; }
			}
			virtual IMediaContainerSink * CreateContainer(Streaming::Stream * dest, const string & format) noexcept override
			{
				if (format == ContainerFormatMPEG4) { try { return new MPEG4Sink(dest, this, mpeg4); } catch (...) { return 0; } }
				else return 0;
			}
		};
		class Engine3GPPTTCodec : public Subtitles::ISubtitleCodec
		{
		public:
			virtual bool CanEncode(const string & format) const noexcept override { return format == Subtitles::SubtitleFormat3GPPTimedText; }
			virtual bool CanDecode(const string & format) const noexcept override { return format == Subtitles::SubtitleFormat3GPPTimedText; }
			virtual Array<string> * GetFormatsCanEncode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(0x1);
				result->Append(Subtitles::SubtitleFormat3GPPTimedText);
				result->Retain();
				return result;
			}
			virtual Array<string> * GetFormatsCanDecode(void) const override
			{
				SafePointer< Array<string> > result = new Array<string>(0x1);
				result->Append(Subtitles::SubtitleFormat3GPPTimedText);
				result->Retain();
				return result;
			}
			virtual string GetCodecName(void) const override { return L"Engine 3GPP Timed Text Codec"; }
			virtual Subtitles::ISubtitleDecoder * CreateDecoder(const Media::TrackFormatDesc & format) noexcept override { try { return new TX3GDecoder(this, format); } catch (...) { return 0; } }
			virtual Subtitles::ISubtitleEncoder * CreateEncoder(const string & format, const Subtitles::SubtitleDesc & desc) noexcept override { try { return new TX3GEncoder(this, format, desc); } catch (...) { return 0; } }
		};

		SafePointer<IMediaContainerCodec> _mpeg4_codec;

		IMediaContainerCodec * InitializeMPEG4Codec(void)
		{
			if (!_mpeg4_codec) {
				_mpeg4_codec = new EngineMPEG4Codec;
				RegisterCodec(_mpeg4_codec);
			}
			return _mpeg4_codec;
		}
	}
	namespace Subtitles
	{
		SafePointer<ISubtitleCodec> _3gpp_tt_codec;

		ISubtitleCodec * Initialize3GPPTTCodec(void)
		{
			if (!_3gpp_tt_codec) {
				_3gpp_tt_codec = new Media::Engine3GPPTTCodec;
				RegisterCodec(_3gpp_tt_codec);
			}
			return _3gpp_tt_codec;
		}
	}
}