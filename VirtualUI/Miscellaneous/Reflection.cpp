#include "Reflection.h"

namespace Engine
{
	namespace Reflection
	{
		GetPropertyEnumerator::GetPropertyEnumerator(const string & property_name) { Result.Address = 0; Result.Type = PropertyType::Unknown; Result.Name = property_name; }
		void GetPropertyEnumerator::EnumerateProperty(const string & name, void * address, PropertyType type) { (*conout) << name << IO::NewLineChar; if (name == Result.Name) { Result.Address = address; Result.Type = type; } }
	}
}