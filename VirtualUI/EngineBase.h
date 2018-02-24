#pragma once

#include "PlatformDependent.h"

#include <new>

namespace Engine
{
	class Object;
	class ImmutableString;

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

	class ImmutableString final : public Object
	{
	private:
		widechar * text;
		int GeneralizedFindFirst(int from, const widechar * seq, int length) const;
		ImmutableString GeneralizedReplace(const widechar * seq, int length, const widechar * with, int withlen) const;
	public:
		ImmutableString(void);
		ImmutableString(const ImmutableString & src);
		ImmutableString(ImmutableString && src);
		ImmutableString(const widechar * src);
		ImmutableString(const widechar * sequence, int length);
		ImmutableString(int src);
		ImmutableString(uint src);
		ImmutableString(int64 src);
		ImmutableString(uint64 src);
		ImmutableString(uint value, const ImmutableString & digits);
		ImmutableString(uint64 value, const ImmutableString & digits);
		ImmutableString(const void * src);
		ImmutableString(const void * Sequence, int Length, Encoding SequenceEncoding);
		ImmutableString(float src, widechar separator);
		ImmutableString(double src, widechar separator);
		ImmutableString(bool src);
		ImmutableString(widechar src);
		ImmutableString(const Object * object);
		~ImmutableString(void);

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

		static int Compare(const ImmutableString & a, const ImmutableString & b);
		static int CompareIgnoreCase(const ImmutableString & a, const ImmutableString & b);

		widechar operator [] (int index) const;

		ImmutableString ToString(void) const override;
		
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
	};

	typedef ImmutableString string;
}