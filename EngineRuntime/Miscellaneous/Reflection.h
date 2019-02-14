#pragma once

#include "../EngineBase.h"
#include "../UserInterface/ShapeBase.h"
#include "../Math/MathBase.h"
#include "../Math/Complex.h"
#include "Time.h"

namespace Engine
{
	namespace Reflection
	{
		enum class PropertyType {
			Byte = 0, UInt8 = 0, Int8 = 1, UInt16 = 2, Int16 = 3, UInt32 = 4, Int32 = 5, Integer = 5, UInt64 = 6, Int64 = 7, LongInteger = 7,
			Float = 8, ShortReal = 8, Double = 9, Real = 9, Complex = 10,
			Boolean = 11, String = 12, Color = 13, Time = 14,
			Texture = 15, Font = 16, Application = 17, Dialog = 18, Rectangle = 19,
			Array = 20, SafeArray = 21, Structure = 22,
			Unknown = -1
		};
			//Integer = 0, Double = 1, Boolean = 2, Color = 3, String = 4, Texture = 5, Font = 6, Application = 7, Dialog = 8, Rectangle = 9, Unknown = -1 };
		struct PropertyInfo
		{
			void * Address;
			PropertyType Type;
			PropertyType InnerType;
			int Volume;
			int ElementSize;
			string Name;

			template <class V> void Set(const V & value) { *reinterpret_cast<V *>(Address) = value; }
			template <class V> const V & Get(void) const { return *reinterpret_cast<const V *>(Address); }
			template <class V> V & Get(void) { return *reinterpret_cast<V *>(Address); }

			PropertyInfo VolumeElement(int at) { return PropertyInfo{ reinterpret_cast<uint8 *>(Address) + at * ElementSize, Type, InnerType, 1, ElementSize, Name }; }
		};
		class IPropertyEnumerator
		{
		public:
			virtual void EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size) = 0;
		};
		class GetPropertyEnumerator : public IPropertyEnumerator
		{
		public:
			PropertyInfo Result;
			GetPropertyEnumerator(const string & property_name);
			GetPropertyEnumerator(const GetPropertyEnumerator & src) = delete;
			GetPropertyEnumerator & operator = (const GetPropertyEnumerator & src) = delete;
			virtual void EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size) override;
		};
		class PropertyZeroInitializer : public IPropertyEnumerator
		{
		public:
			virtual void EnumerateProperty(const string & name, void * address, PropertyType type, PropertyType inner, int volume, int element_size) override;
		};
		class Reflected;
		class ISerializer : public Object
		{
		public:
			virtual void SerializeObject(Reflected & obj) = 0;
			virtual void DeserializeObject(Reflected & obj) = 0;
		};
		class Reflected
		{
		public:
			virtual void EnumerateProperties(IPropertyEnumerator & enumerator);
			virtual PropertyInfo GetProperty(const string & name) = 0;
			virtual void WasDeserialized(void);
			void Serialize(ISerializer * serializer);
			void Deserialize(ISerializer * serializer);
		};
		class ReflectedArray : public Object
		{
		public:
			virtual void AppendNew(void) = 0;
			virtual void SwapAt(int i, int j) = 0;
			virtual void InsertNew(int at) = 0;
			virtual Reflected & ElementAt(int index) = 0;
			virtual Reflected & FirstElement(void) = 0;
			virtual Reflected & LastElement(void) = 0;
			virtual void Remove(int index) = 0;
			virtual void RemoveFirst(void) = 0;
			virtual void RemoveLast(void) = 0;
			virtual void Clear(void) = 0;
			virtual void SetLength(int length) = 0;
			virtual int Length(void) = 0;
		};
		template <class V> class ReflectedPlainArray : public ReflectedArray
		{
		public:
			Array<V> InnerArray;

			ReflectedPlainArray(void) : InnerArray(0x80) {}
			virtual ~ReflectedPlainArray(void) override {}

			operator Array<V> & (void) { return InnerArray; }
			operator const Array<V> & (void) const { return InnerArray; }
			const Array<V> * operator -> (void) const { return &InnerArray; }
			Array<V> * operator -> (void) { return &InnerArray; }
			V & operator [] (int i) { return InnerArray[i]; }
			const V & operator [] (int i) const { return InnerArray[i]; }
			ReflectedPlainArray & operator << (const V & v) { InnerArray << v; return *this; }

			virtual void AppendNew(void) override { InnerArray.Append(V()); }
			virtual void SwapAt(int i, int j) override { InnerArray.SwapAt(i, j); }
			virtual void InsertNew(int at) override { InnerArray.Insert(V(), at); }
			virtual Reflected & ElementAt(int index) override { return InnerArray.ElementAt(index); }
			virtual Reflected & FirstElement(void) override { return InnerArray.FirstElement(); }
			virtual Reflected & LastElement(void) override { return InnerArray.LastElement(); }
			virtual void Remove(int index) override { InnerArray.Remove(index); }
			virtual void RemoveFirst(void) override { InnerArray.RemoveFirst(); }
			virtual void RemoveLast(void) override { InnerArray.RemoveLast(); }
			virtual void Clear(void) override { InnerArray.Clear(); }
			virtual void SetLength(int length) override { InnerArray.SetLength(length); }
			virtual int Length(void) override { return InnerArray.Length(); }
		};
		template <class V> class ReflectedSafeArray : public ReflectedArray
		{
		public:
			SafeArray<V> InnerArray;

			ReflectedSafeArray(void) : InnerArray(0x80) {}
			virtual ~ReflectedSafeArray(void) override {}

			operator SafeArray<V> & (void) { return InnerArray; }
			operator const SafeArray<V> & (void) const { return InnerArray; }
			const SafeArray<V> * operator -> (void) const { return &InnerArray; }
			SafeArray<V> * operator -> (void) { return &InnerArray; }
			V & operator [] (int i) { return InnerArray[i]; }
			const V & operator [] (int i) const { return InnerArray[i]; }
			ReflectedSafeArray & operator << (const V & v) { InnerArray << v; return *this; }

			virtual void AppendNew(void) override { InnerArray.Append(V()); }
			virtual void SwapAt(int i, int j) override { InnerArray.SwapAt(i, j); }
			virtual void InsertNew(int at) override { InnerArray.Insert(V(), at); }
			virtual Reflected & ElementAt(int index) override { return InnerArray.ElementAt(index); }
			virtual Reflected & FirstElement(void) override { return InnerArray.FirstElement(); }
			virtual Reflected & LastElement(void) override { return InnerArray.LastElement(); }
			virtual void Remove(int index) override { InnerArray.Remove(index); }
			virtual void RemoveFirst(void) override { InnerArray.RemoveFirst(); }
			virtual void RemoveLast(void) override { InnerArray.RemoveLast(); }
			virtual void Clear(void) override { InnerArray.Clear(); }
			virtual void SetLength(int length) override { InnerArray.SetLength(length); }
			virtual int Length(void) override { return InnerArray.Length(); }
		};

