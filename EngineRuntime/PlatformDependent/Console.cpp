#include "Console.h"

#include <Windows.h>

namespace Engine
{
    namespace IO
    {
		Console::Console(void) : writer(new Streaming::FileStream(GetStandartOutput())), file(GetStandartOutput()) { CONSOLE_SCREEN_BUFFER_INFO info; GetConsoleScreenBufferInfo(file, &info); attr = info.wAttributes; }
        Console::Console(handle output) : writer(new Streaming::FileStream(output)), file(output) { CONSOLE_SCREEN_BUFFER_INFO info; GetConsoleScreenBufferInfo(file, &info); attr = info.wAttributes; }
        Console::~Console(void) {}
        void Console::Write(const string & text) const { writer.Write(text); }
        void Console::WriteLine(const string & text) const { writer.WriteLine(text); }
        void Console::WriteEncodingSignature(void) const { writer.WriteEncodingSignature(); }
        string Console::ToString(void) const { return L"Console"; }

        Streaming::TextWriter & Console::GetWriter(void) { return writer; }
        void Console::SetTextColor(int color)
        {
			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo(file, &info);
			if (color >= 0 && color <= 15) {
				SetConsoleTextAttribute(file, (info.wAttributes & 0xF0) | uint16(color));
			} else {
				SetConsoleTextAttribute(file, (info.wAttributes & 0xF0) | (attr & 0x0F));
			}
        }
        void Console::SetBackgroundColor(int color)
        {
			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo(file, &info);
			if (color >= 0 && color <= 15) {
				SetConsoleTextAttribute(file, (info.wAttributes & 0x0F) | (uint16(color) << 4));
			} else {
				SetConsoleTextAttribute(file, (info.wAttributes & 0x0F) | (attr & 0xF0));
			}
        }
        void Console::ClearScreen(void)
        {
			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo(file, &info);
			DWORD Written; COORD Begin; Begin.X = Begin.Y = 0;
			FillConsoleOutputCharacterW(file, L' ', info.dwSize.X * info.dwSize.Y, Begin, &Written);
			FillConsoleOutputAttribute(file, info.wAttributes, info.dwSize.X * info.dwSize.Y, Begin, &Written);
			SetConsoleCursorPosition(file, Begin);
        }
        void Console::ClearLine(void)
        {
			CONSOLE_SCREEN_BUFFER_INFO info;
			GetConsoleScreenBufferInfo(file, &info);
			DWORD Written; COORD Begin; Begin.X = 0; Begin.Y = info.dwCursorPosition.Y;
			FillConsoleOutputCharacterW(file, L' ', info.dwSize.X, Begin, &Written);
			FillConsoleOutputAttribute(file, info.wAttributes, info.dwSize.X, Begin, &Written);
			SetConsoleCursorPosition(file, Begin);
        }
        void Console::MoveCaret(int x, int y)
        {
			COORD pos;
			pos.X = x; pos.Y = y;
			SetConsoleCursorPosition(file, pos);
        }
		void Console::SetTitle(const string & title) { SetConsoleTitleW(title); }

        Console & Console::operator << (const string & text) { writer << text; return *this; }
        const Console & Console::operator << (const string & text) const { writer << text; return *this; }
    }
}