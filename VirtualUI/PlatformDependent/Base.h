#pragma once

namespace Engine
{
	enum class Encoding { ANSI = 0, UTF8 = 1, UTF16 = 2, UTF32 = 3 };
	typedef unsigned int uint;
	typedef signed int int32;
	typedef unsigned int uint32;
	typedef signed long long int64;
	typedef unsigned long long uint64;
	typedef signed short int16;
	typedef unsigned short uint16;
	typedef signed char int8;
	typedef unsigned char uint8;

	// OS macros may be changed - WINDOWS and _WIN64 are not standart
#ifdef WINDOWS
	constexpr Encoding SystemEncoding = Encoding::UTF16;

	// First macro MUST turn off any field alignment in structures, second one should restore defaults
#define DATA_ALIGN_UNALIGNED \
#pragma pack(push)\
#pragma pack(1)
#define DATA_ALIGN_RESTORE #pragma pack(pop)

#ifdef _WIN64
	typedef uint64 intptr;
#else
	typedef uint32 intptr;
#endif
#endif
	typedef intptr eint;
	typedef wchar_t widechar;
	typedef void * handle;

	// Atomic increment and decrement; memory initialization
	uint InterlockedIncrement(uint & Value);
	uint InterlockedDecrement(uint & Value);
	void ZeroMemory(void * Memory, intptr Size);

	// Some C standart library and language dependent case insensitive comparation
	void * MemoryCopy(void * Dest, const void * Source, intptr Length);
	widechar * StringCopy(widechar * Dest, const widechar * Source);
	int StringCompare(const widechar * A, const widechar * B);
	int SequenceCompare(const widechar * A, const widechar * B, int Length);
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B);
	int StringLength(const widechar * str);
	void StringAppend(widechar * str, widechar letter);

	// Case converters for fixed-length strings - should work with any language chars
	void StringLower(widechar * str, int length);
	void StringUpper(widechar * str, int length);

	// Encoding converters - OS and language independent - why are they there? I don't know.
	int MeasureSequenceLength(const void * Source, int SourceLength, Encoding From, Encoding To);
	void ConvertEncoding(void * Dest, const void * Source, int SourceLength, Encoding From, Encoding To);
	int GetBytesPerChar(Encoding encoding);
}