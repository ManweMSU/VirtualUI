#include "KeyCodes.h"

#include "../EngineBase.h"

#include <Windows.h>

namespace Engine
{
	namespace Keyboard
	{
		bool IsKeyPressed(uint key_code)
		{
			if (key_code == KeyCodes::System) {
				return (GetKeyState(KeyCodes::LeftSystem) & 0x8000) || (GetKeyState(KeyCodes::RightSystem) & 0x8000);
			} else return (GetKeyState(key_code) & 0x8000) != 0;
		}
		bool IsKeyToggled(uint key_code)
		{
			if (key_code != KeyCodes::CapsLock && key_code != KeyCodes::NumLock && key_code != KeyCodes::ScrollLock) throw InvalidArgumentException();
			return (GetKeyState(key_code) & 1) != 0;
		}
	}
}