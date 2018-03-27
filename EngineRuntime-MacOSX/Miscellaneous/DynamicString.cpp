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
}