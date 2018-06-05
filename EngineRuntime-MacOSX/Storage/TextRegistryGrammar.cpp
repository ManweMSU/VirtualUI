#include "TextRegistryGrammar.h"

namespace Engine
{
	namespace Storage
	{
		void CreateTextRegistrySpelling(Syntax::Spelling & spelling)
		{
			spelling.BooleanFalseLiteral = L"false";
			spelling.BooleanTrueLiteral = L"true";
			spelling.InfinityLiteral = L"float_infinity";
			spelling.NonNumberLiteral = L"float_nan";
			spelling.CommentBlockClosingWord = L"/*";
			spelling.CommentBlockOpeningWord = L"*/";
			spelling.CommentEndOfLineWord = L"//";
			spelling.IsolatedChars << L'{';
			spelling.IsolatedChars << L'}';
			spelling.IsolatedChars << L'=';
			spelling.IsolatedChars << L'-';
			spelling.Keywords << L"long";
			spelling.Keywords << L"color";
			spelling.Keywords << L"time";
			spelling.Keywords << L"binary";
		}
		void CreateTextRegistryGrammar(Syntax::Grammar & grammar)
		{
			SafePointer<Syntax::Grammar::GrammarRule> node =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"NODE"));
			node->Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"", 0, -1);
			node->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::VariantRule(L"");
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::ConstantToken());
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::KeywordToken(L"long"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::KeywordToken(L"color"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::KeywordToken(L"time"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::KeywordToken(L"binary"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"NAME", Syntax::Token::IdentifierToken());
			node->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::VariantRule(L"");
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"NODEDEF");
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"{"));
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"NODE");
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"}"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"VALDEF");
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"="));
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"VALDEF");
			SafePointer<Syntax::Grammar::GrammarRule> valdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::VariantRule(L"VALDEF"));
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"INTDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"BOOLDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"STRDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"LINTDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"COLORDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"TIMEDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(L"", L"BINDEF");
			SafePointer<Syntax::Grammar::GrammarRule> intdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"INTDEF"));
			intdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"-"), 0, 1);
			intdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			SafePointer<Syntax::Grammar::GrammarRule> booldef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"BOOLDEF"));
			booldef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Boolean));
			SafePointer<Syntax::Grammar::GrammarRule> strdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"STRDEF"));
			strdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::String));
			SafePointer<Syntax::Grammar::GrammarRule> lintdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"LINTDEF"));
			lintdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::KeywordToken(L"long"));
			lintdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"-"), 0, 1);
			lintdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			SafePointer<Syntax::Grammar::GrammarRule> colordef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"COLORDEF"));
			colordef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::KeywordToken(L"color"));
			colordef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			SafePointer<Syntax::Grammar::GrammarRule> timedef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"TIMEDEF"));
			timedef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::KeywordToken(L"time"));
			timedef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"{"));
			timedef->Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"", 1, 7);
			timedef->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			timedef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"}"));
			SafePointer<Syntax::Grammar::GrammarRule> bindef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(L"BINDEF"));
			bindef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::KeywordToken(L"binary"));
			bindef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"{"));
			bindef->Rules << Syntax::Grammar::GrammarRule::SequenceRule(L"", 0, -1);
			bindef->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(L"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			bindef->Rules << Syntax::Grammar::GrammarRule::TokenRule(L"", Syntax::Token::CharacterToken(L"}"));
			grammar.Rules.Append(node->Label, node);
			grammar.Rules.Append(valdef->Label, valdef);
			grammar.Rules.Append(intdef->Label, intdef);
			grammar.Rules.Append(booldef->Label, booldef);
			grammar.Rules.Append(strdef->Label, strdef);
			grammar.Rules.Append(lintdef->Label, lintdef);
			grammar.Rules.Append(colordef->Label, colordef);
			grammar.Rules.Append(timedef->Label, timedef);
			grammar.Rules.Append(bindef->Label, bindef);
			grammar.EntranceRule = node->Label;
		}
	}
}