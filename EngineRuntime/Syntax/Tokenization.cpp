#include "Tokenization.h"

#include "../Miscellaneous/DynamicString.h"

namespace Engine
{
	namespace Syntax
	{
		ParserSpellingException::ParserSpellingException(int At, const string & About) : Position(At), Comments(About) {}
		string ParserSpellingException::ToString(void) const { return L"ParserSpellingException: " + Comments + L" at position " + string(Position); }

		uint64 Token::AsInteger(void) const
		{
			try {
				if (Content.Length() > 2) {
					if (Content[1] == L'x' || Content[1] == L'X') {
						return Content.Fragment(2, -1).ToUInt64(L"0123456789ABCDEF");
					} else if (Content[1] == L'o' || Content[1] == L'O') {
						return Content.Fragment(2, -1).ToUInt64(L"01234567");
					} else if (Content[1] == L'b' || Content[1] == L'B') {
						return Content.Fragment(2, -1).ToUInt64(L"01");
					}
				} else {
					return Content.ToUInt64();
				}
			}
			catch (...) {
				throw ParserSpellingException(SourcePosition, L"Integer value is out of range");
			}
			return 0;
		}
		double Token::AsDouble(void) const { return Content.ToDouble(); }
		float Token::AsFloat(void) const { return Content.ToFloat(); }
		bool Token::AsBoolean(void) const { return Content.Length() > 0; }
		bool Token::IsSimilar(const Token & Template) const { return Class == Template.Class && ValueClass == Template.ValueClass && ((Class != TokenClass::Keyword && Class != TokenClass::CharCombo) || Content == Template.Content); }
		bool Token::IsVoid(void) const { return Class == TokenClass::Void; }
		Token Token::VoidToken(void)
		{
			Token token;
			token.Class = TokenClass::Void;
			token.ValueClass = TokenConstantClass::Unknown;
			token.SourcePosition = -1;
			return token;
		}

		Spelling::Spelling(void) : Keywords(0x10), IsolatedChars(0x20), ContinuousCharCombos(0x20) {}
		bool Spelling::IsKeyword(const string & word) const
		{
			for (int i = 0; i < Keywords.Length(); i++) if (Keywords[i] == word) return true;
			return false;
		}
		bool Spelling::IsIsolatedChars(widechar letter) const
		{
			for (int i = 0; i < IsolatedChars.Length(); i++) if (IsolatedChars[i] == letter) return true;
			return false;
		}
		bool Spelling::IsBooleanFalseLiteral(const string & word) const { return word == BooleanFalseLiteral; }
		bool Spelling::IsBooleanTrueLiteral(const string & word) const { return word == BooleanTrueLiteral; }

		bool operator==(const Token & a, const Token & b) { return a.Content == b.Content && a.Class == b.Class && a.ValueClass == b.ValueClass; }
		bool operator!=(const Token & a, const Token & b) { return a.Content != b.Content || a.Class != b.Class || a.ValueClass != b.ValueClass; }

