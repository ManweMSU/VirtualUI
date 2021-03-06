#pragma once

#include "../EngineBase.h"

namespace Engine
{
	class DynamicString : public Object
	{
		Array<widechar> data;
	public:
		DynamicString(void);
		explicit DynamicString(const ImmutableString & src);
		explicit DynamicString(int BlockSize);
		explicit DynamicString(const ImmutableString & src, int BlockSize);

		operator ImmutableString (void) const;
		operator widechar * (void);
		operator const widechar * (void) const;
		ImmutableString ToString(void) const override;
		int Length(void) const;

		widechar * GetBuffer(void);
		const widechar * GetBuffer(void) const;

		widechar CharAt(int index) const;
		widechar & CharAt(int index);
		widechar operator [] (int index) const;
		widechar & operator [] (int index);

		void Concatenate(const ImmutableString & str);
		void Concatenate(widechar letter);
		DynamicString & operator += (const ImmutableString & str);
		DynamicString & operator += (widechar letter);
		DynamicString & operator << (const ImmutableString & str);
		DynamicString & operator << (widechar letter);

		void Insert(const ImmutableString & str, int at);
		void RemoveRange(int start, int amount);
		void Clear(void);
		void ReserveLength(int length);
		int ReservedLength(void) const;
	};

	string FormatString(const string & base, const string & arg1);
	string FormatString(const string & base, const string & arg1, const string & arg2);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7, const string & arg8);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7, const string & arg8, const string & arg9);
	string FormatString(const string & base, const string & arg1, const string & arg2, const string & arg3, const string & arg4, const string & arg5, const string & arg6, const string & arg7, const string & arg8, const string & arg9, const string & arg10);
}