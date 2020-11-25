#include "Base.h"

#include <Windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")

#ifdef InterlockedIncrement
#undef InterlockedIncrement
#define SystemInterlockedIncrement _InterlockedIncrement
#else
#define SystemInterlockedIncrement ::InterlockedIncrement
#endif
#ifdef InterlockedDecrement
#undef InterlockedDecrement
#define SystemInterlockedDecrement _InterlockedDecrement
#else
#define SystemInterlockedDecrement ::InterlockedDecrement
#endif
#undef ZeroMemory

namespace Engine
{
	uint InterlockedIncrement(uint & Value) { return SystemInterlockedIncrement(&Value); }
	uint InterlockedDecrement(uint & Value) { return SystemInterlockedDecrement(&Value); }
	void ZeroMemory(void * Memory, intptr Size) { memset(Memory, 0, Size); }
	uint32 GetTimerValue(void) { return timeGetTime(); }
	uint64 GetNativeTime(void)
	{
		SYSTEMTIME sys_time;
		FILETIME time;
		GetSystemTime(&sys_time);
		SystemTimeToFileTime(&sys_time, &time);
		return time.dwLowDateTime + (uint64(time.dwHighDateTime) << 32);
	}
	uint64 TimeUniversalToLocal(uint64 time)
	{
		uint64 conv;
		FileTimeToLocalFileTime(reinterpret_cast<FILETIME *>(&time), reinterpret_cast<FILETIME *>(&conv));
		return conv;
	}
	uint64 TimeLocalToUniversal(uint64 time)
	{
		uint64 conv;
		LocalFileTimeToFileTime(reinterpret_cast<FILETIME *>(&time), reinterpret_cast<FILETIME *>(&conv));
		return conv;
	}
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
	int MemoryCompare(const void * A, const void * B, intptr Length) { return memcmp(A, B, Length); }
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B) { return StrCmpIW(A, B); }
	int StringLength(const widechar * str) { int l = 0; while (str[l]) l++; return l; }
	void UnicodeNormalize(const widechar * source, widechar ** dest, NormalizeForm form)
	{
		NORM_FORM _form = NormalizationC;
		if (form == NormalizeForm::D) _form = NormalizationD;
		else if (form == NormalizeForm::KC) _form = NormalizationKC;
		else if (form == NormalizeForm::KD) _form = NormalizationKD;
		auto length = NormalizeString(_form, source, -1, 0, 0);
		if (length <= 0 && GetLastError() != ERROR_SUCCESS) { *dest = 0; return; }
		*dest = reinterpret_cast<widechar *>(malloc(sizeof(widechar) * length));
		if (*dest) NormalizeString(_form, source, -1, *dest, length);
	}
	void StringAppend(widechar * str, widechar letter) { auto len = StringLength(str); str[len + 1] = 0; str[len] = letter; }

	void StringLower(widechar * str, int length)
	{
		for (int i = 0; i < length; i++) str[i] = widechar(reinterpret_cast<intptr>(CharLowerW(reinterpret_cast<LPWSTR>(str[i]))));
	}
	void StringUpper(widechar * str, int length)
	{
		for (int i = 0; i < length; i++) str[i] = widechar(reinterpret_cast<intptr>(CharUpperW(reinterpret_cast<LPWSTR>(str[i]))));
	}
	bool IsAlphabetical(uint32 letter)
	{
		return IsCharAlphaW(widechar(letter)) != 0;
	}
	bool IsPlatformAvailable(Platform platform)
	{
		SYSTEM_INFO info;
		GetNativeSystemInfo(&info);
		if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
			if (platform == Platform::X86) return true;
			else return false;
		} else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
			if (platform == Platform::X86 || platform == Platform::X64) return true;
			else return false;
		} else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
			if (platform == Platform::X86 || platform == Platform::ARM) return true;
			else return false;
		} else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) {
			if (platform == Platform::X86 || platform == Platform::ARM || platform == Platform::ARM64) return true;
			else return false;
		} else return false;
	}
	Platform GetSystemPlatform(void)
	{
		SYSTEM_INFO info;
		GetNativeSystemInfo(&info);
		if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) return Platform::X86;
		else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) return Platform::X64;
		else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) return Platform::ARM;
		else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) return Platform::ARM64;
		else return Platform::Unknown;
	}
	int GetProcessorsNumber(void)
	{
		SYSTEM_INFO info;
		GetNativeSystemInfo(&info);
		return int(info.dwNumberOfProcessors);
	}
	uint64 GetInstalledMemory(void)
	{
		uint64 result;
		GetPhysicallyInstalledSystemMemory(&result);
		return result * 0x400;
	}
}