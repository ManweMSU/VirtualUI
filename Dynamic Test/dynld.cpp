#include <EngineRuntime.h>

typedef int (* func_GetInt) (int);

#ifdef ENGINE_WINDOWS
#define LIB_NAME L"dynlib.dll"
#endif
#ifdef ENGINE_MACOSX
#define LIB_NAME L"dynlib.dylib"
#endif

int Main(void)
{
	Engine::IO::Console console;
	console.WriteLine(L"Hello!");
	auto lib = Engine::LoadLibrary(LIB_NAME);
	if (lib) {
		func_GetInt GetInt = reinterpret_cast<func_GetInt>(Engine::GetLibraryRoutine(lib, "GetInt"));
		if (GetInt) {
			console.WriteLine(GetInt(666));
		}
		Engine::ReleaseLibrary(lib);
	}
	return 0;
}