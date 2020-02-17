#pragma once

#include "../EngineBase.h"

#define ENGINE_EXPORT_API extern "C" __declspec(dllexport)
#define ENGINE_LIBRARY_EXTENSION L"dylib"

namespace Engine
{
	handle LoadLibrary(const string & path);
	void ReleaseLibrary(handle library);
	void * GetLibraryRoutine(handle library, const char * routine_name);
}