#include "KeyCodes.h"

#include "../EngineBase.h"
#include "../Miscellaneous/DynamicString.h"

#include <Windows.h>

namespace Engine
{
	namespace Keyboard
	{
		namespace KeyboardHelper
		{
			int KeyboardDelay = -1;
			int KeyboardSpeed = -1;

			void Initialize(void)
			{
				HKEY cp, kb;
				DWORD Size;
				DynamicString value;
				value.ReserveLength(0x100);
				RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel", 0, KEY_READ, &cp);
				RegOpenKeyExW(cp, L"Keyboard", 0, KEY_READ, &kb);
				do {
					Size = value.ReservedLength() * 2;
					auto result = RegQueryValueExW(kb, L"KeyboardDelay", 0, 0, reinterpret_cast<uint8 *>(value.GetBuffer()), &Size);
					if (result == ERROR_SUCCESS) break;
					else if (result != ERROR_MORE_DATA) {
						RegCloseKey(kb);
						RegCloseKey(cp);
						throw Exception();
					}
					value.ReserveLength(value.ReservedLength() * 2);
				} while (true);
				KeyboardDelay = value.ToString().ToInt32();
				do {
					Size = value.ReservedLength() * 2;
					auto result = RegQueryValueExW(kb, L"KeyboardSpeed", 0, 0, reinterpret_cast<uint8 *>(value.GetBuffer()), &Size);
					if (result == ERROR_SUCCESS) break;
					else if (result != ERROR_MORE_DATA) {
						RegCloseKey(kb);
						RegCloseKey(cp);
						throw Exception();
					}
					value.ReserveLength(value.ReservedLength() * 2);
				} while (true);
				KeyboardSpeed = value.ToString().ToInt32();
				RegCloseKey(kb);
				RegCloseKey(cp);
				KeyboardSpeed = 1000 / (2 + 28 * (KeyboardSpeed / 31));
				KeyboardDelay = 250 * (KeyboardDelay + 1);
			}
		}

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
		int GetKeyboardDelay(void)
		{
			if (KeyboardHelper::KeyboardDelay < 0) KeyboardHelper::Initialize();
			return KeyboardHelper::KeyboardDelay;
		}
		int GetKeyboardSpeed(void)
		{
			if (KeyboardHelper::KeyboardSpeed < 0) KeyboardHelper::Initialize();
			return KeyboardHelper::KeyboardSpeed;
		}
	}
}