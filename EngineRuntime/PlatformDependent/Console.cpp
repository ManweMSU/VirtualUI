#include "Console.h"

#include <Windows.h>

namespace Engine
{
	namespace IO
	{
		Console::Console(void) : writer(new Streaming::FileStream(GetStandardOutput()), Encoding::UTF8), file(GetStandardOutput()), reader(new Streaming::FileStream(GetStandardInput()), Encoding::UTF8), file_in(GetStandardInput()), main_buffer(INVALID_HANDLE_VALUE)
		{
			if (file == INVALID_HANDLE_VALUE || !file) throw FileAccessException(Error::InvalidHandle);
			DWORD mode;
			if (GetConsoleMode(file, &mode)) {
				is_true_console = true;
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				attr = info.wAttributes;
			} else {
				is_true_console = false;
				attr = 0;
			}
			if (file_in == INVALID_HANDLE_VALUE || !file_in) console_in_mode = 0;
			else if (GetConsoleMode(file_in, &mode)) console_in_mode = 1;
			else console_in_mode = 2;
		}
		Console::Console(handle output) : writer(new Streaming::FileStream(output), Encoding::UTF8), file(output), reader(new Streaming::FileStream(GetStandardInput()), Encoding::UTF8), file_in(GetStandardInput()), main_buffer(INVALID_HANDLE_VALUE)
		{
			if (file == INVALID_HANDLE_VALUE || !file) throw FileAccessException(Error::InvalidHandle);
			DWORD mode;
			if (GetConsoleMode(file, &mode)) {
				is_true_console = true;
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				attr = info.wAttributes;
			} else {
				is_true_console = false;
				attr = 0;
			}
			if (file_in == INVALID_HANDLE_VALUE || !file_in) console_in_mode = 0;
			else if (GetConsoleMode(file_in, &mode)) console_in_mode = 1;
			else console_in_mode = 2;
		}
		Console::Console(handle output, handle input) : writer(new Streaming::FileStream(output), Encoding::UTF8), file(output), reader(new Streaming::FileStream(input), Encoding::UTF8), file_in(input), main_buffer(INVALID_HANDLE_VALUE)
		{
			if (file == INVALID_HANDLE_VALUE || !file) throw FileAccessException(Error::InvalidHandle);
			DWORD mode;
			if (GetConsoleMode(file, &mode)) {
				is_true_console = true;
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				attr = info.wAttributes;
			} else {
				is_true_console = false;
				attr = 0;
			}
			if (file_in == INVALID_HANDLE_VALUE || !file_in) console_in_mode = 0;
			else if (GetConsoleMode(file_in, &mode)) console_in_mode = 1;
			else console_in_mode = 2;
		}
		Console::~Console(void) {}
		void Console::Write(const string & text) const
		{
			if (is_true_console) WriteConsoleW(file, text, text.Length(), 0, 0);
			else writer.Write(text);
		}
		void Console::WriteLine(const string & text) const
		{
			if (is_true_console) {
				WriteConsoleW(file, text, text.Length(), 0, 0);
				WriteConsoleW(file, L"\n", 1, 0, 0);
			} else writer.WriteLine(text);
		}
		void Console::WriteEncodingSignature(void) const
		{
			if (!is_true_console) writer.WriteEncodingSignature();
		}
		string Console::ToString(void) const { return L"Console"; }

