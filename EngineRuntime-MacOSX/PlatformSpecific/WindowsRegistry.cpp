#include "WindowsRegistry.h"

#ifdef ENGINE_WINDOWS

#include "../Streaming.h"
#include "../Miscellaneous/DynamicString.h"

#include <Windows.h>

namespace Engine
{
	namespace WindowsSpecific
	{
		class RegistryKeyInternal : public RegistryKey
		{
		public:
			HKEY key;
			bool close;
			RegistryKeyInternal(HKEY _key, bool _close) : key(_key), close(_close) {}
			virtual ~RegistryKeyInternal(void) override { if (close) RegCloseKey(key); }

			virtual RegistryKey * CreateKey(const string & Name) override
			{
				if (!Name.Length()) return 0;
				HKEY New;
				if (RegCreateKeyExW(key, Name, 0, 0, 0, KEY_ALL_ACCESS, 0, &New, 0) != ERROR_SUCCESS) return 0;
				return new RegistryKeyInternal(New, true);
			}
			virtual RegistryKey * OpenKey(const string & Name, RegistryKeyAccess access) override
			{
				if (!Name.Length()) return 0;
				if (Name.FindFirst(L'\\') != -1 || Name.FindFirst(L'/') != -1) {
					auto split = IO::NormalizePath(Name).Split(L'\\');
					SafePointer<RegistryKey> Current;
					Current.SetRetain(this);
					for (int i = 0; i < split.Length(); i++) Current = Current->OpenKey(split[i], access);
				} else {
					HKEY New;
					REGSAM Access = KEY_READ;
					if (access == RegistryKeyAccess::Full) Access = KEY_ALL_ACCESS;
					if (RegOpenKeyExW(key, Name, 0, Access, &New) != ERROR_SUCCESS) return 0;
					return new RegistryKeyInternal(New, true);
				}
			}
			virtual void DeleteKey(const string & Name) override
			{
				RegDeleteKeyExW(key, Name, 0, 0);
			}
			virtual void DeleteValue(const string & Name) override
			{
				RegDeleteValueW(key, Name);
			}
			virtual void SetValue(const string & Name, const string & Value) override
			{
				RegSetValueExW(key, Name, 0, REG_SZ, reinterpret_cast<const BYTE *>(static_cast<const widechar *>(Value)), Value.Length() * 2 + 2);
			}
			virtual void SetValueAsExpandable(const string & Name, const string & Value) override
			{
				RegSetValueExW(key, Name, 0, REG_EXPAND_SZ, reinterpret_cast<const BYTE *>(static_cast<const widechar *>(Value)), Value.Length() * 2 + 2);
			}
			virtual void SetValue(const string & Name, const Array<string> & Value) override
			{
				Array<widechar> value(0x100);
				for (int i = 0; i < Value.Length(); i++) {
					value.Append(static_cast<const widechar *>(Value[i]), Value[i].Length() + 1);
				}
				value.Append(0);
				RegSetValueExW(key, Name, 0, REG_MULTI_SZ, reinterpret_cast<const BYTE *>(value.GetBuffer()), value.Length());
			}
			virtual void SetValue(const string & Name, uint32 Value) override
			{
				RegSetValueExW(key, Name, 0, REG_DWORD, reinterpret_cast<const BYTE *>(&Value), 4);
			}
			virtual void SetValue(const string & Name, uint64 Value) override
			{
				RegSetValueExW(key, Name, 0, REG_QWORD, reinterpret_cast<const BYTE *>(&Value), 8);
			}
			virtual void SetValue(const string & Name, const void * Value, int ValueSize) override
			{
				RegSetValueExW(key, Name, 0, REG_BINARY, reinterpret_cast<const BYTE *>(Value), ValueSize);
			}
			virtual string GetValueString(const string & Name) override
			{
				DWORD Size, Type;
				if (RegQueryValueExW(key, Name, 0, &Type, 0, &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				if (Type != REG_SZ && Type != REG_EXPAND_SZ) throw IO::FileAccessException();
				Array<widechar> Data(Size / 2);
				Data.SetLength(Size / 2);
				if (RegQueryValueExW(key, Name, 0, &Type, reinterpret_cast<BYTE *>(Data.GetBuffer()), &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				Data[Size / 2 - 1] = 0;
				return string(Data.GetBuffer());
			}
			virtual Array<string> * GetValueStringArray(const string & Name) override
			{
				DWORD Size, Type;
				if (RegQueryValueExW(key, Name, 0, &Type, 0, &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				if (Type != REG_MULTI_SZ) throw IO::FileAccessException();
				Array<widechar> Data(Size / 2);
				Data.SetLength(Size / 2);
				if (RegQueryValueExW(key, Name, 0, &Type, reinterpret_cast<BYTE *>(Data.GetBuffer()), &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				SafePointer< Array<string> > Result = new Array<string>(0x10);
				if (Size > 2) {
					int p = 0;
					while (p < Data.Length() - 1) {
						Result->Append(string(Data.GetBuffer() + p));
						p += Result->LastElement().Length() + 1;
						if (Result->LastElement().Length() == 0) { Result->RemoveLast(); break; }
					}
				}
				Result->Retain();
				return Result;
			}
			virtual uint32 GetValueUInt32(const string & Name) override
			{
				DWORD Size, Type;
				if (RegQueryValueExW(key, Name, 0, &Type, 0, &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				if (Type != REG_DWORD || Size < 4) throw IO::FileAccessException();
				uint32 Value = 0;
				Size = 4;
				RegQueryValueExW(key, Name, 0, &Type, reinterpret_cast<BYTE *>(&Value), &Size);
				return Value;
			}
			virtual uint64 GetValueUInt64(const string & Name) override
			{
				DWORD Size, Type;
				if (RegQueryValueExW(key, Name, 0, &Type, 0, &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				if (Type != REG_QWORD || Size < 8) throw IO::FileAccessException();
				uint64 Value = 0;
				Size = 8;
				RegQueryValueExW(key, Name, 0, &Type, reinterpret_cast<BYTE *>(&Value), &Size);
				return Value;
			}
			virtual Array<uint8> * GetValueBinary(const string & Name) override
			{
				DWORD Size, Type;
				if (RegQueryValueExW(key, Name, 0, &Type, 0, &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				if (Type != REG_BINARY) throw IO::FileAccessException();
				SafePointer< Array<uint8> > Data = new Array<uint8>(Size);
				Data->SetLength(Size);
				if (RegQueryValueExW(key, Name, 0, &Type, reinterpret_cast<BYTE *>(Data->GetBuffer()), &Size) != ERROR_SUCCESS) throw IO::FileAccessException();
				Data->Retain();
				return Data;
			}
			virtual RegistryValueType GetValueType(const string & Name) override
			{
				DWORD Type;
				RegQueryValueExW(key, Name, 0, &Type, 0, 0);
				if (Type == REG_SZ) return RegistryValueType::String;
				else if (Type == REG_EXPAND_SZ) return RegistryValueType::ExpandableString;
				else if (Type == REG_MULTI_SZ) return RegistryValueType::StringArray;
				else if (Type == REG_BINARY) return RegistryValueType::Binary;
				else if (Type == REG_DWORD) return RegistryValueType::UInt32;
				else if (Type == REG_QWORD) return RegistryValueType::UInt64;
				else return RegistryValueType::Unknown;
			}
			virtual Array<string> * EnumerateSubkeys(void) override
			{
				SafePointer< Array<string> > Result = new Array<string>(0x10);
				DynamicString Name;
				DWORD MaxLen;
				RegQueryInfoKeyW(key, 0, 0, 0, 0, &MaxLen, 0, 0, 0, 0, 0, 0);
				Name.ReserveLength(MaxLen + 1);
				int index = 0;
				while (true) {
					DWORD Len = Name.ReservedLength();
					auto Status = RegEnumKeyExW(key, index, Name, &Len, 0, 0, 0, 0);
					if (Status == ERROR_NO_MORE_ITEMS) break;
					if (Status != ERROR_SUCCESS) throw IO::FileAccessException();
					Result->Append(Name);
					index++;
				}
				Result->Retain();
				return Result;
			}
			virtual Array<string> * EnumerateValues(void) override
			{
				SafePointer< Array<string> > Result = new Array<string>(0x10);
				DynamicString Name;
				DWORD MaxLen;
				RegQueryInfoKeyW(key, 0, 0, 0, 0, 0, 0, 0, &MaxLen, 0, 0, 0);
				Name.ReserveLength(MaxLen + 1);
				int index = 0;
				while (true) {
					DWORD Len = Name.ReservedLength();
					auto Status = RegEnumValueW(key, index, Name, &Len, 0, 0, 0, 0);
					if (Status == ERROR_NO_MORE_ITEMS) break;
					if (Status != ERROR_SUCCESS) throw IO::FileAccessException();
					Result->Append(Name);
					index++;
				}
				Result->Retain();
				return Result;
			}
		};
		RegistryKey * OpenRootRegistryKey(RegistryRootKey key)
		{
			if (key == RegistryRootKey::Classes) return new RegistryKeyInternal(HKEY_CLASSES_ROOT, false);
			else if (key == RegistryRootKey::CurrentConfig) return new RegistryKeyInternal(HKEY_CURRENT_CONFIG, false);
			else if (key == RegistryRootKey::CurrentUser) return new RegistryKeyInternal(HKEY_CURRENT_USER, false);
			else if (key == RegistryRootKey::LocalMachine) return new RegistryKeyInternal(HKEY_LOCAL_MACHINE, false);
			else if (key == RegistryRootKey::Users) return new RegistryKeyInternal(HKEY_USERS, false);
			else return 0;
		}
		string ExpandEnvironmentalString(const string & src)
		{
			DynamicString Result;
			auto Length = ExpandEnvironmentStringsW(src, 0, 0);
			Result.ReserveLength(Length);
			ExpandEnvironmentStringsW(src, Result, Result.ReservedLength());
			return Result;
		}
	}
}

#endif