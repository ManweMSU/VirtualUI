#include "Grammar.h"

namespace Engine
{
	namespace Syntax
	{
		ParserSyntaxException::ParserSyntaxException(int At, const string & About) : Position(At), Comments(About) {}
		string ParserSyntaxException::ToString(void) const { return L"ParserSyntaxException: " + Comments + L" at token " + string(Position); }

		Grammar::GrammarRule::GrammarRule(void) : Rules(0x20), CanBeginWith(0x10) {}
		Grammar::GrammarRule::GrammarRule(const GrammarRule & src) : Class(src.Class), Rules(src.Rules), Reference(src.Reference), TokenClass(src.TokenClass), MinRepeat(src.MinRepeat), MaxRepeat(src.MaxRepeat), Label(src.Label) {}
		Grammar::GrammarRule Grammar::GrammarRule::VariantRule(const string & label, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Variant;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			return result;
		}
		Grammar::GrammarRule Grammar::GrammarRule::SequenceRule(const string & label, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Sequence;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			return result;
		}
		Grammar::GrammarRule Grammar::GrammarRule::ReferenceRule(const string & label, const string & another_rule, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Reference;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			result.Reference = another_rule;
			return result;
		}
		Grammar::GrammarRule Grammar::GrammarRule::TokenRule(const string & label, const Token & token, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Reference;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			result.TokenClass = token;
			return result;
		}
		bool Grammar::GrammarRule::CanBeEmpty(void) const
		{
			for (int i = 0; i < CanBeginWith.Length(); i++) if (CanBeginWith[i].IsVoid()) return true;
			return false;
		}
		bool Grammar::GrammarRule::IsPossibleBeginning(const Token & token) const
		{
			for (int i = 0; i < CanBeginWith.Length(); i++) if (CanBeginWith[i].IsSimilar(token)) return true;
			return false;
		}
		void Grammar::GrammarRule::BuildBeginnings(Grammar & grammar)
		{
			CanBeginWith.Clear();
			if (MinRepeat == 0) {
				CanBeginWith << Token::VoidToken();
			}
			_sinal = true;
			if (Class == RuleClass::Variant) {
				for (int i = 0; i < Rules.Length(); i++) {
					if (!Rules[i].CanBeginWith.Length()) {
						if (Rules[i]._sinal) throw Exception();
						Rules[i].BuildBeginnings(grammar);
					}
					for (int j = 0; j < Rules[i].CanBeginWith.Length(); j++) {
						bool present = false;
						for (int k = 0; k < CanBeginWith.Length(); k++) {
							if (CanBeginWith[k] == Rules[i].CanBeginWith[j]) { present = true; break; }
						}
						if (!present) CanBeginWith << Rules[i].CanBeginWith[j];
					}
				}
			} else if (Class == RuleClass::Sequence) {
				for (int i = 0; i < Rules.Length(); i++) {
					if (!Rules[i].CanBeginWith.Length()) {
						if (Rules[i]._sinal) throw Exception();
						Rules[i].BuildBeginnings(grammar);
					}
					bool CanBeEmpty = false;
					for (int j = 0; j < Rules[i].CanBeginWith.Length(); j++) {
						if (Rules[i].CanBeginWith[j].IsVoid()) CanBeEmpty = true;
						bool present = false;
						for (int k = 0; k < CanBeginWith.Length(); k++) {
							if (CanBeginWith[k] == Rules[i].CanBeginWith[j]) { present = true; break; }
						}
						if (!present) CanBeginWith << Rules[i].CanBeginWith[j];
					}
					if (!CanBeEmpty) break;
				}
			} else if (Class == RuleClass::Reference) {
				if (!grammar.Rules.ElementPresent(Reference)) throw Exception();
				auto rule = grammar.Rules[Reference];
				if (!rule->CanBeginWith.Length()) {
					if (rule->_sinal) throw Exception();
					rule->BuildBeginnings(grammar);
				}
				for (int j = 0; j < rule->CanBeginWith.Length(); j++) {
					bool present = false;
					for (int k = 0; k < CanBeginWith.Length(); k++) {
						if (CanBeginWith[k] == rule->CanBeginWith[j]) { present = true; break; }
					}
					if (!present) CanBeginWith << rule->CanBeginWith[j];
				}
			} else if (Class == RuleClass::Token) {
				CanBeginWith << TokenClass;
			}
			_sinal = false;
		}

		Grammar::Grammar(void) : Rules(0x20) {}

		SyntaxTreeNode::SyntaxTreeNode(void) : Subnodes(0x20) {}

		void SyntaxTreeNode::EnumerateNodes(INodeEnumerator * enumerator) const
		{
			enumerator->RegisterNode(*this);
			for (int i = 0; i < Subnodes.Length(); i++) Subnodes[i].EnumerateNodes(enumerator);
		}
		void SyntaxTreeNode::EnumerateNodes(const string & with_label, INodeEnumerator * enumerator) const
		{
			if (Label == with_label) enumerator->RegisterNode(*this);
			for (int i = 0; i < Subnodes.Length(); i++) Subnodes[i].EnumerateNodes(with_label, enumerator);
		}
		void SyntaxTreeNode::EnumerateFinalNodes(INodeEnumerator * enumerator) const
		{
			if (Expands.Class != TokenClass::Unknown && !Subnodes.Length()) enumerator->RegisterNode(*this);
			else for (int i = 0; i < Subnodes.Length(); i++) Subnodes[i].EnumerateFinalNodes(enumerator);
		}
		void SyntaxTreeNode::CollectSequence(Array<Token>& to) const
		{
			if (Expands.Class != TokenClass::Unknown && !Subnodes.Length()) to << Expands;
			else for (int i = 0; i < Subnodes.Length(); i++) Subnodes[i].CollectSequence(to);
		}
		Array<Token>* SyntaxTreeNode::CollectSequence(void) const
		{
			SafePointer< Array<Token> > result = new Array<Token>;
			CollectSequence(*result);
			result->Retain();
			return result;
		}

		namespace SyntaxParser
		{
			void ParseRule(SyntaxTreeNode & node, Grammar::GrammarRule & rule, Grammar & grammar, const Array<Token> & stream, int & position)
			{
				if (!rule.CanBeginWith.Length()) rule.BuildBeginnings(grammar);
				if (rule.Class == Grammar::GrammarRule::RuleClass::Sequence) {

				} else if (rule.Class == Grammar::GrammarRule::RuleClass::Variant) {

				} else if (rule.Class == Grammar::GrammarRule::RuleClass::Reference) {

				} else if (rule.Class == Grammar::GrammarRule::RuleClass::Token) {

				}
				/*int repeat_count = 0;
				while (rule.IsPossibleBeginning(stream[position]) && (rule.MaxRepeat <= 0 || repeat_count < rule.MaxRepeat)) {
					repeat_count++;
				}
				if (repeat_count < rule.MinRepeat) throw ParserSyntaxException(position, L"Token repeat is too short.");*/
			}
		}

		SyntaxTree::SyntaxTree(const Array<Token>& token_stream, Grammar & grammar)
		{
			int position = 0;
			SyntaxParser::ParseRule(Root, *grammar.Rules[grammar.EntranceRule], grammar, token_stream, position);
		}
		SyntaxTree::~SyntaxTree(void) {}
	}
}