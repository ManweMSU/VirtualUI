#pragma once

#include "PlatformDependent/Base.h"

#include <new>

namespace Engine
{
	class Object;
	class ImmutableString;
	template <class V> class Array;
	template <class V> class SafeArray;
	template <class V> class ObjectArray;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//                    Retain/Release Policy
	// Function must call Retain() on object pointer if:
	//  - this object is an input argument and function wants
	//    to store it's reference,
	//  - object reference is the result value of this function,
	//    function assumes object's creation, but object was
	//    taken from another place (for example, from cache)
	//  Function mush call Release() on object pointer if is is
	//  not required anymore and object's reference was taken as
	//  a result value of "Create" function or was retained
	//  explicitly.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	class Object
	{
	private:
		uint _refcount;
	public:
		Object(void);
		Object(const Object & src) = delete;
		Object & operator = (const Object & src) = delete;
		virtual uint Retain(void);
		virtual uint Release(void);
		virtual ~Object(void);
		virtual ImmutableString ToString(void) const;
		uint GetReferenceCount(void) const;
	};

	class Exception : public Object
	{
	public:
		Exception(void);
		Exception(const Exception & e);
		Exception(Exception && e);
		Exception & operator = (const Exception & e);
		ImmutableString ToString(void) const override;
	};
	class OutOfMemoryException : public Exception { public: ImmutableString ToString(void) const override; };
	class InvalidArgumentException : public Exception { public: ImmutableString ToString(void) const override; };
	class InvalidFormatException : public Exception { public: ImmutableString ToString(void) const override; };
	class InvalidStateException : public Exception { public: ImmutableString ToString(void) const override; };

	class ImmutableString final : public Object
	{
	private:
		widechar * text;
		int GeneralizedFindFirst(int from, const widechar * seq, int length) const;
		ImmutableString GeneralizedReplace(const widechar * seq, int length, const widechar * with, int withlen) const;
		uint64 GeneralizedToUInt64(int dfrom, int dto) const;
		uint64 GeneralizedToUInt64(int dfrom, int dto, const ImmutableString & digits, bool case_sensitive) const;
	public:
		ImmutableString(void);
		ImmutableString(const ImmutableString & src);
		ImmutableString(ImmutableString && src);
		ImmutableString(const widechar * src);
		ImmutableString(const widechar * sequence, int length);
		ImmutableString(int32 src);
		ImmutableString(uint32 src);
		ImmutableString(int64 src);
		ImmutableString(uint64 src);
		ImmutableString(uint32 value, const ImmutableString & digits, int minimal_length = 0);
		ImmutableString(uint64 value, const ImmutableString & digits, int minimal_length = 0);
		ImmutableString(const void * Sequence, int Length, Encoding SequenceEncoding);
		ImmutableString(float src, widechar separator = L'.');
		ImmutableString(double src, widechar separator = L'.');
		ImmutableString(bool src);
		ImmutableString(widechar src);
		ImmutableString(widechar src, int repeats);
		explicit ImmutableString(const Object * object);
		explicit ImmutableString(const void * src);
		~ImmutableString(void) override;

		ImmutableString & operator = (const ImmutableString & src);
		ImmutableString & operator = (const widechar * src);
		operator const widechar * (void) const;
		int Length(void) const;

		bool friend operator == (const ImmutableString & a, const ImmutableString & b);
		bool friend operator == (const widechar * a, const ImmutableString & b);
		bool friend operator == (const ImmutableString & a, const widechar * b);
		bool friend operator != (const ImmutableString & a, const ImmutableString & b);
		bool friend operator != (const widechar * a, const ImmutableString & b);
		bool friend operator != (const ImmutableString & a, const widechar * b);

		bool friend operator <= (const ImmutableString & a, const ImmutableString & b);
		bool friend operator >= (const ImmutableString & a, const ImmutableString & b);
		bool friend operator < (const ImmutableString & a, const ImmutableString & b);
		bool friend operator > (const ImmutableString & a, const ImmutableString & b);

		static int Compare(const ImmutableString & a, const ImmutableString & b);
		static int CompareIgnoreCase(const ImmutableString & a, const ImmutableString & b);

