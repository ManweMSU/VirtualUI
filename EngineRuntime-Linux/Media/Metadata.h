#pragma once

#include "../Miscellaneous/Dictionary.h"

namespace Engine
{
	namespace Media
	{
		enum class MetadataKey : uint {
			Title = 0x00010000, Album = 0x00010001, Artist = 0x00010002, AlbumArtist = 0x00010003,
			Year = 0x00010004, Genre = 0x00010005, Composer = 0x00010006, Encoder = 0x00010007,
			Copyright = 0x00010008, Description = 0x00010009, Publisher = 0x0001000A,
			Comment = 0x00020000, Lyrics = 0x00020001,
			TrackNumber = 0x00030000, TrackCount = 0x00030001, DiskNumber = 0x00030002, DiskCount = 0x00030003,
			BeatsPerMinute = 0x00030004, GenreIndex = 0x00030005,
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
			SafePointer<DataBlock> Picture;

			MetadataValue(void);
			MetadataValue(const string & text);
			MetadataValue(uint32 number);
			MetadataValue(DataBlock * picture, const string & encoder);

			operator string (void) const;
			operator uint32 (void) const;
			operator DataBlock * (void) const;
			bool friend operator == (const MetadataValue & a, const MetadataValue & b);
		};

		typedef Dictionary::PlainDictionary<MetadataKey, MetadataValue> Metadata;

		Metadata * CloneMetadata(const Metadata * metadata);
	}
}