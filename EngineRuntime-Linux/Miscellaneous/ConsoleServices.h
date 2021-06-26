#pragma once

#include "../Interfaces/Console.h"

namespace Engine
{
	namespace IO
	{
		namespace ConsoleControl
		{
			class ConsoleControlToken
			{
			public:
				uint32 Value;
				explicit ConsoleControlToken(uint32 arg);
			};
			ConsoleControlToken LineFeed(void);
			ConsoleControlToken TextColor(int color);
			ConsoleControlToken TextColor(ConsoleColor color);
			ConsoleControlToken TextColorDefault(void);
			ConsoleControlToken TextBackground(int color);
			ConsoleControlToken TextBackground(ConsoleColor color);
			ConsoleControlToken TextBackgroundDefault(void);
		}
		const Console & operator << (const Console & console, const string & text);
		const Console & operator << (const Console & console, ConsoleControl::ConsoleControlToken token);
		const Console & operator >> (const Console & console, uint32 & code);
		const Console & operator >> (const Console & console, string & text);
		const Console & operator >> (const Console & console, ConsoleEventDesc & desc);
	}
}