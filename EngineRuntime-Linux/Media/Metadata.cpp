#include "Metadata.h"

namespace Engine
{
	namespace Media
	{
		bool operator==(const MetadataValue & a, const MetadataValue & b) { return MemoryCompare(&a, &b, sizeof(MetadataValue)) == 0; }
		Metadata * CloneMetadata(const Metadata * metadata)
		{
			SafePointer<Metadata> result = new Metadata(metadata->Length());
			for (auto & meta : metadata->Elements()) {
				MetadataValue value;
				value.Number = meta.value.Number;
				value.Text = meta.value.Text;
				if (meta.value.Picture) {
					value.Picture = new DataBlock(meta.value.Picture->Length());
					value.Picture->Append(*meta.value.Picture);
				}
				result->Append(meta.key, value);
			}
			result->Retain();
			return result;
		}
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
		MetadataValue::MetadataValue(DataBlock * picture, const string & encoder) : Number(0), Text(encoder) { Picture.SetRetain(picture); }
		MetadataValue::operator string(void) const { return Text; }
		MetadataValue::operator uint32(void) const { return Number; }
		MetadataValue::operator DataBlock * (void) const { return Picture; }
	}
}