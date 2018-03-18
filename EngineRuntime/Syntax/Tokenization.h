#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Syntax
	{
		enum class TokenClass { EndOfStream, Keyword, Identifier, Constant, CharCombo, Unknown, Void };
		enum class TokenConstantClass { String, Numeric, Boolean, Unknown };

		class ParserSpellingException : public Exception { public: int Position; string Comments; ParserSpellingException(int At, const string & About); string ToString(void) const override; };

		class Token
		{
		public:
			string Content;
			TokenClass Class;
			TokenConstantClass ValueClass;
			int SourcePosition;

			uint64 AsInteger(void) const;
			double AsDouble(void) const;
			float AsFloat(void) const;
			bool AsBoolean(void) const;

			friend bool operator == (const Token & a, const Token & b);
			friend bool operator != (const Token & a, const Token & b);

			bool IsSimilar(const Token & Template) const;
			bool IsVoid(void) const;
			static Token VoidToken(void);
		};
		class Spelling
		{
		public:
			Array<string> Keywords;
			Array<widechar> IsolatedChars;
			Array<string> ContinuousCharCombos;
			string BooleanFalseLiteral;
			string BooleanTrueLiteral;
			string CommentEndOfLineWord;
			string CommentBlockOpeningWord;
			string CommentBlockClosingWord;

			Spelling(void);

			bool IsKeyword(const string & word) const;
			bool IsIsolatedChars(widechar letter) const;
			bool IsBooleanFalseLiteral(const string & word) const;
			bool IsBooleanTrueLiteral(const string & word) const;
		};

		Array<Token> * ParseText(const string & text, const Spelling & spelling);
	}
}