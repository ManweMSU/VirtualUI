#include "PlatformDependent.h"

#include <Windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#undef InterlockedIncrement
#undef InterlockedDecrement
#undef ZeroMemory

namespace Engine
{
	uint InterlockedIncrement(uint & Value) { return _InterlockedIncrement(&Value); }
	uint InterlockedDecrement(uint & Value) { return _InterlockedDecrement(&Value); }
	void ZeroMemory(void * Memory, intptr Size) { memset(Memory, 0, Size); }
	void * MemoryCopy(void * Dest, const void * Source, intptr Length) { return memcpy(Dest, Source, Length); }
	widechar * StringCopy(widechar * Dest, const widechar * Source)
	{
		int i = -1;
		do { i++; Dest[i] = Source[i]; } while (Source[i]);
		return Dest;
	}
	int StringCompare(const widechar * A, const widechar * B)
	{
		int i = 0;
		while (A[i] == B[i] && A[i]) i++;
		if (A[i] == 0 && B[i] == 0) return 0;
		if (A[i] < B[i]) return -1;
		return 1;
	}
	int SequenceCompare(const widechar * A, const widechar * B, int Length)
	{
		int i = 0;
		while (A[i] == B[i] && i < Length) i++;
		if (i == Length) return 0;
		if (A[i] < B[i]) return -1;
		return 1;
	}
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B) { return StrCmpIW(A, B); }
	int StringLength(const widechar * str) { int l = 0; while (str[l]) l++; return l; }
	void StringAppend(widechar * str, widechar letter) { auto len = StringLength(str); str[len + 1] = 0; str[len] = letter; }

	void StringLower(widechar * str, int length)
	{
		for (int i = 0; i < length; i++) str[i] = reinterpret_cast<widechar>(CharLowerW(reinterpret_cast<LPWSTR>(str[i])));
	}
	void StringUpper(widechar * str, int length)
	{
		for (int i = 0; i < length; i++) str[i] = reinterpret_cast<widechar>(CharUpperW(reinterpret_cast<LPWSTR>(str[i])));
	}

	struct CharReader
	{
		const uint8 * Source;
		int Position, Length;
		Encoding Coding;
		
		bool EOF(void) const { return Position < Length; }
		uint ReadByte(void)
		{
			uint8 r = *(Source + Position);
			Position++;
			return r;
		}
		uint ReadWord(void)
		{
			uint16 r = *reinterpret_cast<const uint16*>(Source + Position);
			Position += 2;
			return r;
		}
		uint ReadDWord(void)
		{
			uint r = *reinterpret_cast<const uint*>(Source + Position);
			Position += 4;
			return r;
		}
		uint ReadChar(void)
		{
			if (Coding == Encoding::ANSI) return ReadByte();
			else if (Coding == Encoding::UTF8) {
				uint code = ReadByte();
				if (code & 0x80) {
					code &= 0x7F;
					if (code & 0x20) {
						code &= 0x1F;
						if (code & 0x10) {
							code &= 0x0F;
							code |= (ReadByte() & 0x3F) << 3;
							code |= (ReadByte() & 0x3F) << 9;
							code |= (ReadByte() & 0x3F) << 15;
						} else {
							code |= (ReadByte() & 0x3F) << 4;
							code |= (ReadByte() & 0x3F) << 10;
						}
					} else {
						code |= (ReadByte() & 0x3F) << 5;
					}
				}
				return code;
			} else if (Coding == Encoding::UTF16) {
				uint code = ReadWord();
				if (code && 0xFC00 == 0xD800) {
					code &= 0x03FF;
					code |= (ReadWord() & 0x3FF) << 10;
					code += 0x10000;
				}
				return code;
			} else if (Coding == Encoding::UTF32) return ReadDWord();
			return 0;
		}
		static int CharLength(uint chr, Encoding in)
		{
			if (in == Encoding::ANSI) return 1;
			else if (in == Encoding::UTF32) return 1;
			else if (in == Encoding::UTF16) {
				if (chr > 0xFFFF) return 2;
				else return 1;
			} else if (in == Encoding::UTF8) {
				if (chr > 0xFFFF) return 4;
				else if (chr > 0x7FF) return 3;
				else if (chr > 0x7F) return 2;
				else return 1;
			}
			return 1;
		}
		void MeasureLength(void)
		{
			if (Length == -1) {
				int pos = Position;
				uint c;
				do { c = ReadChar(); } while (c);
				Length = Position;
				Position = pos;
			} else {
				if (Coding == Encoding::UTF16) Length *= 2;
				else if (Coding == Encoding::UTF32) Length *= 4;
			}
		}
	};
	struct CharWriter
	{
		uint8 * Dest;
		int Position;
		Encoding Coding;
		void WriteByte(uint8 v)
		{
			*(Dest + Position) = v;
			Position++;
		}
		void WriteWord(uint16 v)
		{
			*reinterpret_cast<uint16*>(Dest + Position) = v;
			Position += 2;
		}
		void WriteDWord(uint v)
		{
			*reinterpret_cast<uint*>(Dest + Position) = v;
			Position += 4;
		}
		void WriteChar(uint chr)
		{
			if (Coding == Encoding::ANSI) WriteByte(chr);
			else if (Coding == Encoding::UTF8) {
				if (chr > 0xFFFF) {
					WriteByte((chr & 0x07) | 0xF0);
					WriteByte(((chr >> 3) & 0x3F) | 0x80);
					WriteByte(((chr >> 9) & 0x3F) | 0x80);
					WriteByte(((chr >> 15) & 0x3F) | 0x80);
				} else if (chr > 0x7FF) {
					WriteByte((chr & 0x0F) | 0xE0);
					WriteByte(((chr >> 4) & 0x3F) | 0x80);
					WriteByte(((chr >> 10) & 0x3F) | 0x80);
				} else if (chr > 0x7F) {
					WriteByte((chr & 0x1F) | 0xC0);
					WriteByte(((chr >> 5) & 0x3F) | 0x80);
				} else WriteByte(chr);
			} else if (Coding == Encoding::UTF16) {
				if (chr > 0xFFFF) {
					uint s = chr - 0x10000;
					WriteWord((s & 0x3FF) | 0xD800);
					WriteWord(((s >> 10) & 0x3FF) | 0xDC00);
				} else WriteWord(chr);
			} else if (Coding == Encoding::UTF32) WriteDWord(chr);
		}
	};
	int MeasureSequenceLength(const void * Source, int SourceLength, Encoding From, Encoding To)
	{
		if (From == To) return SourceLength;
		int len = 0;
		CharReader Reader;
		Reader.Source = reinterpret_cast<const uint8 *>(Source); Reader.Position = 0; Reader.Length = SourceLength; Reader.Coding = From;
		Reader.MeasureLength();
		while (!Reader.EOF()) len += Reader.CharLength(Reader.ReadChar(), To);
		return len;
	}
	void ConvertEncoding(void * Dest, const void * Source, int SourceLength, Encoding From, Encoding To)
	{
		CharReader Reader;
		Reader.Source = reinterpret_cast<const uint8 *>(Source); Reader.Position = 0; Reader.Length = SourceLength; Reader.Coding = From;
		Reader.MeasureLength();
		if (From == To) {
			MemoryCopy(Dest, Source, Reader.Length);
		} else {
			CharWriter Writer;
			Writer.Dest = reinterpret_cast<uint8 *>(Dest); Writer.Position = 0; Writer.Coding = To;
			while (!Reader.EOF()) Writer.WriteChar(Reader.ReadChar());
		}
	}
}