		widechar operator [] (int index) const;
		widechar CharAt(int index) const;

		ImmutableString ToString(void) const override;

		virtual void Concatenate(const ImmutableString & str);
		
		ImmutableString friend operator + (const ImmutableString & a, const ImmutableString & b);
		ImmutableString friend operator + (const widechar * a, const ImmutableString & b);
		ImmutableString friend operator + (const ImmutableString & a, const widechar * b);
		ImmutableString & operator += (const ImmutableString & str);

		int FindFirst(widechar letter) const;
		int FindFirst(const ImmutableString & str) const;
		int FindLast(widechar letter) const;
		int FindLast(const ImmutableString & str) const;

		ImmutableString Fragment(int PosStart, int CharLength) const;
		ImmutableString Replace(const ImmutableString & Substring, const ImmutableString & with) const;
		ImmutableString Replace(widechar Substring, const ImmutableString & with) const;
		ImmutableString Replace(const ImmutableString & Substring, widechar with) const;
		ImmutableString Replace(widechar Substring, widechar with) const;

		ImmutableString LowerCase(void) const;
		ImmutableString UpperCase(void) const;

		int GetEncodedLength(Encoding encoding) const;
		void Encode(void * buffer, Encoding encoding, bool include_terminator) const;
		Array<uint8> * EncodeSequence(Encoding encoding, bool include_terminator) const;
		Array<ImmutableString> Split(widechar divider) const;

		uint64 ToUInt64(void) const;
		uint64 ToUInt64(const ImmutableString & digits, bool case_sensitive = false) const;
		int64 ToInt64(void) const;
		int64 ToInt64(const ImmutableString & digits, bool case_sensitive = false) const;
		uint32 ToUInt32(void) const;
		uint32 ToUInt32(const ImmutableString & digits, bool case_sensitive = false) const;
		int32 ToInt32(void) const;
		int32 ToInt32(const ImmutableString & digits, bool case_sensitive = false) const;
		float ToFloat(void) const;
		float ToFloat(const ImmutableString & separators) const;
		double ToDouble(void) const;
		double ToDouble(const ImmutableString & separators) const;
		bool ToBoolean(void) const;
	};

	typedef ImmutableString string;

	template <class V> void swap(V & a, V & b) { if (&a == &b) return; uint8 buffer[sizeof(V)]; MemoryCopy(buffer, &a, sizeof(V)); MemoryCopy(&a, &b, sizeof(V)); MemoryCopy(&b, buffer, sizeof(V)); }
	template <class V> void safe_swap(V & a, V & b) { V e = a; a = b; b = e; }

	template <class V> V min(V a, V b) { return (a < b) ? a : b; }
	template <class V> V max(V a, V b) { return (a < b) ? b : a; }

	double sgn(double x);
	float sgn(float x);
	double saturate(double x);
	float saturate(float x);

	template <class V> V lerp(const V & a, const V & b, double t) { return t * (b - a) + a; }

