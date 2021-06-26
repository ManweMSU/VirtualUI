#include "Reflection.h"

namespace Engine
{
	namespace Reflection
	{
		GetPropertyEnumerator::GetPropertyEnumerator(const string & property_name) { Result.Address = 0; Result.Type = PropertyType::Unknown; Result.InnerType = PropertyType::Unknown; Result.Volume = -1; Result.ElementSize = 0; Result.Name = property_name; }
		void GetPropertyEnumerator::EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size) { if (name == Result.Name) { Result.Address = address; Result.Type = type; Result.InnerType = inner; Result.Volume = volume; Result.ElementSize = element_size; } }
		void Reflected::EnumerateProperties(IPropertyEnumerator & enumerator) {}
		void Reflected::WillBeSerialized(void) {}
		void Reflected::WasSerialized(void) {}
		void Reflected::WillBeDeserialized(void) {}
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
					Info.Set<Color>(0);
				} else if (type == PropertyType::Time) {
					Info.Set<Time>(0);
				} else if (type == PropertyType::Rectangle) {
					Info.Set<UI::Rectangle>(UI::Rectangle(0, 0, 0, 0));
				} else if (type == PropertyType::Structure) {
					Info.Get<Reflected>().EnumerateProperties(*this);
				}
			}
		}
		PropertyCopyInitializer::PropertyCopyInitializer(Reflected & base_object) : _base(base_object) {}
		void PropertyCopyInitializer::EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size)
		{
			auto src = _base.GetProperty(name);
			PropertyInfo dest = PropertyInfo{ address, type, inner, volume, element_size, name };
			if (!src.Address || src.Type != type || src.InnerType != inner || src.Volume != volume || src.ElementSize != element_size) return;
			if (type == PropertyType::String) {
				for (int i = 0; i < volume; i++) {
					auto esrc = src.VolumeElement(i), edest = dest.VolumeElement(i);
					edest.Set(esrc.Get<string>());
				}
			} else if (type == PropertyType::Texture) {
				for (int i = 0; i < volume; i++) {
					auto esrc = src.VolumeElement(i), edest = dest.VolumeElement(i);
					edest.Get< SafePointer<Graphics::IBitmap> >().SetRetain(esrc.Get< SafePointer<Graphics::IBitmap> >());
				}
			} else if (type == PropertyType::Font) {
				for (int i = 0; i < volume; i++) {
					auto esrc = src.VolumeElement(i), edest = dest.VolumeElement(i);
					edest.Get< SafePointer<Graphics::IFont> >().SetRetain(esrc.Get< SafePointer<Graphics::IFont> >());
				}
			} else if (type == PropertyType::Application) {
				for (int i = 0; i < volume; i++) {
					auto esrc = src.VolumeElement(i), edest = dest.VolumeElement(i);
					edest.Get< SafePointer<UI::Template::Shape> >().SetRetain(esrc.Get< SafePointer<UI::Template::Shape> >());
				}
			} else if (type == PropertyType::Dialog) {
				for (int i = 0; i < volume; i++) {
					auto esrc = src.VolumeElement(i), edest = dest.VolumeElement(i);
					edest.Get< SafePointer<UI::Template::ControlTemplate> >().SetRetain(esrc.Get< SafePointer<UI::Template::ControlTemplate> >());
				}
			} else if (type == PropertyType::Array) {
				if (inner == PropertyType::UInt8 || inner == PropertyType::Int8 || inner == PropertyType::Boolean) dest.Get< Array<uint8> >() = src.Get< Array<uint8> >();
				else if (inner == PropertyType::UInt16 || inner == PropertyType::Int16) dest.Get< Array<uint16> >() = src.Get< Array<uint16> >();
				else if (inner == PropertyType::UInt32 || inner == PropertyType::Int32 || inner == PropertyType::Float || inner == PropertyType::Color) dest.Get< Array<uint32> >() = src.Get< Array<uint32> >();
				else if (inner == PropertyType::UInt64 || inner == PropertyType::Int64 || inner == PropertyType::Double || inner == PropertyType::Time) dest.Get< Array<uint64> >() = src.Get< Array<uint64> >();
				else if (inner == PropertyType::Complex) dest.Get< Array<Math::Complex> >() = src.Get< Array<Math::Complex> >();
				else if (inner == PropertyType::String) dest.Get< Array<string> >() = src.Get< Array<string> >();
				else if (inner == PropertyType::Rectangle) dest.Get< Array<UI::Rectangle> >() = src.Get< Array<UI::Rectangle> >();
				else if (inner == PropertyType::Texture) dest.Get< Array< SafePointer<Graphics::IBitmap> > >() = src.Get< Array< SafePointer<Graphics::IBitmap> > >();
				else if (inner == PropertyType::Font) dest.Get< Array< SafePointer<Graphics::IFont> > >() = src.Get< Array< SafePointer<Graphics::IFont> > >();
				else if (inner == PropertyType::Application) dest.Get< Array< SafePointer<UI::Template::Shape> > >() = src.Get< Array< SafePointer<UI::Template::Shape> > >();
				else if (inner == PropertyType::Dialog) dest.Get< Array< SafePointer<UI::Template::ControlTemplate> > >() = src.Get< Array< SafePointer<UI::Template::ControlTemplate> > >();
				else if (inner == PropertyType::Structure) {
					auto & asrc = src.Get<ReflectedArray>();
					auto & adest = dest.Get<ReflectedArray>();
					for (int i = 0; i < asrc.Length(); i++) {
						adest.AppendNew();
						auto & esrc = asrc.ElementAt(i);
						auto & edest = adest.LastElement();
						PropertyCopyInitializer inner_initializer(esrc);
						edest.EnumerateProperties(inner_initializer);
					}
				}
			} else if (type == PropertyType::SafeArray) {
				if (inner == PropertyType::UInt8 || inner == PropertyType::Int8 || inner == PropertyType::Boolean) dest.Get< SafeArray<uint8> >() = src.Get< SafeArray<uint8> >();
				else if (inner == PropertyType::UInt16 || inner == PropertyType::Int16) dest.Get< SafeArray<uint16> >() = src.Get< SafeArray<uint16> >();
				else if (inner == PropertyType::UInt32 || inner == PropertyType::Int32 || inner == PropertyType::Float || inner == PropertyType::Color) dest.Get< SafeArray<uint32> >() = src.Get< SafeArray<uint32> >();
				else if (inner == PropertyType::UInt64 || inner == PropertyType::Int64 || inner == PropertyType::Double || inner == PropertyType::Time) dest.Get< SafeArray<uint64> >() = src.Get< SafeArray<uint64> >();
				else if (inner == PropertyType::Complex) dest.Get< SafeArray<Math::Complex> >() = src.Get< SafeArray<Math::Complex> >();
				else if (inner == PropertyType::String) dest.Get< SafeArray<string> >() = src.Get< SafeArray<string> >();
				else if (inner == PropertyType::Rectangle) dest.Get< SafeArray<UI::Rectangle> >() = src.Get< SafeArray<UI::Rectangle> >();
				else if (inner == PropertyType::Texture) dest.Get< SafeArray< SafePointer<Graphics::IBitmap> > >() = src.Get< SafeArray< SafePointer<Graphics::IBitmap> > >();
				else if (inner == PropertyType::Font) dest.Get< SafeArray< SafePointer<Graphics::IFont> > >() = src.Get< SafeArray< SafePointer<Graphics::IFont> > >();
				else if (inner == PropertyType::Application) dest.Get< SafeArray< SafePointer<UI::Template::Shape> > >() = src.Get< SafeArray< SafePointer<UI::Template::Shape> > >();
				else if (inner == PropertyType::Dialog) dest.Get< SafeArray< SafePointer<UI::Template::ControlTemplate> > >() = src.Get< SafeArray< SafePointer<UI::Template::ControlTemplate> > >();
				else if (inner == PropertyType::Structure) {
					auto & asrc = src.Get<ReflectedArray>();
					auto & adest = dest.Get<ReflectedArray>();
					for (int i = 0; i < asrc.Length(); i++) {
						adest.AppendNew();
						auto & esrc = asrc.ElementAt(i);
						auto & edest = adest.LastElement();
						PropertyCopyInitializer inner_initializer(esrc);
						edest.EnumerateProperties(inner_initializer);
					}
				}
			} else if (type == PropertyType::Structure) {
				PropertyCopyInitializer inner_initializer(src.Get<Reflected>());
				dest.Get<Reflected>().EnumerateProperties(inner_initializer);
			} else MemoryCopy(address, src.Address, volume * element_size);
		}
	}
}