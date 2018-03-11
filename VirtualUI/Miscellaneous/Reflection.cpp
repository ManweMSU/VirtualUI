#include "Reflection.h"

namespace Engine
{
	namespace Reflection
	{
		GetPropertyEnumerator::GetPropertyEnumerator(const string & property_name) { Result.Address = 0; Result.Type = PropertyType::Unknown; Result.Name = property_name; }
		void GetPropertyEnumerator::EnumerateProperty(const string & name, void * address, PropertyType type) { if (name == Result.Name) { Result.Address = address; Result.Type = type; } }
		void Reflected::EnumerateProperties(IPropertyEnumerator & enumerator) {}
		void PropertyZeroInitializer::EnumerateProperty(const string & name, void * address, PropertyType type)
		{
			PropertyInfo Info = { address, type, name };
			if (type == PropertyType::Integer) {
				Info.Set<int>(0);
			} else if (type == PropertyType::Double) {
				Info.Set<double>(0.0);
			} else if (type == PropertyType::Boolean) {
				Info.Set<bool>(false);
			} else if (type == PropertyType::Color) {
				Info.Set<UI::Color>(0);
			} else if (type == PropertyType::Rectangle) {
				Info.Set<UI::Rectangle>(UI::Rectangle(0, 0, 0, 0));
			}
		}
	}
}