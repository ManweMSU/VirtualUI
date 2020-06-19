#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Dictionary {
		template <class K, class V> class PlainDictionaryKeyPair
		{
		public:
			K key;
			V value;

			PlainDictionaryKeyPair(void) {}
			PlainDictionaryKeyPair(const K & pair_key, const V & pair_value) : key(pair_key), value(pair_value) {}
			PlainDictionaryKeyPair(const PlainDictionaryKeyPair & src) : key(src.key), value(src.value) {}
			PlainDictionaryKeyPair & operator = (const PlainDictionaryKeyPair & src) { if (this == &src) return *this; key = src.key; value = src.value; return *this; }

			bool friend operator == (const PlainDictionaryKeyPair & a, const PlainDictionaryKeyPair & b) { return a.key == b.key; }
			bool friend operator != (const PlainDictionaryKeyPair & a, const PlainDictionaryKeyPair & b) { return a.key != b.key; }
			bool friend operator < (const PlainDictionaryKeyPair & a, const PlainDictionaryKeyPair & b) { return a.key < b.key; }
			bool friend operator > (const PlainDictionaryKeyPair & a, const PlainDictionaryKeyPair & b) { return a.key > b.key; }
			bool friend operator <= (const PlainDictionaryKeyPair & a, const PlainDictionaryKeyPair & b) { return a.key <= b.key; }
			bool friend operator >= (const PlainDictionaryKeyPair & a, const PlainDictionaryKeyPair & b) { return a.key >= b.key; }
		};
		template <class K, class O> class DictionaryKeyPair
		{
		public:
			K key;
			SafePointer<O> object;

			DictionaryKeyPair(void) {}
			DictionaryKeyPair(const K & object_key, O * object_refrence) : key(object_key), object(object_refrence) { if (object) object->Retain(); }
			DictionaryKeyPair(const DictionaryKeyPair & src) : key(src.key), object(src.object.Inner()) { if (object) object->Retain(); }
			DictionaryKeyPair(DictionaryKeyPair && src) : key(src.key), object(src.object) { src.object.SetReference(0); }
			DictionaryKeyPair & operator = (const DictionaryKeyPair & src) { if (this == &src) return *this; key = src.key; object = src.object; if (object) object->Retain(); return *this; }

			bool friend operator == (const DictionaryKeyPair & a, const DictionaryKeyPair & b) { return a.key == b.key; }
			bool friend operator != (const DictionaryKeyPair & a, const DictionaryKeyPair & b) { return a.key != b.key; }
			bool friend operator < (const DictionaryKeyPair & a, const DictionaryKeyPair & b) { return a.key < b.key; }
			bool friend operator > (const DictionaryKeyPair & a, const DictionaryKeyPair & b) { return a.key > b.key; }
			bool friend operator <= (const DictionaryKeyPair & a, const DictionaryKeyPair & b) { return a.key <= b.key; }
			bool friend operator >= (const DictionaryKeyPair & a, const DictionaryKeyPair & b) { return a.key >= b.key; }
		};
		template <class K, class V> class PlainDictionary : public Object
		{
			Array<PlainDictionaryKeyPair<K, V> > data;
		public:
			PlainDictionary(void) : data(0x400) {}
			explicit PlainDictionary(int BlockSize) : data(BlockSize) {}

			virtual bool Append(const K & key, const V & value)
			{
				if (data.Length()) {
					auto pair = PlainDictionaryKeyPair<K, V>(key, value);
					int element_lesser_equal = BinarySearchLE(data, pair);
					if (element_lesser_equal < 0) data.Insert(pair, 0);
					else if (data[element_lesser_equal] == pair) {
						return false;
					} else {
						if (element_lesser_equal == data.Length() - 1) data.Append(pair);
						else data.Insert(pair, element_lesser_equal + 1);
					}
				} else data.Append(PlainDictionaryKeyPair<K, V>(key, value));
				return true;
			}
			virtual const V * ElementByKey(const K & key) const
			{
				if (!data.Length()) return 0;
				int element = BinarySearchLE(data, PlainDictionaryKeyPair<K, V>(key, 0));
				if (element < 0 || data[element].key != key) return 0;
				return &data[element].value;
			}
			virtual V * ElementByKey(const K & key)
			{
				if (!data.Length()) return 0;
				int element = BinarySearchLE(data, PlainDictionaryKeyPair<K, V>(key, 0));
				if (element < 0 || data[element].key != key) return 0;
				return &data[element].value;
			}
			virtual const PlainDictionaryKeyPair<K, V> & ElementByIndex(int index) const { return data[index]; }
			virtual PlainDictionaryKeyPair<K, V> & ElementByIndex(int index) { return data[index]; }
			virtual void RemoveValue(const V & value)
			{
				for (int i = 0; i < data.Length(); i++) if (data[i].value == value) { data.Remove(i); break; }
			}
			virtual void RemoveByKey(const K & key)
			{
				if (!data.Length()) return;
				int element = BinarySearchLE(data, PlainDictionaryKeyPair<K, V>(key, 0));
				if (element >= 0 && data[element].key == key) data.Remove(element);
			}
			virtual void RemoveAt(int index) { data.Remove(index); }
			virtual void Clear(void) { data.Clear(); }
			bool ElementPresent(const K & key) const
			{
				if (!data.Length()) return false;
				int element = BinarySearchLE(data, PlainDictionaryKeyPair<K, V>(key, 0));
				if (element >= 0 && data[element].key == key) return true; else return false;
			}
			int Length(void) const { return data.Length(); }

			string ToString(void) const override { return L"Dictionary"; }
			const V * operator [] (const K & key) const { return ElementByKey(key); }
			V * operator [] (const K & key) { return ElementByKey(key); }
			const PlainDictionaryKeyPair<K, V> & operator [] (int index) const { return data[index]; }
			PlainDictionaryKeyPair<K, V> & operator [] (int index) { return data[index]; }
		};
		template <class K, class O> class Dictionary : public Object
		{
			Array<DictionaryKeyPair<K, O> > data;
		public:
			Dictionary(void) : data(0x400) {}
			explicit Dictionary(int BlockSize) : data(BlockSize) {}

			virtual bool Append(const K & key, O * object)
			{
				if (data.Length()) {
					auto pair = DictionaryKeyPair<K, O>(key, object);
					int element_lesser_equal = BinarySearchLE(data, pair);
					if (element_lesser_equal < 0) data.Insert(pair, 0);
					else if (data[element_lesser_equal] == pair) {
						return false;
					} else {
						if (element_lesser_equal == data.Length() - 1) data.Append(pair);
						else data.Insert(pair, element_lesser_equal + 1);
					}
				} else data.Append(DictionaryKeyPair<K, O>(key, object));
				return true;
			}
			virtual O * ElementByKey(const K & key) const
			{
				if (!data.Length()) return 0;
				int element = BinarySearchLE(data, DictionaryKeyPair<K, O>(key, 0));
				if (element < 0 || data[element].key != key) return 0;
				return data[element].object;
			}
			virtual const DictionaryKeyPair<K, O> & ElementByIndex(int index) const { return data[index]; }
			virtual DictionaryKeyPair<K, O> & ElementByIndex(int index) { return data[index]; }
			virtual void RemoveObject(O * object)
			{
				for (int i = 0; i < data.Length(); i++) if (data[i].object.Inner() == object) { data.Remove(i); break; }
			}
			virtual void RemoveByKey(const K & key)
			{
				if (!data.Length()) return;
				int element = BinarySearchLE(data, DictionaryKeyPair<K, O>(key, 0));
				if (element >= 0 && data[element].key == key) data.Remove(element);
			}
			virtual void RemoveAt(int index) { data.Remove(index); }
			virtual void Clear(void) { data.Clear(); }
			bool ElementPresent(const K & key) const
			{
				if (!data.Length()) return false;
				int element = BinarySearchLE(data, DictionaryKeyPair<K, O>(key, 0));
				if (element >= 0 && data[element].key == key) return true; else return false;
			}
			bool ElementPresent(O * object) const
			{
				for (int i = 0; i < data.Length(); i++) if (data[i].object.Inner() == object) return true;
				return false;
			}
			int Length(void) const { return data.Length(); }

			string ToString(void) const override { return L"Dictionary"; }
			O * operator [] (const K & key) const { return ElementByKey(key); }
			const DictionaryKeyPair<K, O> & operator [] (int index) const { return data[index]; }
			DictionaryKeyPair<K, O> & operator [] (int index) { return data[index]; }
		};
		enum class ExcludePolicy { DoNotExclude = 0, ExcludeLeastRefrenced = 1 };
		template <class K, class O> class ObjectCache : public Dictionary<K, O>
		{
			int max_capacity;
			ExcludePolicy policy;
		public:
			ObjectCache(void) : max_capacity(0x100), Dictionary<K, O>(0x20), policy(ExcludePolicy::DoNotExclude) {}
			explicit ObjectCache(int capacity) : max_capacity(capacity), Dictionary<K, O>(0x20), policy(ExcludePolicy::DoNotExclude) {}
			explicit ObjectCache(int capacity, ExcludePolicy exclude_policy) : max_capacity(capacity), Dictionary<K, O>(0x20), policy(exclude_policy) {}
			virtual bool Append(const K & key, O * object) override
			{
				if (Dictionary<K, O>::Length() == max_capacity) {
					if (policy == ExcludePolicy::DoNotExclude) return false;
					else if (policy == ExcludePolicy::ExcludeLeastRefrenced) {
						uint least = Dictionary<K, O>::ElementByIndex(0).object->GetReferenceCount();
						int least_index = 0;
						for (int i = 1; i < Dictionary<K, O>::Length(); i++) {
							uint l;
							if ((l = Dictionary<K, O>::ElementByIndex(i).object->GetReferenceCount()) < least) {
								least = l; least_index = i;
							}
						}
						Dictionary<K, O>::RemoveAt(least_index);
						return Dictionary<K, O>::Append(key, object);
					} else return false;
				} else return Dictionary<K, O>::Append(key, object);
			}
			string ToString(void) const override { return L"ObjectCache"; }
		};
	}
}