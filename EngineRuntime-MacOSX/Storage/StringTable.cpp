#include "StringTable.h"

namespace Engine
{
	namespace Storage
	{
		ENGINE_PACKED_STRUCTURE(StringTableHeader)
			char Signature[8];	// "ecs.1.0\0"
			uint32 SignatureEx;	// 0x80000007
			uint32 Version;		// 0
			uint32 DataOffset;
			uint32 DataSize;
			uint32 StringCount;
		ENGINE_END_PACKED_STRUCTURE
		ENGINE_PACKED_STRUCTURE(StringTableEntity)
			uint32 ID;
			uint32 Offset;
		ENGINE_END_PACKED_STRUCTURE
		StringTable::StringTable(void) : strings(0x10) {}
		StringTable::StringTable(Streaming::Stream * stream) : strings(0x10)
		{
			StringTableHeader hdr;
			stream->Read(&hdr, sizeof(hdr));
			if (MemoryCompare(hdr.Signature, "ecs.1.0\0", 8) || hdr.SignatureEx != 0x80000007 || hdr.Version) throw InvalidFormatException();
			Array<uint8> buffer(hdr.DataSize);
			Array<StringTableEntity> ehdr(hdr.StringCount);
			buffer.SetLength(hdr.DataSize);
			ehdr.SetLength(hdr.StringCount);
			stream->Read(ehdr.GetBuffer(), ehdr.Length() * sizeof(StringTableEntity));
			stream->Seek(hdr.DataOffset, Streaming::Begin);
			stream->Read(buffer.GetBuffer(), buffer.Length());
			for (int i = 0; i < ehdr.Length(); i++) {
				string text(buffer.GetBuffer() + ehdr[i].Offset, -1, Encoding::UTF16);
				AddString(text, ehdr[i].ID);
			}
		}
		StringTable::~StringTable(void) {}
		const string & StringTable::GetString(int ID) const
		{
			int l = 0, u = strings.Length() - 1;
			while (l <= u) {
				int m = (l + u) / 2;
				if (strings[m].ID < ID) l = m + 1;
				else if (strings[m].ID > ID) u = m - 1;
				else return strings[m].String;
			}
			throw InvalidArgumentException();
		}
		Array<int>* StringTable::GetIndex(void) const
		{
			SafePointer< Array<int> > index = new Array<int>(strings.Length());
			for (int i = 0; i < strings.Length(); i++) *index << strings[i].ID;
			index->Retain();
			return index;
		}
		void StringTable::AddString(const string & text, int ID)
		{
			int at = -1;
			if (strings.Length()) {
				int l = 0, u = strings.Length() - 1;
				if (strings.FirstElement().ID >= ID) {
					if (strings.FirstElement().ID == ID) {
						strings.FirstElement().String = text;
						return;
					} else at = 0;
				} else if (strings.LastElement().ID <= ID) {
					if (strings.LastElement().ID == ID) {
						strings.LastElement().String = text;
						return;
					} else at = strings.Length();
				} else {
					while (u - l > 1) {
						int m = (l + u) / 2;
						if (strings[m].ID < ID) l = m;
						else if (strings[m].ID > ID) u = m;
						else { strings[m].String = text; return; }
					}
					at = u;
				}
			} else at = 0;
			strings.Insert(StringEntity{ ID, text }, at);
		}
		void StringTable::RemoveString(int ID)
		{
			int l = 0, u = strings.Length() - 1;
			while (l <= u) {
				int m = (l + u) / 2;
				if (strings[m].ID < ID) l = m + 1;
				else if (strings[m].ID > ID) u = m - 1;
				else { strings.Remove(m); return; }
			}
		}
		void StringTable::Save(Streaming::Stream * stream) const
		{
			StringTableHeader hdr;
			Array<StringTableEntity> ehdr(strings.Length());
			ehdr.SetLength(strings.Length());
			Array<uint16> buffer(0x1000);
			Array<uint16> encoded(0x100);
			for (int i = 0; i < strings.Length(); i++) {
				ehdr[i].ID = strings[i].ID;
				encoded.SetLength(strings[i].String.GetEncodedLength(Encoding::UTF16) + 1);
				strings[i].String.Encode(encoded.GetBuffer(), Encoding::UTF16, true);
				int insert = -1;
				if (encoded.Length() <= buffer.Length()) {
					for (int j = buffer.Length() - encoded.Length(); j >= 0; j--) {
						bool match = true;
						for (int k = encoded.Length() - 1; k >= 0; k--) {
							if (encoded[k] != buffer[j + k]) {
								match = false;
								break;
							}
						}
						if (match) {
							insert = j;
							break;
						}
					}
				}
				if (insert == -1) {
					ehdr[i].Offset = buffer.Length() << 1;
					buffer << encoded;
				} else ehdr[i].Offset = insert << 1;
			}
			MemoryCopy(hdr.Signature, "ecs.1.0\0", 8);
			hdr.SignatureEx = 0x80000007;
			hdr.Version = 0;
			hdr.DataOffset = sizeof(hdr) + sizeof(StringTableEntity) * ehdr.Length();
			hdr.DataSize = buffer.Length() << 1;
			hdr.StringCount = ehdr.Length();
			stream->Write(&hdr, sizeof(hdr));
			stream->Write(ehdr.GetBuffer(), sizeof(StringTableEntity) * ehdr.Length());
			stream->Write(buffer.GetBuffer(), hdr.DataSize);
		}
	}
}