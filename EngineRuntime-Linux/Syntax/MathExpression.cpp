#include "MathExpression.h"

#include "../Math/MathBase.h"

namespace Engine
{
	namespace Syntax
	{
		namespace Math
		{
			bool lang_initialized = false;
			Spelling lang_spelling;
			Grammar lang_grammar;
			void InitializeLanguage(void)
			{
				if (!lang_initialized) {
					GetLanguageInfo(lang_spelling, lang_grammar);
					lang_initialized = true;
				}
			}

			int64 IVariableProvider::GetInteger(const string & name) { return 0; }
			double IVariableProvider::GetDouble(const string & name) { return 0.0; }
			void GetLanguageInfo(Spelling & spelling, Grammar & grammar)
			{
				spelling.IsolatedChars.Append(L'(');
				spelling.IsolatedChars.Append(L')');
				spelling.IsolatedChars.Append(L'+');
				spelling.IsolatedChars.Append(L'-');
				spelling.IsolatedChars.Append(L'*');
				spelling.IsolatedChars.Append(L'/');
				spelling.IsolatedChars.Append(L'%');
				spelling.Keywords << L"sgn";
				spelling.Keywords << L"abs";
				spelling.Keywords << L"sin";
				spelling.Keywords << L"cos";
				spelling.Keywords << L"tg";
				spelling.Keywords << L"ctg";
				spelling.Keywords << L"arcsin";
				spelling.Keywords << L"arccos";
				spelling.Keywords << L"arctg";
				spelling.Keywords << L"arcctg";
				spelling.Keywords << L"ln";
				spelling.Keywords << L"exp";
				spelling.Keywords << L"sqrt";
				grammar.EntranceRule = L"EXPRESSION";
				SafePointer<Grammar::GrammarRule> Expression = new Grammar::GrammarRule(Grammar::GrammarRule::SequenceRule(L"EXPRESSION"));
				SafePointer<Grammar::GrammarRule> MulArg = new Grammar::GrammarRule(Grammar::GrammarRule::SequenceRule(L"MULARG"));
				SafePointer<Grammar::GrammarRule> Operand = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(L"OPERAND"));
				SafePointer<Grammar::GrammarRule> AddOp = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(L"ADDOP"));
				SafePointer<Grammar::GrammarRule> MulOp = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(L"MULOP"));
				SafePointer<Grammar::GrammarRule> FuncWord = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(L"FUNCWORD"));
				Expression->Rules << Grammar::GrammarRule::ReferenceRule(L"", L"MULARG");
				Expression->Rules << Grammar::GrammarRule::SequenceRule(L"", 0, -1);
				Expression->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"ADDOP");
				Expression->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"MULARG");
				MulArg->Rules << Grammar::GrammarRule::ReferenceRule(L"", L"OPERAND");
				MulArg->Rules << Grammar::GrammarRule::SequenceRule(L"", 0, -1);
				MulArg->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"MULOP");
				MulArg->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"OPERAND");
				Operand->Rules << Grammar::GrammarRule::SequenceRule(L"");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L"("));
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"EXPRESSION");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L")"));
				Operand->Rules << Grammar::GrammarRule::SequenceRule(L"");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"ADDOP", 0, 1);
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(L"", Token::ConstantToken(TokenConstantClass::Numeric));
				Operand->Rules << Grammar::GrammarRule::TokenRule(L"", Token::IdentifierToken());
				Operand->Rules << Grammar::GrammarRule::SequenceRule(L"");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"FUNCWORD");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L"("));
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(L"", L"EXPRESSION");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L")"));
				AddOp->Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L"+"));
				AddOp->Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L"-"));
				MulOp->Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L"*"));
				MulOp->Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L"/"));
				MulOp->Rules << Grammar::GrammarRule::TokenRule(L"", Token::CharacterToken(L"%"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"sgn"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"abs"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"sin"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"cos"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"tg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"ctg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"arcsin"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"arccos"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"arctg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"arcctg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"ln"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"exp"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(L"", Token::KeywordToken(L"sqrt"));
				grammar.Rules.Append(Expression->Label, Expression);
				grammar.Rules.Append(MulArg->Label, MulArg);
				grammar.Rules.Append(Operand->Label, Operand);
				grammar.Rules.Append(AddOp->Label, AddOp);
				grammar.Rules.Append(MulOp->Label, MulOp);
				grammar.Rules.Append(FuncWord->Label, FuncWord);
			}
			int64 CalculateIntegerValue(SyntaxTreeNode & expression, IVariableProvider * variables);
			double CalculateDoubleValue(SyntaxTreeNode & expression, IVariableProvider * variables);
			int64 CalculateIntegerValue(SyntaxTreeNode & expression, IVariableProvider * variables)
			{
				if (expression.Label == L"OPERAND") {
					if (expression.Subnodes[0].Expands.Content == L"(") return CalculateIntegerValue(expression.Subnodes[1], variables);
					else if (expression.Subnodes[0].Expands.Class == TokenClass::Identifier) {
						if (variables) {
							return variables->GetInteger(expression.Subnodes[0].Expands.Content);
						} else return 0;
					} else if (expression.Subnodes[0].Label == L"FUNCWORD") {
						string func = expression.Subnodes[0].Subnodes[0].Expands.Content;
						if (func == L"sgn") {
							int64 value = CalculateIntegerValue(expression.Subnodes[2], variables);
							return (value > 0) ? 1 : ((value < 0) ? -1 : 0);
						} else if (func == L"abs") {
							int64 value = CalculateIntegerValue(expression.Subnodes[2], variables);
							return (value > 0) ? value : -value;
						} else if (func == L"sin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::sin(value));
						} else if (func == L"cos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::cos(value));
						} else if (func == L"tg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::tg(value));
						} else if (func == L"ctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::ctg(value));
						} else if (func == L"arcsin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arcsin(value));
						} else if (func == L"arccos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arccos(value));
						} else if (func == L"arctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arctg(value));
						} else if (func == L"arcctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arcctg(value));
						} else if (func == L"ln") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::ln(value));
						} else if (func == L"exp") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::exp(value));
						} else if (func == L"sqrt") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::sqrt(value));
						}
						return 0;
					} else {
						int vi = 0;
						int sgn = 1;
						if (expression.Subnodes[0].Label == L"ADDOP") {
							vi = 1;
							if (expression.Subnodes[0].Subnodes[0].Expands.Content == L"-") sgn = -1;
						}
						if (expression.Subnodes[vi].Expands.NumericClass() == NumericTokenClass::Integer) {
							return int64(expression.Subnodes[vi].Expands.AsInteger()) * sgn;
						} else {
							return int64(expression.Subnodes[vi].Expands.AsDouble()) * sgn;
						}
					}
				} else {
					int64 value = CalculateIntegerValue(expression.Subnodes[0], variables);
					for (int i = 1; i < expression.Subnodes.Length(); i += 2) {
						int64 value2 = CalculateIntegerValue(expression.Subnodes[i + 1], variables);
						if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"+") {
							value += value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"-") {
							value -= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"*") {
							value *= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"/") {
							value /= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"%") {
							value %= value2;
						}
					}
					return value;
				}
			}
			double CalculateDoubleValue(SyntaxTreeNode & expression, IVariableProvider * variables)
			{
				if (expression.Label == L"OPERAND") {
					if (expression.Subnodes[0].Expands.Content == L"(") return CalculateDoubleValue(expression.Subnodes[1], variables);
					else if (expression.Subnodes[0].Expands.Class == TokenClass::Identifier) {
						if (variables) {
							return variables->GetDouble(expression.Subnodes[0].Expands.Content);
						} else return 0.0;
					} else if (expression.Subnodes[0].Label == L"FUNCWORD") {
						string func = expression.Subnodes[0].Subnodes[0].Expands.Content;
						if (func == L"sgn") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return (value > 0.0) ? 1.0 : ((value < 0) ? -1.0 : 0.0);
						} else if (func == L"abs") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return (value > 0.0) ? value : -value;
						} else if (func == L"sin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::sin(value);
						} else if (func == L"cos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::cos(value);
						} else if (func == L"tg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::tg(value);
						} else if (func == L"ctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::ctg(value);
						} else if (func == L"arcsin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arcsin(value);
						} else if (func == L"arccos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arccos(value);
						} else if (func == L"arctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arctg(value);
						} else if (func == L"arcctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arcctg(value);
						} else if (func == L"ln") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::ln(value);
						} else if (func == L"exp") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::exp(value);
						} else if (func == L"sqrt") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::sqrt(value);
						}
						return 0.0;
					} else {
						int vi = 0;
						double sgn = 1.0;
						if (expression.Subnodes[0].Label == L"ADDOP") {
							vi = 1;
							if (expression.Subnodes[0].Subnodes[0].Expands.Content == L"-") sgn = -1.0;
						}
						if (expression.Subnodes[vi].Expands.NumericClass() == NumericTokenClass::Integer) {
							return double(expression.Subnodes[vi].Expands.AsInteger()) * sgn;
						} else {
							return expression.Subnodes[vi].Expands.AsDouble() * sgn;
						}
					}
				} else {
					double value = CalculateDoubleValue(expression.Subnodes[0], variables);
					for (int i = 1; i < expression.Subnodes.Length(); i += 2) {
						double value2 = CalculateDoubleValue(expression.Subnodes[i + 1], variables);
						if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"+") {
							value += value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"-") {
							value -= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"*") {
							value *= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == L"/") {
							value /= value2;
						}
					}
					return value;
				}
			}
			int64 CalculateExpressionInteger(const string & expression, IVariableProvider * variables)
			{
				InitializeLanguage();
				SafePointer< Array<Token> > Stream = ParseText(expression, lang_spelling);
				SafePointer<SyntaxTree> Tree = new SyntaxTree(*Stream, lang_grammar);
				Tree->Root.OptimizeNode();
				return CalculateIntegerValue(Tree->Root, variables);
			}
			double CalculateExpressionDouble(const string & expression, IVariableProvider * variables)
			{
				InitializeLanguage();
				SafePointer< Array<Token> > Stream = ParseText(expression, lang_spelling);
				SafePointer<SyntaxTree> Tree = new SyntaxTree(*Stream, lang_grammar);
				Tree->Root.OptimizeNode();
				return CalculateDoubleValue(Tree->Root, variables);
			}
		}
	}
}