#pragma once

#include "../EngineBase.h"
#include "../Miscellaneous/Dictionary.h"
#include "Tokenization.h"

namespace Engine
{
	namespace Syntax
	{
		class ParserSyntaxException : public Exception { public: int Position; string Comments; ParserSyntaxException(int At, const string & About); string ToString(void) const override; };

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
				// If Class is Variant, then this rule represents an appearance of one of the rules from Rules array.
				// If Class is Sequence, then this rule represents a strict sequence of the rules from array.
				// If Class is Reference, then this rule should be replaced with another rule from the grammar with name from Reference.
				// If Class is Token, then this rule represents an appearance of token, specified in Token.

				// Label is technical identifier, attached to this rule.
				// Repeat counts specify minimal and maximal repeat count of this rule in the text.

				// CanBeginWith is a list of all possible first tokens this rule expands to.
				// This list is empty by default. User routines should build it manually by calling BuildBeginnings().
				// Syntax parser builds this list automatically on demand. Do not rely on it.

				RuleClass Class;
				Array<GrammarRule> Rules;
				string Reference;
				Token TokenClass;
				int MinRepeat = 1;
				int MaxRepeat = 1;
				string Label;

				Array<Token> CanBeginWith;

				GrammarRule(void);
				GrammarRule(const GrammarRule & src);

				static GrammarRule VariantRule(const string & label, int min_repeat = 1, int max_repeat = 1);
				static GrammarRule SequenceRule(const string & label, int min_repeat = 1, int max_repeat = 1);
				static GrammarRule ReferenceRule(const string & label, const string & another_rule, int min_repeat = 1, int max_repeat = 1);
				static GrammarRule TokenRule(const string & label, const Token & token, int min_repeat = 1, int max_repeat = 1);

				bool CanBeEmpty(void) const;
				bool IsPossibleBeginning(const Token & token) const;

				void BuildBeginnings(Grammar & grammar);
			};

			Dictionary::Dictionary<string, GrammarRule> Rules;
			string EntranceRule;

			Grammar(void);
		};
		class SyntaxTreeNode;
		class INodeEnumerator
		{
		public:
			virtual void RegisterNode(const SyntaxTreeNode & node) = 0;
		};
		class SyntaxTreeNode
		{
		public:
			string Label;
			Array<SyntaxTreeNode> Subnodes;
			Token Expands;

			SyntaxTreeNode(void);

			void EnumerateNodes(INodeEnumerator * enumerator) const;
			void EnumerateNodes(const string & with_label, INodeEnumerator * enumerator) const;
			void EnumerateFinalNodes(INodeEnumerator * enumerator) const;
			void CollectSequence(Array<Token> & to) const;
			Array<Token> * CollectSequence(void) const;
		};
		class SyntaxTree : public Object
		{
		public:
			SyntaxTreeNode Root;

			SyntaxTree(const Array<Token> & token_stream, Grammar & grammar);
			~SyntaxTree(void) override;
		};
	}
}