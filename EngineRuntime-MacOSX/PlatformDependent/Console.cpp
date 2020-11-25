#include "Console.h"

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

namespace Engine
{
	namespace IO
	{
		Console::Console(void) : writer(new Streaming::FileStream(GetStandardOutput())), file(GetStandardOutput()), reader(new Streaming::FileStream(GetStandardInput()), Encoding::UTF8), file_in(GetStandardInput())
		{ chars = new Array<uint32>(0x100); alt_buffer = false; }
		Console::Console(handle output) : writer(new Streaming::FileStream(output)), file(output), reader(new Streaming::FileStream(GetStandardInput()), Encoding::UTF8), file_in(GetStandardInput())
		{ chars = new Array<uint32>(0x100); alt_buffer = false; }
		Console::Console(handle output, handle input) : writer(new Streaming::FileStream(output)), file(output), reader(new Streaming::FileStream(input), Encoding::UTF8), file_in(input)
		{ chars = new Array<uint32>(0x100); alt_buffer = false; }
		Console::~Console(void) {}
		void Console::Write(const string & text) const { writer.Write(text); }
		void Console::WriteLine(const string & text) const { if (iostat) { writer.Write(text); writer.Write(L"\n\r"); } else writer.WriteLine(text); }
		void Console::WriteEncodingSignature(void) const { writer.WriteEncodingSignature(); }
		string Console::ToString(void) const { return L"Console"; }

