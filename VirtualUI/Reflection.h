#pragma once

#include "EngineBase.h"

namespace Engine
{
	namespace Reflection
	{
		class ReflectableObject;
		class TypeReflection
		{
		public:
			virtual string GetTypeName(void) const = 0;
		};
		class NameReflection
		{
		public:
			virtual string GetName(void) const = 0;
		};
		class PropertyBase
		{
		public:
			PropertyBase(void);
			PropertyBase(const PropertyBase & prop) = delete;
			PropertyBase & operator = (const PropertyBase & prop) = delete;
			virtual ~PropertyBase(void);
			virtual const TypeReflection & Type(void) const = 0;
			virtual const NameReflection & Name(void) const = 0;
			virtual void Get(void * var) const = 0;
			virtual void Set(const void * var) = 0;
		};

		class ReflectableObject : public Object
		{
			template<class V, class N, class T> friend class Property;
			template<class V, class N, class T> friend class ObjectProperty;
			Array<int> _properties;
			void _reflect_property(const string & Name, PropertyBase * refrence);
		public:
			ReflectableObject(void);
			~ReflectableObject(void) override;
			PropertyBase * GetProperty(const string & Name);
			const PropertyBase * GetProperty(const string & Name) const;
			int PropertyCount(void) const;
			PropertyBase * PropertyAt(int Index);
			const PropertyBase * PropertyAt(int Index) const;
		};

		template<class V, class N, class T> class Property final : public PropertyBase
		{
		private:
			V inner;
			N name;
			T type;
		public:
			Property(ReflectableObject * owner) { owner->_reflect_property(N().GetName(), this); }
			Property(const Property & src) = delete;
			~Property(void) override {}
			Property & operator = (const Property & src) { inner = src.inner; return *this; }
			operator V & (void) { return inner; }
			operator const V & (void) const { return inner; }

			virtual const NameReflection & Name(void) const override { return name; }
			virtual const TypeReflection & Type(void) const override { return type; }
			virtual void Get(void * var) const override { *reinterpret_cast<V*>(var) = inner; }
			virtual void Set(const void * var) override { inner = *reinterpret_cast<const V*>(var); }
		};
		template<class V, class N, class T> class ObjectProperty final : public PropertyBase
		{
		private:
			V * inner = 0;
			N name;
			T type;
		public:
			ObjectProperty(ReflectableObject * owner) { owner->_reflect_property(N().GetName(), this); }
			ObjectProperty(const ObjectProperty & src) = delete;
			~ObjectProperty(void) override { if (inner) inner->Release(); }
			ObjectProperty & operator = (const ObjectProperty & src) { if (this == &src) return *this; if (inner) inner->Release(); inner = src.inner; if (inner) inner->Retain(); return *this; }
			operator V & (void) const { return inner; }

			virtual const NameReflection & Name(void) const override { return name; }
			virtual const TypeReflection & Type(void) const override { return type; }
			virtual void Get(void * var) const override { *reinterpret_cast<V**>(var) = inner; if (inner) inner->Retain(); }
			virtual void Set(const void * var) override { if (inner) inner->Release(); inner = *reinterpret_cast<V**>(var); if (inner) inner->Retain(); }
		};
#define STRING_MACRO(S) #S
#define __NAME_REFLECTION(N) template<int> class __##N##PropertyName final : public NameReflection { public: __##N##PropertyName(void) {} virtual string GetName(void) const override { return L"" ## STRING_MACRO(N) ## ""; } }
#define __TYPE_REFLECTION(T) template<int> class __##T##Type final : public TypeReflection { public: __##T##Type(void) {} virtual string GetTypeName(void) const override { return L"" ## STRING_MACRO(T) ## ""; } };
#define DECLARE_PROPERTY(T, N) __NAME_REFLECTION(N); Property<T, __##N##PropertyName<0>, __##T##Type<0>> N = this
#define DECLARE_OBJECT_PROPERTY(T, N) __NAME_REFLECTION(N); ObjectProperty<T, __##N##PropertyName<0>, __##T##Type<0>> N = this

		__TYPE_REFLECTION(int);
		__TYPE_REFLECTION(uint);
		__TYPE_REFLECTION(int64);
		__TYPE_REFLECTION(uint64);
		__TYPE_REFLECTION(int16);
		__TYPE_REFLECTION(uint16);
		__TYPE_REFLECTION(int8);
		__TYPE_REFLECTION(uint8);
		__TYPE_REFLECTION(string);
		__TYPE_REFLECTION(float);
		__TYPE_REFLECTION(double);
		__TYPE_REFLECTION(bool);
	}
}