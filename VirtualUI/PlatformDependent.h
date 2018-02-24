#pragma once

namespace Engine
{
	enum class Encoding { ANSI = 0, UTF8 = 1, UTF16 = 2, UTF32 = 3 };
	typedef unsigned int uint;
	typedef signed long long int64;
	typedef unsigned long long uint64;
	typedef signed short int16;
	typedef unsigned short uint16;
	typedef signed char int8;
	typedef unsigned char uint8;
#ifdef WINDOWS
	constexpr Encoding SystemEncoding = Encoding::UTF16;
#ifdef _WIN64
	typedef uint64 intptr;
#else
	typedef uint intptr;
#endif
#endif
	typedef intptr eint;
	typedef wchar_t widechar;

	uint InterlockedIncrement(uint & Value);
	uint InterlockedDecrement(uint & Value);
	void ZeroMemory(void * Memory, intptr Size);

	void * MemoryCopy(void * Dest, const void * Source, intptr Length);
	widechar * StringCopy(widechar * Dest, const widechar * Source);
	int StringCompare(const widechar * A, const widechar * B);
	int SequenceCompare(const widechar * A, const widechar * B, int Length);
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B);
	int StringLength(const widechar * str);
	void StringAppend(widechar * str, widechar letter);

	void StringLower(widechar * str, int length);
	void StringUpper(widechar * str, int length);

	int MeasureSequenceLength(const void * Source, int SourceLength, Encoding From, Encoding To);
	void ConvertEncoding(void * Dest, const void * Source, int SourceLength, Encoding From, Encoding To);
	int GetBytesPerChar(Encoding encoding);
}