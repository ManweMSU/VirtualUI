#include "Grammar.h"

namespace Engine
{
	namespace Syntax
	{
		ParserSyntaxException::ParserSyntaxException(int At, const string & About) : Position(At), Comments(About) {}
		string ParserSyntaxException::ToString(void) const { return L"ParserSyntaxException: " + Comments + L" at token " + string(Position); }

		Grammar::GrammarRule::GrammarRule(void) : Rules(0x20), CanBeginWith(0x10) {}
		Grammar::GrammarRule::GrammarRule(const GrammarRule & src) : Class(src.Class), Rules(src.Rules), CanBeginWith(0x10), Reference(src.Reference), TokenClass(src.TokenClass), MinRepeat(src.MinRepeat), MaxRepeat(src.MaxRepeat), Label(src.Label) {}
		Grammar::GrammarRule Grammar::GrammarRule::VariantRule(const string & label, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Variant;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			result.Label = label;
			return result;
		}
		Grammar::GrammarRule Grammar::GrammarRule::SequenceRule(const string & label, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Sequence;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			result.Label = label;
			return result;
		}
		Grammar::GrammarRule Grammar::GrammarRule::ReferenceRule(const string & label, const string & another_rule, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Reference;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			result.Reference = another_rule;
			result.Label = label;
			return result;
		}
		Grammar::GrammarRule Grammar::GrammarRule::TokenRule(const string & label, const Token & token, int min_repeat, int max_repeat)
		{
			GrammarRule result;
			result.Class = RuleClass::Token;
			result.MinRepeat = min_repeat;
			result.MaxRepeat = max_repeat;
			result.TokenClass = token;
			result.Label = label;
			return result;
		}
		bool Grammar::GrammarRule::CanBeEmpty(void) const
		{
			for (int i = 0; i < CanBeginWith.Length(); i++) if (CanBeginWith[i].IsVoid()) return true;
			return false;
		}
		bool Grammar::GrammarRule::IsPossibleBeginning(const Token & token) const
		{
			for (int i = 0; i < CanBeginWith.Length(); i++) {
				if (CanBeginWith[i].IsVoid()) return true;
				if (CanBeginWith[i].IsSimilar(token)) return true;
			}
			return false;
		}
		void Grammar::GrammarRule::BuildBeginnings(Grammar & grammar)
		{
			CanBeginWith.Clear();
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
						if (Rules[i].CanBeginWith[j].IsVoid() || Rules[i].MinRepeat == 0) CanBeEmpty = true;
						if (!Rules[i].CanBeginWith[j].IsVoid() || j == Rules[i].CanBeginWith.Length() - 1) {
							bool present = false;
							for (int k = 0; k < CanBeginWith.Length(); k++) {
								if (CanBeginWith[k] == Rules[i].CanBeginWith[j]) { present = true; break; }
							}
							if (!present) CanBeginWith << Rules[i].CanBeginWith[j];
						}
					}
					if (!CanBeEmpty) break;
					if (i == Rules.Length() - 1) {
						bool present = false;
						for (int k = 0; k < CanBeginWith.Length(); k++) {
							if (CanBeginWith[k].IsVoid()) { present = true; break; }
						}
						if (!present) CanBeginWith << Token::VoidToken();
					}
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

		void SyntaxTreeNode::EnumerateNodes(INodeEnumerator * enumerator)
		{
			enumerator->RegisterNode(*this);
			for (int i = 0; i < Subnodes.Length(); i++) Subnodes[i].EnumerateNodes(enumerator);
		}
		void SyntaxTreeNode::EnumerateNodes(const string & with_label, INodeEnumerator * enumerator)
		{
			if (Label == with_label) enumerator->RegisterNode(*this);
			for (int i = 0; i < Subnodes.Length(); i++) Subnodes[i].EnumerateNodes(with_label, enumerator);
		}
		void SyntaxTreeNode::EnumerateFinalNodes(INodeEnumerator * enumerator)
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
		bool SyntaxTreeNode::IsUnlabeledNode(void) const { return !Label.Length() && Expands.Class == TokenClass::Unknown; }
		void SyntaxTreeNode::OptimizeNode(void)
		{
			for (int i = 0; i < Subnodes.Length(); i++) {
				if (Subnodes[i].IsUnlabeledNode()) {
					for (int j = Subnodes[i].Subnodes.Length() - 1; j >= 0; j--) {
						Subnodes.Insert(Subnodes[i].Subnodes[j], i + 1);
					}
					Subnodes.Remove(i);
					i--;
				}
			}
			for (int i = 0; i < Subnodes.Length(); i++) Subnodes[i].OptimizeNode();
		}

