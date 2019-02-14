#include "Base.h"

#include <Windows.h>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")

#undef InterlockedIncrement
#undef InterlockedDecrement
#undef ZeroMemory

namespace Engine
{
	uint InterlockedIncrement(uint & Value) { return _InterlockedIncrement(&Value); }
	uint InterlockedDecrement(uint & Value) { return _InterlockedDecrement(&Value); }
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
		if (platform == Platform::X86) return true;
		if (platform == Platform::X64) {
#ifdef ENGINE_X64
			return true;
#else
			SYSTEM_INFO info;
			GetNativeSystemInfo(&info);
			return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64;
#endif
		}
		return false;
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