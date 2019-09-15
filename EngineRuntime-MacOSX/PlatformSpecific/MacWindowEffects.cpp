#include "MacWindowEffects.h"

#ifdef ENGINE_MACOSX
void __SetEngineWindowBackgroundColor(Engine::UI::Window * window, Engine::UI::Color color);
void __SetEngineWindowAlpha(Engine::UI::Window * window, double value);
void __SetEngineWindowEffectBackgroundMaterial(Engine::UI::Window * window, long material);
namespace Engine
{
    namespace MacOSXSpecific
    {
        int WndAttribute;
        void SetWindowCreationAttribute(int attribute) { WndAttribute = attribute; }
        int GetWindowCreationAttribute(void) { return WndAttribute; }
        void SetWindowBackgroundColor(UI::Window * window, UI::Color color) { __SetEngineWindowBackgroundColor(window, color); }
        void SetWindowTransparentcy(UI::Window * window, double value) { __SetEngineWindowAlpha(window, value); }
        void SetEffectBackgroundMaterial(UI::Window * window, EffectBackgroundMaterial material)
        {
            if (material == EffectBackgroundMaterial::Titlebar) __SetEngineWindowEffectBackgroundMaterial(window, 3);
            else if (material == EffectBackgroundMaterial::Selection) __SetEngineWindowEffectBackgroundMaterial(window, 4);
            else if (material == EffectBackgroundMaterial::Menu) __SetEngineWindowEffectBackgroundMaterial(window, 5);
            else if (material == EffectBackgroundMaterial::Popover) __SetEngineWindowEffectBackgroundMaterial(window, 6);
            else if (material == EffectBackgroundMaterial::Sidebar) __SetEngineWindowEffectBackgroundMaterial(window, 7);
            else if (material == EffectBackgroundMaterial::HeaderView) __SetEngineWindowEffectBackgroundMaterial(window, 10);
            else if (material == EffectBackgroundMaterial::Sheet) __SetEngineWindowEffectBackgroundMaterial(window, 11);
            else if (material == EffectBackgroundMaterial::WindowBackground) __SetEngineWindowEffectBackgroundMaterial(window, 12);
            else if (material == EffectBackgroundMaterial::HUD) __SetEngineWindowEffectBackgroundMaterial(window, 13);
            else if (material == EffectBackgroundMaterial::FullScreenUI) __SetEngineWindowEffectBackgroundMaterial(window, 15);
            else if (material == EffectBackgroundMaterial::ToolTip) __SetEngineWindowEffectBackgroundMaterial(window, 17);
        }
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
        void SetEffectBackgroundMaterial(UI::Window * window, EffectBackgroundMaterial material) {}
    }
}
#endif