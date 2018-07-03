#include "EngineBase.h"

namespace Engine
{
	namespace SymbolicMath
	{
		string Summ(const string & a, const string & b)
		{
			int dot1 = a.FindFirst(L'.');
			int dot2 = b.FindFirst(L'.');
			string a1 = (dot1 < dot2) ? string(L'0', dot2 - dot1) + a : a;
			string b1 = (dot2 < dot1) ? string(L'0', dot1 - dot2) + b : b;
			if (a1.Length() > b1.Length()) b1 += string(L'0', a1.Length() - b1.Length());
			else a1 += string(L'0', b1.Length() - a1.Length());
			widechar * s1 = new (std::nothrow) widechar[a1.Length() + 1];
			if (!s1) throw OutOfMemoryException();
			int carry = 0;
			for (int i = a1.Length() - 1; i >= 0; i--) {
				if (a1[i] == L'.') {
					s1[i + 1] = L'.';
				} else {
					int summ = int(a1[i] - L'0') + int(b1[i] - L'0') + carry;
					s1[i + 1] = (summ % 10) + L'0';
					carry = summ / 10;
				}
			}
			s1[0] = L'0' + carry;
			try {
				string s = (s1[0] == L'0') ? string(s1 + 1, a1.Length()) : string(s1, a1.Length() + 1);
				delete[] s1; s1 = 0;
				return s;
			}
			catch (...) { delete[] s1; throw; }
		}
		string DivideByTwo(const string & a)
		{
			widechar * s1 = new (std::nothrow) widechar[a.Length() + 1];
			if (!s1) throw OutOfMemoryException();
			int carry = 0;
			for (int i = 0; i < a.Length(); i++) {
				if (a[i] == L'.') {
					s1[i] = L'.';
				} else {
					int value = carry * 10 + int(a[i] - L'0');
					s1[i] = L'0' + (value / 2);
					carry = value % 2;
				}
			}
			if (carry) s1[a.Length()] = L'5';
			try {
				string s = (carry) ? string(s1, a.Length() + 1) : string(s1, a.Length());
				delete[] s1; s1 = 0;
				return s;
			}
			catch (...) { delete[] s1; throw; }
		}
		string TwoPower(int power)
		{
			if (power > 0) {
				int len = power / 2 + 5;
				widechar * s1 = new (std::nothrow) widechar[len];
				if (!s1) throw OutOfMemoryException();
				for (int i = 0; i < len - 3; i++) s1[i] = L'0';
				s1[len - 3] = L'1'; s1[len - 2] = L'.'; s1[len - 1] = L'0';
				int carry, nsig = len - 3;
				for (int j = 0; j < power; j++) {
					carry = 0;
					for (int i = len - 3; i >= 0; i--) {
						int value = int(s1[i] - L'0') * 2 + carry;
						s1[i] = L'0' + (value % 10);
						carry = value / 10;
						if (!carry && i <= nsig) { nsig = i; break; }
					}
				}
				try {
					string s = string(s1, len);
					delete[] s1; s1 = 0;
					return s;
				} catch (...) { delete[] s1; throw; }
			} else if (power < 0) {
				string base = L"1.0";
				for (int i = 0; i < -power; i++) base = DivideByTwo(base);
				return base;
			} else return L"1.0";
		}
		string Round(const string & a, int signchars, widechar separator)
		{
			int fnz = 0;
			while (a[fnz] == L'0' || a[fnz] == L'.') fnz++;
			int truncf = fnz;
			for (int i = 0; i < signchars; i++) {
				truncf++;
				if (truncf >= a.Length()) break;
				if (a[truncf] == L'.') truncf++;
			}
			int len = truncf + 1;
			int dot = a.FindFirst(L'.');
			int ll;
			if (truncf > dot) ll = len; else ll = len + (dot - truncf) + 1;
			widechar * s1 = new (std::nothrow) widechar[ll];
			if (!s1) throw OutOfMemoryException();
			for (int i = len; i < ll; i++) s1[i] = L'0';
			if (len != ll) s1[ll - 1] = L'.';
			int carry = ((truncf < a.Length()) && (a[truncf] >= L'5')) ? 1 : 0;
			if (carry) {
				for (int i = len - 1; i >= 1; i--) {
					if (a[i - 1] == L'.') {
						s1[i] = L'.';
					} else {
						int value = int(a[i - 1] - L'0') + carry;
						s1[i] = L'0' + (value % 10);
						carry = value / 10;
					}
				}
				s1[0] = L'0' + carry;
			} else {
				for (int i = 1; i < len; i++) s1[i] = a[i - 1];
				s1[0] = L'0';
			}
			int begin = 0;
			while (s1[begin] == L'0') begin++;
			if (s1[begin] == L'.') begin--;
			int len2 = ll - 1;
			while (s1[len2] == L'0') len2--;
			if (s1[len2] == L'.') len2--;
			for (int i = 0; i < ll; i++) if (s1[i] == L'.') { s1[i] = separator; break; }
			try {
				string s = string(s1 + begin, len2 + 1 - begin);
				delete[] s1; s1 = 0;
				return s;
			} catch (...) { delete[] s1; throw; }
		}
	}

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
	uint Object::GetReferenceCount(void) const { return _refcount; }

