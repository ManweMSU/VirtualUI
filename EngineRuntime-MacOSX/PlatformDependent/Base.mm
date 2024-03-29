#include "../Interfaces/Base.h"

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
		NSString * a = [[NSString alloc] initWithBytes: A length: StringLength(A) * sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
		NSString * b = [[NSString alloc] initWithBytes: B length: StringLength(B) * sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
		NSComparisonResult result = [a localizedCaseInsensitiveCompare: b];
		[a release];
		[b release];
		if (result == NSOrderedAscending) return -1;
		else if (result == NSOrderedDescending) return 1;
		else return 0;
    }
	int StringLength(const widechar * str) { int l = 0; while (str[l]) l++; return l; }
	void UnicodeNormalize(const widechar * source, widechar ** dest, NormalizeForm form)
	{
		@autoreleasepool {
			NSString * s = [[NSString alloc] initWithBytes: source length: StringLength(source) * 4 encoding: NSUTF32LittleEndianStringEncoding];
			NSString * c = 0;
			if (form == NormalizeForm::D) c = [s decomposedStringWithCanonicalMapping];
			else if (form == NormalizeForm::KC) c = [s precomposedStringWithCompatibilityMapping];
			else if (form == NormalizeForm::KD) c = [s decomposedStringWithCompatibilityMapping];
			else c = [s precomposedStringWithCanonicalMapping];
			[s release];
			int length = [c lengthOfBytesUsingEncoding: NSUTF32LittleEndianStringEncoding];
			*dest = reinterpret_cast<widechar *>(malloc(length + 4));
			if (*dest) {
				[c getBytes: *dest maxLength: length usedLength: NULL encoding: NSUTF32LittleEndianStringEncoding
                	options: 0 range: NSMakeRange(0, [c length]) remainingRange: NULL];
				(*dest)[length / 4] = 0;
			}
		}
	}
	void StringAppend(widechar * str, widechar letter) { auto len = StringLength(str); str[len + 1] = 0; str[len] = letter; }

	void StringLower(widechar * str, int length)
	{
		@autoreleasepool {
			for (int i = 0; i < length; i++) {
				NSString * s = [[NSString alloc] initWithBytes: str + i length: sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
				NSString * c = [s localizedLowercaseString];
				[c getBytes: (str + i) maxLength: sizeof(widechar) usedLength: NULL encoding: NSUTF32LittleEndianStringEncoding
					options: 0 range: NSMakeRange(0, 1) remainingRange: NULL];
				[s release];
			}
		}
	}
	void StringUpper(widechar * str, int length)
	{
		@autoreleasepool {
			for (int i = 0; i < length; i++) {
				NSString * s = [[NSString alloc] initWithBytes: str + i length: sizeof(widechar) encoding: NSUTF32LittleEndianStringEncoding];
				NSString * c = [s localizedUppercaseString];
				[c getBytes: (str + i) maxLength: sizeof(widechar) usedLength: NULL encoding: NSUTF32LittleEndianStringEncoding
					options: 0 range: NSMakeRange(0, 1) remainingRange: NULL];
				[s release];
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
		int32 proc_type = 0;
		int32 proc_emulated = 0;
		uint64 length = sizeof(proc_type);
		if (sysctlbyname("hw.cputype", &proc_type, reinterpret_cast<size_t *>(&length), 0, 0) == -1) proc_type = -1;
		length = sizeof(proc_emulated);
		if (sysctlbyname("sysctl.proc_translated", &proc_emulated, reinterpret_cast<size_t *>(&length), 0, 0) == -1) proc_emulated = -1;
		proc_type &= 0xFF;
		if (proc_type == CPU_TYPE_X86) {
			if (platform == Platform::X64) return true;
			else if (platform == Platform::ARM64 && proc_emulated == 1) return true;
			else return false;
		} else if (proc_type == CPU_TYPE_ARM) {
			if (platform == Platform::ARM64) return true;
			else if (platform == Platform::X64 && proc_emulated == 0) return true;
			else return false;
		} else return false;
	}
	Platform GetSystemPlatform(void)
	{
		int32 proc_type = 0;
		int32 proc_is64 = 0;
		int32 proc_emulated = 0;
		uint64 length = sizeof(proc_type);
		if (sysctlbyname("hw.cputype", &proc_type, reinterpret_cast<size_t *>(&length), 0, 0) == -1) proc_type = -1;
		length = sizeof(proc_is64);
		if (sysctlbyname("hw.cpu64bit_capable", &proc_is64, reinterpret_cast<size_t *>(&length), 0, 0) == -1) proc_is64 = 0;
		length = sizeof(proc_emulated);
		if (sysctlbyname("sysctl.proc_translated", &proc_emulated, reinterpret_cast<size_t *>(&length), 0, 0) == -1) proc_emulated = -1;
		proc_type &= 0xFF;
		if (proc_type == CPU_TYPE_X86) {
			if (proc_is64) {
				if (proc_emulated > 0) return Platform::ARM64;
				else return Platform::X64;
			} else return Platform::X86;
		} else if (proc_type == CPU_TYPE_ARM) {
			if (proc_is64) return Platform::ARM64;
			else return Platform::ARM;
		} else return Platform::Unknown;
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
	bool GetSystemInformation(SystemDesc & desc)
	{
		ZeroMemory(&desc, sizeof(desc));
		desc.Architecture = GetSystemPlatform();
		desc.PhysicalMemory = GetInstalledMemory();
		int32 cores_physical, cores_logical;
		uint64 length = sizeof(int32), frequency;
		if (sysctlbyname("hw.logicalcpu", &cores_logical, reinterpret_cast<size_t *>(&length), 0, 0) != -1) {
			desc.VirtualCores = cores_logical;
		}
		length = sizeof(int32);
		if (sysctlbyname("hw.physicalcpu", &cores_physical, reinterpret_cast<size_t *>(&length), 0, 0) != -1) {
			desc.PhysicalCores = cores_physical;
		}
		length = sizeof(frequency);
		if (sysctlbyname("hw.cpufrequency", &frequency, reinterpret_cast<size_t *>(&length), 0, 0) != -1) {
			desc.ClockFrequency = frequency;
		}
		char cpu_brand[0x80];
		length = sizeof(cpu_brand);
		if (sysctlbyname("machdep.cpu.brand_string", &cpu_brand, reinterpret_cast<size_t *>(&length), 0, 0) != -1) {
			for (int i = 0; i < length; i++) desc.ProcessorName[i] = cpu_brand[i];
		}
		@autoreleasepool {
			auto info = [NSProcessInfo processInfo];
			auto vi = [info operatingSystemVersion];
			desc.SystemVersionMajor = vi.majorVersion;
			desc.SystemVersionMinor = vi.minorVersion;
		}
		return true;
	}
}
