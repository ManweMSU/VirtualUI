#ifdef ENGINE_WINDOWS
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#include <Windows.h>
#include <CommCtrl.h>
#include <objbase.h>
#ifdef ENGINE_SUBSYSTEM_CONSOLE
int Main(void);
int wmain(void)
{
	InitCommonControls();
	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	int result = Main();
	return result;
}
#endif
#ifdef ENGINE_SUBSYSTEM_GUI
int Main(void);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	InitCommonControls();
	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	int result = Main();
	return result;
}
#endif
#ifdef ENGINE_SUBSYSTEM_SILENT
int Main(void);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	InitCommonControls();
	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	int result = Main();
	return result;
}
#endif
#ifdef ENGINE_SUBSYSTEM_LIBRARY
void LibraryLoaded(void);
void LibraryUnloaded(void);
BOOL APIENTRY DllMain(HMODULE hModule, DWORD Reason, LPVOID Reserved)
{
	if (Reason == DLL_PROCESS_ATTACH) LibraryLoaded();
	else if (Reason == DLL_PROCESS_DETACH) LibraryUnloaded();
	return TRUE;
}
#endif
#endif
#ifdef ENGINE_MACOSX
#ifdef ENGINE_SUBSYSTEM_CONSOLE
int Main(void);
int main(void) { return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_SILENT
int Main(void);
int main(void) { return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_GUI
int Main(void);
int main(void) { return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_LIBRARY
void LibraryLoaded(void);
void LibraryUnloaded(void);
__attribute__((constructor)) static void _lib_init(void) { LibraryLoaded(); }
__attribute__((destructor)) static void _lib_uninit(void) { LibraryUnloaded(); }
#endif
#endif