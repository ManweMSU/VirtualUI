#include "MacWindowEffects.h"

#ifdef ENGINE_MACOSX
void __SetEngineWindowBackgroundColor(Engine::UI::Window * window, Engine::UI::Color color);
void __SetEngineWindowAlpha(Engine::UI::Window * window, double value);
namespace Engine
{
    namespace MacOSXSpecific
    {
        int WndAttribute;
        void SetWindowCreationAttribute(int attribute) { WndAttribute = attribute; }
        int GetWindowCreationAttribute(void) { return WndAttribute; }
        void SetWindowBackgroundColor(UI::Window * window, UI::Color color) { __SetEngineWindowBackgroundColor(window, color); }
        void SetWindowTransparentcy(UI::Window * window, double value) { __SetEngineWindowAlpha(window, value); }
    }
}
#else
namespace Engine
{
    namespace MacOSXSpecific
    {
        void SetWindowCreationAttribute(int attribute) {}
        int GetWindowCreationAttribute(void) { return 0; }
        void SetWindowBackgroundColor(UI::Window * window, UI::Color color) {}
        void SetWindowTransparentcy(UI::Window * window, double value) {}
    }
}
#endif