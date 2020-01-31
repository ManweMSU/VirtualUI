#include <EngineRuntime.h>

void LibraryLoaded(void)
{
	Engine::IO::Console console;
	console.WriteLine(L"Library loaded!");
	console.WriteLine(Engine::IO::GetExecutablePath());
}
void LibraryUnloaded(void)
{
	Engine::IO::Console console;
	console.WriteLine(L"Library unloaded.");
}

ENGINE_EXPORT_API int GetInt(int arg) { return arg * 10; }