		Array<Token>* ParseText(const string & text, const Spelling & spelling)
		{
			int pos = 0;
			SafePointer< Array<Token> > Result = new Array<Token>(0x1000);
			while (true) {
				Token token;
				while (text[pos] == L' ' || text[pos] == L'\t' || text[pos] == L'\r' || text[pos] == L'\n') pos++;
				if (!text[pos]) {
					token.Class = TokenClass::EndOfStream;
					token.ValueClass = TokenConstantClass::Unknown;
					token.SourcePosition = pos;
					Result->Append(token);
					break;
				} else if ((text[pos] >= L'A' && text[pos] <= L'Z') || (text[pos] >= L'a' && text[pos] <= L'z') || text[pos] == L'_') {
					token.SourcePosition = pos;
					int sp = pos;
					while ((text[pos] >= L'A' && text[pos] <= L'Z') || (text[pos] >= L'a' && text[pos] <= L'z') || (text[pos] >= L'0' && text[pos] <= L'9') || text[pos] == L'_') pos++;
					token.Content = text.Fragment(sp, pos - sp);
					if (spelling.IsBooleanFalseLiteral(token.Content)) {
						token.Content = L"";
						token.Class = TokenClass::Constant;
						token.ValueClass = TokenConstantClass::Boolean;
					} else if (spelling.IsBooleanTrueLiteral(token.Content)) {
						token.Content = L"1";
						token.Class = TokenClass::Constant;
						token.ValueClass = TokenConstantClass::Boolean;
					} else if (spelling.IsKeyword(token.Content)) {
						token.Class = TokenClass::Keyword;
						token.ValueClass = TokenConstantClass::Unknown;
					} else {
						token.Class = TokenClass::Identifier;
						token.ValueClass = TokenConstantClass::Unknown;
					}
					Result->Append(token);
				} else if (text[pos] >= L'0' && text[pos] <= L'9') {
					token.SourcePosition = pos;
					token.Class = TokenClass::Constant;
					token.ValueClass = TokenConstantClass::Numeric;
					int sp = pos;
					while ((text[pos] >= L'A' && text[pos] <= L'Z') || (text[pos] >= L'a' && text[pos] <= L'z') || (text[pos] >= L'0' && text[pos] <= L'9')) pos++;
					if (text[pos] == L'.') {
						pos++;
						while ((text[pos] >= L'A' && text[pos] <= L'Z') || (text[pos] >= L'a' && text[pos] <= L'z') || (text[pos] >= L'0' && text[pos] <= L'9')) pos++;
					}
					token.Content = text.Fragment(sp, pos - sp);
					Result->Append(token);
				} else if (spelling.CommentEndOfLineWord.Length() && text.Length() - pos >= spelling.CommentEndOfLineWord.Length() && text.Fragment(pos, spelling.CommentEndOfLineWord.Length()) == spelling.CommentEndOfLineWord) {
					while (text[pos] != L'\n' && text[pos] != 0) pos++;
				} else if (spelling.CommentBlockOpeningWord.Length() && text.Length() - pos >= spelling.CommentBlockOpeningWord.Length() && text.Fragment(pos, spelling.CommentBlockOpeningWord.Length()) == spelling.CommentBlockOpeningWord) {
					pos += spelling.CommentBlockOpeningWord.Length();
					while (text[pos] && !(spelling.CommentBlockClosingWord.Length() && text.Length() - pos >= spelling.CommentBlockClosingWord.Length() && text.Fragment(pos, spelling.CommentBlockClosingWord.Length()) == spelling.CommentBlockClosingWord)) pos++;
					if (text[pos]) pos += spelling.CommentBlockClosingWord.Length();
				} else if (text[pos] == L'\"') {
					token.SourcePosition = pos;
					token.Class = TokenClass::Constant;
					token.ValueClass = TokenConstantClass::String;
					DynamicString Text;
					pos++;
					do {
						if (text[pos] == L'\n') throw ParserSpellingException(pos, L"Unexpected caret return inside string constant");
						else if (text[pos] == 0) throw ParserSpellingException(pos, L"Unexpected end-of-stream inside string constant");
						else if (text[pos] != L'\"') {
							if (text[pos] == L'\\') {
								pos++;
								uint32 ucsdec = 0xFFFFFFFF;
								widechar escc = text[pos];
								if ((escc == L'\\') || (escc == L'\'') || (escc == L'\"') || (escc == L'?')) {
									Text += escc;
									pos++;
								} else if (escc == L'a' || escc == L'A') {
									Text += L'\a';
									pos++;
								} else if (escc == L'b' || escc == L'B') {
									Text += L'\b';
									pos++;
								} else if (escc == L'f' || escc == L'F') {
									Text += L'\f';
									pos++;
								} else if (escc == L'n' || escc == L'N') {
									Text += L'\n';
									pos++;
								} else if (escc == L'r' || escc == L'R') {
									Text += L'\r';
									pos++;
								} else if (escc == L't' || escc == L'T') {
									Text += L'\t';
									pos++;
								} else if (escc == L'v' || escc == L'V') {
									Text += L'\v';
									pos++;
								} else if (escc == L'x' || escc == L'X') {
									pos++;
									while ((text[pos] >= L'0' && text[pos] <= L'9') || (text[pos] >= L'A' && text[pos] <= L'F') || (text[pos] >= L'a' && text[pos] <= L'f')) {
										int rec = 0;
										ucsdec = 0;
										if ((text[pos] >= L'0') && (text[pos] <= L'9')) rec = text[pos] - L'0';
										else if ((text[pos] >= L'A') && (text[pos] <= L'F')) rec = text[pos] - L'A' + 10;
										else rec = text[pos] - L'a' + 10;
										ucsdec <<= 4;
										ucsdec |= rec;
										pos++;
									}
								} else if ((escc >= L'0') && (escc <= L'7')) {
									int count = 1;
									ucsdec = 0;
									while ((count < 4) && (escc >= L'0') && (escc <= L'7')) {
										int rec = text[pos] - L'0';
										ucsdec <<= 3;
										ucsdec |= rec;
										count++;
										pos++;
									}
								} else throw ParserSpellingException(pos, L"Unknown escape sequence");
								if (ucsdec != 0xFFFFFFFF) {
									Text += string(&ucsdec, 1, Encoding::UTF32);
								}
							} else {
								Text += text[pos];
								pos++;
							}
						}
					} while (text[pos] != L'\"' && text[pos]);
					if (text[pos] == L'\"') pos++;
					token.Content = Text.ToString();
					Result->Append(token);
				} else if (spelling.IsIsolatedChars(text[pos])) {
					token.SourcePosition = pos;
					token.Class = TokenClass::CharCombo;
					token.ValueClass = TokenConstantClass::Unknown;
					token.Content = text[pos];
					pos++;
					Result->Append(token);
				} else {
					bool found = false;
					for (int i = 0; i < spelling.ContinuousCharCombos.Length(); i++) {
						if (text.Length() - pos >= spelling.ContinuousCharCombos[i].Length() && text.Fragment(pos, spelling.ContinuousCharCombos[i].Length()) == spelling.ContinuousCharCombos[i]) {
							token.SourcePosition = pos;
							token.Class = TokenClass::CharCombo;
							token.ValueClass = TokenConstantClass::Unknown;
							token.Content = spelling.ContinuousCharCombos[i];
							Result->Append(token);
							pos += spelling.ContinuousCharCombos[i].Length();
							found = true;
							break;
						}
					}
					if (!found) throw ParserSpellingException(pos, L"Illegal character input");		
				}
			}
			Result->Retain();
			return Result;
		}
	}
}