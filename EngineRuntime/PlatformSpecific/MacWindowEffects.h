#pragma once

#include "../EngineBase.h"
#include "../UserInterface/ControlBase.h"

namespace Engine
{
    namespace MacOSXSpecific
    {
        namespace CreationAttribute
        {
            enum CreationAttributeFlags {
                Normal              = 0x0000,
                TransparentTitle    = 0x0001,
                EffectBackground    = 0x0002,
                Transparent         = 0x0004,
                LightTheme          = 0x0008,
                DarkTheme           = 0x0010,
            };
        }
        void SetWindowCreationAttribute(int attribute);
        int GetWindowCreationAttribute(void);
        void SetWindowBackgroundColor(UI::Window * window, UI::Color color);
        void SetWindowTransparentcy(UI::Window * window, double value);
    }
}