		void Console::SetTextColor(int color)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				string val;
				if (color == -1) val = L"\e[39m"; // revert
				else if (color == 0) val = L"\e[38;5;0m"; // black
				else if (color == 1) val = L"\e[38;5;4m"; // dark blue
				else if (color == 2) val = L"\e[38;5;2m"; // dark green
				else if (color == 3) val = L"\e[38;5;6m"; // dark cyan
				else if (color == 4) val = L"\e[38;5;1m"; // dark red
				else if (color == 5) val = L"\e[38;5;5m"; // dark magenta
				else if (color == 6) val = L"\e[38;5;3m"; // dark yellow
				else if (color == 7) val = L"\e[38;5;7m"; // grey
				else if (color == 8) val = L"\e[38;5;8m"; // dark grey
				else if (color == 9) val = L"\e[38;5;12m"; // blue
				else if (color == 10) val = L"\e[38;5;10m"; // green
				else if (color == 11) val = L"\e[38;5;14m"; // cyan
				else if (color == 12) val = L"\e[38;5;9m"; // red
				else if (color == 13) val = L"\e[38;5;13m"; // magenta
				else if (color == 14) val = L"\e[38;5;11m"; // yellow
				else if (color == 15) val = L"\e[38;5;15m"; // white
				Write(val);
			}
		}
		void Console::SetBackgroundColor(int color)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				string val;
				if (color == -1) val = L"\e[49m"; // revert
				else if (color == 0) val = L"\e[48;5;0m"; // black
				else if (color == 1) val = L"\e[48;5;4m"; // dark blue
				else if (color == 2) val = L"\e[48;5;2m"; // dark green
				else if (color == 3) val = L"\e[48;5;6m"; // dark cyan
				else if (color == 4) val = L"\e[48;5;1m"; // dark red
				else if (color == 5) val = L"\e[48;5;5m"; // dark magenta
				else if (color == 6) val = L"\e[48;5;3m"; // dark yellow
				else if (color == 7) val = L"\e[48;5;7m"; // gray
				else if (color == 8) val = L"\e[48;5;8m"; // dark gray
				else if (color == 9) val = L"\e[48;5;12m"; // blue
				else if (color == 10) val = L"\e[48;5;10m"; // green
				else if (color == 11) val = L"\e[48;5;14m"; // cyan
				else if (color == 12) val = L"\e[48;5;9m"; // red
				else if (color == 13) val = L"\e[48;5;13m"; // magenta
				else if (color == 14) val = L"\e[48;5;11m"; // yellow
				else if (color == 15) val = L"\e[48;5;15m"; // white
				Write(val);
			}
		}
		void Console::ClearScreen(void)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				Write(L"\e[2J\e[1;1H");
			}
		}
		void Console::ClearLine(void)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				Write(L"\e[2K\e[1G");
			}
		}
		void Console::MoveCaret(int x, int y)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				Write(L"\e[" + string(y + 1) + L";" + string(x + 1) + L"H");
			}
		}
		void Console::SetTitle(const string & title)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				string val = L"\e]0;" + title + L"\a";
				Write(val);
			}
		}
		void Console::SetInputMode(InputMode mode)
		{
			if (isatty(reinterpret_cast<intptr>(file_in))) {
				tcdrain(isatty(reinterpret_cast<intptr>(file)));
				auto in = reinterpret_cast<intptr>(file_in);
				if (mode == InputMode::Echo) {
					if (iostat) {
						termios tio;
						MemoryCopy(&tio, iostat->GetBuffer(), sizeof(tio));
						tcflush(in, TCIOFLUSH);
						tcsetattr(in, TCSANOW, &tio);
						iostat.SetReference(0);
					}
				} else if (mode == InputMode::Raw) {
					if (!iostat) {
						termios old_tio, tio;
						tcflush(in, TCIOFLUSH);
						tcgetattr(in, &tio);
						tcgetattr(in, &old_tio);
						cfmakeraw(&tio);
						tcsetattr(in, TCSANOW, &tio);
						iostat = new DataBlock(0x100);
						iostat->SetLength(sizeof(old_tio));
						MemoryCopy(iostat->GetBuffer(), &old_tio, sizeof(old_tio));
					}
				}
			}
		}
		void Console::AlternateScreenBuffer(bool alternate)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				tcdrain(isatty(reinterpret_cast<intptr>(file)));
				if (alternate) {
					if (!alt_buffer) {
						Write(L"\e[?1049h");
						alt_buffer = true;
					}
				} else {
					if (alt_buffer) {
						Write(L"\e[?1049l");
						alt_buffer = false;
					}
				}
			}
		}
		uint32 Console::ReadChar(void) const
		{
			if (chars->Length()) {
				auto chr = chars->FirstElement();
				chars->RemoveFirst();
				return chr;
			} else return reader.ReadChar();
		}
		void Console::GetCaretPosition(int & x, int & y)
		{
			if (isatty(reinterpret_cast<intptr>(file)) && isatty(reinterpret_cast<intptr>(file_in))) {
				tcdrain(isatty(reinterpret_cast<intptr>(file)));
				auto in = reinterpret_cast<intptr>(file_in);
				termios old_tio, tio;
				tcflush(in, TCIOFLUSH);
				tcgetattr(in, &tio);
				tcgetattr(in, &old_tio);
				cfmakeraw(&tio);
				tcsetattr(in, TCSANOW, &tio);
				Write(L"\e[6n");
				while (true) {
					auto chr = ReadChar();
					if (chr == 27) {
						Array<uint32> cmd(0x10);
						while (chr != L'R' && chr != 0xFFFFFFFF) { cmd << chr; chr = ReadChar(); }
						if (cmd.Length() < 3) { x = y = 0; tcsetattr(in, TCSANOW, &old_tio); return; }
						string conv = string(cmd.GetBuffer() + 2, cmd.Length() - 2, Encoding::UTF32);
						int sep = conv.FindFirst(L';');
						if (sep == -1) { x = y = 0; tcsetattr(in, TCSANOW, &old_tio); return; }
						try {
							y = conv.Fragment(0, sep).ToInt32() - 1;
							x = conv.Fragment(sep + 1, -1).ToInt32() - 1;
						} catch (...) { x = y = 0; }
						tcsetattr(in, TCSANOW, &old_tio);
						return;
					} else if (chr == 0xFFFFFFFF) { x = y = 0; tcsetattr(in, TCSANOW, &old_tio); return; }
					else chars->Append(chr);
				}
			} else { x = y = 0; }
		}
		void Console::GetScreenBufferDimensions(int & w, int & h)
		{
			if (isatty(reinterpret_cast<intptr>(file))) {
				auto out = reinterpret_cast<intptr>(file);
				winsize size;
				ioctl(out, TIOCGWINSZ, &size);
				w = size.ws_col;
				h = size.ws_row;
			} else { w = h = 0; }
		}
		bool Console::IsConsoleDevice(void) { return isatty(reinterpret_cast<intptr>(file)); }

		Console & Console::operator<<(const string & text) { Write(text); return *this; }
		Console & Console::operator<<(ControlToken token)
		{
			auto cmd = token.value & 0xFFFF0000;
			if (cmd == 0) {
				LineFeed();
			} else if (cmd == 0x00010000) {
				auto clr = token.value & 0xFFFF;
				if (clr == 0xFFFF) SetTextColor(-1);
				else SetTextColor(clr);
			} else if (cmd == 0x00020000) {
				auto clr = token.value & 0xFFFF;
				if (clr == 0xFFFF) SetBackgroundColor(-1);
				else SetBackgroundColor(clr);
			}
			return *this;
		}
		Console & Console::operator>>(string & str) { str = ReadLine(); return *this; }
		ControlToken::ControlToken(uint32 arg) : value(arg) {}

		namespace ConsoleControl
		{
			ControlToken LineFeed(void) { return ControlToken(0); }
			ControlToken TextColor(int color) { return ControlToken(0x00010000 | color); }
			ControlToken TextColorDefault(void) { return ControlToken(0x0001FFFF); }
			ControlToken TextBackground(int color) { return ControlToken(0x00020000 | color); }
			ControlToken TextBackgroundDefault(void) { return ControlToken(0x0002FFFF); }
		}
	}
}