	template <class V> class Array : public Object
	{
		V * data;
		int count;
		int allocated;
		int block;
		int block_align(int element_count) { return ((int64(element_count) + block - 1) / block) * block; }
		void require(int elements, bool nothrow = false)
		{
			int new_size = block_align(elements);
			if (new_size != allocated) {
				if (new_size > allocated) {
					V * new_data = reinterpret_cast<V*>(realloc(data, sizeof(V) * new_size));
					if (new_data || new_size == 0) {
						data = new_data;
						allocated = new_size;
					} else if (!nothrow) throw OutOfMemoryException();
				} else {
					V * new_data = reinterpret_cast<V*>(realloc(data, sizeof(V) * new_size));
					if (new_data || new_size == 0) {
						data = new_data;
						allocated = new_size;
					}
				}
			}
		}
		void append(const V & v) { new (data + count) V(v); count++; }
	public:
		Array(void) : count(0), allocated(0), data(0), block(0x400) {}
		Array(const Array & src) : count(src.count), allocated(0), data(0), block(src.block)
		{
			require(count); int i = 0;
			try { for (i = 0; i < count; i++) new (data + i) V(src.data[i]); }
			catch (...) {
				for (int j = i - 1; j >= 0; j--) data[i].V::~V();
				free(data); throw;
			}
		}
		Array(Array && src) : count(src.count), allocated(src.allocated), data(src.data), block(src.block) { src.data = 0; src.allocated = 0; src.count = 0; }
		explicit Array(int BlockSize) : count(0), allocated(0), data(0), block(BlockSize) {}
		~Array(void) override { for (int i = 0; i < count; i++) data[i].V::~V(); free(data); }

		Array & operator = (const Array & src)
		{
			if (this == &src) return *this;
			Array Copy(src.block);
			Copy.require(src.count);
			for (int i = 0; i < src.count; i++) Copy.append(src.data[i]);
			for (int i = 0; i < count; i++) data[i].V::~V();
			free(data);
			data = Copy.data; count = Copy.count; allocated = Copy.allocated; block = Copy.block;
			Copy.data = 0; Copy.count = 0; Copy.allocated = 0;
			return *this;
		}

		virtual void Append(const V & v) { require(count + 1); append(v); }
		virtual void Append(const Array & src) { if (&src == this) throw InvalidArgumentException(); require(count + src.count); for (int i = 0; i < src.count; i++) append(src.data[i]); }
		virtual void Append(const V * v, int Count) { if (!Count) return; if (data == v) throw InvalidArgumentException(); require(count + Count); for (int i = 0; i < Count; i++) append(v[i]); }
		virtual void SwapAt(int i, int j) { swap(data[i], data[j]); }
		virtual void Insert(const V & v, int IndexAt)
		{
			require(count + 1);
			for (int i = count - 1; i >= IndexAt; i--) swap(data[i], data[i + 1]);
			try { new (data + IndexAt) V(v); count++; }
			catch (...) { for (int i = IndexAt; i < count; i++) swap(data[i], data[i + 1]); throw; }
		}
		virtual V & ElementAt(int index) { return data[index]; }
		virtual const V & ElementAt(int index) const { return data[index]; }
		virtual V & FirstElement(void) { return data[0]; }
		virtual const V & FirstElement(void) const { return data[0]; }
		virtual V & LastElement(void) { return data[count - 1]; }
		virtual const V & LastElement(void) const { return data[count - 1]; }
		virtual void Remove(int index) { data[index].V::~V(); for (int i = index; i < count - 1; i++) swap(data[i], data[i + 1]); count--; require(count, true); }
		virtual void RemoveFirst(void) { Remove(0); }
		virtual void RemoveLast(void) { Remove(count - 1); }
		virtual void Clear(void) { for (int i = 0; i < count; i++) data[i].V::~V(); free(data); data = 0; count = 0; allocated = 0; }
		virtual void SetLength(int length)
		{
			if (length > count) {
				require(length); int i;
				try { for (i = count; i < length; i++) new (data + i) V(); }
				catch (...) { for (int j = i - 1; j >= count; j--) data[i].V::~V(); throw; }
				count = length;
			} else if (length < count) {
				if (length < 0) throw InvalidArgumentException();
				for (int i = length; i < count; i++) data[i].V::~V(); count = length;
				require(count, true);
			}
		}
		int Length(void) const { return count; }
		V * GetBuffer(void) { return data; }
		const V * GetBuffer(void) const { return data; }
		
		string ToString(void) const override { return L"Array[" + string(count) + L"]"; }

		operator V * (void) { return data; }
		operator const V * (void) { return data; }
		Array & operator << (const V & v) { Append(v); return *this; }
		Array & operator << (const Array & src) { Append(src); return *this; }
		V & operator [] (int index) { return data[index]; }
		const V & operator [] (int index) const { return data[index]; }
		bool friend operator == (const Array & a, const Array & b)
		{
			if (a.count != b.count) return false;
			for (int i = 0; i < a.count; i++) if (a[i] != b[i]) return false;
			return true;
		}
		bool friend operator != (const Array & a, const Array & b)
		{
			if (a.count != b.count) return true;
			for (int i = 0; i < a.count; i++) if (a[i] != b[i]) return true;
			return false;
		}
	};

