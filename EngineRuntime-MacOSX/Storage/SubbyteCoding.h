#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Storage
	{
		class Code
		{
		private:
			uint8 * data;
			int32 bit_length;
		public:
			Code(void);
			Code(uint32 value, int32 length);
			Code(const void * source, int32 length);
			Code(const Code & code);
			Code(Code && code);
			~Code(void);

			Code & operator = (const Code & code);
			Code & operator = (Code && code);
			bool friend operator == (const Code & a, const Code & b);
			bool friend operator != (const Code & a, const Code & b);
			bool operator [] (int at) const;

			bool GetBit(int at) const;
			void SetBit(int at, bool value);
			uint8 GetByte(int at) const;
			int Length(void) const;
			const void * GetBuffer(void) const;
			void Append(bool bit);
			void Append(const Code & code);
			Code NumericIncrement(void);
			string ToString(void) const;
		};
		class BitStream : public Object
		{
		private:
			SafePointer< Array<uint8> > data;
			uint64 bit_length;
			uint64 pointer;
		public:
			BitStream(void);
			BitStream(Array<uint8> * source);
			~BitStream(void);

			void Require(uint64 bit_amount);
			uint64 Length(void) const;
			uint64 Pointer(void) const;
			void Write(bool bit);
			void Write(const void * bits, uint32 length);
			void Write(const Code & code);
			bool ReadBit(void);
			uint8 ReadByte(void);
			uint16 ReadWord(void);
			uint32 ReadDWord(void);
			uint64 ReadQWord(void);
			Code ReadCode(uint32 length);
			void ReadCode(Code & buffer, uint32 length);

			Array<uint8> * GetStorage(void);
			Array<uint8> * GetRetainedStorage(void);
		};
	}
}