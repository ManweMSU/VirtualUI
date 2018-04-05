#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Syntax
	{
		bool MatchFilePattern(const string & path, const string & filter);
	}
}