	template <class V> class SafeArray : public Object
	{
		V ** data;
		int count;
		int allocated;
		int block;
		int block_align(int element_count) { return ((int64(element_count) + block - 1) / block) * block; }
		void require(int elements, bool nothrow = false)
		{
			int new_size = block_align(elements);
			if (new_size != allocated) {
				if (new_size > allocated) {
					V ** new_data = reinterpret_cast<V**>(realloc(data, sizeof(V*) * new_size));
					if (new_data || new_size == 0) {
						data = new_data;
						allocated = new_size;
					} else if (!nothrow) throw OutOfMemoryException();
				} else {
					V ** new_data = reinterpret_cast<V**>(realloc(data, sizeof(V*) * new_size));
					if (new_data || new_size == 0) {
						data = new_data;
						allocated = new_size;
					}
				}
			}
		}
		void append(const V & v) { data[count] = new V(v); count++; }
	public:
		SafeArray(void) : count(0), allocated(0), data(0), block(0x400) {}
		SafeArray(const SafeArray & src) : count(src.count), allocated(0), data(0), block(src.block)
		{
			require(count); int i = 0;
			try { for (i = 0; i < count; i++) data[i] = new V(src.data[i]); } catch (...) {
				for (int j = i - 1; j >= 0; j--) delete data[i];
				free(data); throw;
			}
		}
		SafeArray(SafeArray && src) : count(src.count), allocated(src.allocated), data(src.data), block(src.block) { src.data = 0; src.allocated = 0; src.count = 0; }
		explicit SafeArray(int BlockSize) : count(0), allocated(0), data(0), block(BlockSize) {}
		~SafeArray(void) override { for (int i = 0; i < count; i++) delete data[i]; free(data); }

		SafeArray & operator = (const SafeArray & src)
		{
			if (this == &src) return *this;
			SafeArray Copy(src.block);
			Copy.require(src.count);
			for (int i = 0; i < src.count; i++) Copy.append(src.data[i]);
			for (int i = 0; i < count; i++) delete data[i];
			free(data);
			data = Copy.data; count = Copy.count; allocated = Copy.allocated; block = Copy.block;
			Copy.data = 0; Copy.count = 0; Copy.allocated = 0;
			return *this;
		}

		virtual void Append(const V & v) { require(count + 1); append(v); }
		virtual void Append(const SafeArray & src) { if (&src == this) throw InvalidArgumentException(); require(count + src.count); for (int i = 0; i < src.count; i++) append(*src.data[i]); }
		virtual void Append(const V * v, int Count) { require(count + Count); for (int i = 0; i < Count; i++) append(v[i]); }
		virtual void SwapAt(int i, int j) { safe_swap(data[i], data[j]); }
		virtual void Insert(const V & v, int IndexAt)
		{
			require(count + 1);
			for (int i = count - 1; i >= IndexAt; i--) safe_swap(data[i], data[i + 1]);
			try { data[IndexAt] = new V(v); count++; } catch (...) { for (int i = IndexAt; i < count; i++) safe_swap(data[i], data[i + 1]); throw; }
		}
		virtual V & ElementAt(int index) { return *data[index]; }
		virtual const V & ElementAt(int index) const { return *data[index]; }
		virtual V & FirstElement(void) { return *data[0]; }
		virtual const V & FirstElement(void) const { return *data[0]; }
		virtual V & LastElement(void) { return *data[count - 1]; }
		virtual const V & LastElement(void) const { return *data[count - 1]; }
		virtual void Remove(int index) { delete data[index]; for (int i = index; i < count - 1; i++) safe_swap(data[i], data[i + 1]); count--; require(count, true); }
		virtual void RemoveFirst(void) { Remove(0); }
		virtual void RemoveLast(void) { Remove(count - 1); }
		virtual void Clear(void) { for (int i = 0; i < count; i++) delete data[i]; free(data); data = 0; count = 0; allocated = 0; }
		virtual void SetLength(int length)
		{
			if (length > count) {
				require(length); int i;
				try { for (i = count; i < length; i++) data[i] = new V(); } catch (...) { for (int j = i - 1; j >= count; j--) delete data[i]; throw; }
				count = length;
			} else if (length < count) {
				if (length < 0) throw InvalidArgumentException();
				for (int i = length; i < count; i++) delete data[i]; count = length;
				require(count, true);
			}
		}
		int Length(void) const { return count; }

		string ToString(void) const override { return L"SafeArray[" + string(count) + L"]"; }

		SafeArray & operator << (const V & v) { Append(v); return *this; }
		SafeArray & operator << (const SafeArray & src) { Append(src); return *this; }
		V & operator [] (int index) { return *data[index]; }
		const V & operator [] (int index) const { return *data[index]; }
		bool friend operator == (const SafeArray & a, const SafeArray & b)
		{
			if (a.count != b.count) return false;
			for (int i = 0; i < a.count; i++) if (a[i] != b[i]) return false;
			return true;
		}
		bool friend operator != (const SafeArray & a, const SafeArray & b)
		{
			if (a.count != b.count) return true;
			for (int i = 0; i < a.count; i++) if (a[i] != b[i]) return true;
			return false;
		}
	};

