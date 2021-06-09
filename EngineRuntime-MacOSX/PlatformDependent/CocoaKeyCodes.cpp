#include "CocoaKeyCodes.h"

#include "../Interfaces/KeyCodes.h"

namespace Engine
{
    namespace Cocoa
    {
        uint32 EngineKeyCode(uint32 key, bool & dead)
        {
            dead = false;
            switch (key) {
                case 0x00: return KeyCodes::A;
                case 0x01: return KeyCodes::S;
                case 0x02: return KeyCodes::D;
                case 0x03: return KeyCodes::F;
                case 0x04: return KeyCodes::H;
                case 0x05: return KeyCodes::G;
                case 0x06: return KeyCodes::Z;
                case 0x07: return KeyCodes::X;
                case 0x08: return KeyCodes::C;
                case 0x09: return KeyCodes::V;
                case 0x0A: return KeyCodes::Oem8;
                case 0x0B: return KeyCodes::B;
                case 0x0C: return KeyCodes::Q;
                case 0x0D: return KeyCodes::W;
                case 0x0E: return KeyCodes::E;
                case 0x0F: return KeyCodes::R;

                case 0x10: return KeyCodes::Y;
                case 0x11: return KeyCodes::T;
                case 0x12: return KeyCodes::D1;
                case 0x13: return KeyCodes::D2;
                case 0x14: return KeyCodes::D3;
                case 0x15: return KeyCodes::D4;
                case 0x16: return KeyCodes::D6;
                case 0x17: return KeyCodes::D5;
                case 0x18: return KeyCodes::OemPlus;
                case 0x19: return KeyCodes::D9;
                case 0x1A: return KeyCodes::D7;
                case 0x1B: return KeyCodes::OemMinus;
                case 0x1C: return KeyCodes::D8;
                case 0x1D: return KeyCodes::D0;
                case 0x1E: return KeyCodes::Oem6;
                case 0x1F: return KeyCodes::O;

                case 0x20: return KeyCodes::U;
                case 0x21: return KeyCodes::Oem4;
                case 0x22: return KeyCodes::I;
                case 0x23: return KeyCodes::P;
                case 0x25: return KeyCodes::L;
                case 0x26: return KeyCodes::J;
                case 0x27: return KeyCodes::Oem7;
                case 0x28: return KeyCodes::K;
                case 0x29: return KeyCodes::Oem1;
                case 0x2A: return KeyCodes::Oem5;
                case 0x2B: return KeyCodes::OemComma;
                case 0x2C: return KeyCodes::Oem2;
                case 0x2D: return KeyCodes::N;
                case 0x2E: return KeyCodes::M;
                case 0x2F: return KeyCodes::OemPeriod;

                case 0x32: return KeyCodes::Oem3;
                case 0x41: return KeyCodes::Decimal;
                case 0x43: return KeyCodes::Multiply;
                case 0x45: return KeyCodes::Add;
                case 0x47: return KeyCodes::OemClear;
                case 0x4B: return KeyCodes::Divide;
                case 0x4C: return KeyCodes::Return;
                case 0x4E: return KeyCodes::Subtract;

                case 0x51: return KeyCodes::OemPlus; // Redirect "Equality" key
                case 0x52: return KeyCodes::Num0;
                case 0x53: return KeyCodes::Num1;
                case 0x54: return KeyCodes::Num2;
                case 0x55: return KeyCodes::Num3;
                case 0x56: return KeyCodes::Num4;
                case 0x57: return KeyCodes::Num5;
                case 0x58: return KeyCodes::Num6;
                case 0x59: return KeyCodes::Num7;
                case 0x5B: return KeyCodes::Num8;
                case 0x5C: return KeyCodes::Num9;

                case 0x30: return KeyCodes::Tab;
                case 0x31: return KeyCodes::Space;
            }
            dead = true;
            switch (key) {
                case 0x24: return KeyCodes::Return;
                case 0x33: return KeyCodes::Back;
                case 0x35: return KeyCodes::Escape;
                case 0x37: return KeyCodes::LeftSystem;
                case 0x38: return KeyCodes::LeftShift;
                case 0x39: return KeyCodes::CapsLock;
                case 0x3A: return KeyCodes::LeftAlternative;
                case 0x3B: return KeyCodes::LeftControl;
                case 0x3C: return KeyCodes::RightShift;
                case 0x3D: return KeyCodes::RightAlternative;
                case 0x3E: return KeyCodes::RightControl;
                case 0x3F: return 0; // "Function" key redirection

                case 0x40: return KeyCodes::F17;
                case 0x48: return KeyCodes::VolumeUp;
                case 0x49: return KeyCodes::VolumeDown;
                case 0x4A: return KeyCodes::VolumeMute;
                case 0x4F: return KeyCodes::F18;
                case 0x50: return KeyCodes::F19;
                case 0x5A: return KeyCodes::F20;

                case 0x60: return KeyCodes::F5;
                case 0x61: return KeyCodes::F6;
                case 0x62: return KeyCodes::F7;
                case 0x63: return KeyCodes::F3;
                case 0x64: return KeyCodes::F8;
                case 0x65: return KeyCodes::F9;
                case 0x67: return KeyCodes::F11;
                case 0x69: return KeyCodes::F13;
                case 0x6A: return KeyCodes::F16;
                case 0x6B: return KeyCodes::F14;
                case 0x6D: return KeyCodes::F10;
                case 0x6F: return KeyCodes::F12;

                case 0x71: return KeyCodes::F15;
                case 0x72: return KeyCodes::Help;
                case 0x73: return KeyCodes::Home;
                case 0x74: return KeyCodes::PageUp;
                case 0x75: return KeyCodes::Delete;
                case 0x76: return KeyCodes::F4;
                case 0x77: return KeyCodes::End;
                case 0x78: return KeyCodes::F2;
                case 0x79: return KeyCodes::PageDown;
                case 0x7A: return KeyCodes::F1;
                case 0x7B: return KeyCodes::Left;
                case 0x7C: return KeyCodes::Right;
                case 0x7D: return KeyCodes::Down;
                case 0x7E: return KeyCodes::Up;

                default: return 0;
            }
        }
    }
}