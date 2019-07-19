#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Syntax
	{
		enum class TokenClass { EndOfStream, Keyword, Identifier, Constant, CharCombo, Unknown, Void };
		enum class TokenConstantClass { String, Numeric, Boolean, Unknown };
		enum class NumericTokenClass { Integer, Float, Double };

		class ParserSpellingException : public Exception { public: int Position; string Comments; ParserSpellingException(int At, const string & About); string ToString(void) const override; };

		class Token
		{
		public:
			string Content;
			TokenClass Class = TokenClass::Unknown;
			TokenConstantClass ValueClass = TokenConstantClass::Unknown;
			int SourcePosition = -1;
			union {
				void * UserPtr;
				eint UserInt;
			};

			Token(void);
			
			static Token EndOfStreamToken(void);
			static Token KeywordToken(const string & word);
			static Token IdentifierToken(const string & ident);
			static Token IdentifierToken(void);
			static Token ConstantToken(const string & content, TokenConstantClass type);
			static Token ConstantToken(TokenConstantClass type);
			static Token ConstantToken(void);
			static Token CharacterToken(const string & combo);
			static Token VoidToken(void);

			uint64 AsInteger(void) const;
			double AsDouble(void) const;
			float AsFloat(void) const;
			bool AsBoolean(void) const;
			NumericTokenClass NumericClass(void) const;

			friend bool operator == (const Token & a, const Token & b);
			friend bool operator != (const Token & a, const Token & b);

			bool IsSimilar(const Token & Template) const;
			bool IsVoid(void) const;
		};
		class Spelling
		{
		public:
			Array<string> Keywords;
			Array<widechar> IsolatedChars;
			Array<string> ContinuousCharCombos;
			string BooleanFalseLiteral;
			string BooleanTrueLiteral;
			string InfinityLiteral;
			string NonNumberLiteral;
			string CommentEndOfLineWord;
			string CommentBlockOpeningWord;
			string CommentBlockClosingWord;
			bool AllowNonLatinNames;

			Spelling(void);

			bool IsKeyword(const string & word) const;
			bool IsIsolatedChars(widechar letter) const;
			bool IsBooleanFalseLiteral(const string & word) const;
			bool IsBooleanTrueLiteral(const string & word) const;
			bool IsFloatInfinityLiteral(const string & word) const;
			bool IsFloatNonNumberLiteral(const string & word) const;
		};

		Array<Token> * ParseText(const string & text, const Spelling & spelling);
		string FormatStringToken(const string & input);
		string FormatFloatToken(float input, const Spelling & spelling);
		string FormatDoubleToken(double input, const Spelling & spelling);
	}
}