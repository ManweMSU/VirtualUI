#pragma once

#include "Tokenization.h"
#include "Grammar.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Mathematical expression grammar
// EXPRESSION -> MULARG { ADDOP MULARG }
// MULARG     -> OPERAND { MULOP OPERAND }
// OPERAND    -> '(' EXPRESSION ')' | [ ADDOP ] <numeric literal constant> | <identifier> | FUNCWORD '(' EXPRESSION ')'
// ADDOP      -> '+' | '-'
// MULOP      -> '*' | '/' | '%'
// FUNCWORD   -> 'sgn' | 'abs' | 'sin' | 'cos' | 'tg' | 'ctg' | 'arcsin' | 'arccos' | 'arctg' | 'arcctg' | 'ln' | 'exp' | 'sqrt'
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Engine
{
	namespace Syntax
	{
		namespace Math
		{
			class IVariableProvider
			{
			public:
				virtual int64 GetInteger(const string & name);
				virtual double GetDouble(const string & name);
			};
			void GetLanguageInfo(Spelling & spelling, Grammar & grammar);
			int64 CalculateExpressionInteger(const string & expression, IVariableProvider * variables = 0);
			double CalculateExpressionDouble(const string & expression, IVariableProvider * variables = 0);
		}
	}
}