	template <class V> class ObjectArray : public Object
	{
		V ** data;
		int count;
		int allocated;
		int block;
		int block_align(int element_count) { return ((int64(element_count) + block - 1) / block) * block; }
		void require(int elements, bool nothrow = false)
		{
			int new_size = block_align(elements);
			if (new_size != allocated) {
				if (new_size > allocated) {
					V ** new_data = reinterpret_cast<V**>(realloc(data, sizeof(V*) * new_size));
					if (new_data || new_size == 0) {
						data = new_data;
						allocated = new_size;
					} else if (!nothrow) throw OutOfMemoryException();
				} else {
					V ** new_data = reinterpret_cast<V**>(realloc(data, sizeof(V*) * new_size));
					if (new_data || new_size == 0) {
						data = new_data;
						allocated = new_size;
					}
				}
			}
		}
		void append(V * v) { data[count] = v; if (v) v->Retain(); count++; }
	public:
		ObjectArray(void) : count(0), allocated(0), data(0), block(0x400) {}
		ObjectArray(const ObjectArray & src) : count(src.count), allocated(0), data(0), block(src.block)
		{
			require(count); int i = 0;
			try { for (i = 0; i < count; i++) { data[i] = src.data[i]; if (src.data[i]) src.data[i]->Retain(); } } catch (...) {
				for (int j = i - 1; j >= 0; j--) if (data[i]) data[i]->Release();
				free(data); throw;
			}
		}
		ObjectArray(ObjectArray && src) : count(src.count), allocated(src.allocated), data(src.data), block(src.block) { src.data = 0; src.allocated = 0; src.count = 0; }
		explicit ObjectArray(int BlockSize) : count(0), allocated(0), data(0), block(BlockSize) {}
		~ObjectArray(void) override { for (int i = 0; i < count; i++) if (data[i]) data[i]->Release(); free(data); }

		ObjectArray & operator = (const ObjectArray & src)
		{
			if (this == &src) return *this;
			ObjectArray Copy(src.block);
			Copy.require(src.count);
			for (int i = 0; i < src.count; i++) Copy.append(src.data[i]);
			for (int i = 0; i < count; i++) if (data[i]) data[i]->Release();
			free(data);
			data = Copy.data; count = Copy.count; allocated = Copy.allocated; block = Copy.block;
			Copy.data = 0; Copy.count = 0; Copy.allocated = 0;
			return *this;
		}

		virtual void Append(V * v) { require(count + 1); append(v); }
		virtual void Append(const ObjectArray & src) { if (&src == this) throw InvalidArgumentException(); require(count + src.count); for (int i = 0; i < src.count; i++) append(src.data[i]); }
		virtual void SwapAt(int i, int j) { safe_swap(data[i], data[j]); }
		virtual void Insert(V * v, int IndexAt)
		{
			require(count + 1);
			for (int i = count - 1; i >= IndexAt; i--) safe_swap(data[i], data[i + 1]);
			try { data[IndexAt] = v; if (v) v->Retain(); count++; } catch (...) { for (int i = IndexAt; i < count; i++) safe_swap(data[i], data[i + 1]); throw; }
		}
		virtual V * ElementAt(int index) const { return data[index]; }
		virtual V * FirstElement(void) const { return data[0]; }
		virtual V * LastElement(void) const { return data[count - 1]; }
		virtual void Remove(int index) { if (data[index]) data[index]->Release(); for (int i = index; i < count - 1; i++) safe_swap(data[i], data[i + 1]); count--; require(count, true); }
		virtual void RemoveFirst(void) { Remove(0); }
		virtual void RemoveLast(void) { Remove(count - 1); }
		virtual void Clear(void) { for (int i = 0; i < count; i++) if (data[i]) data[i]->Release(); free(data); data = 0; count = 0; allocated = 0; }
		virtual void SetElement(V * v, int index) { if (data[index]) data[index]->Release(); data[index] = v; if (v) v->Retain(); }
		int Length(void) const { return count; }

		string ToString(void) const override
		{
			string result = L"ObjectArray[";
			if (count) for (int i = 0; i < count; i++) { result += data[i]->ToString() + ((i == count - 1) ? L"]" : L", "); }
			else result += L"]";
			return result;
		}

		ObjectArray & operator << (V * v) { Append(v); return *this; }
		ObjectArray & operator << (const ObjectArray & src) { Append(src); return *this; }
		V & operator [] (int index) const { return *data[index]; }
		bool friend operator == (const ObjectArray & a, const ObjectArray & b)
		{
			if (a.count != b.count) return false;
			for (int i = 0; i < a.count; i++) if (a[i] != b[i]) return false;
			return true;
		}
		bool friend operator != (const ObjectArray & a, const ObjectArray & b)
		{
			if (a.count != b.count) return true;
			for (int i = 0; i < a.count; i++) if (a[i] != b[i]) return true;
			return false;
		}
	};

