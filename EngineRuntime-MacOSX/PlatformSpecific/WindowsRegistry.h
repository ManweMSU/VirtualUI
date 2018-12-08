#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace WindowsSpecific
	{
#ifdef ENGINE_WINDOWS
		enum class RegistryRootKey { Classes, CurrentConfig, CurrentUser, LocalMachine, Users };
		enum class RegistryValueType { Unknown, Binary, String, ExpandableString, StringArray, UInt32, UInt64 };
		enum class RegistryKeyAccess { Full, ReadOnly };
		class RegistryKey : public Object
		{
		public:
			virtual RegistryKey * CreateKey(const string & Name) = 0;
			virtual RegistryKey * OpenKey(const string & Name, RegistryKeyAccess access) = 0;
			virtual void DeleteKey(const string & Name) = 0;
			virtual void DeleteValue(const string & Name) = 0;
			virtual void SetValue(const string & Name, const string & Value) = 0;
			virtual void SetValueAsExpandable(const string & Name, const string & Value) = 0;
			virtual void SetValue(const string & Name, const Array<string> & Value) = 0;
			virtual void SetValue(const string & Name, uint32 Value) = 0;
			virtual void SetValue(const string & Name, uint64 Value) = 0;
			virtual void SetValue(const string & Name, const void * Value, int ValueSize) = 0;
			virtual string GetValueString(const string & Name) = 0;
			virtual Array<string> * GetValueStringArray(const string & Name) = 0;
			virtual uint32 GetValueUInt32(const string & Name) = 0;
			virtual uint64 GetValueUInt64(const string & Name) = 0;
			virtual Array<uint8> * GetValueBinary(const string & Name) = 0;
			virtual RegistryValueType GetValueType(const string & Name) = 0;
			virtual Array<string> * EnumerateSubkeys(void) = 0;
			virtual Array<string> * EnumerateValues(void) = 0;
		};
		RegistryKey * OpenRootRegistryKey(RegistryRootKey key);
		string ExpandEnvironmentalString(const string & src);
#endif
	}
}