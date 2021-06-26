#include "../Interfaces/Base.h"

#include <wchar.h>
#include <wctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <uninorm.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

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
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B) { return wcscasecmp(A, B); }
	int StringLength(const widechar * str) { int l = 0; while (str[l]) l++; return l; }
	void UnicodeNormalize(const widechar * source, widechar ** dest, NormalizeForm form)
	{
		uninorm_t f;
		if (form == NormalizeForm::C) f = UNINORM_NFC;
		else if (form == NormalizeForm::D) f = UNINORM_NFD;
		else if (form == NormalizeForm::KC) f = UNINORM_NFKC;
		else f = UNINORM_NFKD;
		size_t size;
		auto result = u32_normalize(f, reinterpret_cast<const uint32_t *>(source), StringLength(source), 0, &size);
		if (result) {
			widechar * buffer = reinterpret_cast<widechar *>(malloc(sizeof(widechar) * (size + 1)));
			if (buffer) {
				MemoryCopy(buffer, result, sizeof(widechar) * size);
				buffer[size] = 0;
				*dest = buffer;
			} else *dest = 0;
			free(result);
		} else *dest = 0;
	}
	void StringAppend(widechar * str, widechar letter) { auto len = StringLength(str); str[len + 1] = 0; str[len] = letter; }
	void StringLower(widechar * str, int length) { for (int i = 0; i < length; i++) str[i] = towlower(str[i]); }
	void StringUpper(widechar * str, int length) { for (int i = 0; i < length; i++) str[i] = towupper(str[i]); }
	bool IsAlphabetical(uint32 letter) { return iswalpha(letter); }
	void InternalGetProcessorInfo(widechar * name, uint32 * cores_num, uint32 * phys_cores_num, uint64 * freq)
	{
		if (cores_num) *cores_num = get_nprocs();
		if (name || phys_cores_num) {
			int file = open("/proc/cpuinfo", O_RDONLY, 0666);
			if (file != -1) {
				bool nset = false;
				int * proc_known = reinterpret_cast<int *>(malloc(sizeof(int) * 0x100));
				char * buffer = reinterpret_cast<char *>(malloc(0x1000));
				if (proc_known && buffer) {
					for (int i = 0; i < 0x100; i++) proc_known[i] = -1;
					buffer[0] = 0;
					while (true) {
						char r;
						int status = read(file, &r, 1);
						if (status < 1) break;
						if (r == '\n') {
							char * value = 0;
							for (int i = 0; i < strlen(buffer); i++) if (buffer[i] == ':') {
								for (int j = i; j >= 0; j--) {
									if (buffer[j] != ':' && buffer[j] != ' ' && buffer[j] != '\t') break;
									buffer[j] = 0;
								}
								if (buffer[i + 1]) {
									value = buffer + i + 2;
								}
								break;
							}
							if (value) {
								if (strcmp(buffer, "model name") == 0 && !nset) {
									if (strlen(value) > 0x7F) value[0x7F] = 0;
									if (name) ConvertEncoding(name, value, strlen(value) + 1, Encoding::UTF8, Encoding::UTF32);
									nset = true;
								} else if (strcmp(buffer, "core id") == 0) {
									int cidx = -1;
									sscanf(value, "%i", &cidx);
									for (int i = 0; i < 0x100; i++) {
										if (proc_known[i] == cidx) break;
										if (proc_known[i] == -1) {
											proc_known[i] = cidx;
											break;
										}
									}
								}
							}
							buffer[0] = 0;
						} else {
							int len = strlen(buffer);
							if (len == 0xFFF) break;
							buffer[len] = r;
							buffer[len + 1] = 0;
						}
					}
					uint n_cores = 0;
					for (int i = 0; i < 0x100; i++) if (proc_known[i] != -1) n_cores++;
					if (phys_cores_num) *phys_cores_num = n_cores; 
				}
				free(proc_known);
				free(buffer);
				close(file);
			}
		}
		if (freq) {
			int file = open("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", O_RDONLY, 0666);
			if (file != -1) {
				char * buffer = reinterpret_cast<char *>(malloc(0x100));
				if (buffer) {
					int size = read(file, buffer, 0xFF);
					if (size >= 0) {
						buffer[size] = 0;
						uint freq_khz = 0;
						sscanf(buffer, "%i", &freq_khz);
						*freq = uint64(freq_khz) * 1000UL;
					} else *freq = 0;
					free(buffer);
				} else *freq = 0;
				close(file);
			} else *freq = 0;
		}
	}
	bool IsPlatformAvailable(Platform platform)
	{
		auto sys = GetSystemPlatform();
		if (sys == Platform::X64) {
			if (platform == Platform::X64) return true;
			else if (platform == Platform::X86) return true;
			else return false;
		} else if (sys == Platform::X86) {
			if (platform == Platform::X86) return true;
			else return false;
		} else return false;
	}
	Platform GetSystemPlatform(void)
	{
		struct utsname sn;
		if (uname(&sn) == 0) {
			if (strcmp(sn.machine, "x86_64") == 0) return Platform::X64;
			else if (strcmp(sn.machine, "amd64") == 0) return Platform::X64;
			else if (strcmp(sn.machine, "i386") == 0) return Platform::X86;
			else if (strcmp(sn.machine, "i686") == 0) return Platform::X86;
			else if (strcmp(sn.machine, "i686-AT386") == 0) return Platform::X86;
			else return Platform::Unknown;
		} else return Platform::Unknown;
	}
	int GetProcessorsNumber(void) { uint32 num; InternalGetProcessorInfo(0, &num, 0, 0); return num; }
	uint64 GetInstalledMemory(void) { struct sysinfo si; sysinfo(&si); return si.totalram; }
	bool GetSystemInformation(SystemDesc & desc)
	{
		ZeroMemory(&desc, sizeof(desc));
		InternalGetProcessorInfo(desc.ProcessorName, &desc.VirtualCores, &desc.PhysicalCores, &desc.ClockFrequency);
		desc.Architecture = GetSystemPlatform();
		desc.PhysicalMemory = GetInstalledMemory();
		struct utsname sn;
		uname(&sn);
		desc.SystemVersionMajor = desc.SystemVersionMinor = 0;
		for (int i = 0; i < sizeof(sn.release); i++) if (sn.release[i] == '.') {
			for (int j = i + 1; j < sizeof(sn.release); j++) if (sn.release[j] == '.') { sn.release[j] = 0; break; }
			int major = 0, minor = 0;
			sn.release[i] = 0;
			sscanf(sn.release, "%i", &major);
			sscanf(sn.release + i + 1, "%i", &minor);
			desc.SystemVersionMajor = major;
			desc.SystemVersionMinor = minor;
			break;
		}
		return true;
	}
}