	template <class A> void SortArray(A & volume, bool ascending = true)
	{
		int len = volume.Length();
		for (int i = 0; i < len - 1; i++) {
			for (int j = i + 1; j < len; j++) {
				bool swap = false;
				if (ascending) {
					swap = volume[i] > volume[j];
				} else {
					swap = volume[i] < volume[j];
				}
				if (swap) {
					volume.SwapAt(i, j);
				}
			}
		}
	}
	template <class V> int BinarySearchLE(const Array<V> & volume, const V & value)
	{
		if (!volume.Length()) throw InvalidArgumentException();
		int bl = 0, bh = volume.Length();
		while (bh - bl > 1) {
			int mid = (bl + bh) / 2;
			if (volume[mid] > value) { bh = mid; } else { bl = mid; }
		}
		if (bl == 0) {
			if (value < volume[0]) bl = -1;
		}
		return bl;
	}

	template <class O> class SafePointer final : Object
	{
	private:
		O * reference = 0;
	public:
		SafePointer(void) : reference(0) {}
		SafePointer(O * ref) : reference(ref) {}
		SafePointer(const SafePointer & src)
		{
			reference = src.reference;
			if (reference) reference->Retain();
		}
		SafePointer(SafePointer && src) : reference(src.reference) { src.reference = 0; }
		~SafePointer(void) { if (reference) reference->Release(); reference = 0; }
		SafePointer & operator = (const SafePointer & src)
		{
			if (this == &src) return *this;
			if (reference) reference->Release();
			reference = src.reference;
			if (reference) reference->Retain();
			return *this;
		}

		O & operator * (void) const { return *reference; }
		O * operator -> (void) const { return reference; }
		operator O * (void) const { return reference; }
		operator bool (void) const { return reference != 0; }
		O * Inner(void) const { return reference; }
		O ** InnerRef(void) { return &reference; }

		void SetReference(O * ref) { if (reference) reference->Release(); reference = ref; }
		void SetRetain(O * ref) { if (reference) reference->Release(); reference = ref; if (reference) reference->Retain(); }

		bool friend operator == (const SafePointer & a, const SafePointer & b) { return a.reference == b.reference; }
		bool friend operator != (const SafePointer & a, const SafePointer & b) { return a.reference != b.reference; }
	};
}