	Exception::Exception(void) {}
	Exception::Exception(const Exception & e) {}
	Exception::Exception(Exception && e) {}
	Exception & Exception::operator=(const Exception & e) { return *this; }
	ImmutableString Exception::ToString(void) const { return ImmutableString(L"Exception"); }
	ImmutableString OutOfMemoryException::ToString(void) const { return ImmutableString(L"OutOfMemoryException"); }
	ImmutableString InvalidArgumentException::ToString(void) const { return ImmutableString(L"InvalidArgumentException"); }
	ImmutableString InvalidFormatException::ToString(void) const { return ImmutableString(L"InvalidFormatException"); }
	ImmutableString InvalidStateException::ToString(void) const { return ImmutableString(L"InvalidStateException"); }

	ImmutableString::ImmutableString(void) { text = new (std::nothrow) widechar[1]; if (!text) throw OutOfMemoryException(); text[0] = 0; }
	ImmutableString::ImmutableString(const ImmutableString & src) { text = new (std::nothrow) widechar[src.Length() + 1]; if (!text) throw OutOfMemoryException(); StringCopy(text, src.text); }
	ImmutableString::ImmutableString(ImmutableString && src) { text = src.text; src.text = 0; }
	ImmutableString::ImmutableString(const widechar * src) { text = new (std::nothrow) widechar[StringLength(src) + 1]; if (!text) throw OutOfMemoryException(); StringCopy(text, src); }
	ImmutableString::ImmutableString(const widechar * sequence, int length) { text = new (std::nothrow) widechar[length + 1]; if (!text) throw OutOfMemoryException(); text[length] = 0; MemoryCopy(text, sequence, sizeof(widechar) * length); }
	ImmutableString::ImmutableString(int32 src)
	{
		widechar * conv = new (std::nothrow) widechar[0x10];
		if (!conv) throw OutOfMemoryException();
		conv[0] = 0;
		bool rev = true;
		if (src == 0x80000000) {
			rev = false;
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
		if (rev) for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		else for (int i = 0; i < len; i++) text[i] = conv[i];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(uint32 src)
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
		bool rev = true;
		if (src == 0x8000000000000000) {
			rev = false;
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
		if (rev) for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		else for (int i = 0; i < len; i++) text[i] = conv[i];
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
	ImmutableString::ImmutableString(uint32 value, const ImmutableString & digits, int minimal_length)
	{
		if (minimal_length < 0 || minimal_length > 0x20) throw InvalidArgumentException();
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
		while (StringLength(conv) < minimal_length) StringAppend(conv, digits[0]);
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(uint64 value, const ImmutableString & digits, int minimal_length)
	{
		if (minimal_length < 0 || minimal_length > 0x40) throw InvalidArgumentException();
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
		while (StringLength(conv) < minimal_length) StringAppend(conv, digits[0]);
		int len = StringLength(conv);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) { delete[] conv; throw OutOfMemoryException(); }
		for (int i = 0; i < len; i++) text[i] = conv[len - i - 1];
		text[len] = 0;
		delete[] conv;
	}
	ImmutableString::ImmutableString(const void * src) : ImmutableString(intptr(src), L"0123456789ABCDEF", sizeof(void*) * 2) {}
	ImmutableString::ImmutableString(const void * Sequence, int Length, Encoding SequenceEncoding)
	{
		int len = MeasureSequenceLength(Sequence, Length, SequenceEncoding, SystemEncoding);
		text = new (std::nothrow) widechar[len + 1];
		if (!text) throw OutOfMemoryException();
		ConvertEncoding(text, Sequence, Length, SequenceEncoding, SystemEncoding);
		text[len] = 0;
	}
	ImmutableString::ImmutableString(float src, widechar separator) : ImmutableString()
	{
		uint32 & value = reinterpret_cast<uint32&>(src);
		bool negative = (value & 0x80000000) != 0;
		int exp = (value & 0x7F800000) >> 23;
		value &= 0x007FFFFF;
		if (exp == 0) {
			if (value == 0) *this = L"0";
			else {
				string power = SymbolicMath::TwoPower(-126);
				string base = L"0.0";
				for (int i = 22; i >= 0; i--) {
					power = SymbolicMath::DivideByTwo(power);
					if ((value >> i) & 1) base = SymbolicMath::Summ(base, power);
				}
				base = SymbolicMath::Round(base, 7, separator);
				*this = (negative) ? (L"-" + base) : base;
			}
		} else if (exp == 0xFF) {
			if (value == 0) *this = (negative) ? L"-\x221E" : L"+\x221E";
			else *this = L"NaN";
		} else {
			exp -= 127;
			string power = SymbolicMath::TwoPower(exp);
			string base = power;
			for (int i = 22; i >= 0; i--) {
				power = SymbolicMath::DivideByTwo(power);
				if ((value >> i) & 1) base = SymbolicMath::Summ(base, power);
			}
			base = SymbolicMath::Round(base, 7, separator);
			*this = (negative) ? (L"-" + base) : base;
		}
	}
	ImmutableString::ImmutableString(double src, widechar separator) : ImmutableString()
	{
		uint64 & value = reinterpret_cast<uint64&>(src);
		bool negative = (value & 0x8000000000000000) != 0;
		int exp = (value & 0x7FF0000000000000) >> 52;
		value &= 0x000FFFFFFFFFFFFF;
		if (exp == 0) {
			if (value == 0) *this = L"0";
			else {
				string power = SymbolicMath::TwoPower(-1022);
				string base = L"0.0";
				for (int i = 51; i >= 0; i--) {
					power = SymbolicMath::DivideByTwo(power);
					if ((value >> i) & 1) base = SymbolicMath::Summ(base, power);
				}
				base = SymbolicMath::Round(base, 16, separator);
				*this = (negative) ? (L"-" + base) : base;
			}
		} else if (exp == 0x7FF) {
			if (value == 0) *this = (negative) ? L"-\x221E" : L"+\x221E";
			else *this = L"NaN";
		} else {
			exp -= 1023;
			string power = SymbolicMath::TwoPower(exp);
			string base = power;
			for (int i = 51; i >= 0; i--) {
				power = SymbolicMath::DivideByTwo(power);
				if ((value >> i) & 1) base = SymbolicMath::Summ(base, power);
			}
			base = SymbolicMath::Round(base, 16, separator);
			*this = (negative) ? (L"-" + base) : base;
		}
	}
	ImmutableString::ImmutableString(bool src) : ImmutableString(src ? L"true" : L"false") {}
	ImmutableString::ImmutableString(widechar src) { text = new (std::nothrow) widechar[2]; if (!text) throw OutOfMemoryException(); text[0] = src; text[1] = 0; }
	ImmutableString::ImmutableString(widechar src, int repeats)
	{
		if (repeats < 0) throw InvalidArgumentException();
		text = new (std::nothrow) widechar[repeats + 1]; if (!text) throw OutOfMemoryException();
		for (int i = 0; i < repeats; i++) text[i] = src; text[repeats] = 0;
	}
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
	uint64 ImmutableString::GeneralizedToUInt64(int dfrom, int dto) const
	{
		uint64 value = 0;
		for (int i = dfrom; i < dto; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			uint64 digit = text[i] - L'0';
			if (value >= 0x199999999999999A) throw InvalidFormatException();
			value *= 10;
			if (value > 0xFFFFFFFFFFFFFFFF - digit) throw InvalidFormatException();
			value += digit;
		}
		return value;
	}
	uint64 ImmutableString::GeneralizedToUInt64(int dfrom, int dto, const ImmutableString & digits, bool case_sensitive) const
	{
		uint64 value = 0;
		auto base = digits.Length();
		if (base < 2) throw InvalidArgumentException();
		widechar input[2], compare[2];
		input[1] = compare[1] = 0;
		uint64 max_prem = 0xFFFFFFFFFFFFFFFF / base + 1;
		for (int i = dfrom; i < dto; i++) {
			int dn = -1;
			input[0] = text[i];
			for (int j = 0; j < digits.Length(); j++) {
				compare[0] = digits[j];
				if ((case_sensitive && StringCompare(input, compare) == 0) || (!case_sensitive && StringCompareCaseInsensitive(input, compare) == 0)) { dn = j; break; }
			}
			if (dn == -1) throw InvalidFormatException();
			uint64 digit = dn;
			if (value >= max_prem) throw InvalidFormatException();
			value *= base;
			if (value > 0xFFFFFFFFFFFFFFFF - digit) throw InvalidFormatException();
			value += digit;
		}
		return value;
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
			result->SetLength(char_length * GetBytesPerChar(encoding));
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
	uint64 ImmutableString::ToUInt64(void) const { return GeneralizedToUInt64(0, Length()); }
	uint64 ImmutableString::ToUInt64(const ImmutableString & digits, bool case_sensitive) const { return GeneralizedToUInt64(0, Length(), digits, case_sensitive); }
	int64 ImmutableString::ToInt64(void) const
	{
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0;
		if (text[0] == L'-') { start = 1; negative = true; }
		else if (text[0] == L'+') { start = 1; }
		auto absolute = GeneralizedToUInt64(start, len);
		if (negative) {
			if (absolute > 0x8000000000000000) throw InvalidFormatException();
			else return -int64(absolute);
		} else {
			if (absolute > 0x7FFFFFFFFFFFFFFF) throw InvalidFormatException();
			return absolute;
		}
	}
	int64 ImmutableString::ToInt64(const ImmutableString & digits, bool case_sensitive) const
	{
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0;
		if (text[0] == L'-') { start = 1; negative = true; } else if (text[0] == L'+') { start = 1; }
		auto absolute = GeneralizedToUInt64(start, len, digits, case_sensitive);
		if (negative) {
			if (absolute > 0x8000000000000000) throw InvalidFormatException();
			else return -int64(absolute);
		} else {
			if (absolute > 0x7FFFFFFFFFFFFFFF) throw InvalidFormatException();
			return absolute;
		}
	}
	uint32 ImmutableString::ToUInt32(void) const
	{
		uint64 parsed = GeneralizedToUInt64(0, Length());
		if (parsed > 0xFFFFFFFF) throw InvalidFormatException();
		return uint32(parsed);
	}
	uint32 ImmutableString::ToUInt32(const ImmutableString & digits, bool case_sensitive) const
	{
		uint64 parsed = GeneralizedToUInt64(0, Length(), digits, case_sensitive);
		if (parsed > 0xFFFFFFFF) throw InvalidFormatException();
		return uint32(parsed);
	}
	int32 ImmutableString::ToInt32(void) const
	{
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0;
		if (text[0] == L'-') { start = 1; negative = true; } else if (text[0] == L'+') { start = 1; }
		auto absolute = GeneralizedToUInt64(start, len);
		if (negative) {
			if (absolute > 0x80000000) throw InvalidFormatException();
			else return -int32(absolute);
		} else {
			if (absolute > 0x7FFFFFFF) throw InvalidFormatException();
			return int32(absolute);
		}
	}
	int32 ImmutableString::ToInt32(const ImmutableString & digits, bool case_sensitive) const
	{
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0;
		if (text[0] == L'-') { start = 1; negative = true; } else if (text[0] == L'+') { start = 1; }
		auto absolute = GeneralizedToUInt64(start, len, digits, case_sensitive);
		if (negative) {
			if (absolute > 0x80000000) throw InvalidFormatException();
			else return -int32(absolute);
		} else {
			if (absolute > 0x7FFFFFFF) throw InvalidFormatException();
			return int32(absolute);
		}
	}
	float ImmutableString::ToFloat(void) const
	{
		float value = 0.0f;
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0.0f;
		if (text[0] == L'-') { start = 1; negative = true; } else if (text[0] == L'+') { start = 1; }
		int point = FindFirst(L',');
		if (point == -1) point = FindFirst(L'.');
		if (point == -1) point = len;
		if (point != max(FindLast(L','), FindLast(L'.'))) throw InvalidFormatException();
		for (int i = start; i < point; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			value *= 10.0f;
			value += float(digit);
		}
		float exp = 1.0f;
		for (int i = point + 1; i < len; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			exp /= 10.0f;
			value += exp * float(digit);
		}
		return negative ? -value : value;
	}
	float ImmutableString::ToFloat(const ImmutableString & separators) const
	{
		float value = 0.0f;
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0.0f;
		if (text[0] == L'-') { start = 1; negative = true; } else if (text[0] == L'+') { start = 1; }
		int point = len;
		for (int i = 0; i < separators.Length(); i++) {
			int fp = FindFirst(separators[i]);
			if (fp != -1 && fp < point) point = fp;
		}
		for (int i = 0; i < separators.Length(); i++) {
			if (FindLast(separators[i]) > point) throw InvalidFormatException();
		}
		for (int i = start; i < point; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			value *= 10.0f;
			value += float(digit);
		}
		float exp = 1.0f;
		for (int i = point + 1; i < len; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			exp /= 10.0f;
			value += exp * float(digit);
		}
		return negative ? -value : value;
	}
	double ImmutableString::ToDouble(void) const
	{
		double value = 0.0;
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0.0;
		if (text[0] == L'-') { start = 1; negative = true; } else if (text[0] == L'+') { start = 1; }
		int point = FindFirst(L',');
		if (point == -1) point = FindFirst(L'.');
		if (point == -1) point = len;
		if (point != max(FindLast(L','), FindLast(L'.'))) throw InvalidFormatException();
		for (int i = start; i < point; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			value *= 10.0;
			value += double(digit);
		}
		double exp = 1.0;
		for (int i = point + 1; i < len; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			exp /= 10.0;
			value += exp * double(digit);
		}
		return negative ? -value : value;
	}
	double ImmutableString::ToDouble(const ImmutableString & separators) const
	{
		double value = 0.0;
		bool negative = false;
		int start = 0;
		auto len = Length();
		if (!len) return 0.0;
		if (text[0] == L'-') { start = 1; negative = true; } else if (text[0] == L'+') { start = 1; }
		int point = len;
		for (int i = 0; i < separators.Length(); i++) {
			int fp = FindFirst(separators[i]);
			if (fp != -1 && fp < point) point = fp;
		}
		for (int i = 0; i < separators.Length(); i++) {
			if (FindLast(separators[i]) > point) throw InvalidFormatException();
		}
		for (int i = start; i < point; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			value *= 10.0;
			value += double(digit);
		}
		double exp = 1.0;
		for (int i = point + 1; i < len; i++) {
			if (text[i] < L'0' || text[i] > L'9') throw InvalidFormatException();
			int digit = text[i] - L'0';
			exp /= 10.0;
			value += exp * double(digit);
		}
		return negative ? -value : value;
	}
	bool ImmutableString::ToBoolean(void) const
	{
		if (CompareIgnoreCase(*this, L"true") == 0 || CompareIgnoreCase(*this, L"1") == 0) return true;
		else if (CompareIgnoreCase(*this, L"false") == 0 || CompareIgnoreCase(*this, L"0") == 0 || Length() == 0) return false;
		else throw InvalidFormatException();
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
	double sgn(double x) { return (x > 0.0) ? 1.0 : ((x < 0.0) ? -1.0 : 0.0); }
	float sgn(float x) { return (x > 0.0f) ? 1.0f : ((x < 0.0f) ? -1.0f : 0.0f); }
	double saturate(double x) { if (x < 0.0) return 0.0; else if (x > 1.0) return 1.0; else return x; }
	float saturate(float x) { if (x < 0.0f) return 0.0f; else if (x > 1.0f) return 1.0f; else return x; }
}