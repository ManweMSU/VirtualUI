#pragma once

#include "StringConvert.h"

namespace Engine
{
	typedef unsigned int uint;
	typedef signed int int32;
	typedef unsigned int uint32;
	typedef signed long long int64;
	typedef unsigned long long uint64;
	typedef signed short int16;
	typedef unsigned short uint16;
	typedef signed char int8;
	typedef unsigned char uint8;

#define ENGINE_PACKED_STRUCTURE __pragma(pack(push, 1))
#define ENGINE_END_PACKED_STRUCTURE __pragma(pack(pop))
#define ENGINE_PI 3.14159265358979323846

#ifdef ENGINE_WINDOWS
	constexpr Encoding SystemEncoding = Encoding::UTF16;
#endif
#ifdef ENGINE_UNIX
	constexpr Encoding SystemEncoding = Encoding::UTF32;
#endif

#ifdef ENGINE_X64
	typedef uint64 intptr;
#else
	typedef uint32 intptr;
#endif

	typedef intptr eint;
	typedef wchar_t widechar;
	typedef void * handle;

	// Atomic increment and decrement; memory initialization
	uint InterlockedIncrement(uint & Value);
	uint InterlockedDecrement(uint & Value);
	void ZeroMemory(void * Memory, intptr Size);

	// System timer's value in milliseconds. The beginning of this time axis is not important
	uint32 GetTimerValue(void);

	// Some C standart library and language dependent case insensitive comparation
	void * MemoryCopy(void * Dest, const void * Source, intptr Length);
	widechar * StringCopy(widechar * Dest, const widechar * Source);
	int StringCompare(const widechar * A, const widechar * B);
	int SequenceCompare(const widechar * A, const widechar * B, int Length);
	int MemoryCompare(const void * A, const void * B, intptr Length);
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B);
	int StringLength(const widechar * str);
	void StringAppend(widechar * str, widechar letter);

	// Case converters for fixed-length strings - should work with any language chars
	void StringLower(widechar * str, int length);
	void StringUpper(widechar * str, int length);
	bool IsAlphabetical(uint32 letter);
}