#if (defined(_MSC_VER))

#define ENGINE_REFLECTED_CLASS(class_name, parent_class) class class_name : public parent_class { \
private: template<uint __index> void _mirror_property(::Engine::Reflection::IPropertyEnumerator & enumerator) {} \
public: virtual void EnumerateProperties(::Engine::Reflection::IPropertyEnumerator & enumerator) override { parent_class::EnumerateProperties(enumerator); _mirror_property<__COUNTER__ + 1>(enumerator); } \
public: virtual ::Engine::Reflection::PropertyInfo GetProperty(const string & name) override { ::Engine::Reflection::GetPropertyEnumerator enumerator(name); EnumerateProperties(enumerator); return enumerator.Result; } \

#define ENGINE_END_REFLECTED_CLASS };

#define ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, reflected_type, reflected_inner_type, cxx_type, cxx_volume, cxx_cast, define_postfix) \
private: template<> void _mirror_property<__COUNTER__>(::Engine::Reflection::IPropertyEnumerator & enumerator) {\
enumerator.EnumerateProperty(ENGINE_STRING(property_name), &cxx_cast (property_name), reflected_type, reflected_inner_type, cxx_volume, sizeof(cxx_type));\
_mirror_property<__COUNTER__ + 1>(enumerator); } \
public: cxx_type property_name define_postfix;

#else

#define ENGINE_REFLECTED_CLASS(class_name, parent_class) class class_name : public parent_class { \
private: using __class_name = class_name; \
private: template<class __enum, uint __index> struct __mirror { static void _mirror_property(__enum & enumerator, __class_name & obj) {} }; \
public: virtual void EnumerateProperties(::Engine::Reflection::IPropertyEnumerator & enumerator) override { parent_class::EnumerateProperties(enumerator); __mirror<::Engine::Reflection::IPropertyEnumerator, __COUNTER__ + 1>::_mirror_property(enumerator, *this); } \
public: virtual ::Engine::Reflection::PropertyInfo GetProperty(const string & name) override { ::Engine::Reflection::GetPropertyEnumerator enumerator(name); EnumerateProperties(enumerator); return enumerator.Result; } \

