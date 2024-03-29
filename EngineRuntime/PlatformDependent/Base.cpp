#include "../Interfaces/Base.h"

#include <Windows.h>
#include <Shlwapi.h>
#include <Wbemidl.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "wbemuuid.lib")

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
	
	typedef HRESULT (WINAPI * func_IsWow64GuestMachineSupported) (USHORT machine, BOOL * result);
	typedef BOOL (WINAPI * func_IsWow64Process2) (HANDLE process, USHORT * machine_process, USHORT * machine_host);

	HRESULT WINAPI engine_IsWow64GuestMachineSupported(USHORT machine, BOOL * result)
	{
		SYSTEM_INFO info;
		GetNativeSystemInfo(&info);
		if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
			*result = FALSE;
		} else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
			if (machine == IMAGE_FILE_MACHINE_I386) *result = TRUE;
			else *result = FALSE;
		} else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
			*result = FALSE;
		} else *result = FALSE;
		return S_OK;
	}
	BOOL WINAPI engine_IsWow64Process2(HANDLE process, USHORT * machine_process, USHORT * machine_host)
	{
		SYSTEM_INFO info;
		GetNativeSystemInfo(&info);
		if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
			*machine_host = IMAGE_FILE_MACHINE_I386;
			*machine_process = IMAGE_FILE_MACHINE_I386;
		} else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
			*machine_host = IMAGE_FILE_MACHINE_AMD64;
			#ifdef ENGINE_X64
			*machine_process = IMAGE_FILE_MACHINE_AMD64;
			#else
			*machine_process = IMAGE_FILE_MACHINE_I386;
			#endif
		} else if (info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM) {
			*machine_host = IMAGE_FILE_MACHINE_ARM;
			*machine_process = IMAGE_FILE_MACHINE_ARM;
		} else return FALSE;
		return TRUE;
	}

	func_IsWow64GuestMachineSupported var_IsWow64GuestMachineSupported = 0;
	func_IsWow64Process2 var_IsWow64Process2 = 0;

	void WindowsWow64FunctionsInit(void)
	{
		if (!var_IsWow64GuestMachineSupported || !var_IsWow64Process2) {
			HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
			if (kernel32) {
				var_IsWow64GuestMachineSupported = reinterpret_cast<func_IsWow64GuestMachineSupported>(GetProcAddress(kernel32, "IsWow64GuestMachineSupported"));
				var_IsWow64Process2 = reinterpret_cast<func_IsWow64Process2>(GetProcAddress(kernel32, "IsWow64Process2"));
				FreeLibrary(kernel32);
			}
			if (!var_IsWow64GuestMachineSupported) var_IsWow64GuestMachineSupported = engine_IsWow64GuestMachineSupported;
			if (!var_IsWow64Process2) var_IsWow64Process2 = engine_IsWow64Process2;
		}
	}

	bool IsPlatformAvailable(Platform platform)
	{
		WindowsWow64FunctionsInit();
		if (GetSystemPlatform() == platform) return true;
		BOOL result;
		USHORT machine;
		if (platform == Platform::X86) machine = IMAGE_FILE_MACHINE_I386;
		else if (platform == Platform::X64) machine = IMAGE_FILE_MACHINE_AMD64;
		else if (platform == Platform::ARM) machine = IMAGE_FILE_MACHINE_ARM;
		else if (platform == Platform::ARM64) machine = IMAGE_FILE_MACHINE_ARM64;
		else return false;
		if (var_IsWow64GuestMachineSupported(machine, &result) != S_OK) return false;
		return result != 0;
	}
	Platform GetSystemPlatform(void)
	{
		WindowsWow64FunctionsInit();
		USHORT self, host;
		if (!var_IsWow64Process2(GetCurrentProcess(), &self, &host)) return Platform::Unknown;
		if (host == IMAGE_FILE_MACHINE_I386) return Platform::X86;
		else if (host == IMAGE_FILE_MACHINE_AMD64) return Platform::X64;
		else if (host == IMAGE_FILE_MACHINE_ARM) return Platform::ARM;
		else if (host == IMAGE_FILE_MACHINE_ARM64) return Platform::ARM64;
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
	bool GetSystemInformation(SystemDesc & desc)
	{
		ZeroMemory(&desc, sizeof(desc));
		desc.Architecture = GetSystemPlatform();
		desc.PhysicalMemory = GetInstalledMemory();
		OSVERSIONINFOW vi;
		vi.dwOSVersionInfoSize = sizeof(vi);
		if (!GetVersionExW(&vi)) return false;
		desc.SystemVersionMajor = vi.dwMajorVersion;
		desc.SystemVersionMinor = vi.dwMinorVersion;
		IWbemLocator * locator = 0;
		auto status = CoInitializeSecurity(0, -1, 0, 0, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, 0, EOAC_NONE, 0);
		if (status != S_OK && status != RPC_E_TOO_LATE) return false;
		status = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&locator)); //CLSID_WbemAdministrativeLocator
		if (status != S_OK) return false;
		IWbemServices * services = 0;
		status = locator->ConnectServer(L"Root\\CIMV2", 0, 0, 0, WBEM_FLAG_CONNECT_USE_MAX_WAIT, 0, 0, &services);
		locator->Release();
		if (status != S_OK) return false;
		IEnumWbemClassObject * enumerator = 0;
		status = services->ExecQuery(L"WQL", L"SELECT * FROM Win32_Processor", WBEM_FLAG_RETURN_IMMEDIATELY | WBEM_FLAG_FORWARD_ONLY, 0, &enumerator);
		services->Release();
		if (status != S_OK) return false;
		while (true) {
			ULONG num_objects;
			IWbemClassObject * object = 0;
			status = enumerator->Next(WBEM_INFINITE, 1, &object, &num_objects);
			if (status < 0) {
				enumerator->Release();
				return false;
			}
			if (!num_objects) break;
			VARIANT prop;
			if (!desc.ProcessorName[0]) {
				if (object->Get(L"Name", 0, &prop, 0, 0) == S_OK) {
					MemoryCopy(desc.ProcessorName, prop.bstrVal, min(sizeof(desc.ProcessorName) - 2, StringLength(prop.bstrVal) * 2));
					VariantClear(&prop);
				}
			}
			if (!desc.ClockFrequency) {
				if (object->Get(L"MaxClockSpeed", 0, &prop, 0, 0) == S_OK) {
					desc.ClockFrequency = uint64(prop.ulVal) * 1000000;
					VariantClear(&prop);
				}
			}
			if (object->Get(L"NumberOfCores", 0, &prop, 0, 0) == S_OK) {
				desc.PhysicalCores = prop.ulVal;
				VariantClear(&prop);
			}
			if (object->Get(L"NumberOfLogicalProcessors", 0, &prop, 0, 0) == S_OK) {
				desc.VirtualCores = prop.ulVal;
				VariantClear(&prop);
			}
			object->Release();
			if (status == WBEM_S_FALSE) break;
		}
		enumerator->Release();
		return true;
	}
}