#include "DynamicLibrary.h"

#include "../PlatformDependent/FileApi.h"

#include <dlfcn.h>

namespace Engine
{
	handle LoadLibrary(const string & path)
	{
		SafePointer< Array<uint8> > enc = IO::ExpandPath(path).EncodeSequence(Encoding::UTF8, true);
		return dlopen(reinterpret_cast<char *>(enc->GetBuffer()), RTLD_NOW);
	}
	void ReleaseLibrary(handle library) { dlclose(library); }
	void * GetLibraryRoutine(handle library, const char * routine_name) { return dlsym(library, routine_name); }
}