#define ENGINE_END_REFLECTED_CLASS };

#define ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, reflected_type, reflected_inner_type, cxx_type, cxx_volume, cxx_cast, define_postfix) \
private: template<class __enum> struct __mirror<__enum, __COUNTER__> { static void _mirror_property(__enum & enumerator, __class_name & obj) {\
enumerator.EnumerateProperty(ENGINE_STRING(property_name), &cxx_cast (obj.property_name), reflected_type, reflected_inner_type, cxx_volume, sizeof(cxx_type));\
__mirror<__enum, __COUNTER__ + 1>::_mirror_property(enumerator, obj); } }; \
public: cxx_type property_name define_postfix;

#endif

#define ENGINE_DEFINE_REFLECTED_PROPERTY(reflected_type, property_name) ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, ENGINE_REFLECTED_TYPE_##reflected_type, ::Engine::Reflection::PropertyType::Unknown, ENGINE_CXX_TYPE_##reflected_type, 1, , )
#define ENGINE_DEFINE_REFLECTED_PROPERTY_VOLUME(reflected_type, property_name, volume) ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, ENGINE_REFLECTED_TYPE_##reflected_type, ::Engine::Reflection::PropertyType::Unknown, ENGINE_CXX_TYPE_##reflected_type, volume, , [volume])
#define ENGINE_DEFINE_REFLECTED_ARRAY(reflected_type, property_name) ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, ENGINE_REFLECTED_TYPE_ARRAY, ENGINE_REFLECTED_TYPE_##reflected_type, ::Engine::Array< ENGINE_CXX_TYPE_##reflected_type >, 1, , = ::Engine::Array< ENGINE_CXX_TYPE_##reflected_type >(0x80))
#define ENGINE_DEFINE_REFLECTED_SAFE_ARRAY(reflected_type, property_name) ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, ENGINE_REFLECTED_TYPE_SAFEARRAY, ENGINE_REFLECTED_TYPE_##reflected_type, ::Engine::SafeArray< ENGINE_CXX_TYPE_##reflected_type >, 1, , = ::Engine::SafeArray< ENGINE_CXX_TYPE_##reflected_type >(0x80))
#define ENGINE_DEFINE_REFLECTED_GENERIC_PROPERTY(generic_type, property_name) ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, ENGINE_REFLECTED_TYPE_STRUCTURE, ::Engine::Reflection::PropertyType::Unknown, generic_type, 1, static_cast<::Engine::Reflection::Reflected &>, )
#define ENGINE_DEFINE_REFLECTED_GENERIC_ARRAY(generic_type, property_name) ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, ENGINE_REFLECTED_TYPE_ARRAY, ENGINE_REFLECTED_TYPE_STRUCTURE, ::Engine::Reflection::ReflectedPlainArray<generic_type>, 1, static_cast<::Engine::Reflection::ReflectedArray &>, )
#define ENGINE_DEFINE_REFLECTED_GENERIC_SAFE_ARRAY(generic_type, property_name) ENGINE_INTERNAL_DEFINE_REFLECTED_PROPERTY(property_name, ENGINE_REFLECTED_TYPE_SAFEARRAY, ENGINE_REFLECTED_TYPE_STRUCTURE, ::Engine::Reflection::ReflectedSafeArray<generic_type>, 1, static_cast<::Engine::Reflection::ReflectedArray &>, )

#define ENGINE_STRING(macro_arg) ENGINE_STRING_INTERNAL(#macro_arg)
#define ENGINE_STRING_INTERNAL(macro_arg) L##macro_arg

