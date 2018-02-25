#include "Reflection.h"

namespace Engine
{
	namespace Reflection
	{
		PropertyBase::PropertyBase(void) {}
		PropertyBase::~PropertyBase(void) {}
		void ReflectableObject::_reflect_property(const string & Name, PropertyBase * refrence) { _properties << reinterpret_cast<uint8*>(refrence) - reinterpret_cast<uint8*>(this); }
		ReflectableObject::ReflectableObject(void) : _properties(0x10) {}
		ReflectableObject::~ReflectableObject(void) {}
		PropertyBase * ReflectableObject::GetProperty(const string & Name)
		{
			for (int i = 0; i < _properties.Length(); i++) {
				auto p = reinterpret_cast<PropertyBase *>(reinterpret_cast<uint8*>(this) + _properties[i]);
				if (p->Name().GetName() == Name) return p;
			}
			return 0;
		}
		const PropertyBase * ReflectableObject::GetProperty(const string & Name) const
		{
			for (int i = 0; i < _properties.Length(); i++) {
				auto p = reinterpret_cast<const PropertyBase *>(reinterpret_cast<const uint8*>(this) + _properties[i]);
				if (p->Name().GetName() == Name) return p;
			}
			return 0;
		}
		int ReflectableObject::PropertyCount(void) const { return _properties.Length(); }
		PropertyBase * ReflectableObject::PropertyAt(int Index)
		{
			return reinterpret_cast<PropertyBase *>(reinterpret_cast<uint8*>(this) + _properties[Index]);
		}
		const PropertyBase * ReflectableObject::PropertyAt(int Index) const
		{
			return reinterpret_cast<const PropertyBase *>(reinterpret_cast<const uint8*>(this) + _properties[Index]);
		}
	}
}