#pragma once

#include "../Streaming.h"
#include "../Miscellaneous/Dictionary.h"
#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace Media
	{
		constexpr const widechar * MetadataFormatMPEG3ID3 = L"MPEG3/ID3";
		constexpr const widechar * MetadataFormatMPEG4iTunes = L"MPEG4/iTunes";
		constexpr const widechar * MetadataFormatFreeLossless = L"FLAC/VC";

		enum class MetadataKey : uint {
			Title = 0x00010000, Album = 0x00010001, Artist = 0x00010002, AlbumArtist = 0x00010003,
			Year = 0x00010004, Genre = 0x00010005, Composer = 0x00010006, Encoder = 0x00010007,
			Copyright = 0x00010008, Description = 0x00010009, Publisher = 0x0001000A,
			Comment = 0x00020000, Lyrics = 0x00020001,
			TrackNumber = 0x00030000, TrackCount = 0x00030001, DiskNumber = 0x00030002, DiskCount = 0x00030003, BeatsPerMinute = 0x00030004,
			Artwork = 0x00040000
		};
		bool MetadataKeyIsString(MetadataKey key);
		bool MetadataKeyIsMultilineString(MetadataKey key);
		bool MetadataKeyIsInteger(MetadataKey key);
		bool MetadataKeyIsPicture(MetadataKey key);

		struct MetadataValue
		{
			string Text;
			uint32 Number;
			SafePointer<Codec::Frame> Picture;

			MetadataValue(void);
			MetadataValue(const string & text);
			MetadataValue(uint32 number);
			MetadataValue(Codec::Frame * picture, const string & encoder);

			operator string (void) const;
			operator uint32 (void) const;
			operator Codec::Frame * (void) const;
			bool friend operator == (const MetadataValue & a, const MetadataValue & b);
		};

		typedef Dictionary::PlainDictionary<MetadataKey, MetadataValue> Metadata;

		bool EncodeMetadata(Streaming::Stream * stream, Metadata * metadata, string * metadata_format = 0);
		bool EncodeMetadata(Streaming::Stream * source, Streaming::Stream * dest, Metadata * metadata, string * metadata_format = 0);
		Metadata * DecodeMetadata(Streaming::Stream * stream, string * metadata_format = 0);
	}
}