#define ENGINE_CXX_TYPE_UINT8				::Engine::uint8
#define ENGINE_CXX_TYPE_BYTE				::Engine::uint8
#define ENGINE_CXX_TYPE_INT8				::Engine::int8
#define ENGINE_CXX_TYPE_UINT16				::Engine::uint16
#define ENGINE_CXX_TYPE_INT16				::Engine::int16
#define ENGINE_CXX_TYPE_UINT32				::Engine::uint32
#define ENGINE_CXX_TYPE_INT32				::Engine::int32
#define ENGINE_CXX_TYPE_INTEGER				::Engine::int32
#define ENGINE_CXX_TYPE_UINT64				::Engine::uint64
#define ENGINE_CXX_TYPE_INT64				::Engine::int64
#define ENGINE_CXX_TYPE_LONGINTEGER			::Engine::int64
#define ENGINE_CXX_TYPE_FLOAT				::Engine::Math::ShortReal
#define ENGINE_CXX_TYPE_SHORTREAL			::Engine::Math::ShortReal
#define ENGINE_CXX_TYPE_DOUBLE				::Engine::Math::Real
#define ENGINE_CXX_TYPE_REAL				::Engine::Math::Real
#define ENGINE_CXX_TYPE_COMPLEX				::Engine::Math::Complex
#define ENGINE_CXX_TYPE_BOOLEAN				::Engine::Boolean
#define ENGINE_CXX_TYPE_STRING				::Engine::ImmutableString
#define ENGINE_CXX_TYPE_COLOR				::Engine::UI::Color
#define ENGINE_CXX_TYPE_TIME				::Engine::Time
#define ENGINE_CXX_TYPE_TEXTURE				::Engine::SafePointer<::Engine::UI::ITexture>
#define ENGINE_CXX_TYPE_FONT				::Engine::SafePointer<::Engine::UI::IFont>
#define ENGINE_CXX_TYPE_APPLICATION			::Engine::SafePointer<::Engine::UI::Template::Shape>
#define ENGINE_CXX_TYPE_DIALOG				::Engine::SafePointer<::Engine::UI::Template::ControlTemplate>
#define ENGINE_CXX_TYPE_RECTANGLE			::Engine::UI::Rectangle

#define ENGINE_REFLECTED_TYPE_UINT8			::Engine::Reflection::PropertyType::UInt8
#define ENGINE_REFLECTED_TYPE_BYTE			::Engine::Reflection::PropertyType::Byte
#define ENGINE_REFLECTED_TYPE_INT8			::Engine::Reflection::PropertyType::Int8
#define ENGINE_REFLECTED_TYPE_UINT16		::Engine::Reflection::PropertyType::UInt16
#define ENGINE_REFLECTED_TYPE_INT16			::Engine::Reflection::PropertyType::Int16
#define ENGINE_REFLECTED_TYPE_UINT32		::Engine::Reflection::PropertyType::UInt32
#define ENGINE_REFLECTED_TYPE_INT32			::Engine::Reflection::PropertyType::Int32
#define ENGINE_REFLECTED_TYPE_INTEGER		::Engine::Reflection::PropertyType::Integer
#define ENGINE_REFLECTED_TYPE_UINT64		::Engine::Reflection::PropertyType::UInt64
#define ENGINE_REFLECTED_TYPE_INT64			::Engine::Reflection::PropertyType::Int64
#define ENGINE_REFLECTED_TYPE_LONGINTEGER	::Engine::Reflection::PropertyType::LongInteger
#define ENGINE_REFLECTED_TYPE_FLOAT			::Engine::Reflection::PropertyType::Float
#define ENGINE_REFLECTED_TYPE_SHORTREAL		::Engine::Reflection::PropertyType::ShortReal
#define ENGINE_REFLECTED_TYPE_DOUBLE		::Engine::Reflection::PropertyType::Double
#define ENGINE_REFLECTED_TYPE_REAL			::Engine::Reflection::PropertyType::Real
#define ENGINE_REFLECTED_TYPE_COMPLEX		::Engine::Reflection::PropertyType::Complex
#define ENGINE_REFLECTED_TYPE_BOOLEAN		::Engine::Reflection::PropertyType::Boolean
#define ENGINE_REFLECTED_TYPE_STRING		::Engine::Reflection::PropertyType::String
#define ENGINE_REFLECTED_TYPE_COLOR			::Engine::Reflection::PropertyType::Color
#define ENGINE_REFLECTED_TYPE_TIME			::Engine::Reflection::PropertyType::Time
#define ENGINE_REFLECTED_TYPE_TEXTURE		::Engine::Reflection::PropertyType::Texture
#define ENGINE_REFLECTED_TYPE_FONT			::Engine::Reflection::PropertyType::Font
#define ENGINE_REFLECTED_TYPE_APPLICATION	::Engine::Reflection::PropertyType::Application
#define ENGINE_REFLECTED_TYPE_DIALOG		::Engine::Reflection::PropertyType::Dialog
#define ENGINE_REFLECTED_TYPE_RECTANGLE		::Engine::Reflection::PropertyType::Rectangle
#define ENGINE_REFLECTED_TYPE_ARRAY			::Engine::Reflection::PropertyType::Array
#define ENGINE_REFLECTED_TYPE_SAFEARRAY		::Engine::Reflection::PropertyType::SafeArray
#define ENGINE_REFLECTED_TYPE_STRUCTURE		::Engine::Reflection::PropertyType::Structure
	}
}