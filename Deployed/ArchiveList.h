#pragma once

#include "Archive.h"

namespace Engine
{
	namespace Storage
	{
		class ArchiveList : public Object
		{
		public:
			enum class ArchiveTime { None, Manual, Automatic, SourceTime, ArchivationTime };
			struct FileTime
			{
				ArchiveTime Class;
				Time Value;
			};
			struct FileAttribute
			{
				string Key;
				string Value;
			};
			struct FileInfo
			{
				string SourcePath;
				Array<uint8> SourceData;
				string Name;
				string Type;
				uint32 ID;
				uint32 Custom;
				FileTime CreationTime;
				FileTime AccessTime;
				FileTime AlterTime;
				Array<FileAttribute> Attributes = Array<FileAttribute>(0x10);
				MethodChain Compression;
				CompressionQuality CompressionQuality;
				uint32 CompressionBlock;
				bool EmbedMetadata;
			};

			bool UseLongArchiveFormat;
			bool EmbedMetadata;
			Array<FileInfo> Files;

			ArchiveList(void);
			ArchiveList(Streaming::Stream * source);
			~ArchiveList(void) override;
		};
	}
}