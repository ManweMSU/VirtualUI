#include "Reflection.h"

namespace Engine
{
	namespace Reflection
	{
		GetPropertyEnumerator::GetPropertyEnumerator(const string & property_name) { Result.Address = 0; Result.Type = PropertyType::Unknown; Result.InnerType = PropertyType::Unknown; Result.Volume = -1; Result.ElementSize = 0; Result.Name = property_name; }
		void GetPropertyEnumerator::EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size) { if (name == Result.Name) { Result.Address = address; Result.Type = type; Result.InnerType = inner; Result.Volume = volume; Result.ElementSize = element_size; } }
		void Reflected::EnumerateProperties(IPropertyEnumerator & enumerator) {}
		void Reflected::WasDeserialized(void) {}
		void Reflected::Serialize(ISerializer * serializer) { serializer->SerializeObject(*this); }
		void Reflected::Deserialize(ISerializer * serializer) { serializer->DeserializeObject(*this); }
		void PropertyZeroInitializer::EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size)
		{
			PropertyInfo Prop = { address, type, inner, volume, element_size, name };
			for (int i = 0; i < Prop.Volume; i++) {
				PropertyInfo Info = Prop.VolumeElement(i);
				if (type == PropertyType::UInt8 || type == PropertyType::Int8) {
					Info.Set<uint8>(0);
				} else if (type == PropertyType::UInt16 || type == PropertyType::Int16) {
					Info.Set<uint16>(0);
				} else if (type == PropertyType::UInt32 || type == PropertyType::Int32) {
					Info.Set<uint32>(0);
				} else if (type == PropertyType::UInt64 || type == PropertyType::Int64) {
					Info.Set<uint64>(0);
				} else if (type == PropertyType::Float) {
					Info.Set<float>(0.0f);
				} else if (type == PropertyType::Double) {
					Info.Set<double>(0.0);
				} else if (type == PropertyType::Complex) {
					Info.Set<Math::complex>(0.0);
				} else if (type == PropertyType::Boolean) {
					Info.Set<bool>(false);
				} else if (type == PropertyType::Color) {
					Info.Set<UI::Color>(0);
				} else if (type == PropertyType::Time) {
					Info.Set<Time>(0);
				} else if (type == PropertyType::Rectangle) {
					Info.Set<UI::Rectangle>(UI::Rectangle(0, 0, 0, 0));
				} else if (type == PropertyType::Structure) {
					Info.Get<Reflected>().EnumerateProperties(*this);
				}
			}
		}
	}
}