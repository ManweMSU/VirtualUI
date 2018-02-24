#pragma once

namespace Engine
{
	typedef unsigned int uint;
	typedef signed long long int64;
	typedef unsigned long long uint64;
	typedef signed short int16;
	typedef unsigned short uint16;
	typedef signed char int8;
	typedef unsigned char uint8;
#ifdef WIN32
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
}