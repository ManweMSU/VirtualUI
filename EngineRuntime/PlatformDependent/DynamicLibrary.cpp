#include "../Interfaces/DynamicLibrary.h"

#include <Windows.h>

#undef LoadLibrary

namespace Engine
{
	handle LoadLibrary(const string & path) { return LoadLibraryW(path); }
	void ReleaseLibrary(handle library) { FreeLibrary(reinterpret_cast<HMODULE>(library)); }
	void * GetLibraryRoutine(handle library, const char * routine_name) { return GetProcAddress(reinterpret_cast<HMODULE>(library), routine_name); }
}