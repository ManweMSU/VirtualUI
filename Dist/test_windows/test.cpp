#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Application;
using namespace Engine::UI;

class Callback : public IApplicationCallback {};

int Main(void)
{
    Callback callback;
    CreateController(&callback);
#ifdef ENGINE_X64
    GetController()->SystemMessageBox(0, L"Hello, 64-bit Engine Runtime!", ENGINE_VI_APPNAME, 0, MessageBoxButtonSet::Ok, MessageBoxStyle::Information, 0);
#else
    GetController()->SystemMessageBox(0, L"Hello, 32-bit Engine Runtime!", ENGINE_VI_APPNAME, 0, MessageBoxButtonSet::Ok, MessageBoxStyle::Information, 0);
#endif
    return 0;
}