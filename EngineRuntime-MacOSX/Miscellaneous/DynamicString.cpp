#include "DynamicString.h"

namespace Engine
{
	DynamicString::DynamicString(void) : data(0x100) { data << L'\0'; }
	DynamicString::DynamicString(const ImmutableString & src) : data(0x100) { data.Append(static_cast<const widechar *>(src), src.Length() + 1); }
	DynamicString::DynamicString(int BlockSize) : data(BlockSize) { data << L'\0'; }
	DynamicString::DynamicString(const ImmutableString & src, int BlockSize) : data(BlockSize) { data.Append(static_cast<const widechar *>(src), src.Length() + 1); }
	DynamicString::operator ImmutableString(void) const { return ToString(); }
	DynamicString::operator widechar*(void) { return data.GetBuffer(); }
	DynamicString::operator const widechar*(void) const { return data.GetBuffer(); }
	ImmutableString DynamicString::ToString(void) const { return ImmutableString(data.GetBuffer(), data.Length()); }
	int DynamicString::Length(void) const { return StringLength(data.GetBuffer()); }
	widechar * DynamicString::GetBuffer(void) { return data.GetBuffer(); }
	const widechar * DynamicString::GetBuffer(void) const { return data.GetBuffer(); }
	widechar DynamicString::CharAt(int index) const { return data[index]; }
	widechar & DynamicString::CharAt(int index) { return data[index]; }
	widechar DynamicString::operator[](int index) const { return data[index]; }
	widechar & DynamicString::operator[](int index) { return data[index]; }
	void DynamicString::Concatenate(const ImmutableString & str) { int len = Length(); data.SetLength(len + str.Length() + 1); StringCopy(data.GetBuffer() + len, str); }
	void DynamicString::Concatenate(widechar letter) { int len = Length(); data[len] = letter; data << L'\0'; }
	DynamicString & DynamicString::operator+=(const ImmutableString & str) { Concatenate(str); return *this; }
	DynamicString & DynamicString::operator+=(widechar letter) { Concatenate(letter); return *this; }
	DynamicString & DynamicString::operator<<(const ImmutableString & str) { Concatenate(str); return *this; }
	DynamicString & DynamicString::operator<<(widechar letter) { Concatenate(letter); return *this; }
	void DynamicString::Insert(const ImmutableString & str, int at)
	{
		int len = Length();
		int ins = str.Length();
		data.SetLength(len + ins + 1);
		for (int i = len; i >= at; i--) data[i + ins] = data[i];
		MemoryCopy(data.GetBuffer() + at, str, ins * sizeof(widechar));
	}
	void DynamicString::RemoveRange(int start, int amount)
	{
		int len = Length();
		for (int i = start + amount; i <= len; i++) data[i - amount] = data[i];
		data.SetLength(len - amount + 1);
	}
	void DynamicString::Clear(void) { data.Clear(); data << L'\0'; }
	void DynamicString::ReserveLength(int length) { data.SetLength(length + 1); }
	int DynamicString::ReservedLength(void) const { return data.Length(); }

	string FormatString(const string & base, const string & arg1)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
			else if (base[i] == L'3') result << arg4;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
			else if (base[i] == L'3') result << arg4;
			else if (base[i] == L'4') result << arg5;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
			else if (base[i] == L'3') result << arg4;
			else if (base[i] == L'4') result << arg5;
			else if (base[i] == L'5') result << arg6;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
			else if (base[i] == L'3') result << arg4;
			else if (base[i] == L'4') result << arg5;
			else if (base[i] == L'5') result << arg6;
			else if (base[i] == L'6') result << arg7;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7, const string & arg8)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
			else if (base[i] == L'3') result << arg4;
			else if (base[i] == L'4') result << arg5;
			else if (base[i] == L'5') result << arg6;
			else if (base[i] == L'6') result << arg7;
			else if (base[i] == L'7') result << arg8;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7, const string & arg8, const string & arg9)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
			else if (base[i] == L'3') result << arg4;
			else if (base[i] == L'4') result << arg5;
			else if (base[i] == L'5') result << arg6;
			else if (base[i] == L'6') result << arg7;
			else if (base[i] == L'7') result << arg8;
			else if (base[i] == L'8') result << arg9;
		} else result << base[i];
		return result.ToString();
	}
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7, const string & arg8, const string & arg9, const string & arg10)
	{
		DynamicString result;
		for (int i = 0; i < base.Length(); i++) if (base[i] == L'%') {
			i++; if (base[i] == L'%') result << base[i];
			else if (base[i] == L'0') result << arg1;
			else if (base[i] == L'1') result << arg2;
			else if (base[i] == L'2') result << arg3;
			else if (base[i] == L'3') result << arg4;
			else if (base[i] == L'4') result << arg5;
			else if (base[i] == L'5') result << arg6;
			else if (base[i] == L'6') result << arg7;
			else if (base[i] == L'7') result << arg8;
			else if (base[i] == L'8') result << arg9;
			else if (base[i] == L'9') result << arg10;
		} else result << base[i];
		return result.ToString();
	}
}