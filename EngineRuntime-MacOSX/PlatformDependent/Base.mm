#include "Base.h"

#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/time.h>

@import Foundation;

namespace Engine
{
    uint InterlockedIncrement(uint & Value) { return __sync_add_and_fetch(&Value, 1); }
	uint InterlockedDecrement(uint & Value) { return __sync_sub_and_fetch(&Value, 1); }
    void ZeroMemory(void * Memory, intptr Size) { memset(Memory, 0, Size); }
    uint32 GetTimerValue(void) { timespec time; clock_gettime(CLOCK_MONOTONIC_RAW, &time); return time.tv_nsec / 1000000 + time.tv_sec * 1000; }
    uint64 GetNativeTime(void)
	{
		struct timeval time;
		gettimeofday(&time, 0);
		return time.tv_sec;
	}
	uint64 TimeUniversalToLocal(uint64 time)
	{
		time_t t = time_t(time);
		return timegm(localtime(&t));
	}
	uint64 TimeLocalToUniversal(uint64 time)
	{
		time_t t = time_t(time);
		return mktime(gmtime(&t));
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
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B)
    {
		@autoreleasepool {
			NSString * a = [[NSString alloc] initWithBytes: A length: StringLength(A) * sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
			NSString * b = [[NSString alloc] initWithBytes: B length: StringLength(B) * sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
			[a autorelease];
			[b autorelease];
			NSComparisonResult result = [a localizedCaseInsensitiveCompare: b];
			if (result == NSOrderedAscending) return -1;
			else if (result == NSOrderedDescending) return 1;
			else return 0;
		}
    }
	int StringLength(const widechar * str) { int l = 0; while (str[l]) l++; return l; }
	void StringAppend(widechar * str, widechar letter) { auto len = StringLength(str); str[len + 1] = 0; str[len] = letter; }

	void StringLower(widechar * str, int length)
	{
		for (int i = 0; i < length; i++) {
			@autoreleasepool {
				NSString * s = [[NSString alloc] initWithBytes: str + i length: sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
				NSString * c = [s lowercaseStringWithLocale: [NSLocale currentLocale]];
				[s autorelease];
				[c autorelease];
				[c getBytes: (str + i) maxLength: sizeof(widechar) usedLength: NULL encoding: NSUTF32LittleEndianStringEncoding
					options: 0 range: NSMakeRange(0, 1) remainingRange: NULL];
			}
		}
	}
	void StringUpper(widechar * str, int length)
	{
		for (int i = 0; i < length; i++) {
			@autoreleasepool {
				NSString * s = [[NSString alloc] initWithBytes: str + i length: sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
				NSString * c = [s uppercaseStringWithLocale: [NSLocale currentLocale]];
				[s autorelease];
				[c autorelease];
				[c getBytes: (str + i) maxLength: sizeof(widechar) usedLength: NULL encoding: NSUTF32LittleEndianStringEncoding
					options: 0 range: NSMakeRange(0, 1) remainingRange: NULL];
			}
		}
	}
	bool IsAlphabetical(uint32 letter)
	{
		if (letter >= 0x10000) return false;
		bool result = false;
		NSCharacterSet * letters = [NSCharacterSet letterCharacterSet];
		if ([letters characterIsMember: unichar(letter)]) result = true;
		return result;
	}
	bool IsPlatformAvailable(Platform platform)
	{
		if (platform == Platform::X86) return false;
		if (platform == Platform::X64) return true;
	}
	int GetProcessorsNumber(void)
	{
		return sysconf(_SC_NPROCESSORS_ONLN);
	}
	uint64 GetInstalledMemory(void)
	{
		int name[2];
		name[0] = CTL_HW;
		name[1] = HW_MEMSIZE;
		uint64 result = 0;
		uint64 length = sizeof(result);
		if (sysctl(name, 2, &result, reinterpret_cast<size_t *>(&length), 0, 0) == 0) return result;
		return 0;
	}
}
