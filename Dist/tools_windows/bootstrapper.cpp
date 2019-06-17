int Main(void);
#ifdef ENGINE_WINDOWS
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")
#include <Windows.h>
#include <CommCtrl.h>
#include <objbase.h>
#ifdef ENGINE_SUBSYSTEM_CONSOLE
int wmain(void)
{
	InitCommonControls();
	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	int result = Main();
	return result;
}
#endif
#ifdef ENGINE_SUBSYSTEM_GUI
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	InitCommonControls();
	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	int result = Main();
	return result;
}
#endif
#endif
#ifdef ENGINE_MACOSX
#ifdef ENGINE_SUBSYSTEM_CONSOLE
int main(void) { return Main(); }
#endif
#ifdef ENGINE_SUBSYSTEM_GUI
int main(void) { return Main(); }
#endif
#endif