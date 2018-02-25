#include "EngineBase.h"

namespace Engine
{
	Object::Object(void) : _refcount(1) {}
	uint Object::Retain(void) { return InterlockedIncrement(_refcount); }
	uint Object::Release(void)
	{
		uint result = InterlockedDecrement(_refcount);
		if (result == 0) delete this;
		return result;
	}
	Object::~Object(void) {}
	ImmutableString Object::ToString(void) const { return ImmutableString(L"Object"); }

	Exception::Exception(void) {}
	Exception::Exception(const Exception & e) {}
	Exception::Exception(Exception && e) {}
	Exception & Exception::operator=(const Exception & e) { return *this; }
	ImmutableString Exception::ToString(void) const { return ImmutableString(L"Exception"); }
	ImmutableString OutOfMemoryException::ToString(void) const { return ImmutableString(L"OutOfMemoryException"); }
	ImmutableString InvalidArgumentException::ToString(void) const { return ImmutableString(L"InvalidArgumentException"); }
	ImmutableString InvalidFormatException::ToString(void) const { return ImmutableString(L"InvalidFormatException"); }

	ImmutableString::ImmutableString(void) { text = new (std::nothrow) widechar[1]; if (!text) throw OutOfMemoryException(); text[0] = 0; }
	ImmutableString::ImmutableString(const ImmutableString & src) { text = new (std::nothrow) widechar[src.Length() + 1]; if (!text) throw OutOfMemoryException(); StringCopy(text, src.text); }
	ImmutableString::ImmutableString(ImmutableString && src) { text = src.text; src.text = 0; }
	ImmutableString::ImmutableString(const widechar * src) { text = new (std::nothrow) widechar[StringLength(src) + 1]; if (!text) throw OutOfMemoryException(); StringCopy(text, src); }
	ImmutableString::ImmutableString(const widechar * sequence, int length) { text = new (std::nothrow) widechar[length + 1]; if (!text) throw OutOfMemoryException(); text[length] = 0; MemoryCopy(text, sequence, sizeof(widechar) * length); }
	ImmutableString::ImmutableString(int src)
	{
		widechar * conv = new (std::nothrow) widechar[0x10];
		if (!conv) throw OutOfMemoryException();
		conv[0] = 0;
		if (src == 0xFFFFFFFF) {
			StringCopy(conv, L"-2147483648");
		} else {
			bool neg = false;
			if (src < 0) {
				src = -src; neg = true;
			}
			do {
				int r = src % 10;
				src /= 10;
				StringAppend(conv, L"0123456789"[r]);
			} while (src);
			if (neg) StringAppend(conv, L'-');
		}
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(uint src)
	{
		widechar * conv = new (std::nothrow) widechar[0x10];
		if (!conv) throw OutOfMemoryException();
		conv[0] = 0;
		do {
			int r = src % 10;
			src /= 10;
			StringAppend(conv, L"0123456789"[r]);
		} while (src);
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(int64 src)
	{
		widechar * conv = new (std::nothrow) widechar[0x20];
		if (!conv) throw OutOfMemoryException();
		conv[0] = 0;
		if (src == 0xFFFFFFFFFFFFFFFF) {
			StringCopy(conv, L"-9223372036854775808");
		} else {
			bool neg = false;
			if (src < 0) {
				src = -src; neg = true;
			}
			do {
				int r = src % 10;
				src /= 10;
				StringAppend(conv, L"0123456789"[r]);
			} while (src);
			if (neg) StringAppend(conv, L'-');
		}
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(uint64 src)
	{
		widechar * conv = new (std::nothrow) widechar[0x20];
		if (!conv) throw OutOfMemoryException();
		conv[0] = 0;
		do {
			int r = src % 10;
			src /= 10;
			StringAppend(conv, L"0123456789"[r]);
		} while (src);
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(uint value, const ImmutableString & digits)
	{
		widechar * conv = new (std::nothrow) widechar[0x40];
		if (!conv) throw OutOfMemoryException();
		conv[0] = 0;
		int radix = digits.Length();
		if (radix <= 1) { delete[] conv; throw InvalidArgumentException(); }
		do {
			int r = value % radix;
			value /= radix;
			StringAppend(conv, digits[r]);
		} while (value);
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(uint64 value, const ImmutableString & digits)
	{
		widechar * conv = new (std::nothrow) widechar[0x80];
		if (!conv) throw OutOfMemoryException();
		conv[0] = 0;
		int radix = digits.Length();
		if (radix <= 1) { delete[] conv; throw InvalidArgumentException(); }
		do {
			int r = value % radix;
			value /= radix;
			StringAppend(conv, digits[r]);
		} while (value);
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(const void * src) : ImmutableString(intptr(src), L"0123456789ABCDEF") {}
	ImmutableString::ImmutableString(const void * Sequence, int Length, Encoding SequenceEncoding)
	{
		int len = MeasureSequenceLength(Sequence, Length, SequenceEncoding, SystemEncoding);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) throw OutOfMemoryException();
		ConvertEncoding(text, Sequence, Length, SequenceEncoding, SystemEncoding);
		text[len] = 0;
	}
	ImmutableString::ImmutableString(float src, widechar separator)
	{
		throw Exception();
#pragma message ("METHOD NOT IMPLEMENTED, IMPLEMENT IT!")
	}
	ImmutableString::ImmutableString(double src, widechar separator)
	{
		throw Exception();
#pragma message ("METHOD NOT IMPLEMENTED, IMPLEMENT IT!")
	}
	ImmutableString::ImmutableString(bool src) : ImmutableString(src ? L"true" : L"false") {}
	ImmutableString::ImmutableString(widechar src) { text = new (std::nothrow) widechar[2]; if (!text) throw OutOfMemoryException(); text[0] = src; text[1] = 0; }
	ImmutableString::ImmutableString(const Object * object) : ImmutableString(object->ToString()) {}
	ImmutableString::~ImmutableString(void) { delete[] text; }
	ImmutableString & ImmutableString::operator=(const ImmutableString & src)
	{
		if (this == &src) return *this;
		widechar * alloc = new (std::nothrow) widechar[src.Length() + 1];
		if (!alloc) throw new OutOfMemoryException();
		delete[] text; text = alloc;
		StringCopy(text, src.text);
		return *this;
	}
	ImmutableString & ImmutableString::operator=(const widechar * src)
	{
		if (text == src) return *this;
		widechar * alloc = new (std::nothrow) widechar[StringLength(src) + 1];
		if (!alloc) throw new OutOfMemoryException();
		delete[] text; text = alloc;
		StringCopy(text, src);
		return *this;
	}
	ImmutableString::operator const widechar*(void) const { return text; }
	int ImmutableString::Length(void) const { return StringLength(text); }
	int ImmutableString::Compare(const ImmutableString & a, const ImmutableString & b) { return StringCompare(a.text, b.text); }
	int ImmutableString::CompareIgnoreCase(const ImmutableString & a, const ImmutableString & b) { return StringCompareCaseInsensitive(a.text, b.text); }
	widechar ImmutableString::operator[](int index) const { return text[index]; }
	widechar ImmutableString::CharAt(int index) const { return text[index]; }
	ImmutableString ImmutableString::ToString(void) const { return *this; }
	void ImmutableString::Concatenate(const ImmutableString & str)
	{
		int len = Length();
		widechar * alloc = new (std::nothrow) widechar[len + str.Length() + 1];
		if (!alloc) throw OutOfMemoryException();
		StringCopy(alloc, text);
		StringCopy(alloc + len, str.text);
		delete[] text;
		text = alloc;
	}
	ImmutableString & ImmutableString::operator+=(const ImmutableString & str) { Concatenate(str); return *this; }
	int ImmutableString::FindFirst(widechar letter) const { for (int i = 0; i < Length(); i++) if (text[i] == letter) return i; return -1; }
	int ImmutableString::FindFirst(const ImmutableString & str) const
	{
		int len = str.Length();
		if (!len) throw InvalidArgumentException();
		for (int i = 0; i < Length() - len + 1; i++) if (SequenceCompare(text + i, str.text, len) == 0) return i;
		return -1;
	}
	int ImmutableString::FindLast(widechar letter) const { for (int i = Length() - 1; i >= 0; i--) if (text[i] == letter) return i; return -1; }
	int ImmutableString::FindLast(const ImmutableString & str) const
	{
		int len = str.Length();
		if (!len) throw InvalidArgumentException();
		for (int i = Length() - len; i >= 0; i--) if (SequenceCompare(text + i, str.text, len) == 0) return i;
		return -1;
	}
	ImmutableString ImmutableString::Fragment(int PosStart, int CharLength) const
	{
		int len = Length();
		if (PosStart < 0 || PosStart > len || !CharLength) return L"";
		if (CharLength < -1) return L"";
		if (PosStart + CharLength > len || CharLength == -1) CharLength = len - PosStart;
		return ImmutableString(text + PosStart, CharLength);
	}
	int ImmutableString::GeneralizedFindFirst(int from, const widechar * seq, int length) const
	{
		for (int i = from; i < Length() - length + 1; i++) if (SequenceCompare(text + i, seq, length) == 0) return i;
		return -1;
	}
	ImmutableString ImmutableString::GeneralizedReplace(const widechar * seq, int length, const widechar * with, int withlen) const
	{
		if (!length) throw InvalidArgumentException();
		ImmutableString result;
		ImmutableString With(with, withlen);
		int pos = 0, len = Length();
		while (pos < len) {
			int next = GeneralizedFindFirst(pos, seq, length);
			if (next != -1) {
				int copylen = next - pos;
				if (copylen) result += Fragment(pos, copylen);
				result += With;
				pos = next + length;
			} else {
				result += Fragment(pos, len - pos);
				len = pos;
			}
		}
		return result;
	}
	ImmutableString ImmutableString::Replace(const ImmutableString & Substring, const ImmutableString & with) const { return GeneralizedReplace(Substring, Substring.Length(), with, with.Length()); }
	ImmutableString ImmutableString::Replace(widechar Substring, const ImmutableString & with) const { return GeneralizedReplace(&Substring, 1, with, with.Length()); }
	ImmutableString ImmutableString::Replace(const ImmutableString & Substring, widechar with) const { return GeneralizedReplace(Substring, Substring.Length(), &with, 1); }
	ImmutableString ImmutableString::Replace(widechar Substring, widechar with) const { return GeneralizedReplace(&Substring, 1, &with, 1); }
	ImmutableString ImmutableString::LowerCase(void) const { ImmutableString result = *this; StringLower(result.text, Length()); return result; }
	ImmutableString ImmutableString::UpperCase(void) const { ImmutableString result = *this; StringUpper(result.text, Length()); return result; }
	int ImmutableString::GetEncodedLength(Encoding encoding) const { return MeasureSequenceLength(text, Length(), SystemEncoding, encoding); }
	void ImmutableString::Encode(void * buffer, Encoding encoding, bool include_terminator) const { ConvertEncoding(buffer, text, Length() + (include_terminator ? 1 : 0), SystemEncoding, encoding); }
	Array<uint8>* ImmutableString::EncodeSequence(Encoding encoding, bool include_terminator) const
	{
		int char_length = GetEncodedLength(encoding) + (include_terminator ? 1 : 0);
		int src_length = Length() + (include_terminator ? 1 : 0);
		Array<uint8> * result = new Array<uint8>;
		try {
			result->SetLength(char_length);
			ConvertEncoding(*result, text, src_length, SystemEncoding, encoding);
		}
		catch (...) { result->Release(); throw; }
		return result;
	}
	Array<ImmutableString> ImmutableString::Split(widechar divider) const
	{
		Array<ImmutableString> result;
		int pos = -1, len = Length();
		while (pos < len) {
			int div = GeneralizedFindFirst(pos + 1, &divider, 1);
			pos++;
			if (div == -1) {
				result << Fragment(pos, len - pos);
				pos = len;
			} else {
				result << Fragment(pos, div - pos);
				pos = div;
			}
		}
		return result;
	}
	bool operator==(const ImmutableString & a, const ImmutableString & b) { return ImmutableString::Compare(a, b) == 0; }
	bool operator==(const widechar * a, const ImmutableString & b) { return StringCompare(a, b) == 0; }
	bool operator==(const ImmutableString & a, const widechar * b) { return StringCompare(a, b) == 0; }
	bool operator!=(const ImmutableString & a, const ImmutableString & b) { return ImmutableString::Compare(a, b) != 0; }
	bool operator!=(const widechar * a, const ImmutableString & b) { return StringCompare(a, b) != 0; }
	bool operator!=(const ImmutableString & a, const widechar * b) { return StringCompare(a, b) != 0; }
	bool operator<=(const ImmutableString & a, const ImmutableString & b) { return StringCompare(a, b) <= 0; }
	bool operator>=(const ImmutableString & a, const ImmutableString & b) { return StringCompare(a, b) >= 0; }
	bool operator<(const ImmutableString & a, const ImmutableString & b) { return StringCompare(a, b) < 0; }
	bool operator>(const ImmutableString & a, const ImmutableString & b) { return StringCompare(a, b) > 0; }
	ImmutableString operator+(const ImmutableString & a, const ImmutableString & b) { ImmutableString result = a; return result += b; }
	ImmutableString operator+(const widechar * a, const ImmutableString & b) { return ImmutableString(a) + b; }
	ImmutableString operator+(const ImmutableString & a, const widechar * b) { return a + ImmutableString(b); }
}