		void Console::SetTextColor(int color)
		{
			if (is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				if (color >= 0 && color <= 15) {
					SetConsoleTextAttribute(file, (info.wAttributes & 0xF0) | uint16(color));
				} else {
					SetConsoleTextAttribute(file, (info.wAttributes & 0xF0) | (attr & 0x0F));
				}
			}
		}
		void Console::SetBackgroundColor(int color)
		{
			if (is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				if (color >= 0 && color <= 15) {
					SetConsoleTextAttribute(file, (info.wAttributes & 0x0F) | (uint16(color) << 4));
				} else {
					SetConsoleTextAttribute(file, (info.wAttributes & 0x0F) | (attr & 0xF0));
				}
			}
		}
		void Console::ClearScreen(void)
		{
			if (is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				DWORD Written; COORD Begin; Begin.X = Begin.Y = 0;
				FillConsoleOutputCharacterW(file, L' ', info.dwSize.X * info.dwSize.Y, Begin, &Written);
				FillConsoleOutputAttribute(file, info.wAttributes, info.dwSize.X * info.dwSize.Y, Begin, &Written);
				SetConsoleCursorPosition(file, Begin);
			}
		}
		void Console::ClearLine(void)
		{
			if (is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				DWORD Written; COORD Begin; Begin.X = 0; Begin.Y = info.dwCursorPosition.Y;
				FillConsoleOutputCharacterW(file, L' ', info.dwSize.X, Begin, &Written);
				FillConsoleOutputAttribute(file, info.wAttributes, info.dwSize.X, Begin, &Written);
				SetConsoleCursorPosition(file, Begin);
			}
		}
		void Console::MoveCaret(int x, int y)
		{
			if (is_true_console) {
				COORD pos;
				pos.X = x; pos.Y = y;
				SetConsoleCursorPosition(file, pos);
			}
		}
		void Console::SetTitle(const string & title) { SetConsoleTitleW(title); }
		void Console::SetInputMode(InputMode mode)
		{
			if (console_in_mode == 1) {
				DWORD mode_var;
				GetConsoleMode(file_in, &mode_var);
				if (mode == InputMode::Echo) {
					mode_var |= ENABLE_ECHO_INPUT;
					mode_var |= ENABLE_LINE_INPUT;
				} else if (mode == InputMode::Raw) {
					mode_var &= ~ENABLE_ECHO_INPUT;
					mode_var &= ~ENABLE_LINE_INPUT;
				}
				FlushConsoleInputBuffer(file_in);
				SetConsoleMode(file_in, mode_var);
			}
		}
		void Console::AlternateScreenBuffer(bool alternate)
		{
			if (is_true_console) {
				if (alternate) {
					if (main_buffer == INVALID_HANDLE_VALUE) {
						main_buffer = file;
						file = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CONSOLE_TEXTMODE_BUFFER, 0);
						SetConsoleActiveScreenBuffer(file);
					}
				} else {
					if (main_buffer != INVALID_HANDLE_VALUE) {
						SetConsoleActiveScreenBuffer(main_buffer);
						CloseHandle(file);
						file = main_buffer;
						main_buffer = INVALID_HANDLE_VALUE;
					}
				}
			}
		}
		uint32 Console::ReadChar(void) const
		{
			if (console_in_mode) {
				if (console_in_mode == 1) {
					DWORD read;
					uint32 code = 0;
					widechar chr;
					if (!ReadConsoleW(file_in, &chr, 1, &read, 0)) throw FileAccessException(Error::ReadFailure);
					if (read == 1) {
						if ((chr & 0xFC00) == 0xD800) {
							code = chr & 0x03FF;
							code <<= 10;
							if (!ReadConsoleW(file_in, &chr, 1, &read, 0)) throw FileAccessException(Error::ReadFailure);
							if (read == 0) { eof = true; return 0xFFFFFFFF; }
							code |= (chr & 0x3FF);
							code += 0x10000;
							return code;
						} else return chr;
					} else { eof = true; return 0xFFFFFFFF; }
				} else {
					auto chr = reader.ReadChar();
					eof = reader.EofReached();
					return chr;
				}
			} else throw FileAccessException(Error::NotImplemented);
		}
		void Console::GetCaretPosition(int & x, int & y)
		{
			if (is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				x = info.dwCursorPosition.X;
				y = info.dwCursorPosition.Y;
			} else { x = y = 0; }
		}
		void Console::GetScreenBufferDimensions(int & w, int & h)
		{
			if (is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				GetConsoleScreenBufferInfo(file, &info);
				w = info.dwSize.X;
				h = info.dwSize.Y;
			} else { w = h = 0; }
		}
		bool Console::IsConsoleDevice(void) { return is_true_console && console_in_mode == 1; }
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