#pragma once

#include "../Syntax/Grammar.h"

namespace Engine
{
	namespace Storage
	{
		void CreateTextRegistrySpelling(Syntax::Spelling & spelling);
		void CreateTextRegistryGrammar(Syntax::Grammar & grammar);
	}
}