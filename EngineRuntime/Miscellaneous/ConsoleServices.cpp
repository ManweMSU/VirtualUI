#include "ConsoleServices.h"

namespace Engine
{
	namespace IO
	{
		namespace ConsoleControl
		{
			ConsoleControlToken::ConsoleControlToken(uint32 arg) : Value(arg) {}
			ConsoleControlToken LineFeed(void) { return ConsoleControlToken(0); }
			ConsoleControlToken TextColor(int color) { return ConsoleControlToken(0x00010000 | color); }
			ConsoleControlToken TextColor(ConsoleColor color) { return ConsoleControlToken(0x00010000 | uint16(color)); }
			ConsoleControlToken TextColorDefault(void) { return ConsoleControlToken(0x0001FFFF); }
			ConsoleControlToken TextBackground(int color) { return ConsoleControlToken(0x00020000 | color); }
			ConsoleControlToken TextBackground(ConsoleColor color) { return ConsoleControlToken(0x00020000 | uint16(color)); }
			ConsoleControlToken TextBackgroundDefault(void) { return ConsoleControlToken(0x0002FFFF); }
		}
		const Console & operator<<(const Console & console, const string & text) { console.Write(text); return console; }
		const Console & operator<<(const Console & console, ConsoleControl::ConsoleControlToken token)
		{
			auto cmd = token.Value & 0xFFFF0000;
			if (cmd == 0) {
				console.LineFeed();
			} else if (cmd == 0x00010000) {
				auto clr = token.Value & 0xFFFF;
				if (clr == 0xFFFF) console.SetTextColor(-1);
				else console.SetTextColor(clr);
			} else if (cmd == 0x00020000) {
				auto clr = token.Value & 0xFFFF;
				if (clr == 0xFFFF) console.SetBackgroundColor(-1);
				else console.SetBackgroundColor(clr);
			}
			return console;
		}
		const Console & operator>>(const Console & console, uint32 & code) { code = console.ReadChar(); return console; }
		const Console & operator>>(const Console & console, string & text) { text = console.ReadLine(); return console; }
		const Console & operator>>(const Console & console, ConsoleEventDesc & desc) { console.ReadEvent(desc); return console; }
	}
}