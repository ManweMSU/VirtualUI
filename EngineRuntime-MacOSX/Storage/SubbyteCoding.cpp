#include "SubbyteCoding.h"

#include "../Miscellaneous/DynamicString.h"

namespace Engine
{
	namespace Storage
	{
		Code::Code(void) : Code(uint32(0), 0) {}
		Code::Code(uint32 value, int32 length)
		{
			int byte_length = (length + 7) / 8;
			data = reinterpret_cast<uint8 *>(malloc(byte_length));
			if (!data) throw OutOfMemoryException();
			bit_length = length;
			MemoryCopy(data, &value, byte_length);
		}
		Code::Code(const void * source, int32 length)
		{
			int byte_length = (length + 7) / 8;
			data = reinterpret_cast<uint8 *>(malloc(byte_length));
			if (!data) throw OutOfMemoryException();
			bit_length = length;
			MemoryCopy(data, source, byte_length);
		}
		Code::Code(const Code & code) : Code(code.data, code.bit_length) {}
		Code::Code(Code && code) { data = code.data; bit_length = code.bit_length; code.data = 0; }
		Code::~Code(void) { free(data); }
		Code & Code::operator=(const Code & code)
		{
			if (this == &code) return *this;
			int byte_length = (code.bit_length + 7) / 8;
			auto new_data = reinterpret_cast<uint8 *>(malloc(byte_length));
			if (!new_data) throw OutOfMemoryException();
			free(data);
			data = new_data;
			MemoryCopy(data, code.data, byte_length);
			bit_length = code.bit_length;
			return *this;
		}
		Code & Code::operator=(Code && code) { free(data); data = code.data; bit_length = code.bit_length; code.data = 0; return *this; }
		bool Code::operator[](int at) const { return GetBit(at); }
		bool Code::GetBit(int at) const { int byte = at / 8, bit = at % 8; return (data[byte] & (1 << bit)) != 0; }
		void Code::SetBit(int at, bool value) { int byte_affects = at / 8, bit_affects = at % 8; data[byte_affects] &= 0xFF ^ (1 << bit_affects); if (value) data[byte_affects] |= 1 << bit_affects; }
		uint8 Code::GetByte(int at) const { return data[at]; }
		int Code::Length(void) const { return bit_length; }
		const void * Code::GetBuffer(void) const { return data; }
		void Code::Append(bool bit)
		{
			int byte_count = (bit_length + 7) / 8;
			int new_byte_count = (bit_length + 8) / 8;
			if (byte_count != new_byte_count) {
				auto new_data = reinterpret_cast<uint8 *>(realloc(data, new_byte_count));
				if (!new_data) throw OutOfMemoryException();
				data = new_data;
				data[byte_count] = 0;
			}
			if (bit) {
				int byte_affects = bit_length / 8;
				int bit_affects = bit_length % 8;
				data[byte_affects] |= 1 << bit_affects;
			}
			bit_length++;
		}
		void Code::Append(const Code & code) { for (int i = 0; i < code.bit_length; i++) Append(code[i]); }
		Code Code::NumericIncrement(void)
		{
			Code result(*this);
			for (int i = 0; i < bit_length; i++) {
				if (result[i]) result.SetBit(i, 0);
				else { result.SetBit(i, 1); break; }
			}
			return result;
		}
		string Code::ToString(void) const
		{
			DynamicString result;
			for (int i = 0; i < bit_length; i++) result += GetBit(i) ? L"1" : L"0";
			return result;
		}
		bool operator==(const Code & a, const Code & b) { if (a.bit_length != b.bit_length) return false; return MemoryCompare(a.data, b.data, (a.bit_length + 7) / 8) == 0; }
		bool operator!=(const Code & a, const Code & b) { return !(a == b); }
		BitStream::BitStream(void) : data(new Array<uint8>(0x10000)), bit_length(0), pointer(0) {}
		BitStream::BitStream(Array<uint8>* source) : bit_length(uint64(source->Length()) * 8), pointer(0) { data.SetRetain(source); }
		BitStream::~BitStream(void) {}
		void BitStream::Require(uint64 bit_amount)
		{
			if (bit_amount > uint64(data->Length()) * 8) {
				int length = int((bit_amount + 7) / 8);
				int old_length = data->Length();
				data->SetLength(length);
				for (int i = old_length; i < length; i++) data->ElementAt(i) = 0;
			}
		}
		uint64 BitStream::Length(void) const { return bit_length; }
		uint64 BitStream::Pointer(void) const { return pointer; }
		void BitStream::Write(bool bit)
		{
			int at_byte = int(bit_length / 8);
			int at_bit = bit_length % 8;
			data->ElementAt(at_byte) &= 0xFF ^ (1 << at_bit);
			if (bit) data->ElementAt(at_byte) |= 1 << at_bit;
			bit_length++;
			pointer++;
		}
		void BitStream::Write(const void * bits, uint32 length)
		{
			Require(bit_length + length);
			for (uint32 i = 0; i < length; i++) {
				int byte = i / 8, bit = i % 8;
				Write((reinterpret_cast<const uint8*>(bits)[byte] & (1 << bit)) != 0);
			}
		}
		void BitStream::Write(const Code & code) { Write(code.GetBuffer(), code.Length()); }
		bool BitStream::ReadBit(void)
		{
			int byte = int(pointer / 8), bit = int(pointer % 8);
			bool result = (data->ElementAt(byte) & (1 << bit)) != 0;
			pointer++;
			return result;
		}
		uint8 BitStream::ReadByte(void)
		{
			uint8 result = reinterpret_cast<uint8*>(data->GetBuffer())[int(pointer / 8)];
			pointer += 8;
			return result;
		}
		uint16 BitStream::ReadWord(void)
		{
			uint16 result = *reinterpret_cast<uint16*>(reinterpret_cast<uint8*>(data->GetBuffer()) + int(pointer / 8));
			pointer += 16;
			return result;
		}
		uint32 BitStream::ReadDWord(void)
		{
			uint32 result = *reinterpret_cast<uint32*>(reinterpret_cast<uint8*>(data->GetBuffer()) + int(pointer / 8));
			pointer += 32;
			return result;
		}
		uint64 BitStream::ReadQWord(void)
		{
			uint64 result = *reinterpret_cast<uint64*>(reinterpret_cast<uint8*>(data->GetBuffer()) + int(pointer / 8));
			pointer += 64;
			return result;
		}
		Code BitStream::ReadCode(uint32 length) { Code result; ReadCode(result, length); return result; }
		void BitStream::ReadCode(Code & buffer, uint32 length) { for (uint32 i = 0; i < length; i++) buffer.Append(ReadBit()); }
		Array<uint8>* BitStream::GetStorage(void) { return data; }
		Array<uint8>* BitStream::GetRetainedStorage(void) { data->Retain(); return data; }
	}
}