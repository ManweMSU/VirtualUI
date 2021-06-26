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
#include <unistd.h>
#include <signal.h>
#ifdef ENGINE_SUBSYSTEM_CONSOLE
int Main(void);
int main(void) { signal(SIGPIPE, SIG_IGN); return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_SILENT
int Main(void);
int main(void) { signal(SIGPIPE, SIG_IGN); return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_GUI
int Main(void);
int main(void) { signal(SIGPIPE, SIG_IGN); return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_LIBRARY
void LibraryLoaded(void);
void LibraryUnloaded(void);
__attribute__((constructor)) static void _lib_init(void) { LibraryLoaded(); }
__attribute__((destructor)) static void _lib_uninit(void) { LibraryUnloaded(); }
#endif
#endif
#ifdef ENGINE_LINUX
#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <PlatformDependent/ThreadingAPI.h>
#include <PlatformDependent/ProcessAPI.h>
#if defined(ENGINE_SUBSYSTEM_CONSOLE) || defined(ENGINE_SUBSYSTEM_SILENT) || defined(ENGINE_SUBSYSTEM_GUI)
int Main(void);
int main(int argc, char ** argv)
{
	Engine::ThisProcessID = getpid();
	Engine::ThisProcessArgV = argv;
	Engine::ThisProcessArgC = argc;
	setlocale(LC_ALL, "");
	signal(SIGPIPE, SIG_IGN);
	if (argc > 3 && strcmp(argv[1], "--ert_elevate") == 0) {
		argv[argc - 1] = 0;
		return Engine::CreateDetachedProcess(argv + 2) ? 0 : 1;
	}
	int result = Main();
	Engine::ProcessWaiterThreadLock();
	return result;
}
#endif
#if defined(ENGINE_SUBSYSTEM_LIBRARY)
void LibraryLoaded(void);
void LibraryUnloaded(void);
__attribute__((constructor)) static void _lib_init(void) { Engine::ThisProcessID = getpid(); LibraryLoaded(); }
__attribute__((destructor)) static void _lib_uninit(void) { LibraryUnloaded(); Engine::ProcessWaiterThreadLock(); }
#endif
#endif