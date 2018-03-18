#pragma once

#include "../EngineBase.h"
#include "../Miscellaneous/Dictionary.h"
#include "Tokenization.h"

namespace Engine
{
	namespace Syntax
	{
		class Grammar : public Object
		{
		public:
			class GrammarRule : public Object
			{
			private:
				bool _sinal = false;
			public:
				enum class RuleClass { Variant, Sequence, Reference, Token };

				// Depending on the class:
				// If Class is Variant, then this rule represents an appearance of one of the rules from Rules array
				// If Class is Sequence, then this rule represents a strict sequence of the rules from array
				// If Class is Reference, then this rule should be replaced with another rule from the grammar with name from Reference
				// If Class is Token, then this rule represents an appearance of token, specified in Token

				// Label is technical identifier, attached to this rule
				// Repeat counts specify minimal and maximal repeat count of this rule in the text

				RuleClass Class;
				Array<GrammarRule> Rules;
				string Reference;
				Token TokenClass;
				int MinRepeat = 1;
				int MaxRepeat = 1;
				string Label;

				Array<Token> CanBeginWith;

				//GrammarRule(void) : Rules(0x10), CanBeginWith(0x10) {}

				//void BuildBeginnings(Grammar & grammar)
				//{
				//	CanBeginWith.Clear();
				//	if (MinRepeat == 0) {
				//		CanBeginWith << T::VoidToken();
				//	}
				//	_sinal = true;
				//	if (Class == RuleClass::Variant) {
				//		for (int i = 0; i < Rules.Length(); i++) {
				//			if (!Rules[i].CanBeginWith.Length()) {
				//				if (Rules[i]._sinal) throw Exception();
				//				Rules[i].BuildBeginnings(grammar);
				//			}
				//			// merge
				//		}
				//	} else if (Class == RuleClass::Sequence) {
				//		if (Rules.Length()) {
				//			if (!Rules[0].CanBeginWith.Length()) {
				//				if (Rules[0]._sinal) throw Exception();
				//				Rules[0].BuildBeginnings(grammar);
				//			}
				//			// merge
				//		}
				//	} else if (Class == RuleClass::Reference) {
				//		if (!grammar.Rules.ElementPresent(Reference)) throw Exception;
				//		auto & rule = grammar.Rules[Reference];
				//		if (!rule.CanBeginWith.Length()) {
				//			if (rule._sinal) throw Exception();
				//			rule.BuildBeginnings(grammar);
				//		}
				//		// merge
				//	} else if (Class == RuleClass::Token) {
				//		CanBeginWith << Token;
				//	}
				//	_sinal = false;
				//}
			};

			Dictionary::Dictionary<string, GrammarRule> Rules;

			/*Grammar(void) : Rules(0x20) {}*/
		};
		template <class T> class SyntaxTree : public Object
		{
		public:

		};
	}
}