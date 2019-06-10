#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Application;
using namespace Engine::UI;

class Callback : public IApplicationCallback {};

int Main(void)
{
    Callback callback;
    CreateController(&callback);
    GetController()->SystemMessageBox(0, L"Hello, Engine Runtime!", ENGINE_VI_APPNAME, 0, MessageBoxButtonSet::Ok, MessageBoxStyle::Information, 0);
    return 0;
}