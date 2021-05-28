#pragma once

#include "StringConvert.h"

#include <stdlib.h>
#include <math.h>

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

	typedef wchar_t widechar;

	enum class Platform { Unknown, X86, X64, ARM, ARM64 };
	enum class NormalizeForm { C, D, KC, KD };

#define ENGINE_PI 3.14159265358979323846

#ifdef ENGINE_WINDOWS

#define ENGINE_PACKED_STRUCTURE(NAME) __pragma(pack(push, 1)) struct NAME {
#define ENGINE_END_PACKED_STRUCTURE }; __pragma(pack(pop))

	constexpr Encoding SystemEncoding = Encoding::UTF16;
	constexpr const widechar * OperatingSystemName = L"Windows";
#endif
#ifdef ENGINE_UNIX

#define ENGINE_PACKED_STRUCTURE(NAME) struct NAME {
#define ENGINE_END_PACKED_STRUCTURE } __attribute__((packed));

	constexpr Encoding SystemEncoding = Encoding::UTF32;
#ifdef ENGINE_MACOSX
	constexpr const widechar * OperatingSystemName = L"Mac OS";
#else
	constexpr const widechar * OperatingSystemName = L"Unix";
#endif
#endif

#ifdef ENGINE_ARM
#ifdef ENGINE_X64
	typedef uint64 intptr;
	constexpr Platform ApplicationPlatform = Platform::ARM64;
#else
	typedef uint32 intptr;
	constexpr Platform ApplicationPlatform = Platform::ARM;
#endif
#else
#ifdef ENGINE_X64
	typedef uint64 intptr;
	constexpr Platform ApplicationPlatform = Platform::X64;
#else
	typedef uint32 intptr;
	constexpr Platform ApplicationPlatform = Platform::X86;
#endif
#endif

	typedef intptr eint;
	typedef void * handle;

	// Atomic increment and decrement; memory initialization
	uint InterlockedIncrement(uint & Value);
	uint InterlockedDecrement(uint & Value);
	void ZeroMemory(void * Memory, intptr Size);

	// System timer's value in milliseconds. The beginning of this time axis is not important
	uint32 GetTimerValue(void);
	// OS Native Time functions (both in OS dependent currency: Windows or Unix)
	uint64 GetNativeTime(void);
	uint64 TimeUniversalToLocal(uint64 time);
	uint64 TimeLocalToUniversal(uint64 time);

	// Some C standard library and language dependent case insensitive comparation
	void * MemoryCopy(void * Dest, const void * Source, intptr Length);
	widechar * StringCopy(widechar * Dest, const widechar * Source);
	int StringCompare(const widechar * A, const widechar * B);
	int SequenceCompare(const widechar * A, const widechar * B, int Length);
	int MemoryCompare(const void * A, const void * B, intptr Length);
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B);
	int StringLength(const widechar * str);
	void UnicodeNormalize(const widechar * source, widechar ** dest, NormalizeForm form = NormalizeForm::C);
	void StringAppend(widechar * str, widechar letter);

	// Case converters for fixed-length strings - should work with any language chars
	void StringLower(widechar * str, int length);
	void StringUpper(widechar * str, int length);
	bool IsAlphabetical(uint32 letter);

	// Query system information
	bool IsPlatformAvailable(Platform platform);
	Platform GetSystemPlatform(void);
	int GetProcessorsNumber(void);
	uint64 GetInstalledMemory(void);
}