		namespace SyntaxParser
		{
			void ParseRule(SyntaxTreeNode & node, Grammar::GrammarRule & rule, Grammar & grammar, const Array<Token> & stream, int & position)
			{
				if (!rule.CanBeginWith.Length()) rule.BuildBeginnings(grammar);
				node.Label = rule.Label;
				if (rule.Class == Grammar::GrammarRule::RuleClass::Sequence) {
					int repeat = 0;
					while (repeat < rule.MaxRepeat || rule.MaxRepeat <= 0) {
						if (rule.IsPossibleBeginning(stream[position])) {
							repeat++;
							for (int i = 0; i < rule.Rules.Length(); i++) {
								node.Subnodes << SyntaxTreeNode();
								ParseRule(node.Subnodes.LastElement(), rule.Rules[i], grammar, stream, position);
							}
						} else break;
					}
					if (repeat < rule.MinRepeat) throw ParserSyntaxException(position, L"Another token expected");
				} else if (rule.Class == Grammar::GrammarRule::RuleClass::Variant) {
					int repeat = 0;
					while (repeat < rule.MaxRepeat || rule.MaxRepeat <= 0) {
						if (rule.IsPossibleBeginning(stream[position])) {
							repeat++;
							bool found = false;
							for (int i = 0; i < rule.Rules.Length(); i++) {
								if (rule.Rules[i].IsPossibleBeginning(stream[position])) {
									node.Subnodes << SyntaxTreeNode();
									ParseRule(node.Subnodes.LastElement(), rule.Rules[i], grammar, stream, position);
									found = true;
									break;
								}
							}
							if (!found) throw ParserSyntaxException(position, L"Invalid variant token");
						} else break;
					}
					if (repeat < rule.MinRepeat) throw ParserSyntaxException(position, L"Another token expected");
				} else if (rule.Class == Grammar::GrammarRule::RuleClass::Reference) {
					int repeat = 0;
					while (repeat < rule.MaxRepeat || rule.MaxRepeat <= 0) {
						if (rule.IsPossibleBeginning(stream[position])) {
							repeat++;
							auto replace = grammar.Rules[rule.Reference];
							node.Subnodes << SyntaxTreeNode();
							ParseRule(node.Subnodes.LastElement(), *replace, grammar, stream, position);
						} else break;
					}
					if (repeat < rule.MinRepeat) throw ParserSyntaxException(position, L"Another token expected");
				} else if (rule.Class == Grammar::GrammarRule::RuleClass::Token) {
					int repeat = 0;
					while (repeat < rule.MaxRepeat || rule.MaxRepeat <= 0) {
						if (rule.IsPossibleBeginning(stream[position])) {
							repeat++;
							node.Expands = stream[position];
							position++;
						} else break;
					}
					if (repeat < rule.MinRepeat) throw ParserSyntaxException(position, L"Another token expected");
				}
			}
		}

		SyntaxTree::SyntaxTree(const Array<Token>& token_stream, Grammar & grammar)
		{
			int position = 0;
			auto entrance = grammar.Rules[grammar.EntranceRule];
			if (!entrance) throw Exception();
			SyntaxParser::ParseRule(Root, *entrance, grammar, token_stream, position);
			if (token_stream[position].Class != TokenClass::EndOfStream) throw ParserSyntaxException(position, L"End-of-stream expected");
		}
		SyntaxTree::~SyntaxTree(void) {}
	}
}