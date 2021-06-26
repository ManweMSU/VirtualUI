#pragma once

#include "../EngineBase.h"

#ifdef ENGINE_WINDOWS

#define ENGINE_EXPORT_API extern "C" __declspec(dllexport)
#define ENGINE_LIBRARY_EXTENSION L"dll"

#endif
#ifdef ENGINE_MACOSX

#define ENGINE_EXPORT_API extern "C" __attribute__((visibility("default")))
#define ENGINE_LIBRARY_EXTENSION L"dylib"

#endif
#ifdef ENGINE_LINUX

#define ENGINE_EXPORT_API extern "C" __attribute__((visibility("default")))
#define ENGINE_LIBRARY_EXTENSION L"so"

#endif

namespace Engine
{
	handle LoadLibrary(const string & path);
	void ReleaseLibrary(handle library);
	void * GetLibraryRoutine(handle library, const char * routine_name);
}