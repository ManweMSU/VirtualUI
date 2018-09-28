#pragma once

#include "../Streaming.h"

namespace Engine
{
	namespace Assembly
	{
		extern string CurrentLocale;

		string GetCurrentUserLocale(void);
		Streaming::Stream * QueryResource(const widechar * identifier);
		Streaming::Stream * QueryLocalizedResource(const widechar * identifier);
	}
}